#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

void CheckValidPosition(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Target position is not valid"s);
    }
}

void Sheet::SetCell(Position pos, std::string text) {
    CheckValidPosition(pos);

    if (!GetCell(pos)) {
        table_.resize(std::max(static_cast<size_t>(pos.row) + 1, table_.size()));

        table_[pos.row].resize(std::max(static_cast<size_t>(pos.col) + 1, table_.at(pos.row).size()));
        
        table_.at(pos.row).at(pos.col) = std::make_unique<Cell>(*this);
    }
    table_.at(pos.row).at(pos.col)->Set(std::move(text));
}

const CellInterface* Sheet::GetCell(Position pos) const {
    CheckValidPosition(pos);

    if (pos.row >= static_cast<int>(table_.size()) || pos.col >= static_cast<int>(table_[pos.row].size())) {
        return nullptr;
    }

    return table_.at(pos.row).at(pos.col).get();
}

CellInterface* Sheet::GetCell(Position pos) {
    CheckValidPosition(pos);

    if (pos.row >= static_cast<int>(table_.size()) || pos.col >= static_cast<int>(table_[pos.row].size())) {
        return nullptr;
    }

    return table_.at(pos.row).at(pos.col).get();
}

void Sheet::ClearCell(Position pos) {
    CheckValidPosition(pos);

    if (pos.row >= static_cast<int>(table_.size()) || pos.col >= static_cast<int>(table_[pos.row].size())) {
        return;
    }

    if (GetCell(pos)) {
        table_.at(pos.row).at(pos.col).reset();
    }
}

Size Sheet::GetPrintableSize() const {
    Size size;

    for (int row = 0; row < static_cast<int>(table_.size()); ++row) {
        for (int col = table_[row].size() - 1; col >= 0; --col) {
            if (table_[row][col] && !table_[row][col]->GetText().empty()) {
                size.rows = std::max(size.rows, row + 1);
                size.cols = std::max(size.cols, col + 1);
                break;
            }
        }
    }
    return size;
}

void Sheet::PrintValues(std::ostream& output) const {
    Size size = GetPrintableSize();

    for (int row = 0; row < size.rows; ++row) {
        for (int col = 0; col < size.cols; ++col) {
            if (col > 0) {
                output << '\t';
            }
            if (col < static_cast<int>(table_[row].size())) {
                if (const auto& cell = table_.at(row).at(col)) {
                    std::visit([&](const auto& value) { output << value; }, cell->GetValue());
                }
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    Size size = GetPrintableSize();

    for (int row = 0; row < size.rows; ++row) {
        for (int col = 0; col < size.cols; ++col) {
            if (col > 0) {
                output << '\t';
            }
            if (col < static_cast<int>(table_[row].size())) {
                if (const auto& cell = table_.at(row).at(col)) {
                    output << cell->GetText();
                }
            }
        }
        output << '\n';
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
