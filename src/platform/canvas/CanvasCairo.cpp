/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#if defined(PORT_GRAPHIC_BACKEND_DALI) && defined(PORT_CANVAS_BACKEND_CAIRO)

#include "core/modules/canvas/Canvas.h"
#include "core/modules//canvas/font/Font.h"
#include "core/modules//canvas/image/ImageData.h"
#include "core/style/UnitHelper.h"

#include <vector>
#include <SkMatrix.h>
#include <clipper.hpp>

#include <cairo.h>
#include <cairo/cairo-ft.h>
#include <dali-toolkit/dali-toolkit.h>

#define CAIRO_FORMAT CAIRO_FORMAT_ARGB32

// TODO : Should remove !
#include <Evas.h>
#include <Evas_Engine_Buffer.h>
#include <Elementary.h>
#include <Ecore_X.h>
Evas* g_internalCanvas;

namespace StarFish {

// TODO : Should remove !
Evas* internalCanvas()
{
    STARFISH_RELEASE_ASSERT(g_internalCanvas);
    return g_internalCanvas;
}

extern bool g_enablePixelTest;

class CanvasState {
public:
    SkMatrix m_matrix;
    Unit::Color m_color;
    // Evas_Object* m_clipper;
    SkRect m_clipRect;
    ClipperLib::Paths m_clipPath;
    float m_opacity;
    Font* m_font;
    LayoutUnit m_baseX;
    LayoutUnit m_baseY;
    Unit::Color m_underLineColor;
    Unit::Color m_lineThroughColor;

    bool m_mapMode;
    bool m_didClip;
    bool m_hasPathClip;
    bool m_visible;
    bool m_hasUnderLine;
    bool m_hasLineThrough;

    CanvasState()
    {
        // m_clipper = NULL;
        m_opacity = 1;
        m_font = nullptr;
        m_mapMode = false;
        m_didClip = false;
        m_hasPathClip = false;
        m_visible = true;
        m_hasUnderLine = false;
        m_hasLineThrough = false;
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
        m_imageCount = 0;
        m_buffer = NULL;
        m_directDraw = true;
        struct dummy {
            Dali::BufferImage image;
            int w;
            int h;
        };
        dummy* d = (dummy*)data;
        m_width = d->w;
        m_height = d->h;
        {
            m_buffer = (void*)d->image.GetBuffer();
            initFromBuffer(m_buffer, m_width, m_height,
                           d->image.GetBufferStride());
        }

        save();
    }

    CanvasCairo(CanvasSurface* data)
    {
        m_canvas = nullptr;
        m_surface = nullptr;
        m_imageCount = 0;
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
        STARFISH_ASSERT(m_state.size() == 0);
        cairo_destroy(m_canvas);
        cairo_surface_flush(m_surface);
        cairo_surface_destroy(m_surface);
    }

    virtual void clearColor(const Unit::Color& clr)
    {
        cairo_save(m_canvas);
        applyClippers();
        cairo_new_path(m_canvas);

        cairo_set_source_rgba(m_canvas, clr.R(), clr.G(), clr.B(), clr.A());
        cairo_rectangle(m_canvas, 0, 0, m_width, m_height);
        cairo_fill(m_canvas);
        cairo_restore(m_canvas);
    }

    // state
    virtual void save()
    {
        CanvasState state;
        if (m_state.size()) {
            state.m_matrix = lastState().m_matrix;
            state.m_clipRect = lastState().m_clipRect;
            state.m_clipPath = lastState().m_clipPath;
            // state.m_clipper = lastState().m_clipper;
            state.m_color = lastState().m_color;
            state.m_opacity = lastState().m_opacity;
            state.m_baseX = lastState().m_baseX;
            state.m_baseY = lastState().m_baseY;
            state.m_font = lastState().m_font;
            state.m_mapMode = lastState().m_mapMode;
            state.m_visible = lastState().m_visible;
            state.m_didClip = lastState().m_didClip;
            state.m_hasPathClip = lastState().m_hasPathClip;
            state.m_hasUnderLine = lastState().m_hasUnderLine;
            state.m_hasLineThrough = lastState().m_hasLineThrough;
            state.m_underLineColor = lastState().m_underLineColor;
            state.m_lineThroughColor = lastState().m_lineThroughColor;
        } else {
            state.m_matrix.reset();
            state.m_clipRect.setLTRB(0, 0, SkFloatToScalar((float)m_width),
                                     SkFloatToScalar((float)m_height));
            // state.m_clipper = NULL;
        }
        m_state.push_back(state);
    }

