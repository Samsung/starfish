/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishFrameReplacedImage__
#define __StarFishFrameReplacedImage__

#include "core/layout/FrameReplaced.h"

namespace StarFish {

class FrameReplacedImage final : public FrameReplaced {
public:
    FrameReplacedImage(Node* node)
        : FrameReplaced(node, nullptr)
    {
    }

    virtual bool isFrameReplacedImage() override
    {
        return true;
    }

    virtual const char* name() override
    {
        return "FrameReplacedImage";
    }

    virtual void paintReplaced(Canvas* canvas) override;
    virtual IntrinsicSize intrinsicSize() override;

    void* operator new(size_t size)
    {
        STARFISH_ASSERT(size == sizeof(FrameReplacedImage));
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word desc[GC_BITMAP_SIZE(FrameReplacedImage)] = { 0 };
            FrameReplacedImage::fillGCDescriptor(desc);
            descr = GC_make_descriptor(desc, GC_WORD_LEN(FrameReplacedImage));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        FrameReplaced::fillGCDescriptor(desc);
    }
};
}

#endif
