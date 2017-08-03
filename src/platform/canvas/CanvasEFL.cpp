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

#if defined(PORT_GRAPHIC_BACKEND_EFL)
#include "StarFish.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/font/Font.h"
#include "core/modules/canvas/image/ImageData.h"
#include "core/style/UnitHelper.h"

#include <Evas.h>
#include <Evas_Engine_Buffer.h>
#include <Elementary.h>
#if defined(STARFISH_TIZEN_3_0) || defined(STARFISH_TIZEN_OBS)
#include <Ecore.h>
#else
#include <Ecore_X.h>
#endif

#include <vector>
#include <SkMatrix.h>
#include <clipper.hpp>

#include <cairo.h>

Evas* g_internalCanvas;

namespace StarFish {

Evas* internalCanvas()
{
    STARFISH_RELEASE_ASSERT(g_internalCanvas);
    return g_internalCanvas;
}

class CanvasStateEFL : public CanvasState {
public:
    SkMatrix m_matrix;
    Evas_Object* m_clipper;
    SkRect m_clipRect;
    ClipperLib::Paths m_clipPath;

    bool m_mapMode;
    bool m_didClip;
    bool m_hasPathClip;

    CanvasStateEFL()
        : CanvasState()
    {
        m_clipper = NULL;
        m_mapMode = false;
        m_didClip = false;
        m_hasPathClip = false;
    }
};

Unit::Rect boundingRect(const ClipperLib::Path& path)
{
    int minX = 0, minY = 0, maxX = 0, maxY = 0;

    if (path.size()) {
        minX = path[0].X;
        minY = path[0].Y;
        maxX = path[0].X;
        maxY = path[0].Y;
    }

    for (size_t i = 1; i < path.size(); i++) {
        minX = std::min((int)path[i].X, minX);
        minY = std::min((int)path[i].Y, minY);
        maxX = std::max((int)path[i].X, maxX);
        maxX = std::max((int)path[i].Y, maxY);
    }

    return Unit::Rect(minX, minY, std::abs(maxX - minX), std::abs(maxY - minY));
}

class CanvasEFL : public Canvas {
    void initFromBuffer(void* buffer, int width, int height, int stride)
    {
        Evas* canvas;
        int method;
        method = evas_render_method_lookup("buffer");
        if (method <= 0) {
            fputs("ERROR: evas was not compiled with 'buffer' engine!\n",
                  stderr);
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        canvas = evas_new();
        if (!canvas) {
            fputs("ERROR: could not instantiate new evas canvas.\n", stderr);
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        evas_output_method_set(canvas, method);

        if (stride == -1) {
            stride = width * 4;
        }

        Evas_Engine_Info_Buffer* einfo;
        void* pixels = buffer;

        evas_output_size_set(canvas, width, height);
        evas_output_viewport_set(canvas, 0, 0, width, height);
        einfo = (Evas_Engine_Info_Buffer*)evas_engine_info_get(canvas);

        if (!einfo) {
            fputs("ERROR: could not get evas engine info!\n", stderr);
            evas_free(canvas);
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }

        einfo->info.depth_type = EVAS_ENGINE_BUFFER_DEPTH_ARGB32;
        einfo->info.dest_buffer = pixels;
        einfo->info.dest_buffer_row_bytes = stride;
        einfo->info.use_color_key = 0;
        einfo->info.alpha_threshold = 0;
        einfo->info.func.new_update_region = NULL;
        einfo->info.func.free_update_region = NULL;
        evas_engine_info_set(canvas, (Evas_Engine_Info*)einfo);

        m_width = width;
        m_height = height;
        m_canvas = canvas;
        m_buffer = buffer;

        initState();
    }

    void initState()
    {
        m_stateSize = 1;
        m_state.push_back(CanvasStateEFL());

        lastState().m_matrix.reset();
        lastState().m_clipRect.setLTRB(0, 0, SkFloatToScalar((float)m_width),
                                       SkFloatToScalar((float)m_height));
        lastState().m_clipper = NULL;
    }

public:
    CanvasEFL(void* data)
    {
        m_imageCount = 0;
        m_image = NULL;
        m_buffer = NULL;
        m_directDraw = true;
        struct dummy {
            void* a;
            void* b;
            int w;
            int h;
            std::vector<Evas_Object*>* objList;
            std::vector<Evas_Object*>* surfaceList;
        };
        dummy* d = (dummy*)data;
        m_canvas = (Evas*)d->a;
        m_width = d->w;
        m_height = d->h;
        m_objList = d->objList;
        m_surfaceList = d->surfaceList;

        initState();
    }

    CanvasEFL(CanvasSurface* data)
    {
        m_imageCount = 0;
        m_objList = NULL;
        m_surfaceList = NULL;
        m_directDraw = false;
        m_image = (Evas_Object*)data->unwrap();
        void* buffer = evas_object_image_data_get(m_image, EINA_TRUE);
        m_buffer = buffer;
        initFromBuffer(buffer, data->width(), data->height(),
                       evas_object_image_stride_get(m_image));
    }

    virtual ~CanvasEFL()
    {
        restore();
        STARFISH_ASSERT(m_stateSize == 0);
        m_statePerFrame.clear();
        if (m_image && m_buffer) {
            evas_object_image_data_set(m_image, m_buffer);
            // evas_object_image_data_update_add(m_image, 0, 0, m_width,
            // m_height);
        }

        if (m_directDraw) {
            // evas_damage_rectangle_add(m_canvas, 0, 0, m_width, m_height);
        } else {
            evas_render(m_canvas);
            evas_free(m_canvas);
        }

        m_state.clear();
        m_state.shrink_to_fit();
    }

    virtual void clearColor(const Unit::Color& clr)
    {
        Evas_Object* eo = evas_object_rectangle_add(m_canvas);
        if (m_objList) {
            m_objList->push_back(eo);
        }
        save();
        setColor(clr);
        Unit::Color c = computedAlphaColor();
        evas_object_color_set(eo, c.r(), c.g(), c.b(), c.a());
        evas_object_move(eo, 0, 0);
        evas_object_resize(eo, m_width, m_height);
        applyClippers(eo);
        evas_object_show(eo);
        restore();
    }

    // state
    virtual void save()
    {
        size_t size = m_stateSize++;
        if (m_state.size() < m_stateSize) {
            auto& last = m_state[size - 1];
            m_state.push_back(last);
            return;
        }
        auto& state = m_state[size];
        auto& last = m_state[size - 1];
        if (UNLIKELY(last.m_clipPath.size())) {
            state.m_matrix = last.m_matrix;
            state.m_clipRect = last.m_clipRect;
            state.m_clipPath = last.m_clipPath;
            state.m_clipper = last.m_clipper;
            state.m_color = last.m_color;
            state.m_opacity = last.m_opacity;
            state.m_baseX = last.m_baseX;
            state.m_baseY = last.m_baseY;
            state.m_font = last.m_font;
            state.m_mapMode = last.m_mapMode;
            state.m_visible = last.m_visible;
            state.m_didClip = last.m_didClip;
            state.m_hasPathClip = last.m_hasPathClip;
            state.m_textDecorationData = last.m_textDecorationData;
        } else {
            memcpy(&state, &last, sizeof(CanvasStateEFL));
        }
    }

    // pop state stack and restore state
    virtual void restore()
    {
        m_stateSize--;
        m_state.erase(m_state.end() - 1);
    }

    virtual void saveByFrame(Frame* f)
    {
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
        auto& lastState = m_state.back();

        CanvasStateEFL* eflState = (CanvasStateEFL*)state;
        lastState.m_clipRect = eflState->m_clipRect;
        lastState.m_clipPath = eflState->m_clipPath;
        lastState.m_clipper = eflState->m_clipper;
        lastState.m_didClip = eflState->m_didClip;
        lastState.m_hasPathClip = eflState->m_hasPathClip;
        if (flag == ReplaceFlag::All) {
            lastState.m_color = eflState->m_color;
            lastState.m_matrix = eflState->m_matrix;
            lastState.m_opacity = eflState->m_opacity;
            lastState.m_baseX = eflState->m_baseX;
            lastState.m_baseY = eflState->m_baseY;
            lastState.m_font = eflState->m_font;
            lastState.m_mapMode = eflState->m_mapMode;
            lastState.m_visible = eflState->m_visible;
            lastState.m_textDecorationData = eflState->m_textDecorationData;
        }
    }

    void assureMapMode()
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

    bool canSkipPainting(const Unit::Rect& rt)
    {
        if (!lastState().m_visible) {
            return true;
        }

        if (lastState().m_didClip && lastState().m_hasPathClip) {
            // TODO
            return false;
        }

        // compute abs rect
        float xx, yy, ww, hh;
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
            ww = rt.width();
            hh = rt.height();
        }

        SkRect absRect =
            SkRect::MakeXYWH(SkFloatToScalar(xx), SkFloatToScalar(yy),
                             SkFloatToScalar(ww), SkFloatToScalar(hh));

        // check cliprect if exists
        if (lastState().m_didClip) {
            if (lastState().m_clipRect.isEmpty())
                return true;
            if (SkRect::Intersects(absRect, lastState().m_clipRect)) {
                return false;
            } else {
                return true;
            }
        }

        // check canvas bound
        SkRect screenRect = SkRect::MakeXYWH(
            SkFloatToScalar(0), SkFloatToScalar(0), SkFloatToScalar(m_width),
            SkFloatToScalar(m_height));

        if (SkRect::Intersects(absRect, screenRect)) {
            return false;
        }

        return true;
    }

    virtual void clip(const Unit::Rect& rt)
    {
        lastState().m_didClip = true;
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
            lastState().m_clipper = NULL;
            return;
        } else if (!isMatrixRemainsRectangle()) {
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
            lastState().m_clipper = NULL;
            lastState().m_hasPathClip = true;
            return;
        }
        SkRect sss;

        // assureMapMode();
        if (lastState().m_mapMode) {
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

        if (!SkRect::Intersects(lastState().m_clipRect, sss)) {
            lastState().m_clipRect.setEmpty();
            lastState().m_clipper = NULL;
        } else {
            lastState().m_clipRect.sort();
            sss.sort();

            SkRect tmp;
            tmp.fLeft = std::max(lastState().m_clipRect.fLeft, sss.fLeft);
            tmp.fRight = std::min(lastState().m_clipRect.fRight, sss.fRight);
            tmp.fTop = std::max(lastState().m_clipRect.fTop, sss.fTop);
            tmp.fBottom = std::min(lastState().m_clipRect.fBottom, sss.fBottom);
            lastState().m_clipRect = tmp;
            lastState().m_clipper = NULL;
        }
    }

    Evas_Object* createPathClipper(float o = 1.0)
    {
        STARFISH_ASSERT(lastState().m_didClip);
        STARFISH_ASSERT(lastState().m_hasPathClip);

        const ClipperLib::Paths& clipPaths = lastState().m_clipPath;
        // FIXME evas could not support polygon clipping
        /*
        STARFISH_ASSERT(clipPaths.size() < 2);

        const ClipperLib::Path& path = clipPaths[0];
        Evas_Object* cl = evas_object_polygon_add(m_canvas);
        evas_object_color_set(cl, 255, 255, 255, 255);
        evas_object_show(cl);

        for (size_t i = 0; i < path.size(); i ++) {
            evas_object_polygon_point_add(cl, path[i].X, path[i].Y);
        }*/

        Unit::Rect clipRt(0, 0, 0, 0);
        for (size_t i = 0; i < clipPaths.size(); i++) {
            Unit::Rect rt = boundingRect(clipPaths[i]);
            if (rt.width() && rt.height()) {
                clipRt.unite(rt);
            }
        }

        if (clipRt.width() && clipRt.height()) {
            int w = clipRt.width();
            int h = clipRt.height();
            Evas_Object* cl = evas_object_image_add(m_canvas);

            evas_object_image_size_set(cl, w, h);
            evas_object_image_filled_set(cl, EINA_TRUE);
            evas_object_image_colorspace_set(
                cl, Evas_Colorspace::EVAS_COLORSPACE_ARGB8888);
            evas_object_image_alpha_set(cl, EINA_TRUE);
            evas_object_anti_alias_set(cl, EINA_TRUE);
            evas_object_move(cl, clipRt.x(), clipRt.y());
            evas_object_resize(cl, clipRt.width(), clipRt.height());

            cairo_surface_t* surface;
            cairo_t* cr;

            void* data = evas_object_image_data_get(cl, EINA_TRUE);
            surface = cairo_image_surface_create_for_data(
                (unsigned char*)data, CAIRO_FORMAT_ARGB32, w, h,
                evas_object_image_stride_get(cl));
            cr = cairo_create(surface);

            for (size_t i = 0; i < clipPaths.size(); i++) {
                Unit::Rect rt = boundingRect(clipPaths[i]);
                if (rt.width() && rt.height()) {
                    cairo_save(cr);
                    cairo_translate(cr, -rt.x(), -rt.y());
                    cairo_set_source_rgba(cr, o, o, o, o);

                    const ClipperLib::Path& path = clipPaths[i];
                    cairo_move_to(cr, path[0].X, path[0].Y);
                    for (size_t j = 1; j < path.size(); j++) {
                        cairo_line_to(cr, path[j].X, path[j].Y);
                    }

                    cairo_fill(cr);

                    cairo_restore(cr);
                }
            }

            cairo_destroy(cr);
            cairo_surface_flush(surface);
            cairo_surface_destroy(surface);

            evas_object_image_data_set(cl, data);
            evas_object_show(cl);
            return cl;
        }

        return nullptr;
    }

    void applyClippers(Evas_Object* eo, bool isImage = false)
    {
        if (lastState().m_didClip) {
            if (lastState().m_hasPathClip) {
                if (!lastState().m_clipper) {
                    const ClipperLib::Paths& clipPaths = lastState().m_clipPath;

                    if (clipPaths.size() > 0) {
                        Evas_Object* cl =
                            createPathClipper(lastState().m_opacity);
                        if (cl) {
                            lastState().m_clipper = cl;
                            if (m_objList) {
                                m_objList->push_back(lastState().m_clipper);
                            }
                            evas_object_clip_set(eo, lastState().m_clipper);
                        }
                    }
                } else {
                    evas_object_clip_set(eo, lastState().m_clipper);
                }
            } else {
                if (isImage) {
                    Evas_Object* ceo = evas_object_rectangle_add(m_canvas);
                    int c = 255 * lastState().m_opacity;
                    evas_object_color_set(ceo, c, c, c, c);
                    evas_object_move(ceo, lastState().m_clipRect.x(),
                                     lastState().m_clipRect.y());
                    evas_object_resize(ceo, lastState().m_clipRect.width(),
                                       lastState().m_clipRect.height());
                    evas_object_show(ceo);
                    if (m_objList) {
                        m_objList->push_back(ceo);
                    }
                    evas_object_clip_set(eo, ceo);
                } else {
                    if (!lastState().m_clipper) {
                        Evas_Object* eo = evas_object_rectangle_add(m_canvas);
                        evas_object_color_set(eo, 255, 255, 255, 255);
                        evas_object_move(eo, lastState().m_clipRect.x(),
                                         lastState().m_clipRect.y());
                        evas_object_resize(eo, lastState().m_clipRect.width(),
                                           lastState().m_clipRect.height());
                        evas_object_show(eo);
                        lastState().m_clipper = eo;
                        if (m_objList) {
                            m_objList->push_back(lastState().m_clipper);
                        }
                    }
                    evas_object_clip_set(eo, lastState().m_clipper);
                }
            }
        } else {
            if (isImage && lastState().m_opacity != 1) {
                Evas_Object* ceo = evas_object_rectangle_add(m_canvas);
                int c = 255 * lastState().m_opacity;
                evas_object_color_set(ceo, c, c, c, c);
                evas_object_move(ceo, 0, 0);
                evas_object_resize(ceo, m_width, m_height);
                evas_object_show(ceo);
                if (m_objList) {
                    m_objList->push_back(ceo);
                }
                evas_object_clip_set(eo, ceo);
            }
        }
    }

    virtual void setColor(const Unit::Color& clr_)
    {
        lastState().m_color = clr_;
    }

    Unit::Color computedAlphaColor()
    {
        int r = lastState().m_color.r();
        int g = lastState().m_color.g();
        int b = lastState().m_color.b();
        int a = lastState().m_color.a() * lastState().m_opacity;
        evas_color_argb_premul(a, &r, &g, &b);
        Unit::Color clr;
        clr.m_a = a;
        clr.m_r = r;
        clr.m_g = g;
        clr.m_b = b;
        return clr;
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

    void drawEvasRect(int xx, int yy, int ww, int hh, const Unit::Rect& rt,
                      bool isHole = false)
    {
        Evas_Object* eo = evas_object_rectangle_add(m_canvas);
        if (m_objList) {
            m_objList->push_back(eo);
        }
        Unit::Color c = computedAlphaColor();
        evas_object_color_set(eo, c.r(), c.g(), c.b(), c.a());
        evas_object_move(eo, xx, yy);
        evas_object_resize(eo, ww, hh);
        if (!isHole) {
            applyClippers(eo);
        }
        applyEvasMapIfNeeded(eo, rt);
        evas_object_show(eo);

        if (isHole) {
            evas_object_color_set(eo, 0, 0, 0, 0);
            evas_object_render_op_set(eo, EVAS_RENDER_COPY);
        }
    }

    virtual void drawRect(const Unit::Rect& rt)
    {
        if (canSkipPainting(rt)) {
            return;
        }

        float xx = 0.0, yy = 0.0, ww = 0.0, hh = 0.0;
        if (lastState().m_mapMode) {
            SkRect sss = SkRect::MakeXYWH(SkFloatToScalar((float)rt.x()),
                                          SkFloatToScalar((float)rt.y()),
                                          SkFloatToScalar((float)rt.width()),
                                          SkFloatToScalar((float)rt.height()));
            if (!shouldApplyEvasMap()) {
                lastState().m_matrix.mapRect(&sss);
            }
            xx = sss.x();
            yy = sss.y();
            ww = sss.width();
            hh = sss.height();
        } else {
            if (!shouldApplyEvasMap()) {
                xx = lastState().m_baseX + rt.x();
                yy = lastState().m_baseY + rt.y();
            } else {
                xx = rt.x();
                yy = rt.y();
            }

            ww = rt.width();
            hh = rt.height();
        }
        drawEvasRect(xx, yy, ww, hh, rt);
    }

    // NOTE punchHole && Evas can not apply clip
    virtual void punchHole(const Unit::Rect& rt)
    {
        if (canSkipPainting(rt)) {
            return;
        }

        float xx = 0.0, yy = 0.0, ww = 0.0, hh = 0.0;
        if (lastState().m_mapMode) {
            SkRect sss = SkRect::MakeXYWH(SkFloatToScalar((float)rt.x()),
                                          SkFloatToScalar((float)rt.y()),
                                          SkFloatToScalar((float)rt.width()),
                                          SkFloatToScalar((float)rt.height()));
            if (!shouldApplyEvasMap()) {
                lastState().m_matrix.mapRect(&sss);
            }
            xx = sss.x();
            yy = sss.y();
            ww = sss.width();
            hh = sss.height();
        } else {
            if (!shouldApplyEvasMap()) {
                xx = lastState().m_baseX + rt.x();
                yy = lastState().m_baseY + rt.y();
            } else {
                xx = rt.x();
                yy = rt.y();
            }

            ww = rt.width();
            hh = rt.height();
        }
        drawEvasRect(xx, yy, ww, hh, rt, true);
    }

    virtual void drawRect(const LayoutRect& rt)
    {
        if (canSkipPainting(
                Unit::Rect(rt.x(), rt.y(), rt.width(), rt.height()))) {
            return;
        }

        int xx = 0, yy = 0, ww = 0, hh = 0;
        if (lastState().m_mapMode) {
            SkRect sss = SkRect::MakeXYWH(SkFloatToScalar((float)rt.x()),
                                          SkFloatToScalar((float)rt.y()),
                                          SkFloatToScalar((float)rt.width()),
                                          SkFloatToScalar((float)rt.height()));
            if (!shouldApplyEvasMap()) {
                lastState().m_matrix.mapRect(&sss);
            }
            xx = sss.x();
            yy = sss.y();
            ww = sss.width();
            hh = sss.height();
        } else {
            LayoutUnit rx = rt.x();
            LayoutUnit ry = rt.y();
            if (!shouldApplyEvasMap()) {
                rx += lastState().m_baseX;
                ry += lastState().m_baseY;
            }
            xx = rx.floor();
            yy = ry.floor();
            ww = snapSizeToPixel(rt.width(), rx);
            hh = snapSizeToPixel(rt.height(), ry);
        }
        drawEvasRect(xx, yy, ww, hh, Unit::Rect(xx, yy, ww, hh));
    }

    virtual void drawRect(LayoutLocation p1, LayoutLocation p2,
                          LayoutLocation p3, LayoutLocation p4)
    {
        if (canSkipPainting(
                Unit::Rect(std::min(p1.x(), p4.x()), std::min(p1.y(), p2.y()),
                           std::max(p3.x() - p4.x(), p2.x() - p1.x()),
                           std::max(p3.y() - p2.y(), p4.y() - p1.y())))) {
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

        Evas_Object* eo = evas_object_polygon_add(m_canvas);
        if (m_objList) {
            m_objList->push_back(eo);
        }

        auto clr = computedAlphaColor();
        evas_object_color_set(eo, clr.r(), clr.g(), clr.b(), clr.a());

        evas_object_polygon_point_add(eo, p1.x().floor(), p1.y().floor());
        evas_object_polygon_point_add(eo, p2.x().floor(), p2.y().floor());
        evas_object_polygon_point_add(eo, p3.x().floor(), p3.y().floor());
        evas_object_polygon_point_add(eo, p4.x().floor(), p4.y().floor());
        evas_object_show(eo);
    }

    virtual void drawText(LayoutUnit x, LayoutUnit y, LayoutUnit stringWidth,
                          const StringView& sv)
    {
        if (canSkipPainting(
                Unit::Rect(x, y, stringWidth,
                           lastState().m_font->metrics().m_fontHeight))) {
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

        // FIXME: evas textblock doesn't render 1 length space char
        bool isSpace = false;
        if (!lastState().m_textDecorationData.hasUnderLine() &&
            !lastState().m_textDecorationData.hasLineThrough()) {
            if (sv.length() == 1 &&
                sv.originalString()->charAt(sv.start()) == ' ') {
                return;
            }

            Evas_Object* eo = evas_object_text_add(m_canvas);
            if (m_objList) {
                m_objList->push_back(eo);
            }
            LayoutSize sz(stringWidth,
                          lastState().m_font->metrics().m_fontHeight);
            if (lastState().m_mapMode) {
                sz.setWidth(lastState().m_font->measureText(sv));
            }
            LayoutRect rt(x, y, sz.width(), sz.height());

            LayoutUnit xx = 0, yy = 0;
            if (lastState().m_mapMode) {
                SkRect sss =
                    SkRect::MakeXYWH(SkFloatToScalar((float)rt.x()),
                                     SkFloatToScalar((float)rt.y()),
                                     SkFloatToScalar((float)rt.width()),
                                     SkFloatToScalar((float)rt.height()));
                if (!shouldApplyEvasMap()) {
                    lastState().m_matrix.mapRect(&sss);
                }
                xx = sss.x();
                yy = sss.y();
            } else {
                if (!shouldApplyEvasMap()) {
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
                } else {
                    xx = x;
                    yy = y;
                }
            }

            int siz;
            evas_object_text_font_get(
                (Evas_Object*)lastState().m_font->unwrap(), NULL, &siz);
            float ptSize = siz;
            evas_object_text_font_set(
                eo, lastState().m_font->familyName()->utf8Data(), ptSize);
            Unit::Color c = computedAlphaColor();
            evas_object_color_set(eo, c.r(), c.g(), c.b(), c.a());
            UTF8StringDataNonGCStd us = sv.originalString()->toUTF8NonGCString(
                sv.start(), sv.end(), true);
            evas_object_text_text_set(eo, us.c_str());

            evas_object_move(eo, (int)xx, (int)yy);
            applyClippers(eo);
            applyEvasMapIfNeeded(eo, rt);

            evas_object_show(eo);
        } else {
            StringView stringToDraw = sv;
            if (sv.length() == 1 &&
                (sv.originalString()->charAt(sv.start()) == ' ' ||
                 sv.originalString()->charAt(sv.start()) == 0xA0)) {
                // FIXME evas textblock doesn't render 1 length space char
                stringToDraw =
                    StringView(String::createASCIIString("  "), 0, 2);
                isSpace = true;
                stringWidth *= 2;
            }

            Evas_Object* eo = evas_object_textblock_add(m_canvas);
            if (m_objList) {
                m_objList->push_back(eo);
            }
            LayoutSize sz(stringWidth,
                          lastState().m_font->metrics().m_fontHeight);
            if (lastState().m_mapMode) {
                sz.setWidth(lastState().m_font->measureText(sv));
            }
            // FIXME: evas textblock doesn't render 1 length space char
            if (isSpace) {
                sz.setWidth(sz.width() / 2);
            }
            LayoutRect rt(x, y, sz.width(), sz.height());

            float xx = 0.0, yy = 0.0, ww = 0.0, hh = 0.0;
            if (lastState().m_mapMode) {
                SkRect sss =
                    SkRect::MakeXYWH(SkFloatToScalar((float)rt.x()),
                                     SkFloatToScalar((float)rt.y()),
                                     SkFloatToScalar((float)rt.width()),
                                     SkFloatToScalar((float)rt.height()));
                if (!shouldApplyEvasMap()) {
                    lastState().m_matrix.mapRect(&sss);
                }
                xx = sss.x();
                yy = sss.y();
                ww = sss.width();
                hh = sss.height();
            } else {
                if (!shouldApplyEvasMap()) {
                    xx = lastState().m_baseX + rt.x();
                    yy = lastState().m_baseY + rt.y();
                } else {
                    xx = x;
                    yy = y;
                }
                ww = rt.width();
                hh = rt.height();
            }

            Evas_Textblock_Style* st = evas_textblock_style_new();
            char buf[512];
            // float ptSize = convertFromPxToPt(lastState().m_font->size());
            int siz;
            evas_object_text_font_get(
                (Evas_Object*)lastState().m_font->unwrap(), NULL, &siz);
            float ptSize = siz;
            const char* weight;
            switch (lastState().m_font->weight()) {
            case 1:
                weight = "thin";
                break;
            case 2:
                weight = "ultralight";
                break;
            case 3:
                weight = "light";
                break;
            case 4:
                weight = "medium";
                break;
            case 5:
                weight = "semibold";
                break;
            case 6:
                weight = "bold";
                break;
            case 7:
                weight = "ultrabold";
                break;
            case 8:
                weight = "black";
                break;
            case 9:
                weight = "extrablack";
                break;
            default:
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }

            const char* fontStyle = "normal";

            Font* fnt = lastState().m_font;
            if (fnt->style() == FontStyleItalic) {
                fontStyle = "italic";
            } else if (fnt->style() == FontStyleOblique) {
                fontStyle = "oblique";
            }

            const char* underlineMode =
                lastState().m_textDecorationData.hasUnderLine() ? "on" : "off";
            const char* lineThroughMode =
                lastState().m_textDecorationData.hasLineThrough() ? "on"
                                                                  : "off";

            char underlineColor[128];
            char lineThroughColor[128];
            snprintf(
                underlineColor, sizeof(underlineColor), "#%02x%02x%02x%02x",
                (int)lastState().m_textDecorationData.underLineColor().r(),
                (int)lastState().m_textDecorationData.underLineColor().g(),
                (int)lastState().m_textDecorationData.underLineColor().b(),
                (int)lastState().m_textDecorationData.underLineColor().a());
            snprintf(
                lineThroughColor, sizeof(lineThroughColor), "#%02x%02x%02x%02x",
                (int)lastState().m_textDecorationData.lineThroughColor().r(),
                (int)lastState().m_textDecorationData.lineThroughColor().g(),
                (int)lastState().m_textDecorationData.lineThroughColor().b(),
                (int)lastState().m_textDecorationData.lineThroughColor().a());

            snprintf(buf, sizeof(buf),
                     "DEFAULT='font=%s font_size=%f color=#%02x%02x%02x%02x "
                     "valign=middle font_weight=%s font_style=%s "
                     "strikethrough=%s strikethrough_color=%s underline=%s "
                     "underline_color=%s '",
                     lastState().m_font->familyName()->utf8Data(), ptSize,
                     (int)lastState().m_color.r(), (int)lastState().m_color.g(),
                     (int)lastState().m_color.b(), (int)lastState().m_color.a(),
                     weight, fontStyle, lineThroughMode, lineThroughColor,
                     underlineMode, underlineColor);
            evas_textblock_style_set(st, buf);
            evas_object_textblock_style_set(eo, st);
            /*
            evas_object_color_set(eo, lastState().m_color.r(),
                                  lastState().m_color.g(),
                                  lastState().m_color.b(),
                                  lastState().m_color.a()); */
            UTF8StringDataNonGCStd us =
                stringToDraw.originalString()->toUTF8NonGCString(
                    stringToDraw.start(), stringToDraw.end());
            evas_object_textblock_text_markup_set(eo, us.c_str());

            evas_object_resize(eo, ww, hh);
            evas_object_move(eo, xx, yy);

            applyClippers(eo);
            applyEvasMapIfNeeded(eo, rt);

            evas_object_show(eo);
            evas_textblock_style_free(st);
        }
    }

    void drawImageInner(ImageData* data, const Unit::Rect& dst, size_t l,
                        size_t t, size_t r, size_t b, double scale, bool fill)
    {
        if (canSkipPainting(dst)) {
            return;
        }

        float xx = 0.0, yy = 0.0, ww = 0.0, hh = 0.0;
        if (lastState().m_mapMode) {
            SkRect sss = SkRect::MakeXYWH(SkFloatToScalar((float)dst.x()),
                                          SkFloatToScalar((float)dst.y()),
                                          SkFloatToScalar((float)dst.width()),
                                          SkFloatToScalar((float)dst.height()));
            if (!shouldApplyEvasMap()) {
                lastState().m_matrix.mapRect(&sss);
            }
            xx = sss.x();
            yy = sss.y();
            ww = sss.width();
            hh = sss.height();
        } else {
            xx = dst.x();
            yy = dst.y();
            if (!shouldApplyEvasMap()) {
                xx = lastState().m_baseX + dst.x();
                yy = lastState().m_baseY + dst.y();
            }
            ww = dst.width();
            hh = dst.height();
        }
        Evas_Object* eo = nullptr;
        eo = evas_object_image_add(m_canvas);
        if (m_objList) {
            m_objList->push_back(eo);
        }

        Evas_Object* imgData = (Evas_Object*)data->unwrap();
        // NOTE
        // we don't need to check `!shouldApplyEvasMap()` here
        // but, `evas_object_image_source_set(eo, imgData) + evas_map` gives
        // segfault or incorrect result.....
        if (evas_object_evas_get(imgData) == evas_object_evas_get(eo) &&
            !shouldApplyEvasMap()) {
            evas_object_image_source_set(eo, imgData);
        } else {
            if (((char*)evas_object_data_get(imgData, "local"))[0] == '0') {
                void* imgBuf = evas_object_image_data_get(imgData, EINA_FALSE);
                evas_object_image_size_set(eo, data->width(), data->height());
                evas_object_image_colorspace_set(
                    eo, evas_object_image_colorspace_get(imgData));
                evas_object_image_data_set(eo, imgBuf);
            } else {
                const char* path;
                evas_object_image_file_get(imgData, &path, NULL);
                evas_object_image_file_set(eo, path, NULL);
            }
            evas_object_image_size_set(eo, data->width(), data->height());
            evas_object_image_colorspace_set(
                eo, evas_object_image_colorspace_get(imgData));
        }

        evas_object_image_filled_set(eo, EINA_TRUE);
        evas_object_image_alpha_set(eo, EINA_TRUE);
        // evas_object_anti_alias_set(eo, EINA_TRUE);
        evas_object_image_border_set(eo, l, r, t, b);
        evas_object_image_border_scale_set(eo, scale);

        Evas_Border_Fill_Mode isFill =
            Evas_Border_Fill_Mode::EVAS_BORDER_FILL_NONE;
        if (fill) {
            isFill = Evas_Border_Fill_Mode::EVAS_BORDER_FILL_DEFAULT;
        }

        evas_object_image_border_center_fill_set(eo, isFill);
        evas_object_move(eo, xx, yy);
        evas_object_resize(eo, ww, hh);

        applyClippers(eo, true);
        applyEvasMapIfNeeded(eo, dst, true);
        evas_object_show(eo);

        m_imageCount++;
        if (m_imageCount == 101) {
            STARFISH_LOG_ERROR(
                "paint more than 100 image makes poor performance\n");
        }
    }

    virtual void drawImage(ImageData* data, const Unit::Rect& dst)
    {
        drawImageInner(data, dst, 0, 0, 0, 0, 1.0, true);
    }

    virtual void drawBorderImage(ImageData* data, const Unit::Rect& dst,
                                 size_t l, size_t t, size_t r, size_t b,
                                 double scale, bool fill)
    {
        drawImageInner(data, dst, l, t, r, b, scale, fill);
    }

    virtual void drawRepeatImage(ImageData* data, const Unit::Rect& dst,
                                 float imageWidth, float imageHeight,
                                 bool xRepeat, bool yRepeat, bool isRootElement)
    {
        if (canSkipPainting(Unit::Rect(0, 0, dst.width(), dst.height()))) {
            return;
        }

        float xx = 0.0, yy = 0.0, ww = 0.0, hh = 0.0;
        if (lastState().m_mapMode) {
            SkRect sss = SkRect::MakeXYWH(SkFloatToScalar((float)dst.x()),
                                          SkFloatToScalar((float)dst.y()),
                                          SkFloatToScalar((float)dst.width()),
                                          SkFloatToScalar((float)dst.height()));
            if (!shouldApplyEvasMap()) {
                lastState().m_matrix.mapRect(&sss);
            }
            xx = sss.x();
            yy = sss.y();
            ww = sss.width();
            hh = sss.height();
        } else {
            xx = dst.x();
            yy = dst.y();
            if (!shouldApplyEvasMap()) {
                xx = lastState().m_baseX; // + dst.x();
                yy = lastState().m_baseY; // + dst.y();
            }
            ww = dst.width();
            hh = dst.height();
        }

        Evas_Object* imgData = (Evas_Object*)data->unwrap();
        // const char* buf;
        // evas_object_image_file_get(imgData, &buf, NULL);
        // evas_object_image_file_set(eo, buf, NULL);
        void* imgBuf = evas_object_image_data_get(imgData, EINA_FALSE);
        Evas_Colorspace cspace = evas_object_image_colorspace_get(imgData);
        float imgdataW = data->width();
        float imgdataH = data->height();

        if (!shouldApplyEvasMap()) {
            Evas_Object* eo = nullptr;
            eo = evas_object_image_add(m_canvas);
            if (m_objList) {
                m_objList->push_back(eo);
            }

            if (evas_object_evas_get(imgData) == evas_object_evas_get(eo)) {
                evas_object_image_source_set(eo, imgData);
            } else {
                if (((char*)evas_object_data_get(imgData, "local"))[0] == '0') {
                    void* imgBuf =
                        evas_object_image_data_get(imgData, EINA_FALSE);
                    evas_object_image_size_set(eo, data->width(),
                                               data->height());
                    evas_object_image_colorspace_set(
                        eo, evas_object_image_colorspace_get(imgData));
                    evas_object_image_data_set(eo, imgBuf);
                } else {
                    const char* path;
                    evas_object_image_file_get(imgData, &path, NULL);
                    evas_object_image_file_set(eo, path, NULL);
                    evas_object_image_size_set(eo, data->width(),
                                               data->height());
                    evas_object_image_colorspace_set(
                        eo, evas_object_image_colorspace_get(imgData));
                }
            }

            evas_object_image_alpha_set(eo, EINA_TRUE);
            evas_object_image_filled_set(eo, EINA_FALSE);

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

            evas_object_image_fill_set(eo, x, y, imageWidth, imageHeight);
            // evas_object_image_alpha_set(eo, EINA_TRUE);
            // evas_object_anti_alias_set(eo, EINA_TRUE);
            evas_object_move(eo, xx, yy);
            evas_object_resize(eo, ww, hh);

            applyClippers(eo, true);
            applyEvasMapIfNeeded(eo, dst, true);
            evas_object_show(eo);
            m_imageCount++;
        } else {
            // NOTE: Draw all images due to mapping issue for tiled_img
            int cntw = (ww - 1) / imageWidth + 1;
            int cnth = (hh - 1) / imageHeight + 1;
            float curx = 0, cury = 0;
            for (int i = 0; i < cntw; i++) {
                cury = 0;
                for (int j = 0; j < cnth; j++) {
                    Evas_Object* eo = nullptr;
                    eo = evas_object_image_filled_add(m_canvas);
                    if (m_objList) {
                        m_objList->push_back(eo);
                    }

                    if (((char*)evas_object_data_get(imgData, "local"))[0] ==
                        '0') {
                        void* imgBuf =
                            evas_object_image_data_get(imgData, EINA_FALSE);
                        evas_object_image_size_set(eo, data->width(),
                                                   data->height());
                        evas_object_image_colorspace_set(
                            eo, evas_object_image_colorspace_get(imgData));
                        evas_object_image_data_set(eo, imgBuf);
                    } else {
                        const char* path;
                        evas_object_image_file_get(imgData, &path, NULL);
                        evas_object_image_file_set(eo, path, NULL);
                        evas_object_image_size_set(eo, data->width(),
                                                   data->height());
                        evas_object_image_colorspace_set(
                            eo, evas_object_image_colorspace_get(imgData));
                    }

                    evas_object_image_alpha_set(eo, EINA_TRUE);
                    evas_object_move(eo, curx, cury);
                    evas_object_resize(eo, imageWidth, imageHeight);

                    float clipw = (i == cntw - 1) ? ww - curx : imageWidth;
                    float cliph = (j == cnth - 1) ? hh - cury : imageHeight;
                    if (clipw != imageWidth || cliph != imageHeight) {
                        save();
                        clip(Unit::Rect(curx, cury, clipw, cliph));
                    }
                    applyClippers(eo, true);
                    applyEvasMapIfNeeded(
                        eo, Unit::Rect(dst.x() + curx, dst.y() + cury,
                                       imageWidth, imageHeight),
                        true);
                    if (clipw != imageWidth || cliph != imageHeight) {
                        restore();
                    }

                    evas_object_show(eo);
                    cury += imageHeight;
                    m_imageCount++;
                }
                curx += imageWidth;
            }
        }

        if (m_imageCount == 101) {
            STARFISH_LOG_ERROR(
                "paint more than 100 image makes poor performance\n");
        }
    }

    void drawImage(CanvasSurface* data, const Unit::Rect& dst)
    {
        if (!lastState().m_visible) {
            return;
        }
        float xx = 0.0, yy = 0.0, ww = 0.0, hh = 0.0;
        if (lastState().m_mapMode) {
            SkRect sss = SkRect::MakeXYWH(SkFloatToScalar((float)dst.x()),
                                          SkFloatToScalar((float)dst.y()),
                                          SkFloatToScalar((float)dst.width()),
                                          SkFloatToScalar((float)dst.height()));
            if (!shouldApplyEvasMap()) {
                lastState().m_matrix.mapRect(&sss);
            }
            xx = sss.x();
            yy = sss.y();
            ww = sss.width();
            hh = sss.height();
        } else {
            xx = dst.x();
            yy = dst.y();
            if (!shouldApplyEvasMap()) {
                xx = lastState().m_baseX + dst.x();
                yy = lastState().m_baseY + dst.y();
            }
            ww = dst.width();
            hh = dst.height();
        }

        Evas_Object* eo = (Evas_Object*)data->unwrap();
        evas_object_move(eo, xx, yy);
        evas_object_resize(eo, ww, hh);
        evas_object_raise(eo);
#ifdef STARFISH_ENABLE_TEST
        if (evas_object_evas_get(eo) != m_canvas) {
            eo = evas_object_image_add(m_canvas);
            evas_object_move(eo, xx, yy);
            evas_object_resize(eo, ww, hh);
            evas_object_raise(eo);
            evas_object_image_size_set(eo, data->width(), data->height());
            evas_object_image_colorspace_set(
                eo,
                evas_object_image_colorspace_get((Evas_Object*)data->unwrap()));
            evas_object_image_filled_set(eo, EINA_TRUE);
            evas_object_image_alpha_set(eo, EINA_TRUE);
            evas_object_image_data_set(
                eo, evas_object_image_data_get((Evas_Object*)data->unwrap(),
                                               EINA_FALSE));
        }
#endif

        Evas_Object* clip = nullptr;
        if (lastState().m_hasPathClip) {
            clip = createPathClipper(lastState().m_opacity);
            if (!clip) {
                clip = evas_object_rectangle_add(m_canvas);
                int c = 0;
                evas_object_color_set(eo, c, c, c, c);
                evas_object_move(clip, 0, 0);
                evas_object_resize(clip, 0, 0);
                evas_object_show(clip);
            }
        } else {
            clip = evas_object_rectangle_add(m_canvas);
            int c = lastState().m_opacity * 255;
            evas_object_color_set(eo, c, c, c, c);
            evas_object_move(clip, lastState().m_clipRect.x(),
                             lastState().m_clipRect.y());
            evas_object_resize(clip, lastState().m_clipRect.width(),
                               lastState().m_clipRect.height());
            evas_object_show(clip);
        }

        if (m_objList) {
            m_objList->push_back(clip);
        }
        evas_object_clip_set(eo, clip);
        applyEvasMapIfNeeded(eo, dst, true);

        evas_object_show(eo);
        // if (m_surfaceList) {
        //     m_surfaceList->push_back(eo);
        // }
    }

    virtual void postMatrix(const SkMatrix& matrix)
    {
        assureMapMode();
        lastState().m_matrix.preConcat(matrix);
    }

    virtual void applyMatrixTo(LayoutLocation& lp)
    {
        if (lastState().m_mapMode) {
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

    CanvasStateEFL& lastState()
    {
        STARFISH_ASSERT(m_stateSize);
        return m_state[m_stateSize - 1];
    }

    // no rotate, no skew
    bool isMatrixRemainsRectangle()
    {
        if (lastState().m_matrix.isScaleTranslate() ||
            lastState().m_matrix.isIdentity()) {
            return true;
        }
        return false;
    }

    bool shouldApplyEvasMap()
    {
        return lastState().m_mapMode && !lastState().m_matrix.isIdentity();
    }

    void applyEvasMapIfNeeded(Evas_Object* eo, const LayoutRect& dst,
                              bool isImage = false)
    {
        Unit::Rect rt((float)dst.x(), (float)dst.y(), (float)dst.width(),
                      (float)dst.height());
        applyEvasMapIfNeeded(eo, rt, isImage);
    }

    void applyEvasMapIfNeeded(Evas_Object* eo, const Unit::Rect& dst,
                              bool isImage = false)
    {
        if (shouldApplyEvasMap()) {
            Evas_Map* map = evas_map_new(4);

            evas_map_util_points_populate_from_object(map, eo);
            if (isImage) {
                int img_w, img_h;
                evas_object_image_size_get(eo, &img_w, &img_h);
                evas_map_point_image_uv_set(map, 0, 0, 0);
                evas_map_point_image_uv_set(map, 1, img_w, 0);
                evas_map_point_image_uv_set(map, 2, img_w, img_h);
                evas_map_point_image_uv_set(map, 3, 0, img_h);
            }

            {
                SkScalar fromX;
                SkScalar fromY;
                SkPoint to;
                fromX = SkFloatToScalar((float)dst.x());
                fromY = SkFloatToScalar((float)dst.y());
                if (lastState().m_mapMode) {
                    lastState().m_matrix.mapXY(fromX, fromY, &to);
                } else {
                    to.set(lastState().m_baseX + fromX,
                           lastState().m_baseY + fromY);
                }
                evas_map_point_coord_set(map, 0, SkScalarToFloat(to.x()),
                                         SkScalarToFloat(to.y()), 0);

                fromX = SkFloatToScalar((float)(dst.x() + dst.width()));
                fromY = SkFloatToScalar((float)dst.y());
                if (lastState().m_mapMode) {
                    lastState().m_matrix.mapXY(fromX, fromY, &to);
                } else {
                    to.set(lastState().m_baseX + fromX,
                           lastState().m_baseY + fromY);
                }
                evas_map_point_coord_set(map, 1, SkScalarToFloat(to.x()),
                                         SkScalarToFloat(to.y()), 0);

                fromX = SkFloatToScalar((float)(dst.x() + dst.width()));
                fromY = SkFloatToScalar((float)(dst.y() + dst.height()));
                if (lastState().m_mapMode) {
                    lastState().m_matrix.mapXY(fromX, fromY, &to);
                } else {
                    to.set(lastState().m_baseX + fromX,
                           lastState().m_baseY + fromY);
                }
                evas_map_point_coord_set(map, 2, SkScalarToFloat(to.x()),
                                         SkScalarToFloat(to.y()), 0);

                fromX = SkFloatToScalar((float)dst.x());
                fromY = SkFloatToScalar((float)(dst.y() + dst.height()));
                if (lastState().m_mapMode) {
                    lastState().m_matrix.mapXY(fromX, fromY, &to);
                } else {
                    to.set(lastState().m_baseX + fromX,
                           lastState().m_baseY + fromY);
                }
                evas_map_point_coord_set(map, 3, SkScalarToFloat(to.x()),
                                         SkScalarToFloat(to.y()), 0);
            }

            evas_object_anti_alias_set(eo, EINA_TRUE);

            /*
            evas_map_alpha_set(map, EINA_TRUE);
            if (lastState().m_opacity != 1) {
                int c = lastState().m_opacity * 255;
                evas_map_point_color_set(map, 0, c, c, c, c);
                evas_map_point_color_set(map, 1, c, c, c, c);
                evas_map_point_color_set(map, 2, c, c, c, c);
                evas_map_point_color_set(map, 3, c, c, c, c);
            }
            */
            evas_map_smooth_set(map, EINA_TRUE);

            evas_object_map_set(eo, map);
            evas_object_map_enable_set(eo, EINA_TRUE);
            evas_map_free(map);
        }
    }

protected:
    std::vector<CanvasStateEFL> m_state;
    size_t m_stateSize;
    std::unordered_map<Frame*, CanvasStateEFL> m_statePerFrame;
    Evas* m_canvas;
    bool m_directDraw;
    Evas_Object* m_image;
    void* m_buffer;
    unsigned m_width;
    unsigned m_height;
    size_t m_imageCount;
    std::vector<Evas_Object*>* m_objList;
    std::vector<Evas_Object*>* m_surfaceList;
    GCUnorderedMap<ImageData*, std::vector<std::pair<Evas_Object*, bool>>,
                   std::hash<ImageData*>,
                   std::equal_to<ImageData*>>* m_prevDrawnImageMap;
};

Canvas* Canvas::createDirect(void* data)
{
    return new CanvasEFL(data);
}

Canvas* Canvas::create(CanvasSurface* data)
{
    return new CanvasEFL(data);
}
}
#endif
