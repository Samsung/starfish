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

#if defined(PORT_CANVAS_BACKEND_CAIRO)

#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/font/Font.h"
#include "core/modules/canvas/image/ImageData.h"
#include "core/modules/canvas/ShadowBlur.h"
#include "core/style/UnitHelper.h"
#include "platform/window/PlatformWindow.h"

#include "platform/canvas/font/FontImplCairo.h"

#include <vector>
#include <SkMatrix.h>
#include <clipper.hpp>

#include <cairo.h>
#include <cairo/cairo-ft.h>

#include <hb.h>

#include "core/modules/profiling/Profiling.h"

#define CAIRO_FORMAT CAIRO_FORMAT_ARGB32
// #define STARFISH_ENABLE_TIMER

#ifdef STARFISH_ENABLE_TIMER
#define INSTALL_PROFILE_TIMER(s) ProfilerTimer _p(s);
#else
#define INSTALL_PROFILE_TIMER(s)
#endif

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
        m_surface = cairo_image_surface_create_for_data(
            (unsigned char*)buffer, CAIRO_FORMAT, width, height, stride);
        m_canvas = cairo_create(m_surface);
        m_width = width;
        m_height = height;
    }
    void init()
    {
        cairo_set_antialias(m_canvas, CAIRO_ANTIALIAS_FAST);
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
        m_width = width;
        m_height = height;
        {
            initFromBuffer(buffer, m_width, m_height, stride);
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
        m_width = d->w;
        m_height = d->h;
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
                       cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32,
                                                     data->bufferWidth()));

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
        INSTALL_PROFILE_TIMER("CanvasImplCairo::clearColor");
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
        INSTALL_PROFILE_TIMER("CanvasImplCairo::beginOpacityLayer");
        save();
        lastState().m_opacity = c;
        cairo_push_group(m_canvas);
    }

    virtual void endOpacityLayer()
    {
        INSTALL_PROFILE_TIMER("CanvasImplCairo::endOpacityLayer");
        cairo_pop_group_to_source(m_canvas);
        cairo_paint_with_alpha(m_canvas, lastState().m_opacity);
        restore();
    }

    virtual void clip(const Unit::Rect& rt)
    {
        cairo_rectangle(m_canvas, rt.x(), rt.y(), rt.width(), rt.height());
        cairo_clip(m_canvas);
    }

    virtual void setColor(const Unit::Color& clr_)
    {
        STARFISH_ASSERT(m_canvas);
        lastState().m_color = clr_;
        cairo_set_source_rgba(m_canvas, clr_.R(), clr_.G(), clr_.B(), clr_.A());
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

    virtual void setTextShadowData(CanvasShadowDataList& list)
    {
        m_textShadowDataList = list;
    }

    virtual void clearTextShadowData()
    {
        m_textShadowDataList.clear();
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
        INSTALL_PROFILE_TIMER("CanvasImplCairo::drawCairoRect");
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
        int xx = 0, yy = 0, ww = 0, hh = 0;
        LayoutUnit rx = rt.x();
        LayoutUnit ry = rt.y();

        xx = rx.floor();
        yy = ry.floor();
        ww = snapSizeToPixel(rt.width(), rx);
        hh = snapSizeToPixel(rt.height(), ry);
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

        INSTALL_PROFILE_TIMER("CanvasImplCairo::drawText");

        LayoutSize sz(stringWidth, lastState().m_font->metrics().m_fontHeight);
        LayoutRect rt(x, y, sz.width(), sz.height());

        if (m_textShadowDataList.size() == 0) {
            double x1, x2;
            double y1, y2;
            cairo_clip_extents(m_canvas, &x1, &y1, &x2, &y2);
            LayoutRect c(x1, y1, x2 - x1, y2 - y1);
            if (c.contains(rt.x(), rt.y()) || c.contains(rt.maxX(), rt.y()) ||
                c.contains(rt.x(), rt.maxY()) ||
                c.contains(rt.maxX(), rt.maxY())) {
            } else {
                return;
            }
        }

        cairo_save(m_canvas);
        if (m_textShadowDataList.size()) {
            for (auto& sd : m_textShadowDataList) {
                drawTextInner(rt, sv, &sd);
            }
        }

        drawTextInner(rt, sv);
        cairo_restore(m_canvas);
    }

    void drawImageCairo(cairo_surface_t* localSurface, const Unit::Rect& dst,
                        double surfaceWidth, double surfaceHeight,
                        bool isFromSurface = false)
    {
        INSTALL_PROFILE_TIMER("CanvasImplCairo::drawImageCairo");

        cairo_save(m_canvas);

        float xx = 0.0, yy = 0.0, ww = 0.0, hh = 0.0;
        xx = dst.x();
        yy = dst.y();
        ww = dst.width();
        hh = dst.height();

        cairo_pattern_t* resizePattern;
        cairo_matrix_t matrix;

        resizePattern = cairo_pattern_create_for_surface(localSurface);
        cairo_translate(m_canvas, xx, yy);

        cairo_matrix_init_identity(&matrix);
        cairo_matrix_scale(&matrix, surfaceWidth / ww, surfaceHeight / hh);
        cairo_pattern_set_matrix(resizePattern, &matrix);
        cairo_pattern_set_filter(resizePattern, CAIRO_FILTER_FAST);

        cairo_set_source(m_canvas, resizePattern);

        cairo_rectangle(m_canvas, 0, 0, ww, hh);
        cairo_clip(m_canvas);

        cairo_paint(m_canvas);

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

    virtual void drawImage(ImageData* data, const Unit::Rect& dst)
    {
        if (!lastState().m_visible) {
            return;
        }

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

    virtual void drawImage(CanvasSurface* data, const Unit::Rect& dst)
    {
        if (!lastState().m_visible) {
            return;
        }
        cairo_surface_t* image;
        image = cairo_image_surface_create_for_data(
            (unsigned char*)data->data(), CAIRO_FORMAT_ARGB32,
            data->bufferWidth(), data->bufferHeight(),
            cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32,
                                          data->bufferWidth()));

        drawImageCairo(image, dst, data->bufferWidth(), data->bufferHeight(),
                       true);
        cairo_surface_destroy(image);
    }

    virtual void drawBorderImage(ImageData* data, const Unit::Rect& dst,
                                 size_t l, size_t t, size_t r, size_t b,
                                 double scale, bool fill)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        drawImage(data, dst);
        return;
    }

    virtual void drawRepeatImage(ImageData* data, const Unit::Rect& dst,
                                 float imageWidth, float imageHeight,
                                 bool xRepeat, bool yRepeat)
    {
        if (!lastState().m_visible) {
            return;
        }

        INSTALL_PROFILE_TIMER("CanvasImplCairo::drawRepeatImage");

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
            int stride =
                cairo_format_stride_for_width(CAIRO_FORMAT, data->width());
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
        cairo_stroke(m_canvas);
    }
    virtual void strokePreserve()
    {
        cairo_stroke_preserve(m_canvas);
    }
    virtual void fill()
    {
        cairo_fill(m_canvas);
    }
    virtual void fillPreserve()
    {
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
                          const StringView& sv)
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
                    xx += h;
                }
            }
        }
    }
