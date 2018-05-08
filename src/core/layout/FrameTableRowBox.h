/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
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

    virtual const char* name() override
    {
        return "FrameTableRow";
    }

    virtual bool isFrameTableRowBox() override
    {
        return true;
    }

    bool hasChildCells();
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

    virtual bool hasBlockFlow() override
    {
        // FIXME: TableRow always contains a blockflow
        return true;
    }

    virtual void paintBackgroundAndBorders(Canvas* canvas) override;

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

private:
    virtual void layout(LayoutContext& ctx,
                        Frame::LayoutWantToResolve resolveWhat) override;

    LayoutUnit m_baseline;
};
}

#endif
