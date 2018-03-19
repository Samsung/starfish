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

#include "StarFishConfig.h"
#ifdef PORT_WINDOW_BACKEND_ANDROID

#include "StarFish.h"
#include <cairo.h>
#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>

#include "core/animation/Animation.h"
#include "core/dom/MouseEvent.h"
#include "core/dom/KeyboardEvent.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"
#include "core/modules/threading/Locker.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/threading/Thread.h"

#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "platform/window/PlatformWindow.h"

#include "inc/LWEWebView.h"

using namespace StarFish;

unsigned char* g_androidBitmapAddress;
size_t g_androidBitmapWidth;
size_t g_androidBitmapHeight;
size_t g_androidBitmapStride;

namespace StarFish {

struct IdlerData {
    void (*m_fn)(void*);
    void* m_data;
};

class WindowImplAndroid : public PlatformWindow {
public:
    WindowImplAndroid(StarFish* sf, int32_t width, int32_t height)
        : PlatformWindow(sf)
        , m_width(width)
        , m_height(height)
        , m_rendingLockMutex(new Mutex())
    {
        m_renderingAnimator = 0;
        m_renderingIdlerData = nullptr;
        m_lastKeyPressedTimestamp = 0;
        m_offsetYDueToSoftwareKeyboard = 0;
        m_lastMouseX = m_lastMouseY = 0;
        m_isMouseLbuttonDown = false;

        GC_REGISTER_FINALIZER_NO_ORDER(
            this,
            [](void* obj, void* cd) {
                STARFISH_LOG_INFO("WindowImplAndroid::~WindowImplAndroid\n");
                WindowImplAndroid* s = (WindowImplAndroid*)obj;
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
        onResize();
    }

    virtual void* unwrap()
    {
        return nullptr;
    }

    virtual bool rendering() override
    {
        m_stride = g_androidBitmapStride;
        m_surface = cairo_image_surface_create_for_data(
            (unsigned char*)g_androidBitmapAddress, CAIRO_FORMAT_ARGB32,
            g_androidBitmapWidth, g_androidBitmapHeight, m_stride);

        cairo_surface_set_device_scale(
            m_surface, m_starFish->screenInfo().deviceScaleFactor,
            m_starFish->screenInfo().deviceScaleFactor);

        m_cairo = cairo_create(m_surface);

        bool ret = PlatformWindow::rendering();

        cairo_destroy(m_cairo);
        cairo_surface_destroy(m_surface);

        return ret;
    }

    virtual void clearResources();
    virtual Canvas* preparePainting();
    virtual Compositor* prepareCompositor();

    int32_t m_width;
    int32_t m_height;
    size_t m_renderingAnimator;
    IdlerData* m_renderingIdlerData;
    size_t m_stride;
    Mutex* m_rendingLockMutex;

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

class CanvasSurfaceAndroid : public CanvasSurface {
public:
    CanvasSurfaceAndroid(PlatformWindow* wnd, size_t w, size_t h)
    {
        m_width = w;
        m_height = h;
        m_window = (WindowImplAndroid*)wnd;

        m_bufferStride = m_imageWidth = m_bufferWidth = m_width = SIZE_MAX;
        m_imageHeight = m_bufferHeight = m_height = SIZE_MAX;

        m_bufferWidth = m_width = -1;
        m_bufferHeight = m_height = -1;
        m_pixelRatio = 1;
        attachNativeBuffer(w, h);

        resize(w, h);
        GC_REGISTER_FINALIZER_NO_ORDER(this,
                                       [](void* obj, void* cd) {
                                           CanvasSurfaceAndroid* s =
                                               (CanvasSurfaceAndroid*)obj;
                                           // STARFISH_LOG_INFO("release
                                           // CanvasSurfaceDALI %p\n", s);
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
    WindowImplAndroid* m_window;
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
    return new CanvasSurfaceAndroid(wnd, w, h);
}

PlatformWindow* PlatformWindow::create(StarFish* sf, void* win, int width,
                                       int height)
{
    auto wnd = new WindowImplAndroid(sf, width, height);
    wnd->m_starFish = sf;
    return wnd;
}

PlatformWindow::~PlatformWindow()
{
    STARFISH_LOG_INFO("PlatformWindow::~PlatformWindow\n");
}

void WebView::setNeedsRendering()
{
    WindowImplAndroid* wnd = (WindowImplAndroid*)starFish()->platformWindow();
    m_needsRendering = true;

    if (starFish()->LWEWebView() != nullptr) {
        requestRender(starFish()->LWEWebView());
    }
}

Canvas* WindowImplAndroid::preparePainting()
{
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

Compositor* WindowImplAndroid::prepareCompositor()
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

void WindowImplAndroid::clearResources()
{
    if (m_renderingAnimator) {
        starFish()->messageLoop()->removeIdler(m_renderingAnimator);
        m_renderingAnimator = 0;
        GC_FREE(m_renderingIdlerData);
    }

    webView()->clearStackingContext(false);
}
}

#endif
