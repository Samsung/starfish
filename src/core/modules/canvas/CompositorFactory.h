/*
 * Copyright (c) 2021-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishCompositorFactory__
#define __StarfishCompositorFactory__

namespace Starfish {

class Renderer;
class Canvas;
class CanvasSurface;
class Compositor;
class CompositorContext;

// The if-def statements below are temporary soluation to avoid affecting other
// ports of LWE except flutter. In the future, It will be removed when LWE's all
// ports are changed to a single binary.

namespace CompositorFactory {

#if !defined(STARFISH_EFL_HEADLESS)
    Compositor* create3dCairo(WebView* webview, CompositorContext* ctx);
    Compositor* create2dCairo(WebView* webview, CompositorContext* ctx,
                              CanvasSurface* surface);
    CompositorContext* initCompositorContextCairo(Renderer* renderer);
    void destroyCompositorContextCairo(Renderer* renderer,
                                       CompositorContext* ctx);
    uint32_t maximumTextureSizeCairo();
    bool supportsFilterEffectCairo(size_t textureWidth, size_t textureHeight);
#endif
#if !defined(STARFISH_EFL_HEADLESS)
    Compositor* create3dGl(WebView* webview, CompositorContext* ctx);
    Compositor* create2dGl(WebView* webview, CompositorContext* ctx,
                           CanvasSurface* surface);
    CompositorContext* initCompositorContextGl(Renderer* renderer);
    void destroyCompositorContextGl(Renderer* renderer, CompositorContext* ctx);
    uint32_t maximumTextureSizeGl();
    bool supportsFilterEffectGl(size_t textureWidth, size_t textureHeight);
#endif
#if defined(STARFISH_EFL_HEADLESS)
    Compositor* create3dMock(WebView* webview, CompositorContext* ctx);
    Compositor* create2dMock(WebView* webview, CompositorContext* ctx,
                             CanvasSurface* surface);
    CompositorContext* initCompositorContextMock(Renderer* renderer);
    void destroyCompositorContextMock(Renderer* renderer,
                                      CompositorContext* ctx);
    uint32_t maximumTextureSizeMock();
    bool supportsFilterEffectMock(size_t textureWidth, size_t textureHeight);
#endif

}; // namespace CompositorFactory
} // namespace Starfish

#endif
