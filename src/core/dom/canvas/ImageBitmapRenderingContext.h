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

#ifndef __StarfishImageBitmapRenderingContext__
#define __StarfishImageBitmapRenderingContext__

#ifdef STARFISH_ENABLE_CANVAS

#include "core/dom/canvas/CanvasRenderingContext.h"

namespace Starfish {

class HTMLCanvasElement;
class ImageBitmapRenderingContext : public CanvasRenderingContext {
public:
    ImageBitmapRenderingContext(HTMLCanvasElement* canvasElement);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isImageBitmapRenderingContext() const override;
    virtual void initialize() override
    {
        STARFISH_UNIMPLEMENTED();
    }

    virtual void flush() override
    {
        STARFISH_UNIMPLEMENTED();
    }

    virtual void onResize() override
    {
        STARFISH_UNIMPLEMENTED();
    }

    virtual CanvasSurface* surface() override
    {
        STARFISH_UNIMPLEMENTED();
        return nullptr;
    }

    HTMLCanvasElement* canvas()
    {
        return m_ownerHTMLCanvasElement;
    }

    void* operator new(size_t size)
    {
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word desc[GC_BITMAP_SIZE(ImageBitmapRenderingContext)] = { 0 };
            ImageBitmapRenderingContext::fillGCDescriptor(desc);
            descr = GC_make_descriptor(
                desc, GC_WORD_LEN(ImageBitmapRenderingContext));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        CanvasRenderingContext::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(ImageBitmapRenderingContext,
                                        m_ownerHTMLCanvasElement));
    }

private:
    HTMLCanvasElement* m_ownerHTMLCanvasElement;
};
} // namespace Starfish

#endif
#endif
