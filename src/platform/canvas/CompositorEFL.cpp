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
#include "StarFish.h"
#include "core/modules/canvas/Compositor.h"
#include "core/modules/canvas/Canvas.h"

#if defined(PORT_COMPOSITOR_BACKEND_EFL)

namespace StarFish {

Canvas* createCanvasDirectEFL(StarFish* starfish, void* data);
Canvas* createCanvasEFL(StarFish* starfish, CanvasSurface* data);

class CompositorEFL : public Compositor {
    Canvas* m_canvas;
    StarFish* m_starfish;

public:
    CompositorEFL(StarFish* starfish, void* data)
    {
        m_starfish = starfish;
        m_canvas = createCanvasDirectEFL(starfish, data);
        resetMatrixAndClip();
    }

    CompositorEFL(StarFish* starfish, CanvasSurface* data)
    {
        m_starfish = starfish;
        m_canvas = createCanvasEFL(starfish, data);
        resetMatrixAndClip();
    }

    ~CompositorEFL()
    {
        delete m_canvas;
    }

    virtual void clearColor(const Unit::Color& clr)
    {
        m_canvas->clearColor(clr);
    }

    // state
    virtual void save()
    {
        m_canvas->save();
    }

    virtual void restore()
    {
        m_canvas->restore();
    }

    virtual void scale(double x, double y)
    {
        m_canvas->scale(x, y);
    }

    virtual void rotate(double angle)
    {
        m_canvas->rotate(angle);
    }

    virtual void translate(double x, double y)
    {
        m_canvas->translate(x, y);
    }

    virtual void translate(LayoutUnit x, LayoutUnit y)
    {
        m_canvas->translate(x, y);
    }

    virtual void postMatrix(const SkMatrix& matrix)
    {
        m_canvas->postMatrix(matrix);
    }

    virtual void clip(const Unit::Rect& rt)
    {
        m_canvas->clip(rt);
    }

    // reset transform matrix & clip
    virtual void resetMatrixAndClip()
    {
        m_canvas->resetMatrixAndClip();

        m_canvas->scale(m_starfish->screenInfo().devicePixelRatio,
                        m_starfish->screenInfo().devicePixelRatio);
    }

    // reset transform clip
    virtual void resetClip()
    {
        m_canvas->resetClip();
    }

    virtual void setColor(const Unit::Color& clr)
    {
        m_canvas->setColor(clr);
    }

    virtual void beginOpacityLayer(float c)
    {
        m_canvas->beginOpacityLayer(c);
    }

    virtual void endOpacityLayer()
    {
        m_canvas->endOpacityLayer();
    }

    virtual void drawRect(const Unit::Rect& rt)
    {
        m_canvas->drawRect(rt);
    }

    virtual void drawRect(const LayoutRect& rt)
    {
        m_canvas->drawRect(rt);
    }

    virtual void punchHole(const Unit::Rect& rt)
    {
        m_canvas->punchHole(rt);
    }

    virtual void drawSurface(CanvasSurface* data, const Unit::Rect& dst)
    {
        m_canvas->drawImage(data, dst);
    }

    virtual void drawImage(NativeImageData* data, const Unit::Rect& dst)
    {
        m_canvas->drawImage(data, dst);
    }

    virtual void drawImage(NativeImageData* data, const Unit::Rect& src,
                           const Unit::Rect& dst, bool xRepeat, bool yRepeat)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    virtual void drawRepeatImage(NativeImageData* data, const Unit::Rect& dst,
                                 float imageWidth, float imageHeight,
                                 bool xRepeat, bool yRepeat)
    {
        m_canvas->drawImage(data, dst);
    }

    virtual void applyMatrixTo(LayoutLocation& lp)
    {
        m_canvas->applyMatrixTo(lp);
    }

    virtual void applyMatrixTo(LayoutRect& lp)
    {
        m_canvas->applyMatrixTo(lp);
    }
};

Compositor* Compositor::create(StarFish* starfish, void* data)
{
    return new CompositorEFL(starfish, data);
}

Compositor* Compositor::create(StarFish* starfish, CanvasSurface* surface)
{
    return new CompositorEFL(starfish, surface);
}
}

#endif
