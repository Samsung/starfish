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

#if defined(PORT_GRAPHIC_BACKEND_EFL_CAIRO)
#include <cairo.h>
#endif
#if defined(PORT_GRAPHIC_BACKEND_EFL_SKIA)
#include "SkCanvas.h"
#include "SkSurface.h"
#endif

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
        , m_didPaintingOrCompositing(true)
    {
        m_renderingAnimator = SIZE_MAX;
        m_lastKeyPressedTimestamp = 0;
        m_offsetYDueToSoftwareKeyboard = 0;
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
        if (buf != nullptr) {
            PlatformWindow::updateDrawingBufferAddress(buf, width, height,
                                                       stride);
            m_stride = stride;
            m_internalBuffer = buf;
#if defined(PORT_GRAPHIC_BACKEND_EFL_CAIRO)
            updateCairoVariables();
#endif
#if defined(PORT_GRAPHIC_BACKEND_EFL_SKIA)
            updateSkiaVariables();
#endif
        }
    }

    virtual void* unwrap()
    {
        return nullptr;
    }

#if defined(PORT_GRAPHIC_BACKEND_EFL_CAIRO)
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
        cairo_surface_set_device_scale(
            m_surface, m_starFish->screenInfo().devicePixelRatio,
            m_starFish->screenInfo().devicePixelRatio);
        m_cairo = cairo_create(m_surface);
    }
#endif

#if defined(PORT_GRAPHIC_BACKEND_EFL_SKIA)
    void updateSkiaVariables()
    {
        SkImageInfo info = SkImageInfo::MakeN32Premul(m_width, m_height);
        m_surface =
            SkSurface::MakeRasterDirect(info, m_internalBuffer, m_stride);
        m_skia = m_surface->getCanvas();
    }
#endif
    virtual void* drawingBufferAddress()
    {
        return m_internalBuffer;
    }

    virtual void setNeedsRendering() override;

    virtual void clearResources();
    virtual Canvas* preparePainting();
    virtual Compositor* prepareCompositor();

    uint32_t m_width;
    uint32_t m_height;
    size_t m_renderingAnimator;
    void* m_internalBuffer;
    size_t m_stride;
    bool m_didPaintingOrCompositing;

    float m_lastMouseX, m_lastMouseY;
    bool m_isMouseLbuttonDown;
    bool m_isKeyDown;
    bool m_canRendering;
    uint32_t m_lastClickedTimestamp;
    uint32_t m_clickedCount;
    uint32_t m_lastKeyPressedTimestamp;
    int m_offsetYDueToSoftwareKeyboard;
#if defined(PORT_GRAPHIC_BACKEND_EFL_CAIRO)
    cairo_surface_t* m_surface;
    cairo_t* m_cairo;
#endif
#if defined(PORT_GRAPHIC_BACKEND_EFL_SKIA)
    sk_sp<SkSurface> m_surface;
    SkCanvas* m_skia;
#endif
};

PlatformWindow* PlatformWindow::create(StarFish* sf, void* win, int width,
                                       int height)
{
    auto wnd = new WindowImplGB(sf, width, height);
    wnd->m_starFish = sf;
    return wnd;
}

void WindowImplGB::setNeedsRendering()
{
    WindowImplGB* wnd = this;

    // refresh rendering animator
    if (wnd->m_renderingAnimator != SIZE_MAX) {
        starFish()->messageLoop()->removeIdler(m_renderingAnimator);
        wnd->m_renderingAnimator = SIZE_MAX;
    }

    wnd->m_renderingAnimator = starFish()->messageLoop()->addIdler(
        nullptr,
        [](size_t handle, void* data) {
            WindowImplGB* wnd = (WindowImplGB*)data;
            if (!wnd->starFish()) {
                return;
            }
            ((WindowImplGB*)wnd)->m_renderingAnimator = SIZE_MAX;
            StarFishEnterer enter(wnd->starFish());
            wnd->rendering();
        },
        starFish()->platformWindow());
}

Canvas* WindowImplGB::preparePainting()
{
#ifdef STARFISH_ENABLE_TEST
    {
        const char* path = getenv("SCREEN_SHOT");
        if (path && strlen(path) && g_fireOnloadEvent) {
            g_surfaceForScreehShot = CanvasSurface::create(
                this, width() / starFish()->screenInfo().devicePixelRatio,
                height() / starFish()->screenInfo().devicePixelRatio);
            g_imgBufferForScreehShot =
                (unsigned char*)g_surfaceForScreehShot->unwrap();
            Canvas* c = Canvas::create(starFish(), g_surfaceForScreehShot);
            return c;
        }
    }
#endif

#if defined(PORT_GRAPHIC_BACKEND_EFL_CAIRO)
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
#endif
#if defined(PORT_GRAPHIC_BACKEND_EFL_SKIA)
    struct dummy {
        SkCanvas* canvas;
        sk_sp<SkSurface> surface;
        int w;
        int h;
    };
    dummy* d = new dummy;

    d->canvas = m_skia;
    d->surface = m_surface;
    d->w = width() + starFish()->posX();
    d->h = height() + starFish()->posY();
#endif
    Canvas* canvas = Canvas::createDirect(starFish(), d);
    delete d;

    return canvas;
}

Compositor* WindowImplGB::prepareCompositor()
{
#if defined(PORT_GRAPHIC_BACKEND_EFL_CAIRO)
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

#endif
#if defined(PORT_GRAPHIC_BACKEND_EFL_SKIA)
    struct dummy {
        SkCanvas* canvas;
        sk_sp<SkSurface> surface;
        int w;
        int h;
    } d;
    d.canvas = m_skia;
    d.surface = m_surface;
    d.w = width();
    d.h = height();
    return Compositor::create(starFish(), &d);
#endif
    return nullptr;
}

void WindowImplGB::clearResources()
{
    if (m_renderingAnimator != SIZE_MAX) {
        starFish()->messageLoop()->removeIdler(m_renderingAnimator);
        m_renderingAnimator = SIZE_MAX;
    }
    webView()->clearStackingContext(false);
}
} // namespace StarFish
#endif
