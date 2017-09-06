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
    virtual void addChild(Node* child, FrameTreeBuilderContext& ctx,
                          bool force);
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

    GCVector<FrameTableColBox*>& colObjects()
    {
        return m_colObjects;
    }

    virtual void paintBackgroundAndBorders(Canvas* canvas);

    void computeTableWidth(LayoutContext& ctx);
    void layoutTable(LayoutContext& ctx);

    LayoutUnit calBaseline(LayoutContext& ctx);

    // This function return nullptr if there is no valid column object
    FrameTableColBox* columnAtAbsoluteColumnIndex(unsigned index);
    FrameTableSectionBox* firstSectionBoxInVisualOrder();

    bool isCellWidthAuto(unsigned i);

    virtual void iterateChildFrameBox(const std::function<void(FrameBox*)>& fn)
    {
        fn(this);
        Frame* box = firstChild();
        while (box) {
            box->asFrameBox()->iterateChildFrameBox(fn);
            box = box->next();
        }
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
        LayoutUnit tableWidth, LayoutUnit* sumOfAutoCellPreferredWidths,
        LayoutUnit* sumOfAdjustedSpecifiedCellWidths,
        std::vector<ColSizeStruct*>* columnsAdjustedToMinWidths,
        std::vector<ColSizeStruct*>* columnsMayNeedToAdjustWidths,
        LayoutUnit* sumOfColWidths);

    void calCellWidthsWithPercentageWidths(
        LayoutUnit remainingWidth,
        std::vector<ColSizeStruct*> columnsMayNeedToAdjustWidths,
        LayoutUnit* sumOfPercentageWidth);

    void collectColumnWidths(GCAtomicVector<ColSizeStruct>& columnWidthsSoFar,
                             GCAtomicVector<ColSizeStruct>& columnWidths);

    void resetColspanIfPossible();

    // This function returns nullptr if the table has no non-empty sections.
    FrameTableSectionBox* firstNonEmptySectionBoxInVisualOrder();

    size_t numOfRowsInTheTable();

    // width() is removed from HTML5. But We implement it as it is extensively
    // used in w3c test cases.
    LayoutUnit widthFromAttribute(LayoutUnit parentContentWidth);
    LayoutUnit cellspacingFromAttribute();

    template <typename Func>
    void forEachRowStruct(Func filter);

    GCVector<FrameTableCaptionBox*> m_captions;
    GCVector<FrameTableColBox*> m_colObjects;
    GCAtomicVector<ColSizeStruct> m_columnWidths;

    GCVector<FrameTableCellBox*> m_cellsInTheFirstRow;

    // Border and background is drawn around FrameTableSections not FrameTable
    // Keep track of FrameTableSections for border and background
    LayoutRect m_tableRect;

    FrameTableSectionBox* m_thead;
    FrameTableSectionBox* m_tfoot;
    LayoutUnit m_candidateWidth;
};
}

#endif