    // pop state stack and restore state
    virtual void restore()
    {
        m_state.erase(m_state.end() - 1);
    }

    virtual void assureMapMode()
    {
        if (lastState().m_mapMode) {
            return;
        }
        lastState().m_matrix.preTranslate(lastState().m_baseX,
                                          lastState().m_baseY);
        lastState().m_mapMode = true;
    }

    // transformations (default transform is the identity matrix)
    virtual void scale(double x, double y)
    {
        assureMapMode();
        lastState().m_matrix.preScale(SkDoubleToScalar(x), SkDoubleToScalar(y));
    }

    virtual void scale(double x, double y, double ox, double oy)
    {
        assureMapMode();
        lastState().m_matrix.preScale(SkDoubleToScalar(x), SkDoubleToScalar(y),
                                      SkDoubleToScalar(ox),
                                      SkDoubleToScalar(oy));
    }

    virtual void rotate(double angle)
    {
        assureMapMode();
        lastState().m_matrix.preRotate(SkDoubleToScalar(angle));
    }

    virtual void rotate(double angle, double ox, double oy)
    {
        assureMapMode();
        lastState().m_matrix.preRotate(SkDoubleToScalar(angle),
                                       SkDoubleToScalar(ox),
                                       SkDoubleToScalar(oy));
    }

    virtual void translate(double x, double y)
    {
        if (lastState().m_mapMode) {
            lastState().m_matrix.preTranslate(x, y);
        } else {
            lastState().m_baseX = lastState().m_baseX.toDouble() + x;
            lastState().m_baseY = lastState().m_baseY.toDouble() + y;
        }
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
        if (lastState().m_hasPathClip) {
            ClipperLib::Clipper clipper;

            clipper.AddPaths(lastState().m_clipPath,
                             ClipperLib::PolyType::ptSubject, true);

            ClipperLib::Path path;
            STARFISH_ASSERT(lastState().m_mapMode);
            SkPoint pt;
            pt = SkPoint::Make(rt.x(), rt.y());
            lastState().m_matrix.mapPoints(&pt, 1);
            path.emplace_back(pt.x(), pt.y());

            pt = SkPoint::Make(rt.x() + rt.width(), rt.y());
            lastState().m_matrix.mapPoints(&pt, 1);
            path.emplace_back(pt.x(), pt.y());

            pt = SkPoint::Make(rt.x() + rt.width(), rt.y() + rt.height());
            lastState().m_matrix.mapPoints(&pt, 1);
            path.emplace_back(pt.x(), pt.y());

            pt = SkPoint::Make(rt.x(), rt.y() + rt.height());
            lastState().m_matrix.mapPoints(&pt, 1);
            path.emplace_back(pt.x(), pt.y());

            clipper.AddPath(path, ClipperLib::PolyType::ptClip, true);

            ClipperLib::Paths result;
            clipper.Execute(ClipperLib::ClipType::ctIntersection, result);

            lastState().m_clipPath = result;
            // lastState().m_clipper = NULL;
        } else if (hasValidMatrixValue()) {
            ClipperLib::Path path;

            path.emplace_back(lastState().m_clipRect.x(),
                              lastState().m_clipRect.y());
            path.emplace_back(lastState().m_clipRect.x() +
                                  lastState().m_clipRect.width(),
                              lastState().m_clipRect.y());
            path.emplace_back(
                lastState().m_clipRect.x() + lastState().m_clipRect.width(),
                lastState().m_clipRect.y() + lastState().m_clipRect.height());
            path.emplace_back(lastState().m_clipRect.x(),
                              lastState().m_clipRect.y() +
                                  lastState().m_clipRect.height());
            path.push_back(path[0]);

            ClipperLib::Clipper clipper;

            clipper.AddPath(path, ClipperLib::PolyType::ptSubject, true);

            path.clear();
            STARFISH_ASSERT(lastState().m_mapMode);
            SkPoint pt;
            pt = SkPoint::Make(rt.x(), rt.y());
            lastState().m_matrix.mapPoints(&pt, 1);
            path.emplace_back(pt.x(), pt.y());

            pt = SkPoint::Make(rt.x() + rt.width(), rt.y());
            lastState().m_matrix.mapPoints(&pt, 1);
            path.emplace_back(pt.x(), pt.y());

            pt = SkPoint::Make(rt.x() + rt.width(), rt.y() + rt.height());
            lastState().m_matrix.mapPoints(&pt, 1);
            path.emplace_back(pt.x(), pt.y());

            pt = SkPoint::Make(rt.x(), rt.y() + rt.height());
            lastState().m_matrix.mapPoints(&pt, 1);
            path.emplace_back(pt.x(), pt.y());

            path.push_back(path[0]);

            clipper.AddPath(path, ClipperLib::PolyType::ptClip, true);

            ClipperLib::Paths result;
            clipper.Execute(ClipperLib::ClipType::ctIntersection, result);

            lastState().m_clipPath = result;
            // lastState().m_clipper = NULL;
            lastState().m_hasPathClip = true;
        } else {
            SkRect sss;

            if (lastState().m_mapMode) {
                STARFISH_ASSERT(!hasValidMatrixValue());
                sss = SkRect::MakeXYWH(SkFloatToScalar((float)rt.x()),
                                       SkFloatToScalar((float)rt.y()),
                                       SkFloatToScalar((float)rt.width()),
                                       SkFloatToScalar((float)rt.height()));
                lastState().m_matrix.mapRect(&sss);
            } else {
                sss = SkRect::MakeXYWH(
                    SkFloatToScalar((float)rt.x() + lastState().m_baseX),
                    SkFloatToScalar((float)rt.y() + lastState().m_baseY),
                    SkFloatToScalar((float)rt.width()),
                    SkFloatToScalar((float)rt.height()));
            }

            if (SkRect::Intersects(lastState().m_clipRect, sss)) {
                lastState().m_clipRect.sort();
                sss.sort();

                SkRect tmp;
                tmp.fLeft = std::max(lastState().m_clipRect.fLeft, sss.fLeft);
                tmp.fRight =
                    std::min(lastState().m_clipRect.fRight, sss.fRight);
                tmp.fTop = std::max(lastState().m_clipRect.fTop, sss.fTop);
                tmp.fBottom =
                    std::min(lastState().m_clipRect.fBottom, sss.fBottom);
                lastState().m_clipRect = tmp;
            }
        }
        lastState().m_didClip = true;
    }

