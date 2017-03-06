/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishFrameTableRowBox__
#define __StarFishFrameTableRowBox__

#include "layout/FrameTableObjectBox.h"

namespace StarFish {

class FrameTreeBuilderContext;
class FrameTableCellBox;

class FrameTableRowBox : public FrameTableObjectBox {
public:
    FrameTableRowBox(Node* node, ComputedStyle* style);

    virtual const char* name()
    {
        return "FrameTableRow";
    }

    virtual bool isFrameTableRowBox()
    {
        return true;
    }

    static FrameTableRowBox* buildFrameTableRow(Node* current,
                                                FrameTreeBuilderContext& ctx,
                                                bool force = false);
    static FrameTableRowBox* createAnonymousWithParent(FrameBlockBox* parent,
                                                       Node* node);

    void calCellWidth(LayoutContext& ctx);
    void layoutWidth(LayoutContext& ctx);
    void layoutHeight(LayoutContext& ctx);
    LayoutUnit calBaseline();

    LayoutUnit baseline()
    {
        return m_baseline;
    }

    void addChild(Node* child, FrameTreeBuilderContext& ctx, bool force);

    FrameTableSectionBox* sectionBox()
    {
        return parent()->asFrameTableSectionBox();
    }

    virtual bool hasBlockFlow()
    {
        // FIXME: TableRow always contains a blockflow
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

    virtual void paintBackgroundAndBorders(Canvas* canvas);

private:
    virtual void layout(LayoutContext& ctx,
                        Frame::LayoutWantToResolve resolveWhat);

    unsigned m_rowIndex;
    unsigned m_lastAbsoluteColumnIndex;
    LayoutUnit m_baseline;
};
}

#endif
