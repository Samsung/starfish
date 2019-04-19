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
#include "binding/ScriptBindingInstance.h"
#include "core/style/Style.h"
#include "core/style/ComputedStyle.h"
#include "core/dom/Node.h"
#include "core/dom/canvas/CanvasFillRule.h"
#include "core/dom/canvas/CanvasLineCap.h"
#include "core/dom/canvas/CanvasLineJoin.h"
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

#ifndef CRASH
#define CRASH STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE
#endif
#include "../third_party/escargot/third_party/checked_arithmetic/CheckedArithmetic.h"

#if defined(PORT_CANVAS_BACKEND_CAIRO) || defined(PORT_CANVAS_BACKEND_SKIA)
#define NEEDS_UNPREMULTIPLIED
#endif

namespace Starfish {

static inline CanvasFillRule stringToCanvasFillRule(String* rule)
{
    if (rule && rule->equals("evenodd")) {
        return CanvasFillRule::EvenOdd;
    }
    return CanvasFillRule::NonZero;
}

static inline String* canvasLineCapToString(CanvasLineCap lineCap)
{
    if (lineCap == CanvasLineCap::Round) {
        return String::createASCIIString("round");
    } else if (lineCap == CanvasLineCap::Square) {
        return String::createASCIIString("square");
    }
    return String::createASCIIString("butt");
}

static inline bool stringToCanvasLineCap(String* lineCap, CanvasLineCap& out)
{
    if (lineCap) {
        if (lineCap->equals("round")) {
            out = CanvasLineCap::Round;
            return true;
        } else if (lineCap->equals("square")) {
            out = CanvasLineCap::Square;
            return true;
        } else if (lineCap->equals("butt")) {
            out = CanvasLineCap::Butt;
            return true;
        }
    }
    return false;
}

static inline String* canvasLineJoinToString(CanvasLineJoin lineJoin)
{
    if (lineJoin == CanvasLineJoin::Round) {
        return String::createASCIIString("round");
    } else if (lineJoin == CanvasLineJoin::Bevel) {
        return String::createASCIIString("bevel");
    }
    return String::createASCIIString("miter");
}

static inline bool stringToCanvasLineJoin(String* lineJoin, CanvasLineJoin& out)
{
    if (lineJoin) {
        if (lineJoin->equals("round")) {
            out = CanvasLineJoin::Round;
            return true;
        } else if (lineJoin->equals("bevel")) {
            out = CanvasLineJoin::Bevel;
            return true;
        } else if (lineJoin->equals("miter")) {
            out = CanvasLineJoin::Miter;
            return true;
        }
    }
    return false;
}

CanvasRenderingContext2DMixIn::CanvasRenderingContext2DMixIn(
    HTMLCanvasElement* ownerHTMLCanvasElement)
    : CanvasRenderingContext(ownerHTMLCanvasElement->executionContext())
    , m_ownerHTMLCanvasElement(ownerHTMLCanvasElement)
    , m_canvasSurface(nullptr)
    , m_canvas(nullptr)
    , m_canvasPath(nullptr)
    , m_dashList()
    , m_lineDashOffset(0.0)
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

    m_canvasSurface = CanvasSurface::create(
        m_ownerHTMLCanvasElement->webView()->platformWindow(),
        m_ownerHTMLCanvasElement->width(), m_ownerHTMLCanvasElement->height(),
        CanvasSurface::CanvasElement);
    m_canvas =
        Canvas::create(m_ownerHTMLCanvasElement->webView(), m_canvasSurface);
    m_canvas->clearColor(Unit::Color(0, 0, 0, 0));
    m_canvasPath = new CanvasPath(executionContext());

    auto black = Unit::Color(0, 0, 0, 255);
    setLineWidth(1.0f);                 // default 1.0
    setLineCap(CanvasLineCap::Butt);    // default "butt"
    setLineJoin(CanvasLineJoin::Miter); // default "miter"
    setMiterLimit(10.0f);

    setLineDash(GCVector<double>()); // default empty
    setLineDashOffset(0.0f);         // default 0.0

    m_canvas->setFillColor(black);
    m_canvas->setStrokeColor(black);
    m_canvas->setGlobalAlpha(1.0f);
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

