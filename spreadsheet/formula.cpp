#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <forward_list>
#include <regex>


using namespace std::literals;

FormulaError::FormulaError(FormulaError::Category category)
    : category_(category) {
}

FormulaError::Category FormulaError::GetCategory() const {
    return category_;
}

bool FormulaError::operator==(FormulaError rhs) const {
    return category_ == rhs.category_;
}

std::string_view FormulaError::ToString() const {
    switch (category_) {
        case FormulaError::Category::Ref :
            return "#REF!"sv;
        case FormulaError::Category::Value :
            return "#VALUE!"sv;
        case FormulaError::Category::Div0 :
            return "#DIV/0!"sv;
        default :
            assert(false);
            return ""sv;
    }
}

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

namespace {
class Formula : public FormulaInterface {
public:
    explicit Formula(std::string expression)
        : ast_(ParseFormulaAST(std::move(expression))), referenced_cells_(ast_.GetCells().begin(), ast_.GetCells().end()) {
        
        auto last = std::unique(referenced_cells_.begin(), referenced_cells_.end());
        referenced_cells_.erase(last, referenced_cells_.end());
        std::sort(referenced_cells_.begin(), referenced_cells_.end());
    }

    Value Evaluate(const SheetInterface& sheet) const override {
        try {
            auto get_value = [&sheet](const Position& pos) {
                if (auto* cell = sheet.GetCell(pos)) {
                    CellInterface::Value value = cell->GetValue();

                    if (std::holds_alternative<double>(value)) {
                        return std::get<double>(value);
                    } else if (std::holds_alternative<FormulaError>(value)) {
                        throw std::get<FormulaError>(value);
                    } else if (std::holds_alternative<std::string>(value)) {
                        std::string text = std::get<std::string>(value);
                        if (text.empty()) {
                            return 0.;
                        }
                        try {
                            static const std::regex kDoubleValuePattern("^(-?)(0|([1-9][0-9]*))(.[0-9]+)?$");
                            std::smatch match;
                            if (std::regex_match(text.cbegin(), text.cend(), match, kDoubleValuePattern)) {
                                return std::stod(text);
                            }
                            throw std::runtime_error(""); // if string could not be converted to double
                        } catch (...) {
                            throw FormulaError(FormulaError::Category::Value);
                        }
                    }
                }
                return 0.; // if cell is out of boundaries or empty
            };
            return ast_.Execute(get_value);
        } catch (FormulaError& err) {
            return err;
        }
    }

    std::string GetExpression() const override {
        std::stringstream ss;
        ast_.PrintFormula(ss);

        return ss.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        return referenced_cells_;
    }

private:
    FormulaAST ast_;
    std::vector<Position> referenced_cells_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {
       return std::make_unique<Formula>(std::move(expression));
    } catch (std::exception& exc) {
       // std::throw FormulaException(exc.what());
       std::throw_with_nested(FormulaException(exc.what()));
    }
}
