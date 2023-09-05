/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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
#include "binding/generated/DOMStringOrCanvasGradientOrCanvasPatternUnion.h"
#include "binding/ScriptBindingInstance.h"
#include "core/style/Style.h"
#include "core/style/ComputedStyle.h"
#include "core/style/GradientData.h"
#include "core/style/CSSGradientValue.h"
#include "core/dom/Node.h"
#include "core/dom/canvas/CanvasFillRule.h"
#include "core/dom/canvas/CanvasGradient.h"
#include "core/dom/canvas/CanvasImageSource.h"
#include "core/dom/canvas/CanvasLineCap.h"
#include "core/dom/canvas/CanvasLineJoin.h"
#include "core/dom/canvas/CanvasDirection.h"
#include "core/dom/canvas/CanvasTextAlign.h"
#include "core/dom/canvas/CanvasTextBaseline.h"
#include "core/modules/canvas/Canvas.h"
#include "core/dom/canvas/ImageSmoothingQuality.h"
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
#include "core/dom/canvas/CanvasPath.h"
#include "core/dom/canvas/CanvasRenderingContext.h"
#include "core/dom/canvas/CanvasRenderingContext2DMixIn.h"
#include "core/layout/FrameDocument.h"
#include "core/dom/Text.h"
#include "core/modules/canvas/NativeGradient.h"
#include "core/dom/canvas/TextMetrics.h"

#ifndef CRASH
#define CRASH STARFISH_CRASH
#endif
#include "../third_party/escargot/third_party/checked_arithmetic/CheckedArithmetic.h"

#if defined(PORT_CANVAS_BACKEND_CAIRO) || defined(PORT_CANVAS_BACKEND_SKIA)
#define NEEDS_UNPREMULTIPLIED
#endif

