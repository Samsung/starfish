/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "StarFishConfig.h"

#if defined(PORT_WINDOW_BACKEND_EFL_HEADLESS)
#include "StarFish.h"

#include "core/dom/Element.h"
#include "core/dom/MouseEvent.h"
#include "core/dom/TouchEvent.h"
#include "core/dom/KeyboardEvent.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "platform/window/PlatformWindow.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/profiling/Profiling.h"
#include "core/dom/CompositionEvent.h"

#include <Ecore.h>

namespace StarFish {

class WindowImplEFLHeadLess : public PlatformWindow {
public:
    WindowImplEFLHeadLess(StarFish* sf, int width, int height)
        : PlatformWindow(sf)
    {
        m_renderingAnimator = nullptr;
        m_isMouseLbuttonDown = false;
        m_isKeyDown = false;
        m_lastClickedTimestamp = 0;
        m_clickedCount = 0;
        m_inRendering = false;
        m_lastKeyPressedTimestamp = 0;
        m_width = width;
        m_height = height;

        GC_REGISTER_FINALIZER_NO_ORDER(
            this,
            [](void* obj, void* cd) {
                STARFISH_LOG_INFO(
                    "WindowImplEFLHeadLess::~WindowImplEFLHeadLess\n");
            },
            NULL, NULL, NULL);
    }

    virtual int32_t width() override
    {
        return m_width;
    }

    virtual int32_t height() override
    {
        return m_height;
    }

    virtual void resizeTo(int w, int h)
    {
        m_width = w;
        m_height = h;
    }

    virtual void* unwrap()
    {
        return nullptr;
    }

    virtual void clearResources();
    virtual Canvas* preparePainting();
    virtual Compositor* prepareCompositor();

    virtual void showSoftwareKeyboardIfPossible()
    {
    }
    virtual void hideSoftwareKeyboardIfPossible()
    {
    }

    virtual bool isIMEEnabledNow()
    {
        return true;
    }

    virtual void onIdle()
    {
        PlatformWindow::onIdle();
    }

    bool rendering()
    {
        m_inRendering = true;
        bool ret = PlatformWindow::rendering();
        m_inRendering = false;
        return ret;
    }

    uintptr_t m_handle;

    float m_lastMouseX, m_lastMouseY;
    bool m_isMouseLbuttonDown;
    bool m_isKeyDown;
    bool m_canRendering;
    bool m_inRendering;
    bool m_isEvasFlushed;
    uint32_t m_lastClickedTimestamp;
    uint32_t m_clickedCount;
    uint32_t m_lastKeyPressedTimestamp;
    uint64_t m_lastRenderingTime;
    int m_width;
    int m_height;
    Ecore_Animator* m_renderingAnimator;
};

class CanvasSurfaceSimple : public CanvasSurface {
public:
    CanvasSurfaceSimple(PlatformWindow* wnd, size_t w, size_t h)
    {
        m_window = wnd;
        m_width = w;
        m_height = h;
        m_imageWidth = m_bufferWidth = m_width = -1;
        m_imageHeight = m_bufferHeight = m_height = -1;
        m_pixelRatio = 1;

        attachNativeBuffer(w, h);
        GC_REGISTER_FINALIZER_NO_ORDER(this,
                                       [](void* obj, void* cd) {
                                           CanvasSurfaceSimple* s =
                                               (CanvasSurfaceSimple*)obj;
                                           s->detachNativeBuffer();
                                       },
                                       NULL, NULL, NULL);
    }

    virtual void detachNativeBuffer()
    {
        free(m_buffer);
        m_buffer = nullptr;
    }

    void attachNativeBuffer(size_t w, size_t h)
    {
        if (m_width != w || m_height != h) {
            m_width = w;
            m_height = h;

            if ((int)w < m_window->starFish()->screenInfo().rect.width()) {
                w += STARFISH_CANVAS_SURFACE_MARGIN;
            }
            if ((int)h < m_window->starFish()->screenInfo().rect.height()) {
                h += STARFISH_CANVAS_SURFACE_MARGIN;
            }

            m_pixelRatio = 1;

            while ((m_width / m_pixelRatio > 20000) ||
                   (m_height / m_pixelRatio > 20000)) {
                m_pixelRatio++;
            }

            m_imageWidth = std::max((size_t)1, m_width / m_pixelRatio);
            m_imageHeight = std::max((size_t)1, m_height / m_pixelRatio);

            m_bufferWidth = std::max((size_t)1, w / m_pixelRatio);
            m_bufferHeight = std::max((size_t)1, h / m_pixelRatio);
            m_bufferStride = m_bufferWidth * 4;

            detachNativeBuffer();
            m_buffer = (unsigned char*)malloc(m_bufferWidth * m_bufferHeight *
                                              sizeof(uint32_t));
        }
    }

