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

#ifndef __StarFishFrameTableCellBox__
#define __StarFishFrameTableCellBox__

#include "layout/FrameBlockBox.h"

namespace StarFish {

class FrameTreeBuilderContext;

class FrameTableCellBox : public FrameBlockBox {
public:
    FrameTableCellBox(Node* node, ComputedStyle* style);

    static FrameTableCellBox* buildFrameTableCell(Node* current, FrameTreeBuilderContext& ctx, bool force = false);
    static FrameTableCellBox* createAnonymousWithParent(FrameBlockBox* parent, Node* node);

    void calCellWidth(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat);
    void layoutWidth(LayoutContext& ctx);
    void layoutHeight(LayoutContext& ctx);

    virtual const char* name()
    {
        return "FrameTableCell";
    }

    FrameTableRowBox* rowBox()
    {
        return parent()->asFrameTableRowBox();
    }

    int colspan();

    virtual bool isFrameTableCellBox()
    {
        return true;
    }

    void setAbsoluteColumnIndex(unsigned column)
    {
        m_absoluteColumnIndex = column;
    }

    unsigned absoluteColumnIndex()
    {
        return m_absoluteColumnIndex;
    }

    LayoutUnit minCellWidth()
    {
        return m_minCellWidth;
    }

    LayoutUnit maxCellWidth()
    {
        return m_maxCellWidth;
    }

private:
    virtual void layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat);

    LayoutUnit calMinCellWidth(LayoutContext& ctx);
    LayoutUnit calMaxCellWidth(LayoutContext& ctx);
    static LayoutUnit calPreferredFrameWidth(LayoutContext& ctx, FrameBlockBox* box);

    unsigned m_absoluteColumnIndex; // starts with 0
    LayoutUnit m_minCellWidth;
    LayoutUnit m_maxCellWidth;

};

}

#endif
