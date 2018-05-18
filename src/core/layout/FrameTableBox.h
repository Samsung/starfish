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

#ifndef __StarFishFrameTableBox__
#define __StarFishFrameTableBox__

#include "core/layout/FrameTableObjectBox.h"

namespace StarFish {

class FrameTableCaptionBox;
class FrameTableCellBox;
class FrameTableColBox;
class FrameTreeBuilderContext;

// Table has the following table structure
//
//               FrameTableObjectBox
//                       |
//                 FrameTableBox
//                 |         |
//   FrameTableCaptionBox  FrameTableSectionBox
//                           |
//                         FrameTableRowBox
//                           |
//                         FrameTableCellBox
//
// FrameTableCaptionBox, FrameTableSectionBox, FrameTableRowBox,
// FrameTableCellBox are the only child nodes that can appear under
// FrameTableBox. When other nodes appear, anonymous nodes are created.
//
// Table has its own table layout algorithm, that has minimum interaction
// with the existing box layout algorithm.

class Cell : public gc {
    friend class FrameTableBox;

public:
    Cell(FrameTableCellBox* cellBox, int x, int y, int width, int height)
        : m_cellBox(cellBox)
        , m_slotX(x)
        , m_slotY(y)
        , m_width(width)
        , m_height(height)
    {
    }

    void* operator new(size_t size)
    {
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word desc[GC_BITMAP_SIZE(Cell)] = { 0 };
            Cell::fillGCDescriptor(desc);
            descr = GC_make_descriptor(desc, GC_WORD_LEN(Cell));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }

    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc, GC_WORD_OFFSET(Cell, m_cellBox));
    }

private:
    FrameTableCellBox* m_cellBox;
    size_t m_slotX;
    size_t m_slotY;
    size_t m_width;
    size_t m_height;
};

class Row : public gc {
    friend class FrameTableBox;

    void* operator new(size_t size)
    {
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word desc[GC_BITMAP_SIZE(Row)] = { 0 };
            Row::fillGCDescriptor(desc);
            descr = GC_make_descriptor(desc, GC_WORD_LEN(Row));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }

    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc, GC_WORD_OFFSET(Row, m_cells));
    }

private:
    // represents a row of cells
    GCVector<Cell*> m_cells;
};

class ColGroup : public gc {
    friend class FrameTableBox;

public:
    ColGroup(int x)
        : m_slotX(x)
        , m_slotWidth(1)
    {
    }

    void* operator new(size_t size)
    {
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word desc[GC_BITMAP_SIZE(ColGroup)] = { 0 };
            ColGroup::fillGCDescriptor(desc);
            descr = GC_make_descriptor(desc, GC_WORD_LEN(ColGroup));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }

    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc, GC_WORD_OFFSET(ColGroup, m_column));
    }

private:
    size_t m_slotX; // slotY = 0 by definition
    size_t m_slotWidth;
    GCVector<Cell*> m_column;
};

class Table : public gc {
    friend class FrameTableBox;

public:
    Table()
        : m_width(0)
        , m_height(0)
    {
    }

    void* operator new(size_t size)
    {
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word desc[GC_BITMAP_SIZE(Table)] = { 0 };
            Table::fillGCDescriptor(desc);
            descr = GC_make_descriptor(desc, GC_WORD_LEN(Table));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new[](size_t size) = delete;

    void clear()
    {
        m_width = 0;
        m_height = 0;
        m_rows.clear();
        m_colGroups.clear();
    }

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc, GC_WORD_OFFSET(Table, m_rows));
        GC_set_bit(desc, GC_WORD_OFFSET(Table, m_colGroups));
    }

private:
    size_t m_width;
    size_t m_height;
    GCVector<Row*> m_rows; // contains all rows in correct visual order
    GCVector<ColGroup*> m_colGroups;
};

class ColSizeStruct : public gc {
public:
    ColSizeStruct()
        : id(0)
        , maxSpecifiedWidth(0)
        , maxPercentageWidth(0)
        , minCellWidth(0)
        , maxCellWidth(0)
        , cellWidth(0)
        , isNullCell(true)
    {
    }

    bool hasSpecifiedWidth()
    {
        return maxSpecifiedWidth != 0;
    }

    bool hasPercentageWidth()
    {
        return maxPercentageWidth != 0;
    }

    bool isEmptyCell()
    {
        return (minCellWidth == 0) && (maxCellWidth == 0);
    }

    size_t id;
    LayoutUnit maxSpecifiedWidth;
    double maxPercentageWidth;
    LayoutUnit minCellWidth;
    LayoutUnit maxCellWidth;
    LayoutUnit cellWidth;
    bool isNullCell;

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* obj_bitmap)
    {
    }
};

class FrameTableBox : public FrameTableObjectBox {
public:
    FrameTableBox(Node* node, ComputedStyle* style);

    virtual void computePreferredWidth(PreferredWidthContext& ctx) override;
    virtual const char* name() override
    {
        return "FrameTable";
    }

    virtual bool isFrameTableBox() override
    {
        return true;
    }

    virtual bool hasBlockFlow() override
    {
        // FrameTable always contains blockflow
        return true;
    }

