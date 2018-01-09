/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifdef STARFISH_ENABLE_CANVAS

#include "StarFishConfig.h"
#include "StarFish.h"
#include "binding/DOMStringOrCanvasGradientOrCanvasPatternUnion.h"
#include "core/dom/canvas/CanvasRenderingContext2D.h"
#include "core/dom/canvas/HTMLCanvasElement.h"
#include "core/dom/Document.h"

namespace StarFish {

CanvasRenderingContext2D::CanvasRenderingContext2D(
    HTMLCanvasElement* canvasElement)
    : RenderingContext(canvasElement)
{
    STARFISH_LOG_INFO("NOTE: Current CanvasRenderingContext2D is mock\n");
}

void CanvasRenderingContext2D::save()
{
}

void CanvasRenderingContext2D::restore()
{
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
}

void CanvasRenderingContext2D::lineTo(double x, double y)
{
}

void CanvasRenderingContext2D::rect(double x, double y, double w, double h)
{
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