    void applyClippers()
    {
        if (lastState().m_didClip) {
            if (lastState().m_hasPathClip) {
                const ClipperLib::Paths& clipPaths = lastState().m_clipPath;

                if (clipPaths.size() > 0) {
                    Unit::Rect clipRt(0, 0, 0, 0);
                    for (size_t i = 0; i < clipPaths.size(); i++) {
                        Unit::Rect rt = boundingRect(clipPaths[i]);
                        if (rt.width() && rt.height()) {
                            clipRt.unite(rt);
                        }
                    }

                    if (clipRt.width() && clipRt.height()) {
                        for (size_t i = 0; i < clipPaths.size(); i++) {
                            Unit::Rect rt = boundingRect(clipPaths[i]);
                            if (rt.width() && rt.height()) {
                                cairo_save(m_canvas);
                                cairo_translate(m_canvas, -rt.x(), -rt.y());

                                const ClipperLib::Path& path = clipPaths[i];
                                cairo_move_to(m_canvas, path[0].X, path[0].Y);
                                for (size_t j = 1; j < path.size(); j++) {
                                    cairo_line_to(m_canvas, path[j].X,
                                                  path[j].Y);
                                }
                                cairo_clip(m_canvas);
                                // cairo_fill(m_canvas);

                                cairo_restore(m_canvas);
                            }
                        }
                    }
                }
            } else {
                cairo_rectangle(m_canvas, lastState().m_clipRect.x(),
                                lastState().m_clipRect.y(),
                                lastState().m_clipRect.width(),
                                lastState().m_clipRect.height());
                cairo_clip(m_canvas);
            }
        }
    }
    Unit::Rect boundingRect(const ClipperLib::Path& path)
    {
        int minX = 0, minY = 0, maxX = 0, maxY = 0;

        if (path.size()) {
            minX = path[0].X;
            minY = path[0].X;
            maxX = path[0].X;
            maxY = path[0].X;
        }

        for (size_t i = 1; i < path.size(); i++) {
            minX = std::min((int)path[i].X, minX);
            minY = std::min((int)path[i].Y, minY);
            maxX = std::max((int)path[i].X, maxX);
            maxX = std::max((int)path[i].Y, maxY);
        }

        return Unit::Rect(minX, minY, maxX - minX, maxY - minY);
    }

