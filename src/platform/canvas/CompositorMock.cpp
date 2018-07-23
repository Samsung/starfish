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

#include "StarFishConfig.h"
#include "core/modules/canvas/Compositor.h"
#include "core/modules/canvas/Canvas.h"

#if defined(PORT_COMPOSITOR_BACKEND_MOCK)

namespace StarFish {

class CompositorMock : public Compositor {
public:
    CompositorMock(StarFish* starfish, void* data)
    {
    }

    CompositorMock(StarFish* starfish, CanvasSurface* data)
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

    virtual void setColor(const Unit::Color& clr)
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
};

Compositor* Compositor::create(StarFish* starfish, void* data)
{
    return new CompositorMock(starfish, data);
}

Compositor* Compositor::create(StarFish* starfish, CanvasSurface* surface)
{
    return new CompositorMock(starfish, surface);
}
}

#endif
