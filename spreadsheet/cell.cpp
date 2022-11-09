#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <string>
#include <stack>

using namespace std::literals;

class Cell::Impl {
public:
    virtual ~Impl() = default;

    virtual Value GetValue() const = 0;

    virtual std::string GetText() const = 0;

    virtual std::vector<Position> GetReferencedCells() const {
        return { };
    }

    virtual void InvalidateCache() { }
};

class Cell::EmptyImpl : public Impl {
public:
    Value GetValue() const override {
        return "";
    }

    std::string GetText() const override {
        return "";
    }
};

class Cell::TextImpl : public Impl {  
public:
    TextImpl(std::string text)
        : text_(std::move(text)) {
    }

    Value GetValue() const override {
        if (text_.at(0) == ESCAPE_SIGN) {
            return text_.substr(1);
        }
        return text_;
    }

    std::string GetText() const override {
        return text_;
    }
private:
    std::string text_;
};

class Cell::FormulaImpl : public Impl {
public:
    explicit FormulaImpl(std::string text, const SheetInterface& sheet)
        : formula_(ParseFormula(text.substr(1))), sheet_(sheet) {
    }

    Value GetValue() const override {
        if (!cached_value_) {
            cached_value_ = formula_->Evaluate(sheet_);
        }
        if (std::holds_alternative<double>(*cached_value_)) {
            return std::get<double>(*cached_value_);
        }
        return std::get<FormulaError>(*cached_value_);
    }

    std::string GetText() const override {
        return FORMULA_SIGN + formula_->GetExpression();
    }

    std::vector<Position> GetReferencedCells() const override {
        return formula_->GetReferencedCells();
    }

    void InvalidateCache() override {
        cached_value_.reset();
    }

private:
    std::unique_ptr<FormulaInterface> formula_;
    const SheetInterface& sheet_;
    mutable std::optional<FormulaInterface::Value> cached_value_;
};

Cell::Cell(Sheet& sheet)
    : sheet_(sheet), impl_(std::make_unique<EmptyImpl>()) {
}

Cell::~Cell() {}

void Cell::Set(const std::string& text) {
    std::unique_ptr<Impl> impl;

    if (text.empty()) {
        impl = std::make_unique<EmptyImpl>();
    }
    else if (text.size() > 1 && text.at(0) == FORMULA_SIGN) {
        impl = std::make_unique<FormulaImpl>(std::move(text), sheet_);
    }
    else {
        impl = std::make_unique<TextImpl>(std::move(text));
    }

    if (CheckCircularDependency(*impl)) {
        throw CircularDependencyException("");
    }

    impl_ = std::move(impl);

    for (Cell* cell : descending_cells_) {
        cell->ascending_cells_.erase(this);
    }

    descending_cells_.clear();
    
    for (const auto& pos : impl_->GetReferencedCells()) {
        Cell* cell = dynamic_cast<Cell*>(sheet_.GetCell(pos));
        
        if (!cell) {
            sheet_.SetCell(pos, "");
            cell = dynamic_cast<Cell*>(sheet_.GetCell(pos));
        }

        descending_cells_.insert(cell);
        cell->ascending_cells_.insert(this);
    }

    InvalidateCachedValues();
}

void Cell::Clear() {
	impl_ = std::make_unique<EmptyImpl>();
}

Cell::Value Cell::GetValue() const {
	return impl_->GetValue();
}
std::string Cell::GetText() const {
	return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

bool Cell::CheckCircularDependency(const Impl& impl) const {
    if (impl.GetReferencedCells().empty()) {
        return false;
    }

    std::unordered_set<const Cell*> referenced;
    for (const auto& pos : impl.GetReferencedCells()) {
        referenced.insert(dynamic_cast<Cell*>(sheet_.GetCell(pos)));
    }

    std::unordered_set<const Cell*> visited;
    std::stack<const Cell*> to_visit;

    to_visit.push(this);

    while (!to_visit.empty()) {
        const Cell* current_cell = to_visit.top();
        to_visit.pop();
        visited.insert(current_cell);

        if (referenced.find(current_cell) != referenced.end()) {
            return true;
        }

        for (const Cell* cell : current_cell->ascending_cells_) {
            if (visited.find(cell) == visited.end()) {
                to_visit.push(cell);
            }
        }
    }
    return false;
}

void Cell::InvalidateCachedValues() {
    impl_->InvalidateCache();

    for (Cell* cell : ascending_cells_) {
        cell->InvalidateCachedValues();
    }
}
