/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#if defined(PORT_CANVAS_BACKEND_CAIRO)

#if !defined(PORT_PIXEL_ORDER_BGRA)
#error "cairo only supports BGRA order."
#endif

#include "core/style/Style.h"
#include "core/dom/canvas/CanvasGradient.h"
#include "core/dom/canvas/CanvasLineCap.h"
#include "core/dom/canvas/CanvasLineJoin.h"
#include "core/dom/canvas/CanvasPattern.h"
#include "core/dom/canvas/ImageSmoothingQuality.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/font/Font.h"
#include "core/modules/canvas/NativeGradient.h"
#include "core/modules/canvas/image/NativeImageData.h"
#include "core/modules/canvas/image/BufferedNativeImageData.h"
#include "core/modules/canvas/NativePattern.h"
#include "core/modules/canvas/ShadowBlur.h"
#include "core/style/CSSGradientValue.h"
#include "core/style/GradientData.h"
#include "core/style/UnitHelper.h"
#include "core/page/WebView.h"
#include "platform/canvas/font/FontImplCairo.h"

#include <vector>
#include <SkMatrix.h>
#include <clipper.hpp>
#include <cairo.h>

#include "core/modules/canvas/Path.h"
#include "platform/canvas/PathCairo.h"
#include "platform/canvas/CanvasCairoUtils.h"
#include "core/modules/canvas/CanvasShadowData.h"

#if defined(STARFISH_ANDROID) || defined(STARFISH_WINDOWS) || \
    defined(STARFISH_TIZEN)
#include <cairo-ft.h>
#else
#include <cairo/cairo-ft.h>
#endif

#include <hb.h>

#include "core/modules/profiling/Profiling.h"

#define CAIRO_FORMAT CAIRO_FORMAT_ARGB32

namespace Starfish {

class FontFaceReferenceHolder {
public:
    FontFaceReferenceHolder(FontFaceImplCairo* fontFace)
        : storage(fontFace)
    {
    }

    bool addToCairoFontFace(cairo_font_face_t* cairofontFace)
    {
        static cairo_user_data_key_t g_key;

        cairo_status_t status = cairo_font_face_set_user_data(
            cairofontFace, &g_key, this,
            (cairo_destroy_func_t)removeFontFaceReference);

        return status == CAIRO_STATUS_SUCCESS;
    }

    FontFaceImplCairo* storage{ nullptr };

private:
    // cairo_font_face_set_user_data callback;
    static void removeFontFaceReference(void* data)
    {
        FontFaceReferenceHolder* holder = (FontFaceReferenceHolder*)data;
        GC_FREE(holder);
    }
};

class NativeGradientCairo : public NativeGradient {
public:
    NativeGradientCairo(GradientDrawingInfo* info)
        : NativeGradient(info)
        , m_pattern(nullptr)
    {
        initialize(info);
    }

    NativeGradientCairo(double x0, double y0, double x1, double y1)
        : NativeGradient()
        , m_pattern(nullptr)
    {
        initializePatternToLinearGradient(x0, y0, x1, y1);
    }

    NativeGradientCairo(double x0, double y0, double r0, double x1, double y1,
                        double r1)
        : NativeGradient()
        , m_pattern(nullptr)
    {
        initializePatternToRadialGradient(x0, y0, r0, x1, y1, r1);
    }

    ~NativeGradientCairo()
    {
        cairo_pattern_destroy(m_pattern);
    }

    virtual void addColorStop(const double& offset,
                              const Unit::Color& color) override
    {
        cairo_pattern_add_color_stop_rgba(m_pattern, offset, color.R(),
                                          color.G(), color.B(), color.A());
    }

    virtual bool isZeroSize() override
    {
        auto type = cairo_pattern_get_type(m_pattern);
        if (type == CAIRO_PATTERN_TYPE_LINEAR) {
            double x0, y0, x1, y1;
            cairo_pattern_get_linear_points(m_pattern, &x0, &y0, &x1, &y1);
            return (x0 == x1) && (y0 == y1);
        } else {
            STARFISH_ASSERT(type == CAIRO_PATTERN_TYPE_RADIAL);
            double x0, y0, r0, x1, y1, r1;
            cairo_pattern_get_radial_circles(m_pattern, &x0, &y0, &r0, &x1, &y1,
                                             &r1);
            return (x0 == x1) && (y0 == y1) && (r0 == r1);
        }
        return false;
    }

    cairo_pattern_t* pattern()
    {
        return m_pattern;
    }

private:
    void initialize(GradientDrawingInfo* info)
    {
        STARFISH_ASSERT(info != nullptr);

        if (info->type == GradientType::LinearGradient) {
            initializePatternToLinearGradient(info->x1, info->y1, info->x2,
                                              info->y2);
        } else if (info->type == GradientType::RadialGradient) {
            initializePatternToRadialGradient(info->x1, info->y1, info->r1,
                                              info->x2, info->y2, info->r2);
        } else {
            STARFISH_UNSUPPORTED("Canvas: unsupported gradient type");
        }

        size_t size = info->colorStops.size();
        for (size_t i = 0; i < size; ++i) {
            const auto& color = info->colorStops[i]->color();
            const auto& offset = info->colorStops[i]->offset().percent();
            addColorStop(offset, color);
        }
    }

    void initializePatternToLinearGradient(double x0, double y0, double x1,
                                           double y1)
    {
        m_pattern = cairo_pattern_create_linear(x0, y0, x1, y1);
        STARFISH_ASSERT(cairo_pattern_status(m_pattern) ==
                        CAIRO_STATUS_SUCCESS);
    }

    void initializePatternToRadialGradient(double x0, double y0, double r0,
                                           double x1, double y1, double r1)
    {
        m_pattern = cairo_pattern_create_radial(x0, y0, r0, x1, y1, r1);
        STARFISH_ASSERT(cairo_pattern_status(m_pattern) ==
                        CAIRO_STATUS_SUCCESS);
    }

    cairo_pattern_t* m_pattern;
};

std::shared_ptr<NativeGradient> NativeGradient::create(
    GradientDrawingInfo* info)
{
    return std::shared_ptr<NativeGradient>(new NativeGradientCairo(info));
}

std::shared_ptr<NativeGradient> NativeGradient::create(double x0, double y0,
                                                       double x1, double y1)
{
    return std::shared_ptr<NativeGradient>(
        new NativeGradientCairo(x0, y0, x1, y1));
}

std::shared_ptr<NativeGradient> NativeGradient::create(double x0, double y0,
                                                       double r0, double x1,
                                                       double y1, double r1)
{
    return std::shared_ptr<NativeGradient>(
        new NativeGradientCairo(x0, y0, r0, x1, y1, r1));
}

class NativePatternCairo : public NativePattern {
public:
    NativePatternCairo(NULLABLE NativeImageData* image, bool repeatX,
                       bool repeatY)
        : NativePattern(image, repeatX, repeatY)
        , m_pattern(nullptr)
    {
        initialize();
    }

    ~NativePatternCairo()
    {
        if (m_pattern != nullptr) {
            cairo_pattern_destroy(m_pattern);
        }
    }

    cairo_pattern_t* pattern()
    {
        return m_pattern;
    }

protected:
    virtual void applyTransform()
    {
        cairo_matrix_t matrix;
        cairo_matrix_init_identity(&matrix);
        cairo_matrix_init(&matrix, m_matrix.getScaleX(), m_matrix.getSkewY(),
                          m_matrix.getSkewX(), m_matrix.getScaleY(),
                          m_matrix.getTranslateX(), m_matrix.getTranslateY());
        cairo_matrix_invert(&matrix);
        cairo_pattern_set_matrix(m_pattern, &matrix);
    }

private:
    void initialize()
    {
        if (m_nativeImage == nullptr) {
            return;
        }

        bool surfaceWasCreated = false;
        cairo_surface_t* surface = (cairo_surface_t*)m_nativeImage->unwrap();

        if (surface == nullptr &&
            (m_nativeImage->isAttachableNativeImage() == false)) {
            surface = cairo_image_surface_create_for_data(
                (unsigned char*)m_nativeImage->data(), CAIRO_FORMAT,
                m_nativeImage->width(), m_nativeImage->height(),
                m_nativeImage->stride());
            surfaceWasCreated = true;
        } else if (surface != nullptr &&
                   m_nativeImage->isAttachableNativeImage() == true) {
            auto format = cairo_image_surface_get_format(surface);
            auto width = cairo_image_surface_get_width(surface);
            auto height = cairo_image_surface_get_height(surface);
            auto surfaceToCopy = cairo_surface_create_similar_image(
                surface, format, width, height);

            auto context = cairo_create(surfaceToCopy);
            cairo_set_source_surface(context, surface, 0, 0);
            cairo_rectangle(context, 0, 0, width, height);
            cairo_fill(context);
            cairo_surface_flush(surfaceToCopy);
            cairo_destroy(context);

            surface = surfaceToCopy;
            surfaceWasCreated = true;
        }

        m_pattern = cairo_pattern_create_for_surface(surface);
        cairo_pattern_set_extend(m_pattern, CAIRO_EXTEND_REPEAT);

        if (surfaceWasCreated == true) {
            cairo_surface_destroy(surface);
        }

        STARFISH_ASSERT(cairo_pattern_status(m_pattern) ==
                        CAIRO_STATUS_SUCCESS);
    }

    cairo_pattern_t* m_pattern;
};

std::shared_ptr<NativePattern> NativePattern::create(
    NULLABLE NativeImageData* image, bool repeatX, bool repeatY)
{
    return std::shared_ptr<NativePattern>(
        new NativePatternCairo(image, repeatX, repeatY));
}

class CanvasCairo : public Canvas {
    friend class CanvasAttachableNativeImageCairo;
    void initFromBuffer(void* buffer, int width, int height, int stride,
                        float devicePixelRatio)
    {
        STARFISH_ASSERT(buffer != nullptr);

        m_renderTargetInfo.m_buffer = (uint8_t*)buffer;
        m_renderTargetInfo.m_width = width;
        m_renderTargetInfo.m_height = height;
        m_renderTargetInfo.m_stride = stride;
        m_renderTargetInfo.m_devicePixelRatio = devicePixelRatio;

        m_surface = cairo_image_surface_create_for_data(
            (unsigned char*)buffer, CAIRO_FORMAT, width, height, stride);
        m_canvas = cairo_create(m_surface);

        applyDevicePixelRatio();
    }

    void initFromNativeImageData(NativeImageData* data)
    {
        STARFISH_ASSERT(data != nullptr);

        m_renderTargetInfo.m_buffer = (uint8_t*)data->data();
        m_renderTargetInfo.m_width = data->width();
        m_renderTargetInfo.m_height = data->height();
        m_renderTargetInfo.m_stride = data->stride();

        m_surface = cairo_image_surface_create_for_data(
            (unsigned char*)data->data(), CAIRO_FORMAT, data->width(),
            data->height(), data->stride());
        m_canvas = cairo_create(m_surface);

        applyDevicePixelRatio();
    }