    m_ownerHTMLCanvasElement->setNeedsComposite();
}

float CanvasRenderingContext2DMixIn::lineWidth()
{
    return m_canvas->lineWidth();
}

void CanvasRenderingContext2DMixIn::setLineWidth(float width)
{
    if (width <= 0 || isInfOrNan(width)) {
        return;
    }

    m_canvas->setLineWidth(width);
}

String* CanvasRenderingContext2DMixIn::lineCap()
{
    auto cap = m_canvas->lineCap();
    return canvasLineCapToString(cap);
}

void CanvasRenderingContext2DMixIn::setLineCap(String* value)
{
    CanvasLineCap cap;
    if (stringToCanvasLineCap(value, cap)) {
        setLineCap(cap);
    }
}

void CanvasRenderingContext2DMixIn::setLineCap(CanvasLineCap lineCap)
{
    m_canvas->setLineCap(lineCap);
}

String* CanvasRenderingContext2DMixIn::lineJoin()
{
    auto join = m_canvas->lineJoine();
    return canvasLineJoinToString(join);
}

void CanvasRenderingContext2DMixIn::setLineJoin(String* value)
{
    CanvasLineJoin join;
    if (stringToCanvasLineJoin(value, join)) {
        setLineJoin(join);
    }
}

void CanvasRenderingContext2DMixIn::setLineJoin(CanvasLineJoin lineJoin)
{
    m_canvas->setLineJoin(lineJoin);
}

float CanvasRenderingContext2DMixIn::miterLimit()
{
    return m_canvas->miterLimit();
}

void CanvasRenderingContext2DMixIn::setMiterLimit(float limit)
{
    if (limit <= 0 || isInfOrNan(limit)) {
        return;
    }
    m_canvas->setMiterLimit(limit);
}

void CanvasRenderingContext2DMixIn::setLineDash(GCVector<double> segments)
{
    for (auto& segment : segments) {
        if (isInfOrNan(segment) || segment < 0) {
            return;
        }
    }

    m_dashList = segments;
    if (segments.size() % 2 != 0) {
        for (auto& segment : segments) {
            m_dashList.emplace_back(segment);
        }
    }
    setLineDashToCanvas();
}

GCVector<double> CanvasRenderingContext2DMixIn::getLineDash()
{
    return m_dashList;
}

double CanvasRenderingContext2DMixIn::lineDashOffset()
{
    return m_lineDashOffset;
}

void CanvasRenderingContext2DMixIn::setLineDashOffset(double offset)
{
    if (isInfOrNan(offset)) {
        return;
    }
    m_lineDashOffset = offset;
    setLineDashToCanvas();
}

void CanvasRenderingContext2DMixIn::setLineDashToCanvas()
{
    auto size = m_dashList.size();
    if (size) {
        double* dashes = new double[size];
        for (size_t i = 0; i < size; ++i) {
            dashes[i] = m_dashList[i];
        }

        m_canvas->setDash(dashes, static_cast<int>(size), m_lineDashOffset);
        delete[] dashes;
    }
}

void CanvasRenderingContext2DMixIn::save()
{
    m_canvas->setPathTransformMatrix(m_canvasPath->path()->getCTM());
    m_canvas->save();
}

void CanvasRenderingContext2DMixIn::restore()
{
    m_canvas->restore();
    m_canvasPath->path()->setCTM(m_canvas->pathTransformMatrix());
}

void CanvasRenderingContext2DMixIn::scale(float x, float y)
{
    if (m_canvas->hasNonInvertableCTM()) {
        return;
    }
    transform(x, 0, 0, y, 0, 0, false);
}

void CanvasRenderingContext2DMixIn::rotate(float angle)
{
    if (m_canvas->hasNonInvertableCTM()) {
        return;
    }
    float cosValue = cosf(angle);
    float sinValue = sinf(angle);
    transform(cosValue, sinValue, -sinValue, cosValue, 0, 0, false);
}

void CanvasRenderingContext2DMixIn::translate(float x, float y)
{
    if (m_canvas->hasNonInvertableCTM()) {
        return;
    }
    transform(1, 0, 0, 1, x, y, false);
}

