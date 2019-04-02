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
#include "core/modules/canvas/Path.h"
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

static CanvasFillRule StringToCanvasFillRule(String* rule)
{
    if (rule) {
        if (rule->equals("nonzero")) {
            return CanvasFillRule::CanvasFillRuleNonZero;
        } else if (rule->equals("evenodd")) {
            return CanvasFillRule::CanvasFillRuleEvenOdd;
        }
    }
    return CanvasFillRule::CanvasFillRuleInvalid;
}

CanvasRenderingContext2DMixIn::CanvasRenderingContext2DMixIn(
    HTMLCanvasElement* ownerHTMLCanvasElement)
    : CanvasRenderingContext(ownerHTMLCanvasElement->executionContext())
    , m_ownerHTMLCanvasElement(ownerHTMLCanvasElement)
    , m_canvasSurface(nullptr)
    , m_canvas(nullptr)
    , m_canvasPath(nullptr)
    , m_fillColor()
    , m_strokeColor()
    , m_lineWidth(1)
    , m_globalAlpha(1.0)
{
    initialize();
    GC_REGISTER_FINALIZER_NO_ORDER(this,
                                   [](void* obj, void* cd) {
                                       CanvasRenderingContext2DMixIn* c =
                                           (CanvasRenderingContext2DMixIn*)obj;
                                       c->finalize();
                                   },
                                   NULL, NULL, NULL);
}

void CanvasRenderingContext2DMixIn::initialize()
{
    STARFISH_ASSERT(m_canvasSurface == nullptr);
    STARFISH_ASSERT(m_canvas == nullptr);

    // Create CanvasSurface.
    m_canvasSurface = CanvasSurface::create(
        m_ownerHTMLCanvasElement->webView()->platformWindow(),
        m_ownerHTMLCanvasElement->width(), m_ownerHTMLCanvasElement->height(),
        CanvasSurface::CanvasElement);

    auto black = Unit::Color(0, 0, 0, 255);
    // Set defualt values such as color, fill color and stroke color.
    m_canvas =
        Canvas::create(m_ownerHTMLCanvasElement->webView(), m_canvasSurface);
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

    m_canvasPath = new CanvasPath(executionContext());
}

void CanvasRenderingContext2DMixIn::finalize()
{
    STARFISH_ASSERT(m_canvasSurface);
    STARFISH_ASSERT(m_canvas);
    // Do not call m_canvasSurface's detachNativeBuffer, it will be called in
    // GC_REGISTER_FINALIZER_NO_ORDER registered by CanvasSurface
    m_canvasSurface = nullptr;

    delete m_canvas;
    m_canvas = nullptr;
}

void CanvasRenderingContext2DMixIn::flush()
{
    m_canvas->flush();
}

void CanvasRenderingContext2DMixIn::onResize()
{
    finalize();
    initialize();

    m_ownerHTMLCanvasElement->setNeedsPainting();
}

void CanvasRenderingContext2DMixIn::setLineWidth(float width)
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

void CanvasRenderingContext2DMixIn::scale(float x, float y)
{
    m_canvas->scale(x, y);
}

void CanvasRenderingContext2DMixIn::rotate(float angle)
{
    m_canvas->rotate(angle);
}

void CanvasRenderingContext2DMixIn::translate(float x, float y)
{
    m_canvas->translate(x, y);
}

void CanvasRenderingContext2DMixIn::transform(float a, float b, float c,
                                              float d, float e, float f)
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

float CanvasRenderingContext2DMixIn::globalAlpha()
{
    return m_globalAlpha;
}

void CanvasRenderingContext2DMixIn::setGlobalAlpha(float value)
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
        CSSStyleValuePair pair;
        if (pair.updateValueUnitColor(token)) {
            if (pair.valueKind() ==
                CSSStyleValuePair::ValueKind::ColorValueKind) {
                m_fillColor = pair.colorValue();
            } else if (pair.valueKind() ==
                       CSSStyleValuePair::ValueKind::NamedColorValueKind) {
                m_fillColor =
                    NamedColor::namedColorToColor(pair.namedColorValue());
            }
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
        CSSTokenValue token(v->toUTF8NonGCString());
        CSSStyleValuePair pair;
        if (pair.updateValueUnitColor(token)) {
            if (pair.valueKind() ==
                CSSStyleValuePair::ValueKind::ColorValueKind) {
                m_strokeColor = pair.colorValue();
            } else if (pair.valueKind() ==
                       CSSStyleValuePair::ValueKind::NamedColorValueKind) {
                m_strokeColor =
                    NamedColor::namedColorToColor(pair.namedColorValue());
            }
        }
    } else {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
}

CanvasGradient* CanvasRenderingContext2DMixIn::createLinearGradient(float x0,
                                                                    float y0,
                                                                    float x1,
                                                                    float y1)
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

void CanvasRenderingContext2DMixIn::fillRect(float x, float y, float w, float h)
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

void CanvasRenderingContext2DMixIn::strokeRect(float x, float y, float w,
                                               float h)
{
    rect(x, y, w, h);
    stroke();
}

void CanvasRenderingContext2DMixIn::beginPath()
{
    m_canvasPath->path()->clear();
}

void CanvasRenderingContext2DMixIn::fill(String* fillRule)
{
    fill(m_canvasPath->path(), fillRule);
}

void CanvasRenderingContext2DMixIn::fill(Path2D* path, String* fillRule)
{
}

