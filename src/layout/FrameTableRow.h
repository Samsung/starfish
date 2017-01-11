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

#ifndef __StarFishFrameTableRow__
#define __StarFishFrameTableRow__

#include "layout/FrameBlockBox.h"

namespace StarFish {

class FrameTreeBuilderContext;
class FrameTableCell;

class FrameTableRow : public FrameBlockBox {
public:
    FrameTableRow(Node* node, ComputedStyle* style);

    virtual const char* name()
    {
        return "FrameTableRow";
    }

    virtual bool isFrameTableRow()
    {
        return true;
    }

    static FrameTableRow* buildFrameTableRow(Node* current, FrameTreeBuilderContext& ctx, bool force = false);
    static FrameTableRow* createAnonymousWithParent(FrameBlockBox* parent, Node* node);

    void calContentWidth(LayoutContext& ctx);
    void layoutWidth(LayoutContext& ctx);
    void layoutHeight(LayoutContext& ctx);

    FrameTableCell* addChild(Node* child, FrameTreeBuilderContext& ctx, bool force);

    FrameTableSection* tableSection()
    {
        return parent()->asFrameTableSection();
    }

    virtual bool hasBlockFlow()
    {
        // TODO: Fix it after finishing table context properly
        // FIXME: TableRow always contains a blockflow
        Frame* child = firstChild();
        if (!child) {
            STARFISH_ASSERT(child->isNormalFlow());
            // Only TableCell can exist as children of TableRow
            STARFISH_ASSERT(child->isFrameTableCell());
        }

        return true;
    }

    void setRowIndex(unsigned rowIndex)
    {
        m_rowIndex = rowIndex;
    }

    unsigned rowIndex()
    {
        return m_rowIndex;
    }

private:
    virtual void layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat);

    unsigned m_rowIndex;

};

}

#endif
