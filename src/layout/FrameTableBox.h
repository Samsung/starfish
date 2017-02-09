/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

struct ColSizeStruct {
    ColSizeStruct()
        : minCellWidth(LayoutUnit())
        , maxCellWidth(LayoutUnit())
        , cellWidth(LayoutUnit())
    { }

    LayoutUnit minCellWidth;
    LayoutUnit maxCellWidth;
    LayoutUnit cellWidth;
};

class FrameTableBox : public FrameTableObjectBox {
public:
    FrameTableBox(Node* node, ComputedStyle* style);

    static FrameTableBox* buildFrameTable(Node* current, FrameTreeBuilderContext& ctx, bool force = false);
    static FrameTableBox* createAnonymousWithParent(FrameBlockBox* parent, Node* node);

    virtual void layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat);

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

    std::vector<FrameTableCellBox*>& cellsInTheFirstRow()
    {
        return m_cellsInTheFirstRow;
    }

    virtual void paintBackgroundAndBorders(Canvas* canvas);

private:
    void layoutWidth(LayoutContext& ctx);
    void layoutHeight(LayoutContext& ctx);
    void calCellWidth(LayoutContext& ctx);

    void collectColumnWidths(GCVector<ColSizeStruct>& columnWidthsSoFar, GCVector<ColSizeStruct>& columnWidths);
    void calCellWidthForAutoTableLayout(LayoutContext& ctx);
    void calCellWidthForFixedTableLayout(LayoutContext& ctx);

    bool isCellWidthAuto(unsigned i);

    GCVector<FrameTableCaptionBox*> m_captions;
    GCVector<ColSizeStruct> m_columnWidths;

    // We use vector here because "FrameTableCellBox"es are already stored
    // in GCVector, and it is used only to the duration of layoutWidth().
    std::vector<FrameTableCellBox*> m_cellsInTheFirstRow;

    // Border and background is drawn around FrameTableSections not FrameTable
    // Keep track of FrameTableSections for border and background
    LayoutRect m_tableRect;

    FrameTableSectionBox* m_thead;
    FrameTableSectionBox* m_tfoot;
};

}

#endif
