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

#ifdef STARFISH_ENABLE_MULTIMEDIA
#ifndef __StarFishFrameReplacedVideo__
#define __StarFishFrameReplacedVideo__

#include "core/layout/FrameReplaced.h"

namespace StarFish {

class FrameReplacedVideo final : public FrameReplaced {
public:
    FrameReplacedVideo(Node* node)
        : FrameReplaced(node, nullptr)
    {
        computeStyleFlags();
    }

    virtual void computeStyleFlags() override
    {
        FrameReplaced::computeStyleFlags();
        m_flags.m_isEstablishesStackingContext = true;
        m_flags.m_needsGraphicsBuffer = true;
    }

    virtual bool isFrameReplacedVideo() override
    {
        return true;
    }

    virtual const char* name() override
    {
        return "FrameReplacedVideo";
    }

    virtual IntrinsicSize intrinsicSize() override;
    virtual void didCompsiteStackingContext(Compositor* c) override;
    virtual void createGraphicsBuffer(CanvasSurface** surfaceHolder,
                                      size_t visibleWidth,
                                      size_t visibleHeight) override;
    virtual bool hasOwnGraphicsBufferMethod() override
    {
        return true;
    }

    void* operator new(size_t size)
    {
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word desc[GC_BITMAP_SIZE(FrameReplacedVideo)] = { 0 };
            FrameReplacedVideo::fillGCDescriptor(desc);
            descr = GC_make_descriptor(desc, GC_WORD_LEN(FrameReplacedVideo));
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
#endif
