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
#include "core/modules/worker/util/Trace.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "platform/canvas/webgl/GLES.h"
#include "platform/canvas/webgl/XGLPlatform.h"
#include "platform/canvas/webgl/XGLUtil.h"
#include "platform/window/PlatformWindow.h"

namespace Starfish {

WebGLRenderingContextBaseMixIn::WebGLRenderingContextBaseMixIn(
    HTMLCanvasElement* ownerHTMLCanvasElement)
    : CanvasRenderingContext(ownerHTMLCanvasElement->executionContext())
    , m_ownerHTMLCanvasElement(ownerHTMLCanvasElement)
    , m_canvasSurface(nullptr)
{
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
    STARFISH_ASSERT(!m_context.isValid());

    if (!XGLPlatform::instance()->isValid()) {
        m_ownerHTMLCanvasElement->webView()->platformWindow()->glMakeCurrent();
        XGLUtil::initXGLPlatform();
    }

    // Create a GL context for this rendering context
    if (!m_context.create(true)) {
        STARFISH_LOG_ERROR("GLContext creation has failed.");
    }

    // Create a surface for this rendering context
    resetSurface();

    // NOTE: Register a disposer to ensure that it's invoked also when a
    // document, which owns this element, is disposed. We should not only rely
    // on the GC finalizer to release GL resources. The finalizer may be invoked
    // after the GL is disconnected (terminated) from the native display, and
    // using any GL APIs inside will result in an error at the time.
    m_ownerHTMLCanvasElement->window()->registerDisposer(
        this, [this]() { finalize(); });

    m_ownerHTMLCanvasElement->setNeedsComposite();
}

void WebGLRenderingContextBaseMixIn::resetSurface()
{
    STARFISH_ASSERT(m_context.isValid());

    uint32_t bufferWidth, bufferHeight;
    bufferWidth = bufferHeight = 0;

    // In terms of Surface, a canvas element has two dimensions: the size of the
    // drawing buffer (the number of pixels on the canvas) and the display size
    // of the canvas. The display size is affected by CSS. The value we need to
    // set for FramebufferTexture here is the drawing buffer size.

    // TODO: check that the value returned by calculateDimension is fit for
    // the above requirement.

    // calculateDimension also handles that HTMLCanvasElement.width and .height
    // values less than 1 are treated as 1. A 0x0 canvas will yield a 1x1
    // drawingBufferWidth/Height. (Refs: 2.2 The Drawing Buffer)
    calculateDimension(bufferWidth, bufferHeight,
                       m_ownerHTMLCanvasElement->width(),
                       m_ownerHTMLCanvasElement->height());

    TRACE_SCOPE(WEBGL, KV(bufferWidth), KV(bufferHeight));

    {
        GLContextScope scope(m_context);

        // Ensure that the framebufferTexture is destroyed and a new one
        // created when invoked in the resize event.
        m_framebufferTexture.reset();
        m_framebufferTexture = std::make_shared<FramebufferTexture>();

        // Set the SurfaceCreationScope with a framebufferTexture. When
        // CanvasSurface::create detects that a SurfaceCreationScope is
        // specified, it sets the required information to framebufferTexture.
        SurfaceCreationScope surfaceScope(m_framebufferTexture);
        m_canvasSurface = CanvasSurface::create(
            m_ownerHTMLCanvasElement->webView()->platformWindow(), bufferWidth,
            bufferHeight, 1, CanvasSurface::CanvasElement);

        // Seeing CompositorGL::initCompositorContextGl, by default a surface is
        // mapped to u,v coordinates that are set to the opposite of the y-axis
        // of the screen coordinates. This results in that m_canvasSurface is
        // rendered upside down. We here set "FlipY is Needed" so that the
        // compositor can flip the surface to render it correctly.
        m_canvasSurface->setFlipYNeeded(true);

        TRACE(WEBGL, "context", &m_context, "FBO (CanvasSurface)",
              m_framebufferTexture->fbo());
    }
}

void WebGLRenderingContextBaseMixIn::finalize()
{
    if (m_context.isValid()) {
        // Unlike EGL, EvasGL requires context setting before resource release.
        m_context.setCurrent();
        m_framebufferTexture.reset();
        m_context.destory();
    }
    m_canvasSurface = nullptr;
}

void WebGLRenderingContextBaseMixIn::flush()
{
}

void WebGLRenderingContextBaseMixIn::onResize()
{
    TRACE_SCOPE(WEBGL);

    resetSurface();

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
