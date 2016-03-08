#include "StarFishConfig.h"
#include "Canvas.h"
#include "platform/canvas/font/Font.h"
#include "platform/canvas/image/ImageData.h"
#include "style/UnitHelper.h"

#include <Evas.h>
#include <Evas_Engine_Buffer.h>
#include <Elementary.h>
#include <Ecore_X.h>

#include <vector>
#include <SkMatrix.h>

Evas* g_internalCanvas;
namespace StarFish {

extern int g_screenDpi;

/*
static void initInternalCanvas()
{
    if (!g_internalCanvas) {
        Evas *canvas;
        int width=16;
        int height=16;
        Evas_Engine_Info_Buffer *einfo;
        int method;
        void *pixels;
        method = evas_render_method_lookup("buffer");
        if (method <= 0)
        {
            fputs("ERROR: evas was not compiled with 'buffer' engine!\n", stderr);
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        canvas = evas_new();
        if (!canvas)
        {
            fputs("ERROR: could not instantiate new evas canvas.\n", stderr);
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        evas_output_method_set(canvas, method);
        evas_output_size_set(canvas, width, height);
        evas_output_viewport_set(canvas, 0, 0, width, height);
        einfo = (Evas_Engine_Info_Buffer *) evas_engine_info_get(canvas);

        if (!einfo)
        {
            fputs("ERROR: could not get evas engine info!\n", stderr);
            evas_free(canvas);
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }

        // ARGB32 is sizeof(int), that is 4 bytes, per pixel
        pixels = malloc(width * height * sizeof(int));
        if (!pixels)
        {
            fputs("ERROR: could not allocate canvas pixels!\n", stderr);
            evas_free(canvas);
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }

        einfo->info.depth_type = EVAS_ENGINE_BUFFER_DEPTH_ARGB32;
        einfo->info.dest_buffer = pixels;
        einfo->info.dest_buffer_row_bytes = width * sizeof(int);
        einfo->info.use_color_key = 0;
        einfo->info.alpha_threshold = 0;
        einfo->info.func.new_update_region = NULL;
        einfo->info.func.free_update_region = NULL;
        evas_engine_info_set(canvas, (Evas_Engine_Info *) einfo);
        g_internalCanvas = canvas;
    }
}
*/
Evas* internalCanvas()
{
    STARFISH_RELEASE_ASSERT(g_internalCanvas);
    // initInternalCanvas();
    return g_internalCanvas;
}

Evas_Object* createClipper(Evas* canvas, int l, int t, int w, int h)
{
    Evas_Object* eo = evas_object_rectangle_add(canvas);
    evas_object_color_set(eo, 255, 255, 255, 255);
    evas_object_move(eo, l, t);
    evas_object_resize(eo, w, h);
    evas_object_show(eo);

    return eo;
}

class CanvasState {
public:
    SkMatrix m_matrix;
    Color m_color;
    Evas_Object* m_clipper;
    SkRect m_clipRect;
    float m_opacity;
    Font* m_font;
    LayoutUnit m_baseX;
    LayoutUnit m_baseY;
    Color m_underLineColor;
    Color m_lineThroughColor;

    bool m_mapMode;
    bool m_visible;
    bool m_hasUnderLine;
    bool m_hasLineThrough;

