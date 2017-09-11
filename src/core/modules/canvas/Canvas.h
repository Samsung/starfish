/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishCanvas__
#define __StarFishCanvas__

#include "core/modules/canvas/TextDecorationData.h"

namespace StarFish {

class Frame;
class ImageData;
class PlatformWindow;

class CanvasState {
public:
    Unit::Color m_color;
    float m_opacity;
    Font* m_font;
    LayoutUnit m_baseX;
    LayoutUnit m_baseY;
    TextDecorationData m_textDecorationData;

    bool m_visible;

    CanvasState()
    {
        m_opacity = 1;
        m_font = nullptr;
        m_visible = true;
    }
};

class CanvasSurface : public gc {
protected:
    CanvasSurface()
    {
    }

public:
    static CanvasSurface* create(PlatformWindow* window, size_t w, size_t h);
    virtual void* unwrap() = 0;
    virtual void resize(size_t w, size_t h) = 0;
    virtual size_t width() = 0;
    virtual size_t height() = 0;
    virtual void clear() = 0;
    virtual void detachNativeBuffer() = 0;
    virtual ~CanvasSurface()
    {
    }
};

class Canvas : public gc {
protected:
    Canvas()
    {
    }

    LayoutUnit m_viewportWidth;
    LayoutUnit m_viewportHeight;

public:
    enum ReplaceFlag { All, ClippingOnly };

    static Canvas* createDirect(void* data);
    static Canvas* create(CanvasSurface* data);
    static Canvas* createGenericCanvas(ImageData* data);

    virtual ~Canvas()
    {
    }

    void setViewportWidthAndHeight(LayoutUnit width, LayoutUnit height)
    {
        m_viewportWidth = width;
        m_viewportHeight = height;
    }

    void setViewportWidthAndHeight(Canvas* canvas)
    {
        m_viewportWidth = canvas->m_viewportWidth;
        m_viewportHeight = canvas->m_viewportHeight;
    }

    LayoutUnit viewportWidth()
    {
        return m_viewportWidth;
    }
    LayoutUnit viewportHeight()
    {
        return m_viewportHeight;
    }

    virtual void clearColor(const Unit::Color& clr) = 0;

    // state
    virtual void save() = 0;    // push state on state stack
    virtual void restore() = 0; // pop state stack and restore state
    virtual void restoreState(Canvas* canvas) = 0;
    virtual void saveByFrame(Frame* f) = 0;
    virtual CanvasState* getByFrame(Frame* f) = 0;
    virtual void replace(CanvasState* state, ReplaceFlag flag) = 0;
    // transformations (default transform is the identity matrix)
    virtual void scale(double x, double y) = 0;
    virtual void scale(double x, double y, double ox, double oy) = 0;
    virtual void rotate(double angle) = 0;
    virtual void rotate(double angle, double ox, double oy) = 0;
    virtual void translate(double x, double y) = 0;
    virtual void translate(LayoutUnit x, LayoutUnit y) = 0;
    virtual void postMatrix(const SkMatrix& matrix) = 0;

    virtual void clip(const Unit::Rect& rt) = 0;

    virtual void setColor(const Unit::Color& clr) = 0;
    virtual void beginOpacityLayer(float c) = 0;
    virtual void endOpacityLayer() = 0;
    virtual void setFont(Font* font) = 0;
    virtual void resetTextDecorationData() = 0;
    virtual void mergeTextDecorationData(ComputedStyle* style) = 0;

    virtual void drawRect(const Unit::Rect& rt) = 0;
    virtual void drawRect(const LayoutRect& rt) = 0;
    virtual void drawRect(LayoutLocation p1, LayoutLocation p2,
                          LayoutLocation p3,
                          LayoutLocation p4) = 0; // left, top, right, bottom

    virtual void punchHole(const Unit::Rect& rt) = 0;

    virtual void drawText(LayoutUnit x, LayoutUnit y, LayoutUnit stringWidth,
                          const StringView& text) = 0;

    virtual void drawImage(ImageData* data, const Unit::Rect& dst) = 0;
    virtual void drawImage(CanvasSurface* data, const Unit::Rect& dst) = 0;
    virtual void drawBorderImage(ImageData* data, const Unit::Rect& dst,
                                 size_t l, size_t t, size_t r, size_t b,
                                 double scale, bool fill) = 0;
    virtual void drawRepeatImage(ImageData* data, const Unit::Rect& dst,
                                 float imageWidth, float imageHeight,
                                 bool xRepeat, bool yRepeat,
                                 bool isRootElement) = 0;

    virtual void applyMatrixTo(LayoutLocation& lp) = 0;
    virtual void applyMatrixTo(LayoutRect& lp) = 0;

    virtual void setVisible(bool visible) = 0;

    // Generic canvas functions
    virtual void beginPath()
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
    virtual void closePath()
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
    virtual void moveTo(float x, float y)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
    virtual void lineTo(float x, float y)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
    virtual void curveTo(float x1, float y1, float x2, float y2, float x3,
                         float y3)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
    virtual void quadraticCurveTo(float x1, float y1, float x2, float y2)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
    virtual void arc(double xc, double yc, double radius, double angle1,
                     double angle2)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
    virtual void stroke()
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
    virtual void strokePreserve()
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
    virtual void setFillRule(bool shouldUseNonZeroFillRule = true)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
    virtual void fill()
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
    virtual void fillPreserve()
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
    virtual void setStrokeWidth(float width)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    virtual void* unwrap() = 0;
};
}

#endif
