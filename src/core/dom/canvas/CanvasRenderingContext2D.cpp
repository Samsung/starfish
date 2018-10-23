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
#include "Starfish.h"
#include "binding/DOMStringOrCanvasGradientOrCanvasPatternUnion.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"

#include "core/dom/canvas/CanvasRenderingContext2D.h"
#include "core/dom/canvas/HTMLCanvasElement.h"
#include "core/style/Style.h"
#include "core/dom/Document.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameBox.h"
#include "core/layout/FrameReplacedCanvas.h"
#include "core/layout/StackingContext.h"

namespace Starfish {

CanvasRenderingContext2D::CanvasRenderingContext2D(
    HTMLCanvasElement* canvasElement)
    : RenderingContext(canvasElement)
    , m_lineWidth(1)
{
    initialize();
}

void CanvasRenderingContext2D::initialize()
{
    HTMLCanvasElement* canvas = m_canvasElement;

    if (m_surface) {
        m_surface->detachNativeBuffer();
    }

    if (!canvas) {
        return;
    }

    // Create CanvasSurface.
    m_surface = CanvasSurface::create(canvas->webView()->platformWindow(),
                                      canvas->width(), canvas->height());

    if (m_surfaceCanvas) {
        delete m_surfaceCanvas;
    }

    // Set defualt values such as color, fill color and stroke color.
    m_surfaceCanvas = Canvas::create(canvas->webView(), m_surface);
    // Set the defualt color as black.
    m_surfaceCanvas->setColor(Unit::Color(0, 0, 0, 255));
    // Set the fill color as black.
    m_fillColor = Unit::Color(0, 0, 0, 255);
    // Set the stroke color as black.
    m_strokeColor = Unit::Color(0, 0, 0, 255);
    // Set the line width as 1.0f.
    m_lineWidth = 1.0f;
    m_surfaceCanvas->setStrokeWidth(m_lineWidth);
}

void CanvasRenderingContext2D::setLineWidth(double width)
{
    m_lineWidth = width;
    m_surfaceCanvas->setStrokeWidth(m_lineWidth);
}

void CanvasRenderingContext2D::save()
{
    m_surfaceCanvas->save();
}

void CanvasRenderingContext2D::restore()
{
    m_surfaceCanvas->restore();
}

void CanvasRenderingContext2D::scale(double x, double y)
{
}

void CanvasRenderingContext2D::rotate(double angle)
{
}

void CanvasRenderingContext2D::translate(double x, double y)
{
}

void CanvasRenderingContext2D::transform(double a, double b, double c, double d,
                                         double e, double f)
{
}

double CanvasRenderingContext2D::globalAlpha()
{
    return 1.0;
}

void CanvasRenderingContext2D::setGlobalAlpha(double value)
{
}

String* CanvasRenderingContext2D::globalCompositeOperation()
{
    return String::fromUTF8("source-over");
}

void CanvasRenderingContext2D::setGlobalCompositeOperation(String* value)
{
}

DOMStringOrCanvasGradientOrCanvasPattern CanvasRenderingContext2D::fillStyle()
{
    return DOMStringOrCanvasGradientOrCanvasPattern::createDOMString(
        String::fromUTF8("black"));
}

void CanvasRenderingContext2D::setFillStyle(
    DOMStringOrCanvasGradientOrCanvasPattern value)
{
    if (value.isDOMStringValue()) {
        String* v = value.getDOMStringValue();
        auto s1 = v->toUTF8NonGCString();
        NamedColor::NamedColorValue ret;
        if (NamedColor::parseNamedColor(s1.data(), s1.length(), ret)) {
            Unit::Color c = NamedColor::namedColorToColor(ret);
            m_fillColor = Unit::Color(c.r(), c.g(), c.b(), c.a());
        }
    }
}

DOMStringOrCanvasGradientOrCanvasPattern CanvasRenderingContext2D::strokeStyle()
{
    return DOMStringOrCanvasGradientOrCanvasPattern::createDOMString(
        String::fromUTF8("black"));
}

void CanvasRenderingContext2D::setStrokeStyle(
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

CanvasGradient* CanvasRenderingContext2D::createLinearGradient(double x0,
                                                               double y0,
                                                               double x1,
                                                               double y1)
{
    return new CanvasGradient(this);
}

String* CanvasRenderingContext2D::filter()
{
    return String::fromUTF8("none");
}

void CanvasRenderingContext2D::setFilter(String* value)
{
}

void CanvasRenderingContext2D::fillRect(double x, double y, double w, double h)
{
    HTMLCanvasElement* canvas = m_canvasElement;
    canvas->setNeedsPainting();
    m_surfaceCanvas->setColor(m_fillColor);
    m_surfaceCanvas->drawRect(LayoutRect(x, y, w, h));
}

void CanvasRenderingContext2D::beginPath()
{
}

void CanvasRenderingContext2D::fill(String* fillRule)
{
}

void CanvasRenderingContext2D::fill(Path2D* path, String* fillRule)
{
}

void CanvasRenderingContext2D::stroke()
{
    HTMLCanvasElement* canvas = m_canvasElement;
    canvas->setNeedsPainting();
    m_surfaceCanvas->setColor(m_strokeColor);
    m_surfaceCanvas->stroke();
}

void CanvasRenderingContext2D::drawImage(ScriptValue image, double dx,
                                         double dy)
{
}

void CanvasRenderingContext2D::drawImage(ScriptValue image, double dx,
                                         double dy, double dw, double dh)
{
}

void CanvasRenderingContext2D::drawImage(ScriptValue image, double sx,
                                         double sy, double sw, double sh,
                                         double dx, double dy, double dw,
                                         double dh)
{
}

ImageData* CanvasRenderingContext2D::getImageData(long sx, long sy, long sw,
                                                  long sh)
{
    return new ImageData(canvas()->document());
}

void CanvasRenderingContext2D::closePath()
{
}

void CanvasRenderingContext2D::moveTo(double x, double y)
{
    HTMLCanvasElement* canvas = m_canvasElement;
    canvas->setNeedsPainting();
    m_surfaceCanvas->moveTo(x, y);
}

void CanvasRenderingContext2D::lineTo(double x, double y)
{
    HTMLCanvasElement* canvas = m_canvasElement;
    canvas->setNeedsPainting();
    m_surfaceCanvas->lineTo(x, y);
}

void CanvasRenderingContext2D::rect(double x, double y, double w, double h)
{
    HTMLCanvasElement* canvas = m_canvasElement;
    canvas->setNeedsPainting();
    m_surfaceCanvas->moveTo(x - m_lineWidth / 2, y);
    m_surfaceCanvas->lineTo(x + w, y);
    m_surfaceCanvas->lineTo(x + w, y + h);
    m_surfaceCanvas->lineTo(x, y + h);
    m_surfaceCanvas->lineTo(x, y);
}

void CanvasRenderingContext2D::arc(double x, double y, double radius,
                                   double startAngle, double endAngle,
                                   bool anticlockwise)
{
}

void CanvasRenderingContext2D::ellipse(double x, double y, double radiusX,
                                       double radiusY, double rotation,
                                       double startAngle, double endAngle,
                                       bool anticlockwise)
{
}

void CanvasRenderingContext2D::bezierCurveTo(double x1, double y1, double x2,
                                             double y2, double x3, double y3)
{
    HTMLCanvasElement* canvas = m_canvasElement;
    canvas->setNeedsPainting();
    m_surfaceCanvas->curveTo(x1, y1, x2, y2, x3, y3);
}

void CanvasRenderingContext2D::clearRect(double x, double y, double w, double h)
{
    HTMLCanvasElement* canvas = m_canvasElement;
    canvas->setNeedsPainting();
    m_surfaceCanvas->setColor(Unit::Color(255, 255, 255, 255));
    m_surfaceCanvas->drawRect(LayoutRect(x, y, w, h));
    m_surfaceCanvas->fill();
    m_surfaceCanvas->setColor(Unit::Color(0, 0, 0, 255));
}

void CanvasGradient::addColorStop(double offset, String* color)
{
}

ImageData::ImageData(Document* document)
    : ScriptWrappable(this)
    , DocumentHoldable(document)
    , m_data(createEmptyUint8ClampedArray(document->scriptBindingInstance()))
{
}

ScriptBindingInstance* ImageData::scriptBindingInstance()
{
    return document()->scriptBindingInstance();
}

SerializedData* ImageData::serialize(SerializingMap& memory)
{
    return new SerializedImageData();
}

void ImageData::deserialize(SerializedData* serialized,
                            DeserializingMap& memory) const
{
}

uint32_t ImageData::width()
{
    return 1;
}

void ImageData::setWidth(uint32_t value)
{
}

uint32_t ImageData::height()
{
    return 1;
}

void ImageData::setHeight(uint32_t value)
{
}

ScriptUint8ClampedArray ImageData::data()
{
    return m_data;
}

void ImageData::setData(ScriptUint8ClampedArray value)
{
    m_data = value;
}

ScriptWrappable* SerializedImageData::createDeserializingInstance(
    Document* document) const
{
    return new ImageData(document);
}

void Path2D::closePath()
{
}

void Path2D::moveTo(double x, double y)
{
}

void Path2D::lineTo(double x, double y)
{
}

void Path2D::rect(double x, double y, double w, double h)
{
}

void Path2D::arc(double x, double y, double radius, double startAngle,
                 double endAngle, bool anticlockwise)
{
}

void Path2D::ellipse(double x, double y, double radiusX, double radiusY,
                     double rotation, double startAngle, double endAngle,
                     bool anticlockwise)
{
}
}

#endif