    GCVector<ColSizeStruct*>& columnWidths()
    {
        return m_columnWidths;
    }

    GCVector<FrameTableCellBox*>& cellsInTheFirstRow()
    {
        return m_cellsInTheFirstRow;
    }

    FrameTableCellBox* cellInTheFirstRowAt(unsigned id);

    LayoutRect tableRect()
    {
        return m_tableRect;
    }

    virtual void paintBackgroundAndBorders(Canvas* canvas) override;
    virtual Unit::Rect makeRect(BoxValue box) override;

    void computeTableWidth(LayoutContext& ctx);
    void layoutTable(LayoutContext& ctx);

    LayoutUnit calBaseline(LayoutContext& ctx);

    // This function return nullptr if there is no valid column object
    FrameTableColBox* columnAtAbsoluteColumnIndex(unsigned index);
    FrameTableSectionBox* firstSectionBoxInVisualOrder();

    bool isCellWidthAuto(unsigned i);

    virtual void resetIfNeeds(LayoutContext& ctx);
    virtual void iterateChildFrameBox(
        const std::function<void(FrameBox*)>& fn) override
    {
        fn(this);
        Frame* box = firstChild();
        while (box) {
            box->asFrameBox()->iterateChildFrameBox(fn);
            box = box->next();
        }
    }

    FrameTableSectionBox* thead() const
    {
        Frame* child = firstChild();

        while (child) {
            if (child->style()->display() ==
                DisplayValue::TableHeaderGroupDisplayValue) {
                return child->asFrameTableSectionBox();
            }
            child = child->next();
        }

        return nullptr;
    }

    FrameTableSectionBox* tfoot() const
    {
        Frame* child = firstChild();

        while (child) {
            if (child->style()->display() ==
                DisplayValue::TableFooterGroupDisplayValue) {
                return child->asFrameTableSectionBox();
            }
            child = child->next();
        }

        return nullptr;
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* obj_bitmap)
    {
        FrameTableObjectBox::fillGCDescriptor(obj_bitmap);
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableBox, m_table));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableBox, m_captions));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableBox, m_colObjects));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableBox, m_columnWidths));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameTableBox, m_cellsInTheFirstRow));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableBox, m_colBoxes));
    }

private:
    void formingATable();
    void processRow(FrameTableRowBox* rowBox, size_t& yCurrent, size_t& xWidtht,
                    size_t& yHeight);
    Cell* cellAtSlot(size_t x, size_t y);
    bool isSlotOccupied(size_t x, size_t y);
    void assigningHeaderCells(Cell* principalCell);
    void scanningAndAssigningHeaderCells(Cell* principalCell,
                                         GCVector<Cell*>& headers, int deltaX,
                                         int deltaY);

    void calCellWidth(LayoutContext& ctx);
    void calCellWidthsWithColspans();
    void layoutWidth(LayoutContext& ctx);
    void layoutHeight(LayoutContext& ctx);

    LayoutUnit calCellHeightWithRowspan(FrameTableCellBox* cell, size_t rowId,
                                        size_t colId);

    void setCandidateCellWidthsAndReturnCellInfo(
        LayoutContext& ctx, LayoutUnit tableWidth, bool hasTableWidth,
        bool tableLayoutFixed, LayoutUnit* sumOfAutoCellPreferredWidths,
        LayoutUnit* sumOfAdjustedSpecifiedCellWidths,
        std::vector<ColSizeStruct*>* columnsAdjustedToMinWidths,
        std::vector<ColSizeStruct*>* columnsMayNeedToAdjustWidths,
        LayoutUnit* sumOfColWidths);

    void calCellWidthsWithPercentageWidths(
        LayoutUnit remainingWidth,
        std::vector<ColSizeStruct*> columnsMayNeedToAdjustWidths,
        LayoutUnit* sumOfPercentageWidth);

    void collectColBoxes();
    bool hasColBox(size_t i);
    FrameTableCellBox* cellFromFirstRowOrColGroup(bool tableLayoutFixed,
                                                  size_t i);

    void collectColumnWidths(GCVector<ColSizeStruct*>& columnWidthsSoFar,
                             GCVector<ColSizeStruct*>& columnWidths);

    bool resetColspanIfPossible();

    // This function returns nullptr if the table has no non-empty sections.
    FrameTableSectionBox* firstNonEmptySectionBoxInVisualOrder();

    size_t numOfRowsInTheTable();

    LayoutUnit cellspacingFromAttribute();

    template <typename Func>
    void forEachRowStruct(Func filter);

    Table* m_table;

    GCVector<FrameTableCaptionBox*> m_captions;
    GCVector<FrameTableColBox*> m_colObjects;
    GCVector<ColSizeStruct*> m_columnWidths;

    GCVector<FrameTableCellBox*> m_cellsInTheFirstRow;
    GCVector<FrameTableColBox*> m_colBoxes;

    // Border and background is drawn around FrameTableSections not FrameTable
    // Keep track of FrameTableSections for border and background
    LayoutRect m_tableRect;
};
}

#endif
