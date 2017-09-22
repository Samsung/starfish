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
#include "core/style/UnitHelper.h"
#include "platform/window/PlatformWindow.h"

#include <vector>
#include <SkMatrix.h>
#include <clipper.hpp>

#include <cairo.h>
#include <cairo/cairo-ft.h>

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
        m_surface = cairo_image_surface_create_for_data(
            (unsigned char*)buffer, CAIRO_FORMAT, width, height, stride);
        m_canvas = cairo_create(m_surface);
        m_width = width;
        m_height = height;
    }

public:
    CanvasCairo(StarFish* starfish, void* data)
    {
#if defined(STARFISH_TIZEN) && defined(PORT_GRAPHIC_BACKEND_EFL_CAIRO)
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
        m_viewportWidth = m_width = d->w;
        m_viewportHeight = m_height = d->h;
        m_shouldDestroyCairo = false;
        m_shouldDestroySurface = false;
#else
        m_shouldDestroyCairo = true;
        m_shouldDestroySurface = true;
        m_starfish = starfish;
        m_canvas = nullptr;
        m_surface = nullptr;
        struct dummy {
            void* image;
            int w;
            int h;
            int stride;
        };
        dummy* d = (dummy*)data;
        m_viewportWidth = m_width = d->w;
        m_viewportHeight = m_height = d->h;
        {
            initFromBuffer(d->image, m_width, m_height, d->stride);
        }
#endif
        save();
    }

    CanvasCairo(StarFish* starfish, CanvasSurface* data)
    {
        m_starfish = starfish;
        m_canvas = nullptr;
        m_surface = nullptr;
        m_shouldDestroyCairo = true;
        m_shouldDestroySurface = true;

        initFromBuffer(data->unwrap(), data->bufferWidth(),
                       data->bufferHeight(),
                       cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32,
                                                     data->bufferWidth()));

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

    virtual void clearColor(const Unit::Color& clr)
    {
        cairo_set_source_rgba(m_canvas, clr.R(), clr.G(), clr.B(),
                              clr.A() * lastState().m_opacity);
        cairo_rectangle(m_canvas, 0, 0, m_width, m_height);
        cairo_fill(m_canvas);
    }

    // state
    virtual void save()
    {
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
        save();
        lastState().m_opacity = c * lastState().m_opacity;
    }

    virtual void endOpacityLayer()
    {
        restore();
    }

    virtual void clip(const Unit::Rect& rt)
    {
        cairo_rectangle(m_canvas, rt.x(), rt.y(), rt.width(), rt.height());
        cairo_clip(m_canvas);
        cairo_new_path(m_canvas);
    }

    virtual void setColor(const Unit::Color& clr_)
    {
        STARFISH_ASSERT(m_canvas);
        lastState().m_color = clr_;
        cairo_set_source_rgba(m_canvas, clr_.R(), clr_.G(), clr_.B(),
                              clr_.A() * lastState().m_opacity);
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
        cairo_move_to(m_canvas, p1.x(), p1.y());
        cairo_line_to(m_canvas, p2.x(), p2.y());
        cairo_line_to(m_canvas, p3.x(), p3.y());
        cairo_line_to(m_canvas, p4.x(), p4.y());
        cairo_line_to(m_canvas, p1.x(), p1.y());

        cairo_fill(m_canvas);
    }

    virtual void drawText(LayoutUnit x, LayoutUnit y, LayoutUnit stringWidth,
                          const StringView& sv)
    {
        int size = lastState().m_font->size();
        if (!lastState().m_visible || size == 0) {
            return;
        }
#if defined(PORT_CANVAS_BACKEND_EFL)
        if (!lastState().m_font->isGenericFont()) {
            Font* nonGenericFont = lastState().m_font;
            Font* font = m_starfish->fetchGenericFont(
                nonGenericFont->familyName(), nonGenericFont->size(),
                nonGenericFont->style(), nonGenericFont->weight());
            setFont(font);
        }
        // force measure text for fill internal glyph cache
        lastState().m_font->measureText(sv);
#endif

#ifdef STARFISH_ENABLE_TEST
        if (g_enablePixelTest) {
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
                                drawRect(
                                    LayoutLocation(xx + offset - littleLeft, y),
                                    LayoutLocation(xx + h + offset - littleLeft,
                                                   y),
                                    LayoutLocation(xx + h - littleLeft, y + h),
                                    LayoutLocation(xx - littleLeft, y + h));
                            } else {
                                // left, top, w, h
                                Unit::Rect rt(xx, y, h, h);
                                drawRect(rt);
                            }
                        } else {
                            // To sync with phantom-webkit
                            int ph = h * 0.2;
                            drawRect(Unit::Rect(xx, y + h - ph, h, ph));
                            /*
                            save();
                            clip(Rect(xx, y, h, h));
                            g_enablePixelTest = false;
                            drawText(xx, y, String::createASCIIString("p"));
                            g_enablePixelTest = true;
                            restore();
                            */
                        }
                    }
                    xx += h;
                }
            }
            return;
        }
