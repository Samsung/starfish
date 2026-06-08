/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifdef STARFISH_ENABLE_CANVAS

#ifndef __StarfishFrameReplacedCanvas__
#define __StarfishFrameReplacedCanvas__

namespace Starfish {

class FrameReplacedCanvas final : public FrameReplaced {
public:
    FrameReplacedCanvas(Node* node);

    virtual void computeStyleFlags() override
    {
        FrameReplaced::computeStyleFlags();
        m_flags.m_needToEstablishStackingContext = true;
        m_flags.m_needsGraphicsBuffer = true;
    }

    virtual const char* name() override
    {
        return "FrameReplacedCanvas";
    }

    virtual bool isFrameReplacedCanvas()
    {
        return true;
    }

    virtual IntrinsicSize intrinsicSize() override;
    virtual void willCompositeStackingContext(Compositor* c) override;
    virtual void didCompositeStackingContext(Compositor* c) override;

    virtual Optional<CanvasSurface*> contentSurface() override;

    void* operator new(size_t size)
    {
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word desc[GC_BITMAP_SIZE(FrameReplacedCanvas)] = { 0 };
            FrameReplacedCanvas::fillGCDescriptor(desc);
            descr = GC_make_descriptor(desc, GC_WORD_LEN(FrameReplacedCanvas));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        FrameReplaced::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(FrameReplacedCanvas, m_emptySurface));
    }

private:
    CanvasSurface* m_emptySurface;
};
} // namespace Starfish
#endif
#endif
