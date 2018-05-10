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

#ifndef __StarFishFrameTableCellBox__
#define __StarFishFrameTableCellBox__

#include "core/layout/FrameTableObjectBox.h"

namespace StarFish {

class ComputedStyle;
class Node;

class FrameTableCellBox : public FrameTableObjectBox {
public:
    FrameTableCellBox(Node* node, ComputedStyle* style);

    void collectCellWidthInfo(LayoutContext& ctx,
                              Frame::LayoutWantToResolve resolveWhat);
    void layoutWidth(LayoutContext& ctx);
    void layoutHeight(LayoutContext& ctx);

    virtual const char* name() override
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

    virtual bool isFrameTableCellBox() override
    {
        return true;
    }

    bool isHTMLTHElement();

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

    void paintBackgroundAndBorders(Canvas* canvas) override;

    void applyVerticalAlign(LayoutContext& ctx);
    LayoutUnit calBaseline(LayoutContext& ctx);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* obj_bitmap)
    {
        FrameTableObjectBox::fillGCDescriptor(obj_bitmap);
    }

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
