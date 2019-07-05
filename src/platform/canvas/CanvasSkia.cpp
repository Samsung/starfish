/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"

#if defined(PORT_CANVAS_BACKEND_SKIA)
#include "Starfish.h"
#include "core/style/Style.h"
#include "core/dom/canvas/CanvasLineCap.h"
#include "core/dom/canvas/CanvasLineJoin.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/font/Font.h"
#include "platform/canvas/font/FontImplSkia.h"
#include "core/modules/canvas/NativeGradient.h"
#include "core/style/CSSGradientValue.h"
#include "core/modules/canvas/image/NativeImageData.h"
#include "core/modules/canvas/NativePattern.h"
#include "core/style/GradientData.h"
#include "core/style/UnitHelper.h"
#include "core/page/WebView.h"

#include "SkBitmap.h"
#include "SkCanvas.h"
#include "SkDashPathEffect.h"
#include "SkFontMgr.h"
#include "SkPath.h"
#include "SkPoint.h"
#include "SkPixmap.h"
#include "SkShader.h"
#include "SkSurface.h"
#include "SkTypeface.h"
#include "SkGradientShader.h"

#include "core/modules/canvas/Path.h"
#include "platform/canvas/PathSkia.h"
#include "core/dom/canvas/CanvasDirection.h"
#include "core/dom/canvas/CanvasTextAlign.h"
#include "core/dom/canvas/CanvasTextBaseline.h"

#define CLAMP(value, min, max) \
    (((value) > (max)) ? (max) : (((value) < (min)) ? (min) : (value)))

namespace Starfish {

class CanvasStateSkia : public CanvasState {
public:
    CanvasStateSkia()
        : CanvasState()
        , m_fillColor(Unit::Color())
        , m_strokeColor(Unit::Color())
        , m_fillType(SkPath::FillType::kWinding_FillType)
        , m_strokeWidth(0.0f)
    {
    }
    // FiXME : Remove it after applies a CanvasFillStrokeSource
    Unit::Color m_fillColor;
    Unit::Color m_strokeColor;
    SkPath::FillType m_fillType;
    float m_strokeWidth;

    void* operator new(size_t size)
    {
        STARFISH_ASSERT(size == sizeof(CanvasStateSkia));
        static bool typeInited = false;
        static GC_descr descr;
        if (typeInited == false) {
            GC_word obj_bitmap[GC_BITMAP_SIZE(CanvasStateSkia)] = { 0 };
            CanvasStateSkia::fillGCDescriptor(obj_bitmap);
            descr =
                GC_make_descriptor(obj_bitmap, GC_WORD_LEN(CanvasStateSkia));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* obj_bitmap)
    {
        STARFISH_ASSERT(obj_bitmap != nullptr);
        CanvasState::fillGCDescriptor(obj_bitmap);
    }
};

class NativeGradientSkia : public NativeGradient {
public:
    NativeGradientSkia(GradientDrawingInfo* info)
        : NativeGradient(info)
    {
        init(info);
    }

    NativeGradientSkia(double x0, double y0, double x1, double y1)
        : NativeGradient()
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    NativeGradientSkia(double x0, double y0, double r0, double x1, double y1,
                       double r1)
        : NativeGradient()
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    ~NativeGradientSkia()
    {
    }

    sk_sp<SkShader> shader()
    {
        return m_shader;
    }

private:
    void init(GradientDrawingInfo* info)
    {
        size_t colorCount = info->colorStops.size();
        SkColor colors[colorCount];
        SkScalar pos[colorCount];
        for (size_t i = 0; i < colorCount; i++) {
            Unit::Color clr = info->colorStops[i]->color();
            colors[i] = SkColorSetARGB(clr.a(), clr.r(), clr.g(), clr.b());
            pos[i] = info->colorStops[i]->offset().percent();
        }

        if (info->type == GradientType::LinearGradient) {
            SkPoint points[] = { { info->x1, info->y1 },
                                 { info->x2, info->y2 } };
            m_shader = SkGradientShader::MakeLinear(
                points, colors, pos, colorCount, SkShader::kClamp_TileMode, 0,
                nullptr);
        } else if (info->type == GradientType::RadialGradient) {
            m_shader = SkGradientShader::MakeRadial(
                { info->x1, info->y1 }, info->r2, colors, pos, colorCount,
                SkShader::kClamp_TileMode);
        } else {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        }
    }

    sk_sp<SkShader> m_shader;
};

std::shared_ptr<NativeGradient> NativeGradient::create(
    GradientDrawingInfo* info)
{
    return std::shared_ptr<NativeGradient>(new NativeGradientSkia(info));
}

std::shared_ptr<NativeGradient> NativeGradient::create(double x0, double y0,
                                                       double x1, double y1)
{
    return std::shared_ptr<NativeGradient>(
        new NativeGradientSkia(x0, y0, x1, y1));
}

std::shared_ptr<NativeGradient> NativeGradient::create(double x0, double y0,
                                                       double r0, double x1,
                                                       double y1, double r1)
{
    return std::shared_ptr<NativeGradient>(
        new NativeGradientSkia(x0, y0, r0, x1, y1, r1));
}

class NativePatternSkia : public NativePattern {
public:
    NativePatternSkia(NULLABLE NativeImageData* image, bool repeatX,
                      bool repeatY)
        : NativePattern(image, repeatX, repeatY)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    ~NativePatternSkia()
    {
    }

private:
};

std::shared_ptr<NativePattern> NativePattern::create(
    NULLABLE NativeImageData* image, bool repeatX, bool repeatY)
{
    return std::shared_ptr<NativePattern>(
        new NativePatternSkia(image, repeatX, repeatY));
}

class CanvasSkia : public Canvas {
    void initFromBuffer(void* buffer, int width, int height, int stride)
    {
        m_renderTargetInfo.m_buffer = (uint8_t*)buffer;
        m_renderTargetInfo.m_width = width;
        m_renderTargetInfo.m_height = height;
        m_renderTargetInfo.m_stride = stride;

        SkImageInfo info = SkImageInfo::MakeN32Premul(
            m_renderTargetInfo.m_width, m_renderTargetInfo.m_height);
        m_surface = SkSurface::MakeRasterDirect(info, buffer, stride);
        m_canvas = m_surface->getCanvas();
        applyDevicePixelRatio(m_canvas);
    }

