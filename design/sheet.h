#pragma once

#include "cell.h"
#include "common.h"

#include <functional>

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;

    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;

    void PrintTexts(std::ostream& output) const override;

private:
    std::vector<std::vector<std::unique_ptr<CellInterface>>> table_;
    std::unordered_map<Position, std::set<Position>, Hasher> cell_dependencies_;

    bool CheckCurcularDependency(Cell* cell, Position pos);

    void DeleteDependances(Position pos);

    void UpdateDependances(Position pos);

    void InvalidateCaches(Position pos);
};
