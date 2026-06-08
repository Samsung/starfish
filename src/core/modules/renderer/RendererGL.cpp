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

#include "StarfishConfig.h"

#if !defined(STARFISH_HEADLESS)

#include <SkMatrix.h>

#include "Starfish.h"

#include "core/animation/AnimationTask.h"
#include "core/dom/MouseEvent.h"
#include "core/dom/KeyboardEvent.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"
#include "core/modules/threading/Locker.h"
#include "core/modules/message_loop/MessageLoop.h"

#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "core/modules/message_loop/Timer.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/renderer/Renderer.h"
#include "platform/event/PlatformKeyEventData.h"
#include "core/modules/renderer/RendererFactory.h"

namespace Starfish {

#if defined(STARFISH_ENABLE_TEST)
std::function<void()> g_screenShotCallback;
std::string g_screenShotPath;
class RendererGL;
void screenShotImpl(Renderer* renderer, const char* path,
                    std::function<void()> callback);
void screenShotInRendering(WebView* wv, const char* path,
                           std::function<void()> callback)
{
    g_screenShotCallback = callback;
    g_screenShotPath = path;
    return;
}
#endif

class RendererGL : public Renderer {
public:
    RendererGL(Starfish* starfish, uint32_t width, uint32_t height)
        : Renderer(starfish)
        , m_width(width)
        , m_height(height)
        , m_glPaintingSurface(nullptr)
        , m_didPaintingOrCompositing(true)
        , m_isMouseLbuttonDown(true)
        , m_isKeyDown(true)
        , m_mayNeedsSync(false)
    {
        m_offsetYDueToSoftwareKeyboard = 0;
        m_currentContext = kEmptyContextOrUnknown;

        m_lastMouseX = -1;
        m_lastMouseY = -1;
    }

    void ensureCompositorContext()
    {
        STARFISH_ASSERT(!m_compositorContext);
        m_compositorContext = Compositor::initCompositorContext(this);
    }

    virtual uint32_t width() override
    {
        return m_width;
    }

    virtual uint32_t height() override
    {
        return m_height;
    }

    virtual void resizeTo(uint32_t w, uint32_t h) override
    {
        if (w != m_width || h != m_height) {
            m_width = w;
            m_height = h;
            Renderer::resizeTo(w, h);
        }
    }

    virtual void destroy() override
    {
        if (m_glPaintingSurface) {
            m_glPaintingSurface->detachNativeBuffer();
            m_glPaintingSurface = nullptr;
        }
        Renderer::destroy();
    }

    virtual RenderResult rendering() override
    {
        if (UNLIKELY(!canRendering())) {
            return RenderResult();
        }

        if (!m_compositorContext) {
            // calling makeCurrent will create compositor context
            makeCurrent();
            STARFISH_ASSERT(m_compositorContext);
        }

        m_compositorContext->willRendering();
        if (m_renderingPrepareCallback) {
            RenderInfo renderInfo = m_renderingPrepareCallback();
#if defined(PORT_BACKEND_GL_WITH_EXTERNAL_TBM)
            m_compositorContext->prepareExternalSurface(
                renderInfo.updatedBufferAddress);
#endif
        }

        RenderResult ret = Renderer::rendering();
        if (ret.didPaintingOrCompositing) {
            if (webView()->didCompositeBefore()) {
            } else {
                m_glPaintingSurface->unmapBufferAndNotifyUpdatedRegion(
                    (int)ret.updateRect.x(), (int)ret.updateRect.y(),
                    (int)ret.updateRect.width(), (int)ret.updateRect.height());
                float oldDPR = webView()->screenInfo().devicePixelRatio;
                webView()->mutableScreenInfo().devicePixelRatio = 1;
                Compositor* c =
                    Compositor::create3D(webView(), m_compositorContext);
                c->clearColor(Unit::Color(0, 0, 0, 0));
                c->drawSurface(m_glPaintingSurface,
                               Unit::Rect(0, 0, width(), height()));
                delete c;
                webView()->mutableScreenInfo().devicePixelRatio = oldDPR;
            }
            m_compositorContext->didRendering();
            swapBuffers();
#if defined(PORT_BACKEND_GL_WITH_EXTERNAL_TBM)
            m_compositorContext->flushExternalSurface(
                m_surfaceFlushCallback, ret.didPaintingOrCompositing);
#endif
        } else {
#if !defined(STARFISH_ENABLE_TEST)
            if (shouldDrawOnEveryRenderingCallback()) {
                // We should draw every frame in GL backend for non-buffer mode
                if (webView()->didCompositeBefore()) {
                    m_webView->markNeedsCompositeConsiderInRendering();
                    Renderer::rendering();
                } else {
                    float oldDPR = webView()->screenInfo().devicePixelRatio;
                    webView()->mutableScreenInfo().devicePixelRatio = 1;
                    Compositor* c =
                        Compositor::create3D(webView(), m_compositorContext);
                    c->clearColor(Unit::Color(0, 0, 0, 0));
                    if (m_glPaintingSurface != nullptr) {
                        c->drawSurface(m_glPaintingSurface,
                                       Unit::Rect(0, 0, width(), height()));
                    }
                    delete c;
                    webView()->mutableScreenInfo().devicePixelRatio = oldDPR;
                }
                m_compositorContext->didRendering();
                swapBuffers();
            }
#endif
#if defined(PORT_BACKEND_GL_WITH_EXTERNAL_TBM)
            m_compositorContext->flushExternalSurface(m_surfaceFlushCallback,
                                                      false);
#endif
        }

#if defined(STARFISH_ENABLE_TEST)
        if (g_screenShotCallback) {
            screenShotImpl(this, g_screenShotPath.data(), g_screenShotCallback);
            g_screenShotCallback = nullptr;
        }
#endif
        return ret;
    }