    CanvasState()
    {
        m_clipper = NULL;
        m_opacity = 1;
        m_font = nullptr;
        m_mapMode = false;
        m_visible = true;
        m_hasUnderLine = false;
        m_hasLineThrough = false;
    }
};

class CanvasEFL : public Canvas {
    void initFromBuffer(void* buffer, int width, int height, int stride)
    {
        g_screenDpi = ecore_x_dpi_get();
        Evas* canvas;
        int method;
        method = evas_render_method_lookup("buffer");
        if (method <= 0) {
            fputs("ERROR: evas was not compiled with 'buffer' engine!\n", stderr);
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

        save();
    }

public:
    CanvasEFL(void* data)
    {
        g_screenDpi = ecore_x_dpi_get();
        m_image = NULL;
        m_buffer = NULL;
        m_directDraw = true;
        struct dummy {
            void* a;
            void* b;
            int w;
            int h;
            std::vector<Evas_Object*>* objList;
        };
        dummy* d = (dummy*)data;
        m_canvas = (Evas*)d->a;
        m_prevDrawnImageMap = (std::unordered_map<ImageData*, std::vector<std::pair<Evas_Object*, bool> > >*)d->b;

        auto iter = m_prevDrawnImageMap->begin();
        while (iter != m_prevDrawnImageMap->end()) {
            std::vector<std::pair<Evas_Object*, bool> >& vec = iter->second;
            for (unsigned i = 0; i < vec.size(); i++) {
                vec[i].second = false;
            }
            ++iter;
        }
        m_width = d->w;
        m_height = d->h;
        m_objList = d->objList;
        save();
    }

    ~CanvasEFL()
    {
        restore();
        if (m_directDraw) {
            // evas_damage_rectangle_add(m_canvas, 0, 0, m_width, m_height);
        } else {
            evas_render(m_canvas);
            evas_free(m_canvas);
        }

        if (m_image && m_buffer) {
            evas_object_image_data_update_add(m_image, 0, 0, m_width, m_height);
        }

        if (m_prevDrawnImageMap) {
            auto iter = m_prevDrawnImageMap->begin();

            while (iter != m_prevDrawnImageMap->end()) {
                std::vector<std::pair<Evas_Object*, bool> >& vec = iter->second;
                for (unsigned i = 0; i < vec.size();) {
                    if (!vec[i].second) {
                        evas_object_hide(vec[i].first);
                        // evas_object_resize(vec[i].first, 0, 0);
                    }
                    i++;
                    /*
                    if (!vec[i].second) {
                        evas_object_hide(vec[i].first);
                        evas_object_del(vec[i].first);
                        vec.erase(vec.begin() + i);
                    } else {
                        i++;
                    }*/
                }

                ++iter;
            }
        }
    }

    virtual void clearColor(const Color& clr)
    {
        Evas_Object* eo = evas_object_rectangle_add(m_canvas);
        if (m_objList)
            m_objList->push_back(eo);
        int r = clr.r(), g = clr.g(), b = clr.b();

        evas_color_argb_premul(clr.a(), &r, &g, &b);
        evas_object_color_set(eo, r, g, b, clr.a());
        evas_object_move(eo, 0, 0);
        evas_object_resize(eo, m_width, m_height);
        applyClippers(eo);
        evas_object_show(eo);
    }

    // state
    virtual void save()
    {
        CanvasState state;
        if (m_state.size()) {
            state.m_matrix = lastState().m_matrix;
            state.m_clipRect = lastState().m_clipRect;
            state.m_clipper = lastState().m_clipper;
            state.m_color = lastState().m_color;
            state.m_opacity = lastState().m_opacity;
            state.m_baseX = lastState().m_baseX;
            state.m_baseY = lastState().m_baseY;
            state.m_font = lastState().m_font;
            state.m_mapMode = lastState().m_mapMode;
            state.m_visible = lastState().m_visible;
            state.m_hasUnderLine = lastState().m_hasUnderLine;
            state.m_hasLineThrough = lastState().m_hasLineThrough;
            state.m_underLineColor = lastState().m_underLineColor;
            state.m_lineThroughColor = lastState().m_lineThroughColor;
        } else {
            state.m_matrix.reset();
            state.m_clipRect.setLTRB(0, 0, SkFloatToScalar((float)m_width), SkFloatToScalar((float)m_height));
            state.m_clipper = NULL;
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
        if (lastState().m_mapMode)
            return;
        lastState().m_matrix.preTranslate(lastState().m_baseX, lastState().m_baseY);
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
        lastState().m_matrix.preScale(SkDoubleToScalar(x), SkDoubleToScalar(y), SkDoubleToScalar(ox), SkDoubleToScalar(oy));
    }

    virtual void rotate(double angle)
    {
        assureMapMode();
        lastState().m_matrix.preRotate(SkDoubleToScalar(angle));
    }

    virtual void rotate(double angle, double ox, double oy)
    {
        assureMapMode();
        lastState().m_matrix.preRotate(SkDoubleToScalar(angle), SkDoubleToScalar(ox), SkDoubleToScalar(oy));
    }

    virtual void translate(double x, double y)
    {
        if (lastState().m_mapMode)
            lastState().m_matrix.preTranslate(x, y);
        else {
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

    virtual void clip(const Rect& rt)
    {
        if (hasMatrixAffine()) {
            STARFISH_ASSERT_NOT_REACHED();
        }
        SkRect sss = SkRect::MakeXYWH(
            SkFloatToScalar((float)rt.x()),
            SkFloatToScalar((float)rt.y()),
            SkFloatToScalar((float)rt.width()),
            SkFloatToScalar((float)rt.height()));

        assureMapMode();
        lastState().m_matrix.mapRect(&sss);

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

    void applyClippers(Evas_Object* eo)
    {
        if (!lastState().m_clipper) {
            lastState().m_clipper = createClipper(m_canvas, lastState().m_clipRect.x(), lastState().m_clipRect.y(),
                lastState().m_clipRect.width(), lastState().m_clipRect.height());
            if (m_objList)
                m_objList->push_back(lastState().m_clipper);
        }
        evas_object_clip_set(eo, lastState().m_clipper);
    }

    virtual void setColor(const Color& clr_)
    {
        Color clr = clr_;
        int r = clr.r(), g = clr.g(), b = clr.b();
        evas_color_argb_premul(clr.a(), &r, &g, &b);
        clr.m_a = clr_.m_a;
        clr.m_r = r;
        clr.m_g = g;
        clr.m_b = b;
        lastState().m_color = clr;
    }

    virtual void setVisible(bool visible)
    {
        lastState().m_visible = visible;
    }

    virtual Color color()
    {
        int r = lastState().m_color.r(), g = lastState().m_color.g(), b = lastState().m_color.b();
        evas_color_argb_unpremul(lastState().m_color.a(), &r, &g, &b);
        Color clr(r, g, b, lastState().m_color.a());
        return clr;
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

    virtual void setUnderlineColor(Color clr)
    {
        lastState().m_underLineColor = clr;
    }

    virtual void setLineThroughColor(Color clr)
    {
        lastState().m_lineThroughColor = clr;
    }

    void drawEvasRect(int xx, int yy, int ww, int hh, const Rect& rt)
    {
        Evas_Object* eo = evas_object_rectangle_add(m_canvas);
        if (m_objList)
            m_objList->push_back(eo);
        evas_object_color_set(eo, lastState().m_color.r(), lastState().m_color.g(), lastState().m_color.b(), lastState().m_color.a());
        evas_object_move(eo, xx, yy);
        evas_object_resize(eo, ww, hh);
        applyClippers(eo);
        applyEvasMapIfNeeded(eo, rt);
        evas_object_show(eo);
    }

    virtual void drawRect(const Rect& rt)
    {
        if (!lastState().m_visible) {
            return;
        }
        float xx = 0.0, yy = 0.0, ww = 0.0, hh = 0.0;
        if (lastState().m_mapMode) {
            SkRect sss = SkRect::MakeXYWH(
                SkFloatToScalar((float)rt.x()),
                SkFloatToScalar((float)rt.y()),
                SkFloatToScalar((float)rt.width()),
                SkFloatToScalar((float)rt.height()));
            if (!shouldApplyEvasMap())
                lastState().m_matrix.mapRect(&sss);
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

    virtual void drawRect(const LayoutRect& rt)
    {
        if (!lastState().m_visible) {
            return;
        }

        int xx = 0, yy = 0, ww = 0, hh = 0;
        if (lastState().m_mapMode) {
            SkRect sss = SkRect::MakeXYWH(
                SkFloatToScalar((float)rt.x()),
                SkFloatToScalar((float)rt.y()),
                SkFloatToScalar((float)rt.width()),
                SkFloatToScalar((float)rt.height()));
            if (!shouldApplyEvasMap())
                lastState().m_matrix.mapRect(&sss);
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
        drawEvasRect(xx, yy, ww, hh, Rect(xx, yy, ww, hh));
    }

    virtual void drawRect(LayoutLocation p1, LayoutLocation p2, LayoutLocation p3, LayoutLocation p4)
    {
        STARFISH_ASSERT(!hasMatrixAffine());
        STARFISH_ASSERT(!lastState().m_mapMode);
        if (!lastState().m_visible) {
            return;
        }

        p1.setX(p1.x() + lastState().m_baseX);
        p1.setY(p1.y() + lastState().m_baseY);

        p2.setX(p2.x() + lastState().m_baseX);
        p2.setY(p2.y() + lastState().m_baseY);

        p3.setX(p3.x() + lastState().m_baseX);
        p3.setY(p3.y() + lastState().m_baseY);

        p4.setX(p4.x() + lastState().m_baseX);
        p4.setY(p4.y() + lastState().m_baseY);

        Evas_Object* eo = evas_object_polygon_add(m_canvas);
        if (m_objList)
            m_objList->push_back(eo);
        evas_object_color_set(eo, lastState().m_color.r(), lastState().m_color.g(), lastState().m_color.b(), lastState().m_color.a());

        evas_object_polygon_point_add(eo, p1.x().floor(), p1.y().floor());
        evas_object_polygon_point_add(eo, p2.x().floor(), p2.y().floor());
        evas_object_polygon_point_add(eo, p3.x().floor(), p3.y().floor());
        evas_object_polygon_point_add(eo, p4.x().floor(), p4.y().floor());

        if (lastState().m_opacity != 1) {
            Evas_Map* map = evas_map_new(4);

            evas_map_util_points_populate_from_object(map, eo);
            evas_map_alpha_set(map, EINA_TRUE);

            int c = lastState().m_opacity * 255;
            evas_map_point_color_set(map, 0, c, c, c, c);
            evas_map_point_color_set(map, 1, c, c, c, c);
            evas_map_point_color_set(map, 2, c, c, c, c);
            evas_map_point_color_set(map, 3, c, c, c, c);

            evas_object_map_set(eo, map);
            evas_object_map_enable_set(eo, EINA_TRUE);
            evas_map_free(map);
        }

        evas_object_show(eo);
    }

    virtual void drawText(LayoutUnit x, LayoutUnit y, String* text)
    {
        if (!lastState().m_visible) {
            return;
        }

#ifdef STARFISH_ENABLE_PIXEL_TEST
        if (g_enablePixelTest) {
            if (!text->equals(String::spaceString)) {
                float h = lastState().m_font->size();
                float xx = x;
                for (size_t i = 0; i < text->length(); i++) {
                    char32_t ch = text->charAt(i);
                    if (ch == 160) { // nbsp

                    } else {
                        if (ch != 'p') {
                            Rect rt(xx, y, h, h);
                            drawRect(rt);
                        } else {
                            save();
                            clip(Rect(xx, y, h, h));
                            g_enablePixelTest = false;
                            drawText(xx, y, String::createASCIIString("p"));
                            g_enablePixelTest = true;
                            restore();
                        }
                    }
                    xx += h;
                }
            }
            return;
        }
#endif

        if (!lastState().m_hasUnderLine && !lastState().m_hasLineThrough) {
            if (text->equals(String::spaceString))
                return;

            Evas_Object* eo = evas_object_text_add(m_canvas);
            if (m_objList)
                m_objList->push_back(eo);
            LayoutSize sz(lastState().m_font->measureText(text), lastState().m_font->metrics().m_fontHeight);
            LayoutRect rt(x, y, sz.width(), sz.height());

            LayoutUnit xx = 0, yy = 0;
            if (lastState().m_mapMode) {
                SkRect sss = SkRect::MakeXYWH(
                    SkFloatToScalar((float)rt.x()),
                    SkFloatToScalar((float)rt.y()),
                    SkFloatToScalar((float)rt.width()),
                    SkFloatToScalar((float)rt.height()));
                if (!shouldApplyEvasMap())
                    lastState().m_matrix.mapRect(&sss);
                xx = sss.x();
                yy = sss.y();
            } else {
                if (!shouldApplyEvasMap()) {
                    xx = lastState().m_baseX + rt.x();
                    yy = lastState().m_baseY + rt.y();
                } else {
                    xx = x;
                    yy = y;
                }
            }

            int siz;
            evas_object_text_font_get((Evas_Object*)lastState().m_font->unwrap(), NULL, &siz);
            float ptSize = siz;
            evas_object_text_font_set(eo, lastState().m_font->familyName()->utf8Data(), ptSize);
            evas_object_color_set(eo, lastState().m_color.r(), lastState().m_color.g(), lastState().m_color.b(), lastState().m_color.a());
            evas_object_text_text_set(eo, text->utf8Data());

            evas_object_move(eo, (int)xx, (int)yy);
            applyClippers(eo);
            applyEvasMapIfNeeded(eo, rt);

            evas_object_show(eo);
        } else {
            if (text->equals(String::spaceString)) {
                // FIXME evas textblock doesn't render 1 length space char
                text = text->concat(String::spaceString);
            }

            Evas_Object* eo = evas_object_textblock_add(m_canvas);
            if (m_objList)
                m_objList->push_back(eo);
            LayoutSize sz(lastState().m_font->measureText(text), lastState().m_font->metrics().m_fontHeight);
            LayoutRect rt(x, y, sz.width(), sz.height());

            float xx = 0.0, yy = 0.0, ww = 0.0, hh = 0.0;
            if (lastState().m_mapMode) {
                SkRect sss = SkRect::MakeXYWH(
                    SkFloatToScalar((float)rt.x()),
                    SkFloatToScalar((float)rt.y()),
                    SkFloatToScalar((float)rt.width()),
                    SkFloatToScalar((float)rt.height())
                    );
                if (!shouldApplyEvasMap())
                    lastState().m_matrix.mapRect(&sss);
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
            evas_object_text_font_get((Evas_Object*)lastState().m_font->unwrap(), NULL, &siz);
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
            }

            const char* fontStyle = "normal";

            Font* fnt = lastState().m_font;
            if (fnt->style() == FontStyleItalic)
                fontStyle = "italic";
            else if (fnt->style() == FontStyleOblique)
                fontStyle = "oblique";

            const char* underlineMode = lastState().m_hasUnderLine ? "on" : "off";
            const char* lineThroughMode = lastState().m_hasLineThrough ? "on" : "off";

            char underlineColor[128];
            char lineThroughColor[128];
            snprintf(underlineColor, 128, "#%02x%02x%02x%02x", (int)lastState().m_underLineColor.r(), (int)lastState().m_underLineColor.g(), (int)lastState().m_underLineColor.b(), (int)lastState().m_underLineColor.a());
            snprintf(lineThroughColor, 128, "#%02x%02x%02x%02x", (int)lastState().m_lineThroughColor.r(), (int)lastState().m_lineThroughColor.g(), (int)lastState().m_lineThroughColor.b(), (int)lastState().m_lineThroughColor.a());

            snprintf(buf, 512, "DEFAULT='font=%s font_size=%f color=#%02x%02x%02x%02x valign=middle font_weight=%s font_style=%s strikethrough=%s strikethrough_color=%s underline=%s underline_color=%s '", lastState().m_font->familyName()->utf8Data(), ptSize,
                (int)lastState().m_color.r(), (int)lastState().m_color.g(), (int)lastState().m_color.b(), (int)lastState().m_color.a(), weight, fontStyle, lineThroughMode, lineThroughColor, underlineMode, underlineColor);
            evas_textblock_style_set(st, buf);
            evas_object_textblock_style_set(eo, st);
            evas_object_color_set(eo, lastState().m_color.r(), lastState().m_color.g(), lastState().m_color.b(), lastState().m_color.a());
            evas_object_textblock_text_markup_set(eo, text->utf8Data());

            evas_object_resize(eo, ww, hh);
            evas_object_move(eo, xx, yy);

            applyClippers(eo);
            applyEvasMapIfNeeded(eo, rt);

            evas_object_show(eo);
            evas_textblock_style_free(st);
        }
    }

    Evas_Object* findPrevDrawnData(ImageData* data)
    {
        if (m_prevDrawnImageMap) {
            Evas_Object* eo = NULL;
            std::vector<std::pair<Evas_Object*, bool> >& vec = (*m_prevDrawnImageMap)[data];
            for (unsigned i = 0; i < vec.size(); i++) {
                if (!vec[i].second) {
                    eo = vec[i].first;
                    evas_object_raise(eo);
                    evas_object_map_enable_set(eo, EINA_FALSE);
                    vec[i].second = true;
                    break;
                }
            }
            return eo;
        } else {
            return NULL;
        }
    }

    void pushPrevDrawnData(ImageData* data, Evas_Object* eo)
    {
        if (m_prevDrawnImageMap) {
            std::vector<std::pair<Evas_Object*, bool> >& vec = (*m_prevDrawnImageMap)[data];
            vec.push_back(std::pair<Evas_Object*, bool>(eo, true));
        }
    }
    virtual void drawImage(ImageData* data, const Rect& dst)
    {
        if (!lastState().m_visible) {
            return;
        }

        float xx = 0.0, yy = 0.0, ww = 0.0, hh = 0.0;
        if (lastState().m_mapMode) {
            SkRect sss = SkRect::MakeXYWH(
                SkFloatToScalar((float)dst.x()),
                SkFloatToScalar((float)dst.y()),
                SkFloatToScalar((float)dst.width()),
                SkFloatToScalar((float)dst.height()));
            if (!shouldApplyEvasMap())
                lastState().m_matrix.mapRect(&sss);
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
        Evas_Object* eo = findPrevDrawnData(data);

        if (!eo) {
            eo = evas_object_image_add(m_canvas);
            if (!m_prevDrawnImageMap) {
                if (m_objList)
                    m_objList->push_back(eo);
            }

            Evas_Object* imgData = (Evas_Object*)data->unwrap();
            void* imgBuf = evas_object_image_data_get(imgData, EINA_FALSE);

            evas_object_image_size_set(eo, data->width(), data->height());
            evas_object_image_colorspace_set(eo, evas_object_image_colorspace_get(imgData));
            evas_object_image_data_set(eo, imgBuf);
            evas_object_image_filled_set(eo, EINA_TRUE);
            evas_object_image_alpha_set(eo, EINA_TRUE);
            // evas_object_anti_alias_set(eo, EINA_TRUE);

            pushPrevDrawnData(data, eo);
        }

        evas_object_move(eo, xx, yy);
        evas_object_resize(eo, ww, hh);

        applyClippers(eo);
        applyEvasMapIfNeeded(eo, dst, true);
        evas_object_show(eo);
    }

    virtual void setMatrix(const SkMatrix& matrix)
    {
        lastState().m_mapMode = true;
        lastState().m_matrix = matrix;
    }

    virtual SkMatrix matrix()
    {
        assureMapMode();
        return lastState().m_matrix;
    }

    virtual void applyMatrixTo(LayoutLocation lp)
    {
        if (lastState().m_mapMode) {
            STARFISH_ASSERT(!hasMatrixAffine());
            SkPoint point = SkPoint::Make((float)lp.x(), (float)lp.y());
            lastState().m_matrix.mapPoints(&point, 1);
            lp.setX(point.x());
            lp.setY(point.y());
        } else {
            lp.setX(lp.x() + lastState().m_baseX);
            lp.setY(lp.y() + lastState().m_baseY);
        }
    }

    virtual void applyMatrixTo(LayoutRect lp)
    {
        if (lastState().m_mapMode) {
            STARFISH_ASSERT(!hasMatrixAffine());
            SkRect sss = SkRect::MakeXYWH(
                SkFloatToScalar((float)lp.x()),
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

    bool hasMatrixAffine()
    {
        return lastState().m_matrix.getType() & SkMatrix::TypeMask::kAffine_Mask;
    }

    bool shouldApplyEvasMap()
    {
        if (lastState().m_opacity != 1) {
            return true;
        }
        return hasMatrixAffine();
    }

    void applyEvasMapIfNeeded(Evas_Object* eo, const LayoutRect& dst, bool isImage = false)
    {
        Rect rt((float)dst.x(), (float)dst.y(), (float)dst.width(), (float)dst.height());
        applyEvasMapIfNeeded(eo, rt, isImage);
    }

    void applyEvasMapIfNeeded(Evas_Object* eo, const Rect& dst, bool isImage = false)
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
                    to.set(lastState().m_baseX + fromX, lastState().m_baseY + fromY);
                }
                evas_map_point_coord_set(map, 0, SkScalarToFloat(to.x()), SkScalarToFloat(to.y()), 0);

                fromX = SkFloatToScalar((float)(dst.x() + dst.width()));
                fromY = SkFloatToScalar((float)dst.y());
                if (lastState().m_mapMode) {
                    lastState().m_matrix.mapXY(fromX, fromY, &to);
                } else {
                    to.set(lastState().m_baseX + fromX, lastState().m_baseY + fromY);
                }
                evas_map_point_coord_set(map, 1, SkScalarToFloat(to.x()), SkScalarToFloat(to.y()), 0);

                fromX = SkFloatToScalar((float)(dst.x() + dst.width()));
                fromY = SkFloatToScalar((float)(dst.y() + dst.height()));
                if (lastState().m_mapMode) {
                    lastState().m_matrix.mapXY(fromX, fromY, &to);
                } else {
                    to.set(lastState().m_baseX + fromX, lastState().m_baseY + fromY);
                }
                evas_map_point_coord_set(map, 2, SkScalarToFloat(to.x()), SkScalarToFloat(to.y()), 0);

                fromX = SkFloatToScalar((float)dst.x());
                fromY = SkFloatToScalar((float)(dst.y() + dst.height()));
                if (lastState().m_mapMode) {
                    lastState().m_matrix.mapXY(fromX, fromY, &to);
                } else {
                    to.set(lastState().m_baseX + fromX, lastState().m_baseY + fromY);
                }
                evas_map_point_coord_set(map, 3, SkScalarToFloat(to.x()), SkScalarToFloat(to.y()), 0);
            }

            evas_object_anti_alias_set(eo, EINA_TRUE);

            evas_map_alpha_set(map, EINA_TRUE);
            if (lastState().m_opacity != 1) {
                int c = lastState().m_opacity * 255;
                evas_map_point_color_set(map, 0, c, c, c, c);
                evas_map_point_color_set(map, 1, c, c, c, c);
                evas_map_point_color_set(map, 2, c, c, c, c);
                evas_map_point_color_set(map, 3, c, c, c, c);
            }

            evas_object_map_set(eo, map);
            evas_object_map_enable_set(eo, EINA_TRUE);
            evas_map_free(map);
        }
    }

protected:
    std::vector<CanvasState> m_state;
    Evas* m_canvas;
    bool m_directDraw;
    Evas_Object* m_image;
    void* m_buffer;
    unsigned m_width;
    unsigned m_height;
    std::vector<Evas_Object*>* m_objList;
    std::unordered_map<ImageData*, std::vector<std::pair<Evas_Object*, bool> > >* m_prevDrawnImageMap;
};

Canvas* Canvas::createDirect(void* data)
{
    return new CanvasEFL(data);
}
}
