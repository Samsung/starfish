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

#include "StarFishConfig.h"

#if defined(PORT_CANVAS_BACKEND_SKIA)
#include "StarFish.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/font/Font.h"
#include "core/modules/canvas/image/NativeImageData.h"
#include "core/style/UnitHelper.h"

#include "SkCanvas.h"
#include "SkDashPathEffect.h"
#include "SkPath.h"
#include "SkSurface.h"

namespace StarFish {

class CanvasStateSkia : public CanvasState {
public:
    CanvasStateSkia()
        : CanvasState()
    {
    }
};

class CanvasSkia : public Canvas {
public:
    CanvasSkia(StarFish* starfish, void* data)
    {
        struct dummy {
            SkCanvas* canvas;
            sk_sp<SkSurface> surface;
            int w;
            int h;
        };
        dummy* d = (dummy*)data;
        m_starfish = starfish;
        m_canvas = d->canvas;
        m_surface = d->surface;
        m_width = d->w;
        m_height = d->h;
        m_shouldDestroySkia = false;
        m_shouldDestroySurface = false;
        save();
    }

    CanvasSkia(StarFish* starfish, CanvasSurface* data)
    {
    }

    virtual ~CanvasSkia()
    {
        restore();
        STARFISH_ASSERT(m_state.size() == 0);
        m_canvas->flush();
        if (m_shouldDestroySkia) {
            m_canvas = nullptr;
        }
        if (m_shouldDestroySurface) {
            m_surface = nullptr;
        }
    }

    virtual void clearColor(const Unit::Color& clr)
    {
        INSTALL_PROFILE_TIMER(m_starfish, "CanvasImplCairo::clear");
        m_canvas->save();
        if (clr.a() == 0) {
            m_canvas->clear(SK_ColorTRANSPARENT);
        } else {
            m_canvas->clear(SkColorSetARGB(clr.a(), clr.r(), clr.g(), clr.b()));
        }
        m_canvas->restore();
    }

    // state
    virtual void save() // push state on state stack
    {
        CanvasStateSkia state;
        if (m_state.size()) {
            auto& lastState = m_state.back();
            state.m_color = lastState.m_color;
            state.m_opacity = lastState.m_opacity;
            state.m_font = lastState.m_font;
            state.m_visible = lastState.m_visible;
            state.m_textDecorationData = lastState.m_textDecorationData;
        }
        m_state.push_back(state);
        m_canvas->save();
    }

    virtual void restore() // pop state stack and restore state
    {
        m_state.erase(m_state.end() - 1);
        m_canvas->restore();
    }

    virtual void scale(double x, double y)
    {
        m_canvas->scale(x, y);
    }

    virtual void scale(double x, double y, double ox, double oy)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void rotate(double angle)
    {
        m_canvas->rotate(angle);
    }

    virtual void rotate(double angle, double ox, double oy)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void translate(double x, double y)
    {
        m_canvas->translate(x, y);
    }

    virtual void translate(LayoutUnit x, LayoutUnit y)
    {
        m_canvas->translate(x.toDouble(), y.toDouble());
    }

    virtual void postMatrix(const SkMatrix& matrix)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void clip(const Unit::Rect& rt)
    {
        m_canvas->clipRect(
            SkRect::MakeXYWH(rt.x(), rt.y(), rt.width(), rt.height()));
    }

    virtual void setColor(const Unit::Color& clr)
    {
        STARFISH_ASSERT(m_canvas);
        lastState().m_color = clr;
        m_paint.setColor(SkColorSetARGB(clr.a(), clr.r(), clr.g(), clr.b()));
    }

    virtual void beginOpacityLayer(float c)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void endOpacityLayer()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void setFont(Font* font)
    {
        lastState().m_font = font;
    }

    virtual void resetTextDecorationData()
    {
        lastState().m_textDecorationData.reset();
    }

    virtual void mergeTextDecorationData(ComputedStyle* style)
    {
        lastState().m_textDecorationData.merge(style);
    }

    virtual TextDecorationData textDecorationData()
    {
        return lastState().m_textDecorationData;
    }

    virtual void setTextDecorationData(TextDecorationData d)
    {
        lastState().m_textDecorationData = d;
    }

    virtual void drawRect(const Unit::Rect& rt)
    {
        STARFISH_ASSERT(m_canvas);
        if (!lastState().m_visible) {
            return;
        }
        m_canvas->drawRect(
            SkRect::MakeXYWH(rt.x(), rt.y(), rt.width(), rt.height()), m_paint);
    }

    virtual void drawRect(const LayoutRect& rt)
    {
        STARFISH_ASSERT(m_canvas);
        if (!lastState().m_visible) {
            return;
        }
        int xx = 0, yy = 0, ww = 0, hh = 0;
        LayoutUnit rx = rt.x();
        LayoutUnit ry = rt.y();

        xx = rx.floor();
        yy = ry.floor();
        ww = snapSizeToPixel(rt.width(), rx);
        hh = snapSizeToPixel(rt.height(), ry);
        m_canvas->drawRect(SkRect::MakeXYWH(xx, yy, ww, hh), m_paint);
    }