#endif

        LayoutSize sz(stringWidth, lastState().m_font->metrics().m_fontHeight);
        // if (lastState().m_mapMode) {
        //     sz.setWidth(lastState().m_font->measureText(sv));
        // }
        LayoutRect rt(x, y, sz.width(), sz.height());

        LayoutUnit xx = 0, yy = 0;
        xx = rt.x();
        yy = rt.y();

        Unit::Rect test((float)xx, (float)yy, (float)rt.width(),
                        (float)rt.height());
        Unit::Rect canvasSize(0, 0, m_width, m_height);
        if (canvasSize.contains(test.x(), test.y()) ||
            canvasSize.contains(test.maxX(), test.y()) ||
            canvasSize.contains(test.x(), test.maxY()) ||
            canvasSize.contains(test.maxX(), test.maxY())) {
        } else {
            return;
        }

        cairo_save(m_canvas);
        cairo_translate(m_canvas, xx, yy);

        FT_Int x_bias = 0;
        FT_Int y_bias = 0;
        FT_UInt glyph_index = 0;

        FT_Face face =
            lastState().m_font->findFCChar(sv.charAt(0), &glyph_index);
        ;
        cairo_font_face_t* fontFace;
        int glyph_count = sv.length();

        fontFace = cairo_ft_font_face_create_for_ft_face(face, 0);
        cairo_set_font_face(m_canvas, fontFace);
        cairo_set_font_size(m_canvas, size);
        cairo_translate(m_canvas, 0, size);

        for (int i = 0; i < glyph_count; i++) {
            cairo_glyph_t glyph;
            glyph_index = FT_Get_Char_Index(face, sv.charAt(i));
            if (glyph_index != 0) {
                glyph.x = x_bias;
                glyph.y = 0;
                x_bias += lastState().m_font->getGlaphAdvanceX(sv.charAt(i));
            } else {
                face =
                    lastState().m_font->findFCChar(sv.charAt(i), &glyph_index);
                if (glyph_index != 0) {
                    cairo_font_face_destroy(fontFace);
                    fontFace = cairo_ft_font_face_create_for_ft_face(face, 0);
                    cairo_set_font_face(m_canvas, fontFace);
                    cairo_set_font_size(m_canvas, size);
                    glyph.x = x_bias;
                    glyph.y = 0;
                    x_bias +=
                        lastState().m_font->getGlaphAdvanceX(sv.charAt(i));
                } else {
                    glyph.x = 0;
                    glyph.y = 0;
                }
            }
            glyph.index = glyph_index;
            cairo_glyph_path(m_canvas, &glyph, 1);
        }
        cairo_fill(m_canvas);

        if (lastState().m_textDecorationData.hasUnderLine()) {
            cairo_set_source_rgba(
                m_canvas,
                lastState().m_textDecorationData.underLineColor().r() / 255.f,
                lastState().m_textDecorationData.underLineColor().g() / 255.f,
                lastState().m_textDecorationData.underLineColor().b() / 255.f,
                lastState().m_textDecorationData.underLineColor().a() / 255.f);
            cairo_move_to(m_canvas, 0, cairo_get_line_width(m_canvas));
            cairo_line_to(m_canvas, rt.width(), cairo_get_line_width(m_canvas));
            cairo_stroke(m_canvas);
        }

        if (lastState().m_textDecorationData.hasLineThrough()) {
            cairo_set_source_rgba(
                m_canvas,
                lastState().m_textDecorationData.lineThroughColor().r() / 255.f,
                lastState().m_textDecorationData.lineThroughColor().g() / 255.f,
                lastState().m_textDecorationData.lineThroughColor().b() / 255.f,
                lastState().m_textDecorationData.lineThroughColor().a() /
                    255.f);
            cairo_move_to(m_canvas, 0,
                          -(lastState().m_font->metrics().m_ascender / 2));
            cairo_line_to(m_canvas, rt.width(),
                          -(lastState().m_font->metrics().m_ascender / 2));
            cairo_stroke(m_canvas);
        }
        cairo_font_face_destroy(fontFace);
        cairo_restore(m_canvas);
    }

    void drawImageCairo(cairo_surface_t* localSurface, const Unit::Rect& dst,
                        double surfaceWidth, double surfaceHeight,
                        bool isFromSurface = false)
    {
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
        cairo_pattern_set_filter(resizePattern, CAIRO_FILTER_NEAREST);

        cairo_set_source(m_canvas, resizePattern);

        cairo_rectangle(m_canvas, 0, 0, ww, hh);
        cairo_clip(m_canvas);
        cairo_paint_with_alpha(m_canvas, lastState().m_opacity);

        // drawDebugLine(xx,yy,ww,hh);
        cairo_restore(m_canvas);
        cairo_pattern_destroy(resizePattern);
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
        cairo_surface_t* image = nullptr;

        int stride = cairo_format_stride_for_width(CAIRO_FORMAT, data->width());
        image = cairo_image_surface_create_for_data((unsigned char*)imgData,
                                                    CAIRO_FORMAT, data->width(),
                                                    data->height(), stride);
        surfaceWidth = data->width();
        surfaceHeight = data->height();
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
            (unsigned char*)data->unwrap(), CAIRO_FORMAT_ARGB32,
            data->bufferWidth(), data->height(),
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
        // TODO : It's not implemented yet!
        drawImage(data, dst);
        return;
    }

    virtual void drawRepeatImage(ImageData* data, const Unit::Rect& dst,
                                 float imageWidth, float imageHeight,
                                 bool xRepeat, bool yRepeat, bool isRootElement)
    {
        if (!lastState().m_visible) {
            return;
        }

        cairo_save(m_canvas);
        float xx = 0.0, yy = 0.0, ww = 0.0, hh = 0.0;
        ww = dst.width();
        hh = dst.height();

        float x = 0.0, y = 0.0;
        if (xRepeat) {
            x = (dst.x() - floor(dst.x() / imageWidth) * imageWidth) -
                imageWidth;
            if (isRootElement) {
                x += xx;
            }
        } else {
            xx += dst.x();
        }
        if (yRepeat) {
            y = (dst.y() - floor(dst.y() / imageHeight) * imageHeight) -
                imageHeight;
            if (isRootElement) {
                y += yy;
            }
        } else {
            yy += dst.y();
        }

        cairo_pattern_t* pattern;
        cairo_matrix_t matrix;
        cairo_surface_t* image = nullptr;

        void* imgData = data->unwrap();
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
            // TODO
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
        cairo_paint_with_alpha(m_canvas, lastState().m_opacity);

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
        cairo_matrix_init(&b_matrix, matrix.getScaleX(), matrix.getSkewY(),
                          matrix.getSkewX(), matrix.getScaleY(),
                          matrix.getTranslateX(), matrix.getTranslateY());
        cairo_matrix_multiply(&result_matrix, &b_matrix, &a_matrix);
        cairo_set_matrix(m_canvas, &result_matrix);
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

    virtual void arc(double xc, double yc, double radius, double angle1,
                     double angle2)
    {
        cairo_arc(m_canvas, xc, yc, radius, angle1, angle2);
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

protected:
    StarFish* m_starfish;
    std::vector<CanvasStateCairo> m_state;
    cairo_surface_t* m_surface;
    cairo_t* m_canvas;
    unsigned m_width;
    unsigned m_height;

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
    struct dummy {
        void* image;
        int w;
        int h;
        int stride;
    } d;
    d.image = data;
    d.w = w;
    d.h = h;
    d.stride = w * 4;
    return new CanvasCairo(starfish, &d);
}
}

#endif