    void init()
    {
#ifdef STARFISH_ENABLE_TEST
        cairo_set_antialias(m_canvas, CAIRO_ANTIALIAS_FAST);
#else
        cairo_set_antialias(m_canvas, CAIRO_ANTIALIAS_GOOD);
#endif
    }

    void applyCanvasFillStrokeSourceIfNeeds(bool useStrokeSource = false)
    {
        if (m_shouldApplyCanvasFillStrokeSource == false) {
            return;
        }

        CanvasFillStrokeSource* source = nullptr;
        if (useStrokeSource == true) {
            source = lastState()->m_strokeSource;
        } else {
            source = lastState()->m_fillSource;
        }
        STARFISH_ASSERT(source != nullptr);

        if (source->isColorType() == true) {
            Unit::Color clr = source->getColorValue();
            cairo_set_source_rgba(m_canvas, clr.R(), clr.G(), clr.B(),
                                  clr.A() * globalAlpha());
        } else if (source->isCanvasStyleType() == true) {
            auto canvasStyle = source->getCanvasStyleValue();
            if (canvasStyle.isCanvasGradientValue() == true) {
                auto gradientValue =
                    canvasStyle.getCanvasGradientValue()->nativeGradient();

                GradientDrawingInfo* gradientDrawingInfo =
                    gradientValue->gradientDrawingInfo();
                if (gradientDrawingInfo) {
                    float xScale = 1;
                    float yScale = 1;
                    if (gradientDrawingInfo->secondRadius != 0 &&
                        gradientDrawingInfo->firstRadius >
                            gradientDrawingInfo->secondRadius) {
                        xScale = 1;
                        yScale = gradientDrawingInfo->secondRadius /
                                 gradientDrawingInfo->firstRadius;
                    } else if (gradientDrawingInfo->secondRadius != 0 &&
                               gradientDrawingInfo->firstRadius <
                                   gradientDrawingInfo->secondRadius) {
                        xScale = gradientDrawingInfo->firstRadius /
                                 gradientDrawingInfo->secondRadius;
                        yScale = 1;
                    }
                    cairo_scale(m_canvas, xScale, yScale);
                }
                cairo_set_source(
                    m_canvas,
                    ((NativeGradientCairo*)gradientValue.get())->pattern());

            } else if (canvasStyle.isCanvasPatternValue() == true) {
                auto nativePattern =
                    (NativePatternCairo*)(canvasStyle.getCanvasPatternValue()
                                              ->nativePattern()
                                              .get());
                if (nativePattern->isEmpyPattern() == true) {
                    return;
                }

                auto pattern = nativePattern->pattern();

                cairo_surface_t* surface = nullptr;
                auto status = cairo_pattern_get_surface(pattern, &surface);

                STARFISH_ASSERT(status == CAIRO_STATUS_SUCCESS);
                STARFISH_ASSERT(surface != nullptr);

                ImageRenderingValue imageRenderingValue = toImageRenderingValue(
                    imageSmoothingEnabled(), imageSmoothingQuality());
                setImageRenderingModeToPattern(pattern, imageRenderingValue);

                cairo_set_source(m_canvas, pattern);

                auto width = cairo_image_surface_get_width(surface);
                auto height = cairo_image_surface_get_height(surface);
                auto currentPath = cairo_copy_path(m_canvas);
                cairo_new_path(m_canvas);

                double x1, y1, x2, y2;
                cairo_clip_extents(m_canvas, &x1, &y1, &x2, &y2);
                Unit::Rect clipRect(x1, y1, x2 - x1, y2 - y1);
                Unit::Rect patternRect(0, 0, width, height);

                bool repeatX = nativePattern->repeatX();
                bool repeatY = nativePattern->repeatY();

                if (repeatX == false) {
                    clipRect.setX(patternRect.x());
                    clipRect.setWidth(patternRect.width());
                }
                if (repeatY == false) {
                    clipRect.setY(patternRect.y());
                    clipRect.setHeight(patternRect.height());
                }
                if (repeatX == false || repeatY == false) {
                    cairo_rectangle(m_canvas, clipRect.x(), clipRect.y(),
                                    clipRect.width(), clipRect.height());
                    cairo_clip(m_canvas);
                }
                cairo_append_path(m_canvas, currentPath);
                cairo_path_destroy(currentPath);
            } else {
                STARFISH_ASSERT(canvasStyle.isDOMStringValue() ||
                                canvasStyle.isNoneValue());
                return;
            }
        }
    }

public:
    CanvasCairo(void* buffer, int width, int height, int stride,
                float devicePixelRatio)
    {
        STARFISH_ASSERT(buffer != nullptr);

        m_shouldDestroyCairo = true;
        m_shouldDestroySurface = true;
        m_shouldApplyCanvasFillStrokeSource = false;
        m_canvas = nullptr;
        m_surface = nullptr;
        initFromBuffer(buffer, width, height, stride, devicePixelRatio);

        STARFISH_ASSERT(m_canvas != nullptr);
        STARFISH_ASSERT(m_surface != nullptr);

        init();
        save();
    }

    CanvasCairo(WebView* webView, CanvasSurface* data, CanvasFlag flag)
    {
        STARFISH_ASSERT(webView != nullptr);
        STARFISH_ASSERT(data != nullptr);

        m_canvas = nullptr;
        m_surface = nullptr;
        m_shouldDestroyCairo = true;
        m_shouldDestroySurface = true;
        m_shouldApplyCanvasFillStrokeSource = false;
        m_flag = flag;
        m_targetSurface = data;

        initFromBuffer(data->mapBuffer(), data->bufferWidth(),
                       data->bufferHeight(), data->bufferStride(),
                       webView->screenInfo().devicePixelRatio);

        STARFISH_ASSERT(m_canvas != nullptr);
        STARFISH_ASSERT(m_surface != nullptr);

        init();
        save();
    }

    CanvasCairo(WebView* webView, NativeImageData* data)
    {
        STARFISH_ASSERT(webView != nullptr);
        STARFISH_ASSERT(data != nullptr);

        m_shouldDestroyCairo = true;
        m_shouldDestroySurface = true;
        m_shouldApplyCanvasFillStrokeSource = false;
        m_renderTargetInfo.m_devicePixelRatio =
            webView->screenInfo().devicePixelRatio;
        m_canvas = nullptr;
        m_surface = nullptr;
        initFromNativeImageData(data);

        STARFISH_ASSERT(m_canvas != nullptr);
        STARFISH_ASSERT(m_surface != nullptr);

        init();
        save();
    }

    ~CanvasCairo()
    {
        while (m_state.size() != 0) {
            restore();
        }
        STARFISH_ASSERT(m_state.size() == 0);
        if (m_shouldDestroyCairo == true) {
            cairo_destroy(m_canvas);
        }
        cairo_surface_flush(m_surface);
        if (m_shouldDestroySurface == true) {
            cairo_surface_destroy(m_surface);
        }
    }

    void checkError()
    {
#ifndef NDEBUG
        auto status = cairo_status(m_canvas);
        if (status != CAIRO_STATUS_SUCCESS) {
            STARFISH_LOG_ERROR("%s", cairo_status_to_string(status));
            STARFISH_ASSERT_NOT_REACHED();
        }
#endif
    }

