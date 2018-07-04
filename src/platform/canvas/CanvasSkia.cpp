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

#include "StarFishConfig.h"

#if defined(PORT_CANVAS_BACKEND_SKIA)
#include "StarFish.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/font/Font.h"
#include "platform/canvas/font/FontImplSkia.h"
#include "core/modules/canvas/image/NativeImageData.h"
#include "core/style/UnitHelper.h"

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

#define CLAMP(value, min, max) \
    (((value) > (max)) ? (max) : (((value) < (min)) ? (min) : (value)))

namespace StarFish {

class CanvasStateSkia : public CanvasState {
public:
    CanvasStateSkia()
        : CanvasState()
    {
    }
};

class CanvasSkia : public Canvas {
    void initFromBuffer(void* buffer, int width, int height, int stride)
    {
        m_width = width;
        m_height = height;

        SkImageInfo info = SkImageInfo::MakeN32Premul(m_width, m_height);
        m_surface = SkSurface::MakeRasterDirect(info, buffer, stride);
        m_canvas = m_surface->getCanvas();
        applyDevicePixelRatio(m_canvas);
    }

    void initFromNativeImageData(NativeImageData* data)
    {
        m_width = data->width();
        m_height = data->height();

        SkImageInfo info = SkImageInfo::MakeN32Premul(m_width, m_height);
        m_surface =
            SkSurface::MakeRasterDirect(info, data->data(), data->stride());
        m_canvas = m_surface->getCanvas();
        applyDevicePixelRatio(m_canvas);
    }

    void applyDevicePixelRatio(SkCanvas* canvas)
    {
        canvas->scale(m_starfish->screenInfo().devicePixelRatio,
                      m_starfish->screenInfo().devicePixelRatio);
    }

public:
    CanvasSkia(StarFish* starfish, void* buffer, int width, int height,
               int stride)
    {
        m_shouldDestroySkia = true;
        m_shouldDestroySurface = true;
        m_starfish = starfish;
        m_canvas = nullptr;
        m_surface = nullptr;
        m_width = width;
        m_height = height;

        SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
        m_surface = SkSurface::MakeRasterDirect(info, buffer, stride);
        m_canvas = m_surface->getCanvas();
        applyDevicePixelRatio(m_canvas);
        save();
    }

    CanvasSkia(StarFish* starfish, void* data)
    {
        struct dummy {
            SkCanvas* canvas;
            sk_sp<SkSurface> surface;
            int w;
            int h;
        };
        dummy* d = (dummy*)data;
        m_starfish = starfish;
        m_canvas = d->canvas;
        m_surface = d->surface;
        m_width = d->w;
        m_height = d->h;
        m_shouldDestroySkia = false;
        m_shouldDestroySurface = false;
        save();
    }

    CanvasSkia(StarFish* starfish, CanvasSurface* data)
    {
        m_starfish = starfish;
        m_canvas = nullptr;
        m_surface = nullptr;
        m_shouldDestroySkia = true;
        m_shouldDestroySurface = true;

        initFromBuffer(data->data(), data->bufferWidth(), data->bufferHeight(),
                       data->bufferStride());

        save();
    }

    CanvasSkia(StarFish* starfish, NativeImageData* data)
    {
        m_shouldDestroySkia = true;
        m_shouldDestroySurface = false;
        m_starfish = starfish;
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
        INSTALL_PROFILE_TIMER(m_starfish, "CanvasSkia::clear");
        m_canvas->save();
        if (clr.a() == 0) {
            m_canvas->clear(SK_ColorTRANSPARENT);
        } else {
            m_canvas->clear(SkColorSetARGB(clr.a(), clr.r(), clr.g(), clr.b()));
        }
        m_canvas->restore();
    }

    // state
    virtual void save() // push state on state stack
    {
        CanvasStateSkia state;
        if (m_state.size()) {
            auto& lastState = m_state.back();
            state.m_color = lastState.m_color;
            state.m_opacity = lastState.m_opacity;
            state.m_font = lastState.m_font;
            state.m_visible = lastState.m_visible;
            state.m_textDecorationData = lastState.m_textDecorationData;
        }
        m_state.push_back(state);
        m_canvas->save();
    }