    void initFromNativeImageData(NativeImageData* data)
    {
        m_renderTargetInfo.m_buffer = (uint8_t*)data->data();
        m_renderTargetInfo.m_width = data->width();
        m_renderTargetInfo.m_height = data->height();
        m_renderTargetInfo.m_stride = data->stride();

        SkImageInfo info = SkImageInfo::MakeN32Premul(
            m_renderTargetInfo.m_width, m_renderTargetInfo.m_height);
        m_surface =
            SkSurface::MakeRasterDirect(info, data->data(), data->stride());
        m_canvas = m_surface->getCanvas();
        applyDevicePixelRatio(m_canvas);
    }

    void applyDevicePixelRatio(SkCanvas* canvas)
    {
        canvas->scale(m_webView->screenInfo().devicePixelRatio,
                      m_webView->screenInfo().devicePixelRatio);
    }

public:
    CanvasSkia(WebView* webView, void* buffer, int width, int height,
               int stride)
    {
        STARFISH_ASSERT(webView != nullptr);
        STARFISH_ASSERT(buffer != nullptr);

        m_shouldDestroySkia = true;
        m_shouldDestroySurface = true;
        m_webView = webView;
        m_canvas = nullptr;
        m_surface = nullptr;
        m_renderTargetInfo.m_width = width;
        m_renderTargetInfo.m_height = height;

        SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
        m_surface = SkSurface::MakeRasterDirect(info, buffer, stride);
        m_canvas = m_surface->getCanvas();

        applyDevicePixelRatio(m_canvas);
        save();
    }

    CanvasSkia(WebView* webView, CanvasSurface* data)
    {
        STARFISH_ASSERT(webView != nullptr);
        STARFISH_ASSERT(data != nullptr);

        m_webView = webView;
        m_canvas = nullptr;
        m_surface = nullptr;
        m_shouldDestroySkia = true;
        m_shouldDestroySurface = true;

        initFromBuffer(data->mapBuffer(), data->bufferWidth(),
                       data->bufferHeight(), data->bufferStride());

        save();
    }

    CanvasSkia(WebView* webView, NativeImageData* data)
    {
        STARFISH_ASSERT(webView != nullptr);
        STARFISH_ASSERT(data != nullptr);

        m_shouldDestroySkia = true;
        m_shouldDestroySurface = false;
        m_webView = webView;
        m_canvas = nullptr;
        m_surface = nullptr;

        initFromNativeImageData(data);
        save();
    }

    virtual ~CanvasSkia()
    {
        restore();
        STARFISH_ASSERT(m_state.size() == 0);
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
        INSTALL_PROFILE_TIMER("CanvasSkia::clear");
        m_canvas->save();
        if (clr.a() == 0) {
            m_canvas->clear(SK_ColorTRANSPARENT);
        } else {
            m_canvas->clear(SkColorSetARGB(clr.a(), clr.r(), clr.g(), clr.b()));
        }
        m_canvas->restore();
    }

    virtual void flush()
    {
        m_canvas->flush();
    }

    // state
    virtual void save() // push state on state stack
    {
        if (m_stateMemoryPool.size() == 0) {
            CanvasStateSkia* state = new CanvasStateSkia();
            memset(state, 0, sizeof(CanvasStateSkia));
            m_stateMemoryPool.push_back(state);
        }

        Canvas::save();

        if (m_state.size() >= 2) {
            CanvasStateSkia* state = (CanvasStateSkia*)m_state.back();
            CanvasStateSkia* lastState =
                (CanvasStateSkia*)(*(m_state.end() - 2));
            state->m_fillColor = lastState->m_fillColor;
            state->m_strokeColor = lastState->m_strokeColor;
            state->m_fillType = lastState->m_fillType;
            state->m_strokeWidth = lastState->m_strokeWidth;
        }

        m_canvas->save();
    }

    virtual void restore() // pop state stack and restore state
    {
        Canvas::restore();
        m_canvas->restore();
    }

    virtual void scale(double x, double y)
    {
        m_canvas->scale(x, y);
    }

    virtual void scale(double x, double y, double ox, double oy)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    virtual void rotate(double angle)
    {
        m_canvas->rotate(UnitHelper::convertFromRadToDeg(angle));
    }

    virtual void rotate(double angle, double ox, double oy)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    virtual void translate(double x, double y)
    {
        m_canvas->translate(x, y);
    }

    virtual void translate(LayoutUnit x, LayoutUnit y)
    {
        m_canvas->translate(x.toDouble(), y.toDouble());
    }

    virtual void postMatrix(const SkMatrix& matrix)
    {
        m_canvas->concat(matrix);
    }

