/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#ifdef PORT_WINDOW_BACKEND_GB

#include "StarFish.h"
#include <cairo.h>

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
#include "platform/window/PlatformWindow.h"

#if defined(OS_WINDOWS)
#include <Windows.h>
#endif

#ifdef STARFISH_ENABLE_TEST
extern bool g_fireOnloadEvent;
unsigned char* g_imgBufferForScreehShot;
extern StarFish::CanvasSurface* g_surfaceForScreehShot;
#endif

// #define STARFISH_ENABLE_TIMER
namespace StarFish {

struct IdlerData {
    void (*m_fn)(void*);
    void* m_data;
};

class WindowImplGB : public PlatformWindow {
public:
    void releaseNativeResources()
    {
#if defined(STARFISH_TIZEN)
#else
        free(m_internalBuffer);
#endif
    }
    WindowImplGB(StarFish* sf, int32_t width, int32_t height)
        : PlatformWindow(sf)
        , m_width(width)
        , m_height(height)
        , m_internalBuffer(nullptr)
        , m_rendingLockMutex(new Mutex())
        , m_didPaintingOrCompositing(true)
    {
        m_renderingAnimator = 0;
        m_renderingIdlerData = nullptr;
        m_lastKeyPressedTimestamp = 0;
        m_offsetYDueToSoftwareKeyboard = 0;
        prepareBuffer();

        GC_REGISTER_FINALIZER_NO_ORDER(this,
                                       [](void* obj, void* cd) {
                                           STARFISH_LOG_INFO(
                                               "WindowImplGB::~WindowImplGB\n");
                                           WindowImplGB* s = (WindowImplGB*)obj;
                                           s->releaseNativeResources();
                                       },
                                       NULL, NULL, NULL);
    }

    virtual int32_t width() override
    {
#ifdef STARFISH_ENABLE_TEST
        if (getenv("SCREEN_SHOT_WIDTH") &&
            strlen(getenv("SCREEN_SHOT_WIDTH"))) {
            return atoi(getenv("SCREEN_SHOT_WIDTH"));
        }
#endif
        return m_width;
    }

    virtual int32_t height() override
    {
#ifdef STARFISH_ENABLE_TEST
        if (getenv("SCREEN_SHOT_HEIGHT") &&
            strlen(getenv("SCREEN_SHOT_HEIGHT"))) {
            return atoi(getenv("SCREEN_SHOT_HEIGHT"));
        }
#endif
        return m_height;
    }

    virtual void resizeTo(int w, int h)
    {
        free(m_internalBuffer);
        m_internalBuffer = nullptr;
        m_width = w;
        m_height = h;
        prepareBuffer();

        PlatformWindow::resizeTo(w, h);
    }

    virtual void* unwrap()
    {
        // return getCompletedBuffer();
        return nullptr;
    }

    void prepareBuffer()
    {
        if (m_didPaintingOrCompositing) {
#if defined(STARFISH_TIZEN)
            m_internalBuffer = m_starFish->frameBuffer();
#else
            if (m_internalBuffer == nullptr)
                m_internalBuffer =
                    malloc(m_width * m_height * sizeof(uint32_t));
#endif
            m_stride = m_width * 4;
            if (m_cairo) {
                cairo_destroy(m_cairo);
            }
            if (m_surface) {
                cairo_surface_destroy(m_surface);
            }
            m_surface = cairo_image_surface_create_for_data(
                (unsigned char*)m_internalBuffer, CAIRO_FORMAT_ARGB32, m_width,
                m_height, m_stride);
            m_cairo = cairo_create(m_surface);
        }
    }

    void flushBuffer()
    {
        if (m_didPaintingOrCompositing) {
#if defined(STARFISH_TIZEN)
            m_starFish->setNeedsUpdate();

#elif defined(STARFISH_WINDOWS)
            PostMessage(NULL, WM_USER, 0, 0);
#else
            memcpy(m_starFish->frameBuffer(), m_internalBuffer,
                   m_width * m_height * sizeof(uint32_t));
            m_starFish->setNeedsUpdate();
#endif
        }
    }

    virtual void* drawingBufferAddress()
    {
        return m_internalBuffer;
    }
    virtual uint32_t drawingBufferWidth()
    {
        return m_width;
    }
    virtual uint32_t drawingBufferHeight()
    {
        return m_height;
    }
    virtual uint32_t drawingBufferStride()
    {
        return m_stride;
    }

    virtual void clearResources();
    virtual Canvas* preparePainting();
    virtual Compositor* prepareCompositor();

    int32_t m_width;
    int32_t m_height;
    size_t m_renderingAnimator;
    IdlerData* m_renderingIdlerData;
    void* m_internalBuffer;
    size_t m_stride;
    Mutex* m_rendingLockMutex;
    bool m_didPaintingOrCompositing;

    float m_lastMouseX, m_lastMouseY;
    bool m_isMouseLbuttonDown;
    bool m_isKeyDown;
    bool m_canRendering;
    uint32_t m_lastClickedTimestamp;
    uint32_t m_clickedCount;
    uint32_t m_lastKeyPressedTimestamp;
    int m_offsetYDueToSoftwareKeyboard;

