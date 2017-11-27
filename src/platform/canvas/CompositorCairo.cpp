/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include "StarFish.h"

#if defined(PORT_COMPOSITOR_BACKEND_CAIRO)

#include "core/modules/canvas/Compositor.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/image/ImageData.h"
#include "platform/window/PlatformWindow.h"

#include <vector>
#include <SkMatrix.h>

#include <cairo.h>

#include "core/modules/profiling/Profiling.h"

#define CAIRO_FORMAT CAIRO_FORMAT_ARGB32
// #define STARFISH_ENABLE_TIMER

#ifdef STARFISH_ENABLE_TIMER
#define INSTALL_PROFILE_TIMER(s) ProfilerTimer _p(s);
#else
#define INSTALL_PROFILE_TIMER(s)
#endif

namespace StarFish {

class CompositorImplCairo : public Compositor {
    void initFromBuffer(void* buffer, int width, int height, int stride)
    {
        m_surface = cairo_image_surface_create_for_data(
            (unsigned char*)buffer, CAIRO_FORMAT, width, height, stride);
        m_canvas = cairo_create(m_surface);
        m_width = width;
        m_height = height;
    }

public:
    CompositorImplCairo(StarFish* starfish, CanvasSurface* data)
    {
        m_starfish = starfish;
        m_canvas = nullptr;
        m_surface = nullptr;
        m_shouldDestroyCairo = true;
        m_shouldDestroySurface = true;

        initFromBuffer(data->data(), data->bufferWidth(), data->bufferHeight(),
                       data->bufferStride());

        m_stateSize = 0;
        m_opacityVector.push_back(1);
        cairo_set_antialias(m_canvas, CAIRO_ANTIALIAS_FAST);
        save();
    }
    CompositorImplCairo(StarFish* starfish, void* data)
    {
        struct dummy {
            cairo_t* cairo;
            cairo_surface_t* surface;
            int w;
            int h;
        };
        dummy* d = (dummy*)data;
        m_starfish = starfish;
        m_canvas = (cairo_t*)d->cairo;
        m_surface = (cairo_surface_t*)d->surface;
        m_width = d->w;
        m_height = d->h;
        m_shouldDestroyCairo = false;
        m_shouldDestroySurface = false;
        m_stateSize = 0;
        m_opacityVector.push_back(1);
        cairo_set_antialias(m_canvas, CAIRO_ANTIALIAS_FAST);
        save();
    }

    ~CompositorImplCairo()
    {
        restore();
        STARFISH_ASSERT(m_stateSize == 0);
        if (m_shouldDestroyCairo) {
            cairo_destroy(m_canvas);
        }
        cairo_surface_flush(m_surface);
        if (m_shouldDestroySurface) {
            cairo_surface_destroy(m_surface);
        }
    }

    void checkError()
    {
#ifndef NDEBUG
        auto status = cairo_status(m_canvas);
        if (status != CAIRO_STATUS_SUCCESS) {
            STARFISH_LOG_ERROR("%s\n", cairo_status_to_string(status));
            STARFISH_ASSERT_NOT_REACHED();
        }
#endif
    }

    virtual void clearColor(const Unit::Color& clr)
    {
        cairo_save(m_canvas);
        cairo_set_source_rgba(m_canvas, clr.R(), clr.G(), clr.B(), clr.A());
        cairo_set_operator(m_canvas, CAIRO_OPERATOR_SOURCE);
        cairo_paint(m_canvas);
        cairo_restore(m_canvas);
    }

    // state
    virtual void save()
    {
        checkError();
        m_stateSize++;
        cairo_save(m_canvas);
    }

    // pop state stack and restore state
    virtual void restore()
    {
        checkError();
        m_stateSize--;
        cairo_restore(m_canvas);
    }

    // transformations (default transform is the identity matrix)
    virtual void scale(double x, double y)
    {
        cairo_scale(m_canvas, x, y);
    }

    virtual void rotate(double angle)
    {
        cairo_rotate(m_canvas, angle);
    }

    virtual void translate(double x, double y)
    {
        cairo_translate(m_canvas, x, y);
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
        cairo_rectangle(m_canvas, rt.x(), rt.y(), rt.width(), rt.height());
        cairo_clip(m_canvas);
    }

    virtual void setColor(const Unit::Color& clr_)
    {
        Unit::Color clr = clr_;
        clr.m_a = clr.m_a * m_opacityVector.back();
        cairo_set_source_rgba(m_canvas, clr.R(), clr.G(), clr.B(), clr.A());
    }

    virtual void punchHole(const Unit::Rect& rt)
    {
        STARFISH_ASSERT(m_canvas);
        float xx = rt.x(), yy = rt.y(), ww = rt.width(), hh = rt.height();
        drawCairoRect(xx, yy, ww, hh, true);
    }