    virtual void setMatrix(const SkMatrix& matrix)
    {
        m_canvas->setMatrix(matrix);
    }

    virtual SkMatrix currentTransformMatrix()
    {
        return m_canvas->getTotalMatrix();
    }

    virtual void clip(const Unit::Rect& rt)
    {
        moveTo(rt.x(), rt.y());
        lineTo(rt.x() + rt.width(), rt.y());
        lineTo(rt.x() + rt.width(), rt.y() + rt.height());
        lineTo(rt.x(), rt.y() + rt.height());
        lineTo(rt.x(), rt.y());
        closePath();
        clipPath();
    }

    virtual LayoutRect pixelSnappedClip(const LayoutRect& rt)
    {
        if (rt.width() == 0 || rt.height() == 0) {
            clip(Unit::Rect(0, 0, 0, 0));
            return LayoutRect(0, 0, 0, 0);
        }
        LayoutRect deviceRect;
        double xSoFar = rt.x();
        double ySoFar = rt.y();
        double maxXSoFar = rt.maxX();
        double maxYSoFar = rt.maxY();

        SkMatrix m = m_canvas->getTotalMatrix();
        SkPoint point;
        m.mapXY(xSoFar, ySoFar, &point);
        xSoFar = point.x();
        ySoFar = point.y();

        m.mapXY(maxXSoFar, maxYSoFar, &point);
        maxXSoFar = point.x();
        maxYSoFar = point.y();

        double x = std::min(xSoFar, maxXSoFar);
        double y = std::min(ySoFar, maxYSoFar);
        double maxX = std::max(xSoFar, maxXSoFar);
        double maxY = std::max(ySoFar, maxYSoFar);

        x = floor(x);
        y = floor(y);
        maxX = ceil(maxX);
        maxY = ceil(maxY);

        if (x < 0) {
            x = 0;
        } else if (x > (int)m_renderTargetInfo.m_width) {
            maxX = x = m_renderTargetInfo.m_width;
        }
        if (y < 0) {
            y = 0;
        } else if (y > (int)m_renderTargetInfo.m_height) {
            maxY = y = m_renderTargetInfo.m_height;
        }

        if (maxX < 0) {
            maxX = 0;
        } else if (maxX > (int)m_renderTargetInfo.m_width) {
            maxX = m_renderTargetInfo.m_width;
        }
        if (maxY < 0) {
            maxY = 0;
        } else if (maxY > (int)m_renderTargetInfo.m_height) {
            maxY = m_renderTargetInfo.m_height;
        }

        deviceRect.setX(x);
        deviceRect.setY(y);
        deviceRect.setWidth(maxX - x);
        deviceRect.setHeight(maxY - y);

        SkPath path;
        path.reset();
        path.moveTo(x, y);
        path.lineTo(maxX, y);
        path.lineTo(maxX, maxY);
        path.lineTo(x, maxY);
        path.lineTo(x, y);
        path.close();
        m_canvas->resetMatrix();
        m_canvas->clipPath(path);
        m_canvas->setMatrix(m);

        return deviceRect;
    }

    virtual void unsetDevicePixelRatio()
    {
        m_canvas->scale(1 / m_webView->screenInfo().devicePixelRatio,
                        1 / m_webView->screenInfo().devicePixelRatio);
    }

    virtual void setFillColor(const Unit::Color& clr)
    {
        STARFISH_ASSERT(m_canvas);
        ((CanvasStateSkia*)lastState())->m_fillColor = clr;
    }

    virtual void setFillSource(CanvasFillStrokeSource* source)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    virtual CanvasFillStrokeSource* fillSource()
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        return new CanvasFillStrokeSource();
    }

    virtual void setStrokeColor(const Unit::Color& clr)
    {
        STARFISH_ASSERT(m_canvas);
        ((CanvasStateSkia*)lastState())->m_strokeColor = clr;
    }

    virtual void setStrokeSource(CanvasFillStrokeSource* source)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    virtual CanvasFillStrokeSource* strokeSource()
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        return new CanvasFillStrokeSource();
    }

    virtual void setGlobalAlpha(float c)
    {
        lastState()->m_globalAlpha = c;
    }

