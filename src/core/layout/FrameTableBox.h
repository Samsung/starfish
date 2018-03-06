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

class ColSizeStruct {
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
};

class FrameTableBox : public FrameTableObjectBox {
public:
    FrameTableBox(Node* node, ComputedStyle* style);

    virtual void computePreferredWidth(PreferredWidthContext& ctx);
    virtual const char* name()
    {
        return "FrameTable";
    }

    virtual bool isFrameTableBox()
    {
        return true;
    }

    virtual bool hasBlockFlow()
    {
        // FrameTable always contains blockflow
        return true;
    }

    GCAtomicVector<ColSizeStruct>& columnWidths()
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

    virtual void paintBackgroundAndBorders(Canvas* canvas);
    virtual Unit::Rect makeRect(BoxValue box) override;

    void computeTableWidth(LayoutContext& ctx);
    void layoutTable(LayoutContext& ctx);

    LayoutUnit calBaseline(LayoutContext& ctx);

    // This function return nullptr if there is no valid column object
    FrameTableColBox* columnAtAbsoluteColumnIndex(unsigned index);
    FrameTableSectionBox* firstSectionBoxInVisualOrder();

    bool isCellWidthAuto(unsigned i);

    virtual void resetIfNeeds(LayoutContext& ctx);
    virtual void iterateChildFrameBox(const std::function<void(FrameBox*)>& fn)
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

private:
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

    void collectColumnWidths(GCAtomicVector<ColSizeStruct>& columnWidthsSoFar,
                             GCAtomicVector<ColSizeStruct>& columnWidths);

    bool resetColspanIfPossible();

    // This function returns nullptr if the table has no non-empty sections.
    FrameTableSectionBox* firstNonEmptySectionBoxInVisualOrder();

    size_t numOfRowsInTheTable();

    LayoutUnit cellspacingFromAttribute();

    template <typename Func>
    void forEachRowStruct(Func filter);

    GCVector<FrameTableCaptionBox*> m_captions;
    GCVector<FrameTableColBox*> m_colObjects;
    GCAtomicVector<ColSizeStruct> m_columnWidths;

    GCVector<FrameTableCellBox*> m_cellsInTheFirstRow;
    GCVector<FrameTableColBox*> m_colBoxes;

    // Border and background is drawn around FrameTableSections not FrameTable
    // Keep track of FrameTableSections for border and background
    LayoutRect m_tableRect;
};
}

#endif
