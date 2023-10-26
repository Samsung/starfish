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

#if defined(PORT_WINDOW_BACKEND_GL)

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
#include "platform/window/PlatformWindow.h"
#include "platform/event/PlatformKeyEventData.h"
#include "platform/window/PlatformWindowFactory.h"

namespace Starfish {

#if defined(STARFISH_ENABLE_TEST)
std::function<void()> g_screenShotCallback;
std::string g_screenShotPath;
class WindowImplGL;
void screenShotImpl(PlatformWindow* wnd, const char* path,
                    std::function<void()> callback);
void screenShotInRendering(WebView* wv, const char* path,
                           std::function<void()> callback)
{
    g_screenShotCallback = callback;
    g_screenShotPath = path;
    return;
}
#endif

class WindowImplGL : public PlatformWindow {
public:
    WindowImplGL(Starfish* starfish, uint32_t width, uint32_t height)
        : PlatformWindow(starfish)
        , m_width(width)
        , m_height(height)
        , m_glPaintingSurface(nullptr)
        , m_didPaintingOrCompositing(true)
        , m_isMouseLbuttonDown(true)
        , m_isKeyDown(true)
        , m_mayNeedsSync(false)
    {
        m_offsetYDueToSoftwareKeyboard = 0;

        m_lastMouseX = -1;
        m_lastMouseY = -1;
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
            PlatformWindow::resizeTo(w, h);
        }
    }

    virtual void destroy() override
    {
        PlatformWindow::destroy();

        if (m_glPaintingSurface) {
            m_glPaintingSurface->detachNativeBuffer();
            m_glPaintingSurface = nullptr;
        }
    }

    virtual RenderResult rendering() override
    {
        if (UNLIKELY(!canRendering())) {
            return RenderResult();
        }

        if (!m_compostiorContext) {
            m_compostiorContext = Compositor::initCompositorContext(this);
        }

        m_compostiorContext->willRendering();
#if defined(PORT_BACKEND_GL_WITH_EXTERNAL_TBM)
        {
            RenderInfo renderInfo = m_renderingPrepareCallback();
            m_compostiorContext->prepareExternalSurface(
                renderInfo.updatedBufferAddress);
        }
#endif
        RenderResult ret = PlatformWindow::rendering();
        if (ret.didPaintingOrCompositing) {
            if (webView()->didCompositeBefore()) {
            } else {
                m_glPaintingSurface->unmapBufferAndNotifyUpdatedRegion(
                    (int)ret.updateRect.x(), (int)ret.updateRect.y(),
                    (int)ret.updateRect.width(), (int)ret.updateRect.height());
                float oldDPR = webView()->screenInfo().devicePixelRatio;
                webView()->mutableScreenInfo().devicePixelRatio = 1;
                Compositor* c =
                    Compositor::create3D(webView(), m_compostiorContext);
                c->clearColor(Unit::Color(0, 0, 0, 0));
                c->drawSurface(m_glPaintingSurface,
                               Unit::Rect(0, 0, width(), height()));
                delete c;
                webView()->mutableScreenInfo().devicePixelRatio = oldDPR;
            }
            m_compostiorContext->didRendering();
            glSwapBuffers();
#if defined(PORT_BACKEND_GL_WITH_EXTERNAL_TBM)
            m_compostiorContext->flushExternalSurface(
                m_surfaceFlushCallback, ret.didPaintingOrCompositing);
#endif
        } else {
#if !defined(STARFISH_ENABLE_TEST)
            if (shouldDrawOnEveryRenderingCallback()) {
                // We should draw every frame in GL backend for non-buffer mode
                if (webView()->didCompositeBefore()) {
                    m_webView->markNeedsCompositeConsiderInRendering();
                    PlatformWindow::rendering();
                } else {
                    float oldDPR = webView()->screenInfo().devicePixelRatio;
                    webView()->mutableScreenInfo().devicePixelRatio = 1;
                    Compositor* c =
                        Compositor::create3D(webView(), m_compostiorContext);
                    c->clearColor(Unit::Color(0, 0, 0, 0));
                    if (m_glPaintingSurface != nullptr) {
                        c->drawSurface(m_glPaintingSurface,
                                       Unit::Rect(0, 0, width(), height()));
                    }
                    delete c;
                    webView()->mutableScreenInfo().devicePixelRatio = oldDPR;
                }
                m_compostiorContext->didRendering();
                glSwapBuffers();
            }
#endif
#if defined(PORT_BACKEND_GL_WITH_EXTERNAL_TBM)
            m_compostiorContext->flushExternalSurface(m_surfaceFlushCallback,
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

    virtual Canvas* preparePainting() override;
    virtual void willCompositing() override
    {
        glMakeCurrent();
        if (m_glPaintingSurface) {
            STARFISH_LOG_INFO(
                "WindowImplGL::willCompositing - remove "
                "m_glPaintingSurface");
            m_glPaintingSurface->detachNativeBuffer();
            m_glPaintingSurface = nullptr;
        }
    }

    virtual Compositor* prepareCompositor() override;

    virtual bool glMakeCurrent() override
    {
        if (m_isDestroyed) {
            return false;
        }
        m_glMakeCurrentCallback(this);
        return true;
    }

    virtual void glSwapBuffers() override
    {
        m_glSwapBufferCallback(this, m_mayNeedsSync);
        m_mayNeedsSync = false;
    }

    virtual void glMayNeedsSync() override
    {
        m_mayNeedsSync = true;
    }

    virtual void pause() override
    {
        // release m_glPaintingSurface && m_compostiorContext for reducing
        // memory usage
        glMakeCurrent();

        m_webView->clearDrawnBuffers();

        if (m_glPaintingSurface) {
            m_glPaintingSurface->detachNativeBuffer();
            m_glPaintingSurface = nullptr;
        }

        Compositor::destroyCompositorContext(this, m_compostiorContext);
        m_compostiorContext = nullptr;

        PlatformWindow::pause();
    }

    virtual void onClearDrawnBuffers() override
    {
        STARFISH_LOG_INFO("WindowImplGL::onClearDrawnBuffers");

        if (m_compostiorContext) {
            glMakeCurrent();

            if (m_glPaintingSurface) {
                m_glPaintingSurface->detachNativeBuffer();
                m_glPaintingSurface = nullptr;
            }

            m_compostiorContext->onIdle();
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
};

Canvas* WindowImplGL::preparePainting()
{
    LongTaskFinder p("WindowImplGL::preparePainting", 1);

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

Compositor* WindowImplGL::prepareCompositor()
{
    LongTaskFinder p("WindowImplGL::prepareCompositor", 1);
    if (m_glPaintingSurface) {
        STARFISH_LOG_INFO(
            "WindowImplGL::prepareCompositor - remove "
            "m_glPaintingSurface");
        m_glPaintingSurface->detachNativeBuffer();
        m_glPaintingSurface = nullptr;
    }
    return Compositor::create3D(webView(), m_compostiorContext);
}

PlatformWindow* PlatformWindowFactory::createGl(Starfish* starfish,
                                                uint32_t width, uint32_t height)
{
    return new WindowImplGL(starfish, width, height);
}

} // namespace Starfish
#endif