void CanvasRenderingContext2DMixIn::stroke()
{
    stroke(m_canvasPath->path());
}

void CanvasRenderingContext2DMixIn::closePath()
{
    m_canvasPath->closePath();
}

void CanvasRenderingContext2DMixIn::moveTo(float x, float y)
{
    m_canvasPath->moveTo(x, y);
}

void CanvasRenderingContext2DMixIn::lineTo(float x, float y)
{
    m_canvasPath->lineTo(x, y);
}

void CanvasRenderingContext2DMixIn::quadraticCurveTo(float cpx, float cpy,
                                                     float x, float y)
{
    m_canvasPath->quadraticCurveTo(cpx, cpy, x, y);
}

void CanvasRenderingContext2DMixIn::bezierCurveTo(float cp1x, float cp1y,
                                                  float cp2x, float cp2y,
                                                  float x, float y)
{
    m_canvasPath->bezierCurveTo(cp1x, cp1y, cp2x, cp2y, x, y);
}

void CanvasRenderingContext2DMixIn::arcTo(float x1, float y1, float x2,
                                          float y2, float radius)
{
    m_canvasPath->arcTo(x1, y1, x2, y2, radius);
}

void CanvasRenderingContext2DMixIn::rect(float x, float y, float w, float h)
{
    m_canvasPath->rect(x, y, w, h);
}

void CanvasRenderingContext2DMixIn::arc(float x, float y, float radius,
                                        float startAngle, float endAngle,
                                        bool anticlockwise /*=false*/)
{
    m_canvasPath->arc(x, y, radius, startAngle, endAngle, anticlockwise);
}

void CanvasRenderingContext2DMixIn::ellipse(float x, float y, float radiusX,
                                            float radiusY, float rotation,
                                            float startAngle, float endAngle,
                                            bool anticlockwise /*=false*/)
{
    m_canvasPath->ellipse(x, y, radiusX, radiusY, rotation, startAngle,
                          endAngle, anticlockwise);
}

void CanvasRenderingContext2DMixIn::drawImage(ScriptValue image, float dx,
                                              float dy)
{
}

void CanvasRenderingContext2DMixIn::drawImage(ScriptValue image, float dx,
                                              float dy, float dw, float dh)
{
}

void CanvasRenderingContext2DMixIn::drawImage(ScriptValue image, float sx,
                                              float sy, float sw, float sh,
                                              float dx, float dy, float dw,
                                              float dh)
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
    if (m_canvasSurface->bufferWidth() && m_canvasSurface->bufferStride()) {
        stride =
            m_canvasSurface->bufferStride() / m_canvasSurface->bufferWidth();
    } else {
        stride = 4;
    }

    size_t destSize = sw * stride * sh;

    // TODO : If the Canvas Pixel ArrayBuffer cannot be allocated, then rethrow
    // the RangeError thrown by JavaScript, and return.
    auto canvasPixelArrayBuffer = createArrayBuffer(
        executionContext()->scriptBindingInstance(), destSize);
    ContextRef* ctx =
        executionContext()->scriptBindingInstance()->scriptContext();
    ExecutionStateRef* state = ExecutionStateRef::create(ctx);
    uint8_t* dest = canvasPixelArrayBuffer->toObject(state)
                        ->asArrayBufferObject()
                        ->rawBuffer();

    auto width = m_canvasSurface->bufferWidth();
    auto height = m_canvasSurface->bufferHeight();

    uint8_t* src = m_canvasSurface->mapBuffer();
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
        executionContext()->scriptBindingInstance());

    uint8ClampedArray->setBuffer(
        canvasPixelArrayBuffer->toObject(state)->asArrayBufferObject(), 0,
        destSize, destSize);

    auto ret = new ImageData(executionContext(), uint8ClampedArray);
    ret->setWidth(sw);
    ret->setHeight(sh);

    state->destroy();

    return ret;
}

void CanvasRenderingContext2DMixIn::clearRect(float x, float y, float w,
                                              float h)
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

void CanvasRenderingContext2DMixIn::fill(Path* path, String* fillRule)
{
    m_ownerHTMLCanvasElement->setNeedsPainting();

    auto rule = StringToCanvasFillRule(fillRule);
    if (!path->isEmpty()) {
        m_canvas->save();
        if (rule == CanvasFillRule::CanvasFillRuleNonZero) {
            m_canvas->setFillRule(true);
        } else if (rule == CanvasFillRule::CanvasFillRuleEvenOdd) {
            m_canvas->setFillRule(false);
        }
        Unit::Color color =
            Unit::Color(m_fillColor.r(), m_fillColor.g(), m_fillColor.b(),
                        m_fillColor.a() * m_globalAlpha);
        m_canvas->setColor(color);
        m_canvas->fillPath(path);
        m_canvas->restore();
    }
}
void CanvasRenderingContext2DMixIn::stroke(Path* path)
{
    m_ownerHTMLCanvasElement->setNeedsPainting();

    if (!path->isEmpty()) {
        m_canvas->save();
        Unit::Color color =
            Unit::Color(m_strokeColor.r(), m_strokeColor.g(), m_strokeColor.b(),
                        m_strokeColor.a() * m_globalAlpha);
        m_canvas->setColor(color);
        m_canvas->strokePath(path);
        m_canvas->restore();
    }
}
}

#endif
