/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishWebGLRenderingContextBaseMixIn__
#define __StarfishWebGLRenderingContextBaseMixIn__

#if defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#include "core/dom/canvas/CanvasRenderingContext.h"
#include "platform/canvas/webgl/GLUtil.h"
#include "platform/canvas/webgl/GLContext.h"
#include <memory>

namespace Starfish {

class Canvas;
class CanvasSurface;

class WebGLRenderingContextBaseMixIn : public CanvasRenderingContext {
public:
    WebGLRenderingContextBaseMixIn(HTMLCanvasElement* ownerHTMLCanvasElement);
    virtual ~WebGLRenderingContextBaseMixIn()
    {
    }

    void initialize() override;
    void flush() override;
    void onResize() override;
    CanvasSurface* surface() override;

    void resetSurface();
    void finalize();

    void* operator new(size_t size)
    {
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word desc[GC_BITMAP_SIZE(WebGLRenderingContextBaseMixIn)] = {
                0
            };
            WebGLRenderingContextBaseMixIn::fillGCDescriptor(desc);
            descr = GC_make_descriptor(
                desc, GC_WORD_LEN(WebGLRenderingContextBaseMixIn));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new[](size_t size) = delete;

    HTMLCanvasElement* canvas()
    {
        return m_ownerHTMLCanvasElement;
    }

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        CanvasRenderingContext::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(WebGLRenderingContextBaseMixIn,
                                        m_ownerHTMLCanvasElement));
        GC_set_bit(desc, GC_WORD_OFFSET(WebGLRenderingContextBaseMixIn,
                                        m_canvasSurface));
    }
    HTMLCanvasElement* m_ownerHTMLCanvasElement;
    CanvasSurface* m_canvasSurface;
    std::shared_ptr<FramebufferTexture> m_framebufferTexture;
    GLContext m_context;
};
} // namespace Starfish
#endif
#endif
