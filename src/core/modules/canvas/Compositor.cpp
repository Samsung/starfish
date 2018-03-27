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
#include "Compositor.h"
#include "Canvas.h"

namespace StarFish {

class CompositorCanvasAdaptor : public Canvas {
public:
    CompositorCanvasAdaptor(Compositor* compositor)
        : m_compositor(compositor)
    {
    }

    virtual void clearColor(const Unit::Color& clr)
    {
        m_compositor->clearColor(clr);
    }
    virtual void save()
    {
        m_compositor->save();
    }
    virtual void restore()
    {
        m_compositor->restore();
    }
    virtual void scale(double x, double y)
    {
        m_compositor->scale(x, y);
    }
    virtual void rotate(double angle)
    {
        m_compositor->rotate(angle);
    }
    virtual void translate(double x, double y)
    {
        m_compositor->translate(x, y);
    }
    virtual void translate(LayoutUnit x, LayoutUnit y)
    {
        m_compositor->translate(x, y);
    }
    virtual void postMatrix(const SkMatrix& matrix)
    {
        m_compositor->postMatrix(matrix);
    }
    virtual void clip(const Unit::Rect& rt)
    {
        m_compositor->clip(rt);
    }
    virtual void resetMatrixAndClip()
    {
        m_compositor->resetMatrixAndClip();
    }
    virtual void resetClip()
    {
        m_compositor->resetClip();
    }
    virtual void setColor(const Unit::Color& clr)
    {
        m_compositor->setColor(clr);
    }
    virtual void beginOpacityLayer(float c)
    {
        m_compositor->beginOpacityLayer(c);
    }
    virtual void endOpacityLayer()
    {
        m_compositor->endOpacityLayer();
    }
    virtual void setFont(Font* font)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    virtual void resetTextDecorationData()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    virtual void mergeTextDecorationData(ComputedStyle* style)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    virtual TextDecorationData textDecorationData()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    virtual void setTextDecorationData(TextDecorationData d)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    virtual void setTextShadowData(CanvasShadowDataList& list)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    virtual void clearTextShadowData()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    virtual void drawRect(const Unit::Rect& rt)
    {
        m_compositor->drawRect(rt);
    }
    virtual void drawRect(const LayoutRect& rt)
    {
        m_compositor->drawRect(rt);
    }
    virtual void drawRect(LayoutLocation p1, LayoutLocation p2,
                          LayoutLocation p3, LayoutLocation p4)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    virtual void setDash(double* dashes, int dashCnt, double offset)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void punchHole(const Unit::Rect& rt)
    {
        m_compositor->punchHole(rt);
    }

    virtual void drawText(LayoutUnit x, LayoutUnit y, LayoutUnit stringWidth,
                          const StringView& text)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void drawImage(NativeImageData* data, const Unit::Rect& dst,
                           ImageRenderingValue imageRenderingMode)
    {
        m_compositor->drawImage(data, dst);
    }

    virtual void drawImage(NativeImageData* data, const Unit::Rect& src,
                           const Unit::Rect& dst,
                           const DrawImageInfo& borderinfo,
                           ImageRenderingValue imageRenderingMode)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void drawImage(CanvasSurface* data, const Unit::Rect& dst,
                           ImageRenderingValue imageRenderingMode)
    {
        m_compositor->drawSurface(data, dst);
    }

    virtual void drawBorderImage(NativeImageData* data, const Unit::Rect& dst,
                                 size_t l, size_t t, size_t r, size_t b,
                                 double scale, bool fill)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    virtual void drawRepeatImage(NativeImageData* data, const Unit::Rect& dst,
                                 float imageWidth, float imageHeight,
                                 bool xRepeat, bool yRepeat,
                                 ImageRenderingValue imageRenderingMode)
    {
        m_compositor->drawRepeatImage(data, dst, imageWidth, imageHeight,
                                      xRepeat, yRepeat);
    }

    virtual void applyMatrixTo(LayoutLocation& lp)
    {
        m_compositor->applyMatrixTo(lp);
    }
    virtual void applyMatrixTo(LayoutRect& lp)
    {
        m_compositor->applyMatrixTo(lp);
    }

    virtual void setVisible(bool visible)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    // Generic canvas functions
    virtual void beginPath()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    virtual void closePath()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    virtual void moveTo(float x, float y)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    virtual void lineTo(float x, float y)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    virtual void curveTo(float x1, float y1, float x2, float y2, float x3,
                         float y3)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    virtual void quadraticCurveTo(float x1, float y1, float x2, float y2)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    virtual void arc(double xc, double yc, double radius, double angle1,
                     double angle2)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    virtual void arcNegative(double xc, double yc, double radius, double angle1,
                             double angle2)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    virtual void stroke()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    virtual void strokePreserve()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    virtual void setFillRule(bool shouldUseNonZeroFillRule)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    virtual void fill()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    virtual void fillPreserve()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    virtual void clipPath()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    virtual void clipPathPreserve()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    virtual void setStrokeWidth(float width)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    virtual void* unwrap()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

protected:
    Compositor* m_compositor;
};

Canvas* Compositor::createCanvasAdaptor(Compositor* compositor)
{
    return new CompositorCanvasAdaptor(compositor);
}
}
