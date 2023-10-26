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

#if defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#include "StarfishConfig.h"
#include "WebGLRenderingContextBaseMixIn.h"
#include "core/modules/canvas/Canvas.h"
#include "core/dom/canvas/HTMLCanvasElement.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "platform/canvas/webgl/GLES.h"

namespace Starfish {

WebGLRenderingContextBaseMixIn::WebGLRenderingContextBaseMixIn(
    HTMLCanvasElement* ownerHTMLCanvasElement)
    : CanvasRenderingContext(ownerHTMLCanvasElement->executionContext())
    , m_ownerHTMLCanvasElement(ownerHTMLCanvasElement)
    , m_canvasSurface(nullptr)
{
    initialize();
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            WebGLRenderingContextBaseMixIn* c =
                (WebGLRenderingContextBaseMixIn*)obj;
            c->finalize();
        },
        NULL, NULL, NULL);
}

void WebGLRenderingContextBaseMixIn::initialize()
{
    STARFISH_ASSERT(m_canvasSurface == nullptr);

    m_framebufferTexture = std::make_shared<FramebufferTexture>();

    uint32_t width, height;
    calculateDimension(width, height, m_ownerHTMLCanvasElement->width(),
                       m_ownerHTMLCanvasElement->height());

    // Create a GL context for this rendering context
    {
        if (!m_context.create(true)) {
            STARFISH_LOG_ERROR("GLContext creation has failed.");
        }
    }

    // Create a surface for this rendering context
    {
        GLContextScope scope(m_context);
        SurfaceCreationScope surfaceScope(m_framebufferTexture);
        m_canvasSurface = CanvasSurface::create(
            m_ownerHTMLCanvasElement->webView()->platformWindow(), width,
            height, 1, CanvasSurface::CanvasElement);
    }

    // NOTE: Register a disposer to ensure that it's invoked also when a
    // document, which owns this element, is disposed. We should not only rely
    // on the GC finalizer to release GL resources. The finalizer may be invoked
    // after the GL is disconnected (terminated) from the native display, and
    // using any GL APIs inside will result in an error at the time.
    m_ownerHTMLCanvasElement->window()->registerDisposer(
        this, [this]() { finalize(); });
}

void WebGLRenderingContextBaseMixIn::finalize()
{
    if (m_context.isValid()) {
        // Unlike EGL, EvasGL requires context setting before resource release.
        m_context.setCurrent();
        m_framebufferTexture.reset();
        m_context.destory();
    }
}

void WebGLRenderingContextBaseMixIn::flush()
{
}

void WebGLRenderingContextBaseMixIn::onResize()
{
    STARFISH_UNIMPLEMENTED();
    m_ownerHTMLCanvasElement->setNeedsComposite();
}

CanvasSurface* WebGLRenderingContextBaseMixIn::surface()
{
    // NOTE: "FrameReplacedCanvas::willCompositeStackingContext" checks whether
    // there is a surface on a CanvasElement. If a valid surface is returned, it
    // requests filling the surface using 'flush()', and then unmaps the buffer
    // of the surface.
    return m_canvasSurface;
}

} // namespace Starfish
#endif
