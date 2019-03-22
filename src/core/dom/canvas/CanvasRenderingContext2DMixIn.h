/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishCanvasRenderingContext2DMixIn__
#define __StarfishCanvasRenderingContext2DMixIn__

#ifdef STARFISH_ENABLE_CANVAS

#include "core/dom/canvas/CanvasRenderingContext.h"

namespace Starfish {

class Canvas;
class CanvasGradient;
class DOMStringOrCanvasGradientOrCanvasPattern;
class ImageData;
class ExecutionContext;
class HTMLCanvasElement;

enum class CanvasFillRule {
    CanvasFillRuleInvalid,
    CanvasFillRuleNonZero,
    CanvasFillRuleEvenOdd
};

class CanvasRenderingContext2DMixIn : public CanvasRenderingContext {
public:
    CanvasRenderingContext2DMixIn(HTMLCanvasElement* ownerHTMLCanvasElement);
    ~CanvasRenderingContext2DMixIn()
    {
    }

    virtual void initialize() override;
    virtual void flush() override;

    // CanvasState
    void save();
    void restore();

    // CanvasTransform
    void scale(double x, double y);
    void rotate(double angle);
    void translate(double x, double y);
    void transform(double a, double b, double c, double d, double e, double f);

    // CanvasCompositing
    double globalAlpha();
    void setGlobalAlpha(double value);
    String* globalCompositeOperation();
    void setGlobalCompositeOperation(String* value);

    // CanvasImageSmoothing

    // CanvasFillStrokeStyles
    DOMStringOrCanvasGradientOrCanvasPattern fillStyle();
    void setFillStyle(DOMStringOrCanvasGradientOrCanvasPattern value);

    DOMStringOrCanvasGradientOrCanvasPattern strokeStyle();
    void setStrokeStyle(DOMStringOrCanvasGradientOrCanvasPattern value);
    CanvasGradient* createLinearGradient(double x0, double y0, double x1,
                                         double y1);

    // CanvasShadowStyles

    // CanvasFilters
    String* filter();
    void setFilter(String* value);

    // CanvasRect
    void fillRect(double x, double y, double w, double h);
    void strokeRect(double x, double y, double w, double h);

    // CanvasDrawPath
    void beginPath();
    void fill(String* fillRule);
    void fill(Path2D* path, String* fillRule);
    void stroke();

    // CanvasUserInterface

    // CanvasText

    // CanvasDrawImage
    // NOTE Replace first argument's type of "drawImage" temporarily to
    // implement mock
    // CanvasImageSource -> ScriptValue
    void drawImage(ScriptValue image, double dx, double dy);
    void drawImage(ScriptValue image, double dx, double dy, double dw,
                   double dh);
    void drawImage(ScriptValue image, double sx, double sy, double sw,
                   double sh, double dx, double dy, double dw, double dh);

    // CanvasImageData
    ImageData* getImageData(int32_t sx, int32_t sy, int32_t sw, int32_t sh);

    // CanvasPathDrawingStyles

    // CanvasTextDrawingStyles

    // CanvasPath
    void closePath();
    void moveTo(double x, double y);
    void lineTo(double x, double y);
    void rect(double x, double y, double w, double h);
    void arc(double x, double y, double radius, double startAngle,
             double endAngle, bool anticlockwise = false);
    void ellipse(double x, double y, double radiusX, double radiusY,
                 double rotation, double startAngle, double endAngle,
                 bool anticlockwise = false);

    void bezierCurveTo(double x1, double y1, double x2, double y2, double x3,
                       double y3);

    void clearRect(double x, double y, double w, double h);

    double lineWidth()
    {
        return m_lineWidth;
    }

    void setLineWidth(double width);

    void* operator new(size_t size)
    {
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word desc[GC_BITMAP_SIZE(CanvasRenderingContext2DMixIn)] = { 0 };
            CanvasRenderingContext2DMixIn::fillGCDescriptor(desc);
            descr = GC_make_descriptor(
                desc, GC_WORD_LEN(CanvasRenderingContext2DMixIn));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        CanvasRenderingContext::fillGCDescriptor(desc);
        GC_set_bit(desc,
                   GC_WORD_OFFSET(CanvasRenderingContext2DMixIn, m_canvas));
        GC_set_bit(desc, GC_WORD_OFFSET(CanvasRenderingContext2DMixIn,
                                        m_ownerHTMLCanvasElement));
    }
    HTMLCanvasElement* m_ownerHTMLCanvasElement;

private:
    Canvas* m_canvas;
    Unit::Color m_fillColor;
    Unit::Color m_strokeColor;
    double m_lineWidth;
    double m_globalAlpha;
};
}
#endif
#endif
