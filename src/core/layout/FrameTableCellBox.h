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

    void collectCellWidthInfo(LayoutContext& ctx,
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

    size_t colspan();
    size_t rowspan();
    size_t updatedColspan();
    size_t updatedRowspan();
    void updateColspanForLayout(size_t colspan);
    void resetColspanForLayout();

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

    unsigned absoluteColumnIndex()
    {
        return m_absoluteColumnIndex;
    }

    LayoutUnit minCellWidth()
    {
        return m_minCellWidth;
    }

    void setMinCellWidth(LayoutUnit minCellWidth)
    {
        m_minCellWidth = minCellWidth;
    }

    LayoutUnit maxCellWidth()
    {
        return m_maxCellWidth;
    }

    void setMaxCellWidth(LayoutUnit maxCellWidth)
    {
        m_maxCellWidth = maxCellWidth;
    }

    void paintBackgroundAndBorders(Canvas* canvas);

    void applyVerticalAlign(LayoutContext& ctx);
    LayoutUnit calBaseline(LayoutContext& ctx);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

private:
    bool emptyContent();

    unsigned m_absoluteColumnIndex; // starts with 0
    LayoutUnit m_minCellWidth;
    LayoutUnit m_maxCellWidth;

    size_t m_updatedColspan;
    size_t m_updatedRowspan;
};
}

#endif
