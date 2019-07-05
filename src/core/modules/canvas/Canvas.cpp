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
#include "core/style/Style.h"
#include "Canvas.h"
#include "CanvasFillStrokeSource.h"
#include "core/dom/canvas/ImageSmoothingQuality.h"
#include "Starfish.h"
#include "core/page/WebView.h"
#include "platform/window/PlatformWindow.h"
#include "core/style/Style.h"
#include "core/style/ComputedStyle.h"
#include "core/dom/canvas/CanvasDirection.h"
#include "core/dom/canvas/CanvasTextAlign.h"
#include "core/dom/canvas/CanvasTextBaseline.h"
#include "core/modules/canvas/CanvasShadowData.h"
#include "core/modules/canvas/image/NativeImageData.h"
#include "core/modules/canvas/ShadowBlur.h"
#include "core/style/UnitHelper.h"

namespace Starfish {

size_t CanvasSurface::g_totalAllocatedCanvasSurfaceSize = 0;
#ifndef STARFISH_CANVAS_SURFACE_TILE_SIZE
#define STARFISH_CANVAS_SURFACE_TILE_SIZE 256
#endif
size_t CanvasSurface::g_canvasSurfaceTileSize =
    STARFISH_CANVAS_SURFACE_TILE_SIZE;

#if !defined(PORT_COMPOSITOR_BACKEND_GL)
class CanvasSurfaceSimple : public CanvasSurface {
public:
    CanvasSurfaceSimple(PlatformWindow* wnd, size_t w, size_t h)
    {
        m_window = wnd;
        m_width = w;
        m_height = h;
        m_bufferStride = m_bufferWidth = m_width = -1;
        m_bufferHeight = m_height = -1;
        m_buffer = nullptr;

        attachNativeBuffer(w, h, CanvasSurface::PlainElement);
        GC_REGISTER_FINALIZER_NO_ORDER(this,
                                       [](void* obj, void* cd) {
                                           CanvasSurfaceSimple* s =
                                               (CanvasSurfaceSimple*)obj;
                                           s->detachNativeBuffer();
                                       },
                                       NULL, NULL, NULL);
    }

    virtual void detachNativeBuffer() override
    {
        if (m_buffer) {
            g_totalAllocatedCanvasSurfaceSize -=
                m_bufferWidth * m_bufferHeight * sizeof(uint32_t);
            free(m_buffer);
            m_buffer = nullptr;
            m_width = 0;
            m_height = 0;
            m_bufferStride = m_bufferWidth = m_width = 0;
            m_bufferHeight = m_height = 0;
        }
    }

    virtual bool attachNativeBuffer(size_t w, size_t h,
                                    CanvasSurfaceFlag flag) override
    {
        if (m_width != w || m_height != h) {
            detachNativeBuffer();
            m_width = w;
            m_height = h;

            float windowDevicePixelRatio =
                m_window->webView()->screenInfo().devicePixelRatio;

            m_bufferWidth =
                std::max((size_t)1, (size_t)(w * windowDevicePixelRatio));
            m_bufferHeight =
                std::max((size_t)1, (size_t)(h * windowDevicePixelRatio));

            m_bufferStride = m_bufferWidth * 4;
            m_buffer = (unsigned char*)calloc(
                1, m_bufferWidth * m_bufferHeight * sizeof(uint32_t));
            STARFISH_RELEASE_ASSERT(m_buffer);
            g_totalAllocatedCanvasSurfaceSize +=
                m_bufferWidth * m_bufferHeight * sizeof(uint32_t);
            return true;
        }
        return false;
    }

    virtual MappedNativeBuffer mapBuffer(size_t bufferX, size_t bufferY,
                                         size_t bufferWidth,
                                         size_t bufferHeight) override
    {
        CanvasSurface::MappedNativeBuffer b;
        b.m_bufferAddress = m_buffer;
        b.m_mappedBufferX = 0;
        b.m_mappedBufferY = 0;
        b.m_mappedBufferWidth = m_bufferWidth;
        b.m_mappedBufferHeight = m_bufferHeight;
        b.m_mappedBufferStride = m_bufferStride;

        return b;
    }

