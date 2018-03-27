/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishCanvas__
#define __StarFishCanvas__

#define STARFISH_CANVAS_LENGTH_MAX 65535

#include "core/modules/canvas/TextDecorationData.h"
#include "core/modules/canvas/CanvasShadowData.h"
#include "core/layout/Frame.h"

namespace StarFish {

class Frame;
class NativeImageData;
class PlatformWindow;

class CanvasState {
public:
    Unit::Color m_color;
    float m_opacity;
    Font* m_font;
    TextDecorationData m_textDecorationData;

    bool m_visible;

    CanvasState()
    {
        m_opacity = 1;
        m_font = nullptr;
        m_visible = true;
    }
};

#ifndef STARFISH_CANVAS_SURFACE_MARGIN
#define STARFISH_CANVAS_SURFACE_MARGIN 5
#endif

class CanvasSurface : public gc {
protected:
    CanvasSurface()
    {
    }

public:
    static CanvasSurface* create(PlatformWindow* window, size_t w, size_t h);
    virtual void* unwrap() = 0;
    virtual uint8_t* data() = 0;
    virtual void resize(size_t w, size_t h) = 0;
    virtual void clear() = 0;
    virtual void detachNativeBuffer() = 0;
    virtual ~CanvasSurface()
    {
    }

    // width / pixelRatio == imageWidth
    // bufferWidth > imageWidth
    // buffer is can be lager than image
    virtual size_t width() = 0;
    virtual size_t height() = 0;
    virtual size_t pixelRatio() = 0;

    virtual size_t imageWidth() = 0;
    virtual size_t imageHeight() = 0;

    virtual size_t bufferWidth() = 0;
    virtual size_t bufferHeight() = 0;
    virtual size_t bufferStride() = 0;
};

struct BorderInfo {
    double scale;
    BorderImageRepeatValue hRepeat;
    BorderImageRepeatValue vRepeat;
};

class Canvas : public gc {
protected:
    Canvas()
    {
    }

public:
    static Canvas* createDirect(StarFish* starfish, void* data);
    static Canvas* create(StarFish* starfish, CanvasSurface* data);
    static Canvas* createGenericCanvas(StarFish* starfish, void* data, size_t w,
                                       size_t h);
    static Canvas* createGenericCanvas(StarFish* starfish,
                                       NativeImageData* data);

    virtual ~Canvas()
    {
    }

    virtual void clearColor(const Unit::Color& clr) = 0;

    // state
    virtual void save() = 0;    // push state on state stack
    virtual void restore() = 0; // pop state stack and restore state
    // transformations (default transform is the identity matrix)
    virtual void scale(double x, double y) = 0;
    virtual void rotate(double angle) = 0;
    virtual void translate(double x, double y) = 0;
    virtual void translate(LayoutUnit x, LayoutUnit y) = 0;
    virtual void postMatrix(const SkMatrix& matrix) = 0;

    virtual void clip(const Unit::Rect& rt) = 0;

    // reset transform matrix & clip
    virtual void resetMatrixAndClip() = 0;
    // reset transform clip
    virtual void resetClip() = 0;

    virtual void setColor(const Unit::Color& clr) = 0;
    virtual void beginOpacityLayer(float c) = 0;
    virtual void endOpacityLayer() = 0;
    virtual void setFont(Font* font) = 0;
    virtual void resetTextDecorationData() = 0;
    virtual void mergeTextDecorationData(ComputedStyle* style) = 0;
    virtual TextDecorationData textDecorationData() = 0;
    virtual void setTextDecorationData(TextDecorationData d) = 0;
    virtual void drawRect(const Unit::Rect& rt) = 0;
    virtual void drawRect(const LayoutRect& rt) = 0;
    virtual void drawRect(LayoutLocation p1, LayoutLocation p2,
                          LayoutLocation p3,
                          LayoutLocation p4) = 0; // left, top, right, bottom
    virtual void setDash(double* dashes, int dashCnt, double offset) = 0;

    virtual void punchHole(const Unit::Rect& rt) = 0;

    virtual void drawText(LayoutUnit x, LayoutUnit y, LayoutUnit stringWidth,
                          const StringView& text) = 0;

    virtual void drawImage(
        NativeImageData* data, const Unit::Rect& dst,
        ImageRenderingValue imageRenderingMode =
            ImageRenderingValue::ImageRenderingAutoValue) = 0;
    virtual void drawImage(
        CanvasSurface* data, const Unit::Rect& dst,
        ImageRenderingValue imageRenderingMode =
            ImageRenderingValue::ImageRenderingAutoValue) = 0;
    virtual void drawImage(
        NativeImageData* data, const Unit::Rect& src, const Unit::Rect& dst,
        BorderInfo& borderinfo,
        ImageRenderingValue imageRenderingMode =
            ImageRenderingValue::ImageRenderingAutoValue) = 0;
    virtual void drawBorderImage(NativeImageData* data, const Unit::Rect& dst,
                                 size_t l, size_t t, size_t r, size_t b,
                                 double scale, bool fill) = 0;
    virtual void drawRepeatImage(
        NativeImageData* data, const Unit::Rect& dst, float imageWidth,
        float imageHeight, bool xRepeat, bool yRepeat,
        ImageRenderingValue imageRenderingMode =
            ImageRenderingValue::ImageRenderingAutoValue) = 0;

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
    virtual void arcNegative(double xc, double yc, double radius, double angle1,
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
    virtual void clipPath()
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
    virtual void clipPathPreserve()
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
    virtual void setStrokeWidth(float width)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
    virtual void setNeedsFastAntialias()
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
    virtual void setNeedsGoodQualityAntialias()
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    virtual void* unwrap() = 0;
};
}

#endif
