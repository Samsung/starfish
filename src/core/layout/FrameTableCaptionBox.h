/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#ifndef __StarfishFrameTableCaptionBox__
#define __StarfishFrameTableCaptionBox__

#include "core/layout/FrameTableObjectBox.h"

namespace Starfish {

class FrameTableCaptionBox final : public FrameTableObjectBox {
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

protected:
    static inline void fillGCDescriptor(GC_word* obj_bitmap);
};
}

#endif
