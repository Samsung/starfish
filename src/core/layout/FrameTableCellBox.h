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

#ifndef __StarFishFrameTableCellBox__
#define __StarFishFrameTableCellBox__

#include "core/layout/FrameTableObjectBox.h"

namespace StarFish {

class ComputedStyle;
class FrameTreeBuilderContext;
class Node;

class FrameTableCellBox : public FrameTableObjectBox {
public:
    FrameTableCellBox(Node* node, ComputedStyle* style);

    void calCellWidth(LayoutContext& ctx,
                      Frame::LayoutWantToResolve resolveWhat);
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

    unsigned colspan();

    virtual bool isFrameTableCellBox()
    {
        return true;
    }

    virtual void addChild(Node* child, FrameTreeBuilderContext& ctx,
                          bool force);

    void setAbsoluteColumnIndex(unsigned column)
    {
        m_absoluteColumnIndex = column;
    }

    void setActualContentHeight(LayoutUnit height)
    {
        m_actualContentHeight = height;
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

    void paintBackgroundAndBorders(Canvas* canvas);

    void applyVerticalAlign();
    LayoutUnit calBaseline();

private:
    virtual void layout(LayoutContext& ctx,
                        Frame::LayoutWantToResolve resolveWhat);
    bool emptyContent();

    unsigned m_absoluteColumnIndex; // starts with 0
    LayoutUnit m_minCellWidth;
    LayoutUnit m_maxCellWidth;
    LayoutUnit m_actualContentHeight;
};
}

#endif