void CanvasRenderingContext2DMixIn::transform(float a, float b, float c,
                                              float d, float e, float f,
                                              bool needResetMatrix)
{
    if (isInfOrNan(a) || isInfOrNan(b) || isInfOrNan(c) || isInfOrNan(d) ||
        isInfOrNan(e) || isInfOrNan(f)) {
        return;
    }

    if (needResetMatrix) {
        resetTransform();
    }

    SkMatrix matrix;
    SkMatrix invertMatrix;
    matrix.reset();
    matrix.set(0, a);
    matrix.set(1, c);
    matrix.set(2, e);
    matrix.set(3, b);
    matrix.set(4, d);
    matrix.set(5, f);
    if (matrix.invert(&invertMatrix)) {
        m_canvasPath->setShouldDisable(false);
        m_canvasPath->path()->postMatrix(matrix);

        m_canvas->postMatrix(matrix);
        m_canvas->setNonInvertableCTM(false);
        return;
    }
    m_canvas->setNonInvertableCTM(true);
    m_canvasPath->setShouldDisable(true);
}

void CanvasRenderingContext2DMixIn::transform(float a, float b, float c,
                                              float d, float e, float f)
{
    transform(a, b, c, d, e, f, false);
}

void CanvasRenderingContext2DMixIn::setTransform(float a, float b, float c,
                                                 float d, float e, float f)
{
    transform(a, b, c, d, e, f, true);
}

void CanvasRenderingContext2DMixIn::resetTransform()
{
    m_canvas->setNonInvertableCTM(false);
    m_canvas->resetMatrixAndClip();
    m_canvasPath->setShouldDisable(false);
    m_canvasPath->path()->resetCTM();
}

float CanvasRenderingContext2DMixIn::globalAlpha()
{
    return m_canvas->globalAlpha();
}

void CanvasRenderingContext2DMixIn::setGlobalAlpha(float value)
{
    if (isInfOrNan(value) || value < .0f || value > 1.f) {
        return;
    }
    m_canvas->setGlobalAlpha(value);
}

String* CanvasRenderingContext2DMixIn::globalCompositeOperation()
{
    STARFISH_ASSERT(m_canvas != nullptr);

    if (m_canvas->blendMode() != CanvasBlendMode::Normal) {
        return String::fromUTF8(
            CanvasCompositing::canvasBlendModeNames[static_cast<unsigned>(
                m_canvas->blendMode())]);
    }
    return String::fromUTF8(
        CanvasCompositing::canvasCompositeOperatorNames[static_cast<unsigned>(
            m_canvas->compositeOperator())]);
}

void CanvasRenderingContext2DMixIn::setGlobalCompositeOperation(String* value)
{
    CanvasCompositeOperator cco = CanvasCompositeOperator::SourceOver;
    CanvasBlendMode cbm = CanvasBlendMode::Normal;

    for (int i = 0; i < CanvasCompositing::sizeOfCanvasCompositeOperatorNames;
         i++) {
        if (value->equals(CanvasCompositing::canvasCompositeOperatorNames[i])) {
            cco = static_cast<CanvasCompositeOperator>(i);
            m_canvas->setCompositeOperator(cco, cbm);
            return;
        }
    }
    for (int i = 0; i < CanvasCompositing::sizeOfCanvasBlendModeNames; i++) {
        if (value->equals(CanvasCompositing::canvasBlendModeNames[i])) {
            cbm = static_cast<CanvasBlendMode>(i);
            cco = CanvasCompositeOperator::SourceOver;
            m_canvas->setCompositeOperator(cco, cbm);
            return;
        }
    }
}

DOMStringOrCanvasGradientOrCanvasPattern
CanvasRenderingContext2DMixIn::fillStyle()
{
    return DOMStringOrCanvasGradientOrCanvasPattern::createDOMString(
        m_canvas->color().toHTMLColorCodeString());
}