#endif
    void drawGlyphsCairo(cairo_t* canvas, LayoutRect rect, const StringView& sv)
    {
        LayoutUnit xBias = 0;
        FT_UInt glyph_index = 0;
        FT_Face lastFontFace = nullptr;
        cairo_font_face_t* fontFace = nullptr;
        FontImplCairo* f = (FontImplCairo*)lastState().m_font;
        int size = lastState().m_font->size();

        cairo_translate(canvas, 0, lastState().m_font->metrics().m_ascender);
        cairo_glyph_t* glyphs = nullptr;
        size_t glyphCount = 0;

        if (cairoBackendCanUseSimpleFontPath(f, sv)) {
            glyphs = ALLOCA(sv.length() * sizeof(cairo_glyph_t), cairo_glyph_t);
            for (size_t i = 0; i < sv.length(); i++) {
                std::pair<std::pair<FontFaceImplCairo*, size_t>,
                          std::pair<unsigned, LayoutUnit>>
                    g = cairoBackendInternalLoadGlyph(f, sv.charAt(i));
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
                    glyphs[glyphCount].x = xBias;
                    glyphs[glyphCount].y = 0;
                    glyphCount++;
                    STARFISH_ASSERT(glyphCount <= sv.length());
                    xBias += g.second.second;
                } else {
                    if (true) { // skip webfont enabled
                        if (f->seenUnresolvedWebFontIndex() != SIZE_MAX) {
                            xBias += lastState().m_font->spaceWidth();
                            continue;
                        }
                    }
                    cairo_save(canvas);
                    cairo_set_line_width(canvas, 1);
                    cairo_new_path(canvas);
                    cairo_rectangle(canvas, xBias,
                                    -lastState().m_font->metrics().m_ascender,
                                    lastState().m_font->spaceWidth(),
                                    lastState().m_font->metrics().m_fontHeight);
                    cairo_stroke(canvas);
                    cairo_restore(canvas);
                    xBias += lastState().m_font->spaceWidth();
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

                if (run.m_ftFace == nullptr) {
                    if (/* skip webfont enabled*/ f
                            ->seenUnresolvedWebFontIndex() != SIZE_MAX) {
                    } else {
                        cairo_save(canvas);
                        cairo_set_line_width(canvas, 1);
                        for (size_t j = 0; j < run.m_text.length(); j++) {
                            cairo_new_path(canvas);
                            cairo_rectangle(
                                canvas,
                                xBias + j * lastState().m_font->spaceWidth(),
                                -lastState().m_font->metrics().m_ascender,
                                lastState().m_font->spaceWidth(),
                                lastState().m_font->metrics().m_fontHeight);
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
                            glyphs[glyphCount].x =
                                run.m_glyphPositions[j].x() + xBias;
                            glyphs[glyphCount].y = run.m_glyphPositions[j].y();
                            STARFISH_ASSERT(glyphCount < glyphAllocCount);
                            glyphCount++;
                        }
                    }
                }

                xBias += run.m_runWidth;
            }
        }
        cairo_show_glyphs(canvas, glyphs, glyphCount);
        cairo_font_face_destroy(fontFace);
        cairo_translate(canvas, 0, -lastState().m_font->metrics().m_ascender);
    }
    void drawTextDecorationCairo(cairo_t* canvas, LayoutRect rect,
                                 const StringView& sv,
                                 CanvasShadowData* shadow = nullptr)
    {
        FontImplCairo* f = (FontImplCairo*)lastState().m_font;
        FontFaceImplCairo* fc = (FontFaceImplCairo*)f->fontFaceList()[0];
        FT_Face face = fc->freetypeFace();
        int intSize(f->size() + 0.5f);

        float lineWidth =
            face->underline_thickness / (float)fc->m_unitsPerEM * intSize;
        if (lastState().m_textDecorationData.hasUnderLine()) {
            cairo_set_line_width(canvas, lineWidth);
            if (!shadow) {
                cairo_set_source_rgba(
                    canvas,
                    lastState().m_textDecorationData.underLineColor().R(),
                    lastState().m_textDecorationData.underLineColor().G(),
                    lastState().m_textDecorationData.underLineColor().B(),
                    lastState().m_textDecorationData.underLineColor().A());
            }
            float y = face->underline_position / (float)fc->m_unitsPerEM *
                          intSize / 72 +
                      intSize;
            cairo_move_to(canvas, 0, y + lineWidth / 2);
            cairo_line_to(canvas, rect.width(), y + lineWidth / 2);
            cairo_stroke(canvas);
        }

        if (lastState().m_textDecorationData.hasLineThrough()) {
            cairo_set_line_width(canvas, lineWidth);
            if (!shadow) {
                cairo_set_source_rgba(
                    canvas,
                    lastState().m_textDecorationData.lineThroughColor().R(),
                    lastState().m_textDecorationData.lineThroughColor().G(),
                    lastState().m_textDecorationData.lineThroughColor().B(),
                    lastState().m_textDecorationData.lineThroughColor().A());
            }
            float y =
                (lastState().m_font->metrics().m_ascender) -
                intSize * (lastState().m_font->metrics().m_xheightRate) / 2;

            cairo_move_to(canvas, 0, y);
            cairo_line_to(canvas, rect.width(), y);
            cairo_stroke(canvas);
        }
    }
    void drawTextInner(LayoutRect rect, const StringView& sv,
                       CanvasShadowData* shadow = nullptr)
    {
        cairo_save(m_canvas);
        LayoutUnit xx = rect.x(), yy = rect.y();
        cairo_t* canvas = nullptr;
        cairo_surface_t* surfaceForBlur = nullptr;
        float radiusOffset = 0.0f;

        if (shadow) {
            Unit::Color color;
            if (shadow->radius()) {
                radiusOffset = shadow->radius();
                radiusOffset = std::min(ShadowBlur::RADIUS_LIMIT, radiusOffset);
                radiusOffset *= 2;
            }
            if (shadow->hasColor()) {
                color = shadow->color();
            } else {
                color = lastState().m_color;
            }

            surfaceForBlur = cairo_surface_create_similar(
                cairo_get_target(m_canvas), CAIRO_CONTENT_COLOR_ALPHA,
                ceil(rect.width().toFloat() + radiusOffset),
                ceil(rect.height().toFloat() + radiusOffset));

            canvas = cairo_create(surfaceForBlur);

            cairo_set_source_rgba(canvas, color.R(), color.G(), color.B(),
                                  color.A());
        } else {
            canvas = m_canvas;
        }

        if (radiusOffset > 0.0f) {
            xx = xx + ceil(radiusOffset / 2);
            yy = yy + ceil(radiusOffset / 2);
        }
        cairo_translate(canvas, xx.toDouble(), yy.toDouble());

#ifdef STARFISH_ENABLE_TEST
        if (g_enablePixelTest) {
            drawAhemBoxCairo(canvas, rect, sv);
        } else {
            drawGlyphsCairo(canvas, rect, sv);
            drawTextDecorationCairo(canvas, rect, sv, shadow);
        }
#else
        drawGlyphsCairo(canvas, rect, sv);
        drawTextDecorationCairo(canvas, rect, sv, shadow);
#endif
        if (shadow) {
            if (shadow->radius()) {
                int width = cairo_image_surface_get_width(surfaceForBlur);
                int height = cairo_image_surface_get_height(surfaceForBlur);
                int stride = cairo_image_surface_get_stride(surfaceForBlur);
                cairo_format_t format =
                    cairo_image_surface_get_format(surfaceForBlur);
                unsigned char* data =
                    cairo_image_surface_get_data(surfaceForBlur);

                if (data && format == CAIRO_FORMAT) {
                    ShadowBlur sb(data, width, height, stride, 4);
                    if (sb.process(shadow->radius())) {
                        cairo_surface_mark_dirty(surfaceForBlur);
                    }
                }
            }

            cairo_set_source_surface(m_canvas, surfaceForBlur,
                                     shadow->offsetX() - ceil(radiusOffset / 2),
                                     shadow->offsetY() -
                                         ceil(radiusOffset / 2));
            cairo_paint(m_canvas);
            cairo_surface_destroy(surfaceForBlur);
            cairo_destroy(canvas);
        }
        cairo_restore(m_canvas);
    }

protected:
    StarFish* m_starfish;
    std::vector<CanvasStateCairo> m_state;
    cairo_surface_t* m_surface;
    cairo_t* m_canvas;
    unsigned m_width;
    unsigned m_height;
    CanvasShadowDataList m_textShadowDataList;

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
}

#endif
