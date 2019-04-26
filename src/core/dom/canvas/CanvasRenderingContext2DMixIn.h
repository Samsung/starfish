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
#include "core/dom/canvas/CanvasPath.h"
#include "core/dom/DOMExceptionOr.h"

namespace Starfish {

class Canvas;
class CanvasSurface;
class CanvasGradient;
class CanvasPath;
class DOMStringOrCanvasGradientOrCanvasPattern;
class ImageData;
class ExecutionContext;
class HTMLCanvasElement;
class HTMLOrSVGImageElementOrHTMLVideoElementOrHTMLCanvasElement;

enum class CanvasLineCap : int;
enum class CanvasLineJoin : int;

typedef HTMLOrSVGImageElementOrHTMLVideoElementOrHTMLCanvasElement
    CanvasImageSource;

class CanvasRenderingContext2DMixIn : public CanvasRenderingContext,
                                      public CanvasPathInterfaceMixIn {
public:
    CanvasRenderingContext2DMixIn(HTMLCanvasElement* ownerHTMLCanvasElement);
    virtual ~CanvasRenderingContext2DMixIn()
    {
    }

    virtual void initialize() override;
    virtual void flush() override;
    virtual void onResize() override;

    virtual CanvasSurface* surface() override
    {
        return m_canvasSurface;
    }

    void finalize();

    // CanvasState
    void save();
    void restore();

    // Note :
    // Use a float instead of double for operations related to Canvas.
    // Canvas-related calculation with double type cause a bug in cairo
    // occasionally. So we use float like other major browsers although idl is
    // specified as double

    // CanvasTransform
    void scale(float x, float y);
    void rotate(float angle);
    void translate(float x, float y);
    void transform(float a, float b, float c, float d, float e, float f);
    void setTransform(float a, float b, float c, float d, float e, float f);
    void resetTransform();

    // CanvasCompositing
    float globalAlpha();
    void setGlobalAlpha(float value);
    String* globalCompositeOperation();
    void setGlobalCompositeOperation(String* value);

    // CanvasImageSmoothing

    // CanvasFillStrokeStyles
    DOMStringOrCanvasGradientOrCanvasPattern fillStyle();
    void setFillStyle(DOMStringOrCanvasGradientOrCanvasPattern value);

    DOMStringOrCanvasGradientOrCanvasPattern strokeStyle();
    void setStrokeStyle(DOMStringOrCanvasGradientOrCanvasPattern value);
    CanvasGradient* createLinearGradient(float x0, float y0, float x1,
                                         float y1);

    // CanvasShadowStyles

    // CanvasFilters
    String* filter();
    void setFilter(String* value);

    // CanvasRect
    void clearRect(float x, float y, float w, float h);
    void fillRect(float x, float y, float w, float h);
    void strokeRect(float x, float y, float w, float h);

    // CanvasDrawPath
    void beginPath();
    void fill(String* fillRule);
    void fill(Path2D* path, String* fillRule);
    void stroke();
    void stroke(Path2D* path);
    void clip(String* fillRule);
    void clip(Path2D* path, String* fillRule);
    bool isPointInPath(float x, float y, String* fillRule);
    bool isPointInPath(Path2D* path, float x, float y, String* fillRule);
    bool isPointInStroke(float x, float y);
    bool isPointInStroke(Path2D* path, float x, float y);

    // CanvasPathInterfaceMixIn methods
    virtual void closePath() override;
    virtual void moveTo(float x, float y) override;
    virtual void lineTo(float x, float y) override;
    virtual void quadraticCurveTo(float cpx, float cpy, float x,
                                  float y) override;
    virtual void bezierCurveTo(float cp1x, float cp1y, float cp2x, float cp2y,
                               float x, float y) override;
    virtual void arcTo(float x1, float y1, float x2, float y2,
                       float radius) override;
    virtual void rect(float x, float y, float w, float h) override;
    virtual void arc(float x, float y, float radius, float startAngle,
                     float endAngle, bool anticlockwise = false) override;
    virtual void ellipse(float x, float y, float radiusX, float radiusY,
                         float rotation, float startAngle, float endAngle,
                         bool anticlockwise = false) override;

    // TODO : CanvasUserInterface
    // TODO :CanvasText

    // CanvasDrawImage
    void drawImage(CanvasImageSource image, float dx, float dy);
    void drawImage(CanvasImageSource image, float dx, float dy, float dw,
                   float dh);
    void drawImage(CanvasImageSource image, float sx, float sy, float sw,
                   float sh, float dx, float dy, float dw, float dh);

    // CanvasImageData
    ImageData* createImageData(int32_t sw, int32_t sh);
    ImageData* createImageData(ImageData* imagedata);
    ImageData* getImageData(int32_t sx, int32_t sy, int32_t sw, int32_t sh);
    void putImageData(ImageData* imagedata, int32_t dx, int32_t dy);
    void putImageData(ImageData* imagedata, int32_t dx, int32_t dy,
                      int32_t dirtyX, int32_t dirtyY, int32_t dirtyWidth,
                      int32_t dirtyHeight);

    // CanvasPathDrawingStyles
    float lineWidth();
    void setLineWidth(float width);

    String* lineCap();
    void setLineCap(String* value);

    String* lineJoin();
    void setLineJoin(String* value);

    float miterLimit();
    void setMiterLimit(float limit);

    void setLineDash(GCVector<double> segments);
    GCVector<double> getLineDash();
    double lineDashOffset();
    void setLineDashOffset(double offset);

    // TODO : CanvasTextDrawingStyles

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
        GC_set_bit(desc, GC_WORD_OFFSET(CanvasRenderingContext2DMixIn,
                                        m_ownerHTMLCanvasElement));
        GC_set_bit(desc, GC_WORD_OFFSET(CanvasRenderingContext2DMixIn,
                                        m_canvasSurface));
        GC_set_bit(desc,
                   GC_WORD_OFFSET(CanvasRenderingContext2DMixIn, m_canvas));
        GC_set_bit(desc,
                   GC_WORD_OFFSET(CanvasRenderingContext2DMixIn, m_canvasPath));
        GC_set_bit(desc,
                   GC_WORD_OFFSET(CanvasRenderingContext2DMixIn, m_dashList));
    }
    HTMLCanvasElement* m_ownerHTMLCanvasElement;

private:
    void fill(Path* path, String* fillRule);
    void stroke(Path* path);
    void clip(Path* path, String* fillRule);
    bool isPointInPath(Path* path, float x, float y, String* fillRule);
    bool isPointInStroke(Path* path, float x, float y);
    void getPointsUnaffectedByCurrentTransformation(const float& x,
                                                    const float& y, float& ux,
                                                    float& uy);
    void setLineCap(CanvasLineCap lineCap);
    void setLineJoin(CanvasLineJoin lineJoin);
    void transform(float a, float b, float c, float d, float e, float f,
                   bool needResetMatrix);
    void setLineDashToCanvas();
    DOMExceptionOr<bool> checkUsabilityOfCanvasImageSource(
        CanvasImageSource image);
    void markOriginCleanFlagDirtyIfNeeds(WebOrigin* webOrigin);

    CanvasSurface* m_canvasSurface;
    Canvas* m_canvas;
    CanvasPath* m_canvasPath;
    GCVector<double> m_dashList;
    double m_lineDashOffset;
};
}
#endif
#endif