    virtual size_t width() override
    {
        return m_width;
    }

    virtual size_t height() override
    {
        return m_height;
    }

    virtual size_t bufferWidth() override
    {
        return m_bufferWidth;
    }

    virtual size_t bufferHeight() override
    {
        return m_bufferHeight;
    }

    virtual size_t bufferStride() override
    {
        return m_bufferStride;
    }

protected:
    PlatformWindow* m_window;
    unsigned char* m_buffer;
    size_t m_width;
    size_t m_height;
    size_t m_bufferWidth;
    size_t m_bufferHeight;
    size_t m_bufferStride;
};

CanvasSurface* CanvasSurface::create(PlatformWindow* wnd, size_t w, size_t h,
                                     CanvasSurfaceFlag flag)
{
    return new CanvasSurfaceSimple(wnd, w, h);
}
#endif

#if defined(PORT_WINDOW_BACKEND_GB)
class CanvasSurfaceCanvasTarget : public CanvasSurface {
public:
    CanvasSurfaceCanvasTarget(uint8_t* buffer, size_t w, size_t h,
                              size_t stride)
    {
        m_width = w;
        m_height = h;
        m_bufferStride = stride;
        m_imageWidth = m_bufferWidth = m_width = w;
        m_imageHeight = m_bufferHeight = m_height = h;
        m_pixelRatio = 1;
        m_buffer = buffer;
    }

