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

#ifndef __StarFishFrameTableColBox__
#define __StarFishFrameTableColBox__

#include "core/layout/FrameTableCellBox.h"

namespace StarFish {

// The FrameTableColBox is for <col> and <colgroup>
// In the specification, <col> and <colgroup> are very similar
// The only difference is that <col> should not have children
// So we will use A appropriately for <col> and <colgroup>
class FrameTableColBox : public FrameTableCellBox {
public:
    FrameTableColBox(Node* node, ComputedStyle* style);

    virtual const char* name() override
    {
        return "FrameTableColBox";
    }

    virtual bool isFrameTableCellBox() override
    {
        return false;
    }

    virtual bool isFrameTableColBox() override
    {
        return true;
    }

    uint32_t span();
    bool hasChildColBox();

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* obj_bitmap)
    {
        FrameTableCellBox::fillGCDescriptor(obj_bitmap);
    }

private:
    virtual void layout(LayoutContext& ctx,
                        Frame::LayoutWantToResolve resolveWhat) override;
    virtual void paintContent(PaintingContext& ctx) override;
};
}

#endif