    void drawCairoRect(float xx, float yy, float ww, float hh,
                       bool isHole = false)
    {
        cairo_save(m_canvas);
        if (isHole) {
            cairo_set_source_rgba(m_canvas, 0, 0, 0, 0);
            cairo_set_operator(m_canvas, CAIRO_OPERATOR_SOURCE);
        }
        cairo_translate(m_canvas, xx, yy);
        cairo_rectangle(m_canvas, 0, 0, ww, hh);
        cairo_fill(m_canvas);
        cairo_restore(m_canvas);
    }

    virtual void drawRect(const Unit::Rect& rt)
    {
        STARFISH_ASSERT(m_canvas);
        float xx = rt.x(), yy = rt.y(), ww = rt.width(), hh = rt.height();
        drawCairoRect(xx, yy, ww, hh);
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
        drawCairoRect(xx, yy, ww, hh);
    }

    void drawImageCairo(cairo_surface_t* localSurface, const Unit::Rect& dst,
                        double surfaceWidth, double surfaceHeight,
                        bool isFromSurface = false)
    {
        float xx = dst.x();
        float yy = dst.y();
        float ww = dst.width();
        float hh = dst.height();

        if (!surfaceWidth || !surfaceHeight || !ww || !hh) {
            return;
        }

        cairo_save(m_canvas);

        cairo_pattern_t* resizePattern;
        cairo_matrix_t matrix;

        resizePattern = cairo_pattern_create_for_surface(localSurface);
        cairo_translate(m_canvas, xx, yy);

        cairo_matrix_init_identity(&matrix);
        cairo_matrix_scale(&matrix, surfaceWidth / ww, surfaceHeight / hh);
        cairo_pattern_set_matrix(resizePattern, &matrix);
        cairo_pattern_set_filter(resizePattern, CAIRO_FILTER_FAST);
        checkError();
        cairo_set_source(m_canvas, resizePattern);

        cairo_rectangle(m_canvas, 0, 0, ww, hh);
        cairo_clip(m_canvas);

        cairo_matrix_t t;
        cairo_get_matrix(m_canvas, &t);
        double x, y;
        double minX, minY, maxX, maxY;
        x = dst.x();
        y = dst.y();
        cairo_matrix_transform_point(&t, &x, &y);
        minX = x;
        minY = y;
        maxX = x;
        maxY = y;

        x = dst.maxX();
        y = dst.y();
        cairo_matrix_transform_point(&t, &x, &y);
        minX = std::min(x, minX);
        minY = std::min(y, minY);
        maxX = std::min(x, maxX);
        maxY = std::max(y, maxY);

        x = dst.x();
        y = dst.maxY();
        cairo_matrix_transform_point(&t, &x, &y);
        minX = std::min(x, minX);
        minY = std::min(y, minY);
        maxX = std::min(x, maxX);
        maxY = std::max(y, maxY);

        x = dst.maxX();
        y = dst.maxY();
        cairo_matrix_transform_point(&t, &x, &y);
        minX = std::min(x, minX);
        minY = std::min(y, minY);
        maxX = std::min(x, maxX);
        maxY = std::max(y, maxY);
        if (std::abs(minX - maxX) < 65535 && std::abs(minY - maxY) < 65535) {
            cairo_paint_with_alpha(m_canvas, m_opacityVector.back());
        }

        cairo_pattern_destroy(resizePattern);
        cairo_restore(m_canvas);
        checkError();
    }

    void drawDebugLine(double xx, double yy, double ww, double hh)
    {
        cairo_save(m_canvas);

        cairo_set_source_rgba(m_canvas, 1, 1, 0, 1);
        cairo_rectangle(m_canvas, xx, yy, ww, hh);
        cairo_stroke(m_canvas);

        cairo_restore(m_canvas);
    }

    virtual void drawImage(ImageData* data, const Unit::Rect& dst)
    {
        void* imgData = data->data();
        double surfaceWidth = 0, surfaceHeight = 0;
        cairo_surface_t* image;

        int stride = data->stride();
        image = cairo_image_surface_create_for_data((unsigned char*)imgData,
                                                    CAIRO_FORMAT, data->width(),
                                                    data->height(), stride);
        surfaceWidth = data->width();
        surfaceHeight = data->height();
        STARFISH_ASSERT(surfaceWidth);
        STARFISH_ASSERT(surfaceHeight);
        STARFISH_ASSERT(stride);
        drawImageCairo(image, dst, surfaceWidth, surfaceHeight);
        cairo_surface_destroy(image);
    }