    void detachNativeBuffer() override
    {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    bool attachNativeBuffer(size_t w, size_t h, CanvasSurfaceFlag flag) override
    {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return false;
    }

    virtual void resize(size_t w, size_t h)
    {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    virtual MappedNativeBuffer mapBuffer(size_t bufferX, size_t bufferY,
                                         size_t bufferWidth,
                                         size_t bufferHeight) override
    {
        CanvasSurface::MappedNativeBuffer b;
        b.m_bufferAddress = m_buffer;
        b.m_mappedBufferX = 0;
        b.m_mappedBufferY = 0;
        b.m_mappedBufferWidth = m_bufferWidth;
        b.m_mappedBufferHeight = m_bufferHeight;
        b.m_mappedBufferStride = m_bufferStride;

        return b;
    }

    size_t width() override
    {
        return m_width;
    }

    size_t height() override
    {
        return m_height;
    }

    size_t bufferWidth() override
    {
        return m_bufferWidth;
    }

    size_t bufferHeight() override
    {
        return m_bufferHeight;
    }

    virtual size_t imageWidth()
    {
        return m_imageWidth;
    }

    virtual size_t imageHeight()
    {
        return m_imageHeight;
    }

    virtual size_t pixelRatio()
    {
        return m_pixelRatio;
    }

    size_t bufferStride() override
    {
        return m_bufferStride;
    }

protected:
    unsigned char* m_buffer;
    size_t m_width;
    size_t m_height;
    size_t m_imageWidth;
    size_t m_imageHeight;
    size_t m_bufferWidth;
    size_t m_bufferHeight;
    size_t m_bufferStride;
    size_t m_pixelRatio;
};

CanvasSurface* CanvasSurface::createCanvasTarget(uint8_t* buffer, size_t w,
                                                 size_t h, size_t stride)
{
    return new CanvasSurfaceCanvasTarget(buffer, w, h, stride);
}

#endif

CanvasState::CanvasState()
    : m_fillSource(new CanvasFillStrokeSource(Unit::Color()))
    , m_strokeSource(new CanvasFillStrokeSource(Unit::Color()))
    , m_layerOpacity(1.0f)
    , m_font(nullptr)
    , m_textDecorationData()
    , m_pathTM(SkMatrix::I())
    , m_globalAlpha(1.0f)
    , m_compositeOperator(CanvasCompositeOperator::SourceOver)
    , m_blendMode(CanvasBlendMode::Normal)
    , m_dashOffset(0.0f)
    , m_dashes()
    , m_canvasTextAlign(CanvasTextAlign::Start)
    , m_canvasTextBaseline(CanvasTextBaseline::Alphabetic)
    , m_canvasDirection(CanvasDirection::Inherit)
    , m_canvasFontOrginalStr(nullptr)
    , m_imageSmoothingEnabled(true)
    , m_imageSmoothingQuality(ImageSmoothingQuality::Low)
    , m_canvasFontState(0)
    , m_visible(true)
    , m_hasNonInvertableCTM(false)
    , m_shadowData(new CanvasShadowData())
{
}

void Canvas::save()
{
    CanvasState* state = nullptr;
    if (m_stateMemoryPool.size() != 0) {
        state = m_stateMemoryPool.back();
        m_stateMemoryPool.pop_back();
    } else {
        state = new CanvasState();
    }

    if (m_state.size() != 0) {
        auto lastState = m_state.back();
        state->m_fillSource = lastState->m_fillSource;
        state->m_strokeSource = lastState->m_strokeSource;
        state->m_layerOpacity = lastState->m_layerOpacity;
        state->m_font = lastState->m_font;
        state->m_visible = lastState->m_visible;
        state->m_textDecorationData = lastState->m_textDecorationData;
        state->m_hasNonInvertableCTM = lastState->m_hasNonInvertableCTM;
        state->m_pathTM = lastState->m_pathTM;
        state->m_globalAlpha = lastState->m_globalAlpha;
        state->m_compositeOperator = lastState->m_compositeOperator;
        state->m_blendMode = lastState->m_blendMode;
        state->m_dashOffset = lastState->m_dashOffset;
        state->m_dashes = lastState->m_dashes;
        state->m_canvasTextAlign = lastState->m_canvasTextAlign;
        state->m_canvasTextBaseline = lastState->m_canvasTextBaseline;
        state->m_canvasDirection = lastState->m_canvasDirection;
        state->m_canvasFontOrginalStr = lastState->m_canvasFontOrginalStr;
        state->m_imageSmoothingEnabled = lastState->m_imageSmoothingEnabled;
        state->m_imageSmoothingQuality = lastState->m_imageSmoothingQuality;
        state->m_canvasFontState = lastState->m_canvasFontState;
        state->m_shadowData = lastState->m_shadowData;
    }
    m_state.push_back(state);
}

void Canvas::restore()
{
    auto toStore = m_state.back();
    memset(toStore, 0, sizeof(CanvasState));
    m_stateMemoryPool.push_back(toStore);
    m_state.erase(m_state.end() - 1);
    m_shouldApplyCanvasFillStrokeSource = true;

    while (m_state.size() * 2 < m_stateMemoryPool.size()) {
        m_stateMemoryPool.pop_back();
    }
}

void Canvas::drawRectShadow(float x, float y, float width, float height)
{
    int xx = x, yy = y, ww = width, hh = height;
    Unit::Rect shadowRect(0, 0, ww, hh);
    CanvasShadowData* shadow = lastState()->m_shadowData;
    float radius = shadow->radius();
    float shadowOffsetX = shadow->offsetX();
    float shadowOffsetY = shadow->offsetY();
    float radiusOffset = 0.0f;

    if (radius == 0) {
        save();
        setFillColor(shadow->color());
        drawRectInner(shadowOffsetX + xx, shadowOffsetY + yy,
                      shadowRect.width(), shadowRect.height());
        restore();
    } else {
        radiusOffset = std::min(ShadowBlur::RADIUS_LIMIT, radius);
        radiusOffset *= 2;
    }

    float shortSide = std::min(shadowRect.width(), shadowRect.height());
    bool canUseFastPath = (radius < shortSide / 2) && radius > 0;

    if (canUseFastPath) {
        save();
        setNeedsNoneAntialias();
        size_t bufImageSize = (size_t)ceil(radiusOffset);
        NativeImageData* nativeImage =
            NativeImageData::create(bufImageSize, bufImageSize);
        Canvas* cv = Canvas::create(m_webView, nativeImage);
        cv->unsetDevicePixelRatio();
        auto shadowColor = shadow->color();
        cv->setFillColor(shadowColor);
        cv->clearColor(Unit::Color(0, 0, 0, 0));

        cv->translate(ceil(radiusOffset / 2), ceil(radiusOffset / 2));
        cv->drawRect(shadowRect);
        ShadowBlur sb(nativeImage->data(), nativeImage->width(),
                      nativeImage->height(), nativeImage->stride());
        sb.process(radius / 2);
        delete cv;

        float offset = ceil(radiusOffset / 2);
        Unit::Rect imageRect(-offset + shadowOffsetX + xx,
                             -offset + shadowOffsetY + yy,
                             ceil(shadowRect.width() + radiusOffset),
                             ceil(shadowRect.height() + radiusOffset));

        float pieceSize = bufImageSize;

        // center

        // pick color from blurred buffer
        uint8_t* buf = nativeImage->data();
        size_t edgeHeight =
            (nativeImage->height() > 0 ? nativeImage->height() - 1 : 0);
        size_t edgeWidth =
            (nativeImage->width() > 0 ? nativeImage->width() - 1 : 0);
        size_t base = edgeHeight * nativeImage->stride() + edgeWidth * 4;
#ifdef PORT_PIXEL_ORDER_RGBA
        unsigned char r = buf[base];
        unsigned char g = buf[base + 1];
        unsigned char b = buf[base + 2];
        unsigned char a = buf[base + 3];
#else
        unsigned char b = buf[base];
        unsigned char g = buf[base + 1];
        unsigned char r = buf[base + 2];
        unsigned char a = buf[base + 3];
#endif

        setFillColor(Unit::Color(r, g, b, a));
        drawRect(Unit::Rect(imageRect.x() + pieceSize,
                            imageRect.y() + pieceSize,
                            imageRect.width() - pieceSize * 2,
                            imageRect.height() - pieceSize * 2));

        // top-left
        Unit::Rect src;
        Unit::Rect dst;
        src = Unit::Rect(0, 0, pieceSize, pieceSize);
        dst = Unit::Rect(imageRect.x(), imageRect.y(), pieceSize, pieceSize);
        DrawImageInfo drawImageInfo = { 1.0, 1.0,
                                        BorderImageRepeatValue::StretchValue,
                                        BorderImageRepeatValue::StretchValue };
        drawImage(nativeImage, src, dst, drawImageInfo);

        // top-left -> top-right
        src = Unit::Rect(pieceSize - 1, 0, 1, pieceSize);
        dst = Unit::Rect(imageRect.x() + pieceSize, imageRect.y(),
                         imageRect.width() - pieceSize * 2, pieceSize);
        drawImage(nativeImage, src, dst, drawImageInfo);

        // top-right
        src = Unit::Rect(0, 0, pieceSize, pieceSize);
        dst = Unit::Rect(imageRect.maxX() - pieceSize, imageRect.y(), pieceSize,
                         pieceSize);
        save();
        translate(dst.x(), dst.y());
        dst.setX(0);
        dst.setY(0);
        translate(dst.width() / 2.f, dst.height() / 2.f);
        rotate(UnitHelper::convertFromDegToRad(90));
        translate(-dst.width() / 2.f, -dst.height() / 2.f);
        drawImage(nativeImage, src, dst, drawImageInfo);
        restore();

        // top-right -> bottom-right
        src = Unit::Rect(0, pieceSize - 1, pieceSize, 1);
        dst =
            Unit::Rect(imageRect.maxX() - pieceSize, imageRect.y() + pieceSize,
                       pieceSize, imageRect.height() - pieceSize * 2);
        save();
        translate(dst.x(), dst.y());
        dst.setX(0);
        dst.setY(0);
        translate(dst.width() / 2.f, dst.height() / 2.f);
        rotate(UnitHelper::convertFromDegToRad(180));
        translate(-dst.width() / 2.f, -dst.height() / 2.f);
        drawImage(nativeImage, src, dst, drawImageInfo);
        restore();

        // bottom-right
        src = Unit::Rect(0, 0, pieceSize, pieceSize);
        dst = Unit::Rect(imageRect.maxX() - pieceSize,
                         imageRect.maxY() - pieceSize, pieceSize, pieceSize);
        save();
        translate(dst.x(), dst.y());
        dst.setX(0);
        dst.setY(0);
        translate(dst.width() / 2.f, dst.height() / 2.f);
        rotate(UnitHelper::convertFromDegToRad(180));
        translate(-dst.width() / 2.f, -dst.height() / 2.f);
        drawImage(nativeImage, src, dst, drawImageInfo);
        restore();

        // bottom-right -> bottom-left
        src = Unit::Rect(pieceSize - 1, 0, 1, pieceSize);
        dst =
            Unit::Rect(imageRect.x() + pieceSize, imageRect.maxY() - pieceSize,
                       imageRect.width() - pieceSize * 2, pieceSize);
        save();
        translate(dst.x(), dst.y());
        dst.setX(0);
        dst.setY(0);
        translate(dst.width() / 2.f, dst.height() / 2.f);
        rotate(UnitHelper::convertFromDegToRad(180));
        translate(-dst.width() / 2.f, -dst.height() / 2.f);
        drawImage(nativeImage, src, dst, drawImageInfo);
        restore();

        // bottom-left
        src = Unit::Rect(0, 0, pieceSize, pieceSize);
        dst = Unit::Rect(imageRect.x(), imageRect.maxY() - pieceSize, pieceSize,
                         pieceSize);
        save();
        translate(dst.x(), dst.y());
        dst.setX(0);
        dst.setY(0);
        translate(dst.width() / 2.f, dst.height() / 2.f);
        rotate(UnitHelper::convertFromDegToRad(270));
        translate(-dst.width() / 2.f, -dst.height() / 2.f);
        drawImage(nativeImage, src, dst, drawImageInfo);
        restore();

        // bottom-left -> top-left
        src = Unit::Rect(0, pieceSize - 1, pieceSize, 1);
        dst = Unit::Rect(imageRect.x(), imageRect.y() + pieceSize, pieceSize,
                         imageRect.height() - pieceSize * 2);
        drawImage(nativeImage, src, dst, drawImageInfo);
        restore();

        delete nativeImage;
    } else {
        save();
        NativeImageData* nativeImage =
            NativeImageData::create(ceil(shadowRect.width() + radiusOffset),
                                    ceil(shadowRect.height() + radiusOffset));
        Canvas* cv = Canvas::create(m_webView, nativeImage);
        cv->unsetDevicePixelRatio();
        cv->clearColor(Unit::Color(0, 0, 0, 0));
        cv->setFillColor(shadow->color());

        cv->translate(ceil(radiusOffset / 2), ceil(radiusOffset / 2));
        cv->drawRect(shadowRect);

        ShadowBlur sb(nativeImage->data(), nativeImage->width(),
                      nativeImage->height(), nativeImage->stride());
        sb.process(radius / 2);
        delete cv;

        float offset = ceil(radiusOffset / 2);
        Unit::Rect imageRect(-offset + shadowOffsetX + xx,
                             -offset + shadowOffsetY + yy,
                             ceil(shadowRect.width() + radiusOffset),
                             ceil(shadowRect.height() + radiusOffset));
        drawImage(nativeImage, imageRect);
        restore();

        delete nativeImage;
    }
}
}
