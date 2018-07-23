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
#include "StarFish.h"

#if defined(PORT_COMPOSITOR_BACKEND_SKIA)

#include "core/modules/canvas/Compositor.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/image/NativeImageData.h"
#include "platform/window/PlatformWindow.h"

#include "SkCanvas.h"
#include "SkSurface.h"

#include <vector>
#include <SkMatrix.h>

#include "core/modules/profiling/Profiling.h"

namespace StarFish {

class CompositorImplSkia : public Compositor {
    void initFromBuffer(void* buffer, int width, int height, int stride)
    {
        SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
        m_surface = SkSurface::MakeRasterDirect(info, buffer, stride);
        m_canvas = m_surface->getCanvas();
        applyDevicePixelRatio(m_canvas);

        m_width = width;
        m_height = height;
    }

    void applyDevicePixelRatio(SkCanvas* canvas)
    {
        canvas->scale(m_starfish->screenInfo().devicePixelRatio,
                      m_starfish->screenInfo().devicePixelRatio);
    }

public:
    CompositorImplSkia(StarFish* starfish, CanvasSurface* data)
    {
        m_starfish = starfish;
        m_canvas = nullptr;
        m_surface = nullptr;
        m_shouldDestroySkia = true;
        m_shouldDestroySurface = true;

        initFromBuffer(data->data(), data->bufferWidth(), data->bufferHeight(),
                       data->bufferStride());

        m_stateSize = 0;
        m_opacityVector.push_back(1);
        save();
    }
    CompositorImplSkia(StarFish* starfish, void* data)
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
        m_stateSize = 0;
        m_opacityVector.push_back(1);
        save();
    }

    ~CompositorImplSkia()
    {
        restore();
        STARFISH_ASSERT(m_stateSize == 0);
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
        m_canvas->save();
        m_canvas->clear(SkColorSetARGB(clr.a(), clr.r(), clr.g(), clr.b()));
        m_canvas->restore();
    }

    // state
    virtual void save()
    {
        m_stateSize++;
        m_canvas->save();
    }

    // pop state stack and restore state
    virtual void restore()
    {
        m_stateSize--;
        m_canvas->restore();
    }

    // transformations (default transform is the identity matrix)
    virtual void scale(double x, double y)
    {
        m_canvas->scale(x, y);
    }

    virtual void rotate(double angle)
    {
        m_canvas->rotate(angle);
    }

    virtual void translate(double x, double y)
    {
        m_canvas->translate(x, y);
    }

    virtual void translate(LayoutUnit x, LayoutUnit y)
    {
        translate(x.toDouble(), y.toDouble());
    }

    virtual void beginOpacityLayer(float c)
    {
        save();
        m_opacityVector.push_back(c * m_opacityVector.back());
    }

    virtual void endOpacityLayer()
    {
        m_opacityVector.pop_back();
        restore();
    }

    virtual void clip(const Unit::Rect& rt)
    {
        m_canvas->clipRect(
            SkRect::MakeXYWH(rt.x(), rt.y(), rt.width(), rt.height()));
    }

    virtual void setColor(const Unit::Color& clr_)
    {
        Unit::Color clr = clr_;
        clr.m_a = clr.m_a * m_opacityVector.back();
        m_paint.setColor(SkColorSetARGB(clr.a(), clr.r(), clr.g(), clr.b()));
    }

    virtual void punchHole(const Unit::Rect& rt)
    {
        STARFISH_ASSERT(m_canvas);
        float xx = rt.x(), yy = rt.y(), ww = rt.width(), hh = rt.height();
        drawSkiaRect(xx, yy, ww, hh, true);
    }

    void drawSkiaRect(float xx, float yy, float ww, float hh,
                      bool isHole = false)
    {
        m_canvas->save();
        if (isHole) {
            SkPaint hole;
            hole.setColor(SkColorSetARGB(0, 0, 0, 0));
            m_canvas->drawRect(SkRect::MakeXYWH(xx, yy, ww, hh), hole);
        } else {
            m_canvas->drawRect(SkRect::MakeXYWH(xx, yy, ww, hh), m_paint);
        }
        m_canvas->restore();
    }