    virtual void setCompositeOperator(CanvasCompositeOperator oper,
                                      CanvasBlendMode mode)
    {
        lastState()->m_compositeOperator = oper;
        lastState()->m_blendMode = mode;

        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    virtual CanvasCompositeOperator compositeOperator()
    {
        return lastState()->m_compositeOperator;
    }

    virtual CanvasBlendMode blendMode()
    {
        return lastState()->m_blendMode;
    }

    virtual float globalAlpha()
    {
        return lastState()->m_globalAlpha;
    }

    virtual void beginOpacityLayer(float c)
    {
        INSTALL_PROFILE_TIMER("CanvasSkia::beginOpacityLayer");
        save();
        lastState()->m_layerOpacity = c;
        m_canvas->saveLayerAlpha(nullptr,
                                 ((uint8_t)(255.0f * CLAMP(c, 0.0, 1.0))));
    }

    virtual void endOpacityLayer()
    {
        INSTALL_PROFILE_TIMER("CanvasSkia::endOpacityLayer");
        m_canvas->restore();
        restore();
    }

    virtual void setFont(Font* font)
    {
        STARFISH_ASSERT(font != nullptr);
        lastState()->m_font = font;
    }

    virtual void resetTextDecorationData()
    {
        lastState()->m_textDecorationData.reset();
    }

    virtual void mergeTextDecorationData(ComputedStyle* style)
    {
        lastState()->m_textDecorationData.merge(style);
    }

    virtual TextDecorationData textDecorationData()
    {
        return lastState()->m_textDecorationData;
    }

    virtual void setTextDecorationData(TextDecorationData d)
    {
        lastState()->m_textDecorationData = d;
    }

    virtual void drawRect(const Unit::Rect& rt)
    {
        STARFISH_ASSERT(m_canvas);
        if (!lastState()->m_visible) {
            return;
        }

        drawRect({ rt.x(), rt.y() }, { rt.x() + rt.width(), rt.y() },
                 { rt.x() + rt.width(), rt.y() + rt.height() },
                 { rt.x(), rt.y() + rt.height() });
    }

    virtual void drawRect(const LayoutRect& rt)
    {
        STARFISH_ASSERT(m_canvas);
        if (!lastState()->m_visible) {
            return;
        }
        int xx = 0, yy = 0, ww = 0, hh = 0;
        LayoutUnit rx = rt.x();
        LayoutUnit ry = rt.y();

        xx = rx.floor();
        yy = ry.floor();
        ww = snapSizeToPixel(rt.width(), rx);
        hh = snapSizeToPixel(rt.height(), ry);
        drawRect({ xx, yy }, { xx + ww, yy }, { xx + ww, yy + hh },
                 { xx, yy + hh });
    }

    virtual void drawRectInner(float x, float y, float w, float h)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    virtual void strokeRect(const Unit::Rect& rt)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    virtual void strokeRect(const LayoutRect& rt)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    virtual void drawRect(LayoutLocation p1, LayoutLocation p2,
                          LayoutLocation p3, LayoutLocation p4)
    {
        if (!lastState()->m_visible) {
            return;
        }
        m_canvas->save();
        moveTo(p1.x(), p1.y());
        lineTo(p2.x(), p2.y());
        lineTo(p3.x(), p3.y());
        lineTo(p4.x(), p4.y());
        lineTo(p1.x(), p1.y());
        closePath();
        fill();
        m_canvas->restore();
    }

    virtual void punchHole(const Unit::Rect& rt)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        if (!lastState()->m_visible) {
            return;
        }
    }

