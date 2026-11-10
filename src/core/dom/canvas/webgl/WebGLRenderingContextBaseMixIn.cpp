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
#include "core/util/debug/Trace.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "core/modules/renderer/Renderer.h"
#include "core/dom/canvas/webgl/WebGLExtensions.h"

#include <cmath>

namespace Starfish {

// WebGL 1.0 §2.2: when the requested drawing buffer size cannot be
// satisfied, a smaller drawing buffer shall be created, its dimensions being
// implementation dependent. The per-axis texture size limit alone still lets
// a page ask for a buffer of several GiB (color plus depth/stencil), which
// exhausts memory on low-end devices, so the area is bounded as well.
static constexpr uint64_t kMaxDrawingBufferPixels = 4096 * 4096;

static void clampDrawingBufferArea(uint32_t& width, uint32_t& height)
{
    const uint64_t area = static_cast<uint64_t>(width) * height;
    if (area <= kMaxDrawingBufferPixels) {
        return;
    }
    // Keep the aspect ratio; flooring keeps the result within the bound.
    const double scale =
        std::sqrt(static_cast<double>(kMaxDrawingBufferPixels) / area);
    width = std::max<uint32_t>(1, static_cast<uint32_t>(width * scale));
    height = std::max<uint32_t>(1, static_cast<uint32_t>(height * scale));
}

WebGLRenderingContextBaseMixIn::WebGLRenderingContextBaseMixIn(
    HTMLCanvasElement* ownerHTMLCanvasElement)
    : CanvasRenderingContext(ownerHTMLCanvasElement->executionContext())
    , m_ownerHTMLCanvasElement(ownerHTMLCanvasElement)
    , m_canvasSurface(nullptr)
    , m_isContextAttributesChecked(false)
{
    // bdwgc keeps one finalizer per object and a later registration replaces
    // this one, so subclasses add their cleanup through
    // releaseOutOfHeapMembers() rather than registering their own.
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            WebGLRenderingContextBaseMixIn* c =
                (WebGLRenderingContextBaseMixIn*)obj;
            c->finalize();
            c->releaseOutOfHeapMembers();
        },
        NULL, NULL, NULL);
}

void WebGLRenderingContextBaseMixIn::initialize()
{
    STARFISH_ASSERT(m_canvasSurface == nullptr);
    STARFISH_ASSERT(!m_context.isValid());
    STARFISH_ASSERT(m_isContextAttributesChecked);

    m_context = GLContext(m_ownerHTMLCanvasElement->webView()->renderer());

    // Create a GL context for this rendering context
    if (!m_context.createSharedContext()) {
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
        this, [](void* holder) {
            static_cast<WebGLRenderingContextBaseMixIn*>(holder)->finalize();
        });

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
    clampDrawingBufferArea(bufferWidth, bufferHeight);

    TRACE_SCOPE(WEBGL, KV(bufferWidth), KV(bufferHeight));

    {
        GLContextScope scope(m_context);

        // The registry reads GL_EXTENSIONS, so it can only be built while a
        // context is current. The constructor runs before m_context exists,
        // and on ports where nothing is current at that point (ANGLE on
        // Windows) getString returned null there and left the registry empty
        // -- every getExtension() answered null and getSupportedExtensions()
        // was empty. FramebufferTexture::create() asks it for
        // EXT_multisampled_render_to_texture, so it is built before the first
        // drawing buffer.
        if (!WebGLExtensionRegistry::instance().isInitialized()) {
            WebGLExtensionRegistry::instance().initialize(
                m_ownerHTMLCanvasElement->webView()->renderer()->gl());
        }

        // Ensure that the framebufferTexture is destroyed and a new one
        // created when invoked in the resize event.
        m_framebufferTexture.reset();
        m_framebufferTexture = std::make_shared<FramebufferTexture>(
            m_ownerHTMLCanvasElement->webView()->renderer());

        STARFISH_ASSERT(m_isContextAttributesChecked);
        m_framebufferTexture->setAttributes(m_frameBufferAttributes);

        // Set the SurfaceCreationScope with a framebufferTexture. When
        // CanvasSurface::create detects that a SurfaceCreationScope is
        // specified, it sets the required information to framebufferTexture.
        SurfaceCreationScope surfaceScope(m_framebufferTexture);
        m_canvasSurface = CanvasSurface::create(
            m_ownerHTMLCanvasElement->webView()->renderer(), bufferWidth,
            bufferHeight, 1, CanvasSurface::PreferUnitedTexture);
        // A drawing buffer that could not be multisampled stays
        // single-sampled for the rest of the context's life, and is reported
        // so (getContextAttributes()).
        m_frameBufferAttributes.antialias =
            m_framebufferTexture->isMultisampled();

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
        // As a GC finalizer this runs from whatever allocation triggered the
        // collection, which may be in the middle of another context's GL
        // calls, so the context current before is made current again.
        Renderer* renderer = m_context.renderer();
        const uintptr_t previous = renderer->getCurrentContext();
        const uintptr_t own = m_context.handle();
        m_context.setCurrent();
        releaseInternalGLObjects();
        m_framebufferTexture.reset();
        m_context.destroy();
        if (previous == Renderer::kEmptyContextOrUnknown || previous == own) {
            renderer->makeCurrent();
        } else {
            renderer->makeCurrentWithContext(previous);
        }
    }
    m_canvasSurface = nullptr;
}

void WebGLRenderingContextBaseMixIn::flushForReadback()
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
    return m_canvasSurface;
}

} // namespace Starfish
#endif