    virtual void setColor(const Unit::Color& clr_)
    {
        STARFISH_ASSERT(m_canvas);
        lastState().m_color = clr_;

        // Change RGB for RGBA8888 <->  ARGB conversion (mh.byun)
        cairo_set_source_rgba(m_canvas, clr_.R(), clr_.G(), clr_.B(), clr_.A());
    }

    virtual void setVisible(bool visible)
    {
        lastState().m_visible = visible;
    }

    virtual Unit::Color color()
    {
        return lastState().m_color;
    }

    virtual void setFont(Font* font)
    {
        lastState().m_font = font;
    }

    virtual void setNeedsUnderline(bool b)
    {
        lastState().m_hasUnderLine = b;
    }

    virtual void setNeedsLineThrough(bool b)
    {
        lastState().m_hasLineThrough = b;
    }

    virtual void setUnderlineColor(Unit::Color clr)
    {
        lastState().m_underLineColor = clr;
    }

    virtual void setLineThroughColor(Unit::Color clr)
    {
        lastState().m_lineThroughColor = clr;
    }

    virtual void punchHole(const Unit::Rect& rt)
    {
        if (!lastState().m_visible) {
            return;
        }
        STARFISH_ASSERT(m_canvas);
        if (!lastState().m_visible) {
            return;
        }
        float xx = rt.x(), yy = rt.y(), ww = rt.width(), hh = rt.height();
        if (lastState().m_mapMode) {
            SkRect sss = SkRect::MakeXYWH(SkFloatToScalar((float)rt.x()),
                                          SkFloatToScalar((float)rt.y()),
                                          SkFloatToScalar((float)rt.width()),
                                          SkFloatToScalar((float)rt.height()));
            lastState().m_matrix.mapRect(&sss);
            xx = sss.x();
            yy = sss.y();
            ww = sss.width();
            hh = sss.height();
        } else {
            xx = lastState().m_baseX + rt.x();
            yy = lastState().m_baseY + rt.y();
        }
        drawCairoRect(xx, yy, ww, hh, true);
    }

    bool hasValidMatrixValue()
    {
        return lastState().m_matrix.getType() &
               (SkMatrix::TypeMask::kTranslate_Mask |
                SkMatrix::TypeMask::kScale_Mask |
                SkMatrix::TypeMask::kAffine_Mask);
    }

    void drawCairoRect(float xx, float yy, float ww, float hh,
                       bool isHole = false)
    {
        cairo_save(m_canvas);
        applyClippers();
        cairo_new_path(m_canvas);

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
        if (lastState().m_mapMode) {
            SkRect sss = SkRect::MakeXYWH(SkFloatToScalar((float)rt.x()),
                                          SkFloatToScalar((float)rt.y()),
                                          SkFloatToScalar((float)rt.width()),
                                          SkFloatToScalar((float)rt.height()));
            lastState().m_matrix.mapRect(&sss);
            xx = sss.x();
            yy = sss.y();
            ww = sss.width();
            hh = sss.height();
        } else {
            xx = lastState().m_baseX + rt.x();
            yy = lastState().m_baseY + rt.y();
        }
        drawCairoRect(xx, yy, ww, hh);
    }