    void drawGlyphsSkia(SkCanvas* canvas, LayoutRect rect, const StringView& sv,
                        LayoutUnit dx, LayoutUnit dy)
    {
        LayoutUnit xBias = 0;
        sk_sp<SkTypeface> lastFontFace = nullptr;
        sk_sp<SkTypeface> fontFace = nullptr;
        SkPaint paint;
        FontImplSkia* f = (FontImplSkia*)lastState()->m_font;
        int size = f->size();

        FontMetrics fontMetrics = f->metrics();

        canvas->save();
        canvas->translate(dx, fontMetrics.m_ascender + dy);

        size_t glyphCount = 0;

        LayoutUnit letterSpacing = f->letterSpacing();
        SkAutoTMalloc<SkGlyphID> glyphs(sv.bufferAccessData().length);
        SkAutoTMalloc<SkPoint> positions(sv.bufferAccessData().length);

        if (skiaBackendCanUseSimpleFontPath(f, sv)) {
            LayoutUnit letterSpacingValueSoFar;
            auto stringAccessData = sv.bufferAccessData();

            for (size_t i = 0; i < stringAccessData.length; i++) {
                std::pair<std::pair<FontFaceImplSkia*, size_t>,
                          std::pair<unsigned, LayoutUnit>>
                    g = skiaBackendInternalLoadGlyph(
                        f, stringAccessData.charAt(i));
                if (g.second.first) {
                    if (true) { // skip webfont enabled
                        if (f->seenUnresolvedWebFontIndex() != SIZE_MAX &&
                            f->seenUnresolvedWebFontIndex() <= g.first.second) {
                            xBias += g.second.second;
                            continue;
                        }
                    }
                    if (lastFontFace != g.first.first->skTypeface()) {
                        if (fontFace) {
                            canvas->drawPosText((glyphs).get(),
                                                glyphCount * sizeof(SkGlyphID),
                                                (positions).get(), paint);
                            glyphCount = 0;
                            fontFace = nullptr;
                        }
                        lastFontFace = g.first.first->skTypeface();

                        fontFace = lastFontFace;
                        paint = g.first.first->skPaint();
                        paint.setTextSize(size);
                        paint.setColor(SkColorSetARGB(
                            ((CanvasStateSkia*)lastState())->m_fillColor.a(),
                            ((CanvasStateSkia*)lastState())->m_fillColor.r(),
                            ((CanvasStateSkia*)lastState())->m_fillColor.g(),
                            ((CanvasStateSkia*)lastState())->m_fillColor.b()));
                    }

                    (glyphs)[glyphCount] = SkToU16(g.second.first);
                    (positions)[glyphCount] = SkPoint{
                        xBias.toFloat() + letterSpacingValueSoFar.toFloat(), 0.0
                    };
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
                    SkPath path;
                    SkPaint tempPaint;
                    tempPaint.setStrokeWidth(1);
                    tempPaint.setStyle(SkPaint::kStroke_Style);
                    tempPaint.setColor(SkColorSetARGB(
                        ((CanvasStateSkia*)lastState())->m_fillColor.a(),
                        ((CanvasStateSkia*)lastState())->m_fillColor.r(),
                        ((CanvasStateSkia*)lastState())->m_fillColor.g(),
                        ((CanvasStateSkia*)lastState())->m_fillColor.b()));
                    path.addRect(SkRect::MakeXYWH(
                        xBias, -fontMetrics.m_ascender, f->spaceWidth(),
                        fontMetrics.m_fontHeight));

                    canvas->drawPath(path, tempPaint);
                    xBias += f->spaceWidth() + letterSpacing;
                }
            }
        } else {
            auto runs = generateFontSkiaTextRuns(&sv, f);
            size_t glyphAllocCount = 0;
            for (size_t i = 0; i < runs.size(); i++) {
                FontSkiaTextRun& run = runs[i];
                glyphAllocCount += run.m_glyphs.size();
            }

            float xBias = 0;
            for (size_t i = 0; i < runs.size(); i++) {
                FontSkiaTextRun& run = runs[i];

                LayoutUnit letterSpacingValueSoFar;
                if (run.m_skTypeface == nullptr) {
                    if (/* skip webfont enabled*/ f
                            ->seenUnresolvedWebFontIndex() != SIZE_MAX) {
                    } else {
                        SkPath path;
                        SkPaint tempPaint;
                        tempPaint.setStrokeWidth(1);
                        tempPaint.setStyle(SkPaint::kStroke_Style);
                        tempPaint.setColor(SkColorSetARGB(
                            ((CanvasStateSkia*)lastState())->m_fillColor.a(),
                            ((CanvasStateSkia*)lastState())->m_fillColor.r(),
                            ((CanvasStateSkia*)lastState())->m_fillColor.g(),
                            ((CanvasStateSkia*)lastState())->m_fillColor.b()));

                        for (size_t j = 0; j < run.m_text.length(); j++) {
                            path.reset();
                            path.addRect(SkRect::MakeXYWH(
                                xBias + j * f->spaceWidth(),
                                -fontMetrics.m_ascender, f->spaceWidth(),
                                fontMetrics.m_fontHeight));
                            canvas->drawPath(path, tempPaint);
                        }
                    }
                } else {
                    if (/* skip webfont enabled*/ f
                                ->seenUnresolvedWebFontIndex() != SIZE_MAX &&
                        f->seenUnresolvedWebFontIndex() <= run.m_faceIndex) {
                    } else {
                        if (run.m_skTypeface != lastFontFace) {
                            if (lastFontFace) {
                                canvas->drawPosText((glyphs).get(),
                                                    glyphCount *
                                                        sizeof(SkGlyphID),
                                                    (positions).get(), paint);
                                glyphCount = 0;
                                fontFace = nullptr;
                            }
                            lastFontFace = run.m_skTypeface;
                            fontFace = lastFontFace;
                            paint = FontFaceImplSkia::skPaint(fontFace);
                            paint.setTextSize(size);
                            paint.setColor(
                                SkColorSetARGB(((CanvasStateSkia*)lastState())
                                                   ->m_fillColor.a(),
                                               ((CanvasStateSkia*)lastState())
                                                   ->m_fillColor.r(),
                                               ((CanvasStateSkia*)lastState())
                                                   ->m_fillColor.g(),
                                               ((CanvasStateSkia*)lastState())
                                                   ->m_fillColor.b()));
                        }

                        for (size_t j = 0; j < run.m_glyphs.size(); j++) {
                            (glyphs)[glyphCount] = SkToU16(run.m_glyphs[j]);
                            (positions)[glyphCount] = SkPoint{
                                run.m_glyphPositions[j].x().toFloat() + xBias +
                                    letterSpacingValueSoFar.toFloat(),
                                run.m_glyphPositions[j].y().toFloat()
                            };

                            letterSpacingValueSoFar += letterSpacing;
                            STARFISH_ASSERT(glyphCount < glyphAllocCount);
                            glyphCount++;
                        }
                    }
                }

                xBias += (run.m_runWidth + letterSpacingValueSoFar);
            }
        }
        if (glyphCount) {
            canvas->drawPosText((glyphs).get(), glyphCount * sizeof(SkGlyphID),
                                (positions).get(), paint);
        }
        canvas->restore();
    }

    virtual void drawText(LayoutUnit x, LayoutUnit y, LayoutUnit stringWidth,
                          const StringView& sv,
                          bool shouldSkipUnresolvedWebFont)
    {
        int size = lastState()->m_font->size();
        if (!lastState()->m_visible || size == 0 || sv.length() == 0) {
            return;
        }
        INSTALL_PROFILE_TIMER("CanvasSkia::drawText");

        LayoutSize sz(stringWidth, lastState()->m_font->metrics().m_fontHeight);
        LayoutRect rt(x, y, sz.width(), sz.height());

#ifdef STARFISH_ENABLE_TEST
        if (g_enablePixelTest) {
            // drawAhemBoxCairo(m_canvas, rt, sv, rt.x(), rt.y());
        } else {
            drawGlyphsSkia(m_canvas, rt, sv, rt.x(), rt.y());
            // drawTextDecorationCairo(m_canvas, rt, sv, rt.x(), rt.y());
        }
#else
        drawGlyphsSkia(m_canvas, rt, sv, rt.x(), rt.y());
//        drawTextDecorationCairo(m_canvas, rt, sv, rt.x(), rt.y());
#endif
    }

    virtual void drawStrokeText(LayoutUnit x, LayoutUnit y,
                                LayoutUnit stringWidth, const StringView& sv,
                                bool shouldSkipUnresolvedWebFont)
    {
    }

