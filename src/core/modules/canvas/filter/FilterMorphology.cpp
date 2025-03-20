/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2010 Igalia, S.L.
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 * Copyright (C) 2015-2016 Apple, Inc. All rights reserved.
 * Copyright (c) 2025-present Samsung Electronics Co., Ltd
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

#include "core/dom/svg/SVGFEMorphologyElement.h"
#include "core/modules/canvas/filter/Filter.h"
#include "core/modules/canvas/filter/FilterMorphology.h"
#include "core/page/WebView.h"
#include "core/layout/svg/FrameSVGBox.h"
#include "core/layout/svg/FrameSVGSVGBox.h"

namespace Starfish {

static inline std::array<uint8_t, 4> perComponentMax(
    const std::array<uint8_t, 4>& a, const std::array<uint8_t, 4>& b)
{
    std::array<uint8_t, 4> result;
    result[0] = std::max(a[0], b[0]);
    result[1] = std::max(a[1], b[1]);
    result[2] = std::max(a[2], b[2]);
    result[3] = std::max(a[3], b[3]);
    return result;
}

static inline std::array<uint8_t, 4> perComponentMin(
    const std::array<uint8_t, 4>& a, const std::array<uint8_t, 4>& b)
{
    std::array<uint8_t, 4> result;
    result[0] = std::min(a[0], b[0]);
    result[1] = std::min(a[1], b[1]);
    result[2] = std::min(a[2], b[2]);
    result[3] = std::min(a[3], b[3]);
    return result;
}

static inline std::array<uint8_t, 4> minOrMax(
    const std::array<uint8_t, 4>& a, const std::array<uint8_t, 4>& b,
    SVGFEMorphologyElement::MorphologyOperator type)
{
    if (type == SVGFEMorphologyElement::MorphologyOperator::
                    SVG_MORPHOLOGY_OPERATOR_ERODE)
        return perComponentMin(a, b);
    return perComponentMax(a, b);
}

static inline std::array<uint8_t, 4> makeColorComponentsfromPixelValue(
    const uint8_t* srcData, int x, int y, int stride)
{
    uint8_t* p = const_cast<uint8_t*>(srcData);
    p += stride * y + x * 4;
    std::array<uint8_t, 4> result;
    result[0] = p[STARFISH_PIXEL_R_INDEX];
    result[1] = p[STARFISH_PIXEL_G_INDEX];
    result[2] = p[STARFISH_PIXEL_B_INDEX];
    result[3] = p[STARFISH_PIXEL_A_INDEX];
    return result;
}

static inline std::array<uint8_t, 4> columnExtremum(
    const uint8_t* srcData, int x, int yStart, int yEnd, int stride,
    SVGFEMorphologyElement::MorphologyOperator type)
{
    auto extremum =
        makeColorComponentsfromPixelValue(srcData, x, yStart, stride);

    for (int y = yStart + 1; y < yEnd; ++y) {
        auto pixel = makeColorComponentsfromPixelValue(srcData, x, y, stride);
        extremum = minOrMax(extremum, pixel, type);
    }
    return extremum;
}

static inline std::array<uint8_t, 4> kernelExtremum(
    std::vector<std::array<uint8_t, 4>>& kernel,
    SVGFEMorphologyElement::MorphologyOperator type)
{
    auto extremum = kernel[0];
    for (size_t i = 1; i < kernel.size(); ++i)
        extremum = minOrMax(extremum, kernel[i], type);

    return extremum;
}

static inline void makePixelValueFromColorComponents(
    const uint8_t* dstData, int x, int y, int stride,
    std::array<uint8_t, 4> components)
{
    uint8_t* pixel = const_cast<uint8_t*>(dstData);
    pixel += y * stride + x * 4;
    pixel[STARFISH_PIXEL_R_INDEX] = components[0];
    pixel[STARFISH_PIXEL_G_INDEX] = components[1];
    pixel[STARFISH_PIXEL_B_INDEX] = components[2];
    pixel[STARFISH_PIXEL_A_INDEX] = components[3];
}

static inline void applyMorphology(
    const uint8_t* srcPixelBuffer, const uint8_t* dstPixelBuffer, int startY,
    int endY, const int radiusX, const int radiusY, const int width,
    const int height, const int stride,
    SVGFEMorphologyElement::MorphologyOperator type)
{
    STARFISH_ASSERT(endY > startY);

    STARFISH_ASSERT(radiusX <= width || radiusY <= height);
    STARFISH_ASSERT(startY >= 0 && endY <= height && startY < endY);

    std::vector<std::array<uint8_t, 4>> extrema;

    for (int y = startY; y < endY; ++y) {
        int yRadiusStart = std::max(0, y - radiusY);
        int yRadiusEnd = std::min(height, y + radiusY + 1);

        extrema.clear();

        // We start at the left edge, so compute extreme for the radiusX
        // columns.
        for (int x = 0; x < radiusX; ++x)
            extrema.push_back(columnExtremum(srcPixelBuffer, x, yRadiusStart,
                                             yRadiusEnd, stride, type));

        // Kernel is filled, get extrema of next column
        for (int x = 0; x < width; ++x) {
            if (x < width - radiusX) {
                extrema.push_back(columnExtremum(srcPixelBuffer, x + radiusX,
                                                 yRadiusStart, yRadiusEnd,
                                                 stride, type));
            }

            if (x > radiusX) {
                extrema.erase(extrema.begin());
            }
            makePixelValueFromColorComponents(dstPixelBuffer, x, y, stride,
                                              kernelExtremum(extrema, type));
        }
    }
}

FilterMorphology::FilterMorphology(
    Filter* filter, SVGFilterPrimitiveStandardAttributes* element)
    : FilterPrimitive(filter, element,
                      element->asSVGFEMorphologyElement()->in1()->baseVal(),
                      element->output()->baseVal())
{
    STARFISH_ASSERT(filter);
    STARFISH_ASSERT(element->isSVGFEMorphologyElement());
}

std::pair<float, float> FilterMorphology::computeRadiusXY(
    const LayoutSize& targetSize, const std::pair<float, float>& viewportScale)
{
    auto e = element()->asSVGFEMorphologyElement();
    float radiusX = filter()->resolveFilterPrimitiveValue(
        e->radiusX()->baseVal(), targetSize.width(), viewportScale.first);
    float radiusY = filter()->resolveFilterPrimitiveValue(
        e->radiusY()->baseVal(), targetSize.height(), viewportScale.second);

    return std::make_pair(radiusX, radiusY);
}

Filter::FilterBias FilterMorphology::computeBias(
    const LayoutSize& targetSize, const std::pair<float, float>& viewportScale)
{
    auto e = element()->asSVGFEMorphologyElement();
    if ((SVGFEMorphologyElement::MorphologyOperator)e->domOperator()
            ->baseVal() == SVGFEMorphologyElement::MorphologyOperator::
                               SVG_MORPHOLOGY_OPERATOR_DILATE) {
        return Filter::FilterBias(computeRadiusXY(targetSize, viewportScale));
    }

    return Filter::FilterBias();
}

void* FilterMorphology::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FilterMorphology));
    static bool typeInited = false;
    static GC_descr descr;
    if (typeInited == false) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(FilterMorphology)] = { 0 };
        STARFISH_ASSERT(obj_bitmap != nullptr);
        descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(FilterMorphology));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void FilterMorphology::apply(size_t x, size_t y, size_t width, size_t height,
                             Filter::FilterApplyContext& ctx)
{
    STARFISH_ASSERT(element()->isSVGFEMorphologyElement());
    auto e = element()->asSVGFEMorphologyElement();

    auto vm = ctx.target->outmostSVGViewportBox()
                  ->computeTranlateScaleOnPaint()
                  .second;
    auto radiusXY =
        computeRadiusXY(ctx.target->unadjustedFrameRectByFilter()->size(),
                        std::make_pair(vm.getScaleX(), vm.getScaleY()));
    if (radiusXY.first <= 0 || radiusXY.second <= 0) {
        return;
    }

    float dpr = element()->webView()->screenInfo().devicePixelRatio;
    radiusXY.first *= dpr;
    radiusXY.second *= dpr;

    std::shared_ptr<Filter::FilterSourceBuffer> inputSource =
        filter()->fetchInputSource(ctx, this);

    if (filter()->shouldMaintainSourceBuffer() &&
        inputSource->data() == ctx.src) {
        inputSource = std::shared_ptr<Filter::FilterSourceBuffer>(
            new Filter::FilterSourceBuffer(ctx.src, ctx.stride * ctx.height,
                                           true));
        memcpy(inputSource->data(), ctx.src, inputSource->size());
    }

    std::shared_ptr<Filter::FilterSourceBuffer> outputBuffer(
        new Filter::FilterSourceBuffer(ctx.src, ctx.stride * ctx.height, true));

    applyMorphology(inputSource->data(), outputBuffer->data(), 0, ctx.height,
                    radiusXY.first, radiusXY.second, ctx.width, ctx.height,
                    ctx.stride,
                    (SVGFEMorphologyElement::MorphologyOperator)e->domOperator()
                        ->baseVal());
    filter()->registerOutput(ctx, this, outputBuffer);
}

} // namespace Starfish
