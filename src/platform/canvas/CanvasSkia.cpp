/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#if defined(PORT_CANVAS_BACKEND_SKIA)
#include "StarFish.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/font/Font.h"
#include "core/modules/canvas/image/NativeImageData.h"
#include "core/style/UnitHelper.h"

#include "SkCanvas.h"

namespace StarFish {

class CanvasSkia : public Canvas {
public:
    CanvasSkia(StarFish* starfish, void* data)
    {
        SkCanvas canvas; // temp
        m_starfish = starfish;
    }

    CanvasSkia(StarFish* starfish, CanvasSurface* data)
    {
        m_starfish = starfish;
    }

    virtual ~CanvasSkia()
    {
    }

    virtual void clearColor(const Unit::Color& clr)
    {
    }

    // state
    virtual void save() // push state on state stack
    {
    }

    virtual void restore() // pop state stack and restore state
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
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        return TextDecorationData();
    }

    virtual void setTextDecorationData(TextDecorationData d)
    {
    }

    virtual void drawRect(const Unit::Rect& rt)
    {
    }

    virtual void drawRect(const LayoutRect& rt)
    {
    }

    virtual void drawRect(LayoutLocation p1, LayoutLocation p2,
                          LayoutLocation p3, LayoutLocation p4)
    {
    } // left, top, right, bottom

    virtual void setDash(double* dashes, int dashCnt, double offset)
    {
    }

    virtual void punchHole(const Unit::Rect& rt)
    {
    }

    virtual void drawText(LayoutUnit x, LayoutUnit y, LayoutUnit stringWidth,
                          const StringView& text)
    {
    }

    virtual void drawImage(NativeImageData* data, const Unit::Rect& dst,
                           ImageRenderingValue imageRenderingMode =
                               ImageRenderingValue::ImageRenderingAutoValue)
    {
    }

    virtual void drawImage(CanvasSurface* data, const Unit::Rect& dst,
                           ImageRenderingValue imageRenderingMode =
                               ImageRenderingValue::ImageRenderingAutoValue)
    {
    }

    virtual void drawImage(NativeImageData* data, const Unit::Rect& src,
                           const Unit::Rect& dst,
                           const DrawImageInfo& borderinfo,
                           ImageRenderingValue imageRenderingMode =
                               ImageRenderingValue::ImageRenderingAutoValue)
    {
    }

    virtual void drawBorderImage(NativeImageData* data, const Unit::Rect& dst,
                                 size_t l, size_t t, size_t r, size_t b,
                                 double scale, bool fill)
    {
    }

    virtual void drawRepeatImage(
        NativeImageData* data, const Unit::Rect& dst, float imageWidth,
        float imageHeight, bool xRepeat, bool yRepeat,
        ImageRenderingValue imageRenderingMode =
            ImageRenderingValue::ImageRenderingAutoValue)
    {
    }

    virtual void drawLinearGradient(const Unit::Rect& dst,
                                    GradientDrawingInfo* info)
    {
    }

    virtual void drawRadialGradient(const Unit::Rect& dst,
                                    GradientDrawingInfo* info)
    {
    }

    virtual void applyMatrixTo(LayoutLocation& lp)
    {
    }

    virtual void applyMatrixTo(LayoutRect& lp)
    {
    }

    virtual void setVisible(bool visible)
    {
    }

    virtual void* unwrap()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
        return NULL;
    }

protected:
    StarFish* m_starfish;
};

Canvas* Canvas::createDirect(StarFish* starfish, void* data)
{
    return new CanvasSkia(starfish, data);
}

Canvas* Canvas::create(StarFish* starfish, CanvasSurface* data)
{
    return new CanvasSkia(starfish, data);
}

Canvas* Canvas::createGenericCanvas(StarFish* starfish, void* data, size_t w,
                                    size_t h)
{
    return new CanvasSkia(starfish, data);
}
Canvas* Canvas::createGenericCanvas(StarFish* starfish, NativeImageData* data)
{
    return new CanvasSkia(starfish, data);
}
}
#endif