    virtual void restore() // pop state stack and restore state
    {
        m_state.erase(m_state.end() - 1);
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
        m_canvas->rotate(angle);
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

    virtual void clip(const Unit::Rect& rt)
    {
        m_canvas->clipRect(
            SkRect::MakeXYWH(rt.x(), rt.y(), rt.width(), rt.height()));
    }

    virtual void setColor(const Unit::Color& clr)
    {
        STARFISH_ASSERT(m_canvas);
        lastState().m_color = clr;
        m_paint.setColor(SkColorSetARGB(clr.a(), clr.r(), clr.g(), clr.b()));
    }

    virtual void beginOpacityLayer(float c)
    {
        INSTALL_PROFILE_TIMER(m_starfish, "CanvasSkia::beginOpacityLayer");
        save();
        lastState().m_opacity = c;
        m_canvas->saveLayerAlpha(nullptr,
                                 ((uint8_t)(255.0f * CLAMP(c, 0.0, 1.0))));
    }

    virtual void endOpacityLayer()
    {
        INSTALL_PROFILE_TIMER(m_starfish, "CanvasSkia::endOpacityLayer");
        m_canvas->restore();
        restore();
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

    virtual void drawRect(const Unit::Rect& rt)
    {
        STARFISH_ASSERT(m_canvas);
        if (!lastState().m_visible) {
            return;
        }
        m_canvas->drawRect(
            SkRect::MakeXYWH(rt.x(), rt.y(), rt.width(), rt.height()), m_paint);
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
        m_canvas->drawRect(SkRect::MakeXYWH(xx, yy, ww, hh), m_paint);
    }

    virtual void drawRect(LayoutLocation p1, LayoutLocation p2,
                          LayoutLocation p3, LayoutLocation p4)
    {
        if (!lastState().m_visible) {
            return;
        }
        m_canvas->save();
        beginPath();
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
        if (!lastState().m_visible) {
            return;
        }
    }

    void drawGlyphsCairo(SkCanvas* canvas, LayoutRect rect,
                         const StringView& sv, LayoutUnit dx, LayoutUnit dy)
    {
        LayoutUnit xBias = 0;
        FT_UInt glyph_index = 0;
        sk_sp<SkTypeface> lastFontFace = nullptr;
        sk_sp<SkTypeface> fontFace = nullptr;
        SkPaint* paint = nullptr;
        FontImplSkia* f = (FontImplSkia*)lastState().m_font;
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
                    if (lastFontFace != g.first.first->skTypeFace()) {
                        if (fontFace) {
                            canvas->drawPosText((glyphs).get(),
                                                glyphCount * sizeof(SkGlyphID),
                                                (positions).get(), *paint);
                            glyphCount = 0;
                            fontFace = nullptr;
                        }
                        lastFontFace = g.first.first->skTypeFace();

                        fontFace = lastFontFace;
                        paint = g.first.first->skPaint();
                        paint->setTextSize(size);
                        paint->setColor(SkColorSetARGB(
                            lastState().m_color.a(), lastState().m_color.r(),
                            lastState().m_color.g(), lastState().m_color.b()));
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
                        lastState().m_color.a(), lastState().m_color.r(),
                        lastState().m_color.g(), lastState().m_color.b()));
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
                if (run.m_skTypeFace == nullptr) {
                    if (/* skip webfont enabled*/ f
                            ->seenUnresolvedWebFontIndex() != SIZE_MAX) {
                    } else {
                        SkPath path;
                        SkPaint tempPaint;
                        tempPaint.setStrokeWidth(1);
                        tempPaint.setStyle(SkPaint::kStroke_Style);
                        tempPaint.setColor(SkColorSetARGB(
                            lastState().m_color.a(), lastState().m_color.r(),
                            lastState().m_color.g(), lastState().m_color.b()));

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
                        if (run.m_skTypeFace != lastFontFace) {
                            if (lastFontFace) {
                                canvas->drawPosText((glyphs).get(),
                                                    glyphCount *
                                                        sizeof(SkGlyphID),
                                                    (positions).get(), *paint);
                                glyphCount = 0;
                                fontFace = nullptr;
                            }
                            lastFontFace = run.m_skTypeFace;
                            fontFace = lastFontFace;
                            paint = run.m_skPaint;
                            paint->setTextSize(size);
                            paint->setColor(
                                SkColorSetARGB(lastState().m_color.a(),
                                               lastState().m_color.r(),
                                               lastState().m_color.g(),
                                               lastState().m_color.b()));
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
                                (positions).get(), *paint);
        }
        canvas->restore();
    }

    virtual void drawText(LayoutUnit x, LayoutUnit y, LayoutUnit stringWidth,
                          const StringView& sv)
    {
        int size = lastState().m_font->size();
        if (!lastState().m_visible || size == 0 || sv.length() == 0) {
            return;
        }
        INSTALL_PROFILE_TIMER(m_starfish, "CanvasSkia::drawText");

        LayoutSize sz(stringWidth, lastState().m_font->metrics().m_fontHeight);
        LayoutRect rt(x, y, sz.width(), sz.height());

#ifdef STARFISH_ENABLE_TEST
        if (g_enablePixelTest) {
            // drawAhemBoxCairo(m_canvas, rt, sv, rt.x(), rt.y());
        } else {
            drawGlyphsCairo(m_canvas, rt, sv, rt.x(), rt.y());
            // drawTextDecorationCairo(m_canvas, rt, sv, rt.x(), rt.y());
        }
#else
        drawGlyphsCairo(m_canvas, rt, sv, rt.x(), rt.y());
//        drawTextDecorationCairo(m_canvas, rt, sv, rt.x(), rt.y());
#endif
    }

    virtual void drawImage(NativeImageData* data, const Unit::Rect& dst,
                           ImageRenderingValue imageRenderingMode)
    {
        if (!lastState().m_visible) {
            return;
        }
        auto pixels = data->data();
        auto w = data->width();
        auto h = data->height();

        SkBitmap bitmap;
        bitmap.installPixels(SkImageInfo::MakeN32Premul(w, h), (void*)pixels,
                             data->stride());

        m_canvas->drawBitmapRect(
            bitmap, SkIRect::MakeWH(w, h),
            SkRect::MakeXYWH(dst.x(), dst.y(), dst.width(), dst.height()),
            &m_paint);
    }

    virtual void drawImage(CanvasSurface* data, const Unit::Rect& dst,
                           ImageRenderingValue imageRenderingMode)
    {
        if (!lastState().m_visible) {
            return;
        }

        auto pixels = data->data();
        auto w = data->imageWidth();
        auto h = data->imageHeight();

        SkBitmap bitmap;
        bitmap.installPixels(SkImageInfo::MakeN32Premul(w, h), (void*)pixels,
                             data->bufferStride());
        m_canvas->drawBitmapRect(
            bitmap, SkIRect::MakeWH(w, h),
            SkRect::MakeXYWH(dst.x(), dst.y(), dst.width(), dst.height()),
            &m_paint);
    }

    virtual void drawImage(NativeImageData* data, const Unit::Rect& src,
                           const Unit::Rect& dst,
                           const DrawImageInfo& borderinfo,
                           ImageRenderingValue imageRenderingMode)
    {
        if (!lastState().m_visible) {
            return;
        }

        auto pixels = data->data();
        auto w = data->width();
        auto h = data->height();

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
                &m_paint);

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

    virtual void drawBorderImage(NativeImageData* data, const Unit::Rect& dst,
                                 size_t l, size_t t, size_t r, size_t b,
                                 double scale, bool fill)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    virtual void drawRepeatImage(
        NativeImageData* data, const Unit::Rect& dst, float imageWidth,
        float imageHeight, bool xRepeat, bool yRepeat,
        ImageRenderingValue imageRenderingMode =
            ImageRenderingValue::ImageRenderingAutoValue)
    {
        if (!lastState().m_visible) {
            return;
        }
        INSTALL_PROFILE_TIMER(m_starfish, "CanvasSkia::drawRepeatImage");
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
                                    GradientDrawingInfo* info)
    {
    }

    virtual void drawRadialGradient(const Unit::Rect& dst,
                                    GradientDrawingInfo* info)
    {
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

    virtual void setVisible(bool visible)
    {
        lastState().m_visible = visible;
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
        if (!lastState().m_visible) {
            closePath();
            return;
        }
        strokePreserve();
        m_path.reset();
    }

    virtual void strokePreserve()
    {
        if (!lastState().m_visible) {
            return;
        }
        m_paint.setStyle(SkPaint::kStroke_Style);
        SkMatrix m = m_canvas->getTotalMatrix();
        m_canvas->resetMatrix();
        m_canvas->drawPath(m_path, m_paint);
        m_canvas->setMatrix(m);
    }

    virtual void fill()
    {
        if (!lastState().m_visible) {
            closePath();
            return;
        }
        fillPreserve();
        m_path.reset();
    }

    virtual void fillPreserve()
    {
        if (!lastState().m_visible) {
            return;
        }
        m_paint.setStyle(SkPaint::kFill_Style);
        SkMatrix m = m_canvas->getTotalMatrix();
        m_canvas->resetMatrix();
        m_canvas->drawPath(m_path, m_paint);
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
        m_canvas->clipPath(m_path);
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

    virtual void setStrokeWidth(float width)
    {
        m_paint.setStrokeWidth(width);
    }

    virtual void setDash(double* dashes, int dashCnt, double offset)
    {
    }

    virtual void* unwrap()
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        return NULL;
    }

    CanvasStateSkia& lastState()
    {
        STARFISH_ASSERT(m_state.size());
        return m_state[m_state.size() - 1];
    }
    // reset transform matrix & clip
    virtual void resetMatrixAndClip()
    {
        m_canvas->resetMatrix();
        applyDevicePixelRatio(m_canvas);
        m_canvas->clipRect(SkRect::MakeXYWH(0, 0, m_width, m_height));
    }

    // reset transform clip
    virtual void resetClip()
    {
        // FIXME : skia doesn't have a way to reset clips only
        SkMatrix m = m_canvas->getTotalMatrix();
        m_canvas->resetMatrix();
        applyDevicePixelRatio(m_canvas);
        m_canvas->clipRect(SkRect::MakeXYWH(0, 0, m_width, m_height));
        m_canvas->concat(m);
    }

    virtual bool canRejectPainting(const LayoutRect& rect)
    {
        return m_canvas->quickReject(
            SkRect::MakeXYWH(rect.x(), rect.y(), rect.width(), rect.height()));
    }

protected:
    StarFish* m_starfish;
    std::vector<CanvasStateSkia> m_state;
    SkCanvas* m_canvas;
    sk_sp<SkSurface> m_surface;
    SkPaint m_paint;
    SkPath m_path;

    unsigned m_width;
    unsigned m_height;

    bool m_shouldDestroySkia;
    bool m_shouldDestroySurface;
};

Canvas* Canvas::createDirect(StarFish* starfish, void* data)
{
    return new CanvasSkia(starfish, data);
}

Canvas* Canvas::create(StarFish* starfish, CanvasSurface* data)
{
    return new CanvasSkia(starfish, data);
}

Canvas* Canvas::createGenericCanvas(StarFish* starfish, void* data, size_t w,
                                    size_t h)
{
    return new CanvasSkia(starfish, data, w, h, w * 4);
}

Canvas* Canvas::createGenericCanvas(StarFish* starfish, NativeImageData* data)
{
    return new CanvasSkia(starfish, data);
}
}
#endif
