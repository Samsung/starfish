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

#include "StarfishConfig.h"

#include "Starfish.h"
#include "core/page/WebView.h"
#include "core/style/Style.h"
#include "core/style/ComputedStyle.h"
#include "core/dom/Node.h"
#include "core/modules/renderer/Renderer.h"
#include "Compositor.h"
#include "Canvas.h"
#include "CompositorFactory.h"

namespace Starfish {

Compositor* Compositor::create3D(WebView* webview, CompositorContext* ctx)
{
    StarfishRendererType rendererType = webview->starfish()->rendererType();
#if defined(STARFISH_EFL_HEADLESS)
    STARFISH_ASSERT(rendererType == StarfishRendererType::kHeadless);
    return CompositorFactory::create3dMock(webview, ctx);
#else
    if (rendererType == StarfishRendererType::kOpenGL) {
        return CompositorFactory::create3dGl(webview, ctx);
    } else if (rendererType == StarfishRendererType::kSoftware) {
        return CompositorFactory::create3dCairo(webview, ctx);
    }
#endif
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return nullptr;
}

Compositor* Compositor::create2D(WebView* webview, CompositorContext* ctx,
                                 CanvasSurface* surface)
{
    StarfishRendererType rendererType = webview->starfish()->rendererType();
#if defined(STARFISH_EFL_HEADLESS)
    STARFISH_ASSERT(rendererType == StarfishRendererType::kHeadless);
    return CompositorFactory::create2dMock(webview, ctx, surface);
#else
    if (rendererType == StarfishRendererType::kOpenGL) {
        return CompositorFactory::create2dGl(webview, ctx, surface);
    } else if (rendererType == StarfishRendererType::kSoftware) {
        return CompositorFactory::create2dCairo(webview, ctx, surface);
    }
#endif
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return nullptr;
}

CompositorContext* Compositor::initCompositorContext(Renderer* renderer)
{
    StarfishRendererType rendererType = renderer->starfish()->rendererType();
#if defined(STARFISH_EFL_HEADLESS)
    STARFISH_ASSERT(rendererType == StarfishRendererType::kHeadless);
    return CompositorFactory::initCompositorContextMock(renderer);
#else
    if (rendererType == StarfishRendererType::kOpenGL) {
        return CompositorFactory::initCompositorContextGl(renderer);
    } else if (rendererType == StarfishRendererType::kSoftware) {
        return CompositorFactory::initCompositorContextCairo(renderer);
    }
#endif
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return nullptr;
}

void Compositor::destroyCompositorContext(Renderer* renderer,
                                          CompositorContext* ctx)
{
    StarfishRendererType rendererType = renderer->starfish()->rendererType();
#if defined(STARFISH_EFL_HEADLESS)
    STARFISH_ASSERT(rendererType == StarfishRendererType::kHeadless);
    return CompositorFactory::destroyCompositorContextMock(renderer, ctx);
#else
    if (rendererType == StarfishRendererType::kOpenGL) {
        return CompositorFactory::destroyCompositorContextGl(renderer, ctx);
    } else if (rendererType == StarfishRendererType::kSoftware) {
        return CompositorFactory::destroyCompositorContextCairo(renderer, ctx);
    }
#endif
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
}

uint32_t Compositor::maximumTextureSize(Starfish* starfish)
{
    StarfishRendererType rendererType = starfish->rendererType();
#if defined(STARFISH_EFL_HEADLESS)
    STARFISH_ASSERT(rendererType == StarfishRendererType::kHeadless);
    return CompositorFactory::maximumTextureSizeMock();
#else
    if (rendererType == StarfishRendererType::kOpenGL) {
        return CompositorFactory::maximumTextureSizeGl();
    } else if (rendererType == StarfishRendererType::kSoftware) {
        return CompositorFactory::maximumTextureSizeCairo();
    }
#endif
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return 0;
}

bool Compositor::supportsFilterEffect(Starfish* starfish, size_t textureWidth,
                                      size_t textureHeight)
{
    StarfishRendererType rendererType = starfish->rendererType();
#if defined(STARFISH_EFL_HEADLESS)
    STARFISH_ASSERT(rendererType == StarfishRendererType::kHeadless);
    return CompositorFactory::supportsFilterEffectMock(textureWidth,
                                                       textureHeight);
#else
    if (rendererType == StarfishRendererType::kOpenGL) {
        return CompositorFactory::supportsFilterEffectGl(textureWidth,
                                                         textureHeight);
    } else if (rendererType == StarfishRendererType::kSoftware) {
        return CompositorFactory::supportsFilterEffectCairo(textureWidth,
                                                            textureHeight);
    }
#endif
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return false;
}
} // namespace Starfish
