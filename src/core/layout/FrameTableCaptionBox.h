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

#ifndef __StarFishFrameTableCaptionBox__
#define __StarFishFrameTableCaptionBox__

#include "core/layout/FrameTableObjectBox.h"

namespace StarFish {

class FrameTableCaptionBox : public FrameTableObjectBox {
public:
    FrameTableCaptionBox(Node* node, ComputedStyle* style);

    virtual const char* name() override
    {
        return "FrameTableCaption";
    }

    virtual bool isFrameTableCaptionBox() override
    {
        return true;
    }

    void layoutWidth(LayoutContext& ctx);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;
    static inline void fillGCDescriptor(GC_word* obj_bitmap);
};
}

#endif
