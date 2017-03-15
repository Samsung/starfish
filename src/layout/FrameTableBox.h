/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

#include "layout/FrameTableObjectBox.h"

namespace StarFish {

class FrameTableCaptionBox;
class FrameTreeBuilderContext;
class TableFormattingContextBlock;
class FrameTableCellBox;
class FrameTableColBox;

// Table has the following table structure
//
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
        , maxSpecifiedWidth(LayoutUnit())
        , minCellWidth(LayoutUnit())
        , maxCellWidth(LayoutUnit())
        , cellWidth(LayoutUnit())
    {
    }

    bool hasSpecifiedWidth()
    {
        return maxSpecifiedWidth == 0 ? false : true;
    }

    size_t id;
    LayoutUnit maxSpecifiedWidth;
    LayoutUnit minCellWidth;
    LayoutUnit maxCellWidth;
    LayoutUnit cellWidth;
};

class FrameTableBox : public FrameTableObjectBox {
public:
    FrameTableBox(Node* node, ComputedStyle* style);

    static FrameTableBox* buildFrameTable(Node* current,
                                          FrameTreeBuilderContext& ctx,
                                          bool force = false);
    static FrameTableBox* createAnonymousWithParent(FrameBlockBox* parent,
                                                    Node* node);

    virtual void layout(LayoutContext& ctx,
                        Frame::LayoutWantToResolve resolveWhat);

    void addChild(Node* child, FrameTreeBuilderContext& ctx, bool force);

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

    GCVector<ColSizeStruct>& columnWidths()
    {
        return m_columnWidths;
    }

    GCVector<FrameTableCellBox*>& cellsInTheFirstRow()
    {
        return m_cellsInTheFirstRow;
    }

    GCVector<FrameTableColBox*>& colObjects()
    {
        return m_colObjects;
    }

    virtual void paintBackgroundAndBorders(Canvas* canvas);
    LayoutUnit calBaseline();

    // This function return nullptr if there is no valid column object
    FrameTableColBox* columnAtAbsoluteColumnIndex(unsigned index);
    FrameTableSectionBox* firstSectionBoxInVisualOrder();

private:
    void layoutWidth(LayoutContext& ctx);
    void layoutHeight(LayoutContext& ctx);
    void calCellWidth(LayoutContext& ctx);

    void collectColumnWidths(GCVector<ColSizeStruct>& columnWidthsSoFar,
                             GCVector<ColSizeStruct>& columnWidths);
    void calCellWidthForAutoTableLayout(LayoutContext& ctx);
    void calCellWidthForFixedTableLayout(LayoutContext& ctx);

    // This function returns nullptr if the table has no non-empty sections.
    FrameTableSectionBox* firstNonEmptySectionBoxInVisualOrder();

    bool isCellWidthAuto(unsigned i);

#ifndef NDEBUG
    // width() is removed from HTML5. But We implement it as it is extensively
    // used in w3c test cases.
    LayoutUnit widthFromAttribute()
    {
        STARFISH_ASSERT(
            node()->asElement()->asHTMLElement()->isHTMLTableElement());
        String* w =
            node()->asElement()->asHTMLElement()->asHTMLTableElement()->width();
        // It is ok to use -1 to indicate both "doesn't exist" and
        // actual negative width, as negative width is invalid.
        // FYI, Blink and Firefox ignore a negative width for table
        if (w->equals(String::emptyString)) {
            return LayoutUnit::fromPixel(-1);
        } else {
            int n = String::parseInt(w);
            return n < 0 ? LayoutUnit::fromPixel(-1) : LayoutUnit::fromPixel(n);
        }
    }
#endif

    GCVector<FrameTableCaptionBox*> m_captions;
    GCVector<FrameTableColBox*> m_colObjects;
    GCVector<ColSizeStruct> m_columnWidths;

    GCVector<FrameTableCellBox*> m_cellsInTheFirstRow;

    // Border and background is drawn around FrameTableSections not FrameTable
    // Keep track of FrameTableSections for border and background
    LayoutRect m_tableRect;

    FrameTableSectionBox* m_thead;
    FrameTableSectionBox* m_tfoot;
};
}

#endif