void CanvasRenderingContext2DMixIn::setFillStyle(
    DOMStringOrCanvasGradientOrCanvasPattern value)
{
    if (value.isDOMStringValue()) {
        String* v = value.getDOMStringValue();
        CSSTokenValue token(v->toUTF8NonGCString());
        CSSStyleValuePair pair;
        Unit::Color fillColor;
        if (pair.updateValueUnitColor(token)) {
            if (pair.valueKind() ==
                CSSStyleValuePair::ValueKind::ColorValueKind) {
                fillColor = pair.colorValue();
            } else if (pair.valueKind() ==
                       CSSStyleValuePair::ValueKind::NamedColorValueKind) {
                fillColor =
                    NamedColor::namedColorToColor(pair.namedColorValue());
            }
            m_canvas->setFillColor(fillColor);
        }
    } else {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
}

DOMStringOrCanvasGradientOrCanvasPattern
CanvasRenderingContext2DMixIn::strokeStyle()
{
    return DOMStringOrCanvasGradientOrCanvasPattern::createDOMString(
        m_canvas->strokeColor().toHTMLColorCodeString());
}

void CanvasRenderingContext2DMixIn::setStrokeStyle(
    DOMStringOrCanvasGradientOrCanvasPattern value)
{
    if (value.isDOMStringValue()) {
        String* v = value.getDOMStringValue();
        CSSTokenValue token(v->toUTF8NonGCString());
        CSSStyleValuePair pair;
        Unit::Color strokeColor;
        if (pair.updateValueUnitColor(token)) {
            if (pair.valueKind() ==
                CSSStyleValuePair::ValueKind::ColorValueKind) {
                strokeColor = pair.colorValue();
            } else if (pair.valueKind() ==
                       CSSStyleValuePair::ValueKind::NamedColorValueKind) {
                strokeColor =
                    NamedColor::namedColorToColor(pair.namedColorValue());
            }
            m_canvas->setStrokeColor(strokeColor);
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
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return String::fromUTF8("none");
}

void CanvasRenderingContext2DMixIn::setFilter(String* value)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void CanvasRenderingContext2DMixIn::fillRect(float x, float y, float w, float h)
{
    STARFISH_ASSERT(m_canvas != nullptr);

    if (m_canvas->hasNonInvertableCTM()) {
        return;
    }

    // https://html.spec.whatwg.org/multipage/canvas.html#dom-context-2d-fillrect
    if (isInfOrNan(x) || isInfOrNan(y) || isInfOrNan(w) || isInfOrNan(h)) {
        return;
    }

    if (!w || !h) {
        return;
    }

    m_ownerHTMLCanvasElement->setNeedsComposite();
    if (m_canvas->compositeOperator() == CanvasCompositeOperator::Copy) {
        m_canvas->clearColor(Unit::Color(0, 0, 0, 0));
    }
    m_canvas->drawRect(Unit::Rect(x, y, w, h));
}

void CanvasRenderingContext2DMixIn::strokeRect(float x, float y, float w,
                                               float h)
{
    if (m_canvas->hasNonInvertableCTM()) {
        return;
    }

    // https://html.spec.whatwg.org/multipage/canvas.html#dom-context-2d-strokerect
    if (isInfOrNan(x) || isInfOrNan(y) || isInfOrNan(w) || isInfOrNan(h)) {
        return;
    }
    if (!w && !h) {
        return;
    }
    m_canvas->strokeRect(Unit::Rect(x, y, w, h));
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
    fill(path->canvasPath()->path(), fillRule);
}

void CanvasRenderingContext2DMixIn::stroke()
{
    stroke(m_canvasPath->path());
}

void CanvasRenderingContext2DMixIn::stroke(Path2D* path)
{
    stroke(path->canvasPath()->path());
}

void CanvasRenderingContext2DMixIn::clip(String* fillRule)
{
    clip(m_canvasPath->path(), fillRule);
}

void CanvasRenderingContext2DMixIn::clip(Path2D* path, String* fillRule)
{
    clip(path->canvasPath()->path(), fillRule);
}

bool CanvasRenderingContext2DMixIn::isPointInPath(float x, float y,
                                                  String* fillRule)
{
    return isPointInPath(m_canvasPath->path(), x, y, fillRule);
}

bool CanvasRenderingContext2DMixIn::isPointInPath(Path2D* path, float x,
                                                  float y, String* fillRule)
{
    return isPointInPath(path->canvasPath()->path(), x, y, fillRule);
}

bool CanvasRenderingContext2DMixIn::isPointInStroke(float x, float y)
{
    return isPointInStroke(m_canvasPath->path(), x, y);
}

bool CanvasRenderingContext2DMixIn::isPointInStroke(Path2D* path, float x,
                                                    float y)
{
    return isPointInStroke(path->canvasPath()->path(), x, y);
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
    if (m_canvas->hasNonInvertableCTM()) {
        return;
    }
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void CanvasRenderingContext2DMixIn::drawImage(ScriptValue image, float dx,
                                              float dy, float dw, float dh)
{
    if (m_canvas->hasNonInvertableCTM()) {
        return;
    }
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void CanvasRenderingContext2DMixIn::drawImage(ScriptValue image, float sx,
                                              float sy, float sw, float sh,
                                              float dx, float dy, float dw,
                                              float dh)
{
    if (m_canvas->hasNonInvertableCTM()) {
        return;
    }
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

ImageData* CanvasRenderingContext2DMixIn::createImageData(int32_t sw,
                                                          int32_t sh)
{
    if (!sw || !sh) {
        throw new DOMException(executionContext(),
                               DOMException::Code::INDEX_SIZE_ERR,
                               "sw and sh are must not zero");
    }
    return new ImageData(executionContext(), abs(sw), abs(sh));
}

ImageData* CanvasRenderingContext2DMixIn::createImageData(ImageData* imagedata)
{
    STARFISH_ASSERT(imagedata != nullptr);
    auto pixelsPerRow = imagedata->width();
    auto rows = imagedata->height();

    if (!imagedata->width() || !imagedata->height()) {
        throw new DOMException(executionContext(),
                               DOMException::Code::INDEX_SIZE_ERR,
                               "sw and sh are must not zero");
    }
    return new ImageData(executionContext(), pixelsPerRow, rows);
}

ImageData* CanvasRenderingContext2DMixIn::getImageData(int32_t sx, int32_t sy,
                                                       int32_t sw, int32_t sh)
{
    if (!sw || !sh) {
        throw new DOMException(executionContext(),
                               DOMException::Code::INDEX_SIZE_ERR,
                               "sw and sh are must not zero");
    }

    if (sw < 0) {
        sx += sw;
        sw = -sw;
    }
    if (sh < 0) {
        sy += sh;
        sh = -sh;
    }

    // FIXME: Remove below codes When createArrayBuffer handles a RangeError
    Checked<int, RecordOverflow> dataSize = 4;
    dataSize *= sw;
    dataSize *= sh;
    if (dataSize.hasOverflowed()) {
        throw new DOMException(
            executionContext(), DOMException::Code::INDEX_SIZE_ERR,
            "The requested image size exceeds the supported range.");
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
    for (int64_t y = 0; y < sh; ++y) {
        if ((y + sy) < 0 || static_cast<int64_t>(height) <= (y + sy)) {
            continue;
        }
        for (int64_t x = 0; x < sw; ++x) {
            if ((x + sx) < 0 || static_cast<int64_t>(width) <= (x + sx)) {
                continue;
            }
            uint8_t* destPixel = dest + (y * sw * stride) + (x * stride);
            uint8_t* srcPixel =
                src + ((y + sy) * width * stride) + ((x + sx) * stride);
            uint8_t r, g, b, a;
#if defined(PORT_PIXEL_ORDER_RGBA)
            r = srcPixel[0];
            g = srcPixel[1];
            b = srcPixel[2];
            a = srcPixel[3];
#else
            r = srcPixel[2];
            g = srcPixel[1];
            b = srcPixel[0];
            a = srcPixel[3];
#endif
#if defined(NEEDS_UNPREMULTIPLIED)
            if (a && a != 255) {
                r = r * 255 / a;
                g = g * 255 / a;
                b = b * 255 / a;
            }
#endif
            destPixel[0] = r;
            destPixel[1] = g;
            destPixel[2] = b;
            destPixel[3] = a;
        }
    }
    auto uint8ClampedArray = createEmptyUint8ClampedArray(
        executionContext()->scriptBindingInstance());

    uint8ClampedArray->setBuffer(
        canvasPixelArrayBuffer->toObject(state)->asArrayBufferObject(), 0,
        destSize, destSize);

    auto ret = new ImageData(executionContext(), uint8ClampedArray, sw, sh);
    state->destroy();
    return ret;
}

void CanvasRenderingContext2DMixIn::putImageData(ImageData* imagedata,
                                                 int32_t dx, int32_t dy)
{
    STARFISH_ASSERT(imagedata);
    putImageData(imagedata, dx, dy, 0, 0, imagedata->width(),
                 imagedata->height());
}

void CanvasRenderingContext2DMixIn::putImageData(ImageData* imagedata,
                                                 int32_t dx, int32_t dy,
                                                 int32_t dirtyX, int32_t dirtyY,
                                                 int32_t dirtyWidth,
                                                 int32_t dirtyHeight)
{
    // https://html.spec.whatwg.org/multipage/canvas.html#dom-context-2d-putimagedata

    STARFISH_ASSERT(imagedata);

    if (isInfOrNan(dx) || isInfOrNan(dy) || isInfOrNan(dirtyX) ||
        isInfOrNan(dirtyY) || isInfOrNan(dirtyWidth) ||
        isInfOrNan(dirtyHeight)) {
        return;
    }

    if (imagedata->data()->asArrayBufferView()->buffer()->isDetachedBuffer()) {
        throw new DOMException(executionContext(),
                               DOMException::Code::INVALID_STATE_ERR,
                               "ImageData's data has a detached buffer");
    }

    if (dirtyWidth < 0) {
        dirtyX += dirtyWidth;
        dirtyWidth = abs(dirtyWidth);
    }

    if (dirtyHeight < 0) {
        dirtyY += dirtyHeight;
        dirtyHeight = abs(dirtyHeight);
    }

    if (dirtyX < 0) {
        dirtyWidth += dirtyX;
        dirtyX = 0;
    }

    if (dirtyY < 0) {
        dirtyHeight += dirtyY;
        dirtyY = 0;
    }

    int64_t imagaDataWidth = imagedata->width();
    if (dirtyX + dirtyWidth > imagaDataWidth) {
        dirtyWidth = imagaDataWidth - dirtyX;
    }

    int64_t imagaDataHeight = imagedata->height();
    if (dirtyY + dirtyHeight > imagaDataHeight) {
        dirtyHeight = imagaDataHeight - dirtyY;
    }

    if (dirtyWidth <= 0 || dirtyHeight <= 0) {
        return;
    }

    auto src = imagedata->data()->asArrayBufferView()->buffer()->rawBuffer();

    uint8_t* dest = m_canvasSurface->mapBuffer();
    auto destWidth = m_canvasSurface->bufferWidth();
    auto destHeight = m_canvasSurface->bufferHeight();
    if (!destWidth || !destHeight) {
        return;
    }
    size_t destStride = 0;
    if (destWidth && m_canvasSurface->bufferStride()) {
        destStride =
            m_canvasSurface->bufferStride() / m_canvasSurface->bufferWidth();
    } else {
        destStride = 4;
    }

    for (int64_t y = dirtyY; y < dirtyY + dirtyHeight; ++y) {
        if ((y + dy) < 0 || static_cast<int64_t>(destHeight) <= (y + dy)) {
            continue;
        }
        for (int64_t x = dirtyX; x < dirtyX + dirtyWidth; ++x) {
            if ((x + dx) < 0 || static_cast<int64_t>(destWidth) <= (x + dx)) {
                continue;
            }
            uint8_t* destPixel = dest + ((dy + y) * destWidth * destStride) +
                                 ((dx + x) * destStride);
            uint8_t* srcPixel = src + (y * imagaDataWidth * 4) + (x * 4);
            uint8_t r, g, b, a;
            r = srcPixel[0];
            g = srcPixel[1];
            b = srcPixel[2];
            a = srcPixel[3];
#if defined(NEEDS_UNPREMULTIPLIED)
            if (a != 255) {
                r = (r * a + 254) / 255;
                g = (g * a + 254) / 255;
                b = (b * a + 254) / 255;
            }
#endif
#if defined(PORT_PIXEL_ORDER_RGBA)
            destPixel[0] = r;
            destPixel[1] = g;
            destPixel[2] = b;
            destPixel[3] = a;
#else
            destPixel[2] = r;
            destPixel[1] = g;
            destPixel[0] = b;
            destPixel[3] = a;
#endif
        }
    }
    m_canvas->markDirtyRect(Unit::Rect(dx, dy, dirtyWidth, dirtyHeight));
    m_ownerHTMLCanvasElement->setNeedsComposite();
}

void CanvasRenderingContext2DMixIn::clearRect(float x, float y, float w,
                                              float h)
{
    if (m_canvas->hasNonInvertableCTM()) {
        return;
    }

    // https://html.spec.whatwg.org/multipage/canvas.html#dom-context-2d-clearrect

    if (isInfOrNan(x) || isInfOrNan(y) || isInfOrNan(w) || isInfOrNan(h)) {
        return;
    }

    m_ownerHTMLCanvasElement->setNeedsComposite();
    m_canvas->save();
    m_canvas->clip(Unit::Rect(x, y, w, h));
    m_canvas->clearColor(Unit::Color(0, 0, 0, 0));
    m_canvas->restore();
}

void CanvasRenderingContext2DMixIn::fill(Path* path, String* fillRule)
{
    if (m_canvas->hasNonInvertableCTM()) {
        return;
    }

    m_ownerHTMLCanvasElement->setNeedsComposite();

    auto rule = stringToCanvasFillRule(fillRule);
    if (!path->isEmpty()) {
        m_canvas->save();
        if (rule == CanvasFillRule::NonZero) {
            m_canvas->setFillRule(true);
        } else if (rule == CanvasFillRule::EvenOdd) {
            m_canvas->setFillRule(false);
        }
        if (m_canvas->compositeOperator() == CanvasCompositeOperator::Copy) {
            m_canvas->clearColor(Unit::Color(0, 0, 0, 0));
        }
        m_canvas->fillPath(path);
        m_canvas->restore();
    }
}
void CanvasRenderingContext2DMixIn::stroke(Path* path)
{
    if (m_canvas->hasNonInvertableCTM()) {
        return;
    }

    m_ownerHTMLCanvasElement->setNeedsComposite();

    if (!path->isEmpty()) {
        m_canvas->save();
        if (m_canvas->compositeOperator() == CanvasCompositeOperator::Copy) {
            m_canvas->clearColor(Unit::Color(0, 0, 0, 0));
        }
        m_canvas->strokePath(path);
        m_canvas->restore();
    }
}

void CanvasRenderingContext2DMixIn::clip(Path* path, String* fillRule)
{
    if (m_canvas->hasNonInvertableCTM()) {
        return;
    }

    auto rule = stringToCanvasFillRule(fillRule);

    if (!path->isEmpty()) {
        if (rule == CanvasFillRule::NonZero) {
            m_canvas->setFillRule(true);
        } else if (rule == CanvasFillRule::EvenOdd) {
            m_canvas->setFillRule(false);
        }
        m_canvas->clipPath(path);
    }
}

void CanvasRenderingContext2DMixIn::getPointsUnaffectedByCurrentTransformation(
    const float& x, const float& y, float& ux, float& uy)
{
    SkMatrix matrix;
    m_canvas->currentTransformMatrix().invert(&matrix);

    SkPoint src;
    src.set(SkFloatToScalar(x), SkFloatToScalar(y));
    matrix.mapPoints(&src, 1);
    ux = SkScalarToFloat(src.x());
    uy = SkScalarToFloat(src.y());
}

bool CanvasRenderingContext2DMixIn::isPointInPath(Path* path, float x, float y,
                                                  String* fillRule)
{
    if (isInfOrNan(x) || isInfOrNan(y)) {
        return false;
    }

    auto rule = stringToCanvasFillRule(fillRule);
    float xx, yy;
    getPointsUnaffectedByCurrentTransformation(x, y, xx, yy);
    return path->isPointInPath(xx, yy, rule);
}

bool CanvasRenderingContext2DMixIn::isPointInStroke(Path* path, float x,
                                                    float y)
{
    if (isInfOrNan(x) || isInfOrNan(y)) {
        return false;
    }

    float xx, yy;
    getPointsUnaffectedByCurrentTransformation(x, y, xx, yy);
    path->applyPathDrawingStyles(m_canvas);
    return path->isPointInStroke(xx, yy);
}
}
#undef NEEDS_UNPREMULTIPLIED
#undef CRASH
#endif
