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

#include "StarfishConfig.h"
#include "Starfish.h"

#if defined(PORT_COMPOSITOR_BACKEND_SKIA)
#include "core/style/Style.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"
#include "core/modules/canvas/image/NativeImageData.h"
#include "core/page/WebView.h"
#include "platform/window/PlatformWindow.h"
#include "core/modules/canvas/CompositorFactory.h"

#include "SkCanvas.h"
#include "SkPath.h"
#include "SkSurface.h"

#include <vector>
#include <SkMatrix.h>

#include "core/modules/profiling/Profiling.h"

namespace Starfish {

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
        canvas->scale(m_webView->screenInfo().devicePixelRatio,
                      m_webView->screenInfo().devicePixelRatio);
    }

public:
    CompositorImplSkia(WebView* webView, CanvasSurface* data)
    {
        m_webView = webView;
        m_canvas = nullptr;
        m_surface = nullptr;
        m_shouldDestroySkia = true;
        m_shouldDestroySurface = true;

        initFromBuffer(data->mapBuffer(), data->bufferWidth(),
                       data->bufferHeight(), data->bufferStride());

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

    virtual void setFillColor(const Unit::Color& clr)
    {
        Unit::Color c = clr;
        c.m_a = c.m_a * m_opacityVector.back();
        m_paint.setColor(SkColorSetARGB(c.a(), c.r(), c.g(), c.b()));
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
        bitmap.installPixels(SkImageInfo::MakeN32Premul(data->bufferWidth(),
                                                        data->bufferHeight()),
                             data->mapBuffer(), data->bufferStride());
        drawImageSkia(&bitmap, dst, data->bufferWidth(), data->bufferHeight());
        data->unmapBufferAndNotifyUpdatedRegion(0, 0, 0, 0);
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

    virtual SkMatrix currentTransformMatrix()
    {
        return m_canvas->getTotalMatrix();
    }

    virtual void moveTo(float x, float y)
    {
        SkMatrix m = m_canvas->getTotalMatrix();
        SkPoint src = SkPoint::Make(x, y);
        m.mapPoints(&src, 1);
        m_path.moveTo(src.x(), src.y());
    }

    virtual void lineTo(float x, float y)
    {
        SkMatrix m = m_canvas->getTotalMatrix();
        SkPoint src = SkPoint::Make(x, y);
        m.mapPoints(&src, 1);
        m_path.lineTo(src.x(), src.y());
    }

    virtual void arcNegative(double xc, double yc, double radius, double angle1,
                             double angle2)
    {
        SkPath path;
        double a1 = angle1 * 180 / M_PI;
        double a2 = angle2 * 180 / M_PI;
        SkRect rect =
            SkRect::MakeXYWH(xc - radius, yc - radius, radius * 2, radius * 2);
        path.arcTo(rect, a1, a2 - a1, true);
        m_path.addPath(path, m_canvas->getTotalMatrix(),
                       SkPath::kExtend_AddPathMode);
    }

    virtual void clipPath()
    {
        SkMatrix m = m_canvas->getTotalMatrix();
        m_canvas->resetMatrix();
        m_canvas->clipPath(m_path, true);
        m_canvas->setMatrix(m);
        m_path.reset();
    }

protected:
    WebView* m_webView;
    std::vector<float> m_opacityVector;
    size_t m_stateSize;
    sk_sp<SkSurface> m_surface;
    SkCanvas* m_canvas;
    SkPaint m_paint;
    SkPath m_path;
    unsigned m_width;
    unsigned m_height;
    bool m_shouldDestroySkia;
    bool m_shouldDestroySurface;
};

uint32_t CompositorFactory::maximumTextureSizeSkia()
{
    return 65535;
}

Compositor* CompositorFactory::create2dSkia(WebView* wv, CompositorContext* ctx,
                                            CanvasSurface* surface)
{
    return new CompositorImplSkia(wv, surface);
}

Compositor* CompositorFactory::create3dSkia(WebView* wv, CompositorContext* ctx)
{
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
}

bool CompositorFactory::supportsFilterEffectSkia(size_t textureWidth,
                                                 size_t textureHeight)
{
    return false;
}

void CompositorFactory::destroyCompositorContextSkia(
    PlatformWindow* wnd, CompositorContext* ctxInput)
{
}

CompositorContext* CompositorFactory::initCompositorContextSkia(
    PlatformWindow* wnd)
{
    return nullptr;
}

} // namespace Starfish

#endif
