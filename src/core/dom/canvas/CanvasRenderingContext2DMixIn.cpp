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

#ifdef STARFISH_ENABLE_CANVAS

#include "StarfishConfig.h"
#include "core/dom/ExecutionContext.h"
#include "binding/DOMStringOrCanvasGradientOrCanvasPatternUnion.h"
#include "core/style/Style.h"
#include "core/style/ComputedStyle.h"
#include "core/dom/Node.h"
#include "core/modules/canvas/Canvas.h"
#include "core/dom/canvas/ImageData.h"
#include "core/dom/canvas/Path2D.h"
#include "core/dom/canvas/CanvasGradient.h"
#include "core/dom/canvas/CanvasPattern.h"
#include "core/dom/canvas/HTMLCanvasElement.h"
#include "core/dom/Document.h"
#include "core/page/WebView.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSTokenValue.h"
#include "EscargotPublic.h"
#include "core/dom/canvas/CanvasRenderingContext.h"
#include "core/dom/canvas/CanvasRenderingContext2DMixIn.h"

namespace Starfish {

CanvasRenderingContext2DMixIn::CanvasRenderingContext2DMixIn(
    HTMLCanvasElement* ownerHTMLCanvasElement)
    : CanvasRenderingContext(ownerHTMLCanvasElement->executionContext())
    , m_ownerHTMLCanvasElement(ownerHTMLCanvasElement)
    , m_canvas(nullptr)
    , m_fillColor()
    , m_strokeColor()
    , m_lineWidth(1)
    , m_globalAlpha(1.0)
{
    initialize();
}

void CanvasRenderingContext2DMixIn::initialize()
{
    if (m_surface) {
        m_surface->detachNativeBuffer();
    }

    // Create CanvasSurface.
    m_surface = CanvasSurface::create(
        m_ownerHTMLCanvasElement->webView()->platformWindow(),
        m_ownerHTMLCanvasElement->width(), m_ownerHTMLCanvasElement->height(),
        CanvasSurface::CanvasElement);

    if (m_canvas) {
        delete m_canvas;
    }
    auto black = Unit::Color(0, 0, 0, 255);
    // Set defualt values such as color, fill color and stroke color.
    m_canvas = Canvas::create(m_ownerHTMLCanvasElement->webView(), m_surface);
    // Set the defualt color as black.
    m_canvas->setColor(black);
    // Set the fill color as black.
    m_fillColor = black;
    // Set the stroke color as black.
    m_strokeColor = black;
    // Set the line width as 1.0f.
    m_lineWidth = 1.0f;
    m_canvas->setStrokeWidth(m_lineWidth);
    m_canvas->clearColor(Unit::Color(0, 0, 0, 0));
}
void CanvasRenderingContext2DMixIn::flush()
{
    m_canvas->flush();
}

void CanvasRenderingContext2DMixIn::setLineWidth(double width)
{
    m_lineWidth = width;
    m_canvas->setStrokeWidth(m_lineWidth);
}

void CanvasRenderingContext2DMixIn::save()
{
    m_canvas->save();
}

void CanvasRenderingContext2DMixIn::restore()
{
    m_canvas->restore();
}

void CanvasRenderingContext2DMixIn::scale(double x, double y)
{
    m_canvas->scale(x, y);
}

void CanvasRenderingContext2DMixIn::rotate(double angle)
{
    m_canvas->rotate(angle);
}

void CanvasRenderingContext2DMixIn::translate(double x, double y)
{
    m_canvas->translate(x, y);
}

void CanvasRenderingContext2DMixIn::transform(double a, double b, double c,
                                              double d, double e, double f)
{
    SkMatrix matrix;
    matrix.reset();
    matrix.set(0, a);
    matrix.set(1, c);
    matrix.set(2, e);
    matrix.set(3, b);
    matrix.set(4, d);
    matrix.set(5, f);
    m_canvas->postMatrix(matrix);
}

double CanvasRenderingContext2DMixIn::globalAlpha()
{
    return m_globalAlpha;
}

void CanvasRenderingContext2DMixIn::setGlobalAlpha(double value)
{
    if (value < .0f || value > 1.f) {
        return;
    }

    m_globalAlpha = value;
}

String* CanvasRenderingContext2DMixIn::globalCompositeOperation()
{
    return String::fromUTF8("source-over");
}

void CanvasRenderingContext2DMixIn::setGlobalCompositeOperation(String* value)
{
}

DOMStringOrCanvasGradientOrCanvasPattern
CanvasRenderingContext2DMixIn::fillStyle()
{
    return DOMStringOrCanvasGradientOrCanvasPattern::createDOMString(
        String::fromUTF8("black"));
}

void CanvasRenderingContext2DMixIn::setFillStyle(
    DOMStringOrCanvasGradientOrCanvasPattern value)
{
    if (value.isDOMStringValue()) {
        String* v = value.getDOMStringValue();
        CSSTokenValue token(v->toUTF8NonGCString());
        CSSStyleValuePair ret;
        if (CSSPropertyParser::parseNonNamedColor(token, &ret)) {
            m_fillColor = ret.colorValue();
            return;
        }
        if (CSSPropertyParser::parseNamedColor(token, &ret)) {
            m_fillColor = NamedColor::namedColorToColor(ret.namedColorValue());
        }
    } else {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
}

DOMStringOrCanvasGradientOrCanvasPattern
CanvasRenderingContext2DMixIn::strokeStyle()
{
    return DOMStringOrCanvasGradientOrCanvasPattern::createDOMString(
        String::fromUTF8("black"));
}

void CanvasRenderingContext2DMixIn::setStrokeStyle(
    DOMStringOrCanvasGradientOrCanvasPattern value)
{
    if (value.isDOMStringValue()) {
        String* v = value.getDOMStringValue();
        auto s1 = v->toUTF8NonGCString();
        NamedColor::NamedColorValue ret;
        if (NamedColor::parseNamedColor(s1.data(), s1.length(), ret)) {
            Unit::Color c = NamedColor::namedColorToColor(ret);
            m_strokeColor = Unit::Color(c.r(), c.g(), c.b(), c.a());
        }
    }
}

CanvasGradient* CanvasRenderingContext2DMixIn::createLinearGradient(double x0,
                                                                    double y0,
                                                                    double x1,
                                                                    double y1)
{
    return new CanvasGradient(this);
}

String* CanvasRenderingContext2DMixIn::filter()
{
    return String::fromUTF8("none");
}

void CanvasRenderingContext2DMixIn::setFilter(String* value)
{
}

void CanvasRenderingContext2DMixIn::fillRect(double x, double y, double w,
                                             double h)
{
    // https://html.spec.whatwg.org/multipage/canvas.html#dom-context-2d-fillrect
    if (std::isnan(x) || std::isnan(y) || std::isnan(w) || std::isnan(h) ||
        std::isinf(x) || std::isinf(y) || std::isinf(w) || std::isinf(h)) {
        return;
    }

    if (!w || !h) {
        return;
    }

    m_ownerHTMLCanvasElement->setNeedsPainting();
    Unit::Color color =
        Unit::Color(m_fillColor.r(), m_fillColor.g(), m_fillColor.b(),
                    m_fillColor.a() * m_globalAlpha);
    m_canvas->setColor(color);
    m_canvas->drawRect(LayoutRect(x, y, w, h));
}

void CanvasRenderingContext2DMixIn::strokeRect(double x, double y, double w,
                                               double h)
{
    rect(x, y, w, h);
    stroke();
}

void CanvasRenderingContext2DMixIn::beginPath()
{
}

void CanvasRenderingContext2DMixIn::fill(String* fillRule)
{
}

void CanvasRenderingContext2DMixIn::fill(Path2D* path, String* fillRule)
{
}

void CanvasRenderingContext2DMixIn::stroke()
{
    m_ownerHTMLCanvasElement->setNeedsPainting();
    Unit::Color color =
        Unit::Color(m_strokeColor.r(), m_strokeColor.g(), m_strokeColor.b(),
                    m_strokeColor.a() * m_globalAlpha);
    m_canvas->setColor(color);
    m_canvas->stroke();
}

void CanvasRenderingContext2DMixIn::drawImage(ScriptValue image, double dx,
                                              double dy)
{
}

void CanvasRenderingContext2DMixIn::drawImage(ScriptValue image, double dx,
                                              double dy, double dw, double dh)
{
}

void CanvasRenderingContext2DMixIn::drawImage(ScriptValue image, double sx,
                                              double sy, double sw, double sh,
                                              double dx, double dy, double dw,
                                              double dh)
{
}

ImageData* CanvasRenderingContext2DMixIn::getImageData(int32_t sx, int32_t sy,
                                                       int32_t sw, int32_t sh)
{
    if (!sw || !sh) {
        throw new DOMException(executionContext(),
                               DOMException::Code::INDEX_SIZE_ERR,
                               "sw and sh are must not zero");
    }

    if (!originCleanFlag()) {
        throw new DOMException(executionContext(),
                               DOMException::Code::SECURITY_ERR);
    }

    flush();

    size_t stride = 0;
    if (m_surface->bufferWidth() && m_surface->bufferStride()) {
        stride = m_surface->bufferStride() / m_surface->bufferWidth();
    } else {
        stride = 4;
    }

    size_t destSize = sw * stride * sh;

    // TODO : If the Canvas Pixel ArrayBuffer cannot be allocated, then rethrow
    // the RangeError thrown by JavaScript, and return.
    auto canvasPixelArrayBuffer = createArrayBuffer(
        executionContext()->ownerScriptBindingInstance(), destSize);
    ContextRef* ctx =
        executionContext()->ownerScriptBindingInstance()->scriptContext();
    ExecutionStateRef* state = ExecutionStateRef::create(ctx);
    uint8_t* dest = canvasPixelArrayBuffer->toObject(state)
                        ->asArrayBufferObject()
                        ->rawBuffer();

    auto width = m_surface->bufferWidth();
    auto height = m_surface->bufferHeight();

    uint8_t* src = m_surface->mapBuffer();
    for (size_t y = 0; y < (size_t)sh && (y + sy) < height; ++y) {
        for (size_t x = 0; x < (size_t)sw && (x + sx) < width; ++x) {
            uint8_t* destPixel = dest + (y * sw * stride) + (x * stride);
            uint8_t* srcPixel =
                src + ((y + sy) * width * stride) + ((x + sx) * stride);
            // R
            destPixel[0] = srcPixel[2];
            // G
            destPixel[1] = srcPixel[1];
            // B
            destPixel[2] = srcPixel[0];
            // A
            destPixel[3] = srcPixel[3];
        }
    }
    auto uint8ClampedArray = createEmptyUint8ClampedArray(
        executionContext()->ownerScriptBindingInstance());

    uint8ClampedArray->setBuffer(
        canvasPixelArrayBuffer->toObject(state)->asArrayBufferObject(), 0,
        destSize, destSize);

    auto ret = new ImageData(executionContext(), uint8ClampedArray);
    ret->setWidth(sw);
    ret->setHeight(sh);

    state->destroy();

    return ret;
}

void CanvasRenderingContext2DMixIn::closePath()
{
}

void CanvasRenderingContext2DMixIn::moveTo(double x, double y)
{
    m_ownerHTMLCanvasElement->setNeedsPainting();
    m_canvas->moveTo(x, y);
}

void CanvasRenderingContext2DMixIn::lineTo(double x, double y)
{
    m_ownerHTMLCanvasElement->setNeedsPainting();
    m_canvas->lineTo(x, y);
}

void CanvasRenderingContext2DMixIn::rect(double x, double y, double w, double h)
{
    m_ownerHTMLCanvasElement->setNeedsPainting();
    m_canvas->moveTo(x - m_lineWidth / 2, y);
    m_canvas->lineTo(x + w, y);
    m_canvas->lineTo(x + w, y + h);
    m_canvas->lineTo(x, y + h);
    m_canvas->lineTo(x, y);
}

void CanvasRenderingContext2DMixIn::arc(double x, double y, double radius,
                                        double startAngle, double endAngle,
                                        bool anticlockwise)
{
}

void CanvasRenderingContext2DMixIn::ellipse(double x, double y, double radiusX,
                                            double radiusY, double rotation,
                                            double startAngle, double endAngle,
                                            bool anticlockwise)
{
}

void CanvasRenderingContext2DMixIn::bezierCurveTo(double x1, double y1,
                                                  double x2, double y2,
                                                  double x3, double y3)
{
    m_ownerHTMLCanvasElement->setNeedsPainting();
    m_canvas->curveTo(x1, y1, x2, y2, x3, y3);
}

void CanvasRenderingContext2DMixIn::clearRect(double x, double y, double w,
                                              double h)
{
    // https://html.spec.whatwg.org/multipage/canvas.html#dom-context-2d-clearrect
    if (std::isnan(x) || std::isnan(y) || std::isnan(w) || std::isnan(h) ||
        std::isinf(x) || std::isinf(y) || std::isinf(w) || std::isinf(h)) {
        return;
    }
    m_ownerHTMLCanvasElement->setNeedsPainting();
    m_canvas->save();
    m_canvas->setColor(Unit::Color(0, 0, 0, 0));
    m_canvas->drawRect(LayoutRect(x, y, w, h));
    m_canvas->fill();
    m_canvas->restore();
}
}

#endif