namespace Starfish {

static inline bool stringToCanvasFillRule(String* rule, CanvasFillRule& out)
{
    STARFISH_ASSERT(rule != nullptr);

    if (rule->equals("evenodd", 7) == true) {
        out = CanvasFillRule::EvenOdd;
        return true;
    } else if (rule->equals("nonzero", 7) == true) {
        out = CanvasFillRule::NonZero;
        return true;
    }

    return false;
}

static inline String* canvasLineCapToString(CanvasLineCap lineCap)
{
    if (lineCap == CanvasLineCap::Round) {
        return String::createASCIIString("round");
    } else if (lineCap == CanvasLineCap::Square) {
        return String::createASCIIString("square");
    } else if (lineCap == CanvasLineCap::Butt) {
        return String::createASCIIString("butt");
    }

    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return String::createASCIIString("butt");
}

static inline String* canvasTextAlignToString(CanvasTextAlign textAlign)
{
    if (textAlign == CanvasTextAlign::End) {
        return String::createASCIIString("end");
    } else if (textAlign == CanvasTextAlign::Left) {
        return String::createASCIIString("left");
    } else if (textAlign == CanvasTextAlign::Right) {
        return String::createASCIIString("right");
    } else if (textAlign == CanvasTextAlign::Center) {
        return String::createASCIIString("center");
    } else if (textAlign == CanvasTextAlign::Start) {
        return String::createASCIIString("start");
    }

    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return String::createASCIIString("start");
}

static inline String* canvasTextBaselineToString(
    CanvasTextBaseline textBaseline)
{
    if (textBaseline == CanvasTextBaseline::Top) {
        return String::createASCIIString("top");
    } else if (textBaseline == CanvasTextBaseline::Hanging) {
        return String::createASCIIString("hanging");
    } else if (textBaseline == CanvasTextBaseline::Middle) {
        return String::createASCIIString("middle");
    } else if (textBaseline == CanvasTextBaseline::Ideographic) {
        return String::createASCIIString("ideographic");
    } else if (textBaseline == CanvasTextBaseline::Bottom) {
        return String::createASCIIString("bottom");
    } else if (textBaseline == CanvasTextBaseline::Alphabetic) {
        return String::createASCIIString("alphabetic");
    }

    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return String::createASCIIString("alphabetic");
}

static inline String* canvasDirectionToString(CanvasDirection direction,
                                              ComputedStyle* style)
{
    STARFISH_ASSERT(style != nullptr);

    if (direction == CanvasDirection::Inherit) {
        if (style->direction() == DirectionValue::RtlDirectionValue) {
            return String::createASCIIString("rtl");
        }
    } else if (direction == CanvasDirection::Rtl) {
        return String::createASCIIString("rtl");
    } else if (direction == CanvasDirection::Ltr) {
        return String::createASCIIString("ltr");
    }

    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return String::createASCIIString("ltr");
}

static inline bool stringToCanvasTextAlign(String* textAlign,
                                           CanvasTextAlign& out)
{
    STARFISH_ASSERT(textAlign != nullptr);

    if (textAlign->equals("start", 5) == true) {
        out = CanvasTextAlign::Start;
        return true;
    } else if (textAlign->equals("end", 3) == true) {
        out = CanvasTextAlign::End;
        return true;
    } else if (textAlign->equals("left", 4) == true) {
        out = CanvasTextAlign::Left;
        return true;
    } else if (textAlign->equals("right", 5) == true) {
        out = CanvasTextAlign::Right;
        return true;
    } else if (textAlign->equals("center", 6) == true) {
        out = CanvasTextAlign::Center;
        return true;
    }
    return false;
}

static inline bool stringToCanvasTextBaseline(String* textBaseline,
                                              CanvasTextBaseline& out)
{
    STARFISH_ASSERT(textBaseline != nullptr);

    if (textBaseline->equals("top", 3) == true) {
        out = CanvasTextBaseline::Top;
        return true;
    } else if (textBaseline->equals("hanging", 7) == true) {
        out = CanvasTextBaseline::Hanging;
        return true;
    } else if (textBaseline->equals("middle", 6) == true) {
        out = CanvasTextBaseline::Middle;
        return true;
    } else if (textBaseline->equals("alphabetic", 10) == true) {
        out = CanvasTextBaseline::Alphabetic;
        return true;
    } else if (textBaseline->equals("ideographic", 11) == true) {
        out = CanvasTextBaseline::Ideographic;
        return true;
    } else if (textBaseline->equals("bottom", 6) == true) {
        out = CanvasTextBaseline::Bottom;
        return true;
    }

    return false;
}

static inline bool stringToCanvasDirection(String* canvasDirection,
                                           CanvasDirection& out)
{
    STARFISH_ASSERT(canvasDirection != nullptr);

    if (canvasDirection->equals("ltr", 3) == true) {
        out = CanvasDirection::Ltr;
        return true;
    } else if (canvasDirection->equals("rtl", 3) == true) {
        out = CanvasDirection::Rtl;
        return true;
    } else if (canvasDirection->equals("inherit", 7) == true) {
        out = CanvasDirection::Inherit;
        return true;
    }

    return false;
}

static inline bool stringToCanvasLineCap(String* lineCap, CanvasLineCap& out)
{
    STARFISH_ASSERT(lineCap != nullptr);

    if (lineCap->equals("round", 5) == true) {
        out = CanvasLineCap::Round;
        return true;
    } else if (lineCap->equals("square", 6) == true) {
        out = CanvasLineCap::Square;
        return true;
    } else if (lineCap->equals("butt", 4) == true) {
        out = CanvasLineCap::Butt;
        return true;
    }

    return false;
}

static inline String* canvasLineJoinToString(CanvasLineJoin lineJoin)
{
    if (lineJoin == CanvasLineJoin::Round) {
        return String::createASCIIString("round");
    } else if (lineJoin == CanvasLineJoin::Bevel) {
        return String::createASCIIString("bevel");
    } else if (lineJoin == CanvasLineJoin::Miter) {
        return String::createASCIIString("miter");
    }

    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return String::createASCIIString("miter");
}

static inline bool stringToCanvasLineJoin(String* lineJoin, CanvasLineJoin& out)
{
    STARFISH_ASSERT(lineJoin != nullptr);

    if (lineJoin->equals("round", 5) == true) {
        out = CanvasLineJoin::Round;
        return true;
    } else if (lineJoin->equals("bevel", 5) == true) {
        out = CanvasLineJoin::Bevel;
        return true;
    } else if (lineJoin->equals("miter", 5) == true) {
        out = CanvasLineJoin::Miter;
        return true;
    }

    return false;
}

static inline String* imageSmoothingQualityToString(
    ImageSmoothingQuality quality)
{
    if (quality == ImageSmoothingQuality::Low) {
        return String::createASCIIString("low");
    } else if (quality == ImageSmoothingQuality::Medium) {
        return String::createASCIIString("medium");
    } else if (quality == ImageSmoothingQuality::High) {
        return String::createASCIIString("high");
    }

    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return String::createASCIIString("low");
}

static inline bool stringToImageSmoothingQuality(String* quality,
                                                 ImageSmoothingQuality& out)
{
    STARFISH_ASSERT(quality != nullptr);

    if (quality->equals("low", 3) == true) {
        out = ImageSmoothingQuality::Low;
        return true;
    } else if (quality->equals("medium", 6) == true) {
        out = ImageSmoothingQuality::Medium;
        return true;
    } else if (quality->equals("high", 4) == true) {
        out = ImageSmoothingQuality::High;
        return true;
    }
    return false;
}

static inline bool stringToColor(String* color, Unit::Color& out)
{
    STARFISH_ASSERT(color != nullptr);

    CSSTokenValue token(color->toUTF8NonGCString());
    CSSStyleValuePair pair;
    if (pair.updateValueUnitColor(token) == true) {
        if (pair.valueKind() == CSSStyleValuePair::ValueKind::ColorValueKind) {
            out = pair.colorValue();
            return true;
        } else if (pair.valueKind() ==
                   CSSStyleValuePair::ValueKind::NamedColorValueKind) {
            out = NamedColor::namedColorToColor(pair.namedColorValue());
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
{
    initialize();
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
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

    uint32_t width, height;
    calculateDimension(width, height, m_ownerHTMLCanvasElement->width(),
                       m_ownerHTMLCanvasElement->height());

    m_canvasSurface = CanvasSurface::create(
        m_ownerHTMLCanvasElement->webView()->platformWindow(), width, height, 1,
        CanvasSurface::CanvasElement);
    m_canvas =
        Canvas::create(m_ownerHTMLCanvasElement->webView(), m_canvasSurface,
                       Canvas::CanvasFlag::CanvasElement);
    m_canvas->unsetDevicePixelRatio();
    m_canvas->clearColor(Unit::Color(0, 0, 0, 0));
    m_canvasPath = new CanvasPath(executionContext());
    auto black = Unit::Color(0, 0, 0, 255);

    setLineWidth(1.0f);                                   // default 1.0
    setLineCap(CanvasLineCap::Butt);                      // default "butt"
    setLineJoin(CanvasLineJoin::Miter);                   // default "miter"
    setMiterLimit(10.0f);                                 // default 10
    setLineDash(GCAtomicVector<double>());                // default empty
    setLineDashOffset(0.0f);                              // default 0.0
    setImageSmoothingEnabled(true);                       // defauilt true
    setImageSmoothingQuality(ImageSmoothingQuality::Low); // default low
    setFont(String::fromUTF8("10px sans-serif"));         // default font
    setShadowOffsetX(0);
    setShadowOffsetY(0);
    setShadowBlur(0);
    setShadowColor(String::fromUTF8("transparent black"));

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
    if (stringToCanvasLineCap(value, cap) == true) {
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
    if (stringToCanvasLineJoin(value, join) == true) {
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
    if (limit <= 0 || isInfOrNan(limit) == true) {
        return;
    }
    m_canvas->setMiterLimit(limit);
}

void CanvasRenderingContext2DMixIn::setLineDash(GCAtomicVector<double> segments)
{
    for (auto& segment : segments) {
        if (isInfOrNan(segment) == true || segment < 0) {
            return;
        }
    }

    std::vector<double> dashes;
    for (auto& segment : segments) {
        dashes.emplace_back(segment);
    }

    if (segments.size() % 2 != 0) {
        for (auto& segment : segments) {
            dashes.emplace_back(segment);
        }
    }
    m_canvas->setDash(dashes);
}

GCAtomicVector<double> CanvasRenderingContext2DMixIn::getLineDash()
{
    GCAtomicVector<double> dashes;
    for (auto& segment : m_canvas->dash()) {
        dashes.emplace_back(segment);
    }
    return dashes;
}

double CanvasRenderingContext2DMixIn::lineDashOffset()
{
    return m_canvas->dashOffset();
}

void CanvasRenderingContext2DMixIn::setLineDashOffset(double offset)
{
    if (isInfOrNan(offset) == true) {
        return;
    }
    m_canvas->setDashOffset(offset);
}

void CanvasRenderingContext2DMixIn::save()
{
    // https://html.spec.whatwg.org/multipage/canvas.html#the-canvas-state
    // Drawing states consist of:
    // The current transformation matrix.
    // The current clipping region.
    // The current values of the following attributes: strokeStyle, fillStyle,
    // globalAlpha, lineWidth, lineCap, lineJoin, miterLimit, lineDashOffset,
    // shadowOffsetX, shadowOffsetY, shadowBlur, shadowColor, filter,
    // globalCompositeOperation, font, textAlign, textBaseline, direction,
    // imageSmoothingEnabled, imageSmoothingQuality.
    // The current dash list.
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
    if (m_canvas->hasNonInvertableCTM() == true) {
        return;
    }
    transform(x, 0, 0, y, 0, 0, false);
}

void CanvasRenderingContext2DMixIn::rotate(float angle)
{
    if (m_canvas->hasNonInvertableCTM() == true) {
        return;
    }
    float cosValue = cosf(angle);
    float sinValue = sinf(angle);
    transform(cosValue, sinValue, -sinValue, cosValue, 0, 0, false);
}

void CanvasRenderingContext2DMixIn::translate(float x, float y)
{
    if (m_canvas->hasNonInvertableCTM() == true) {
        return;
    }
    transform(1, 0, 0, 1, x, y, false);
}

DOMMatrix* CanvasRenderingContext2DMixIn::getTransform()
{
    DOMMatrix* result = new DOMMatrix(executionContext());
    SkMatrix ctm = m_canvas->currentTransformMatrix();
    result->setA(ctm.get(0));
    result->setB(ctm.get(3));
    result->setC(ctm.get(1));
    result->setD(ctm.get(4));
    result->setE(ctm.get(2));
    result->setF(ctm.get(5));
    return result;
}

void CanvasRenderingContext2DMixIn::transform(float a, float b, float c,
                                              float d, float e, float f,
                                              bool needResetMatrix)
{
    if (isInfOrNan(a) == true || isInfOrNan(b) == true ||
        isInfOrNan(c) == true || isInfOrNan(d) == true ||
        isInfOrNan(e) == true || isInfOrNan(f) == true) {
        return;
    }

    if (needResetMatrix == true) {
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
    if (matrix.invert(&invertMatrix) == true) {
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

void CanvasRenderingContext2DMixIn::setTransform()
{
    transform(1, 0, 0, 1, 0, 0, true);
}

void CanvasRenderingContext2DMixIn::setTransform(DOMMatrix2DInit matrix)
{
    transform(matrix.a(), matrix.b(), matrix.c(), matrix.d(), matrix.e(),
              matrix.f(), true);
}

void CanvasRenderingContext2DMixIn::resetTransform()
{
    m_canvas->setNonInvertableCTM(false);
    m_canvas->resetMatrix(false);
    m_canvasPath->setShouldDisable(false);
    m_canvasPath->path()->resetCTM();
}

float CanvasRenderingContext2DMixIn::globalAlpha()
{
    return m_canvas->globalAlpha();
}

void CanvasRenderingContext2DMixIn::setGlobalAlpha(float value)
{
    if (isInfOrNan(value) == true || value < .0f || value > 1.f) {
        return;
    }
    m_canvas->setGlobalAlpha(value);
}

String* CanvasRenderingContext2DMixIn::globalCompositeOperation()
{
    STARFISH_ASSERT(m_canvas != nullptr);

    if (m_canvas->blendMode() != CanvasBlendMode::Normal) {
        const char* p =
            CanvasCompositing::canvasBlendModeNames[static_cast<unsigned>(
                m_canvas->blendMode())];
        STARFISH_ASSERT(p != nullptr);
        return String::fromUTF8(p, strlen(p));
    }
    const char* p =
        CanvasCompositing::canvasCompositeOperatorNames[static_cast<unsigned>(
            m_canvas->compositeOperator())];
    STARFISH_ASSERT(p != nullptr);
    return String::fromUTF8(p, strlen(p));
}

void CanvasRenderingContext2DMixIn::setGlobalCompositeOperation(String* value)
{
    CanvasCompositeOperator cco = CanvasCompositeOperator::SourceOver;
    CanvasBlendMode cbm = CanvasBlendMode::Normal;

    for (int i = 0; i < CanvasCompositing::sizeOfCanvasCompositeOperatorNames;
         ++i) {
        if (value->equals(
                CanvasCompositing::canvasCompositeOperatorNames[i],
                strlen(CanvasCompositing::canvasCompositeOperatorNames[i])) ==
            true) {
            cco = static_cast<CanvasCompositeOperator>(i);
            m_canvas->setCompositeOperator(cco, cbm);
            return;
        }
    }
    for (int i = 0; i < CanvasCompositing::sizeOfCanvasBlendModeNames; ++i) {
        if (value->equals(CanvasCompositing::canvasBlendModeNames[i],
                          strlen(CanvasCompositing::canvasBlendModeNames[i])) ==
            true) {
            cbm = static_cast<CanvasBlendMode>(i);
            cco = CanvasCompositeOperator::SourceOver;
            m_canvas->setCompositeOperator(cco, cbm);
            return;
        }
    }
}

bool CanvasRenderingContext2DMixIn::imageSmoothingEnabled()
{
    return m_canvas->imageSmoothingEnabled();
}

void CanvasRenderingContext2DMixIn::setImageSmoothingEnabled(bool value)
{
    m_canvas->setImageSmoothingEnabled(value);
}

String* CanvasRenderingContext2DMixIn::imageSmoothingQuality()
{
    return imageSmoothingQualityToString(m_canvas->imageSmoothingQuality());
}

void CanvasRenderingContext2DMixIn::setImageSmoothingQuality(String* value)
{
    STARFISH_ASSERT(value != nullptr);

    ImageSmoothingQuality quality;
    if (stringToImageSmoothingQuality(value, quality) == true) {
        setImageSmoothingQuality(quality);
    }
}

void CanvasRenderingContext2DMixIn::setImageSmoothingQuality(
    ImageSmoothingQuality quality)
{
    m_canvas->setImageSmoothingQuality(quality);
}

CanvasStyle CanvasRenderingContext2DMixIn::fillStyle()
{
    auto source = m_canvas->fillSource();
    if (source->isColorType() == true) {
        return CanvasStyle::createDOMString(
            source->getColorValue().toHTMLColorCodeString());
    } else {
        STARFISH_ASSERT(source->isCanvasStyleType() == true);
        return source->getCanvasStyleValue();
    }
}

void CanvasRenderingContext2DMixIn::setFillStyle(CanvasStyle value)
{
    if (value.isDOMStringValue() == true) {
        Unit::Color color;
        if (stringToColor(value.getDOMStringValue(), color) == true) {
            m_canvas->setFillColor(color);
        }
    } else if (value.isNoneValue() == false) {
        m_canvas->setFillSource(new CanvasFillStrokeSource(value));
    }
}

CanvasStyle CanvasRenderingContext2DMixIn::strokeStyle()
{
    auto source = m_canvas->strokeSource();
    if (source->isColorType() == true) {
        return CanvasStyle::createDOMString(
            source->getColorValue().toHTMLColorCodeString());
    } else {
        STARFISH_ASSERT(source->isCanvasStyleType() == true);
        return source->getCanvasStyleValue();
    }
}

void CanvasRenderingContext2DMixIn::setStrokeStyle(CanvasStyle value)
{
    if (value.isDOMStringValue() == true) {
        Unit::Color color;
        if (stringToColor(value.getDOMStringValue(), color) == true) {
            m_canvas->setStrokeColor(color);
        }
    } else if (value.isNoneValue() == false) {
        m_canvas->setStrokeSource(new CanvasFillStrokeSource(value));
    }
}

CanvasGradient* CanvasRenderingContext2DMixIn::createLinearGradient(float x0,
                                                                    float y0,
                                                                    float x1,
                                                                    float y1)
{
    return new CanvasGradient(executionContext(), x0, y0, x1, y1);
}

CanvasGradient* CanvasRenderingContext2DMixIn::createRadialGradient(
    double x0, double y0, double r0, double x1, double y1, double r1)
{
    if (r0 < 0 || r1 < 0) {
        throw new DOMException(executionContext(),
                               DOMException::Code::INDEX_SIZE_ERR,
                               "r0 or r1 are negative");
    }

    return new CanvasGradient(executionContext(), x0, y0, r0, x1, y1, r1);
}

CanvasPattern* CanvasRenderingContext2DMixIn::createPattern(
    CanvasImageSource image, String* repetition)
{
    STARFISH_ASSERT(repetition != nullptr);

    auto usability =
        CanvasImageSourceUtils::checkUsability(m_executionContext, image);
    if (usability.isDOMException() == true) {
        throw usability.asDOMException();
    } else {
        if (usability.asOtherType() == false) {
            return nullptr;
        }
    }

    if (repetition->isEmpty()) {
        repetition = String::createASCIIString("repeat");
    }

    bool repeatX = false;
    bool repeatY = false;
    if (repetition->equals("repeat", 6) == true) {
        repeatX = true;
        repeatY = true;
    } else if (repetition->equals("repeat-x", 8) == true) {
        repeatX = true;
    } else if (repetition->equals("repeat-y", 8) == true) {
        repeatY = true;
    } else if (repetition->equals("no-repeat", 9) == false) {
        throw new DOMException(executionContext(),
                               DOMException::Code::SYNTAX_ERR,
                               "The repetition is not one of 'repeat', "
                               "'no-repeat', 'repeat-x', or 'repeat-y'.");
    }

    auto pair =
        CanvasImageSourceUtils::toNativeImageData(m_executionContext, image);

    NativeImageData* nativeImageData = pair.first;
    if (nativeImageData == nullptr &&
        image.isHTMLCanvasElementValue() == false) {
        // FIXME : nativeImageData can be nullptr after call
        // NativeImageData::attach on a canvas other than CanvasCairo currently
        return nullptr;
    }

    auto ret = new CanvasPattern(executionContext(), nativeImageData, repeatX,
                                 repeatY);

    if (pair.second == false) {
        ret->setOriginCleanFlag(false);
    }

    return ret;
}

double CanvasRenderingContext2DMixIn::shadowOffsetX()
{
    return m_canvas->shadowOffsetX();
}

void CanvasRenderingContext2DMixIn::setShadowOffsetX(double offset)
{
    if (isInfOrNan(offset)) {
        return;
    }
    m_canvas->setShadowOffsetX(offset);
}

double CanvasRenderingContext2DMixIn::shadowOffsetY()
{
    return m_canvas->shadowOffsetY();
}

void CanvasRenderingContext2DMixIn::setShadowOffsetY(double offset)
{
    if (isInfOrNan(offset)) {
        return;
    }
    m_canvas->setShadowOffsetY(offset);
}

double CanvasRenderingContext2DMixIn::shadowBlur()
{
    return m_canvas->shadowBlur();
}

void CanvasRenderingContext2DMixIn::setShadowBlur(double blur)
{
    if (isInfOrNan(blur) || blur < 0) {
        return;
    }
    m_canvas->setShadowBlur(blur);
}

String* CanvasRenderingContext2DMixIn::shadowColor()
{
    // https://html.spec.whatwg.org/multipage/canvas.html#serialisation-of-a-color
    Unit::Color sColor = m_canvas->shadowColor();
    if (sColor.hasAlpha()) {
        return sColor.toString();
    }
    return sColor.toHTMLColorCodeString();
}

void CanvasRenderingContext2DMixIn::setShadowColor(String* colorStr)
{
    Unit::Color color;
    if (stringToColor(colorStr, color) == true) {
        m_canvas->setShadowColor(color);
    }
}

String* CanvasRenderingContext2DMixIn::filter()
{
    STARFISH_UNIMPLEMENTED();
    return String::fromUTF8("none");
}

void CanvasRenderingContext2DMixIn::setFilter(String* value)
{
    STARFISH_UNIMPLEMENTED();
}

void CanvasRenderingContext2DMixIn::fillRect(float x, float y, float w, float h)
{
    STARFISH_ASSERT(m_canvas != nullptr);

    if (m_canvas->hasNonInvertableCTM() == true) {
        return;
    }

    // https://html.spec.whatwg.org/multipage/canvas.html#dom-context-2d-fillrect
    if (isInfOrNan(x) == true || isInfOrNan(y) == true ||
        isInfOrNan(w) == true || isInfOrNan(h) == true) {
        return;
    }

    if (w == 0 || h == 0) {
        return;
    }

    auto fs = fillStyle();
    if (fs.isCanvasGradientValue() == true &&
        fs.getCanvasGradientValue()->isZeroSize() == true) {
        return;
    }

    if (fs.isCanvasPatternValue() == true &&
        fs.getCanvasPatternValue()->isEmptyPattern() == true) {
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
    if (m_canvas->hasNonInvertableCTM() == true) {
        return;
    }

    // https://html.spec.whatwg.org/multipage/canvas.html#dom-context-2d-strokerect
    if (isInfOrNan(x) == true || isInfOrNan(y) == true ||
        isInfOrNan(w) == true || isInfOrNan(h) == true) {
        return;
    }

    if (w == 0 && h == 0) {
        return;
    }

    auto fs = strokeStyle();
    if (fs.isCanvasGradientValue() == true &&
        fs.getCanvasGradientValue()->isZeroSize() == true) {
        return;
    }

    if (fs.isCanvasPatternValue() == true &&
        fs.getCanvasPatternValue()->isEmptyPattern() == true) {
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

void CanvasRenderingContext2DMixIn::fillText(String* text, float x, float y)
{
    STARFISH_ASSERT(text != nullptr);
    fillText(text, x, y, 0, false);
}

void CanvasRenderingContext2DMixIn::fillText(String* text, float x, float y,
                                             float maxWidth)
{
    STARFISH_ASSERT(text != nullptr);
    if (maxWidth <= 0 || isInfOrNan(maxWidth) == true) {
        return;
    }
    fillText(text, x, y, maxWidth, true);
}

void CanvasRenderingContext2DMixIn::strokeText(String* text, float x, float y)
{
    STARFISH_ASSERT(text != nullptr);
    strokeText(text, x, y, 0, false);
}

void CanvasRenderingContext2DMixIn::strokeText(String* text, float x, float y,
                                               float maxWidth)
{
    STARFISH_ASSERT(text != nullptr);
    if (maxWidth <= 0 || isInfOrNan(maxWidth) == true) {
        return;
    }
    strokeText(text, x, y, maxWidth, true);
}

void CanvasRenderingContext2DMixIn::fillTextFastPath(LayoutUnit x, LayoutUnit y,
                                                     LayoutUnit textLength,
                                                     StringView text)
{
    m_canvas->save();
    m_canvas->translate(0, -(float)m_canvas->font()->metrics().m_ascender);
    m_canvas->drawText(x, y, textLength, text, false);
    m_canvas->restore();
}

void CanvasRenderingContext2DMixIn::strokeTextFastPath(LayoutUnit x,
                                                       LayoutUnit y,
                                                       LayoutUnit textLength,
                                                       StringView text)
{
    m_canvas->save();
    m_canvas->translate(0, -(float)m_canvas->font()->metrics().m_ascender);
    m_canvas->drawStrokeText(x, y, textLength, text, false);
    m_canvas->restore();
}

bool CanvasRenderingContext2DMixIn::isLtrDirection()
{
    CanvasDirection canvasDirection = m_canvas->canvasTextDirection();
    if (canvasDirection == CanvasDirection::Ltr) {
        return true;
    }
    if (canvasDirection == CanvasDirection::Inherit) {
        if (m_ownerHTMLCanvasElement->style() != nullptr) {
            if (m_ownerHTMLCanvasElement->style()->direction() ==
                DirectionValue::LtrDirectionValue) {
                return true;
            }
        } else {
            if (executionContext()->document()->style()->direction() ==
                DirectionValue::LtrDirectionValue) {
                return true;
            }
        }
    }
    return false;
}

bool CanvasRenderingContext2DMixIn::canUseFastPathText(String* text,
                                                       bool shouldApplyMaxWidth)
{
    STARFISH_ASSERT(text != nullptr);
    // check text direction
    UBiDiDirection dir;
    CanvasTextAlign canvasTextAlign = m_canvas->canvasTextAlign();
    CanvasTextBaseline canvasTextBaseline = m_canvas->canvasTextBaseline();

    StringView(text, 0, text->length())
        .peekUTF16Buffer(
            [](const char16_t* buf, size_t len, void* data) -> size_t {
                STARFISH_ASSERT(buf != nullptr);
                STARFISH_ASSERT(data != nullptr);

                *((UBiDiDirection*)data) =
                    ubidi_getBaseDirection((const UChar*)buf, len);
                return false;
            },
            &dir);
    if (dir == UBIDI_LTR && !shouldApplyMaxWidth &&
        canvasTextBaseline == CanvasTextBaseline::Alphabetic) {
        if (isLtrDirection() == true) {
            if (canvasTextAlign == CanvasTextAlign::Left ||
                canvasTextAlign == CanvasTextAlign::Start) {
                return true;
            }
        }
    }
    return false;
}

void CanvasRenderingContext2DMixIn::drawTextNormal(String* text, float x,
                                                   float y, float maxWidth,
                                                   bool shouldApplyMaxWidth,
                                                   bool isStroke)
{
    STARFISH_ASSERT(text != nullptr);

    Font* font = m_canvas->font();

    ComputedStyle style =
        ComputedStyle(executionContext()->document()->style());
    style.setFont(font);
    style.setDisplay(DisplayValue::BlockDisplayValue);
    style.setWidth(Length(Length::Fixed, 0));
    style.setWhiteSpace(WhiteSpaceValue::NoWrapWhiteSpaceValue);

    // CanvasTextDrawingStyles.direction
    CanvasDirection canvasDirection = m_canvas->canvasTextDirection();
    if (canvasDirection == CanvasDirection::Ltr) {
        style.setDirection(DirectionValue::LtrDirectionValue);
    } else if (canvasDirection == CanvasDirection::Rtl) {
        style.setDirection(DirectionValue::RtlDirectionValue);
    } else {
        if (m_ownerHTMLCanvasElement->style() != nullptr) {
            style.setDirection(m_ownerHTMLCanvasElement->style()->direction());
        } else {
            style.setDirection(
                executionContext()->document()->style()->direction());
        }
    }

    FrameDocument dummyFrameDocument =
        FrameDocument(executionContext()->document());
    FrameBlockBox dummyFrameBlockContainer = FrameBlockBox(nullptr, &style);
    dummyFrameDocument.appendChild(&dummyFrameBlockContainer);
    Text textNode = Text(executionContext()->document(), text);

    ComputedStyle textStyle = ComputedStyle(&style);
    textStyle.setFont(font);

    FrameText frameText = FrameText(&textNode, &textStyle);
    textNode.setFrame(&frameText);
    textNode.setStyle(&textStyle);
    dummyFrameBlockContainer.appendChild(&frameText);

    // layout
    LayoutContext layoutCtx(executionContext()->starfish(),
                            &dummyFrameDocument);
    dummyFrameBlockContainer.layout(layoutCtx,
                                    Frame::LayoutWantToResolve::ResolveAll);

    // Paint
    PaintingContext paintCtx(m_canvas);
    paintCtx.m_paintingStage = PaintingNormalFlowInline;
    dummyFrameBlockContainer
        .establishesStackingContextIfNeedsAndComputingPaintingFlags();
    paintCtx.m_canvas->save();

    // CanvasTextDrawingStyles.textBaseline
    paintCtx.m_canvas->translate(x, y);

    CanvasTextBaseline canvasTextBaseline = m_canvas->canvasTextBaseline();
    if (canvasTextBaseline == CanvasTextBaseline::Top) {
    } else if (canvasTextBaseline == CanvasTextBaseline::Hanging) {
        // http: // wiki.apache.org/xmlgraphics-fop/LineLayout/AlignmentHandling
        paintCtx.m_canvas->translate(0,
                                     -(float)font->metrics().m_ascender * 0.2);
    } else if (canvasTextBaseline == CanvasTextBaseline::Middle) {
        paintCtx.m_canvas->translate(0,
                                     -(float)font->metrics().m_fontHeight / 2);
    } else if (canvasTextBaseline == CanvasTextBaseline::Ideographic) {
        paintCtx.m_canvas->translate(0, -(float)font->metrics().m_ascender +
                                            (float)font->metrics().m_descender);
    } else if (canvasTextBaseline == CanvasTextBaseline::Bottom) {
        paintCtx.m_canvas->translate(0, -(float)font->metrics().m_fontHeight);
    } else {
        paintCtx.m_canvas->translate(0, -(float)font->metrics().m_ascender);
    }

    float width = 0;
    dummyFrameBlockContainer.iterateChildFrameBox([&width](FrameBox* fb) {
        STARFISH_ASSERT(fb != nullptr);
        if (fb->isInlineTextBox()) {
            width += fb->contentWidth().toFloat();
        }
    });

    CanvasTextAlign canvasTextAlign = m_canvas->canvasTextAlign();
    // CanvasTextDrawingStyles.textAlign
    if (canvasTextAlign == CanvasTextAlign::End) {
        if (isLtrDirection() == true) {
            paintCtx.m_canvas->translate(-width, 0);
        } else {
            paintCtx.m_canvas->translate(width, 0);
        }
    } else if (canvasTextAlign == CanvasTextAlign::Left) {
        if (isLtrDirection() == false) {
            paintCtx.m_canvas->translate(width, 0);
        }
    } else if (canvasTextAlign == CanvasTextAlign::Right) {
        if (isLtrDirection() == true) {
            paintCtx.m_canvas->translate(-width, 0);
        }
    } else if (canvasTextAlign == CanvasTextAlign::Center) {
        if (isLtrDirection() == true) {
            paintCtx.m_canvas->translate(-(width / 2), 0);
        } else {
            paintCtx.m_canvas->translate((width / 2), 0);
        }
    } else {
    }

    if (shouldApplyMaxWidth == true) {
        float scale = maxWidth / width;
        if (isInfOrNan(scale) == false) {
            paintCtx.m_canvas->scale(scale, 1);
        }
    }

    GCVector<LineBox*> lbs = dummyFrameBlockContainer.lineBoxes();
    for (size_t i = 0; i < lbs.size(); ++i) {
        InlineBoxLayoutParentBox& ilp = *lbs[i];
        GCVector<FrameBox*> fbs = ilp.boxes();
        for (size_t k = 0; k < fbs.size(); ++k) {
            InlineTextBox* childBox = (InlineTextBox*)fbs[k];
            LayoutUnit dx = ilp.frameRect().x() + childBox->x();
            LayoutUnit dy = ilp.frameRect().y() + childBox->y();
            LayoutRect vr = childBox->frameVisibleRect();
            vr.setX(vr.x() + dx);
            vr.setY(vr.y() + dy);
            if (paintCtx.m_canvas->canRejectPainting(vr)) {
                continue;
            }

            StringView txt = childBox->text();
            if (isStroke == true) {
                paintCtx.m_canvas->drawStrokeText(
                    dx, dy, childBox->contentWidth(), txt, false);
            } else {
                paintCtx.m_canvas->drawText(dx, dy, childBox->contentWidth(),
                                            txt, false);
            }
        }
    }
    paintCtx.m_canvas->restore();
}

void CanvasRenderingContext2DMixIn::updateFontIfNeeds()
{
    if (executionContext()->document()->canvasWebFontState() !=
        m_canvas->canvasWebFontState()) {
        // font resolve
        setFont(m_canvas->originalFontStr());
        m_canvas->setCanvasWebFontState(
            executionContext()->document()->canvasWebFontState());
    }
}

void CanvasRenderingContext2DMixIn::fillText(String* text, float x, float y,
                                             float maxWidth,
                                             bool isMaxWidthProvided)
{
    STARFISH_ASSERT(text != nullptr);

    if (isInfOrNan(x) == true || isInfOrNan(y) == true) {
        return;
    }

    updateFontIfNeeds();
    bool useMaxWidth = false;
    LayoutUnit textLength = m_canvas->font()->measureText(StringView(text));
    if (isMaxWidthProvided == true && textLength > maxWidth) {
        useMaxWidth = true;
    }

    if (canUseFastPathText(text, useMaxWidth) == true) {
        fillTextFastPath(LayoutUnit(x), LayoutUnit(y), textLength,
                         StringView(text));
        return;
    }

    drawTextNormal(text, x, y, maxWidth, useMaxWidth, false);
}

void CanvasRenderingContext2DMixIn::strokeText(String* text, float x, float y,
                                               float maxWidth,
                                               bool isMaxWidthProvided)
{
    STARFISH_ASSERT(text != nullptr);
    if (isInfOrNan(x) == true || isInfOrNan(y) == true) {
        return;
    }

    updateFontIfNeeds();
    bool useMaxWidth = false;
    LayoutUnit textLength = m_canvas->font()->measureText(StringView(text));
    if (isMaxWidthProvided == true && textLength > maxWidth) {
        useMaxWidth = true;
    }

    if (canUseFastPathText(text, useMaxWidth) == true) {
        strokeTextFastPath(LayoutUnit(x), LayoutUnit(y), textLength,
                           StringView(text));
        return;
    }

    drawTextNormal(text, x, y, maxWidth, useMaxWidth, true);
}

TextMetrics* CanvasRenderingContext2DMixIn::measureText(String* text)
{
    STARFISH_ASSERT(text != nullptr);
    updateFontIfNeeds();
    return new TextMetrics(executionContext(),
                           m_canvas->font()->measureText(StringView(text)), 0,
                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

void CanvasRenderingContext2DMixIn::drawImage(CanvasImageSource image, float dx,
                                              float dy)
{
    drawImage(image, dx, dy, 0, 0);
}

void CanvasRenderingContext2DMixIn::drawImage(CanvasImageSource image, float dx,
                                              float dy, float dw, float dh)
{
    drawImage(image, 0, 0, 0, 0, dx, dy, dw, dh);
}

static inline Unit::Rect normalizeRect(const Unit::Rect& rect)
{
    return Unit::Rect(std::min(rect.x(), rect.maxX()),
                      std::min(rect.y(), rect.maxY()),
                      std::max(rect.width(), -rect.width()),
                      std::max(rect.height(), -rect.height()));
}

static inline void clipRectsToImageRect(const Unit::Rect& img, Unit::Rect& src,
                                        Unit::Rect& dst)
{
    if (img.contains(src) == true) {
        return;
    }

    Unit::Size scale(dst.width() / src.width(), dst.height() / src.height());
    Unit::Location scaledSrcLocation(src.x(), src.y());
    scaledSrcLocation.scale(scale.width(), scale.height());
    Unit::Size offset(dst.x() - scaledSrcLocation.x(),
                      dst.y() - scaledSrcLocation.y());

    src.intersect(img);

    dst = src;
    dst.scale(scale.width(), scale.height());
    dst.setX(dst.x() + offset.width());
    dst.setY(dst.y() + offset.height());
}

void CanvasRenderingContext2DMixIn::drawImage(CanvasImageSource image, float sx,
                                              float sy, float sw, float sh,
                                              float dx, float dy, float dw,
                                              float dh)
{
    // https://html.spec.whatwg.org/multipage/canvas.html#dom-context-2d-drawimage
    if (m_canvas->hasNonInvertableCTM() == true) {
        return;
    }

    if (isInfOrNan(sx) == true || isInfOrNan(sy) == true ||
        isInfOrNan(sw) == true || isInfOrNan(sh) == true ||
        isInfOrNan(dx) == true || isInfOrNan(dy) == true ||
        isInfOrNan(dw) == true || isInfOrNan(dh) == true) {
        return;
    }

    auto usability =
        CanvasImageSourceUtils::checkUsability(m_executionContext, image);
    if (usability.isDOMException() == true) {
        throw usability.asDOMException();
    } else {
        if (!usability.asOtherType()) {
            return;
        }
    }

    auto pair =
        CanvasImageSourceUtils::toNativeImageData(m_executionContext, image);

    NativeImageData* nativeImageData = pair.first;
    if (nativeImageData == nullptr) {
        // FIXME : nativeImageData can be nullptr after call
        // NativeImageData::attach on a canvas other than CanvasCairo currently
        return;
    }

    if (pair.second == false) {
        setOriginCleanFlag(false);
    }

    if (sw == 0) {
        sw = nativeImageData->width();
    }

    if (sh == 0) {
        sh = nativeImageData->height();
    }

    if (sw == 0 || sh == 0) {
        return;
    }
    if (dw == 0) {
        dw = sw;
    }
    if (dh == 0) {
        dh = sh;
    }

    Unit::Rect src = normalizeRect(Unit::Rect(sx, sy, sw, sh));
    Unit::Rect dst = normalizeRect(Unit::Rect(dx, dy, dw, dh));
    const Unit::Rect imageSize =
        Unit::Rect(0, 0, nativeImageData->width(), nativeImageData->height());

    clipRectsToImageRect(imageSize, src, dst);

    Unit::Rect adjustSrcRect(0, 0, nativeImageData->width(),
                             nativeImageData->height());
    if (adjustSrcRect.contains(src) == true) {
        adjustSrcRect = src;
    } else {
        adjustSrcRect.intersect(src);
    }

    Unit::Rect adjustDstRect(0, 0, m_canvasSurface->bufferWidth(),
                             m_canvasSurface->bufferHeight());

    if (adjustDstRect.contains(dst) == true) {
        adjustDstRect = dst;
    } else {
        adjustDstRect.intersect(dst);
    }

    // FIXME : The result of the test below is 150 pass, 1 fail
    // http://web-platform.test:8000/2dcontext/drawing-images-to-the-canvas/drawimage_canvas.html
    DrawImageInfo drawImageInfo = { 1.0, 1.0,
                                    BorderImageRepeatValue::StretchValue,
                                    BorderImageRepeatValue::StretchValue };

    ImageRenderingValue imageRenderingValue = toImageRenderingValue(
        m_canvas->imageSmoothingEnabled(), m_canvas->imageSmoothingQuality());

    m_canvas->drawImage(nativeImageData, src, dst, drawImageInfo,
                        imageRenderingValue);
    m_ownerHTMLCanvasElement->setNeedsComposite();
}

ImageData* CanvasRenderingContext2DMixIn::createImageData(int32_t sw,
                                                          int32_t sh)
{
    if (sw == 0 || sh == 0) {
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

    if (imagedata->width() == 0 || imagedata->height() == 0) {
        throw new DOMException(executionContext(),
                               DOMException::Code::INDEX_SIZE_ERR,
                               "sw and sh are must not zero");
    }
    return new ImageData(executionContext(), pixelsPerRow, rows);
}

ImageData* CanvasRenderingContext2DMixIn::getImageData(int32_t sx, int32_t sy,
                                                       int32_t sw, int32_t sh)
{
    if (sw == 0 || sh == 0) {
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

    // FIXME: Remove below codes When createScriptArrayBuffer handles a
    // RangeError
    Checked<int, RecordOverflow> dataSize = 4;
    dataSize *= sw;
    dataSize *= sh;
    if (dataSize.hasOverflowed() == true) {
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
    if (m_canvasSurface->bufferWidth() != 0 &&
        m_canvasSurface->bufferStride() != 0) {
        stride =
            m_canvasSurface->bufferStride() / m_canvasSurface->bufferWidth();
    } else {
        stride = 4;
    }

    size_t destSize = sw * stride * sh;

    // TODO : If the Canvas Pixel ArrayBuffer cannot be allocated, then rethrow
    // the RangeError thrown by JavaScript, and return.
    auto scriptArrayBuffer = createScriptArrayBuffer(
        executionContext()->scriptBindingInstance(), destSize);
    auto canvasPixelArrayBuffer = createScriptValue(scriptArrayBuffer);
    ContextRef* ctx =
        executionContext()->scriptBindingInstance()->scriptContext();
    uint8_t* dest =
        canvasPixelArrayBuffer->asObject()->asArrayBufferObject()->rawBuffer();

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
            if (a != 0 && a != 255) {
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
        canvasPixelArrayBuffer->asObject()->asArrayBufferObject(), 0, destSize,
        destSize);

    auto ret = new ImageData(executionContext(), uint8ClampedArray, sw, sh);
    return ret;
}

void CanvasRenderingContext2DMixIn::putImageData(ImageData* imagedata,
                                                 int32_t dx, int32_t dy)
{
    STARFISH_ASSERT(imagedata != nullptr);
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

    STARFISH_ASSERT(imagedata != nullptr);

    if (isInfOrNan(dx) == true || isInfOrNan(dy) == true ||
        isInfOrNan(dirtyX) == true || isInfOrNan(dirtyY) == true ||
        isInfOrNan(dirtyWidth) == true || isInfOrNan(dirtyHeight) == true) {
        return;
    }

    if (imagedata->data()->isArrayBufferObject() &&
        imagedata->data()->asArrayBufferObject()->isDetachedBuffer()) {
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
    if (destWidth == 0 || destHeight == 0) {
        return;
    }
    size_t destStride = 0;
    if (destWidth != 0 && m_canvasSurface->bufferStride() != 0) {
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
    if (m_canvas->hasNonInvertableCTM() == true) {
        return;
    }

    // https://html.spec.whatwg.org/multipage/canvas.html#dom-context-2d-clearrect

    if (isInfOrNan(x) == true || isInfOrNan(y) == true ||
        isInfOrNan(w) == true || isInfOrNan(h) == true) {
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
    if (m_canvas->hasNonInvertableCTM() == true) {
        return;
    }

    auto fs = fillStyle();
    if (fs.isCanvasGradientValue() == true &&
        fs.getCanvasGradientValue()->isZeroSize() == true) {
        return;
    }

    if (fs.isCanvasPatternValue() == true &&
        fs.getCanvasPatternValue()->isEmptyPattern() == true) {
        return;
    }

    m_ownerHTMLCanvasElement->setNeedsComposite();

    CanvasFillRule rule;
    if (stringToCanvasFillRule(fillRule, rule) == false) {
        rule = CanvasFillRule::NonZero;
    }

    if (path->isEmpty() == false) {
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
    if (m_canvas->hasNonInvertableCTM() == true) {
        return;
    }

    auto fs = strokeStyle();
    if (fs.isCanvasGradientValue() == true &&
        fs.getCanvasGradientValue()->isZeroSize() == true) {
        return;
    }

    if (fs.isCanvasPatternValue() == true &&
        fs.getCanvasPatternValue()->isEmptyPattern() == true) {
        return;
    }

    m_ownerHTMLCanvasElement->setNeedsComposite();

    if (path->isEmpty() == false) {
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
    if (m_canvas->hasNonInvertableCTM() == true) {
        return;
    }

    CanvasFillRule rule;
    if (stringToCanvasFillRule(fillRule, rule) == false) {
        rule = CanvasFillRule::NonZero;
    }

    if (path->isEmpty() == false) {
        if (rule == CanvasFillRule::NonZero) {
            m_canvas->setFillRule(true);
        } else if (rule == CanvasFillRule::EvenOdd) {
            m_canvas->setFillRule(false);
        }
        m_canvas->clipPath(path);
    }
}

bool CanvasRenderingContext2DMixIn::getPointsUnaffectedByCurrentTransformation(
    const float& x, const float& y, float& ux, float& uy)
{
    SkMatrix matrix;
    if (!m_canvas->currentTransformMatrix().invert(&matrix)) {
        STARFISH_LOG_WARN("Failed to invert current transform matrix");
        return false;
    }

    SkPoint src;
    src.set(SkFloatToScalar(x), SkFloatToScalar(y));
    matrix.mapPoints(&src, 1);
    ux = SkScalarToFloat(src.x());
    uy = SkScalarToFloat(src.y());
    return true;
}

bool CanvasRenderingContext2DMixIn::isPointInPath(Path* path, float x, float y,
                                                  String* fillRule)
{
    if (isInfOrNan(x) == true || isInfOrNan(y) == true) {
        return false;
    }

    CanvasFillRule rule;
    if (stringToCanvasFillRule(fillRule, rule) == false) {
        rule = CanvasFillRule::NonZero;
    }

    float xx, yy;
    if (!getPointsUnaffectedByCurrentTransformation(x, y, xx, yy)) {
        return false;
    }
    return (path->isPointInPath(xx, yy, rule) == true);
}

bool CanvasRenderingContext2DMixIn::isPointInStroke(Path* path, float x,
                                                    float y)
{
    if (isInfOrNan(x) == true || isInfOrNan(y) == true) {
        return false;
    }

    float xx, yy;
    if (!getPointsUnaffectedByCurrentTransformation(x, y, xx, yy)) {
        return false;
    }
    path->applyPathDrawingStyles(m_canvas);
    return (path->isPointInStroke(xx, yy) == true);
}

String* CanvasRenderingContext2DMixIn::font()
{
    return m_canvas->originalFontStr();
}

void CanvasRenderingContext2DMixIn::setFont(String* font)
{
    STARFISH_ASSERT(font != nullptr);
    // Parsing
    auto raw = font->toUTF8NonGCString();
    CSSTokenVector tokens;
    CSSStyleDeclaration::tokenizeCSSValue(tokens, raw.data(), raw.length(),
                                          "/,", 2, true);

    CSSStyleValuePair style /*, variant*/, weight /*, stretch*/, size,
        lineHeight, fontFamily;
    CSSStyleDeclaration::parseFontShorthand(tokens, &style, &weight, &size,
                                            &lineHeight, &fontFamily);

    FontSelector* fs = m_ownerHTMLCanvasElement->document()->fontSelector();
    STARFISH_ASSERT(fs != nullptr);

    String** familyNameArray;
    size_t familyNameArraySize;

    float fixedFontSize = 10;
    char fontStyle = FontStyleValue::NormalFontStyleValue;
    char fontWeight = FontWeightValue::NormalFontWeightValue;

    // font size
    if (size.valueKind() == CSSStyleValuePair::FontSizeValueKind) {
        // fixedFontSize = size.fontSizeValue();
    } else if (size.valueKind() == CSSStyleValuePair::ValueKind::Length) {
        if (size.lengthValue().isFixed()) {
            fixedFontSize = size.lengthValue().fixed();
        }
    }

    // font style
    if (style.valueKind() == CSSStyleValuePair::FontStyleValueKind) {
        fontStyle = style.fontStyleValue();
    }

    // font weight
    if (weight.valueKind() == CSSStyleValuePair::FontWeightValueKind) {
        switch (weight.fontWeightValue()) {
        case OneHundredFontWeightValue:
            fontWeight = 1;
            break;
        case TwoHundredsFontWeightValue:
            fontWeight = 2;
            break;
        case ThreeHundredsFontWeightValue:
            fontWeight = 3;
            break;
        case FourHundredsFontWeightValue:
            fontWeight = 4;
            break;
        case NormalFontWeightValue:
            fontWeight = 4;
            break;
        case FiveHundredsFontWeightValue:
            fontWeight = 5;
            break;
        case SixHundredsFontWeightValue:
            fontWeight = 6;
            break;
        case BoldFontWeightValue:
            fontWeight = 7;
            break;
        case EightHundredsFontWeightValue:
            fontWeight = 8;
            break;
        case NineHundredsFontWeightValue:
            fontWeight = 9;
            break;
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
    }

    // font familyname
    String* fontFamilyStr = nullptr;
    if (fontFamily.valueKind() ==
        CSSStyleValuePair::ValueKind::KeywordValueKind) {
        fontFamilyStr = fontFamily.keywordValue();
        familyNameArray = &fontFamilyStr;
        familyNameArraySize = 1;
    } else if (fontFamily.valueKind() ==
               CSSStyleValuePair::ValueKind::ValueListKind) {
        ValueList* list = fontFamily.multiValue();
        familyNameArraySize = list->size();
        familyNameArray =
            (String**)GC_MALLOC_ATOMIC(sizeof(String*) * (familyNameArraySize));
        for (size_t i = 0; i < familyNameArraySize; ++i) {
            familyNameArray[i] = list->at(i).keywordValue();
        }
    }

    // font fallback
    if (fontFamilyStr == nullptr) {
        String* initialFontFamily = m_ownerHTMLCanvasElement->webView()
                                        ->initialFontFamilyDatas()[1]
                                        .m_familyName.string();
        if (initialFontFamily != nullptr) {
            fontFamilyStr = initialFontFamily;
        } else {
            fontFamilyStr = String::emptyString;
        }
        familyNameArray = &fontFamilyStr;
        familyNameArraySize = 1;
    }

    Font* new_font = fs->loadFont(familyNameArray, familyNameArraySize,
                                  fixedFontSize, fontStyle, fontWeight, 0,
                                  FontKerningValue::FontKerningAutoValue);

    STARFISH_ASSERT(new_font != nullptr);
    m_canvas->setFont(new_font);
    m_canvas->setOriginalFontStr(font);
}

String* CanvasRenderingContext2DMixIn::textAlign()
{
    return canvasTextAlignToString(m_canvas->canvasTextAlign());
}

void CanvasRenderingContext2DMixIn::setTextAlign(String* value)
{
    STARFISH_ASSERT(value != nullptr);
    CanvasTextAlign textAlign;
    if (stringToCanvasTextAlign(value, textAlign) == true) {
        m_canvas->setCanvasTextAlign(textAlign);
    }
}

String* CanvasRenderingContext2DMixIn::textBaseline()
{
    return canvasTextBaselineToString(m_canvas->canvasTextBaseline());
}

void CanvasRenderingContext2DMixIn::setTextBaseline(String* value)
{
    STARFISH_ASSERT(value != nullptr);
    CanvasTextBaseline textBaseline;
    if (stringToCanvasTextBaseline(value, textBaseline) == true) {
        m_canvas->setCanvasTextBaseline(textBaseline);
    }
}

String* CanvasRenderingContext2DMixIn::direction()
{
    if (m_ownerHTMLCanvasElement->style() != nullptr) {
        return canvasDirectionToString(m_canvas->canvasTextDirection(),
                                       m_ownerHTMLCanvasElement->style());
    }
    return canvasDirectionToString(m_canvas->canvasTextDirection(),
                                   executionContext()->document()->style());
}

void CanvasRenderingContext2DMixIn::setDirection(String* value)
{
    STARFISH_ASSERT(value != nullptr);
    CanvasDirection direction;
    if (stringToCanvasDirection(value, direction) == true) {
        m_canvas->setCanvasTextDirection(direction);
    }
}

#ifdef STARFISH_ENABLE_TEST
void CanvasRenderingContext2DMixIn::dump(String* path)
{
    if (m_canvas) {
        m_canvas->dump(path->toUTF8NonGCString().c_str());
    }
}
#endif

} // namespace Starfish

#undef NEEDS_UNPREMULTIPLIED
#undef CRASH
#endif
