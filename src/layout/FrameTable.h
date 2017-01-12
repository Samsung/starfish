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

#ifndef __StarFishFrameTable__
#define __StarFishFrameTable__

#include "layout/FrameBlockBox.h"

namespace StarFish {

class FrameTableCaption;
class FrameTreeBuilderContext;
class TableFormattingContextBlock;

// Table has the following table structure
//
//                 FrameTable
//                 |         |
//   FrameTableCaption     FrameTableSection
//                           |
//                         FrameTableRow
//                           |
//                         FrameTableCell
//
// FrameTableCaption, FrameTableSection, FrameTableRow, FrameTableCell are
// the only child nodes that can appear under FrameTable.
// When other nodes appear, anonymous nodes are created
//
// Table has its own table layout algorithm, that has minimum interaction
// with the existing box layout algorithm.

struct ColSizeStruct {
    LayoutUnit minCellWidth;
    LayoutUnit maxCellWidth;
};

class FrameTable : public FrameBlockBox {
public:
    FrameTable(Node* node, ComputedStyle* style);

    static FrameTable* buildFrameTable(Node* current, FrameTreeBuilderContext& ctx, bool force = false);
    static FrameTable* createAnonymousWithParent(FrameBlockBox* parent, Node* node);

    virtual void layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat);
    void calCellWidth(LayoutContext& ctx);
    void layoutWidth(LayoutContext& ctx);
    void layoutHeight(LayoutContext& ctx);

    void addChild(Node* child, FrameTreeBuilderContext& ctx, bool force);

    virtual const char* name()
    {
        return "FrameTable";
    }

    virtual bool isFrameTable()
    {
        return true;
    }

    virtual bool hasBlockFlow()
    {
        // FrameTable always contains blockflow
        Frame* child = firstChild();
        if (child) {
            STARFISH_ASSERT(child->isNormalFlow());
            // Only FrameTableCaption or FrameTableSection can exist as children
            // of FrameTable
            STARFISH_ASSERT(child->isFrameTableCaption() || child->isFrameTableSection());
        }

        return true;
    }

    GCVector<ColSizeStruct>& columnWidths()
    {
        return m_columnWidths;
    }

private:
    void collectColumnWidths(GCVector<ColSizeStruct>& columnWidthsSoFar, GCVector<ColSizeStruct>& columnWidths);

    GCVector<FrameTableCaption*> m_captions;
    GCVector<ColSizeStruct> m_columnWidths;
};

}

#endif
