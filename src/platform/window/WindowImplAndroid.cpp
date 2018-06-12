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
