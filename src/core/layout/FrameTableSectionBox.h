/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
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

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* obj_bitmap)
    {
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(CellStruct, m_cell));
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
        return m_cells.size() > 0 ? m_cells[m_cells.size() - 1] : nullptr;
    }

    unsigned logicalColumnSize();

    FrameTableCellBox* physicalCellAtLogicalColumn(size_t id);

    FrameTableRowBox* tableRow()
    {
        return m_tableRow;
    }

    GCVector<CellStruct*>& cells()
    {
        return m_cells;
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* obj_bitmap)
    {
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(RowStruct, m_tableRow));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(RowStruct, m_cells));
    }

private:
    FrameTableRowBox* m_tableRow;
    GCVector<CellStruct*> m_cells;
};

class FrameTableSectionBox final : public FrameTableObjectBox {
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

    virtual void paintBackgroundAndBorders(Canvas* canvas) override;

    virtual const char* name() override
    {
        return "FrameTableSection";
    }

    virtual bool isFrameTableSectionBox() override
    {
        return true;
    }

    virtual bool hasBlockFlow() override
    {
        // FrameTableSection always contains blockflow
        return true;
    }

    FrameTableBox* tableBox()
    {
        return parent()->asFrameTableBox();
    }

    GCVector<RowStruct*>& grid()
    {
        return m_grid;
    }

    GCVector<ColSizeStruct*>& columnWidths()
    {
        return m_columnWidths;
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* obj_bitmap)
    {
        FrameTableObjectBox::fillGCDescriptor(obj_bitmap);
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableSectionBox, m_grid));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameTableSectionBox, m_columnWidths));
    }

private:
    void layout(LayoutContext& ctx,
                Frame::LayoutWantToResolve resolveWhat) override;

    void calAbsoluteColumnIndicesForCellsAffectedByColspan(
        FrameTableCellBox* cell, size_t rowId);
    void calAbsoluteColumnIndicesForCellsAffectedByRowspan(
        FrameTableCellBox* cell, size_t rowId);

    LayoutUnit calCellHeightWithRowspan(FrameTableCellBox* cell, size_t rowId,
                                        size_t colId);

    GCVector<RowStruct*> m_grid; // cells in a 2D table for easier traversal
    GCVector<ColSizeStruct*> m_columnWidths;
};
}

#endif
