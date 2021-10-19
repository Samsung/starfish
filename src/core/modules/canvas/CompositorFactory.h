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

class PlatformWindow;
class Canvas;
class CanvasSurface;
class Compositor;
class CompositorContext;

// The if-def statements below are temporary soluation to avoid affecting other
// ports of LWE except flutter. In the future, It will be removed when LWE's all
// ports are changed to a single binary.

namespace CompositorFactory {
#ifdef PORT_COMPOSITOR_BACKEND_CAIRO
    Compositor* create3dCairo(WebView* starfish, CompositorContext* ctx);
    Compositor* create2dCairo(WebView* starfish, CompositorContext* ctx,
                              CanvasSurface* surface);
    CompositorContext* initCompositorContextCairo(PlatformWindow* wnd);
    void destroyCompositorContextCairo(PlatformWindow* wnd,
                                       CompositorContext* ctx);
    size_t maximumTextureSizeCairo();
    bool supportsFilterEffectCairo(size_t textureWidth, size_t textureHeight);
#endif

#ifdef PORT_COMPOSITOR_BACKEND_GL
    Compositor* create3dGl(WebView* starfish, CompositorContext* ctx);
    Compositor* create2dGl(WebView* starfish, CompositorContext* ctx,
                           CanvasSurface* surface);
    CompositorContext* initCompositorContextGl(PlatformWindow* wnd);
    void destroyCompositorContextGl(PlatformWindow* wnd,
                                    CompositorContext* ctx);
    size_t maximumTextureSizeGl();
    bool supportsFilterEffectGl(size_t textureWidth, size_t textureHeight);
#endif

#ifdef PORT_COMPOSITOR_BACKEND_MOCK
    Compositor* create3dMock(WebView* starfish, CompositorContext* ctx);
    Compositor* create2dMock(WebView* starfish, CompositorContext* ctx,
                             CanvasSurface* surface);
    CompositorContext* initCompositorContextMock(PlatformWindow* wnd);
    void destroyCompositorContextMock(PlatformWindow* wnd,
                                      CompositorContext* ctx);
    size_t maximumTextureSizeMock();
    bool supportsFilterEffectMock(size_t textureWidth, size_t textureHeight);
#endif

#ifdef PORT_COMPOSITOR_BACKEND_SKIA
    Compositor* create3dSkia(WebView* starfish, CompositorContext* ctx);
    Compositor* create2dSkia(WebView* starfish, CompositorContext* ctx,
                             CanvasSurface* surface);
    CompositorContext* initCompositorContextSkia(PlatformWindow* wnd);
    void destroyCompositorContextSkia(PlatformWindow* wnd,
                                      CompositorContext* ctx);
    size_t maximumTextureSizeSkia();
    bool supportsFilterEffect_skia(size_t textureWidth, size_t textureHeight);
#endif

}; // namespace CompositorFactory
} // namespace Starfish

#endif
