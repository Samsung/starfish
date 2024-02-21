/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#include <SkMatrix.h>

#include "StarfishConfig.h"
#include "Compositor.h"
#include "core/style/Style.h"
#include "core/style/ComputedStyle.h"
#include "core/dom/Node.h"
#include "Canvas.h"
#include "CompositorFactory.h"

namespace Starfish {

extern int g_portCompositorBackend;

// The if-def statements below are temporary soluation to avoid affecting other
// ports of LWE except flutter. In the future, It will be removed when LWE's all
// ports are changed to a single binary.

Compositor* Compositor::create3D(WebView* starfish, CompositorContext* ctx)
{
    switch (static_cast<PORT_COMPOSITOR_BACKEND>(g_portCompositorBackend)) {
#ifdef PORT_COMPOSITOR_BACKEND_CAIRO
    case PORT_COMPOSITOR_BACKEND::CAIRO:
        return CompositorFactory::create3dCairo(starfish, ctx);
#endif
#ifdef PORT_COMPOSITOR_BACKEND_GL
    case PORT_COMPOSITOR_BACKEND::GL:
        return CompositorFactory::create3dGl(starfish, ctx);
#endif
#ifdef PORT_COMPOSITOR_BACKEND_MOCK
    case PORT_COMPOSITOR_BACKEND::MOCK:
        return CompositorFactory::create3dMock(starfish, ctx);
#endif
    default:
        break;
    }
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return nullptr;
}

Compositor* Compositor::create2D(WebView* starfish, CompositorContext* ctx,
                                 CanvasSurface* surface)
{
    switch (static_cast<PORT_COMPOSITOR_BACKEND>(g_portCompositorBackend)) {
#ifdef PORT_COMPOSITOR_BACKEND_CAIRO
    case PORT_COMPOSITOR_BACKEND::CAIRO:
        return CompositorFactory::create2dCairo(starfish, ctx, surface);
#endif
#ifdef PORT_COMPOSITOR_BACKEND_GL
    case PORT_COMPOSITOR_BACKEND::GL:
        return CompositorFactory::create2dGl(starfish, ctx, surface);
#endif
#ifdef PORT_COMPOSITOR_BACKEND_MOCK
    case PORT_COMPOSITOR_BACKEND::MOCK:
        return CompositorFactory::create2dMock(starfish, ctx, surface);
#endif
    default:
        break;
    }
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return nullptr;
}

CompositorContext* Compositor::initCompositorContext(PlatformWindow* wnd)
{
    switch (static_cast<PORT_COMPOSITOR_BACKEND>(g_portCompositorBackend)) {
#ifdef PORT_COMPOSITOR_BACKEND_CAIRO
    case PORT_COMPOSITOR_BACKEND::CAIRO:
        return CompositorFactory::initCompositorContextCairo(wnd);
#endif
#ifdef PORT_COMPOSITOR_BACKEND_GL
    case PORT_COMPOSITOR_BACKEND::GL:
        return CompositorFactory::initCompositorContextGl(wnd);
#endif
#ifdef PORT_COMPOSITOR_BACKEND_MOCK
    case PORT_COMPOSITOR_BACKEND::MOCK:
        return CompositorFactory::initCompositorContextMock(wnd);
#endif
    default:
        break;
    }
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return nullptr;
}

void Compositor::destroyCompositorContext(PlatformWindow* wnd,
                                          CompositorContext* ctx)
{
    switch (static_cast<PORT_COMPOSITOR_BACKEND>(g_portCompositorBackend)) {
#ifdef PORT_COMPOSITOR_BACKEND_CAIRO
    case PORT_COMPOSITOR_BACKEND::CAIRO:
        return CompositorFactory::destroyCompositorContextCairo(wnd, ctx);
#endif
#ifdef PORT_COMPOSITOR_BACKEND_GL
    case PORT_COMPOSITOR_BACKEND::GL:
        return CompositorFactory::destroyCompositorContextGl(wnd, ctx);
#endif
#ifdef PORT_COMPOSITOR_BACKEND_MOCK
    case PORT_COMPOSITOR_BACKEND::MOCK:
        return CompositorFactory::destroyCompositorContextMock(wnd, ctx);
#endif
    default:
        break;
    }
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
}

uint32_t Compositor::maximumTextureSize()
{
    switch (static_cast<PORT_COMPOSITOR_BACKEND>(g_portCompositorBackend)) {
#ifdef PORT_COMPOSITOR_BACKEND_CAIRO
    case PORT_COMPOSITOR_BACKEND::CAIRO:
        return CompositorFactory::maximumTextureSizeCairo();
#endif
#ifdef PORT_COMPOSITOR_BACKEND_GL
    case PORT_COMPOSITOR_BACKEND::GL:
        return CompositorFactory::maximumTextureSizeGl();
#endif
#ifdef PORT_COMPOSITOR_BACKEND_MOCK
    case PORT_COMPOSITOR_BACKEND::MOCK:
        return CompositorFactory::maximumTextureSizeMock();
#endif
    default:
        break;
    }
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return 0;
}

bool Compositor::supportsFilterEffect(size_t textureWidth, size_t textureHeight)
{
    switch (static_cast<PORT_COMPOSITOR_BACKEND>(g_portCompositorBackend)) {
#ifdef PORT_COMPOSITOR_BACKEND_CAIRO
    case PORT_COMPOSITOR_BACKEND::CAIRO:
        return CompositorFactory::supportsFilterEffectCairo(textureWidth,
                                                            textureHeight);
#endif
#ifdef PORT_COMPOSITOR_BACKEND_GL
    case PORT_COMPOSITOR_BACKEND::GL:
        return CompositorFactory::supportsFilterEffectGl(textureWidth,
                                                         textureHeight);
#endif
#ifdef PORT_COMPOSITOR_BACKEND_MOCK
    case PORT_COMPOSITOR_BACKEND::MOCK:
        return CompositorFactory::supportsFilterEffectMock(textureWidth,
                                                           textureHeight);
#endif
    default:
        break;
    }
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return false;
}
} // namespace Starfish
