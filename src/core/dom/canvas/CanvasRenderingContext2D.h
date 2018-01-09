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

#ifndef __StarFishCanvasRenderingContext2D__
#define __StarFishCanvasRenderingContext2D__

#ifdef STARFISH_ENABLE_CANVAS

#include "core/dom/canvas/RenderingContext.h"
#include "binding/DocumentHoldable.h"
#include "core/page/Serializer.h"

namespace StarFish {

class CanvasGradient;
class DOMStringOrCanvasGradientOrCanvasPattern;

class CanvasRenderingContext2D : public RenderingContext {
public:
    enum CanvasFillRule {
        CanvasFillRuleInvalid,
        CanvasFillRuleNonZero,
        CanvasFillRuleEvenOdd
    };

    CanvasRenderingContext2D(HTMLCanvasElement* canvasElement);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isCanvasRenderingContext2D() const override;

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
    CanvasGradient* createLinearGradient(double x0, double y0, double x1,
                                         double y1);

    // CanvasShadowStyles

    // CanvasFilters
    String* filter();
    void setFilter(String* value);

    // CanvasRect
    void fillRect(double x, double y, double w, double h);

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
    ImageData* getImageData(long sx, long sy, long sw, long sh);

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
};

class CanvasGradient : public ScriptWrappable {
public:
    CanvasGradient(RenderingContext* context)
        : ScriptWrappable(this)
        , m_renderingContext(context)
    {
    }
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isCanvasGradient() const override;
    virtual ScriptBindingInstance* scriptBindingInstance()
    {
        return m_renderingContext->scriptBindingInstance();
    }

    void addColorStop(double offset, String* color);

private:
    RenderingContext* m_renderingContext;
};

class CanvasPattern : public ScriptWrappable {
public:
    CanvasPattern(RenderingContext* context)
        : ScriptWrappable(this)
        , m_renderingContext(context)
    {
    }
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isCanvasPattern() const override;
    virtual ScriptBindingInstance* scriptBindingInstance()
    {
        return m_renderingContext->scriptBindingInstance();
    }

private:
    RenderingContext* m_renderingContext;
};

class ImageData : public ScriptWrappable,
                  public DocumentHoldable,
                  public Serializable {
public:
    ImageData(Document* document);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isImageData() const override;
    virtual ScriptBindingInstance* scriptBindingInstance();

    virtual bool isSerializable() const override;
    virtual Serializable* toSerializable() const override;
    virtual SerializedData* serialize(SerializingMap& memory) override;
    virtual void deserialize(SerializedData* serialized,
                             DeserializingMap& memory) const override;

    uint32_t width();
    void setWidth(uint32_t value);

    uint32_t height();
    void setHeight(uint32_t value);

    ScriptUint8ClampedArray data();
    void setData(ScriptUint8ClampedArray value);

private:
    ScriptUint8ClampedArray m_data;
};

class SerializedImageData : public SerializedPlatformObjectData {
public:
    SerializedImageData()
    {
    }

    ScriptWrappable* createDeserializingInstance(
        Document* document) const override;
};

class Path2D : public ScriptWrappable {
public:
    Path2D(RenderingContext* context)
        : ScriptWrappable(this)
        , m_renderingContext(context)
    {
    }
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isPath2D() const override;
    virtual ScriptBindingInstance* scriptBindingInstance()
    {
        return m_renderingContext->scriptBindingInstance();
    }

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

private:
    RenderingContext* m_renderingContext;
};
}

#endif
#endif