    virtual void drawSurface(CanvasSurface* data, const Unit::Rect& dst)
    {
        cairo_surface_t* image;
        image = cairo_image_surface_create_for_data(
            (unsigned char*)data->data(), CAIRO_FORMAT_ARGB32,
            data->imageWidth(), data->imageHeight(), data->bufferStride());
        checkError();
        drawImageCairo(image, dst, data->imageWidth(), data->imageHeight(),
                       true);
        cairo_surface_destroy(image);
    }

    virtual void drawRepeatImage(ImageData* data, const Unit::Rect& dst,
                                 float imageWidth, float imageHeight,
                                 bool xRepeat, bool yRepeat)
    {
        cairo_save(m_canvas);
        float xx = 0.0, yy = 0.0, ww = 0.0, hh = 0.0;
        ww = dst.width();
        hh = dst.height();

        float x = 0.0, y = 0.0;
        if (xRepeat) {
            x = (dst.x() - floor(dst.x() / imageWidth) * imageWidth) -
                imageWidth;
        } else {
            xx += dst.x();
        }
        if (yRepeat) {
            y = (dst.y() - floor(dst.y() / imageHeight) * imageHeight) -
                imageHeight;
        } else {
            yy += dst.y();
        }

        cairo_pattern_t* pattern;
        cairo_matrix_t matrix;
        cairo_surface_t* image = nullptr;

        void* imgData = data->data();
        double surfaceWidth = 0, surfaceHeight = 0;

        if (imgData) {
            int stride = data->stride();
            image = cairo_image_surface_create_for_data(
                (unsigned char*)imgData, CAIRO_FORMAT, data->width(),
                data->height(), stride);
            surfaceWidth = data->width();
            surfaceHeight = data->height();
        } else {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        }

        pattern = cairo_pattern_create_for_surface(image);
        cairo_matrix_init_scale(&matrix, surfaceWidth / imageWidth,
                                surfaceHeight / imageHeight);
        cairo_matrix_translate(&matrix, -x, -y);

        cairo_pattern_set_matrix(pattern, &matrix);
        cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);

        cairo_translate(m_canvas, xx, yy);
        cairo_set_source(m_canvas, pattern);

        cairo_rectangle(m_canvas, 0, 0, ww, hh);
        cairo_clip(m_canvas);
        cairo_paint(m_canvas);

        cairo_pattern_destroy(pattern);
        cairo_surface_destroy(image);

        cairo_restore(m_canvas);
    }

    virtual void postMatrix(const SkMatrix& matrix)
    {
        cairo_matrix_t result_matrix;
        cairo_matrix_init_identity(&result_matrix);

        cairo_matrix_t a_matrix;
        cairo_matrix_t b_matrix;
        cairo_get_matrix(m_canvas, &a_matrix);
        checkError();
        cairo_matrix_init(&b_matrix, matrix.getScaleX(), matrix.getSkewY(),
                          matrix.getSkewX(), matrix.getScaleY(),
                          matrix.getTranslateX(), matrix.getTranslateY());
        cairo_matrix_multiply(&result_matrix, &b_matrix, &a_matrix);
        cairo_set_matrix(m_canvas, &result_matrix);
        checkError();
    }

    virtual void applyMatrixTo(LayoutLocation& lp)
    {
        double x = lp.x();
        double y = lp.y();
        cairo_matrix_t a_matrix;

        cairo_get_matrix(m_canvas, &a_matrix);
        cairo_matrix_transform_point(&a_matrix, &x, &y);
        lp.setX(x);
        lp.setY(y);
    }

    virtual void applyMatrixTo(LayoutRect& lp)
    {
        double x = lp.x();
        double y = lp.y();
        cairo_matrix_t a_matrix;

        cairo_get_matrix(m_canvas, &a_matrix);
        cairo_matrix_transform_point(&a_matrix, &x, &y);
        lp.setX(x);
        lp.setY(y);
    }

    virtual void resetMatrixAndClip()
    {
        cairo_reset_clip(m_canvas);
        cairo_identity_matrix(m_canvas);
    }

    virtual void resetClip()
    {
        cairo_reset_clip(m_canvas);
    }

protected:
    StarFish* m_starfish;
    std::vector<float> m_opacityVector;
    size_t m_stateSize;
    cairo_surface_t* m_surface;
    cairo_t* m_canvas;
    unsigned m_width;
    unsigned m_height;
    bool m_shouldDestroyCairo;
    bool m_shouldDestroySurface;
};

Compositor* Compositor::create(StarFish* starfish, void* data)
{
    return new CompositorImplCairo(starfish, data);
}

Compositor* Compositor::create(StarFish* starfish, CanvasSurface* surface)
{
    return new CompositorImplCairo(starfish, surface);
}
}

#endif