    virtual void drawRect(const LayoutRect& rt)
    {
        STARFISH_ASSERT(m_canvas);
        if (!lastState().m_visible) {
            return;
        }
        int xx = 0, yy = 0, ww = 0, hh = 0;
        if (lastState().m_mapMode) {
            SkRect sss = SkRect::MakeXYWH(SkFloatToScalar((float)rt.x()),
                                          SkFloatToScalar((float)rt.y()),
                                          SkFloatToScalar((float)rt.width()),
                                          SkFloatToScalar((float)rt.height()));
            lastState().m_matrix.mapRect(&sss);
            xx = sss.x();
            yy = sss.y();
            ww = sss.width();
            hh = sss.height();
        } else {
            LayoutUnit rx = rt.x();
            LayoutUnit ry = rt.y();
            rx += lastState().m_baseX;
            ry += lastState().m_baseY;
            xx = rx.floor();
            yy = ry.floor();
            ww = snapSizeToPixel(rt.width(), rx);
            hh = snapSizeToPixel(rt.height(), ry);
        }

        drawCairoRect(xx, yy, ww, hh);
    }

    virtual void drawRect(LayoutLocation p1, LayoutLocation p2,
                          LayoutLocation p3, LayoutLocation p4)
    {
        if (!lastState().m_visible) {
            return;
        }

        if (lastState().m_mapMode) {
            SkPoint pt;
            pt = SkPoint::Make((float)p1.x(), (float)p1.y());
            lastState().m_matrix.mapPoints(&pt, 1);
            p1.setX(pt.x());
            p1.setY(pt.y());

            pt = SkPoint::Make((float)p2.x(), (float)p2.y());
            lastState().m_matrix.mapPoints(&pt, 1);
            p2.setX(pt.x());
            p2.setY(pt.y());

            pt = SkPoint::Make((float)p3.x(), (float)p3.y());
            lastState().m_matrix.mapPoints(&pt, 1);
            p3.setX(pt.x());
            p3.setY(pt.y());

            pt = SkPoint::Make((float)p4.x(), (float)p4.y());
            lastState().m_matrix.mapPoints(&pt, 1);
            p4.setX(p4.x() + lastState().m_baseX);
            p4.setY(p4.y() + lastState().m_baseY);
            p4.setX(pt.x());
            p4.setY(pt.y());
        } else {
            p1.setX(p1.x() + lastState().m_baseX);
            p1.setY(p1.y() + lastState().m_baseY);

            p2.setX(p2.x() + lastState().m_baseX);
            p2.setY(p2.y() + lastState().m_baseY);

            p3.setX(p3.x() + lastState().m_baseX);
            p3.setY(p3.y() + lastState().m_baseY);

            p4.setX(p4.x() + lastState().m_baseX);
            p4.setY(p4.y() + lastState().m_baseY);
        }
        applyClippers();
        cairo_new_path(m_canvas);

        // TODO(MONG) polygon is aligned?
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

                    } else if (String::isSpaceOrNewline(ch)) {
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
        if (lastState().m_mapMode) {
            sz.setWidth(lastState().m_font->measureText(sv));
        }
        LayoutRect rt(x, y, sz.width(), sz.height());

        LayoutUnit xx = 0, yy = 0;
        if (lastState().m_mapMode) {
            SkRect sss = SkRect::MakeXYWH(SkFloatToScalar((float)rt.x()),
                                          SkFloatToScalar((float)rt.y()),
                                          SkFloatToScalar((float)rt.width()),
                                          SkFloatToScalar((float)rt.height()));
            lastState().m_matrix.mapRect(&sss);
            xx = sss.x();
            yy = sss.y();
        } else {
            xx = lastState().m_baseX + rt.x();
            yy = lastState().m_baseY + rt.y();

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
        }

        cairo_save(m_canvas);
        applyClippers();
        cairo_new_path(m_canvas);

        cairo_translate(m_canvas, xx, yy);

        UTF8NonGCString us =
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

        applyClippers();
        cairo_new_path(m_canvas);
        float xx = 0.0, yy = 0.0, ww = 0.0, hh = 0.0;
        if (lastState().m_mapMode) {
            SkRect sss = SkRect::MakeXYWH(SkFloatToScalar((float)dst.x()),
                                          SkFloatToScalar((float)dst.y()),
                                          SkFloatToScalar((float)dst.width()),
                                          SkFloatToScalar((float)dst.height()));
            lastState().m_matrix.mapRect(&sss);
            xx = sss.x();
            yy = sss.y();
            ww = sss.width();
            hh = sss.height();
        } else {
            xx = dst.x() + lastState().m_baseX;
            yy = dst.y() + lastState().m_baseY;
            ww = dst.width();
            hh = dst.height();
        }

        cairo_pattern_t* resizePattern;

        cairo_matrix_t matrix;

        resizePattern = cairo_pattern_create_for_surface(localSurface);
        cairo_translate(m_canvas, xx, yy);

        cairo_matrix_init_scale(&matrix, 1, 1);
        cairo_matrix_scale(&matrix, surfaceWidth / ww, surfaceHeight / hh);
        cairo_pattern_set_matrix(resizePattern, &matrix);

        cairo_set_source(m_canvas, resizePattern);
        cairo_pattern_set_filter(cairo_get_source(m_canvas),
                                 CAIRO_FILTER_NEAREST);
        cairo_rectangle(m_canvas, 0, 0, ww, hh);
        if (isFromSurface) {
            cairo_clip(m_canvas);
            cairo_paint_with_alpha(m_canvas, lastState().m_opacity);
        } else {
            cairo_fill(m_canvas);
        }

        // drawDebugLine(xx,yy,ww,hh);
        cairo_restore(m_canvas);
    }

