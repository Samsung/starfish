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

#if defined(PORT_GRAPHIC_BACKEND_GENERAL_BUFFER) && \
    defined(PORT_CANVAS_BACKEND_CAIRO)

#include "core/modules/canvas/Canvas.h"
#include "core/modules//canvas/font/Font.h"
#include "core/modules//canvas/image/ImageData.h"
#include "core/style/UnitHelper.h"

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
    cairo_matrix_t m_matrix;
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
        m_buffer = buffer;
    }

public:
    CanvasCairo(void* data)
    {
        m_canvas = nullptr;
        m_surface = nullptr;
        m_buffer = NULL;
        m_directDraw = true;
        struct dummy {
            void* image;
            int w;
            int h;
            int stride;
        };
        dummy* d = (dummy*)data;
        m_width = d->w;
        m_height = d->h;
        {
            m_buffer = d->image;
            initFromBuffer(m_buffer, m_width, m_height, d->stride);
        }

        save();
    }

    CanvasCairo(CanvasSurface* data)
    {
        m_canvas = nullptr;
        m_surface = nullptr;
        m_directDraw = false;

        m_buffer = (void*)data->unwrap();
        initFromBuffer(
            m_buffer, data->width(), data->height(),
            cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, data->width()));

        save();
    }

    ~CanvasCairo()
    {
        restore();
        m_statePerFrame.clear();
        STARFISH_ASSERT(m_state.size() == 0);
        cairo_destroy(m_canvas);
        cairo_surface_flush(m_surface);
        cairo_surface_destroy(m_surface);
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
            state.m_baseX = lastState.m_baseX;
            state.m_baseY = lastState.m_baseY;
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

    virtual void saveByFrame(Frame* f)
    {
        cairo_get_matrix(m_canvas, &lastState().m_matrix);
        m_statePerFrame.emplace(f, lastState());
    }

    virtual CanvasState* getByFrame(Frame* f)
    {
        auto it = m_statePerFrame.find(f);
        if (it == m_statePerFrame.end()) {
            return nullptr;
        }
        return &it->second;
    }

    virtual void replace(CanvasState* state, ReplaceFlag flag)
    {
        CanvasStateCairo* cairoState = (CanvasStateCairo*)state;
        auto& lastState = m_state.back();

        // FIXME: Should replace clipping information here!

        if (flag == ReplaceFlag::All) {
            cairo_set_matrix(m_canvas, &cairoState->m_matrix);
            lastState.m_color = cairoState->m_color;
            lastState.m_opacity = cairoState->m_opacity;
            lastState.m_baseX = cairoState->m_baseX;
            lastState.m_baseY = cairoState->m_baseY;
            lastState.m_font = cairoState->m_font;
            lastState.m_visible = cairoState->m_visible;
            lastState.m_textDecorationData = cairoState->m_textDecorationData;
        }
    }

    virtual void assureMapMode()
    {
    }

    // transformations (default transform is the identity matrix)
    virtual void scale(double x, double y)
    {
        cairo_scale(m_canvas, x, y);
    }

    virtual void scale(double x, double y, double ox, double oy)
    {
        // TODO
    }

    virtual void rotate(double angle)
    {
        cairo_rotate(m_canvas, angle);
    }

    virtual void rotate(double angle, double ox, double oy)
    {
        // TODO
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
        if (!lastState().m_visible) {
            return;
        }

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

        UTF8StringDataNonGCStd us =
            sv.originalString()->toUTF8NonGCString(sv.start(), sv.end(), true);

        FT_Face face = lastState().m_font->metrics().m_FTFace;
        cairo_font_face_t* fontFace;
        fontFace = cairo_ft_font_face_create_for_ft_face(face, 0);
        int size = lastState().m_font->size();

        cairo_set_font_face(m_canvas, fontFace);
        cairo_set_font_size(m_canvas, size);
        auto scaled_face = cairo_get_scaled_font(m_canvas);

        // get glyphs for the text
        cairo_glyph_t* glyphs = NULL;
        int glyph_count;
        cairo_text_cluster_t* clusters = NULL;
        int cluster_count;
        cairo_text_cluster_flags_t clusterflags;

        auto stat = cairo_scaled_font_text_to_glyphs(
            scaled_face, 0, 0, us.c_str(), strlen(us.c_str()), &glyphs,
            &glyph_count, &clusters, &cluster_count, &clusterflags);

        // check if conversion was successful
        if (stat == CAIRO_STATUS_SUCCESS) {
            // text paints on bottom line
            cairo_translate(m_canvas, 0, size);
            // draw each cluster
            int glyph_index = 0;
            int byte_index = 0;
            for (int i = 0; i < cluster_count; i++) {
                cairo_text_cluster_t* cluster = &clusters[i];
                cairo_glyph_t* clusterglyphs = &glyphs[glyph_index];

                // get extents for the glyphs in the cluster
                cairo_text_extents_t extents;
                cairo_scaled_font_glyph_extents(scaled_face, clusterglyphs,
                                                cluster->num_glyphs, &extents);
                cairo_glyph_path(m_canvas, clusterglyphs, cluster->num_glyphs);
                cairo_fill(m_canvas);
                // glyph/byte position
                glyph_index += cluster->num_glyphs;
                byte_index += cluster->num_bytes;
            }
        }
        cairo_glyph_free(glyphs);
        cairo_text_cluster_free(clusters);

        cairo_restore(m_canvas);
    }

    void drawImageCairo(cairo_surface_t* localSurface, const Unit::Rect& dst,
                        double surfaceWidth, double surfaceHeight,
                        bool isFromSurface = false)
    {
        cairo_save(m_canvas);

        float xx = 0.0, yy = 0.0, ww = 0.0, hh = 0.0;
        xx = dst.x() + lastState().m_baseX;
        yy = dst.y() + lastState().m_baseY;
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

        void* imgData = data->unwrap();
        double surfaceWidth = 0, surfaceHeight = 0;
        cairo_surface_t* image = nullptr;

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
            (unsigned char*)data->unwrap(), CAIRO_FORMAT, data->width(),
            data->height(),
            cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, data->width()));

        drawImageCairo(image, dst, data->width(), data->height(), true);
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
        cairo_matrix_multiply(&result_matrix, &a_matrix, &b_matrix);
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

protected:
    std::vector<CanvasStateCairo> m_state;
    std::unordered_map<Frame*, CanvasStateCairo> m_statePerFrame;
    cairo_surface_t* m_surface;
    cairo_t* m_canvas;
    bool m_directDraw;
    void* m_buffer;
    unsigned m_width;
    unsigned m_height;
};

Canvas* Canvas::createDirect(void* data)
{
    return new CanvasCairo(data);
}

Canvas* Canvas::create(CanvasSurface* data)
{
    return new CanvasCairo(data);
}
}

#endif
