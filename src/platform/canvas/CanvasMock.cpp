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

#if defined(PORT_CANVAS_BACKEND_MOCK)
#include "StarFish.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/font/Font.h"
#include "core/modules/canvas/image/NativeImageData.h"
#include "core/style/UnitHelper.h"

namespace StarFish {

class CanvasMock : public Canvas {
public:
    CanvasMock(StarFish* starfish, void* data)
    {
        m_starfish = starfish;
    }

    CanvasMock(StarFish* starfish, CanvasSurface* data)
    {
        m_starfish = starfish;
    }

    virtual ~CanvasMock()
    {
    }

    virtual void clearColor(const Unit::Color& clr)
    {
    }

    // state
    virtual void save()
    {
    }

    // pop state stack and restore state
    virtual void restore()
    {
    }

    // transformations (default transform is the identity matrix)
    virtual void scale(double x, double y)
    {
    }

    virtual void scale(double x, double y, double ox, double oy)
    {
    }

    virtual void rotate(double angle)
    {
    }

    virtual void rotate(double angle, double ox, double oy)
    {
    }

    virtual void translate(double x, double y)
    {
    }

    virtual void translate(LayoutUnit x, LayoutUnit y)
    {
    }

    virtual void beginOpacityLayer(float c)
    {
    }

    virtual void endOpacityLayer()
    {
    }

    virtual void clip(const Unit::Rect& rt)
    {
    }

    virtual void setColor(const Unit::Color& clr_)
    {
    }

    virtual void setVisible(bool visible)
    {
    }

    virtual void setFont(Font* font)
    {
    }

    virtual void resetTextDecorationData()
    {
    }

    virtual void mergeTextDecorationData(ComputedStyle* style)
    {
    }

    virtual TextDecorationData textDecorationData()
    {
        return TextDecorationData();
    }

    virtual void setTextDecorationData(TextDecorationData d)
    {
    }

    virtual void punchHole(const Unit::Rect& rt)
    {
    }

    virtual void drawRect(const LayoutRect& rt)
    {
    }

    virtual void drawRect(LayoutLocation p1, LayoutLocation p2,
                          LayoutLocation p3, LayoutLocation p4)
    {
    }

    virtual void drawRect(const Unit::Rect& rt)
    {
    }

    virtual void setDash(double* dashes, int dashCnt, double offset)
    {
    }

    virtual void drawText(LayoutUnit x, LayoutUnit y, LayoutUnit stringWidth,
                          const StringView& sv)
    {
    }

    virtual void drawImage(NativeImageData* data, const Unit::Rect& dst,
                           ImageRenderingValue imageRenderingMode)
    {
    }

    virtual void drawBorderImage(NativeImageData* data, const Unit::Rect& dst,
                                 size_t l, size_t t, size_t r, size_t b,
                                 double scale, bool fill)
    {
    }

    virtual void drawRepeatImage(NativeImageData* data, const Unit::Rect& dst,
                                 float imageWidth, float imageHeight,
                                 bool xRepeat, bool yRepeat)
    {
    }

    virtual void drawLinearGradient(const Unit::Rect& dst, const float& sx,
                                    const float& sy, const float& ex,
                                    const float& ey,
                                    GCVector<ColorStop*>& colorStops)
    {
    }

    void drawImage(CanvasSurface* data, const Unit::Rect& dst,
                   ImageRenderingValue imageRenderingMode)
    {
    }

    virtual void postMatrix(const SkMatrix& matrix)
    {
    }

    virtual void applyMatrixTo(LayoutLocation& lp)
    {
    }

    virtual void applyMatrixTo(LayoutRect& lp)
    {
    }

    virtual void* unwrap()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
        return nullptr;
    }

    virtual void resetMatrixAndClip()
    {
    }

    virtual void resetClip()
    {
    }

protected:
    StarFish* m_starfish;
};

Canvas* Canvas::createDirect(StarFish* starfish, void* data)
{
    return new CanvasMock(starfish, data);
}

Canvas* Canvas::create(StarFish* starfish, CanvasSurface* data)
{
    return new CanvasMock(starfish, data);
}

Canvas* Canvas::createGenericCanvas(StarFish* starfish, void* data, size_t w,
                                    size_t h)
{
    return new CanvasMock(starfish, data);
}
}
#endif
