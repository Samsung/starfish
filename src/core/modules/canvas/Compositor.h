/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishCompositor__
#define __StarfishCompositor__

namespace Starfish {

class NativeImageData;
class PlatformWindow;
class Canvas;
class CanvasSurface;

class CompositorContext {
public:
    virtual ~CompositorContext()
    {
    }
    virtual void willRendering() = 0;
    virtual void didRendering() = 0;
};

class Compositor : public gc {
protected:
    Compositor()
    {
    }

public:
    static Compositor* create3D(WebView* starfish, CompositorContext* ctx);
    static Compositor* create2D(WebView* starfish, CompositorContext* ctx,
                                CanvasSurface* surface);
    static CompositorContext* initCompositorContext(PlatformWindow* wnd);
    static void destroyCompositorContext(PlatformWindow* wnd,
                                         CompositorContext* ctx);

    virtual ~Compositor()
    {
    }

    virtual void clearColor(const Unit::Color& clr) = 0;

    // state
    virtual void save() = 0;    // push state on state stack
    virtual void restore() = 0; // pop state stack and restore state
    // transformations (default transform is the identity matrix)
    virtual void scale(double x, double y) = 0;
    virtual void rotate(double angle) = 0;
    virtual void translate(double x, double y) = 0;
    virtual void translate(LayoutUnit x, LayoutUnit y) = 0;
    virtual void postMatrix(const SkMatrix& matrix) = 0;
    virtual SkMatrix currentTransformMatrix() = 0;

    virtual void clip(const Unit::Rect& rt) = 0;

    // reset transform matrix & clip
    virtual void resetMatrixAndClip() = 0;
    // reset transform clip
    virtual void resetClip() = 0;

    virtual void setFillColor(const Unit::Color& clr) = 0;
    virtual void beginOpacityLayer(float c) = 0;
    virtual void endOpacityLayer() = 0;

    virtual void drawRect(const Unit::Rect& rt) = 0;
    virtual void drawRect(const LayoutRect& rt) = 0;
    virtual void punchHole(const Unit::Rect& rt) = 0;

    virtual void drawSurface(CanvasSurface* data, const Unit::Rect& dst) = 0;

    virtual void applyMatrixTo(LayoutLocation& lp) = 0;
    virtual void applyMatrixTo(LayoutRect& lp) = 0;

    // those four methods are needed for border-radius clipping
    virtual void moveTo(float x, float y) = 0;
    virtual void lineTo(float x, float y) = 0;
    virtual void arcNegative(double xc, double yc, double radius, double angle1,
                             double angle2) = 0;
    virtual void clipPath() = 0;

    virtual void enableBlurEffect(float blurRadius)
    {
    }

    static bool supportsFilterEffect(size_t textureWidth, size_t textureHeight);
};
}

#endif