    void drawDebugLine(double xx, double yy, double ww, double hh)
    {
        cairo_save(m_canvas);

        cairo_set_source_rgba(m_canvas, 1, 0, 0, 1);
        cairo_rectangle(m_canvas, 0, 0, ww, hh);
        cairo_stroke(m_canvas);

        cairo_restore(m_canvas);
    }

    virtual void drawImage(ImageData* data, const Unit::Rect& dst)
    {
        if (!lastState().m_visible) {
            return;
        }

        void* imgData = data->unwrap();
        double surfaceWidth, surfaceHeight;
        cairo_surface_t* image;

        if (imgData) {
            int stride =
                cairo_format_stride_for_width(CAIRO_FORMAT, data->width());
            image = cairo_image_surface_create_for_data(
                (unsigned char*)imgData, CAIRO_FORMAT, dst.width(),
                dst.height(), stride);
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
            (unsigned char*)data->unwrap(), CAIRO_FORMAT, dst.width(),
            dst.width(),
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
        // TODO : It's not implemented yet!
        drawImage(data, dst);
        return;
    }

    virtual void postMatrix(const SkMatrix& matrix)
    {
        assureMapMode();
        lastState().m_matrix.preConcat(matrix);
    }

    virtual void applyMatrixTo(LayoutLocation& lp)
    {
        if (lastState().m_mapMode) {
            STARFISH_ASSERT(!hasValidMatrixValue());
            SkPoint point = SkPoint::Make((float)lp.x(), (float)lp.y());
            lastState().m_matrix.mapPoints(&point, 1);
            lp.setX(point.x());
            lp.setY(point.y());
        } else {
            lp.setX(lp.x() + lastState().m_baseX);
            lp.setY(lp.y() + lastState().m_baseY);
        }
    }

    virtual void applyMatrixTo(LayoutRect& lp)
    {
        if (lastState().m_mapMode) {
            STARFISH_ASSERT(!hasValidMatrixValue());
            SkRect sss = SkRect::MakeXYWH(SkFloatToScalar((float)lp.x()),
                                          SkFloatToScalar((float)lp.y()),
                                          SkFloatToScalar((float)lp.width()),
                                          SkFloatToScalar((float)lp.height()));
            lastState().m_matrix.mapRect(&sss);
            lp.setX(sss.x());
            lp.setY(sss.y());
            lp.setWidth(sss.width());
            lp.setHeight(sss.height());
        } else {
            lp.setX(lp.x() + lastState().m_baseX);
            lp.setY(lp.y() + lastState().m_baseY);
        }
    }

    virtual void* unwrap()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
        return NULL;
    }

    CanvasState& lastState()
    {
        STARFISH_ASSERT(m_state.size());
        return m_state[m_state.size() - 1];
    }

protected:
    std::vector<CanvasState> m_state;
    cairo_surface_t* m_surface;
    cairo_t* m_canvas;
    bool m_directDraw;
    void* m_buffer;
    unsigned m_width;
    unsigned m_height;
    size_t m_imageCount;
    GCUnorderedMap<ImageData*, std::vector<std::pair<Evas_Object*, bool>>,
                   std::hash<ImageData*>,
                   std::equal_to<ImageData*>>* m_prevDrawnImageMap;
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
