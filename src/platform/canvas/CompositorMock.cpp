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

#include "StarfishConfig.h"
#include <SkMatrix.h>
#include "core/modules/canvas/Compositor.h"

#if defined(PORT_COMPOSITOR_BACKEND_MOCK)

namespace Starfish {

class CompositorMock : public Compositor {
public:
    CompositorMock(WebView* webview, CanvasSurface* data)
    {
    }

    ~CompositorMock()
    {
    }

    virtual void clearColor(const Unit::Color& clr)
    {
    }

    // state
    virtual void save()
    {
    }

    virtual void restore()
    {
    }

    virtual void scale(double x, double y)
    {
    }

    virtual void rotate(double angle)
    {
    }

    virtual void translate(double x, double y)
    {
    }

    virtual void translate(LayoutUnit x, LayoutUnit y)
    {
    }

    virtual void postMatrix(const SkMatrix& matrix)
    {
    }

    virtual void clip(const Unit::Rect& rt)
    {
    }

    // reset transform matrix & clip
    virtual void resetMatrixAndClip()
    {
    }

    // reset transform clip
    virtual void resetClip()
    {
    }

    virtual void setFillColor(const Unit::Color& clr)
    {
    }

    virtual void beginOpacityLayer(float c)
    {
    }

    virtual void endOpacityLayer()
    {
    }

    virtual void drawRect(const Unit::Rect& rt)
    {
    }

    virtual void drawRect(const LayoutRect& rt)
    {
    }

    virtual void punchHole(const Unit::Rect& rt)
    {
    }

    virtual void drawSurface(CanvasSurface* data, const Unit::Rect& dst)
    {
    }

    virtual void applyMatrixTo(LayoutLocation& lp)
    {
    }

    virtual void applyMatrixTo(LayoutRect& lp)
    {
    }

    virtual void clipPath()
    {
    }

    virtual void moveTo(float x, float y)
    {
    }

    virtual void lineTo(float x, float y)
    {
    }

    virtual void arcNegative(double xc, double yc, double radius, double angle1,
                             double angle2)
    {
    }

    virtual SkMatrix currentTransformMatrix()
    {
        return SkMatrix::I();
    }
};

Compositor* Compositor::create2D(WebView* webview, CompositorContext* ctx,
                                 CanvasSurface* surface)
{
    return new CompositorMock(webview, surface);
}

Compositor* Compositor::create3D(WebView* starfish, CompositorContext* ctx)
{
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
}

bool Compositor::supportsFilterEffect(size_t textureWidth, size_t textureHeight)
{
    return false;
}
}

#endif
