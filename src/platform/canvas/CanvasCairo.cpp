/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

// #define STARFISH_ENABLE_PROFILE_TIMER

#include "StarFishConfig.h"
#include "StarFish.h"

#if defined(PORT_CANVAS_BACKEND_CAIRO)

#include "core/modules/canvas/Canvas.h"
#include "core/style/GradientData.h"
#include "core/modules/canvas/font/Font.h"
#include "core/modules/canvas/image/NativeImageData.h"
#include "core/modules/canvas/ShadowBlur.h"
#include "core/style/UnitHelper.h"
#include "platform/window/PlatformWindow.h"

#include "platform/canvas/font/FontImplCairo.h"

#include <vector>
#include <SkMatrix.h>
#include <clipper.hpp>

#include <cairo.h>

#if defined(STARFISH_ANDROID) || defined(STARFISH_WINDOWS)
#include <cairo-ft.h>
#else
#include <cairo/cairo-ft.h>
#endif

#include <hb.h>

#include "core/modules/profiling/Profiling.h"

#define CAIRO_FORMAT CAIRO_FORMAT_ARGB32

namespace StarFish {

extern bool g_enablePixelTest;

class CanvasStateCairo : public CanvasState {
public:
    CanvasStateCairo()
        : CanvasState()
    {
    }
};

class CanvasCairo : public Canvas {
    void initFromBuffer(void* buffer, int width, int height, int stride)
    {
        m_renderTargetInfo.m_width = width;
        m_renderTargetInfo.m_height = height;

        m_surface = cairo_image_surface_create_for_data(
            (unsigned char*)buffer, CAIRO_FORMAT, width, height, stride);
        cairo_surface_set_device_scale(
            m_surface, m_starfish->screenInfo().devicePixelRatio,
            m_starfish->screenInfo().devicePixelRatio);
        m_canvas = cairo_create(m_surface);
    }

    void initFromNativeImageData(NativeImageData* data)
    {
        m_renderTargetInfo.m_width = data->width();
        m_renderTargetInfo.m_height = data->height();

        m_surface = cairo_image_surface_create_for_data(
            (unsigned char*)data->data(), CAIRO_FORMAT, data->width(),
            data->height(), data->stride());
        cairo_surface_set_device_scale(
            m_surface, m_starfish->screenInfo().devicePixelRatio,
            m_starfish->screenInfo().devicePixelRatio);
        m_canvas = cairo_create(m_surface);
    }

    void init()
    {
#ifdef STARFISH_ENABLE_TEST
        cairo_set_antialias(m_canvas, CAIRO_ANTIALIAS_FAST);
#else
        cairo_set_antialias(m_canvas, CAIRO_ANTIALIAS_GOOD);
#endif
    }

public:
    CanvasCairo(StarFish* starfish, void* buffer, int width, int height,
                int stride)
    {
        m_shouldDestroyCairo = true;
        m_shouldDestroySurface = true;
        m_starfish = starfish;
        m_canvas = nullptr;
        m_surface = nullptr;
        {
            initFromBuffer(buffer, width, height, stride);
        }
        init();
        save();
    }

    CanvasCairo(StarFish* starfish, void* data)
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
        m_renderTargetInfo.m_width = d->w;
        m_renderTargetInfo.m_height = d->h;

        m_shouldDestroyCairo = false;
        m_shouldDestroySurface = false;
        init();
        save();
    }

    CanvasCairo(StarFish* starfish, CanvasSurface* data)
    {
        m_starfish = starfish;
        m_canvas = nullptr;
        m_surface = nullptr;
        m_shouldDestroyCairo = true;
        m_shouldDestroySurface = true;

        initFromBuffer(data->data(), data->bufferWidth(), data->bufferHeight(),
                       data->bufferStride());

        init();
        save();
    }

    CanvasCairo(StarFish* starfish, NativeImageData* data)
    {
        m_shouldDestroyCairo = true;
        m_shouldDestroySurface = false;
        m_starfish = starfish;
        m_canvas = nullptr;
        m_surface = nullptr;
        {
            initFromNativeImageData(data);
        }
        init();
        save();
    }

    ~CanvasCairo()
    {
        restore();
        STARFISH_ASSERT(m_state.size() == 0);
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
        INSTALL_PROFILE_TIMER(m_starfish, "CanvasImplCairo::clear");
        cairo_save(m_canvas);
        if (clr.a() == 0) {
            cairo_set_operator(m_canvas, CAIRO_OPERATOR_CLEAR);
        } else {
            cairo_set_source_rgba(m_canvas, clr.R(), clr.G(), clr.B(), clr.A());
            cairo_set_operator(m_canvas, CAIRO_OPERATOR_SOURCE);
        }
        cairo_paint(m_canvas);
        cairo_restore(m_canvas);
    }

    // state
    virtual void save()
    {
        checkError();
        CanvasStateCairo state;
        if (m_state.size()) {
            auto& lastState = m_state.back();
            state.m_color = lastState.m_color;
            state.m_opacity = lastState.m_opacity;
            state.m_font = lastState.m_font;
            state.m_visible = lastState.m_visible;
            state.m_textDecorationData = lastState.m_textDecorationData;
        }
        m_state.push_back(state);
        cairo_save(m_canvas);
    }