    virtual void drawRect(LayoutLocation p1, LayoutLocation p2,
                          LayoutLocation p3, LayoutLocation p4)
    {
    } // left, top, right, bottom

    virtual void punchHole(const Unit::Rect& rt)
    {
    }

    virtual void drawText(LayoutUnit x, LayoutUnit y, LayoutUnit stringWidth,
                          const StringView& text)
    {
    }

    virtual void drawImage(NativeImageData* data, const Unit::Rect& dst,
                           ImageRenderingValue imageRenderingMode)
    {
        if (!lastState().m_visible) {
            return;
        }
    }

    virtual void drawImage(CanvasSurface* data, const Unit::Rect& dst,
                           ImageRenderingValue imageRenderingMode)
    {
    }

    virtual void drawImage(NativeImageData* data, const Unit::Rect& src,
                           const Unit::Rect& dst,
                           const DrawImageInfo& borderinfo,
                           ImageRenderingValue imageRenderingMode)
    {
    }

    virtual void drawBorderImage(NativeImageData* data, const Unit::Rect& dst,
                                 size_t l, size_t t, size_t r, size_t b,
                                 double scale, bool fill)
    {
    }

    virtual void drawRepeatImage(
        NativeImageData* data, const Unit::Rect& dst, float imageWidth,
        float imageHeight, bool xRepeat, bool yRepeat,
        ImageRenderingValue imageRenderingMode =
            ImageRenderingValue::ImageRenderingAutoValue)
    {
    }

    virtual void drawLinearGradient(const Unit::Rect& dst,
                                    GradientDrawingInfo* info)
    {
    }

    virtual void drawRadialGradient(const Unit::Rect& dst,
                                    GradientDrawingInfo* info)
    {
    }

    virtual void applyMatrixTo(LayoutLocation& lp)
    {
    }

    virtual void applyMatrixTo(LayoutRect& lp)
    {
    }

    virtual void setVisible(bool visible)
    {
    }

    virtual void beginPath()
    {
        m_path.reset();
    }

    virtual void closePath()
    {
        m_path.close();
    }

    virtual void moveTo(float x, float y)
    {
        m_path.moveTo(x, y);
    }

    virtual void lineTo(float x, float y)
    {
        m_path.lineTo(x, y);
    }

    virtual void curveTo(float x1, float y1, float x2, float y2, float x3,
                         float y3)
    {
    }

    virtual void quadraticCurveTo(float x1, float y1, float x2, float y2)
    {
    }

    virtual void arc(double xc, double yc, double radius, double angle1,
                     double angle2)
    {
    }

    virtual void arcNegative(double xc, double yc, double radius, double angle1,
                             double angle2)
    {
    }

    virtual void stroke()
    {
        strokePreserve();
        m_path.reset();
    }

    virtual void strokePreserve()
    {
        m_paint.setStyle(SkPaint::kStroke_Style);
        m_canvas->drawPath(m_path, m_paint);
    }

    virtual void fill()
    {
        fillPreserve();
        m_path.reset();
    }

    virtual void fillPreserve()
    {
        m_paint.setStyle(SkPaint::kFill_Style);
        m_canvas->drawPath(m_path, m_paint);
    }

    virtual void clipPath()
    {
        clipPathPreserve();
        m_path.reset();
    }

    virtual void clipPathPreserve()
    {
        m_canvas->clipPath(m_path);
    }

    virtual void setFillRule(bool shouldUseNonZeroFillRule)
    {
    }

    virtual void setStrokeWidth(float width)
    {
        m_paint.setStrokeWidth(width);
    }

    virtual void setDash(double* dashes, int dashCnt, double offset)
    {
    }

    virtual void* unwrap()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
        return NULL;
    }

    CanvasStateSkia& lastState()
    {
        STARFISH_ASSERT(m_state.size());
        return m_state[m_state.size() - 1];
    }
    // reset transform matrix & clip
    virtual void resetMatrixAndClip()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    // reset transform clip
    virtual void resetClip()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

protected:
    StarFish* m_starfish;
    std::vector<CanvasStateSkia> m_state;
    SkCanvas* m_canvas;
    sk_sp<SkSurface> m_surface;
    SkPaint m_paint;
    SkPath m_path;

    unsigned m_width;
    unsigned m_height;

    bool m_shouldDestroySkia;
    bool m_shouldDestroySurface;
};

Canvas* Canvas::createDirect(StarFish* starfish, void* data)
{
    return new CanvasSkia(starfish, data);
}

Canvas* Canvas::create(StarFish* starfish, CanvasSurface* data)
{
    return new CanvasSkia(starfish, data);
}

Canvas* Canvas::createGenericCanvas(StarFish* starfish, void* data, size_t w,
                                    size_t h)
{
    return new CanvasSkia(starfish, data);
}
Canvas* Canvas::createGenericCanvas(StarFish* starfish, NativeImageData* data)
{
    return new CanvasSkia(starfish, data);
}
}
#endif
