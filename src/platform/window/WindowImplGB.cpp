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
    WindowImplGB(StarFish* sf, int32_t width, int32_t height)
        : PlatformWindow(sf)
        , m_width(width)
        , m_height(height)
        , m_internalBuffer(nullptr)
        , m_rendingLockMutex(new Mutex())
        , m_didPaintingOrCompositing(true)
    {
        m_renderingAnimator = SIZE_MAX;
        m_renderingIdlerData = nullptr;
        m_lastKeyPressedTimestamp = 0;
        m_offsetYDueToSoftwareKeyboard = 0;

        GC_REGISTER_FINALIZER_NO_ORDER(this,
                                       [](void* obj, void* cd) {
                                           STARFISH_LOG_INFO(
                                               "WindowImplGB::~WindowImplGB\n");
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
        if (w != (int)m_width || h != (int)m_height) {
            m_width = w;
            m_height = h;
            PlatformWindow::resizeTo(w, h);
        }
    }

    virtual void updateDrawingBufferAddress(void* buf, uint32_t width,
                                            uint32_t height, uint32_t stride)
    {
        PlatformWindow::updateDrawingBufferAddress(buf, width, height, stride);
        m_stride = stride;
        m_internalBuffer = buf;
        updateCairoVariables();
    }

    virtual void* unwrap()
    {
        return nullptr;
    }

    void updateCairoVariables()
    {
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

    virtual void* drawingBufferAddress()
    {
        return m_internalBuffer;
    }

    virtual void clearResources();
    virtual Canvas* preparePainting();
    virtual Compositor* prepareCompositor();

    uint32_t m_width;
    uint32_t m_height;
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
                Locker<Mutex> l(*((WindowImplGB*)wnd)->m_rendingLockMutex);
                bool drawingBufferUpdated = wnd->rendering();
                ((WindowImplGB*)wnd)->m_renderingAnimator = SIZE_MAX;
                ((WindowImplGB*)wnd)->m_renderingIdlerData = nullptr;
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
    if (m_renderingAnimator != SIZE_MAX) {
        starFish()->messageLoop()->removeIdler(m_renderingAnimator);
        m_renderingAnimator = SIZE_MAX;
        GC_FREE(m_renderingIdlerData);
    }

    webView()->clearStackingContext(false);
}
} // namespace StarFish
#endif
