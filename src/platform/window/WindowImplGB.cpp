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

#ifdef STARFISH_ENABLE_TEST
extern bool g_fireOnloadEvent;
extern StarFish::CanvasSurface* g_surfaceForScreehShot;
#endif

namespace StarFish {

#ifdef STARFISH_ENABLE_TEST
void screenShotInRendering(StarFish*, char const*, std::function<void()>)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}
#endif

class WindowImplGB : public PlatformWindow {
public:
    WindowImplGB(StarFish* sf, int32_t width, int32_t height)
        : PlatformWindow(sf)
        , m_width(width)
        , m_height(height)
        , m_internalBuffer(nullptr)
        , m_stride(0)
    {
    }

    virtual int32_t width() override
    {
        return m_width;
    }

    virtual int32_t height() override
    {
        return m_height;
    }

    virtual void resizeTo(int w, int h) override
    {
        if (w != (int)m_width || h != (int)m_height) {
            m_width = w;
            m_height = h;
            PlatformWindow::resizeTo(w, h);
        }
    }

    virtual void updateDrawingBufferAddress(void* buf, uint32_t width,
                                            uint32_t height,
                                            uint32_t stride) override
    {
        if (buf != nullptr) {
            PlatformWindow::updateDrawingBufferAddress(buf, width, height,
                                                       stride);
            m_stride = stride;
            m_internalBuffer = buf;
        }
    }

    virtual void* unwrap() override
    {
        return nullptr;
    }

    virtual void* drawingBufferAddress() override
    {
        return m_internalBuffer;
    }

    virtual Canvas* preparePainting() override;
    virtual Compositor* prepareCompositor() override;

    uint32_t m_width;
    uint32_t m_height;
    void* m_internalBuffer;
    size_t m_stride;
};

PlatformWindow* PlatformWindow::create(StarFish* sf, int width, int height)
{
    return new WindowImplGB(sf, width, height);
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
            Canvas* c = Canvas::create(starFish(), g_surfaceForScreehShot);
            return c;
        }
    }
#endif
    CanvasSurface* target = CanvasSurface::createCanvasTarget(
        (uint8_t*)m_internalBuffer, m_width, m_height, m_stride);
    Canvas* canvas = Canvas::create(starFish(), target);
    return canvas;
}

Compositor* WindowImplGB::prepareCompositor()
{
#ifdef STARFISH_ENABLE_TEST
    {
        const char* path = getenv("SCREEN_SHOT");
        if (path && strlen(path) && g_fireOnloadEvent) {
            g_surfaceForScreehShot = CanvasSurface::create(
                this, width() / starFish()->screenInfo().devicePixelRatio,
                height() / starFish()->screenInfo().devicePixelRatio);
            return Compositor::create2D(starFish(), m_compostiorContext,
                                        g_surfaceForScreehShot);
        }
    }
#endif
    CanvasSurface* target = CanvasSurface::createCanvasTarget(
        (uint8_t*)m_internalBuffer, m_width, m_height, m_stride);
    return Compositor::create2D(starFish(), m_compostiorContext, target);
}

} // namespace StarFish
#endif
