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

#if defined(PORT_CANVAS_BACKEND_MOCK)
#include "Starfish.h"
#include "core/style/Style.h"
#include "core/dom/canvas/CanvasLineCap.h"
#include "core/dom/canvas/CanvasLineJoin.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/font/Font.h"
#include "core/modules/canvas/NativeGradient.h"
#include "core/modules/canvas/image/NativeImageData.h"
#include "core/modules/canvas/NativePattern.h"
#include "core/style/UnitHelper.h"

namespace Starfish {

class NativeGradientMock : public NativeGradient {
public:
    NativeGradientMock(GradientDrawingInfo* info)
        : NativeGradient(info)
    {
    }

    NativeGradientMock(double x0, double y0, double x1, double y1)
        : NativeGradient()
    {
    }

    NativeGradientMock(double x0, double y0, double r0, double x1, double y1,
                       double r1)
        : NativeGradient()
    {
    }

    ~NativeGradientMock()
    {
    }

private:
};

std::shared_ptr<NativeGradient> NativeGradient::create(
    GradientDrawingInfo* info)
{
    return std::shared_ptr<NativeGradient>(new NativeGradientMock(info));
}

std::shared_ptr<NativeGradient> NativeGradient::create(double x0, double y0,
                                                       double x1, double y1)
{
    return std::shared_ptr<NativeGradient>(
        new NativeGradientMock(x0, y0, x1, y1));
}

std::shared_ptr<NativeGradient> NativeGradient::create(double x0, double y0,
                                                       double r0, double x1,
                                                       double y1, double r1)
{
    return std::shared_ptr<NativeGradient>(
        new NativeGradientMock(x0, y0, r0, x1, y1, r1));
}

class NativePatternMock : public NativePattern {
public:
    NativePatternMock(NULLABLE NativeImageData* image, bool repeatX,
                      bool repeatY)
        : NativePattern(image, repeatX, repeatY)
    {
    }

    ~NativePatternMock()
    {
    }

private:
};

std::shared_ptr<NativePattern> NativePattern::create(
    NULLABLE NativeImageData* image, bool repeatX, bool repeatY)
{
    return std::shared_ptr<NativePattern>(
        new NativePatternMock(image, repeatX, repeatY));
}

class CanvasMock : public Canvas {
public:
    CanvasMock(WebView* webView)
    {
        m_webView = webView;
    }

    virtual ~CanvasMock()
    {
    }

    virtual void clearColor(const Unit::Color& clr)
    {
    }

    virtual void flush()
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

    virtual void setFillColor(const Unit::Color& clr)
    {
    }

    virtual void setFillSource(CanvasFillStrokeSource& source)
    {
    }

    virtual CanvasFillStrokeSource fillSource()
    {
        return CanvasFillStrokeSource();
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

    virtual void strokeRect(const Unit::Rect& rt)
    {
    }

    virtual void strokeRect(const LayoutRect& rt)
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

    virtual void drawImage(NativeImageData* data, const Unit::Rect& src,
                           const Unit::Rect& dst,
                           const DrawImageInfo& borderinfo,
                           ImageRenderingValue imageRenderingMode)
    {
    }

    virtual void drawRepeatImage(NativeImageData* data, const Unit::Rect& dst,
                                 float imageWidth, float imageHeight,
                                 bool xRepeat, bool yRepeat,
                                 ImageRenderingValue imageRenderingMode)
    {
    }

    virtual void drawRepeatImage(NativeImageData* data, const Unit::Rect& dst,
                                 float imageWidth, float imageHeight,
                                 bool xRepeat, bool yRepeat)
    {
    }

    virtual void drawLinearGradient(const Unit::Rect& dst,
                                    GradientDrawingInfo* info,
                                    NativeGradient* gradient)
    {
    }

    virtual void drawRadialGradient(const Unit::Rect& dst,
                                    GradientDrawingInfo* info,
                                    NativeGradient* gradient)
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

    virtual void resetMatrixAndClip(bool needsApplyDPR)
    {
    }

    virtual void resetMatrix(bool needsApplyDPR)
    {
    }

    virtual void resetClip()
    {
    }

    virtual void unsetDevicePixelRatio()
    {
    }

    virtual void setMatrix(const SkMatrix& matrix)
    {
    }

    virtual SkMatrix currentTransformMatrix()
    {
        return SkMatrix::I();
    }

    virtual CanvasLineCap lineCap()
    {
        return CanvasLineCap::Butt;
    }

    virtual void setLineCap(CanvasLineCap lineCap)
    {
    }

    virtual CanvasLineJoin lineJoine()
    {
        return CanvasLineJoin::Miter;
    }

    virtual void setLineJoin(CanvasLineJoin lineJoin)
    {
    }

    virtual double miterLimit()
    {
        return 0.0;
    }

    virtual void setMiterLimit(double limit)
    {
    }

    virtual void setStrokeColor(const Unit::Color& clr)
    {
    }

    virtual void setStrokeSource(CanvasFillStrokeSource& source)
    {
    }

    virtual CanvasFillStrokeSource strokeSource()
    {
        return CanvasFillStrokeSource();
    }

    virtual void setNonInvertableCTM(bool validation)
    {
    }

    virtual bool hasNonInvertableCTM()
    {
        return false;
    }

    virtual void setPathTransformMatrix(const SkMatrix& marix)
    {
    }

    virtual SkMatrix pathTransformMatrix()
    {
        return SkMatrix::I();
    }

    virtual void setGlobalAlpha(float c)
    {
    }

    virtual float globalAlpha()
    {
        return 0.0;
    }

    virtual void setCompositeOperator(CanvasCompositeOperator oper,
                                      CanvasBlendMode mode)
    {
    }

    virtual CanvasCompositeOperator compositeOperator()
    {
        return CanvasCompositeOperator::Clear;
    }

    virtual CanvasBlendMode blendMode()
    {
        return CanvasBlendMode::Normal;
    }

protected:
    WebView* m_webView;
};

Canvas* Canvas::create(WebView* webView, CanvasSurface* data)
{
    return new CanvasMock(webView);
}

Canvas* Canvas::create(WebView* webView, uint8_t* data, size_t w, size_t h,
                       size_t stride)
{
    return new CanvasMock(webView);
}

Canvas* Canvas::create(WebView* webView, NativeImageData* data)
{
    return new CanvasMock(webView);
}

NativeImageData* NativeImageData::attach(Canvas* canvas)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return nullptr;
}
}
#endif