    virtual void clearColor(const Unit::Color& clr) override
    {
        INSTALL_PROFILE_TIMER("CanvasImplCairo::clear");
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

    virtual void flush() override
    {
        cairo_surface_flush(m_surface);
    }

    // state
    virtual void save() override
    {
        checkError();
        Canvas::save();
        cairo_save(m_canvas);
    }

    // pop state stack and restore state
    virtual void restore() override
    {
        checkError();
        if (lastState()->m_maskPattern) {
            cairo_pop_group_to_source(m_canvas);

            auto pattern =
                reinterpret_cast<cairo_pattern_t*>(lastState()->m_maskPattern);

            cairo_matrix_t matrix;
            cairo_get_matrix(m_canvas, &matrix);
            {
                cairo_matrix_t maskMatrix;
                SkMatrix matrix = lastState()->m_maskTM;
                cairo_matrix_init(&maskMatrix, matrix.getScaleX(),
                                  matrix.getSkewY(), matrix.getSkewX(),
                                  matrix.getScaleY(), matrix.getTranslateX(),
                                  matrix.getTranslateY());
                cairo_set_matrix(m_canvas, &maskMatrix);
            }

            cairo_mask(m_canvas, pattern);
            cairo_pattern_destroy(pattern);
            cairo_set_matrix(m_canvas, &matrix);

            if (lastState()->m_shouldRemoveImmediately) {
                cairo_surface_flush(m_surface);
                delete ((NativeImageData*)lastState()->m_maskPatternData);
            }

            lastState()->m_maskPatternData = nullptr;
            lastState()->m_maskPattern = nullptr;
        }
        Canvas::restore();
        cairo_restore(m_canvas);
    }

    // transformations (default transform is the identity matrix)
    virtual void scale(double x, double y) override
    {
        cairo_scale(m_canvas, x, y);
    }

    void scale(double x, double y, double ox, double oy)
    {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    virtual void rotate(double angle) override
    {
        cairo_rotate(m_canvas, angle);
    }

    void rotate(double angle, double ox, double oy)
    {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    virtual void translate(double x, double y) override
    {
        cairo_translate(m_canvas, x, y);
    }

    virtual void translate(LayoutUnit x, LayoutUnit y) override
    {
        translate(x.toDouble(), y.toDouble());
    }

    virtual void beginLayer(const Unit::Rect& subCanvasRect, float layerOpacity,
                            CanvasLayerMode mode) override
    {
        save();
        clip(subCanvasRect);
        lastState()->m_layerRect = subCanvasRect;
        lastState()->m_layerMode = mode;
        lastState()->m_layerOpacity =
            std::max<float>(0, std::min<float>(1.0, layerOpacity));
        cairo_push_group(m_canvas);
    }

    virtual void endLayer(LayerPixelModifyFunction fn) override
    {
        INSTALL_PROFILE_TIMER("CanvasImplCairo::endLayer");

        checkError();
        if (fn) {
            auto groupTarget = cairo_get_group_target(m_canvas);
            cairo_surface_flush(groupTarget);
            cairo_surface_t* mappedSurface =
                cairo_surface_map_to_image(groupTarget, NULL);
            Optional<uint8_t*> ptr = static_cast<uint8_t*>(
                cairo_image_surface_get_data(mappedSurface));
            size_t width = static_cast<size_t>(
                cairo_image_surface_get_width(mappedSurface));
            size_t stride = static_cast<size_t>(
                cairo_image_surface_get_stride(mappedSurface));
            size_t height = static_cast<size_t>(
                cairo_image_surface_get_height(mappedSurface));
            // the ptr can be null when width or height are zero
            if (ptr) {
                fn(ptr.value(), width, stride, height);
            }
            cairo_surface_unmap_image(groupTarget, mappedSurface);
        }

        if (lastState()->m_layerMode == CanvasLayerMode::Mask) {
            auto pattern = cairo_pop_group(m_canvas);
            // only 1.0 support here
            STARFISH_ASSERT(lastState()->m_layerOpacity == 1);
            auto subCanvasRect = lastState()->m_layerRect;
            restore();

            cairo_matrix_t matrix;
            cairo_get_matrix(m_canvas, &matrix);
            SkMatrix m = SkMatrix::I();
            m.set(0, matrix.xx);
            m.set(1, matrix.yx);
            m.set(2, matrix.x0);
            m.set(3, matrix.xy);
            m.set(4, matrix.yy);
            m.set(5, matrix.y0);
            lastState()->m_maskTM = m;
            lastState()->m_maskPattern = pattern;

            clip(subCanvasRect);
            cairo_push_group(m_canvas);
        } else {
            cairo_pop_group_to_source(m_canvas);
            cairo_paint_with_alpha(m_canvas, lastState()->m_layerOpacity);
            restore();
        }
    }

    virtual void clip(const Unit::Rect& rt) override
    {
        cairo_rectangle(m_canvas, rt.x(), rt.y(), rt.width(), rt.height());
        cairo_clip(m_canvas);
    }

    virtual void clipPath(Path* path) override
    {
        STARFISH_ASSERT(path != nullptr);

        setPathAsNewPathOnCurrentContext(path);
        clipPath();
    }

    virtual LayoutRect pixelSnappedClip(const LayoutRect& rt) override
    {
        if (rt.width() == 0 || rt.height() == 0) {
            clip(Unit::Rect(0, 0, 0, 0));
            return LayoutRect(0, 0, 0, 0);
        }
        LayoutRect deviceRect;
        cairo_matrix_t m;
        cairo_get_matrix(m_canvas, &m);
        double xSoFar = rt.x();
        double ySoFar = rt.y();
        double maxXSoFar = rt.maxX();
        double maxYSoFar = rt.maxY();

        cairo_matrix_transform_point(&m, &xSoFar, &ySoFar);
        cairo_matrix_transform_point(&m, &maxXSoFar, &maxYSoFar);

        double x = std::min(xSoFar, maxXSoFar);
        double y = std::min(ySoFar, maxYSoFar);
        double maxX = std::max(xSoFar, maxXSoFar);
        double maxY = std::max(ySoFar, maxYSoFar);

        int ix, iy, iMaxX, iMaxY;
        ix = floor(x);
        iy = floor(y);
        iMaxX = ceil(maxX);
        iMaxY = ceil(maxY);

        if (ix < 0) {
            ix = 0;
        } else if (ix > (int)m_renderTargetInfo.m_width) {
            iMaxX = ix = m_renderTargetInfo.m_width;
        }
        if (iy < 0) {
            iy = 0;
        } else if (iy > (int)m_renderTargetInfo.m_height) {
            iMaxY = iy = m_renderTargetInfo.m_height;
        }

        if (iMaxX < 0) {
            iMaxX = 0;
        } else if (iMaxX > (int)m_renderTargetInfo.m_width) {
            iMaxX = m_renderTargetInfo.m_width;
        }
        if (iMaxY < 0) {
            iMaxY = 0;
        } else if (iMaxY > (int)m_renderTargetInfo.m_height) {
            iMaxY = m_renderTargetInfo.m_height;
        }

        deviceRect.setX(ix);
        deviceRect.setY(iy);
        deviceRect.setWidth(iMaxX - ix);
        deviceRect.setHeight(iMaxY - iy);

        cairo_matrix_t m2;
        cairo_matrix_init_identity(&m2);
        cairo_set_matrix(m_canvas, &m2);
        cairo_move_to(m_canvas, ix, iy);
        cairo_line_to(m_canvas, iMaxX, iy);
        cairo_line_to(m_canvas, iMaxX, iMaxY);
        cairo_line_to(m_canvas, ix, iMaxY);
        cairo_line_to(m_canvas, ix, iy);
        cairo_clip(m_canvas);
        cairo_set_matrix(m_canvas, &m);

        return deviceRect;
    }

    virtual void unsetDevicePixelRatio() override
    {
        float dpr = m_renderTargetInfo.m_devicePixelRatio;
        if (m_targetSurface) {
            dpr *= m_targetSurface->additionalPixelRatio();
        }

        scale(1 / dpr, 1 / dpr);
    }

    void applyDevicePixelRatio()
    {
        float dpr = m_renderTargetInfo.m_devicePixelRatio;
        if (m_targetSurface) {
            dpr *= m_targetSurface->additionalPixelRatio();
        }
        scale(dpr, dpr);
    }

    virtual void setFillColor(const Unit::Color& clr) override
    {
        setFillSource(new CanvasFillStrokeSource(clr));
    }

    virtual void setFillSource(CanvasFillStrokeSource* source) override
    {
        STARFISH_ASSERT(source != nullptr);

        if (source->isCanvasAvailableSource() == false) {
            return;
        }
        lastState()->m_fillSource = source;
        m_shouldApplyCanvasFillStrokeSource = true;
    }

    virtual CanvasFillStrokeSource* fillSource() override
    {
        return lastState()->m_fillSource;
    }

    virtual void setStrokeColor(const Unit::Color& clr) override
    {
        setStrokeSource(new CanvasFillStrokeSource(clr));
    }

    virtual void setStrokeSource(CanvasFillStrokeSource* source) override
    {
        STARFISH_ASSERT(source != nullptr);

        if (source->isCanvasAvailableSource() == false) {
            return;
        }
        lastState()->m_strokeSource = source;
        m_shouldApplyCanvasFillStrokeSource = true;
    }

    virtual CanvasFillStrokeSource* strokeSource() override
    {
        return lastState()->m_strokeSource;
    }

    virtual void setGlobalAlpha(float c) override
    {
        lastState()->m_globalAlpha =
            std::max<float>(0, std::min<float>(1.0, c));
        m_shouldApplyCanvasFillStrokeSource = true;
    }

    virtual float globalAlpha() override
    {
        return lastState()->m_globalAlpha;
    }

    virtual void setCompositeOperator(CanvasCompositeOperator oper,
                                      BlendMode mode) override
    {
        lastState()->m_compositeOperator = oper;
        lastState()->m_blendMode = mode;

        cairo_operator_t newOperator = CAIRO_OPERATOR_OVER;

        // Source from webkit project:
        // Source/WebCore/platform/graphics/cairo/CairoUtilities.cpp :
        // toCairoOperator,toCairoCompositeOperator
        if (lastState()->m_blendMode != BlendMode::Normal) {
            newOperator = CanvasCairoUtils::blendModeToCairoOperator(
                lastState()->m_blendMode);
        } else {
            switch (lastState()->m_compositeOperator) {
            case CanvasCompositeOperator::Clear:
                newOperator = CAIRO_OPERATOR_CLEAR;
                break;
            case CanvasCompositeOperator::Copy:
                newOperator = CAIRO_OPERATOR_SOURCE;
                break;
            case CanvasCompositeOperator::SourceOver:
                newOperator = CAIRO_OPERATOR_OVER;
                break;
            case CanvasCompositeOperator::SourceIn:
                newOperator = CAIRO_OPERATOR_IN;
                break;
            case CanvasCompositeOperator::SourceOut:
                newOperator = CAIRO_OPERATOR_OUT;
                break;
            case CanvasCompositeOperator::SourceAtop:
                newOperator = CAIRO_OPERATOR_ATOP;
                break;
            case CanvasCompositeOperator::DestinationOver:
                newOperator = CAIRO_OPERATOR_DEST_OVER;
                break;
            case CanvasCompositeOperator::DestinationIn:
                newOperator = CAIRO_OPERATOR_DEST_IN;
                break;
            case CanvasCompositeOperator::DestinationOut:
                newOperator = CAIRO_OPERATOR_DEST_OUT;
                break;
            case CanvasCompositeOperator::DestinationAtop:
                newOperator = CAIRO_OPERATOR_DEST_ATOP;
                break;
            case CanvasCompositeOperator::Lighter:
                newOperator = CAIRO_OPERATOR_ADD;
                break;
            case CanvasCompositeOperator::XOR:
                newOperator = CAIRO_OPERATOR_XOR;
                break;
            // At this moment, chrome didn't support following spec.
            // case CanvasCompositeOperator::PlusDarker:
            //     newOperator = CAIRO_OPERATOR_DARKEN;
            //     break;
            // case CanvasCompositeOperator::PlusLighter:
            //     newOperator = CAIRO_OPERATOR_ADD;
            //     break;
            default:
                newOperator = CAIRO_OPERATOR_SOURCE;
                break;
            }
        }

        cairo_set_operator(m_canvas, newOperator);
    }

    virtual CanvasCompositeOperator compositeOperator() override
    {
        return lastState()->m_compositeOperator;
    }

    virtual BlendMode blendMode() override
    {
        return lastState()->m_blendMode;
    }

    virtual void setVisible(bool visible) override
    {
        lastState()->m_visible = visible;
    }

    virtual void setNonInvertableCTM(bool validation) override
    {
        lastState()->m_hasNonInvertableCTM = validation;
    }

    virtual bool hasNonInvertableCTM() override
    {
        return lastState()->m_hasNonInvertableCTM;
    }

    virtual void setPathTransformMatrix(const SkMatrix& matrix) override
    {
        lastState()->m_pathTM = matrix;
    }

    virtual SkMatrix pathTransformMatrix() override
    {
        return lastState()->m_pathTM;
    }

    virtual void setOriginalFontStr(String* fontStr)
    {
        STARFISH_ASSERT(fontStr != nullptr);
        lastState()->m_canvasFontOrginalStr = fontStr;
    }

    virtual void setCanvasWebFontState(size_t version)
    {
        lastState()->m_canvasFontState = version;
    }

    virtual size_t canvasWebFontState()
    {
        return lastState()->m_canvasFontState;
    }

    virtual Font* font()
    {
        return lastState()->m_font;
    }

    virtual String* originalFontStr()
    {
        return lastState()->m_canvasFontOrginalStr;
    }

    virtual void setCanvasTextAlign(CanvasTextAlign textAlign)
    {
        lastState()->m_canvasTextAlign = textAlign;
    }

    virtual CanvasTextAlign canvasTextAlign()
    {
        return lastState()->m_canvasTextAlign;
    }

    virtual void setCanvasTextBaseline(CanvasTextBaseline textBaseline)
    {
        lastState()->m_canvasTextBaseline = textBaseline;
    }

    virtual CanvasTextBaseline canvasTextBaseline()
    {
        return lastState()->m_canvasTextBaseline;
    }

    virtual void setCanvasTextDirection(CanvasDirection textDirection)
    {
        lastState()->m_canvasDirection = textDirection;
    }

    virtual CanvasDirection canvasTextDirection()
    {
        return lastState()->m_canvasDirection;
    }

    virtual void setFont(Font* font) override
    {
        STARFISH_ASSERT(font != nullptr);
        lastState()->m_font = font;
    }

    virtual void resetTextDecorationData() override
    {
        lastState()->m_textDecorationData.reset();
    }

    virtual void mergeTextDecorationData(ComputedStyle* style) override
    {
        STARFISH_ASSERT(style != nullptr);
        lastState()->m_textDecorationData.merge(style);
    }

    virtual TextDecorationData textDecorationData() override
    {
        return lastState()->m_textDecorationData;
    }

    virtual void setTextDecorationData(TextDecorationData d) override
    {
        lastState()->m_textDecorationData = d;
    }

    virtual void punchHole(const Unit::Rect& rt) override
    {
        if (lastState()->m_visible == false) {
            return;
        }
        float xx = rt.x(), yy = rt.y(), ww = rt.width(), hh = rt.height();
        drawCairoRect(xx, yy, ww, hh, true);
    }

    void drawCairoRect(float xx, float yy, float ww, float hh,
                       bool isHole = false)
    {
        INSTALL_PROFILE_TIMER("CanvasImplCairo::drawCairoRect");
        cairo_save(m_canvas);
        if (isHole) {
            cairo_set_source_rgba(m_canvas, 0, 0, 0, 0);
            cairo_set_operator(m_canvas, CAIRO_OPERATOR_SOURCE);
        } else {
            applyCanvasFillStrokeSourceIfNeeds();
        }
        cairo_translate(m_canvas, xx, yy);
        cairo_rectangle(m_canvas, 0, 0, ww, hh);
        cairo_fill(m_canvas);
        cairo_restore(m_canvas);
    }

    void strokeCairoRect(float xx, float yy, float ww, float hh)
    {
        cairo_save(m_canvas);
        applyCanvasFillStrokeSourceIfNeeds(true);
        cairo_rectangle(m_canvas, xx, yy, ww, hh);
        cairo_stroke(m_canvas);
        cairo_restore(m_canvas);
    }

    virtual void drawRect(const Unit::Rect& rt) override
    {
        if (lastState()->m_visible == false) {
            return;
        }
        float xx = rt.x(), yy = rt.y(), ww = rt.width(), hh = rt.height();
        if (lastState()->m_shadowData.hasValidValue()) {
            drawFillRectShadow(xx, yy, ww, hh);
        }
        drawRectInner(xx, yy, ww, hh);
    }

    virtual void drawRect(const LayoutRect& rt) override
    {
        if (lastState()->m_visible == false) {
            return;
        }
        int xx = rt.x(), yy = rt.y(), ww = rt.width(), hh = rt.height();
        if (lastState()->m_shadowData.hasValidValue()) {
            drawFillRectShadow(xx, yy, ww, hh);
        }
        drawRectInner(xx, yy, ww, hh);
    }

    virtual void drawRectInner(float x, float y, float w, float h) override
    {
        drawCairoRect(x, y, w, h);
    }

    virtual void strokeRect(const Unit::Rect& rt) override
    {
        if (lastState()->m_visible == false) {
            return;
        }
        float xx = rt.x(), yy = rt.y(), ww = rt.width(), hh = rt.height();
        if (lastState()->m_shadowData.hasValidValue()) {
            drawStrokeRectShadow(xx, yy, ww, hh);
        }
        drawStrokeRectInner(xx, yy, ww, hh);
    }

    virtual void strokeRect(const LayoutRect& rt) override
    {
        if (lastState()->m_visible == false) {
            return;
        }
        int xx = rt.x(), yy = rt.y(), ww = rt.width(), hh = rt.height();
        if (lastState()->m_shadowData.hasValidValue()) {
            drawStrokeRectShadow(xx, yy, ww, hh);
        }
        drawStrokeRectInner(xx, yy, ww, hh);
    }

    virtual void drawStrokeRectInner(float x, float y, float w,
                                     float h) override
    {
        strokeCairoRect(x, y, w, h);
    }

    virtual void drawRect(LayoutLocation p1, LayoutLocation p2,
                          LayoutLocation p3, LayoutLocation p4) override
    {
        if (lastState()->m_visible == false) {
            return;
        }

        if (lastState()->m_shadowData.hasValidValue()) {
            Path* path = Path::create();
            path->moveTo(p1.x(), p1.y());
            path->lineTo(p2.x(), p2.y());
            path->lineTo(p3.x(), p3.y());
            path->lineTo(p4.x(), p4.y());
            path->lineTo(p1.x(), p1.y());
            path->closePath();
            drawFillPathShadow(path);
        }

        cairo_save(m_canvas);

        cairo_move_to(m_canvas, p1.x(), p1.y());
        cairo_line_to(m_canvas, p2.x(), p2.y());
        cairo_line_to(m_canvas, p3.x(), p3.y());
        cairo_line_to(m_canvas, p4.x(), p4.y());
        cairo_line_to(m_canvas, p1.x(), p1.y());
        cairo_close_path(m_canvas);
        applyCanvasFillStrokeSourceIfNeeds();
        cairo_fill(m_canvas);

        cairo_restore(m_canvas);
    }

    virtual void drawText(LayoutUnit x, LayoutUnit y, LayoutUnit stringWidth,
                          const StringView& sv,
                          bool shouldSkipUnresolvedWebFont) override
    {
        int size = lastState()->m_font->size();
        if (lastState()->m_visible == false || size == 0 || sv.length() == 0) {
            return;
        }

        if (lastState()->m_shadowData.hasValidValue()) {
            drawFillTextShadow(x, y, stringWidth, sv);
        }
        INSTALL_PROFILE_TIMER("CanvasImplCairo::drawText");
        drawTextInner(x, y, stringWidth, sv, shouldSkipUnresolvedWebFont);
    }

    virtual void drawTextInner(float x, float y, float stringWidth,
                               const StringView& sv,
                               bool shouldSkipUnresolvedWebFont)
    {
        LayoutSize sz(stringWidth, lastState()->m_font->metrics().m_fontHeight);
        LayoutRect rt(x, y, sz.width(), sz.height());

        applyCanvasFillStrokeSourceIfNeeds();
#ifdef STARFISH_ENABLE_TEST
        if (getenv("PIXEL_TEST") && strlen(getenv("PIXEL_TEST"))) {
            drawAhemBoxCairo(m_canvas, rt, sv, rt.x(), rt.y());
        } else {
            drawGlyphsCairo(m_canvas, rt, sv, rt.x(), rt.y(), false,
                            shouldSkipUnresolvedWebFont);
            drawTextDecorationCairo(m_canvas, rt, sv, rt.x(), rt.y());
        }
#else
        drawGlyphsCairo(m_canvas, rt, sv, rt.x(), rt.y(), false,
                        shouldSkipUnresolvedWebFont);
        drawTextDecorationCairo(m_canvas, rt, sv, rt.x(), rt.y());
#endif
    }

    virtual void drawStrokeText(LayoutUnit x, LayoutUnit y,
                                LayoutUnit stringWidth, const StringView& sv,
                                bool shouldSkipUnresolvedWebFont) override
    {
        int size = lastState()->m_font->size();
        if (lastState()->m_visible == false || size == 0 || sv.length() == 0) {
            return;
        }

        if (lastState()->m_shadowData.hasValidValue()) {
            drawStrokeTextShadow(x, y, stringWidth, sv);
        }
        INSTALL_PROFILE_TIMER("CanvasImplCairo::drawStrokeText");
        drawStrokeTextInner(x, y, stringWidth, sv, shouldSkipUnresolvedWebFont);
    }

    virtual void drawStrokeTextInner(float x, float y, float stringWidth,
                                     const StringView& sv,
                                     bool shouldSkipUnresolvedWebFont)
    {
        LayoutSize sz(stringWidth, lastState()->m_font->metrics().m_fontHeight);
        LayoutRect rt(x, y, sz.width(), sz.height());

        applyCanvasFillStrokeSourceIfNeeds(true);
        drawGlyphsCairo(m_canvas, rt, sv, rt.x(), rt.y(), true,
                        shouldSkipUnresolvedWebFont);
        drawTextDecorationCairo(m_canvas, rt, sv, rt.x(), rt.y());
    }

    void setImageRenderingModeToPattern(cairo_pattern_t* resizePattern,
                                        ImageRenderingValue imageRenderingMode)
    {
        STARFISH_ASSERT(resizePattern != nullptr);
        if (imageRenderingMode == ImageRenderingCrispEdgesValue) {
            cairo_pattern_set_filter(resizePattern, CAIRO_FILTER_NEAREST);
        } else if (imageRenderingMode == ImageRenderingAutoValue) {
            if (m_flag == CanvasFlag::PlainElement) {
                cairo_filter_t filter;
                auto anti = cairo_get_antialias(m_canvas);
                if (anti == CAIRO_ANTIALIAS_NONE) {
                    filter = CAIRO_FILTER_FAST;
                } else {
                    filter = anti >= CAIRO_ANTIALIAS_GOOD ? CAIRO_FILTER_GOOD
                                                          : CAIRO_FILTER_FAST;
                }
                cairo_pattern_set_filter(resizePattern, filter);
            } else {
                STARFISH_ASSERT(m_flag == CanvasFlag::CanvasElement);
                cairo_pattern_set_filter(resizePattern, CAIRO_FILTER_BILINEAR);
            }
        } else if (imageRenderingMode == ImageRenderingPixelatedValue) {
            // TODO PixelatedValue should affect when painting bigger image than
            // original only
            cairo_pattern_set_filter(resizePattern, CAIRO_FILTER_GAUSSIAN);
        }
    }

    virtual void drawImage(uint8_t* image, size_t imageWidth,
                           size_t imageStride, size_t imageHeight,
                           const Unit::Rect& dst,
                           ImageRenderingValue imageRenderingMode) override
    {
        cairo_surface_t* imageSurface = cairo_image_surface_create_for_data(
            (unsigned char*)image, CAIRO_FORMAT, imageWidth, imageHeight,
            imageStride);

        drawImageCairo(imageSurface, dst, imageWidth, imageHeight,
                       imageRenderingMode);

        cairo_surface_destroy(imageSurface);
    }

    void drawImageCairo(cairo_surface_t* localSurface, const Unit::Rect& dst,
                        double surfaceWidth, double surfaceHeight,
                        ImageRenderingValue imageRenderingMode)
    {
        STARFISH_ASSERT(localSurface != nullptr);
        INSTALL_PROFILE_TIMER("CanvasImplCairo::drawImageCairo");

        float xx = 0.0, yy = 0.0, ww = 0.0, hh = 0.0;
        xx = dst.x();
        yy = dst.y();
        ww = dst.width();
        hh = dst.height();

        if (ww == 0 || hh == 0) {
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
        setImageRenderingModeToPattern(resizePattern, imageRenderingMode);
        cairo_pattern_set_extend(resizePattern, CAIRO_EXTEND_PAD);

        cairo_set_source(m_canvas, resizePattern);

        cairo_rectangle(m_canvas, 0, 0, ww, hh);

        cairo_clip(m_canvas);
        cairo_paint_with_alpha(m_canvas, lastState()->m_layerOpacity *
                                             lastState()->m_globalAlpha);

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

    virtual void drawImageInner(NativeImageData* data, const Unit::Rect& dst,
                                ImageRenderingValue imageRenderingMode) override
    {
        size_t surfaceWidth = data->width(), surfaceHeight = data->height();

        bool surfaceWasCreated = false;
        cairo_surface_t* image = (cairo_surface_t*)data->unwrap();
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

    virtual void drawNativeImageData(
        NativeImageData* data, const Unit::Rect& dst,
        ImageRenderingValue imageRenderingMode) override
    {
        STARFISH_ASSERT(data != nullptr);

        if (lastState()->m_visible == false) {
            return;
        }

        if (lastState()->m_shadowData.hasValidValue()) {
            drawImageShadow(data, dst);
        }
        drawImageInner(data, dst, imageRenderingMode);
    }

    virtual void maskNativeImage(NativeImageData* data, const Unit::Rect& dst,
                                 bool removeImmediately = true) override
    {
        cairo_surface_t* image = (cairo_surface_t*)data->unwrap();
        if (image) {
            lastState()->m_maskPatternData = data;
            lastState()->m_maskPattern =
                cairo_pattern_create_for_surface(image);
            lastState()->m_shouldRemoveImmediately = removeImmediately;
            cairo_matrix_t matrix;
            cairo_get_matrix(m_canvas, &matrix);
            {
                SkMatrix m = SkMatrix::I();
                m.set(0, matrix.xx);
                m.set(1, matrix.yx);
                m.set(2, matrix.x0);
                m.set(3, matrix.xy);
                m.set(4, matrix.yy);
                m.set(5, matrix.y0);
                lastState()->m_maskTM = m;
            }

            cairo_matrix_init_identity(&matrix);
            cairo_matrix_translate(&matrix, -dst.x(), -dst.y());
            cairo_matrix_scale(&matrix, data->width() / dst.width(),
                               data->height() / dst.height());
            cairo_pattern_set_matrix(
                (cairo_pattern_t*)lastState()->m_maskPattern, &matrix);

            clip(dst);
            cairo_push_group(m_canvas);
        }
    }

    virtual void drawImageInner(NativeImageData* data, const Unit::Rect& src,
                                const Unit::Rect& dst,
                                const DrawImageInfo& borderinfo,
                                ImageRenderingValue imageRenderingMode) override
    {
        cairo_save(m_canvas);
        bool surfaceWasCreated = false;
        cairo_surface_t* srcImage = (cairo_surface_t*)data->unwrap();
        if (srcImage == nullptr) {
            surfaceWasCreated = true;
            srcImage = cairo_image_surface_create_for_data(
                (unsigned char*)data->data(), CAIRO_FORMAT, data->width(),
                data->height(), data->stride());
        } else if (srcImage == m_surface) {
            auto format = cairo_image_surface_get_format(srcImage);
            auto width = cairo_image_surface_get_width(srcImage);
            auto height = cairo_image_surface_get_height(srcImage);
            auto surfaceToCopy = cairo_surface_create_similar_image(
                srcImage, format, width, height);

            auto context = cairo_create(surfaceToCopy);
            cairo_set_source_surface(context, srcImage, 0, 0);
            cairo_rectangle(context, 0, 0, width, height);
            cairo_fill(context);
            cairo_surface_flush(surfaceToCopy);
            cairo_destroy(context);

            srcImage = surfaceToCopy;
            surfaceWasCreated = true;
        }

        cairo_surface_t* image = srcImage;
        if (src.x() != 0 || src.y() != 0 || src.width() != data->width() ||
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
        if (surfaceWasCreated == true) {
            cairo_surface_destroy(srcImage);
        }
        if (srcImage != image) {
            cairo_surface_destroy(image);
        }
        cairo_restore(m_canvas);
    }

    virtual void drawNativeImageData(
        NativeImageData* data, const Unit::Rect& src, const Unit::Rect& dst,
        const DrawImageInfo& borderinfo,
        ImageRenderingValue imageRenderingMode) override
    {
        STARFISH_ASSERT(data != nullptr);

        if (lastState()->m_visible == false) {
            return;
        }

        if (lastState()->m_shadowData.hasValidValue()) {
            drawImageShadow(data, dst);
        }

        drawImageInner(data, src, dst, borderinfo, imageRenderingMode);
    }

    void drawRepeatImageCairo(cairo_surface_t* localSurface,
                              const Unit::Rect& dst, float imageWidth,
                              float imageHeight,
                              const DrawImageInfo& borderinfo,
                              ImageRenderingValue imageRenderingMode)
    {
        STARFISH_ASSERT(localSurface != nullptr);

        if (lastState()->m_visible == false) {
            return;
        }

        INSTALL_PROFILE_TIMER("CanvasImplCairo::drawRepeatImage");

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
        if (scaledWidth != 0 && scaledHeight != 0) {
            pattern = cairo_pattern_create_for_surface(image);

            cairo_matrix_init_scale(&matrix, hScale, vScale);
            cairo_matrix_translate(&matrix, -x, -y);

            cairo_pattern_set_matrix(pattern, &matrix);
            setImageRenderingModeToPattern(pattern, imageRenderingMode);
            cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);

            cairo_translate(m_canvas, xx, yy);
            cairo_set_source(m_canvas, pattern);

            cairo_rectangle(m_canvas, 0.0, 0.0, ww, hh);

            cairo_clip(m_canvas);
            cairo_paint_with_alpha(m_canvas, lastState()->m_layerOpacity *
                                                 lastState()->m_globalAlpha);

            cairo_pattern_destroy(pattern);
        }
        cairo_restore(m_canvas);
    }

    virtual void drawRepeatNativeImageData(
        NativeImageData* data, const Unit::Rect& dst, float imageWidth,
        float imageHeight, bool xRepeat, bool yRepeat,
        ImageRenderingValue imageRenderingMode) override
    {
        STARFISH_ASSERT(data != nullptr);

        if (lastState()->m_visible == false) {
            return;
        }

        INSTALL_PROFILE_TIMER("CanvasImplCairo::drawRepeatImage");

        cairo_save(m_canvas);
        float xx = 0.0, yy = 0.0, ww = 0.0, hh = 0.0;
        ww = dst.width();
        hh = dst.height();

        float x = 0.0, y = 0.0;
        if (xRepeat == true) {
            x = (dst.x() - floor(dst.x() / imageWidth) * imageWidth) -
                imageWidth;
        } else {
            xx += dst.x();
        }

        if (yRepeat == true) {
            y = (dst.y() - floor(dst.y() / imageHeight) * imageHeight) -
                imageHeight;
        } else {
            yy += dst.y();
        }

        cairo_pattern_t* pattern;
        cairo_matrix_t matrix;

        cairo_surface_t* image = (cairo_surface_t*)data->unwrap();

        bool surfaceWasCreated = false;
        if (image == nullptr) {
            surfaceWasCreated = true;
            image = cairo_image_surface_create_for_data(
                (unsigned char*)data->data(), CAIRO_FORMAT, data->width(),
                data->height(), data->stride());
        }

        double surfaceWidth = data->width(), surfaceHeight = data->height();
        if (surfaceWidth != 0 && surfaceHeight != 0) {
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

            cairo_clip(m_canvas);
            cairo_paint_with_alpha(m_canvas, lastState()->m_layerOpacity *
                                                 lastState()->m_globalAlpha);

            cairo_pattern_destroy(pattern);
        }

        if (surfaceWasCreated == true) {
            cairo_surface_destroy(image);
        }
        cairo_restore(m_canvas);
    }

    virtual void drawLinearGradient(const Unit::Rect& dst,
                                    GradientDrawingInfo* info,
                                    NativeGradient* gradient) override
    {
        STARFISH_ASSERT(info != nullptr);

        if (lastState()->m_visible == false) {
            return;
        }

        INSTALL_PROFILE_TIMER("CanvasImplCairo::drawLinearGradient");
        cairo_save(m_canvas);

        cairo_rectangle(m_canvas, dst.x(), dst.y(), dst.width(), dst.height());
        cairo_set_source(m_canvas, ((NativeGradientCairo*)gradient)->pattern());
        cairo_fill(m_canvas);
        cairo_restore(m_canvas);
    }

    virtual void drawRadialGradient(const Unit::Rect& dst,
                                    GradientDrawingInfo* info,
                                    NativeGradient* gradient) override
    {
        STARFISH_ASSERT(info != nullptr);

        if (lastState()->m_visible == false) {
            return;
        }

        INSTALL_PROFILE_TIMER("CanvasImplCairo::drawRadialGradient");

        cairo_save(m_canvas);
        cairo_rectangle(m_canvas, dst.x(), dst.y(), dst.width(), dst.height());
        cairo_clip(m_canvas);
        cairo_rectangle(m_canvas, dst.x(), dst.y(), dst.width(), dst.height());

        if (info->secondRadius != 0 && info->firstRadius > info->secondRadius) {
            cairo_scale(m_canvas, 1,
                        1 * (info->secondRadius / info->firstRadius));
        } else if (info->secondRadius != 0 &&
                   info->firstRadius < info->secondRadius) {
            cairo_scale(m_canvas, 1 * (info->firstRadius / info->secondRadius),
                        1);
        }

        cairo_arc(m_canvas, info->x2, info->y2, info->r2, 0, 2 * M_PI);
        cairo_set_source(m_canvas, ((NativeGradientCairo*)gradient)->pattern());
        cairo_fill(m_canvas);
        cairo_restore(m_canvas);
    }

    virtual void postMatrix(const SkMatrix& matrix) override
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

    virtual void setMatrix(const SkMatrix& matrix) override
    {
        cairo_matrix_t cm;
        cm.xx = matrix.get(0);
        cm.yx = matrix.get(1);
        cm.x0 = matrix.get(2);
        cm.xy = matrix.get(3);
        cm.yy = matrix.get(4);
        cm.y0 = matrix.get(5);

        cairo_set_matrix(m_canvas, &cm);
        checkError();
    }

    virtual SkMatrix currentTransformMatrix() override
    {
        cairo_matrix_t matrix;
        cairo_get_matrix(m_canvas, &matrix);

        SkMatrix m = SkMatrix::I();
        m.set(0, matrix.xx);
        m.set(1, matrix.yx);
        m.set(2, matrix.x0);
        m.set(3, matrix.xy);
        m.set(4, matrix.yy);
        m.set(5, matrix.y0);

        return m;
    }

    virtual void applyMatrixTo(LayoutLocation& lp) override
    {
        double x = lp.x();
        double y = lp.y();
        cairo_matrix_t a_matrix;

        cairo_get_matrix(m_canvas, &a_matrix);
        cairo_matrix_transform_point(&a_matrix, &x, &y);
        lp.setX(x);
        lp.setY(y);
    }

    virtual void applyMatrixTo(LayoutRect& lp) override
    {
        double x = lp.x();
        double y = lp.y();
        cairo_matrix_t a_matrix;

        cairo_get_matrix(m_canvas, &a_matrix);
        cairo_matrix_transform_point(&a_matrix, &x, &y);
        lp.setX(x);
        lp.setY(y);
    }

    virtual CanvasLineCap lineCap() override
    {
        auto cap = cairo_get_line_cap(m_canvas);
        return CanvasCairoUtils::cairoLineCapToCavansLineCap(cap);
    }

    virtual void setLineCap(CanvasLineCap lineCap) override
    {
        cairo_line_cap_t cap =
            CanvasCairoUtils::canvasLineCapToCairoLineCap(lineCap);
        cairo_set_line_cap(m_canvas, cap);
    }

    virtual CanvasLineJoin lineJoin() override
    {
        auto join = cairo_get_line_join(m_canvas);
        return CanvasCairoUtils::cairoLineJoinToCanvasLineJoin(join);
    }

    virtual void setLineJoin(StrokeLineJoin lineJoin) override
    {
        cairo_line_join_t join =
            CanvasCairoUtils::canvasLineJoinToCairoLineJoin(lineJoin);
        cairo_set_line_join(m_canvas, join);
    }

    virtual double miterLimit() override
    {
        return cairo_get_miter_limit(m_canvas);
    }

    virtual void setMiterLimit(double limit) override
    {
        cairo_set_miter_limit(m_canvas, limit);
    }

    virtual double shadowOffsetX() override
    {
        return lastState()->m_shadowData.offsetX();
    }

    virtual void setShadowOffsetX(double offset) override
    {
        lastState()->m_shadowData.setOffsetX(offset);
    }

    virtual double shadowOffsetY() override
    {
        return lastState()->m_shadowData.offsetY();
    }

    virtual void setShadowOffsetY(double offset) override
    {
        lastState()->m_shadowData.setOffsetY(offset);
    }

    virtual double shadowBlur() override
    {
        return lastState()->m_shadowData.radius();
    }

    virtual void setShadowBlur(double blur) override
    {
        lastState()->m_shadowData.setRadius(blur);
    }

    virtual Unit::Color shadowColor() override
    {
        return lastState()->m_shadowData.color();
    }

    virtual void setShadowColor(const Unit::Color& color) override
    {
        lastState()->m_shadowData.setColor(color);
    }

    virtual bool imageSmoothingEnabled() override
    {
        return lastState()->m_imageSmoothingEnabled;
    }

    virtual void setImageSmoothingEnabled(bool value) override
    {
        lastState()->m_imageSmoothingEnabled = value;
    }

    virtual ImageSmoothingQuality imageSmoothingQuality() override
    {
        return lastState()->m_imageSmoothingQuality;
    }

    virtual void setImageSmoothingQuality(
        ImageSmoothingQuality quality) override
    {
        lastState()->m_imageSmoothingQuality = quality;
    }

    virtual void beginPath() override
    {
        cairo_new_path(m_canvas);
    }

    virtual void closePath() override
    {
        cairo_close_path(m_canvas);
    }

    virtual void moveTo(float x, float y) override
    {
        cairo_move_to(m_canvas, x, y);
    }

    virtual void lineTo(float x, float y) override
    {
        cairo_line_to(m_canvas, x, y);
    }

    virtual void referencePath(Path* path) override
    {
        setPathAsNewPathOnCurrentContext(path);
    }

    virtual void curveTo(float x1, float y1, float x2, float y2, float x3,
                         float y3) override
    {
        cairo_curve_to(m_canvas, x1, y1, x2, y2, x3, y3);
    }

    virtual void quadraticCurveTo(float x1, float y1, float x2,
                                  float y2) override
    {
        double x0, y0;
        cairo_get_current_point(m_canvas, &x0, &y0);
        cairo_curve_to(m_canvas, 2.0 / 3.0 * x1 + 1.0 / 3.0 * x0,
                       2.0 / 3.0 * y1 + 1.0 / 3.0 * y0,
                       2.0 / 3.0 * x1 + 1.0 / 3.0 * x2,
                       2.0 / 3.0 * y1 + 1.0 / 3.0 * y2, x2, y2);
    }

    virtual void arc(double xc, double yc, double radius, double angle1,
                     double angle2) override
    {
        cairo_arc(m_canvas, xc, yc, radius, angle1, angle2);
    }

    virtual void arcNegative(double xc, double yc, double radius, double angle1,
                             double angle2) override
    {
        cairo_arc_negative(m_canvas, xc, yc, radius, angle1, angle2);
    }

    virtual void stroke() override
    {
        if (lastState()->m_visible == false) {
            cairo_close_path(m_canvas);
            return;
        }
        applyCanvasFillStrokeSourceIfNeeds(true);
        cairo_stroke(m_canvas);
    }

    virtual void strokePreserve() override
    {
        if (lastState()->m_visible == false) {
            return;
        }
        applyCanvasFillStrokeSourceIfNeeds(true);
        cairo_stroke_preserve(m_canvas);
    }

    virtual void strokePath(Path* path) override
    {
        STARFISH_ASSERT(path != nullptr);

        if (lastState()->m_visible == false) {
            return;
        }
        if (lastState()->m_shadowData.hasValidValue()) {
            drawStrokePathShadow(path);
        }
        drawStrokePathInner(path);
    }

    virtual void fill() override
    {
        if (lastState()->m_visible == false) {
            cairo_close_path(m_canvas);
            return;
        }
        applyCanvasFillStrokeSourceIfNeeds();
        cairo_fill(m_canvas);
    }

    virtual void fillPreserve() override
    {
        if (lastState()->m_visible == false) {
            return;
        }
        applyCanvasFillStrokeSourceIfNeeds();
        cairo_fill_preserve(m_canvas);
    }

    virtual void fillPath(Path* path) override
    {
        STARFISH_ASSERT(path != nullptr);

        if (lastState()->m_visible == false) {
            return;
        }
        if (lastState()->m_shadowData.hasValidValue()) {
            drawFillPathShadow(path);
        }

        drawPathInner(path);
    }

    virtual void drawPathInner(Path* path) override
    {
        setPathAsNewPathOnCurrentContext(path);
        fill();
    }

    virtual void drawStrokePathInner(Path* path) override
    {
        setPathAsNewPathOnCurrentContext(path);
        stroke();
    }

    virtual void clipPath() override
    {
        cairo_clip(m_canvas);
    }

    virtual void clipPathPreserve() override
    {
        cairo_clip_preserve(m_canvas);
    }

    virtual void setFillRule(bool shouldUseNonZeroFillRule) override
    {
        if (shouldUseNonZeroFillRule == true) {
            cairo_set_fill_rule(m_canvas,
                                cairo_fill_rule_t::CAIRO_FILL_RULE_WINDING);
        } else {
            cairo_set_fill_rule(m_canvas,
                                cairo_fill_rule_t::CAIRO_FILL_RULE_EVEN_ODD);
        }
    }

    virtual float lineWidth() override
    {
        return cairo_get_line_width(m_canvas);
    }

    virtual void setLineWidth(float width) override
    {
        cairo_set_line_width(m_canvas, width);
    }

    virtual void setDash(const GCAtomicVector<double>& dashes) override
    {
        lastState()->m_dashes = dashes;
        updateDashAndDashOffset();
    }

    virtual GCAtomicVector<double> dash() override
    {
        return lastState()->m_dashes;
    }

    virtual double dashOffset() override
    {
        return lastState()->m_dashOffset;
    }

    virtual void setDashOffset(double offset) override
    {
        lastState()->m_dashOffset = offset;
        updateDashAndDashOffset();
    }

    void updateDashAndDashOffset()
    {
        if (std::all_of(lastState()->m_dashes.begin(),
                        lastState()->m_dashes.end(),
                        [](double& dash) { return !dash; })) {
            cairo_set_dash(m_canvas, 0, 0, 0);
        } else {
            cairo_set_dash(m_canvas, lastState()->m_dashes.data(),
                           lastState()->m_dashes.size(),
                           lastState()->m_dashOffset);
        }
        checkError();
    }

    virtual void resetMatrixAndClip(bool needsApplyDPR) override
    {
        cairo_reset_clip(m_canvas);
        cairo_identity_matrix(m_canvas);
        if (needsApplyDPR == true) {
            applyDevicePixelRatio();
        }
    }

    virtual void resetMatrix(bool needsApplyDPR) override
    {
        cairo_identity_matrix(m_canvas);
        if (needsApplyDPR == true) {
            applyDevicePixelRatio();
        }
    }

    virtual void resetClip() override
    {
        cairo_reset_clip(m_canvas);
    }

private:
#ifdef STARFISH_ENABLE_TEST
    void drawAhemBoxCairo(cairo_t* canvas, LayoutRect rect,
                          const StringView& sv, LayoutUnit dx, LayoutUnit dy)
    {
        STARFISH_ASSERT(canvas != nullptr);

        if (getenv("PIXEL_TEST") && strlen(getenv("PIXEL_TEST"))) {
            LayoutUnit x = rect.x();
            LayoutUnit y = rect.y();

            if (sv.originalString() != String::emptyString &&
                sv.originalString()->charAt(sv.start()) != ' ') {
                float h = lastState()->m_font->size();
                float xx = x;
                for (size_t i = sv.start(); i < sv.end(); i++) {
                    char32_t ch = sv.originalString()->charAt(i);
                    if (ch == 160) { // nbsp
                    } else if ((String::isFixedWidthChar(ch) == true) ||
                               (String::isZeroWidthChar(ch) == true)) {
                        // Fixed-width spaces
                        size_t num = Font::spaceSizeNumerator(ch);
                        xx += h * ((float)num / SPACE_SIZE_DENOMINATOR);
                        continue;
                    } else {
                        if (ch != 'p') {
                            Font* fnt = lastState()->m_font;
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
                                applyCanvasFillStrokeSourceIfNeeds();
                                cairo_fill(canvas);
                                cairo_restore(canvas);
                            } else {
                                // left, top, w, h
                                Unit::Rect rt(xx, y, h, h);
                                cairo_rectangle(canvas, xx, y, h, h);
                                applyCanvasFillStrokeSourceIfNeeds();
                                cairo_fill(canvas);
                            }
                        } else {
                            int ph = h * 0.2;
                            cairo_rectangle(canvas, xx, y + h - ph, h, ph);
                            applyCanvasFillStrokeSourceIfNeeds();
                            cairo_fill(canvas);
                        }
                    }
                    xx += h + lastState()->m_font->letterSpacing();
                }
            }
        }
    }
#endif

    void drawGlyphs(cairo_t* canvas, FontFaceImplCairo* fontFace,
                    const cairo_matrix_t& sizeMatrix,
                    const cairo_matrix_t& identityMatrix, cairo_glyph_t* glyphs,
                    size_t glyphCount, bool canUsePath = true)
    {
        STARFISH_ASSERT(canvas != nullptr);
        STARFISH_ASSERT(glyphs != nullptr);
        cairo_font_face_t* cairofontFace =
            cairo_ft_font_face_create_for_ft_face(fontFace->freetypeFace(), 0);

        FontFaceReferenceHolder* holder =
            new (NoGC) FontFaceReferenceHolder(fontFace);
        if (!holder->addToCairoFontFace(cairofontFace)) {
            STARFISH_LOG_ERROR(
                "Failed to add FontFaceHolder to cairo_font_face");
            GC_FREE(holder);
            cairo_font_face_destroy(cairofontFace);
            return;
        }
        cairo_font_options_t* fontOptions = cairo_font_options_create();
        cairo_scaled_font_t* scaledFontFace = cairo_scaled_font_create(
            cairofontFace, &sizeMatrix, &identityMatrix, fontOptions);
        cairo_font_options_destroy(fontOptions);
        auto oldScaledFont = cairo_get_scaled_font(canvas);
        cairo_scaled_font_reference(oldScaledFont);
        cairo_set_scaled_font(canvas, scaledFontFace);

        bool needsToDrawWithPath = false;
        float fontStrokeWidth = 0;
#if !defined(STARFISH_ENABLE_TEST)
        if (glyphCount && canUsePath && m_state.back()->m_font->size() >= 24) {
            cairo_matrix_t m;
            cairo_get_matrix(m_canvas, &m);

            double x = glyphs[0].x;
            double y = glyphs[0].y;

            cairo_matrix_transform_point(&m, &x, &y);

            if ((x - (int)x) != 0 || (y - (int)y) != 0) {
                needsToDrawWithPath = true;
            }
        }

        // draw glpyh with stoke if there was bold style on css but no bold font
        // was selected
        if (glyphCount && canUsePath && lastState()->m_font->weight() > 4 &&
            !(fontFace->freetypeFace()->style_flags & FT_STYLE_FLAG_BOLD)) {
            needsToDrawWithPath = true;
            fontStrokeWidth =
                (lastState()->m_font->weight() / 4.0) *
                std::min(1.f, (lastState()->m_font->size() / 48.f));
        }
#endif

        if (needsToDrawWithPath) {
            cairo_glyph_path(canvas, glyphs, glyphCount);
            if (fontStrokeWidth) {
                cairo_save(canvas);

                cairo_set_source_rgba(canvas, 0, 0, 0, 1);
                cairo_set_line_width(canvas, fontStrokeWidth);
                cairo_set_operator(canvas, CAIRO_OPERATOR_SOURCE);

                double x1 = 0, y1 = 0, x2 = 0, y2 = 0;
                cairo_stroke_extents(canvas, &x1, &y1, &x2, &y2);
                cairo_rectangle(canvas, x1, y1, x2 - x1, y2 - y1);
                cairo_clip(canvas);
                checkError();

                cairo_push_group_with_content(canvas, CAIRO_CONTENT_ALPHA);
                cairo_glyph_path(canvas, glyphs, glyphCount);
                cairo_stroke_preserve(canvas);

                cairo_set_operator(canvas, CAIRO_OPERATOR_CLEAR);
                cairo_fill_preserve(canvas);

                cairo_pattern_t* mask = cairo_pop_group(canvas);
                cairo_mask(canvas, mask);
                cairo_pattern_destroy(mask);

                cairo_restore(canvas);
            }
            cairo_fill(canvas);
        } else {
            cairo_show_glyphs(canvas, glyphs, glyphCount);
        }

        cairo_set_scaled_font(canvas, oldScaledFont);
        cairo_scaled_font_destroy(oldScaledFont);
        cairo_scaled_font_destroy(scaledFontFace);
        cairo_font_face_destroy(cairofontFace);
    }

    void drawStrokeGlyphs(cairo_t* canvas, FontFaceImplCairo* fontFace,
                          const cairo_matrix_t& sizeMatrix,
                          const cairo_matrix_t& identityMatrix,
                          cairo_glyph_t* glyphs, size_t glyphCount,
                          size_t strokeWidth)
    {
        STARFISH_ASSERT(canvas != nullptr);
        STARFISH_ASSERT(glyphs != nullptr);

        cairo_font_face_t* cairofontFace =
            cairo_ft_font_face_create_for_ft_face(fontFace->freetypeFace(), 0);

        FontFaceReferenceHolder* holder =
            new (NoGC) FontFaceReferenceHolder(fontFace);
        if (!holder->addToCairoFontFace(cairofontFace)) {
            STARFISH_LOG_ERROR(
                "Failed to add FontFaceHolder to cairo_font_face");
            GC_FREE(holder);
            cairo_font_face_destroy(cairofontFace);
            return;
        }

        cairo_font_options_t* fontOptions = cairo_font_options_create();
        cairo_scaled_font_t* scaledFontFace = cairo_scaled_font_create(
            cairofontFace, &sizeMatrix, &identityMatrix, fontOptions);
        cairo_font_options_destroy(fontOptions);
        auto oldScaledFont = cairo_get_scaled_font(canvas);
        cairo_set_line_width(canvas, strokeWidth);
        cairo_scaled_font_reference(oldScaledFont);
        cairo_set_scaled_font(canvas, scaledFontFace);
        cairo_glyph_path(canvas, glyphs, glyphCount);
        cairo_stroke(canvas);
        cairo_set_scaled_font(canvas, oldScaledFont);
        cairo_scaled_font_destroy(oldScaledFont);
        cairo_scaled_font_destroy(scaledFontFace);
        cairo_font_face_destroy(cairofontFace);
    }

    void drawGlyphsCairo(cairo_t* canvas, LayoutRect rect, const StringView& sv,
                         LayoutUnit dx, LayoutUnit dy, bool isStroke,
                         bool shouldSkipUnresolvedWebFont)
    {
        STARFISH_ASSERT(canvas != nullptr);

        LayoutUnit xBias = 0;
        FT_UInt glyph_index = 0;
        FontFaceImplCairo* lastFontFace = nullptr;
        FontImplCairo* f = (FontImplCairo*)lastState()->m_font;
        int size = f->size();
        auto stringAccessData = sv.bufferAccessData();

        FontMetrics fontMetrics = f->metrics();
        cairo_translate(canvas, dx, fontMetrics.m_ascender + dy);
        cairo_glyph_t* glyphs = nullptr;

        const size_t stackProcessingSize = 128;
        cairo_glyph_t glyphsStackBuffer[stackProcessingSize];
        glyphs = glyphsStackBuffer;
        if (UNLIKELY(stringAccessData.length > stackProcessingSize)) {
            glyphs = (cairo_glyph_t*)malloc(sizeof(cairo_glyph_t) *
                                            stringAccessData.length);
            STARFISH_ASSERT(glyphs != nullptr);
        }

        size_t glyphCount = 0;
        LayoutUnit letterSpacing = f->letterSpacing();

        // Make the scale matrix of font size.
        cairo_matrix_t sizeMatrix;
        cairo_matrix_init_identity(&sizeMatrix);
        cairo_matrix_scale(&sizeMatrix, size, size);

        cairo_matrix_t identityMatrix;
        cairo_matrix_init_identity(&identityMatrix);

        if (cairoBackendCanUseSimpleFontPath(f, sv) == true) {
            LayoutUnit letterSpacingValueSoFar;

            for (size_t i = 0; i < stringAccessData.length; i++) {
                std::pair<std::pair<FontFaceImplCairo*, size_t>,
                          std::pair<unsigned, LayoutUnit>>
                    g = cairoBackendInternalLoadGlyph(
                        f, stringAccessData.charAt(i));
                if (g.second.first != 0) {
                    if (shouldSkipUnresolvedWebFont ==
                        true) { // skip webfont enabled
                        if (f->seenUnresolvedWebFontIndex() != SIZE_MAX &&
                            f->seenUnresolvedWebFontIndex() <= g.first.second) {
                            xBias += g.second.second;
                            continue;
                        }
                    }
                    if (lastFontFace != g.first.first) {
                        if (glyphCount != 0) {
                            if (isStroke == true) {
                                drawStrokeGlyphs(canvas, lastFontFace,
                                                 sizeMatrix, identityMatrix,
                                                 glyphs, glyphCount,
                                                 lineWidth());
                            } else {
                                drawGlyphs(canvas, lastFontFace, sizeMatrix,
                                           identityMatrix, glyphs, glyphCount);
                            }
                            glyphCount = 0;
                        }
                        lastFontFace = g.first.first;
                    }
                    glyphs[glyphCount].index = g.second.first;
                    glyphs[glyphCount].x = xBias + letterSpacingValueSoFar;
                    glyphs[glyphCount].y = 0;
                    glyphCount++;
                    STARFISH_ASSERT(glyphCount <= sv.length());
                    letterSpacingValueSoFar += letterSpacingValueSoFar;
                    xBias += g.second.second + letterSpacing;
                } else {
                    if (shouldSkipUnresolvedWebFont ==
                        true) { // skip webfont enabled
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
                    applyCanvasFillStrokeSourceIfNeeds();
                    cairo_stroke(canvas);
                    cairo_restore(canvas);
                    xBias += f->spaceWidth() + letterSpacing;
                }
            }

            if (glyphCount != 0) {
                if (isStroke == true) {
                    drawStrokeGlyphs(canvas, lastFontFace, sizeMatrix,
                                     identityMatrix, glyphs, glyphCount,
                                     lineWidth());
                } else {
                    drawGlyphs(canvas, lastFontFace, sizeMatrix, identityMatrix,
                               glyphs, glyphCount);
                }

                glyphCount = 0;
            }
        } else {
            auto runs = generateFontCairoTextRuns(&sv, f);

            int lastUnicodeBlock = 0;
            float xBias = 0;
            for (size_t i = 0; i < runs.size(); i++) {
                const FontCairoTextRun& run = runs[i];

                LayoutUnit letterSpacingValueSoFar;
                if (run.m_fontFace == nullptr) {
                    if (/* skip webfont enabled*/ f
                                ->seenUnresolvedWebFontIndex() != SIZE_MAX &&
                        shouldSkipUnresolvedWebFont) {
                    } else {
                        cairo_save(canvas);
                        cairo_set_line_width(canvas, 1);
                        for (size_t j = 0; j < run.m_text.length(); j++) {
                            cairo_new_path(canvas);
                            cairo_rectangle(canvas, xBias + j * f->spaceWidth(),
                                            -fontMetrics.m_ascender,
                                            f->spaceWidth(),
                                            fontMetrics.m_fontHeight);
                            applyCanvasFillStrokeSourceIfNeeds();
                            cairo_stroke(canvas);
                        }
                        cairo_restore(canvas);
                    }
                } else {
                    if (/* skip webfont enabled*/ f
                                ->seenUnresolvedWebFontIndex() != SIZE_MAX &&
                        f->seenUnresolvedWebFontIndex() <= run.m_faceIndex &&
                        shouldSkipUnresolvedWebFont) {
                    } else {
                        if (run.m_fontFace != lastFontFace ||
                            lastUnicodeBlock != run.m_unicodeBlock) {
                            if (glyphCount != 0) {
                                if (isStroke == true) {
                                    drawStrokeGlyphs(canvas, lastFontFace,
                                                     sizeMatrix, identityMatrix,
                                                     glyphs, glyphCount,
                                                     lineWidth());
                                } else {
                                    drawGlyphs(
                                        canvas, lastFontFace, sizeMatrix,
                                        identityMatrix, glyphs, glyphCount,
                                        !unicodeBlockContainsGraphicSymbol(
                                            lastUnicodeBlock));
                                }
                                glyphCount = 0;
                            }
                            lastFontFace = run.m_fontFace;
                            lastUnicodeBlock = run.m_unicodeBlock;
                        }

                        for (size_t j = 0; j < run.m_glyphs.size(); j++) {
                            glyphs[glyphCount].index = run.m_glyphs[j];
                            glyphs[glyphCount].x = run.m_glyphPositions[j].x() +
                                                   xBias +
                                                   letterSpacingValueSoFar;
                            letterSpacingValueSoFar += letterSpacing;
                            glyphs[glyphCount].y = run.m_glyphPositions[j].y();
                            glyphCount++;
                        }
                    }
                }

                xBias += (run.m_runWidth + letterSpacingValueSoFar);
            }

            if (glyphCount != 0) {
                if (isStroke == true) {
                    drawStrokeGlyphs(canvas, lastFontFace, sizeMatrix,
                                     identityMatrix, glyphs, glyphCount,
                                     lineWidth());
                } else {
                    drawGlyphs(
                        canvas, lastFontFace, sizeMatrix, identityMatrix,
                        glyphs, glyphCount,
                        !unicodeBlockContainsGraphicSymbol(lastUnicodeBlock));
                }

                glyphCount = 0;
            }
            STARFISH_ASSERT(glyphCount == 0);
        }

        cairo_translate(canvas, -dx, -dy - fontMetrics.m_ascender);

        if (UNLIKELY(stringAccessData.length > stackProcessingSize)) {
            free(glyphs);
        }
    }

    void drawTextDecorationCairo(cairo_t* canvas, LayoutRect rect,
                                 const StringView& sv, LayoutUnit dx,
                                 LayoutUnit dy)
    {
        STARFISH_ASSERT(canvas != nullptr);

        FontImplCairo* f = (FontImplCairo*)lastState()->m_font;
        FontFaceImplCairo* fc = (FontFaceImplCairo*)f->fontFaceList()[0];
        FT_Face face = fc->freetypeFace();
        int intSize(f->size() + 0.5f);

        cairo_translate(canvas, dx, dy);

        float lineWidth =
            face->underline_thickness / (float)fc->m_unitsPerEM * intSize;
        if (lastState()->m_textDecorationData.hasUnderLine() == true) {
            cairo_set_line_width(canvas, lineWidth);

            cairo_set_source_rgba(
                canvas, lastState()->m_textDecorationData.underLineColor().R(),
                lastState()->m_textDecorationData.underLineColor().G(),
                lastState()->m_textDecorationData.underLineColor().B(),
                lastState()->m_textDecorationData.underLineColor().A());

            float y = face->underline_position / (float)fc->m_unitsPerEM *
                          intSize / 72 +
                      intSize;
            cairo_move_to(canvas, 0, y + lineWidth / 2);
            cairo_line_to(canvas, rect.width(), y + lineWidth / 2);
            cairo_stroke(canvas);
        }

        if (lastState()->m_textDecorationData.hasLineThrough() == true) {
            cairo_set_line_width(canvas, lineWidth);

            cairo_set_source_rgba(
                canvas,
                lastState()->m_textDecorationData.lineThroughColor().R(),
                lastState()->m_textDecorationData.lineThroughColor().G(),
                lastState()->m_textDecorationData.lineThroughColor().B(),
                lastState()->m_textDecorationData.lineThroughColor().A());

            float y =
                (lastState()->m_font->metrics().m_ascender) -
                intSize * (lastState()->m_font->metrics().m_xheightRate) / 2;

            cairo_move_to(canvas, 0, y);
            cairo_line_to(canvas, rect.width(), y);
            cairo_stroke(canvas);
        }

        cairo_translate(canvas, -dx, -dy);
    }

    virtual bool canRejectPainting(const LayoutRect& rect) override
    {
        double x1, x2;
        double y1, y2;
        cairo_clip_extents(m_canvas, &x1, &y1, &x2, &y2);
        LayoutRect c(x1, y1, x2 - x1, y2 - y1);
        if (c.intersects(rect) == true) {
            return false;
        } else {
            return true;
        }
    }

    virtual void setNeedsNoneAntialias() override
    {
        cairo_set_antialias(m_canvas, CAIRO_ANTIALIAS_NONE);
    }

    virtual void setNeedsFastAntialias() override
    {
        cairo_set_antialias(m_canvas, CAIRO_ANTIALIAS_FAST);
    }

    virtual void setNeedsGoodQualityAntialias() override
    {
        cairo_set_antialias(m_canvas, CAIRO_ANTIALIAS_GOOD);
    }

    void setPathAsNewPathOnCurrentContext(Path* path)
    {
        STARFISH_ASSERT(path != nullptr);

        cairo_new_path(m_canvas);
        PathCairo* pathCairo = (PathCairo*)path;

        auto p = cairo_copy_path(pathCairo->context());
        cairo_append_path(m_canvas, p);
        cairo_path_destroy(p);
    }

    virtual void markDirtyRect(const Unit::Rect& rt) override
    {
        STARFISH_ASSERT(m_surface);
        float xx = rt.x(), yy = rt.y(), ww = rt.width(), hh = rt.height();
        cairo_surface_mark_dirty_rectangle(m_surface, xx, yy, ww, hh);
    }

#if defined(STARFISH_ENABLE_TEST)
    virtual void dump(const char* path) override
    {
        cairo_surface_write_to_png(m_surface, path);
    }
#endif
protected:
    cairo_surface_t* m_surface;
    cairo_t* m_canvas;

    bool m_shouldDestroyCairo;
    bool m_shouldDestroySurface;
    CanvasFlag m_flag{ PlainElement };
};

Canvas* Canvas::create(WebView* webView, CanvasSurface* data, CanvasFlag flag)
{
    STARFISH_ASSERT(webView != nullptr);
    STARFISH_ASSERT(data != nullptr);

    return new CanvasCairo(webView, data, flag);
}

Canvas* Canvas::create(uint8_t* data, size_t w, size_t h, size_t stride,
                       float devicePixelRatio)
{
    STARFISH_ASSERT(data != nullptr);

    return new CanvasCairo(data, w, h, stride, devicePixelRatio);
}

Canvas* Canvas::create(WebView* webView, NativeImageData* data)
{
    STARFISH_ASSERT(webView != nullptr);
    STARFISH_ASSERT(data != nullptr);

    return new CanvasCairo(webView, data);
}

void Canvas::resizeImage(uint8_t* orgBuffer, size_t orgWidth, size_t orgHeight,
                         size_t orgStride, uint8_t* newBuffer, size_t newWidth,
                         size_t newHeight, size_t newStride)
{
    auto imageSurface = cairo_image_surface_create_for_data(
        (unsigned char*)newBuffer, CAIRO_FORMAT, newWidth, newHeight,
        newStride);
    auto orgSurface = cairo_image_surface_create_for_data(
        (unsigned char*)orgBuffer, CAIRO_FORMAT, orgWidth, orgHeight,
        orgStride);
    auto canvas = cairo_create(imageSurface);
    cairo_set_antialias(canvas, CAIRO_ANTIALIAS_GOOD);

    cairo_set_operator(canvas, CAIRO_OPERATOR_CLEAR);
    cairo_set_source_rgba(canvas, 0, 0, 0, 0);
    cairo_set_operator(canvas, CAIRO_OPERATOR_SOURCE);
    cairo_paint(canvas);

    cairo_pattern_t* resizePattern =
        cairo_pattern_create_for_surface(orgSurface);
    cairo_matrix_t matrix;
    cairo_matrix_init_identity(&matrix);
    cairo_matrix_scale(&matrix, orgWidth / (float)newWidth,
                       orgHeight / (float)newHeight);
    cairo_pattern_set_matrix(resizePattern, &matrix);
    cairo_pattern_set_extend(resizePattern, CAIRO_EXTEND_PAD);
    cairo_set_source(canvas, resizePattern);

    cairo_rectangle(canvas, 0, 0, newWidth, newHeight);
    cairo_paint(canvas);

    cairo_pattern_destroy(resizePattern);

    cairo_destroy(canvas);
    cairo_surface_flush(imageSurface);
    cairo_surface_destroy(imageSurface);
    cairo_surface_destroy(orgSurface);
}

class CanvasAttachableNativeImageCairo : public NativeImageData {
public:
    CanvasAttachableNativeImageCairo(CanvasCairo* canvasCairo)
        : m_canvasCairo(canvasCairo)
    {
        STARFISH_ASSERT(canvasCairo != nullptr);
    }

    virtual size_t bufferSize() override
    {
        return m_canvasCairo->m_renderTargetInfo.m_height *
               m_canvasCairo->m_renderTargetInfo.m_stride;
    }

    virtual uint8_t* data() override
    {
        return m_canvasCairo->m_renderTargetInfo.m_buffer;
    }

    virtual void clear() override
    {
    }

    virtual void* unwrap() override
    {
        return m_canvasCairo->m_surface;
    }

    virtual size_t width() override
    {
        return m_canvasCairo->m_renderTargetInfo.m_width;
    }

    virtual size_t height() override
    {
        return m_canvasCairo->m_renderTargetInfo.m_height;
    }

    virtual size_t stride() override
    {
        return m_canvasCairo->m_renderTargetInfo.m_stride;
    }

    virtual bool isAttachableNativeImage() override
    {
        return true;
    }

    virtual void paintContent(Canvas* canvas, const Unit::Rect& dst,
                              ImageRenderingValue imageRenderingMode) override
    {
    }

    void* operator new(size_t size)
    {
        return GC_MALLOC_ATOMIC(sizeof(CanvasAttachableNativeImageCairo));
    }

private:
    CanvasCairo* m_canvasCairo;
};

NativeImageData* NativeImageData::attach(Canvas* canvas)
{
    return new CanvasAttachableNativeImageCairo(castTo<CanvasCairo*>(canvas));
}
} // namespace Starfish

#endif
