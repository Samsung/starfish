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

#ifndef __StarFishFrameTableRowBox__
#define __StarFishFrameTableRowBox__

#include "core/layout/FrameTableObjectBox.h"

namespace StarFish {

class FrameTableCellBox;
class ColSizeStruct;

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

    void collectCellWidthInfo(LayoutContext& ctx);
    void layoutWidth(LayoutContext& ctx);
    void layoutHeight(LayoutContext& ctx);
    void increaseCellHeightBy(LayoutUnit cellHeightOffset);
    void applyVerticalAlign(LayoutContext& ctx);

    LayoutUnit calBaseline(LayoutContext& ctx);

    LayoutUnit baseline()
    {
        return m_baseline;
    }

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

    void setLastAbsoluteColumnIndex(unsigned lastAbsoluteColumnIndex)
    {
        m_lastAbsoluteColumnIndex = lastAbsoluteColumnIndex;
    }

    unsigned lastAbsoluteColumnIndex() const
    {
        return m_lastAbsoluteColumnIndex;
    }

    virtual void paintBackgroundAndBorders(Canvas* canvas);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

private:
    virtual void layout(LayoutContext& ctx,
                        Frame::LayoutWantToResolve resolveWhat) override;

    unsigned m_rowIndex;
    unsigned m_lastAbsoluteColumnIndex;
    LayoutUnit m_baseline;
};
}

#endif
