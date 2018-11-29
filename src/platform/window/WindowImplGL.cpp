/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"

#if defined(PORT_WINDOW_BACKEND_GL)

#include "Starfish.h"

#include "core/animation/Animation.h"
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
        , m_isEGLImageUpdated(true)
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
        if (!m_compostiorContext) {
            m_compostiorContext = Compositor::initCompositorContext(this);
        }

        RenderResult ret = PlatformWindow::rendering();
        if (ret.didPaintingOrCompositing) {
            if (webView()->didCompositeBefore()) {
            } else {
                m_glPaintingSurface->unMapBufferAndNotifyUpdateRegion(
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
            glSwapBuffers();
            glClearEGLImageUpdated();
        }

#if defined(STARFISH_ENABLE_TEST)
        if (g_screenShotCallback) {
            screenShotImpl(this, g_screenShotPath.data(), g_screenShotCallback);
            g_screenShotCallback = nullptr;
        }
#endif
        return ret;
    }

    virtual Canvas* preparePainting() override;
    virtual void willCompositing() override
    {
        glMakeCurrent();
        if (!webView()->hasActiveAnimationExecutor()) {
            if (m_glPaintingSurface) {
                STARFISH_LOG_INFO(
                    "WindowImplGL::willCompositing - remove "
                    "m_glPaintingSurface\n");
                m_glPaintingSurface->detachNativeBuffer();
                m_glPaintingSurface = nullptr;
            }
        }
    }

    virtual Compositor* prepareCompositor() override;

    virtual void glMakeCurrent() override
    {
        m_glMakeCurrentCallback(this);
    }

    virtual void glSwapBuffers() override
    {
        m_glSwapBufferCallback(this, m_isEGLImageUpdated);
    }

    virtual void glEGLImageUpdated() override
    {
        m_isEGLImageUpdated = true;
    }

    virtual void glClearEGLImageUpdated() override
    {
        m_isEGLImageUpdated = false;
    }

    virtual void pause() override
    {
        // release m_glPaintingSurface && m_compostiorContext for reducing
        // memory usage
        glMakeCurrent();

        if (m_glPaintingSurface) {
            m_glPaintingSurface->detachNativeBuffer();
            m_glPaintingSurface = nullptr;
        }

        Compositor::destroyCompositorContext(this, m_compostiorContext);
        m_compostiorContext = nullptr;

        PlatformWindow::pause();
    }

    uint32_t m_width;
    uint32_t m_height;
    CanvasSurface* m_glPaintingSurface;
    bool m_didPaintingOrCompositing;
    bool m_isMouseLbuttonDown;
    bool m_isKeyDown;
    bool m_isEGLImageUpdated;
    float m_lastMouseX, m_lastMouseY;
    int m_offsetYDueToSoftwareKeyboard;
};

Canvas* WindowImplGL::preparePainting()
{
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
    if (!webView()->hasActiveAnimationExecutor()) {
        if (m_glPaintingSurface) {
            STARFISH_LOG_INFO(
                "WindowImplGL::prepareCompositor - remove "
                "m_glPaintingSurface\n");
            m_glPaintingSurface->detachNativeBuffer();
            m_glPaintingSurface = nullptr;
        }
    }
    return Compositor::create3D(webView(), m_compostiorContext);
}

PlatformWindow* PlatformWindow::create(Starfish* starfish, uint32_t width,
                                       uint32_t height)
{
    return new WindowImplGL(starfish, width, height);
}

} // namespace Starfish
#endif
