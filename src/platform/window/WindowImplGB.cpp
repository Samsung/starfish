/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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
#ifdef PORT_WINDOW_BACKEND_GB

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
#include "platform/window/PlatformWindow.h"
#include "platform/window/PlatformWindowFactory.h"

#ifdef STARFISH_ENABLE_TEST
extern Starfish::CanvasSurface* g_surfaceForScreehShot;
#endif

namespace Starfish {

#ifdef STARFISH_ENABLE_TEST
void screenShotInRendering(WebView*, char const*, std::function<void()>)
{
    STARFISH_UNIMPLEMENTED();
}
#endif

class WindowImplGB : public PlatformWindow {
public:
    WindowImplGB(Starfish* starfish, uint32_t width, uint32_t height)
        : PlatformWindow(starfish)
        , m_width(width)
        , m_height(height)
        , m_internalBuffer(nullptr)
        , m_stride(0)
    {
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

    virtual void updateDrawingBufferAddress(void* buf, uint32_t stride) override
    {
        if (buf != nullptr) {
            PlatformWindow::updateDrawingBufferAddress(buf, stride);
            m_stride = stride;
            m_internalBuffer = buf;
        }
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

PlatformWindow* PlatformWindowFactory::createGb(Starfish* starfish,
                                                uint32_t width, uint32_t height)
{
    return new WindowImplGB(starfish, width, height);
}

Canvas* WindowImplGB::preparePainting()
{
#ifdef STARFISH_ENABLE_TEST
    {
        const char* path = getenv("SCREEN_SHOT");
        if (path && strlen(path) && g_fireOnloadEvent) {
            g_surfaceForScreehShot = CanvasSurface::create(
                this, width() / webView()->screenInfo().devicePixelRatio,
                height() / webView()->screenInfo().devicePixelRatio);
            Canvas* c = Canvas::create(webView(), g_surfaceForScreehShot);
            return c;
        }
    }
#endif
#if !defined(STARFISH_TIZEN_VERSION_5_0)
    RenderInfo renderInfo = m_renderingPrepareCallback();
    updateDrawingBufferAddress(renderInfo.updatedBufferAddress,
                               renderInfo.bufferStride);
#endif
    CanvasSurface* target = CanvasSurface::createCanvasTarget(
        (uint8_t*)m_internalBuffer, m_width, m_height, m_stride);
    Canvas* canvas = Canvas::create(webView(), target);
    return canvas;
}

Compositor* WindowImplGB::prepareCompositor()
{
#ifdef STARFISH_ENABLE_TEST
    {
        const char* path = getenv("SCREEN_SHOT");
        if (path && strlen(path) && g_fireOnloadEvent) {
            g_surfaceForScreehShot = CanvasSurface::create(
                this, width() / webView()->screenInfo().devicePixelRatio,
                height() / webView()->screenInfo().devicePixelRatio);
            return Compositor::create2D(webView(), m_compostiorContext,
                                        g_surfaceForScreehShot);
        }
    }
#endif
#if !defined(STARFISH_TIZEN_VERSION_5_0)
    RenderInfo renderInfo = m_renderingPrepareCallback();
    updateDrawingBufferAddress(renderInfo.updatedBufferAddress,
                               renderInfo.bufferStride);
#endif
    CanvasSurface* target = CanvasSurface::createCanvasTarget(
        (uint8_t*)m_internalBuffer, m_width, m_height, m_stride);
    return Compositor::create2D(webView(), m_compostiorContext, target);
}

} // namespace Starfish
#endif