    virtual void drawRect(const Unit::Rect& rt)
    {
        STARFISH_ASSERT(m_canvas);
        float xx = rt.x(), yy = rt.y(), ww = rt.width(), hh = rt.height();
        drawSkiaRect(xx, yy, ww, hh);
    }

    virtual void drawRect(const LayoutRect& rt)
    {
        STARFISH_ASSERT(m_canvas);
        int xx = 0, yy = 0, ww = 0, hh = 0;
        LayoutUnit rx = rt.x();
        LayoutUnit ry = rt.y();

        xx = rx.floor();
        yy = ry.floor();
        ww = snapSizeToPixel(rt.width(), rx);
        hh = snapSizeToPixel(rt.height(), ry);
        drawSkiaRect(xx, yy, ww, hh);
    }

    void drawImageSkia(SkBitmap* bitmap, const Unit::Rect& dst,
                       double surfaceWidth, double surfaceHeight)
    {
        float xx = dst.x();
        float yy = dst.y();
        float ww = dst.width();
        float hh = dst.height();

        if (!surfaceWidth || !surfaceHeight || !ww || !hh) {
            return;
        }

        m_canvas->save();
        SkPaint paint;
        paint.setAlpha((uint8_t)(255.0f * m_opacityVector.back()));
        m_canvas->drawBitmapRect(*bitmap,
                                 SkIRect::MakeWH(surfaceWidth, surfaceHeight),
                                 SkRect::MakeXYWH(xx, yy, ww, hh), &paint);
        m_canvas->restore();
    }

    void drawDebugLine(double xx, double yy, double ww, double hh)
    {
        m_canvas->save();
        // cairo_set_source_rgba(m_canvas, 1, 1, 0, 1);
        // cairo_rectangle(m_canvas, xx, yy, ww, hh);
        // cairo_stroke(m_canvas);
        m_canvas->restore();
    }

    virtual void drawSurface(CanvasSurface* data, const Unit::Rect& dst)
    {
        SkBitmap bitmap;
        bitmap.installPixels(
            SkImageInfo::MakeN32Premul(data->imageWidth(), data->imageHeight()),
            data->data(), data->bufferStride());
        drawImageSkia(&bitmap, dst, data->imageWidth(), data->imageHeight());
    }

    virtual void postMatrix(const SkMatrix& matrix)
    {
        m_canvas->concat(matrix);
    }

    virtual void applyMatrixTo(LayoutLocation& lp)
    {
        double x = lp.x();
        double y = lp.y();

        SkMatrix m = m_canvas->getTotalMatrix();
        SkPoint point;
        m.mapXY(x, y, &point);
        lp.setX(point.x());
        lp.setY(point.y());
    }

    virtual void applyMatrixTo(LayoutRect& lp)
    {
        double x = lp.x();
        double y = lp.y();

        SkMatrix m = m_canvas->getTotalMatrix();
        SkPoint point;
        m.mapXY(x, y, &point);
        lp.setX(point.x());
        lp.setY(point.y());
    }

    virtual void resetMatrixAndClip()
    {
        m_canvas->resetMatrix();
        applyDevicePixelRatio(m_canvas);
        m_canvas->clipRect(SkRect::MakeXYWH(0, 0, m_width, m_height));
    }

    virtual void resetClip()
    {
        SkMatrix m = m_canvas->getTotalMatrix();
        m_canvas->resetMatrix();
        applyDevicePixelRatio(m_canvas);
        m_canvas->clipRect(SkRect::MakeXYWH(0, 0, m_width, m_height));
        m_canvas->concat(m);
    }

protected:
    StarFish* m_starfish;
    std::vector<float> m_opacityVector;
    size_t m_stateSize;
    sk_sp<SkSurface> m_surface;
    SkCanvas* m_canvas;
    SkPaint m_paint;
    unsigned m_width;
    unsigned m_height;
    bool m_shouldDestroySkia;
    bool m_shouldDestroySurface;
};

Compositor* Compositor::create(StarFish* starfish, void* data)
{
    return new CompositorImplSkia(starfish, data);
}

Compositor* Compositor::create(StarFish* starfish, CanvasSurface* surface)
{
    return new CompositorSkia(starfish, surface);
}
} // namespace StarFish

#endif
