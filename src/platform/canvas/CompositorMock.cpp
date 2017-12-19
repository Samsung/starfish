/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
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

    virtual void drawImage(ImageData* data, const Unit::Rect& dst)
    {
    }

    virtual void drawRepeatImage(ImageData* data, const Unit::Rect& dst,
                                 float imageWidth, float imageHeight,
                                 bool xRepeat, bool yRepeat)
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
