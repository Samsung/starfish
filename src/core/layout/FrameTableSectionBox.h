/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef __StarFishFrameTableSectionBox__
#define __StarFishFrameTableSectionBox__

#include "core/layout/FrameTableObjectBox.h"

namespace StarFish {

class FrameTableBox;
class FrameTableRowBox;
class FrameTableCellBox;
class ColSizeStruct;

class CellStruct : public gc {
public:
    CellStruct()
        : CellStruct(nullptr)
    {
    }

    CellStruct(FrameTableCellBox* cell)
        : m_cell(cell)
    {
    }

    FrameTableCellBox* cell()
    {
        return m_cell;
    }

private:
    FrameTableCellBox* m_cell;
};

class RowStruct : public gc {
public:
    RowStruct()
        : m_tableRow(nullptr)
    {
    }
    RowStruct(FrameTableRowBox* tableRow);

    CellStruct* lastCell()
    {
        return m_cells.size() > 0 ? &m_cells[m_cells.size() - 1] : nullptr;
    }

    unsigned logicalColumnSize();

    FrameTableCellBox* physicalCellAtLogicalColumn(size_t id);

    FrameTableRowBox* tableRow()
    {
        return m_tableRow;
    }

    GCVector<CellStruct>& cells()
    {
        return m_cells;
    }

private:
    FrameTableRowBox* m_tableRow;
    GCVector<CellStruct> m_cells;
};

class FrameTableSectionBox : public FrameTableObjectBox {
public:
    FrameTableSectionBox(Node* node, ComputedStyle* style);

    void collectCellWidthInfo(LayoutContext& ctx);
    void calAbsoluteColumnIndicesForCells();
    void calCellWidthsWithColspans();
    void calCellHeightsWithRowspans();
    void layoutWidth(LayoutContext& ctx);
    void layoutHeight(LayoutContext& ctx);

    void increaseRowHeightBy(LayoutUnit rowHeightOffset);
    void applyVerticalAlign(LayoutContext& ctx);

    virtual void paintBackgroundAndBorders(Canvas* canvas);

    virtual const char* name()
    {
        return "FrameTableSection";
    }

    virtual bool isFrameTableSectionBox()
    {
        return true;
    }

    virtual bool hasBlockFlow()
    {
        // FrameTableSection always contains blockflow
        return true;
    }

    FrameTableBox* tableBox()
    {
        return parent()->asFrameTableBox();
    }

    GCVector<RowStruct>& grid()
    {
        return m_grid;
    }

    GCAtomicVector<ColSizeStruct>& columnWidths()
    {
        return m_columnWidths;
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

private:
    void layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat);

    void calAbsoluteColumnIndicesForCellsAffectedByColspan(
        FrameTableCellBox* cell, size_t rowId);
    void calAbsoluteColumnIndicesForCellsAffectedByRowspan(
        FrameTableCellBox* cell, size_t rowId);

    LayoutUnit calCellHeightWithRowspan(FrameTableCellBox* cell, size_t rowId,
                                        size_t colId);

    GCVector<RowStruct> m_grid; // cells in a 2D table for easier traversal
    GCAtomicVector<ColSizeStruct> m_columnWidths;
};
}

#endif