    virtual bool shouldDrawOnEveryRenderingCallback()
    {
        // if there is no setNeedsRenderingCallbask,
        // we can skip drawing at rendering
        return m_setNeedsRenderingCallback != nullptr;
    }

    virtual Canvas* preparePainting() override
    {
        LongTaskFinder p("RendererGL::preparePainting", 1);

        float DPR = webView()->screenInfo().devicePixelRatio;
        if (!m_glPaintingSurface) {
            webView()->setNeedsFullRepainting();
            m_glPaintingSurface =
                CanvasSurface::create(this, width() / DPR, height() / DPR);
        }
        if (m_glPaintingSurface->attachNativeBuffer(width() / DPR,
                                                    height() / DPR)) {
            webView()->setNeedsFullRepainting();
        }
        return Canvas::create(webView(), m_glPaintingSurface);
    }

    virtual void willCompositing() override
    {
        makeCurrent();
        if (m_glPaintingSurface) {
            STARFISH_LOG_INFO(
                "RendererGL::willCompositing - remove "
                "m_glPaintingSurface");
            m_glPaintingSurface->detachNativeBuffer();
            m_glPaintingSurface = nullptr;
        }
    }

    virtual Compositor* prepareCompositor() override
    {
        LongTaskFinder p("RendererGL::prepareCompositor", 1);
        if (m_glPaintingSurface) {
            STARFISH_LOG_INFO(
                "RendererGL::prepareCompositor - remove "
                "m_glPaintingSurface");
            m_glPaintingSurface->detachNativeBuffer();
            m_glPaintingSurface = nullptr;
        }
        return Compositor::create3D(webView(), m_compositorContext);
    }

    virtual bool makeCurrent() override
    {
        if (m_isDestroyed) {
            return false;
        }
        m_onMakeCurrent(this);
        m_currentContext = kEmptyContextOrUnknown;
        if (!m_compositorContext) {
            ensureCompositorContext();
        }
        return true;
    }

    virtual void swapBuffers() override
    {
        m_onSwapBuffer(this, m_mayNeedsSync);
        m_mayNeedsSync = false;
    }

    virtual void mayNeedsSync() override
    {
        m_mayNeedsSync = true;
    }

    virtual uintptr_t createSharedContext() override
    {
        return m_onCreateSharedContext(this);
    }

    virtual bool destroyContext(uintptr_t context) override
    {
        return m_onDestroyContext(this, context);
    }

    virtual bool clearCurrentContext() override
    {
        if (!m_onClearCurrentContext(this)) {
            return false;
        }
        m_currentContext = kEmptyContextOrUnknown;
        return true;
    }

    virtual bool makeCurrentWithContext(uintptr_t context) override
    {
        if (!m_onMakeCurrentWithContext(this, context)) {
            return false;
        }
        m_currentContext = context;
        return true;
    }

    virtual uintptr_t getCurrentContext() override
    {
        return m_currentContext;
    }

    virtual void* getProcAddress(const char* name) override
    {
        return m_onGetProcAddress(this, name);
    }

    virtual bool isSupportedExtension(const char* extension) override
    {
        return m_onIsSupportedExtension(this, extension);
    }

    virtual TransformationMatrix screenMatrix() override
    {
        if (m_getScreenMatrix) {
            return m_getScreenMatrix(this);
        }
        return TransformationMatrix::identityMatrix();
    }

    virtual void pause() override
    {
        // release m_glPaintingSurface && m_compositorContext for reducing
        // memory usage
        makeCurrent();

        m_webView->clearDrawnBuffers();

        if (m_glPaintingSurface) {
            m_glPaintingSurface->detachNativeBuffer();
            m_glPaintingSurface = nullptr;
        }

        if (m_compositorContext) {
            m_compositorContext->onIdle();
        }
        Renderer::pause();
    }

    virtual void onClearDrawnBuffers() override
    {
        STARFISH_LOG_INFO("RendererGL::onClearDrawnBuffers");

        if (m_compositorContext) {
            makeCurrent();

            if (m_glPaintingSurface) {
                m_glPaintingSurface->detachNativeBuffer();
                m_glPaintingSurface = nullptr;
            }

            m_compositorContext->onIdle();
        }
    }

    uint32_t m_width;
    uint32_t m_height;
    CanvasSurface* m_glPaintingSurface;
    bool m_didPaintingOrCompositing;
    bool m_isMouseLbuttonDown;
    bool m_isKeyDown;
    bool m_mayNeedsSync;
    float m_lastMouseX, m_lastMouseY;
    int m_offsetYDueToSoftwareKeyboard;
    uintptr_t m_currentContext;
};

Renderer* RendererFactory::createGL(Starfish* starfish, uint32_t width,
                                    uint32_t height)
{
    return new RendererGL(starfish, width, height);
}

} // namespace Starfish
#endif