    // pop state stack and restore state
    virtual void restore()
    {
        checkError();
        m_state.erase(m_state.end() - 1);
        cairo_restore(m_canvas);
    }

    // transformations (default transform is the identity matrix)
    virtual void scale(double x, double y)
    {
        cairo_scale(m_canvas, x, y);
    }

    virtual void scale(double x, double y, double ox, double oy)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void rotate(double angle)
    {
        cairo_rotate(m_canvas, angle);
    }

    virtual void rotate(double angle, double ox, double oy)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
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
        INSTALL_PROFILE_TIMER(m_starfish, "CanvasImplCairo::beginOpacityLayer");
        save();
        lastState().m_opacity = c;
        cairo_push_group(m_canvas);
    }

    virtual void endOpacityLayer()
    {
        INSTALL_PROFILE_TIMER(m_starfish, "CanvasImplCairo::endOpacityLayer");
        cairo_pop_group_to_source(m_canvas);
        cairo_paint_with_alpha(m_canvas, lastState().m_opacity);
        restore();
    }

    virtual void clip(const Unit::Rect& rt)
    {
        cairo_rectangle(m_canvas, rt.x(), rt.y(), rt.width(), rt.height());
        cairo_clip(m_canvas);
    }

    virtual void pixelSnappedClip(const LayoutRect& rt)
    {
        cairo_matrix_t m;
        cairo_get_matrix(m_canvas, &m);
        double x = rt.x();
        double y = rt.y();

        double maxX = rt.maxX();
        double maxY = rt.maxY();

        cairo_matrix_transform_point(&m, &x, &y);
        cairo_matrix_transform_point(&m, &maxX, &maxY);

        cairo_user_to_device(m_canvas, &x, &y);
        x = floor(x) - 1;
        y = floor(y) - 1;
        cairo_device_to_user(m_canvas, &x, &y);

        cairo_user_to_device(m_canvas, &maxX, &maxY);
        x = ceil(x) + 1;
        y = ceil(y) + 1;
        cairo_device_to_user(m_canvas, &maxX, &maxY);

        cairo_matrix_t m2;
        cairo_matrix_init_identity(&m2);
        cairo_set_matrix(m_canvas, &m2);
        cairo_move_to(m_canvas, x, y);
        cairo_line_to(m_canvas, maxX, y);
        cairo_line_to(m_canvas, maxX, maxY);
        cairo_line_to(m_canvas, x, maxY);
        cairo_line_to(m_canvas, x, y);
        cairo_clip(m_canvas);
        cairo_set_matrix(m_canvas, &m);
    }

    virtual void setColor(const Unit::Color& clr_)
    {
        STARFISH_ASSERT(m_canvas);
        lastState().m_color = clr_;
#ifdef STARFISH_ANDROID
        cairo_set_source_rgba(m_canvas, clr_.B(), clr_.G(), clr_.R(), clr_.A());
#else
        cairo_set_source_rgba(m_canvas, clr_.R(), clr_.G(), clr_.B(), clr_.A());
#endif
    }

    virtual void setVisible(bool visible)
    {
        lastState().m_visible = visible;
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

    virtual void punchHole(const Unit::Rect& rt)
    {
        STARFISH_ASSERT(m_canvas);
        if (!lastState().m_visible) {
            return;
        }
        float xx = rt.x(), yy = rt.y(), ww = rt.width(), hh = rt.height();
        drawCairoRect(xx, yy, ww, hh, true);
    }

    void drawCairoRect(float xx, float yy, float ww, float hh,
                       bool isHole = false)
    {
        INSTALL_PROFILE_TIMER(m_starfish, "CanvasImplCairo::drawCairoRect");
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
        if (!lastState().m_visible) {
            return;
        }
        float xx = rt.x(), yy = rt.y(), ww = rt.width(), hh = rt.height();
        drawCairoRect(xx, yy, ww, hh);
    }

    virtual void drawRect(const LayoutRect& rt)
    {
        STARFISH_ASSERT(m_canvas);
        if (!lastState().m_visible) {
            return;
        }
        int xx = rt.x(), yy = rt.y(), ww = rt.width(), hh = rt.height();
        drawCairoRect(xx, yy, ww, hh);
    }

    virtual void drawRect(LayoutLocation p1, LayoutLocation p2,
                          LayoutLocation p3, LayoutLocation p4)
    {
        if (!lastState().m_visible) {
            return;
        }
        cairo_save(m_canvas);

        cairo_move_to(m_canvas, p1.x(), p1.y());
        cairo_line_to(m_canvas, p2.x(), p2.y());
        cairo_line_to(m_canvas, p3.x(), p3.y());
        cairo_line_to(m_canvas, p4.x(), p4.y());
        cairo_line_to(m_canvas, p1.x(), p1.y());
        cairo_close_path(m_canvas);
        cairo_fill(m_canvas);

        cairo_restore(m_canvas);
    }

    virtual void drawText(LayoutUnit x, LayoutUnit y, LayoutUnit stringWidth,
                          const StringView& sv)
    {
        int size = lastState().m_font->size();
        if (!lastState().m_visible || size == 0 || sv.length() == 0) {
            return;
        }

#if defined(PORT_CANVAS_BACKEND_EFL)
        if (!lastState().m_font->isGenericFont()) {
            Font* nonGenericFont = lastState().m_font;
            auto font = nonGenericFont->fontSelector()->loadFont(
                nullptr, 0, nonGenericFont->size(), nonGenericFont->style(),
                nonGenericFont->weight());
            setFont(font);
        }
#endif

        INSTALL_PROFILE_TIMER(m_starfish, "CanvasImplCairo::drawText");

        LayoutSize sz(stringWidth, lastState().m_font->metrics().m_fontHeight);
        LayoutRect rt(x, y, sz.width(), sz.height());

#ifdef STARFISH_ENABLE_TEST
        if (g_enablePixelTest) {
            drawAhemBoxCairo(m_canvas, rt, sv, rt.x(), rt.y());
        } else {
            drawGlyphsCairo(m_canvas, rt, sv, rt.x(), rt.y());
            drawTextDecorationCairo(m_canvas, rt, sv, rt.x(), rt.y());
        }
#else
        drawGlyphsCairo(m_canvas, rt, sv, rt.x(), rt.y());
        drawTextDecorationCairo(m_canvas, rt, sv, rt.x(), rt.y());
#endif
    }

    void setImageRenderingModeToPattern(cairo_pattern_t* resizePattern,
                                        ImageRenderingValue imageRenderingMode)
    {
        auto anti = cairo_get_antialias(m_canvas);
        cairo_filter_t autoFilterMode;
        if (anti == CAIRO_ANTIALIAS_NONE) {
            autoFilterMode = CAIRO_FILTER_FAST;
        } else {
            autoFilterMode = anti >= CAIRO_ANTIALIAS_GOOD ? CAIRO_FILTER_GOOD
                                                          : CAIRO_FILTER_FAST;
        }
#if defined(STARFISH_ANDROID)
        autoFilterMode = CAIRO_FILTER_FAST;
#endif
        if (imageRenderingMode == ImageRenderingAutoValue) {
            cairo_pattern_set_filter(resizePattern, autoFilterMode);
        } else if (imageRenderingMode == ImageRenderingPixelatedValue) {
            // TODO PixelatedValue should affect when painting bigger image than
            // original only
            cairo_pattern_set_filter(resizePattern, CAIRO_FILTER_GAUSSIAN);
        } else if (imageRenderingMode == ImageRenderingCrispEdgesValue) {
            cairo_pattern_set_filter(resizePattern, CAIRO_FILTER_NEAREST);
        }
    }

    void drawImageCairo(cairo_surface_t* localSurface, const Unit::Rect& dst,
                        double surfaceWidth, double surfaceHeight,
                        ImageRenderingValue imageRenderingMode)
    {
        INSTALL_PROFILE_TIMER(m_starfish, "CanvasImplCairo::drawImageCairo");

        float xx = 0.0, yy = 0.0, ww = 0.0, hh = 0.0;
        xx = dst.x();
        yy = dst.y();
        ww = dst.width();
        hh = dst.height();

        if (ww == 0 || hh == 0)
            return;

        cairo_save(m_canvas);
        cairo_pattern_t* resizePattern;
        cairo_matrix_t matrix;

        resizePattern = cairo_pattern_create_for_surface(localSurface);
        cairo_translate(m_canvas, xx, yy);

        cairo_matrix_init_identity(&matrix);
        cairo_matrix_scale(&matrix, surfaceWidth / ww, surfaceHeight / hh);
        cairo_pattern_set_matrix(resizePattern, &matrix);
        setImageRenderingModeToPattern(resizePattern, imageRenderingMode);
        cairo_pattern_set_extend(resizePattern, CAIRO_EXTEND_PAD);

        cairo_set_source(m_canvas, resizePattern);

        cairo_rectangle(m_canvas, 0, 0, ww, hh);

        if (lastState().m_opacity < 1) {
            cairo_clip(m_canvas);
            cairo_paint_with_alpha(m_canvas, lastState().m_opacity);
        } else {
            cairo_fill(m_canvas);
        }

        // drawDebugLine(xx,yy,ww,hh);
        cairo_pattern_destroy(resizePattern);
        cairo_restore(m_canvas);
    }

    void drawDebugLine(double xx, double yy, double ww, double hh)
    {
        cairo_save(m_canvas);

        cairo_set_source_rgba(m_canvas, 1, 1, 0, 1);
        cairo_rectangle(m_canvas, xx, yy, ww, hh);
        cairo_stroke(m_canvas);

        cairo_restore(m_canvas);
    }

    virtual void drawImage(NativeImageData* data, const Unit::Rect& dst,
                           ImageRenderingValue imageRenderingMode)
    {
        if (!lastState().m_visible) {
            return;
        }

        size_t surfaceWidth = data->width(), surfaceHeight = data->height();

        bool surfaceWasCreated = false;
        cairo_surface_t* image = (cairo_surface_t*)data->internalSurface();
        if (!image) {
            surfaceWasCreated = true;
            image = cairo_image_surface_create_for_data(
                (unsigned char*)data->data(), CAIRO_FORMAT, data->width(),
                data->height(), data->stride());
        }

        drawImageCairo(image, dst, surfaceWidth, surfaceHeight,
                       imageRenderingMode);

        if (surfaceWasCreated) {
            cairo_surface_destroy(image);
        }
    }

    virtual void drawImage(CanvasSurface* data, const Unit::Rect& dst,
                           ImageRenderingValue imageRenderingMode)
    {
        if (!lastState().m_visible) {
            return;
        }
        cairo_surface_t* image;
        image = cairo_image_surface_create_for_data(
            (unsigned char*)data->data(), CAIRO_FORMAT_ARGB32,
            data->imageWidth(), data->imageHeight(), data->bufferStride());

        drawImageCairo(image, dst, data->imageWidth(), data->imageHeight(),
                       imageRenderingMode);
        cairo_surface_destroy(image);
    }

    virtual void drawImage(NativeImageData* data, const Unit::Rect& src,
                           const Unit::Rect& dst,
                           const DrawImageInfo& borderinfo,
                           ImageRenderingValue imageRenderingMode)
    {
        cairo_save(m_canvas);
        if (!lastState().m_visible) {
            return;
        }

        bool surfaceWasCreated = false;
        cairo_surface_t* srcImage = (cairo_surface_t*)data->internalSurface();
        if (!srcImage) {
            surfaceWasCreated = true;
            srcImage = cairo_image_surface_create_for_data(
                (unsigned char*)data->data(), CAIRO_FORMAT, data->width(),
                data->height(), data->stride());
        }

        cairo_surface_t* image = srcImage;
        if (src.x() || src.y() || src.width() != data->width() ||
            src.height() != data->height()) {
            image = cairo_surface_create_for_rectangle(
                srcImage, src.x(), src.y(), src.width(), src.height());
        }

        if (borderinfo.hRepeat == BorderImageRepeatValue::StretchValue &&
            borderinfo.vRepeat == BorderImageRepeatValue::StretchValue) {
            drawImageCairo(image, dst, src.width(), src.height(),
                           imageRenderingMode);
        } else {
            drawRepeatImageCairo(image, dst, src.width(), src.height(),
                                 borderinfo, imageRenderingMode);
        }
        if (surfaceWasCreated) {
            cairo_surface_destroy(srcImage);
        }
        if (srcImage != image) {
            cairo_surface_destroy(image);
        }
        cairo_restore(m_canvas);
    }

    virtual void drawBorderImage(NativeImageData* data, const Unit::Rect& dst,
                                 size_t l, size_t t, size_t r, size_t b,
                                 double scale, bool fill)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        drawImage(data, dst, ImageRenderingAutoValue);
        return;
    }

    virtual void drawRepeatImageCairo(cairo_surface_t* localSurface,
                                      const Unit::Rect& dst, float imageWidth,
                                      float imageHeight,
                                      const DrawImageInfo& borderinfo,
                                      ImageRenderingValue imageRenderingMode)
    {
        if (!lastState().m_visible) {
            return;
        }

        INSTALL_PROFILE_TIMER(m_starfish, "CanvasImplCairo::drawRepeatImage");

        cairo_save(m_canvas);
        double xx = dst.x(), yy = dst.y(), ww = dst.width(), hh = dst.height();
        double x = 0.0, y = 0.0, hScale = borderinfo.hScale,
               vScale = borderinfo.vScale;
        double scaledWidth = imageWidth / borderinfo.hScale;
        double scaledHeight = imageHeight / borderinfo.vScale;

        if (borderinfo.hRepeat == BorderImageRepeatValue::RepeatValue) {
            x = (ww - scaledWidth) / 2;
        } else if (borderinfo.hRepeat == BorderImageRepeatValue::RoundValue) {
            hScale = std::max(1.0, round(ww / scaledWidth));
            hScale = (scaledWidth * hScale) / ww * borderinfo.hScale;
        }
        if (borderinfo.vRepeat == BorderImageRepeatValue::RepeatValue) {
            y = (hh - scaledHeight) / 2;
        } else if (borderinfo.vRepeat == BorderImageRepeatValue::RoundValue) {
            vScale = std::max(1.0, round(hh / scaledHeight));
            vScale = (scaledHeight * vScale) / hh * borderinfo.vScale;
        }

        cairo_pattern_t* pattern;
        cairo_matrix_t matrix;

        cairo_surface_t* image = localSurface;
        if (scaledWidth && scaledHeight) {
            pattern = cairo_pattern_create_for_surface(image);

            cairo_matrix_init_scale(&matrix, hScale, vScale);
            cairo_matrix_translate(&matrix, -x, -y);

            cairo_pattern_set_matrix(pattern, &matrix);
            setImageRenderingModeToPattern(pattern, imageRenderingMode);
            cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);

            cairo_translate(m_canvas, xx, yy);
            cairo_set_source(m_canvas, pattern);

            cairo_rectangle(m_canvas, 0.0, 0.0, ww, hh);

            if (lastState().m_opacity < 1) {
                cairo_clip(m_canvas);
                cairo_paint_with_alpha(m_canvas, lastState().m_opacity);
            } else {
                cairo_fill(m_canvas);
            }

            cairo_pattern_destroy(pattern);
        }
        cairo_restore(m_canvas);
    }

    virtual void drawRepeatImage(NativeImageData* data, const Unit::Rect& dst,
                                 float imageWidth, float imageHeight,
                                 bool xRepeat, bool yRepeat,
                                 ImageRenderingValue imageRenderingMode)
    {
        if (!lastState().m_visible) {
            return;
        }

        INSTALL_PROFILE_TIMER(m_starfish, "CanvasImplCairo::drawRepeatImage");

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

        cairo_surface_t* image = (cairo_surface_t*)data->internalSurface();

        bool surfaceWasCreated = false;
        if (!image) {
            surfaceWasCreated = true;
            image = cairo_image_surface_create_for_data(
                (unsigned char*)data->data(), CAIRO_FORMAT, data->width(),
                data->height(), data->stride());
        }

        double surfaceWidth = data->width(), surfaceHeight = data->height();
        if (surfaceWidth && surfaceHeight) {
            pattern = cairo_pattern_create_for_surface(image);
            cairo_matrix_init_scale(&matrix, surfaceWidth / imageWidth,
                                    surfaceHeight / imageHeight);
            cairo_matrix_translate(&matrix, -x, -y);

            cairo_pattern_set_matrix(pattern, &matrix);
            setImageRenderingModeToPattern(pattern, imageRenderingMode);
            cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);

            cairo_translate(m_canvas, xx, yy);
            cairo_set_source(m_canvas, pattern);

            cairo_rectangle(m_canvas, 0, 0, ww, hh);

            if (lastState().m_opacity < 1) {
                cairo_clip(m_canvas);
                cairo_paint_with_alpha(m_canvas, lastState().m_opacity);
            } else {
                cairo_fill(m_canvas);
            }

            cairo_pattern_destroy(pattern);
        }

        if (surfaceWasCreated) {
            cairo_surface_destroy(image);
        }
        cairo_restore(m_canvas);
    }

    virtual void drawLinearGradient(const Unit::Rect& dst,
                                    GradientDrawingInfo* info)
    {
        STARFISH_ASSERT(m_canvas);
        if (!lastState().m_visible) {
            return;
        }

        cairo_save(m_canvas);

        cairo_pattern_t* pt;
        pt =
            cairo_pattern_create_linear(info->x1, info->y1, info->x2, info->y2);

        size_t size = info->colorStops.size();
        for (size_t i = 0; i < size; ++i) {
            const auto& color = info->colorStops[i]->color();
            const auto& offset = info->colorStops[i]->offset().percent();
            cairo_pattern_add_color_stop_rgba(pt, offset, color.R(), color.G(),
                                              color.B(), color.A());
        }

        cairo_rectangle(m_canvas, dst.x(), dst.y(), dst.width(), dst.height());
        cairo_set_source(m_canvas, pt);
        cairo_fill(m_canvas);
        cairo_pattern_destroy(pt);
        cairo_restore(m_canvas);
    }

    virtual void drawRadialGradient(const Unit::Rect& dst,
                                    GradientDrawingInfo* info)
    {
        STARFISH_ASSERT(m_canvas);
        if (!lastState().m_visible) {
            return;
        }
        cairo_save(m_canvas);
        cairo_rectangle(m_canvas, dst.x(), dst.y(), dst.width(), dst.height());
        cairo_clip(m_canvas);
        cairo_rectangle(m_canvas, dst.x(), dst.y(), dst.width(), dst.height());

        if (info->secondRadius && info->firstRadius > info->secondRadius) {
            info->r2 = info->firstRadius;
            info->y1 = info->y1 * (info->firstRadius / info->secondRadius);
            info->y2 = info->y2 * (info->firstRadius / info->secondRadius);
            cairo_scale(m_canvas, 1,
                        1 * (info->secondRadius / info->firstRadius));
        } else if (info->secondRadius &&
                   info->firstRadius < info->secondRadius) {
            info->r2 = info->secondRadius;
            info->x1 = info->x1 * (info->secondRadius / info->firstRadius);
            info->x2 = info->x2 * (info->secondRadius / info->firstRadius);
            cairo_scale(m_canvas, 1 * (info->firstRadius / info->secondRadius),
                        1);
        }

        cairo_pattern_t* pt;
        pt = cairo_pattern_create_radial(info->x1, info->y1, info->r1, info->x2,
                                         info->y2, info->r2);

        size_t size = info->colorStops.size();
        for (size_t i = 0; i < size; ++i) {
            const auto& color = info->colorStops[i]->color();
            const auto& offset = info->colorStops[i]->offset().percent();
            cairo_pattern_add_color_stop_rgba(pt, offset, color.R(), color.G(),
                                              color.B(), color.A());
        }
        cairo_arc(m_canvas, info->x2, info->y2, info->r2, 0, 2 * M_PI);
        cairo_set_source(m_canvas, pt);
        cairo_fill(m_canvas);
        cairo_pattern_destroy(pt);
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

    virtual void beginPath()
    {
        cairo_new_path(m_canvas);
    }
    virtual void closePath()
    {
        cairo_close_path(m_canvas);
    }
    virtual void moveTo(float x, float y)
    {
        cairo_move_to(m_canvas, x, y);
    }
    virtual void lineTo(float x, float y)
    {
        cairo_line_to(m_canvas, x, y);
    }
    virtual void curveTo(float x1, float y1, float x2, float y2, float x3,
                         float y3)
    {
        cairo_curve_to(m_canvas, x1, y1, x2, y2, x3, y3);
    }
    virtual void quadraticCurveTo(float x1, float y1, float x2, float y2)
    {
        double x0, y0;
        cairo_get_current_point(m_canvas, &x0, &y0);
        cairo_curve_to(m_canvas, 2.0 / 3.0 * x1 + 1.0 / 3.0 * x0,
                       2.0 / 3.0 * y1 + 1.0 / 3.0 * y0,
                       2.0 / 3.0 * x1 + 1.0 / 3.0 * x2,
                       2.0 / 3.0 * y1 + 1.0 / 3.0 * y2, x2, y2);
    }
    virtual void arc(double xc, double yc, double radius, double angle1,
                     double angle2)
    {
        cairo_arc(m_canvas, xc, yc, radius, angle1, angle2);
    }
    virtual void arcNegative(double xc, double yc, double radius, double angle1,
                             double angle2)
    {
        cairo_arc_negative(m_canvas, xc, yc, radius, angle1, angle2);
    }
    virtual void stroke()
    {
        if (!lastState().m_visible) {
            cairo_close_path(m_canvas);
            return;
        }
        cairo_stroke(m_canvas);
    }
    virtual void strokePreserve()
    {
        if (!lastState().m_visible) {
            return;
        }
        cairo_stroke_preserve(m_canvas);
    }
    virtual void fill()
    {
        if (!lastState().m_visible) {
            cairo_close_path(m_canvas);
            return;
        }
        cairo_fill(m_canvas);
    }
    virtual void fillPreserve()
    {
        if (!lastState().m_visible) {
            return;
        }
        cairo_fill_preserve(m_canvas);
    }
    virtual void clipPath()
    {
        cairo_clip(m_canvas);
    }
    virtual void clipPathPreserve()
    {
        cairo_clip_preserve(m_canvas);
    }

    virtual void setFillRule(bool shouldUseNonZeroFillRule)
    {
        if (shouldUseNonZeroFillRule) {
            cairo_set_fill_rule(m_canvas,
                                cairo_fill_rule_t::CAIRO_FILL_RULE_WINDING);
        } else {
            cairo_set_fill_rule(m_canvas,
                                cairo_fill_rule_t::CAIRO_FILL_RULE_EVEN_ODD);
        }
    }

    virtual void setStrokeWidth(float width)
    {
        cairo_set_line_width(m_canvas, width);
    }

    virtual void setDash(double* dashes, int dashCnt, double offset)
    {
        cairo_set_dash(m_canvas, dashes, dashCnt, offset);
    }

    virtual void* unwrap()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
        return NULL;
    }

    CanvasStateCairo& lastState()
    {
        STARFISH_ASSERT(m_state.size());
        return m_state[m_state.size() - 1];
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

private:
#ifdef STARFISH_ENABLE_TEST
    void drawAhemBoxCairo(cairo_t* canvas, LayoutRect rect,
                          const StringView& sv, LayoutUnit dx, LayoutUnit dy)
    {
        if (g_enablePixelTest) {
            LayoutUnit x = rect.x();
            LayoutUnit y = rect.y();

            if (sv.originalString() != String::emptyString &&
                sv.originalString()->charAt(sv.start()) != ' ') {
                float h = lastState().m_font->size();
                float xx = x;
                for (size_t i = sv.start(); i < sv.end(); i++) {
                    char32_t ch = sv.originalString()->charAt(i);
                    if (ch == 160) { // nbsp
                    } else if (String::isFixedWidthChar(ch) ||
                               String::isZeroWidthChar(ch)) {
                        // Fixed-width spaces
                        size_t num = Font::spaceSizeNumerator(ch);
                        xx += h * ((float)num / SPACE_SIZE_DENOMINATOR);
                        continue;
                    } else {
                        if (ch != 'p') {
                            Font* fnt = lastState().m_font;
                            if (fnt->style() == FontStyleItalic) {
                                float offset = h * 0.3;
                                float littleLeft = offset * 0.167;
                                LayoutLocation p1(xx + offset - littleLeft, y);
                                LayoutLocation p2(xx + h + offset - littleLeft,
                                                  y);
                                LayoutLocation p3(xx + h - littleLeft, y + h);
                                LayoutLocation p4(xx - littleLeft, y + h);
                                cairo_save(canvas);
                                cairo_move_to(canvas, p1.x(), p1.y());
                                cairo_line_to(canvas, p2.x(), p2.y());
                                cairo_line_to(canvas, p3.x(), p3.y());
                                cairo_line_to(canvas, p4.x(), p4.y());
                                cairo_line_to(canvas, p1.x(), p1.y());
                                cairo_close_path(canvas);
                                cairo_fill(canvas);
                                cairo_restore(canvas);
                            } else {
                                // left, top, w, h
                                Unit::Rect rt(xx, y, h, h);
                                cairo_rectangle(canvas, xx, y, h, h);
                                cairo_fill(canvas);
                            }
                        } else {
                            int ph = h * 0.2;
                            cairo_rectangle(canvas, xx, y + h - ph, h, ph);
                            cairo_fill(canvas);
                        }
                    }
                    xx += h + lastState().m_font->letterSpacing();
                }
            }
        }
    }
#endif
    void drawGlyphsCairo(cairo_t* canvas, LayoutRect rect, const StringView& sv,
                         LayoutUnit dx, LayoutUnit dy)
    {
        LayoutUnit xBias = 0;
        FT_UInt glyph_index = 0;
        FT_Face lastFontFace = nullptr;
        cairo_font_face_t* fontFace = nullptr;
        FontImplCairo* f = (FontImplCairo*)lastState().m_font;
        int size = f->size();

        FontMetrics fontMetrics = f->metrics();
        cairo_translate(canvas, dx, fontMetrics.m_ascender + dy);
        cairo_glyph_t* glyphs = nullptr;
        size_t glyphCount = 0;
        LayoutUnit letterSpacing = f->letterSpacing();

#define ALLOCA_BIG(bytes, typenameWithoutPointer)                       \
    (typenameWithoutPointer*)(LIKELY(bytes < 1024 * 10) ? alloca(bytes) \
                                                        : GC_MALLOC(bytes))

        if (cairoBackendCanUseSimpleFontPath(f, sv)) {
            LayoutUnit letterSpacingValueSoFar;
            auto stringAccessData = sv.bufferAccessData();
            glyphs = ALLOCA_BIG(stringAccessData.length * sizeof(cairo_glyph_t),
                                cairo_glyph_t);
            for (size_t i = 0; i < stringAccessData.length; i++) {
                std::pair<std::pair<FontFaceImplCairo*, size_t>,
                          std::pair<unsigned, LayoutUnit>>
                    g = cairoBackendInternalLoadGlyph(
                        f, stringAccessData.charAt(i));
                if (g.second.first) {
                    if (true) { // skip webfont enabled
                        if (f->seenUnresolvedWebFontIndex() != SIZE_MAX &&
                            f->seenUnresolvedWebFontIndex() <= g.first.second) {
                            xBias += g.second.second;
                            continue;
                        }
                    }
                    if (lastFontFace != g.first.first->freetypeFace()) {
                        if (fontFace) {
                            cairo_show_glyphs(canvas, glyphs, glyphCount);
                            glyphCount = 0;
                            cairo_font_face_destroy(fontFace);
                        }
                        lastFontFace = g.first.first->freetypeFace();
                        fontFace = cairo_ft_font_face_create_for_ft_face(
                            lastFontFace, 0);
                        cairo_set_font_face(canvas, fontFace);
                        cairo_set_font_size(canvas, size);
                    }
                    glyphs[glyphCount].index = g.second.first;
                    glyphs[glyphCount].x = xBias + letterSpacingValueSoFar;
                    glyphs[glyphCount].y = 0;
                    glyphCount++;
                    STARFISH_ASSERT(glyphCount <= sv.length());
                    letterSpacingValueSoFar += letterSpacingValueSoFar;
                    xBias += g.second.second + letterSpacing;
                } else {
                    if (true) { // skip webfont enabled
                        if (f->seenUnresolvedWebFontIndex() != SIZE_MAX) {
                            xBias += f->spaceWidth();
                            continue;
                        }
                    }
                    cairo_save(canvas);
                    cairo_set_line_width(canvas, 1);
                    cairo_new_path(canvas);
                    cairo_rectangle(canvas, xBias, -fontMetrics.m_ascender,
                                    f->spaceWidth(), fontMetrics.m_fontHeight);
                    cairo_stroke(canvas);
                    cairo_restore(canvas);
                    xBias += f->spaceWidth() + letterSpacing;
                }
            }
        } else {
            auto runs = generateFontCairoTextRuns(&sv, f);

            size_t glyphAllocCount = 0;
            for (size_t i = 0; i < runs.size(); i++) {
                FontCairoTextRun& run = runs[i];
                glyphAllocCount += run.m_glyphs.size();
            }

            glyphs =
                ALLOCA(glyphAllocCount * sizeof(cairo_glyph_t), cairo_glyph_t);

            float xBias = 0;
            for (size_t i = 0; i < runs.size(); i++) {
                FontCairoTextRun& run = runs[i];

                LayoutUnit letterSpacingValueSoFar;
                if (run.m_ftFace == nullptr) {
                    if (/* skip webfont enabled*/ f
                            ->seenUnresolvedWebFontIndex() != SIZE_MAX) {
                    } else {
                        cairo_save(canvas);
                        cairo_set_line_width(canvas, 1);
                        for (size_t j = 0; j < run.m_text.length(); j++) {
                            cairo_new_path(canvas);
                            cairo_rectangle(canvas, xBias + j * f->spaceWidth(),
                                            -fontMetrics.m_ascender,
                                            f->spaceWidth(),
                                            fontMetrics.m_fontHeight);
                            cairo_stroke(canvas);
                        }
                        cairo_restore(canvas);
                    }
                } else {
                    if (/* skip webfont enabled*/ f
                                ->seenUnresolvedWebFontIndex() != SIZE_MAX &&
                        f->seenUnresolvedWebFontIndex() <= run.m_faceIndex) {
                    } else {
                        if (run.m_ftFace != lastFontFace) {
                            if (lastFontFace) {
                                cairo_show_glyphs(canvas, glyphs, glyphCount);
                                glyphCount = 0;
                                cairo_font_face_destroy(fontFace);
                            }
                            lastFontFace = run.m_ftFace;
                            fontFace = cairo_ft_font_face_create_for_ft_face(
                                lastFontFace, 0);
                            cairo_set_font_face(canvas, fontFace);
                            cairo_set_font_size(canvas, size);
                        }

                        for (size_t j = 0; j < run.m_glyphs.size(); j++) {
                            glyphs[glyphCount].index = run.m_glyphs[j];
                            glyphs[glyphCount].x = run.m_glyphPositions[j].x() +
                                                   xBias +
                                                   letterSpacingValueSoFar;
                            letterSpacingValueSoFar += letterSpacing;
                            glyphs[glyphCount].y = run.m_glyphPositions[j].y();
                            STARFISH_ASSERT(glyphCount < glyphAllocCount);
                            glyphCount++;
                        }
                    }
                }

                xBias += (run.m_runWidth + letterSpacingValueSoFar);
            }
        }
        if (glyphCount) {
            cairo_show_glyphs(canvas, glyphs, glyphCount);
        }
        cairo_font_face_destroy(fontFace);
        cairo_translate(canvas, -dx, -dy - fontMetrics.m_ascender);
    }
    void drawTextDecorationCairo(cairo_t* canvas, LayoutRect rect,
                                 const StringView& sv, LayoutUnit dx,
                                 LayoutUnit dy)
    {
        FontImplCairo* f = (FontImplCairo*)lastState().m_font;
        FontFaceImplCairo* fc = (FontFaceImplCairo*)f->fontFaceList()[0];
        FT_Face face = fc->freetypeFace();
        int intSize(f->size() + 0.5f);

        cairo_translate(canvas, dx, dy);

        float lineWidth =
            face->underline_thickness / (float)fc->m_unitsPerEM * intSize;
        if (lastState().m_textDecorationData.hasUnderLine()) {
            cairo_set_line_width(canvas, lineWidth);

#ifdef STARFISH_ANDROID
            cairo_set_source_rgba(
                canvas, lastState().m_textDecorationData.underLineColor().B(),
                lastState().m_textDecorationData.underLineColor().G(),
                lastState().m_textDecorationData.underLineColor().R(),
                lastState().m_textDecorationData.underLineColor().A());
#else
            cairo_set_source_rgba(
                canvas, lastState().m_textDecorationData.underLineColor().R(),
                lastState().m_textDecorationData.underLineColor().G(),
                lastState().m_textDecorationData.underLineColor().B(),
                lastState().m_textDecorationData.underLineColor().A());
#endif

            float y = face->underline_position / (float)fc->m_unitsPerEM *
                          intSize / 72 +
                      intSize;
            cairo_move_to(canvas, 0, y + lineWidth / 2);
            cairo_line_to(canvas, rect.width(), y + lineWidth / 2);
            cairo_stroke(canvas);
        }

        if (lastState().m_textDecorationData.hasLineThrough()) {
            cairo_set_line_width(canvas, lineWidth);

#ifdef STARFISH_ANDROID
            cairo_set_source_rgba(
                canvas, lastState().m_textDecorationData.lineThroughColor().R(),
                lastState().m_textDecorationData.lineThroughColor().G(),
                lastState().m_textDecorationData.lineThroughColor().B(),
                lastState().m_textDecorationData.lineThroughColor().A());
#else
            cairo_set_source_rgba(
                canvas, lastState().m_textDecorationData.lineThroughColor().R(),
                lastState().m_textDecorationData.lineThroughColor().G(),
                lastState().m_textDecorationData.lineThroughColor().B(),
                lastState().m_textDecorationData.lineThroughColor().A());
#endif

            float y =
                (lastState().m_font->metrics().m_ascender) -
                intSize * (lastState().m_font->metrics().m_xheightRate) / 2;

            cairo_move_to(canvas, 0, y);
            cairo_line_to(canvas, rect.width(), y);
            cairo_stroke(canvas);
        }

        cairo_translate(canvas, -dx, -dy);
    }

    virtual bool canRejectPainting(const LayoutRect& rect)
    {
        double x1, x2;
        double y1, y2;
        cairo_clip_extents(m_canvas, &x1, &y1, &x2, &y2);
        LayoutRect c(x1, y1, x2 - x1, y2 - y1);
        if (c.intersects(rect)) {
            return false;
        } else {
            return true;
        }
    }

    virtual void setNeedsNoneAntialias()
    {
        cairo_set_antialias(m_canvas, CAIRO_ANTIALIAS_NONE);
    }

    virtual void setNeedsFastAntialias()
    {
        cairo_set_antialias(m_canvas, CAIRO_ANTIALIAS_FAST);
    }

    virtual void setNeedsGoodQualityAntialias()
    {
        cairo_set_antialias(m_canvas, CAIRO_ANTIALIAS_GOOD);
    }

protected:
    StarFish* m_starfish;
    std::vector<CanvasStateCairo> m_state;
    cairo_surface_t* m_surface;
    cairo_t* m_canvas;

    bool m_shouldDestroyCairo;
    bool m_shouldDestroySurface;
};

#if !defined(PORT_CANVAS_BACKEND_EFL)
Canvas* Canvas::createDirect(StarFish* starfish, void* data)
{
    return new CanvasCairo(starfish, data);
}

Canvas* Canvas::create(StarFish* starfish, CanvasSurface* data)
{
    return new CanvasCairo(starfish, data);
}
#endif

Canvas* Canvas::createGenericCanvas(StarFish* starfish, void* data, size_t w,
                                    size_t h)
{
    return new CanvasCairo(starfish, data, w, h, w * 4);
}

Canvas* Canvas::createGenericCanvas(StarFish* starfish, NativeImageData* data)
{
    return new CanvasCairo(starfish, data);
}
}

#endif
