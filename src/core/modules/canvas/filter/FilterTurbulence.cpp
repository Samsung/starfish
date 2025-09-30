/*
 24* Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann
 <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2010 Renata Hodovan <reni@inf.u-szeged.hu>
 * Copyright (C) 2011 Gabor Loki <loki@webkit.org>
 * Copyright (C) 2017-2022 Apple Inc. All rights reserved.
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

#include "core/dom/svg/SVGFETurbulenceElement.h"
#include "core/dom/svg/SVGFilterElement.h"
#include "core/modules/canvas/filter/Filter.h"
#include "core/modules/canvas/filter/FilterTurbulence.h"
#include "core/page/WebView.h"
#include "core/layout/svg/FrameSVGBox.h"
#include "core/layout/svg/FrameSVGSVGBox.h"
#include <cmath>
#include <algorithm>

namespace Starfish {

float convertSRGBtoLinearRGB(float c)
{
    if (c <= 0.04045) {
        return c / 12.92;
    } else {
        return pow((c + 0.055) / 1.055, 2.4);
    }
}

float convertLinearRGBtoSRGB(float c)
{
    if (c <= 0.0031308) {
        return 12.92 * c;
    } else {
        return 1.055 * pow(c, 1.0 / 2.4) - 0.055;
    }
}

FilterTurbulence::FilterTurbulence(
    Filter* filter, SVGFilterPrimitiveStandardAttributes* element)
    : FilterPrimitive(filter, element, String::emptyString,
                      element->output()->baseVal())
{
}

Filter::FilterBias FilterTurbulence::computeBias(ComputeBiasContext& ctx)
{
    ctx.currentVisibleRect.unite(LayoutRect(
        ctx.candidateFilterFrameRect.x(), ctx.candidateFilterFrameRect.y(),
        ctx.candidateFilterFrameRect.width(),
        ctx.candidateFilterFrameRect.height()));
    return Filter::FilterBias(NullOption, ctx.candidateFilterFrameRect);
}

void* FilterTurbulence::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FilterTurbulence));
    static bool typeInited = false;
    static GC_descr descr;
    if (typeInited == false) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(FilterTurbulence)] = { 0 };
        STARFISH_ASSERT(obj_bitmap != nullptr);
        descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(FilterTurbulence));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

// The turbulence calculation code is an adapted version of what appears in the
// SVG 1.1 specification: http://www.w3.org/TR/SVG11/filters.html#feTurbulence

FilterTurbulence::PaintingData FilterTurbulence::initPaintingData(
    int type, float baseFrequencyX, float baseFrequencyY, int numOctaves,
    long seed, bool stitchTiles, const Unit::IntSize& paintingSize)
{
    PaintingData paintingData{ type, baseFrequencyX, baseFrequencyY, numOctaves,
                               seed, stitchTiles,    paintingSize,   {},
                               {} };

    // The seed value clamp to the range [1, s_randMaximum - 1].
    if (paintingData.seed <= 0)
        paintingData.seed = -(paintingData.seed % (s_randMaximum - 1)) + 1;
    if (paintingData.seed > s_randMaximum - 1)
        paintingData.seed = s_randMaximum - 1;

    for (int channel = 0; channel < 4; ++channel) {
        for (int i = 0; i < s_blockSize; ++i) {
            paintingData.latticeSelector[i] = i;
            auto& gradient = paintingData.gradient[channel][i];
            do {
                gradient[0] = static_cast<float>(
                                  (paintingData.random() % (2 * s_blockSize)) -
                                  s_blockSize) /
                              s_blockSize;
                gradient[1] = static_cast<float>(
                                  (paintingData.random() % (2 * s_blockSize)) -
                                  s_blockSize) /
                              s_blockSize;
            } while (!gradient[0] && !gradient[1]);
            float normalizationFactor = std::hypot(gradient[0], gradient[1]);
            gradient[0] /= normalizationFactor;
            gradient[1] /= normalizationFactor;
        }
    }

    for (int i = s_blockSize - 1; i > 0; --i) {
        int k = paintingData.latticeSelector[i];
        int j = paintingData.random() % s_blockSize;
        STARFISH_ASSERT(j >= 0);
        STARFISH_ASSERT(j < 2 * s_blockSize + 2);
        paintingData.latticeSelector[i] = paintingData.latticeSelector[j];
        paintingData.latticeSelector[j] = k;
    }

    for (int i = 0; i < s_blockSize + 2; ++i) {
        paintingData.latticeSelector[s_blockSize + i] =
            paintingData.latticeSelector[i];
        for (int channel = 0; channel < 4; ++channel) {
            paintingData.gradient[channel][s_blockSize + i][0] =
                paintingData.gradient[channel][i][0];
            paintingData.gradient[channel][s_blockSize + i][1] =
                paintingData.gradient[channel][i][1];
        }
    }

    return paintingData;
}

FilterTurbulence::StitchData FilterTurbulence::computeStitching(
    Unit::IntSize tileSize, float& baseFrequencyX, float& baseFrequencyY,
    bool stitchTiles)
{
    if (!stitchTiles)
        return {};

    float tileWidth = tileSize.width();
    float tileHeight = tileSize.height();
    STARFISH_ASSERT(tileWidth > 0 && tileHeight > 0);

    // When stitching tiled turbulence, the frequencies must be adjusted
    // so that the tile borders will be continuous.
    if (baseFrequencyX) {
        float lowFrequency = floorf(tileWidth * baseFrequencyX) / tileWidth;
        float highFrequency = ceilf(tileWidth * baseFrequencyX) / tileWidth;
        // BaseFrequency should be non-negative according to the standard.
        if (baseFrequencyX / lowFrequency < highFrequency / baseFrequencyX)
            baseFrequencyX = lowFrequency;
        else
            baseFrequencyX = highFrequency;
    }
    if (baseFrequencyY) {
        float lowFrequency = floorf(tileHeight * baseFrequencyY) / tileHeight;
        float highFrequency = ceilf(tileHeight * baseFrequencyY) / tileHeight;
        if (baseFrequencyY / lowFrequency < highFrequency / baseFrequencyY)
            baseFrequencyY = lowFrequency;
        else
            baseFrequencyY = highFrequency;
    }

    StitchData stitchData;
    stitchData.width = roundf(tileWidth * baseFrequencyX);
    stitchData.wrapX = s_perlinNoise + stitchData.width;
    stitchData.height = roundf(tileHeight * baseFrequencyY);
    stitchData.wrapY = s_perlinNoise + stitchData.height;

    return stitchData;
}

// This is taken 1:1 from SVG spec:
// http://www.w3.org/TR/SVG11/filters.html#feTurbulenceElement.
std::array<float, 4> FilterTurbulence::noise2D(
    const PaintingData& paintingData, const StitchData& stitchData,
    const Unit::FloatPoint& noiseVector)
{
    struct NoisePosition {
        int index;      // bx0, by0 in the spec text.
        int nextIndex;  // bx1, by1 in the spec text.
        float fraction; // rx0, ry0 in the spec text.

        NoisePosition(float component)
        {
            //  t = vec[0] + PerlinN;
            //  bx0 = (int)t;
            //  bx1 = bx0+1;
            //  rx0 = t - (int)t;
            float position = component + s_perlinNoise;
            index = static_cast<int>(position);
            nextIndex = index + 1;
            fraction = position - index;
        }

        void stitch(int size, int wrapSize)
        {
            // if (bx0 >= pStitchInfo->nWrapX)
            //   bx0 -= pStitchInfo->nWidth;
            if (index >= wrapSize)
                index -= size;

            // if (bx1 >= pStitchInfo->nWrapX)
            //   bx1 -= pStitchInfo->nWidth;
            if (nextIndex >= wrapSize)
                nextIndex -= size;
        }
    };

    NoisePosition noiseX(noiseVector.x());
    NoisePosition noiseY(noiseVector.y());

    // If stitching, adjust lattice points accordingly.
    if (paintingData.stitchTiles) {
        noiseX.stitch(stitchData.width, stitchData.wrapX);
        noiseY.stitch(stitchData.height, stitchData.wrapY);
    }

    // bx0 &= BM;
    // bx1 &= BM;
    // by0 &= BM;
    // by1 &= BM;
    noiseX.index &= s_blockMask;
    noiseX.nextIndex &= s_blockMask;
    noiseY.index &= s_blockMask;
    noiseY.nextIndex &= s_blockMask;

    // i = uLatticeSelector[bx0];
    // j = uLatticeSelector[bx1];
    int latticeIndex = paintingData.latticeSelector[noiseX.index];
    int nextLatticeIndex = paintingData.latticeSelector[noiseX.nextIndex];

    // sx = double(s_curve(rx0));
    // sy = double(s_curve(ry0));
    float sx = smoothCurve(noiseX.fraction);
    float sy = smoothCurve(noiseY.fraction);

    auto noiseForChannel = [&](int channel) {
        // b00 = uLatticeSelector[i + by0]
        int b00 = paintingData.latticeSelector[latticeIndex + noiseY.index];
        // q = fGradient[nColorChannel][b00]; u = rx0 * q[0] + ry0 * q[1];
        const auto& q1 = paintingData.gradient[channel][b00];
        float u = noiseX.fraction * q1[0] + noiseY.fraction * q1[1];

        // b10 = uLatticeSelector[j + by0];
        int b10 = paintingData.latticeSelector[nextLatticeIndex + noiseY.index];
        // rx1 = rx0 - 1.0f;
        // q = fGradient[nColorChannel][b10]; v = rx1 * q[0] + ry0 * q[1];
        const auto& q2 = paintingData.gradient[channel][b10];
        float v = (noiseX.fraction - 1) * q2[0] + noiseY.fraction * q2[1];
        // a = lerp(sx, u, v);
        float a = linearInterpolation(sx, u, v);

        // b01 = uLatticeSelector[i + by1];
        int b01 = paintingData.latticeSelector[latticeIndex + noiseY.nextIndex];
        // ry1 = ry0 - 1.0f;
        // q = fGradient[nColorChannel][b01]; u = rx0 * q[0] + ry1 * q[1];
        const auto& q3 = paintingData.gradient[channel][b01];
        u = noiseX.fraction * q3[0] + (noiseY.fraction - 1) * q3[1];

        // b11 = uLatticeSelector[j + by1];
        int b11 =
            paintingData.latticeSelector[nextLatticeIndex + noiseY.nextIndex];
        // q = fGradient[nColorChannel][b11]; v = rx1 * q[0] + ry1 * q[1];
        const auto& q4 = paintingData.gradient[channel][b11];
        v = (noiseX.fraction - 1) * q4[0] + (noiseY.fraction - 1) * q4[1];
        // b = lerp(sx, u, v);
        float b = linearInterpolation(sx, u, v);

        // return lerp(sy, a, b);
        return linearInterpolation(sy, a, b);
    };

    return { noiseForChannel(0), noiseForChannel(1), noiseForChannel(2),
             noiseForChannel(3) };
}

std::array<uint8_t, 4> FilterTurbulence::toIntBasedColorComponents(
    const std::array<float, 4>& floatComponents)
{
    return {
        (uint8_t)clamp(static_cast<int>(floatComponents[0] * 255), 0, 255),
        (uint8_t)clamp(static_cast<int>(floatComponents[1] * 255), 0, 255),
        (uint8_t)clamp(static_cast<int>(floatComponents[2] * 255), 0, 255),
        (uint8_t)clamp(static_cast<int>(floatComponents[3] * 255), 0, 255),
    };
}

std::array<uint8_t, 4> FilterTurbulence::calculateTurbulenceValueForPoint(
    const PaintingData& paintingData, StitchData stitchData,
    const Unit::FloatPoint& point)
{
    std::array<float, 4> turbulenceFunctionResult{ 0, 0, 0, 0 };
    Unit::FloatPoint noiseVector(point.x() * paintingData.baseFrequencyX,
                                 point.y() * paintingData.baseFrequencyY);
    float ratio = 1;
    for (int octave = 0; octave < paintingData.numOctaves; ++octave) {
        auto noise = noise2D(paintingData, stitchData, noiseVector);
        if (paintingData.type ==
            SVGFETurbulenceElement::SVG_TURBULENCE_TYPE_FRACTALNOISE) {
            turbulenceFunctionResult[0] += noise[0] / ratio;
            turbulenceFunctionResult[1] += noise[1] / ratio;
            turbulenceFunctionResult[2] += noise[2] / ratio;
            turbulenceFunctionResult[3] += noise[3] / ratio;
        } else {
            turbulenceFunctionResult[0] += std::abs(noise[0]) / ratio;
            turbulenceFunctionResult[1] += std::abs(noise[1]) / ratio;
            turbulenceFunctionResult[2] += std::abs(noise[2]) / ratio;
            turbulenceFunctionResult[3] += std::abs(noise[3]) / ratio;
        }

        noiseVector.setX(noiseVector.x() * 2);
        noiseVector.setY(noiseVector.y() * 2);
        ratio *= 2;

        if (paintingData.stitchTiles) {
            // Update stitch values. Subtracting s_perlinNoise before the
            // multiplication and adding it afterward simplifies to subtracting
            // it once.
            stitchData.width *= 2;
            stitchData.wrapX = 2 * stitchData.wrapX - s_perlinNoise;
            stitchData.height *= 2;
            stitchData.wrapY = 2 * stitchData.wrapY - s_perlinNoise;
        }
    }

    // The value of turbulenceFunctionResult comes from
    // ((turbulenceFunctionResult * 255) + 255) / 2 by fractalNoise and
    // (turbulenceFunctionResult * 255) by turbulence.
    if (paintingData.type ==
        SVGFETurbulenceElement::SVG_TURBULENCE_TYPE_FRACTALNOISE) {
        turbulenceFunctionResult[0] = turbulenceFunctionResult[0] * 0.5f + 0.5f;
        turbulenceFunctionResult[1] = turbulenceFunctionResult[1] * 0.5f + 0.5f;
        turbulenceFunctionResult[2] = turbulenceFunctionResult[2] * 0.5f + 0.5f;
        turbulenceFunctionResult[3] = turbulenceFunctionResult[3] * 0.5f + 0.5f;
    }

    return toIntBasedColorComponents(turbulenceFunctionResult);
}

void FilterTurbulence::apply(const Unit::Rect& subRegionInFloat,
                             Filter::FilterApplyContext& ctx)
{
    STARFISH_ASSERT(element()->isSVGFETurbulenceElement());
    auto e = element()->asSVGFETurbulenceElement();

    auto normalizedSubRegion = normalizeSubRegion(subRegionInFloat);
    auto outputSource = filter()->fetchOutputSource(
        ctx, this, ctx.sourceGraphic(), subRegionInFloat);
    bool isSubRegionCoversAll = subRegionCoversAll(normalizedSubRegion);

    float baseFrequencyX = e->baseFrequencyX()->animVal();
    float baseFrequencyY = e->baseFrequencyY()->animVal();

    size_t xposition = std::max(0, ctx.target->frameRect().x().toInt());
    size_t yposition = std::max(0, ctx.target->frameRect().y().toInt());
    size_t width = ctx.width;
    size_t height = ctx.height;

    if (!isSubRegionCoversAll) {
        xposition = ctx.width * normalizedSubRegion.x();
        width = xposition + ctx.width * normalizedSubRegion.width();
        yposition = ctx.height * normalizedSubRegion.y();
        height = yposition + ctx.height * normalizedSubRegion.height();
    }

    auto stitchData = computeStitching(
        Unit::IntSize(width, height), baseFrequencyX, baseFrequencyY,
        e->stitchTiles()->animVal() ==
            SVGFETurbulenceElement::SVG_STITCHTYPE_STITCH);
    auto paintingData =
        initPaintingData(e->type()->animVal(), baseFrequencyX, baseFrequencyY,
                         e->numOctaves()->animVal(), e->seed()->animVal(),
                         e->stitchTiles()->animVal() ==
                             SVGFETurbulenceElement::SVG_STITCHTYPE_STITCH,
                         Unit::IntSize(width, height));

    unsigned char* data = (unsigned char*)outputSource->data();
    for (uint y = 0; y < height; y++) {
        for (uint x = 0; x < width; x++) {
            auto color = calculateTurbulenceValueForPoint(
                paintingData, stitchData,
                Unit::FloatPoint(x + xposition, y + yposition));
            int offset = y * ctx.stride + x * 4;
            data[offset + STARFISH_PIXEL_R_INDEX] = color[0];
            data[offset + STARFISH_PIXEL_G_INDEX] = color[1];
            data[offset + STARFISH_PIXEL_B_INDEX] = color[2];
            data[offset + STARFISH_PIXEL_A_INDEX] = color[3];
        }
    }

    convertImageBufferAsPremultipliedAlphaIfNeeds(
        outputSource->data(), ctx.width, ctx.stride, ctx.height);

    filter()->registerOutput(ctx, this, outputSource);
}

} // namespace Starfish