    static inline void setImageRenderingMode(
        SkPaint& paint, ImageRenderingValue imageRenderingMode)
    {
        if (imageRenderingMode == ImageRenderingAutoValue) {
            paint.setFilterQuality(SkFilterQuality::kNone_SkFilterQuality);
        } else if (imageRenderingMode == ImageRenderingPixelatedValue) {
            paint.setFilterQuality(SkFilterQuality::kNone_SkFilterQuality);
        } else if (imageRenderingMode == ImageRenderingCrispEdgesValue) {
            paint.setFilterQuality(SkFilterQuality::kLow_SkFilterQuality);
        }
    }

    virtual void drawImage(NativeImageData* data, const Unit::Rect& dst,
                           ImageRenderingValue imageRenderingMode)
    {
        if (!lastState()->m_visible) {
            return;
        }
        auto pixels = data->data();
        auto w = data->width();
        auto h = data->height();

        SkPaint paint;
        setImageRenderingMode(paint, imageRenderingMode);

        SkBitmap bitmap;
        bitmap.installPixels(SkImageInfo::MakeN32Premul(w, h), (void*)pixels,
                             data->stride());

        m_canvas->drawBitmapRect(
            bitmap, SkIRect::MakeWH(w, h),
            SkRect::MakeXYWH(dst.x(), dst.y(), dst.width(), dst.height()),
            &paint);
    }

    virtual void drawImage(NativeImageData* data, const Unit::Rect& src,
                           const Unit::Rect& dst,
                           const DrawImageInfo& borderinfo,
                           ImageRenderingValue imageRenderingMode)
    {
        if (!lastState()->m_visible) {
            return;
        }

        auto pixels = data->data();
        auto w = data->width();
        auto h = data->height();

        SkPaint paint;
        setImageRenderingMode(paint, imageRenderingMode);

        SkBitmap bitmap;
        bitmap.installPixels(SkImageInfo::MakeN32Premul(w, h), (void*)pixels,
                             data->stride());
        SkBitmap subset;
        bitmap.extractSubset(
            &subset,
            SkIRect::MakeXYWH(src.x(), src.y(), src.width(), src.height()));

        if (borderinfo.hRepeat == BorderImageRepeatValue::StretchValue &&
            borderinfo.vRepeat == BorderImageRepeatValue::StretchValue) {
            m_canvas->drawBitmapRect(
                subset, SkIRect::MakeWH(src.width(), src.height()),
                SkRect::MakeXYWH(dst.x(), dst.y(), dst.width(), dst.height()),
                &paint);

        } else {
            double xx = dst.x(), yy = dst.y(), ww = dst.width(),
                   hh = dst.height();
            double x = 0.0, y = 0.0, hScale = borderinfo.hScale,
                   vScale = borderinfo.vScale;
            double scaledWidth = src.width() / borderinfo.hScale;
            double scaledHeight = src.height() / borderinfo.vScale;

            if (borderinfo.hRepeat == BorderImageRepeatValue::RepeatValue) {
                x = (ww - scaledWidth) / 2;
            } else if (borderinfo.hRepeat ==
                       BorderImageRepeatValue::RoundValue) {
                hScale = std::max(1.0, round(ww / scaledWidth));
                hScale = (scaledWidth * hScale) / ww * borderinfo.hScale;
            }
            if (borderinfo.vRepeat == BorderImageRepeatValue::RepeatValue) {
                y = (hh - scaledHeight) / 2;
            } else if (borderinfo.vRepeat ==
                       BorderImageRepeatValue::RoundValue) {
                vScale = std::max(1.0, round(hh / scaledHeight));
                vScale = (scaledHeight * vScale) / hh * borderinfo.vScale;
            }
            SkMatrix matrix;
            matrix.setScaleTranslate(hScale, vScale, x, y);
            auto shader =
                SkShader::MakeBitmapShader(subset, SkShader::kRepeat_TileMode,
                                           SkShader::kRepeat_TileMode, &matrix);

            SkPaint paint;
            paint.setShader(shader);
            m_canvas->save();
            m_canvas->translate(xx, yy);
            auto rect = SkRect::MakeXYWH(0, 0, ww, hh);
            m_canvas->drawRect(rect, paint);
            m_canvas->restore();
        }
    }

    virtual void drawRepeatImage(
        NativeImageData* data, const Unit::Rect& dst, float imageWidth,
        float imageHeight, bool xRepeat, bool yRepeat,
        ImageRenderingValue imageRenderingMode =
            ImageRenderingValue::ImageRenderingAutoValue)
    {
        if (!lastState()->m_visible) {
            return;
        }
        INSTALL_PROFILE_TIMER("CanvasSkia::drawRepeatImage");
        auto pixels = data->data();
        auto dataW = data->width();
        auto dataH = data->height();
        auto x = dst.x();
        auto y = dst.y();

        if (xRepeat) {
            x = 0;
        }
        if (yRepeat) {
            y = 0;
        }

        SkBitmap bitmap;
        bitmap.installPixels(SkImageInfo::MakeN32Premul(dataW, dataH),
                             (void*)pixels, data->stride());
        SkMatrix matrix;
        matrix.setScaleTranslate(imageWidth / dataW, imageHeight / dataH,
                                 dst.x(), dst.y());

        auto shader =
            SkShader::MakeBitmapShader(bitmap, SkShader::kRepeat_TileMode,
                                       SkShader::kRepeat_TileMode, &matrix);

        SkPaint paint;
        paint.setShader(shader);
        auto rect = SkRect::MakeXYWH(x, y, dst.width(), dst.height());
        m_canvas->save();
        m_canvas->drawRect(rect, paint);
        m_canvas->restore();
    }