    cairo_surface_t* m_surface;
    cairo_t* m_cairo;
};

class CanvasSurfaceGB : public CanvasSurface {
public:
    CanvasSurfaceGB(PlatformWindow* wnd, size_t w, size_t h)
    {
        m_width = w;
        m_height = h;
        m_window = (WindowImplGB*)wnd;

        m_bufferStride = m_imageWidth = m_bufferWidth = m_width = SIZE_MAX;
        m_imageHeight = m_bufferHeight = m_height = SIZE_MAX;

        m_bufferWidth = m_width = -1;
        m_bufferHeight = m_height = -1;
        m_pixelRatio = 1;
        attachNativeBuffer(w, h);

        resize(w, h);
        GC_REGISTER_FINALIZER_NO_ORDER(this,
                                       [](void* obj, void* cd) {
                                           CanvasSurfaceGB* s =
                                               (CanvasSurfaceGB*)obj;
                                           // STARFISH_LOG_INFO("release
                                           // CanvasSurfaceGB %p\n", s);
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
            m_bufferStride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32,
                                                           m_bufferWidth);

            detachNativeBuffer();
            m_buffer = (unsigned char*)malloc(m_bufferWidth * m_bufferHeight *
                                              sizeof(uint32_t));
        }
    }

    virtual void resize(size_t w, size_t h)
    {
        if (m_width != w || m_height != h) {
            m_pixelRatio = 1;

            while ((w / m_pixelRatio > 10000) || (h / m_pixelRatio > 10000)) {
                m_pixelRatio++;
            }

            m_width = w;
            m_height = h;

            m_imageWidth = std::max((size_t)1, m_width / m_pixelRatio);
            m_imageHeight = std::max((size_t)1, m_height / m_pixelRatio);

            STARFISH_RELEASE_ASSERT(m_imageWidth <= m_bufferWidth);
            STARFISH_RELEASE_ASSERT(m_imageHeight <= m_bufferHeight);
        }
    }

    virtual void* unwrap()
    {
        return (void*)m_buffer;
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
    WindowImplGB* m_window;
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
    return new CanvasSurfaceGB(wnd, w, h);
}

PlatformWindow* PlatformWindow::create(StarFish* sf, void* win, int width,
                                       int height)
{
    auto wnd = new WindowImplGB(sf, width, height);
    wnd->m_starFish = sf;

#ifdef STARFISH_ENABLE_TEST
    {
        const char* path = getenv("SCREEN_SHOT");
        const char* hide = getenv("HIDE_WINDOW");

        // AFAIK ,There is no way to hide the window in DALi@linux.(mh.byun)
        /*
        {
            Dali::Application* app = (Dali::Application*)sf->nativeHandle();
            if(app){
                Dali::Window dali_win  = app->GetWindow();
                if ((path && strlen(path)) || (hide && strlen(hide))) {
                    Dali::DevelWindow::Hide(dali_win);
                    // wnd->m_mainView.SetVisible(false);
                } else {
                    Dali::DevelWindow::Show(dali_win);
                    // wnd->m_mainView.SetVisible(true);
                }
            }
        }
        */
    }
#endif
    return wnd;
}

PlatformWindow::~PlatformWindow()
{
    STARFISH_LOG_INFO("PlatformWindow::~PlatformWindow\n");
}

void WebView::setNeedsRendering()
{
    WindowImplGB* wnd = (WindowImplGB*)starFish()->platformWindow();

    // TODO: refresh rendering animator here.

    m_needsRendering = true;
    wnd->m_renderingAnimator = starFish()->messageLoop()->addIdler(
        nullptr,
        [](size_t handle, void* data) {
            WindowImplGB* wnd = (WindowImplGB*)data;
            StarFishEnterer enter(wnd->starFish());
            {
                Locker<Mutex> l(*(wnd)->m_rendingLockMutex);
                wnd->prepareBuffer();
                wnd->m_didPaintingOrCompositing = wnd->rendering();
                wnd->m_renderingAnimator = 0;
                wnd->m_renderingIdlerData = nullptr;
                wnd->flushBuffer();
            }
        },
        starFish()->platformWindow());
}

Canvas* WindowImplGB::preparePainting()
{
#ifdef STARFISH_ENABLE_TEST
    {
        const char* path = getenv("SCREEN_SHOT");
        if (path && strlen(path) && g_fireOnloadEvent) {
            g_surfaceForScreehShot =
                CanvasSurface::create(this, width(), height());
            g_imgBufferForScreehShot =
                (unsigned char*)g_surfaceForScreehShot->unwrap();
            Canvas* c = Canvas::create(starFish(), g_surfaceForScreehShot);
            return c;
        }
    }
#endif

    struct dummy {
        cairo_t* cairo;
        cairo_surface_t* surface;
        int w;
        int h;
    };

    dummy* d = new dummy;

    d->cairo = m_cairo;
    d->surface = m_surface;
    d->w = width();
    d->h = height();

    Canvas* canvas = Canvas::createDirect(starFish(), d);
    delete d;

    return canvas;
}

Compositor* WindowImplGB::prepareCompositor()
{
    struct dummy {
        cairo_t* cairo;
        cairo_surface_t* surface;
        int w;
        int h;
    } d;
    d.cairo = m_cairo;
    d.surface = m_surface;
    d.w = width();
    d.h = height();
    return Compositor::create(starFish(), &d);
}

void WindowImplGB::clearResources()
{
    if (m_renderingAnimator) {
        starFish()->messageLoop()->removeIdler(m_renderingAnimator);
        m_renderingAnimator = 0;
        GC_FREE(m_renderingIdlerData);
    }

    webView()->clearStackingContext(false);
}
} // namespace StarFish
#endif