    virtual void resize(size_t w, size_t h)
    {
        STARFISH_RELEASE_ASSERT(w <= m_bufferWidth * m_pixelRatio);
        STARFISH_RELEASE_ASSERT(h <= m_bufferHeight * m_pixelRatio);

        m_width = w;
        m_height = h;

        m_pixelRatio = 1;

        while ((m_width / m_pixelRatio > 20000) ||
               (m_height / m_pixelRatio > 20000)) {
            m_pixelRatio++;
        }

        m_imageWidth = std::max((size_t)1, m_width / m_pixelRatio);
        m_imageHeight = std::max((size_t)1, m_height / m_pixelRatio);

        STARFISH_RELEASE_ASSERT(m_imageWidth <= m_bufferWidth);
        STARFISH_RELEASE_ASSERT(m_imageHeight <= m_bufferHeight);
    }

    virtual void* unwrap()
    {
        return nullptr;
    }

    virtual uint8_t* data()
    {
        return m_buffer;
    }

    virtual size_t width()
    {
        return m_width;
    }

    virtual size_t height()
    {
        return m_height;
    }

    virtual size_t bufferWidth()
    {
        return m_bufferWidth;
    }

    virtual size_t bufferHeight()
    {
        return m_bufferHeight;
    }

    virtual size_t imageWidth()
    {
        return m_imageWidth;
    }

    virtual size_t imageHeight()
    {
        return m_imageHeight;
    }

    virtual size_t pixelRatio()
    {
        return m_pixelRatio;
    }

    virtual size_t bufferStride()
    {
        return m_bufferStride;
    }

    virtual void clear()
    {
        size_t end = m_bufferWidth * m_bufferHeight * sizeof(uint32_t);
        memset(m_buffer, 0x00, end);
    }

protected:
    PlatformWindow* m_window;
    unsigned char* m_buffer;
    size_t m_width;
    size_t m_height;
    size_t m_imageWidth;
    size_t m_imageHeight;
    size_t m_bufferWidth;
    size_t m_bufferHeight;
    size_t m_bufferStride;
    size_t m_pixelRatio;
};

CanvasSurface* CanvasSurface::create(PlatformWindow* wnd, size_t w, size_t h)
{
    return new CanvasSurfaceSimple(wnd, w, h);
}

const uint32_t CLICK_REFRESH_DELAY = 400;

PlatformWindow* PlatformWindow::create(StarFish* sf, void* win, int width,
                                       int height)
{
    auto wnd = new WindowImplEFLHeadLess(sf, width, height);
    wnd->m_starFish = sf;
    return wnd;
}

PlatformWindow::~PlatformWindow()
{
    STARFISH_LOG_INFO("PlatformWindow::~PlatformWindow\n");
}

void WebView::setNeedsRendering()
{
    m_needsRendering = true;
    WindowImplEFLHeadLess* wnd =
        (WindowImplEFLHeadLess*)starFish()->platformWindow();

    // refresh rendering animator
    if (wnd->m_renderingAnimator) {
        ecore_animator_del(wnd->m_renderingAnimator);
    }

    wnd->m_renderingAnimator = ecore_animator_add(
        [](void* data) -> Eina_Bool {
            WindowImplEFLHeadLess* wnd = (WindowImplEFLHeadLess*)data;
            StarFishEnterer enter(wnd->starFish());
            wnd->m_renderingAnimator = nullptr;
            return ECORE_CALLBACK_CANCEL;
        },
        wnd);
}

Canvas* WindowImplEFLHeadLess::preparePainting()
{
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

Compositor* WindowImplEFLHeadLess::prepareCompositor()
{
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

void WindowImplEFLHeadLess::clearResources()
{
    if (m_renderingAnimator) {
        ecore_animator_del(m_renderingAnimator);
    }
}
}
#endif