    virtual void drawLinearGradient(const Unit::Rect& dst,
                                    GradientDrawingInfo* info,
                                    NativeGradient* gradient)
    {
        if (!lastState()->m_visible) {
            return;
        }

        SkPoint points[] = { { info->x1, info->y1 }, { info->x2, info->y2 } };
        size_t colorCount = info->colorStops.size();
        SkColor colors[colorCount];
        SkScalar pos[colorCount];
        for (size_t i = 0; i < colorCount; i++) {
            Unit::Color clr = info->colorStops[i]->color();
            colors[i] = SkColorSetARGB(clr.a(), clr.r(), clr.g(), clr.b());
            pos[i] = info->colorStops[i]->offset().percent();
        }

        SkPaint p;
        // p.setAntiAlias(true); // Uncomment if performance is not an issue
        p.setShader(((NativeGradientSkia*)gradient)->shader());
        SkRect rect =
            SkRect::MakeXYWH(dst.x(), dst.y(), dst.width(), dst.height());
        m_canvas->drawRect(rect, p);
    }

    virtual void drawRadialGradient(const Unit::Rect& dst,
                                    GradientDrawingInfo* info,
                                    NativeGradient* gradient)
    {
        if (!lastState()->m_visible) {
            return;
        }

        m_canvas->save();

        size_t colorCount = info->colorStops.size();
        SkColor colors[colorCount];
        SkScalar pos[colorCount];
        for (size_t i = 0; i < colorCount; i++) {
            Unit::Color clr = info->colorStops[i]->color();
            colors[i] = SkColorSetARGB(clr.a(), clr.r(), clr.g(), clr.b());
            pos[i] = info->colorStops[i]->offset().percent();
        }

        if (info->secondRadius && (info->firstRadius > info->secondRadius)) {
            // width > height
            m_canvas->scale(1, (info->secondRadius / info->firstRadius));
        } else if (info->secondRadius &&
                   (info->firstRadius < info->secondRadius)) {
            // width < height
            m_canvas->scale((info->firstRadius / info->secondRadius), 1);
        }

        SkPaint p;
        // p.setAntiAlias(true); // Uncomment if performance is not an issue
        p.setShader(((NativeGradientSkia*)gradient)->shader());
        m_canvas->drawPaint(p);
        m_canvas->restore();
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

    virtual CanvasLineCap lineCap()
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        return CanvasLineCap::Butt;
    }

