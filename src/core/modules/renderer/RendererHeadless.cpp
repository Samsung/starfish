/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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
#ifdef STARFISH_EFL_HEADLESS

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
#include "core/modules/renderer/Renderer.h"
#include "core/modules/renderer/RendererFactory.h"

#ifdef STARFISH_ENABLE_TEST
extern Starfish::CanvasSurface* g_surfaceForScreehShot;
#endif

namespace Starfish {

#ifdef STARFISH_ENABLE_TEST
void screenShotInRendering(WebView*, char const*, std::function<void()>)
{
    STARFISH_UNSUPPORTED("Capture a screenshot for internal test build");
}
#endif

class RendererHeadless : public Renderer {
public:
    RendererHeadless(Starfish* starfish, uint32_t width, uint32_t height)
        : Renderer(starfish)
        , m_width(width)
        , m_height(height)
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
            Renderer::resizeTo(w, h);
        }
    }

    virtual void updateDrawingBufferAddress(void* buf, uint32_t stride) override
    {
    }

    virtual void* drawingBufferAddress() override
    {
        return nullptr;
    }

    virtual Canvas* preparePainting() override;
    virtual Compositor* prepareCompositor() override;

    uint32_t m_width;
    uint32_t m_height;
};

Renderer* RendererFactory::createHeadless(Starfish* starfish, uint32_t width,
                                          uint32_t height)
{
    return new RendererHeadless(starfish, width, height);
}

Canvas* RendererHeadless::preparePainting()
{
    Canvas* canvas = Canvas::create(webView(), (CanvasSurface*)NULL);
    return canvas;
}

Compositor* RendererHeadless::prepareCompositor()
{
    return Compositor::create2D(webView(), m_compostiorContext,
                                (CanvasSurface*)NULL);
}

} // namespace Starfish
#endif