    virtual void setLineCap(CanvasLineCap lineCap)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    virtual CanvasLineJoin lineJoine()
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        return CanvasLineJoin::Miter;
    }

    virtual void setLineJoin(CanvasLineJoin lineJoin)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    virtual double miterLimit()
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        return 10.0f;
    }

    virtual void setMiterLimit(double limit)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    virtual double shadowOffsetX()
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        return 0;
    }

    virtual void setShadowOffsetX(double offset)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    virtual double shadowOffsetY()
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        return 0;
    }

    virtual void setShadowOffsetY(double offset)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    virtual double shadowBlur()
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        return 0;
    }

    virtual void setShadowBlur(double blur)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    virtual Unit::Color shadowColor() override
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        return Unit::Color();
    }

    virtual void setShadowColor(const Unit::Color& color) override
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    virtual bool imageSmoothingEnabled()
    {
        return lastState()->m_imageSmoothingEnabled;
    }

    virtual void setImageSmoothingEnabled(bool value)
    {
        lastState()->m_imageSmoothingEnabled = value;
    }

    virtual ImageSmoothingQuality imageSmoothingQuality()
    {
        return lastState()->m_imageSmoothingQuality;
    }

    virtual void setImageSmoothingQuality(ImageSmoothingQuality quality)
    {
        lastState()->m_imageSmoothingQuality = quality;
    }

    virtual void setVisible(bool visible)
    {
        lastState()->m_visible = visible;
    }

    virtual void setNonInvertableCTM(bool validation)
    {
        lastState()->m_hasNonInvertableCTM = validation;
    }

    virtual bool hasNonInvertableCTM()
    {
        return lastState()->m_hasNonInvertableCTM;
    }

    virtual void setPathTransformMatrix(const SkMatrix& matrix)
    {
        lastState()->m_pathTM = matrix;
    }

    virtual SkMatrix pathTransformMatrix()
    {
        return lastState()->m_pathTM;
    }

    virtual void setOriginalFontStr(String* fontStr)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    virtual void setCanvasWebFontState(size_t version)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    virtual size_t canvasWebFontState()
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        return 0;
    }

    virtual Font* font()
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        return lastState()->m_font;
    }

    virtual String* originalFontStr()
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        return nullptr;
    }

    virtual void setCanvasTextAlign(CanvasTextAlign textAlign)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    virtual CanvasTextAlign canvasTextAlign()
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        return CanvasTextAlign::Start;
    }

    virtual void setCanvasTextBaseline(CanvasTextBaseline textBaseline)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    virtual CanvasTextBaseline canvasTextBaseline()
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        return CanvasTextBaseline::Alphabetic;
    }

    virtual void setCanvasTextDirection(CanvasDirection textDirection)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    virtual CanvasDirection canvasTextDirection()
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        return CanvasDirection::Inherit;
    }

    virtual void beginPath()
    {
        m_path.reset();
    }

    virtual void closePath()
    {
        m_path.close();
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

    virtual void curveTo(float x1, float y1, float x2, float y2, float x3,
                         float y3)
    {
    }

    virtual void quadraticCurveTo(float x1, float y1, float x2, float y2)
    {
    }

    virtual void arc(double xc, double yc, double radius, double angle1,
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

    virtual void stroke()
    {
        if (!lastState()->m_visible) {
            closePath();
            return;
        }
        strokePreserve();
        m_path.reset();
    }

    virtual void strokePreserve()
    {
        if (!lastState()->m_visible) {
            return;
        }
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(((CanvasStateSkia*)lastState())->m_strokeWidth);
        paint.setColor(
            SkColorSetARGB(((CanvasStateSkia*)lastState())->m_fillColor.a(),
                           ((CanvasStateSkia*)lastState())->m_fillColor.r(),
                           ((CanvasStateSkia*)lastState())->m_fillColor.g(),
                           ((CanvasStateSkia*)lastState())->m_fillColor.b()));

        SkMatrix m = m_canvas->getTotalMatrix();
        m_canvas->resetMatrix();
        m_canvas->drawPath(m_path, paint);
        m_canvas->setMatrix(m);
    }

    virtual void fill()
    {
        if (!lastState()->m_visible) {
            closePath();
            return;
        }
        fillPreserve();
        m_path.reset();
    }

    virtual void strokePath(Path* path)
    {
        if (!lastState()->m_visible) {
            return;
        }
        (((PathSkia*)path)->skiaPath())->dump();
        m_path.addPath(*(((PathSkia*)path)->skiaPath()));
        stroke();
    }

    virtual void fillPath(Path* path)
    {
        if (!lastState()->m_visible) {
            return;
        }
        m_path.addPath(*(((PathSkia*)path)->skiaPath()));
        fill();
    }

    virtual void fillPreserve()
    {
        if (!lastState()->m_visible) {
            return;
        }

        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setStyle(SkPaint::kFill_Style);
        paint.setColor(
            SkColorSetARGB(((CanvasStateSkia*)lastState())->m_fillColor.a(),
                           ((CanvasStateSkia*)lastState())->m_fillColor.r(),
                           ((CanvasStateSkia*)lastState())->m_fillColor.g(),
                           ((CanvasStateSkia*)lastState())->m_fillColor.b()));

        SkMatrix m = m_canvas->getTotalMatrix();
        m_canvas->resetMatrix();
        m_canvas->drawPath(m_path, paint);
        m_canvas->setMatrix(m);
    }

    virtual void clipPath()
    {
        clipPathPreserve();
        m_path.reset();
    }

    virtual void clipPathPreserve()
    {
        SkMatrix m = m_canvas->getTotalMatrix();
        m_canvas->resetMatrix();
        m_canvas->clipPath(m_path, true);
        m_canvas->setMatrix(m);
    }

    virtual void setFillRule(bool shouldUseNonZeroFillRule)
    {
        if (shouldUseNonZeroFillRule) {
            m_path.setFillType(SkPath::kWinding_FillType);
        } else {
            m_path.setFillType(SkPath::kEvenOdd_FillType);
        }
    }

    virtual void setLineWidth(float width)
    {
        STARFISH_ASSERT(m_canvas);
        ((CanvasStateSkia*)lastState())->m_strokeWidth = width;
    }

    virtual void setDash(const std::vector<double>& dashes)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    virtual std::vector<double> dash()
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        return lastState()->m_dashes;
    }

    virtual double dashOffset()
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        return lastState()->m_dashOffset;
    }

    virtual void setDashOffset(double offset)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    // reset transform matrix & clip
    virtual void resetMatrixAndClip(bool needsApplyDPR)
    {
        m_canvas->resetMatrix();
        if (needsApplyDPR) {
            applyDevicePixelRatio(m_canvas);
        }
    }

    virtual void resetMatrix(bool needsApplyDPR)
    {
        m_canvas->resetMatrix();
        if (needsApplyDPR) {
            applyDevicePixelRatio(m_canvas);
        }
    }

    // reset transform clip
    virtual void resetClip()
    {
        // FIXME : skia doesn't have a way to reset clips only
        SkMatrix m = m_canvas->getTotalMatrix();
        m_canvas->resetMatrix();
        applyDevicePixelRatio(m_canvas);
        m_canvas->concat(m);
    }

    virtual bool canRejectPainting(const LayoutRect& rect)
    {
        return m_canvas->quickReject(
            SkRect::MakeXYWH(rect.x(), rect.y(), rect.width(), rect.height()));
    }

protected:
    SkCanvas* m_canvas;
    sk_sp<SkSurface> m_surface;
    SkPath m_path;

    bool m_shouldDestroySkia;
    bool m_shouldDestroySurface;
};

Canvas* Canvas::create(WebView* webView, CanvasSurface* data, CanvasFlag flag)
{
    STARFISH_ASSERT(webView != nullptr);
    STARFISH_ASSERT(data != nullptr);

    return new CanvasSkia(webView, data);
}

Canvas* Canvas::create(WebView* webView, uint8_t* data, size_t w, size_t h,
                       size_t stride)
{
    STARFISH_ASSERT(webView != nullptr);
    STARFISH_ASSERT(data != nullptr);

    return new CanvasSkia(webView, data, w, h, stride);
}

Canvas* Canvas::create(WebView* webView, NativeImageData* data)
{
    STARFISH_ASSERT(webView != nullptr);
    STARFISH_ASSERT(data != nullptr);

    return new CanvasSkia(webView, data);
}

NativeImageData* NativeImageData::attach(Canvas* canvas)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return nullptr;
}
} // namespace Starfish
#endif
