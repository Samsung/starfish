/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann
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
#include "core/modules/threading/ParallelJobExecutor.h"
#include "core/page/WebView.h"
#include "core/layout/svg/FrameSVGBox.h"
#include "core/layout/svg/FrameSVGSVGBox.h"
#include <cmath>
#include <algorithm>

// SIMD headers for different architectures
#if defined(STARFISH_X86) || defined(STARFISH_X86_64)
#include <immintrin.h>
#elif defined(STARFISH_ARM_NEON)
#include <arm_neon.h>
#endif

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

// ============================================================================
// SIMD-optimized noise2D implementation
// Processes all 4 channels simultaneously using SIMD intrinsics
// ============================================================================

#if defined(STARFISH_X86) || defined(STARFISH_X86_64)
// SSE/SSE2 implementation for x86/x86_64
// Returns __m128 directly to avoid SIMD -> memory -> SIMD conversion
ALWAYS_INLINE SimdFloat4 FilterTurbulence::noise2D(
    const PaintingData& paintingData, const StitchData& stitchData,
    const Unit::FloatPoint& noiseVector)
{
    // Calculate noise positions
    float posX = noiseVector.x() + s_perlinNoise;
    float posY = noiseVector.y() + s_perlinNoise;

    int indexX = static_cast<int>(posX);
    int nextIndexX = indexX + 1;
    float fractionX = posX - indexX;

    int indexY = static_cast<int>(posY);
    int nextIndexY = indexY + 1;
    float fractionY = posY - indexY;

    // Handle stitching
    if (paintingData.stitchTiles) {
        if (indexX >= stitchData.wrapX)
            indexX -= stitchData.width;
        if (nextIndexX >= stitchData.wrapX)
            nextIndexX -= stitchData.width;
        if (indexY >= stitchData.wrapY)
            indexY -= stitchData.height;
        if (nextIndexY >= stitchData.wrapY)
            nextIndexY -= stitchData.height;
    }

    // Apply block mask
    indexX &= s_blockMask;
    nextIndexX &= s_blockMask;
    indexY &= s_blockMask;
    nextIndexY &= s_blockMask;

    // Get lattice indices
    int latticeIndex = paintingData.latticeSelector[indexX];
    int nextLatticeIndex = paintingData.latticeSelector[nextIndexX];

    // Get gradient indices
    int b00 = paintingData.latticeSelector[latticeIndex + indexY];
    int b10 = paintingData.latticeSelector[nextLatticeIndex + indexY];
    int b01 = paintingData.latticeSelector[latticeIndex + nextIndexY];
    int b11 = paintingData.latticeSelector[nextLatticeIndex + nextIndexY];

    // Smooth curves
    float sx = smoothCurve(fractionX);
    float sy = smoothCurve(fractionY);

    // Load all gradients for 4 channels at once using SSE
    // _mm_set_ps args are in reverse order: _mm_set_ps(d, c, b, a) stores as
    // [a, b, c, d] We generate noise directly in the correct pixel order
    // without shuffle. BGRA: buffer[0]=B(ch2), buffer[1]=G(ch1),
    // buffer[2]=R(ch0), buffer[3]=A(ch3) RGBA: buffer[0]=R(ch0),
    // buffer[1]=G(ch1), buffer[2]=B(ch2), buffer[3]=A(ch3)
#if defined(PORT_PIXEL_ORDER_BGRA)
    // BGRA order: store as [ch2, ch1, ch0, ch3] = [B, G, R, A]
    __m128 q1_0 = _mm_set_ps(
        paintingData.gradient[3][b00][0], paintingData.gradient[0][b00][0],
        paintingData.gradient[1][b00][0], paintingData.gradient[2][b00][0]);
    __m128 q1_1 = _mm_set_ps(
        paintingData.gradient[3][b00][1], paintingData.gradient[0][b00][1],
        paintingData.gradient[1][b00][1], paintingData.gradient[2][b00][1]);

    __m128 q2_0 = _mm_set_ps(
        paintingData.gradient[3][b10][0], paintingData.gradient[0][b10][0],
        paintingData.gradient[1][b10][0], paintingData.gradient[2][b10][0]);
    __m128 q2_1 = _mm_set_ps(
        paintingData.gradient[3][b10][1], paintingData.gradient[0][b10][1],
        paintingData.gradient[1][b10][1], paintingData.gradient[2][b10][1]);

    __m128 q3_0 = _mm_set_ps(
        paintingData.gradient[3][b01][0], paintingData.gradient[0][b01][0],
        paintingData.gradient[1][b01][0], paintingData.gradient[2][b01][0]);
    __m128 q3_1 = _mm_set_ps(
        paintingData.gradient[3][b01][1], paintingData.gradient[0][b01][1],
        paintingData.gradient[1][b01][1], paintingData.gradient[2][b01][1]);

    __m128 q4_0 = _mm_set_ps(
        paintingData.gradient[3][b11][0], paintingData.gradient[0][b11][0],
        paintingData.gradient[1][b11][0], paintingData.gradient[2][b11][0]);
    __m128 q4_1 = _mm_set_ps(
        paintingData.gradient[3][b11][1], paintingData.gradient[0][b11][1],
        paintingData.gradient[1][b11][1], paintingData.gradient[2][b11][1]);
#else
    // RGBA order: store as [ch0, ch1, ch2, ch3] = [R, G, B, A]
    __m128 q1_0 = _mm_set_ps(
        paintingData.gradient[3][b00][0], paintingData.gradient[2][b00][0],
        paintingData.gradient[1][b00][0], paintingData.gradient[0][b00][0]);
    __m128 q1_1 = _mm_set_ps(
        paintingData.gradient[3][b00][1], paintingData.gradient[2][b00][1],
        paintingData.gradient[1][b00][1], paintingData.gradient[0][b00][1]);

    __m128 q2_0 = _mm_set_ps(
        paintingData.gradient[3][b10][0], paintingData.gradient[2][b10][0],
        paintingData.gradient[1][b10][0], paintingData.gradient[0][b10][0]);
    __m128 q2_1 = _mm_set_ps(
        paintingData.gradient[3][b10][1], paintingData.gradient[2][b10][1],
        paintingData.gradient[1][b10][1], paintingData.gradient[0][b10][1]);

    __m128 q3_0 = _mm_set_ps(
        paintingData.gradient[3][b01][0], paintingData.gradient[2][b01][0],
        paintingData.gradient[1][b01][0], paintingData.gradient[0][b01][0]);
    __m128 q3_1 = _mm_set_ps(
        paintingData.gradient[3][b01][1], paintingData.gradient[2][b01][1],
        paintingData.gradient[1][b01][1], paintingData.gradient[0][b01][1]);

    __m128 q4_0 = _mm_set_ps(
        paintingData.gradient[3][b11][0], paintingData.gradient[2][b11][0],
        paintingData.gradient[1][b11][0], paintingData.gradient[0][b11][0]);
    __m128 q4_1 = _mm_set_ps(
        paintingData.gradient[3][b11][1], paintingData.gradient[2][b11][1],
        paintingData.gradient[1][b11][1], paintingData.gradient[0][b11][1]);
#endif

    // Broadcast fractions
    __m128 fracX = _mm_set1_ps(fractionX);
    __m128 fracY = _mm_set1_ps(fractionY);
    __m128 fracX_minus1 = _mm_set1_ps(fractionX - 1.0f);
    __m128 fracY_minus1 = _mm_set1_ps(fractionY - 1.0f);

    // Compute u and v for first interpolation
    // u = fractionX * q1_0 + fractionY * q1_1
    __m128 u = _mm_add_ps(_mm_mul_ps(fracX, q1_0), _mm_mul_ps(fracY, q1_1));
    // v = (fractionX - 1) * q2_0 + fractionY * q2_1
    __m128 v =
        _mm_add_ps(_mm_mul_ps(fracX_minus1, q2_0), _mm_mul_ps(fracY, q2_1));

    // a = lerp(sx, u, v) = u + sx * (v - u)
    __m128 sx_vec = _mm_set1_ps(sx);
    __m128 a = _mm_add_ps(u, _mm_mul_ps(sx_vec, _mm_sub_ps(v, u)));

    // Compute u and v for second interpolation
    // u = fractionX * q3_0 + (fractionY - 1) * q3_1
    u = _mm_add_ps(_mm_mul_ps(fracX, q3_0), _mm_mul_ps(fracY_minus1, q3_1));
    // v = (fractionX - 1) * q4_0 + (fractionY - 1) * q4_1
    v = _mm_add_ps(_mm_mul_ps(fracX_minus1, q4_0),
                   _mm_mul_ps(fracY_minus1, q4_1));

    // b = lerp(sx, u, v)
    __m128 b = _mm_add_ps(u, _mm_mul_ps(sx_vec, _mm_sub_ps(v, u)));

    // result = lerp(sy, a, b)
    __m128 sy_vec = _mm_set1_ps(sy);
    __m128 result = _mm_add_ps(a, _mm_mul_ps(sy_vec, _mm_sub_ps(b, a)));

    // Return SIMD register directly - no memory store!
    return result;
}

#elif defined(STARFISH_ARM_NEON)
// NEON implementation for ARM/ARM64
// Returns float32x4_t directly to avoid SIMD -> memory -> SIMD conversion
ALWAYS_INLINE SimdFloat4 FilterTurbulence::noise2D(
    const PaintingData& paintingData, const StitchData& stitchData,
    const Unit::FloatPoint& noiseVector)
{
    // Calculate noise positions
    float posX = noiseVector.x() + s_perlinNoise;
    float posY = noiseVector.y() + s_perlinNoise;

    int indexX = static_cast<int>(posX);
    int nextIndexX = indexX + 1;
    float fractionX = posX - indexX;

    int indexY = static_cast<int>(posY);
    int nextIndexY = indexY + 1;
    float fractionY = posY - indexY;

    // Handle stitching
    if (paintingData.stitchTiles) {
        if (indexX >= stitchData.wrapX)
            indexX -= stitchData.width;
        if (nextIndexX >= stitchData.wrapX)
            nextIndexX -= stitchData.width;
        if (indexY >= stitchData.wrapY)
            indexY -= stitchData.height;
        if (nextIndexY >= stitchData.wrapY)
            nextIndexY -= stitchData.height;
    }

    // Apply block mask
    indexX &= s_blockMask;
    nextIndexX &= s_blockMask;
    indexY &= s_blockMask;
    nextIndexY &= s_blockMask;

    // Get lattice indices
    int latticeIndex = paintingData.latticeSelector[indexX];
    int nextLatticeIndex = paintingData.latticeSelector[nextIndexX];

    // Get gradient indices
    int b00 = paintingData.latticeSelector[latticeIndex + indexY];
    int b10 = paintingData.latticeSelector[nextLatticeIndex + indexY];
    int b01 = paintingData.latticeSelector[latticeIndex + nextIndexY];
    int b11 = paintingData.latticeSelector[nextLatticeIndex + nextIndexY];

    // Smooth curves
    float sx = smoothCurve(fractionX);
    float sy = smoothCurve(fractionY);

    // Load all gradients for 4 channels at once using NEON
    // We generate noise directly in the correct pixel order without shuffle.
    // BGRA: buffer[0]=B(ch2), buffer[1]=G(ch1), buffer[2]=R(ch0),
    // buffer[3]=A(ch3) RGBA: buffer[0]=R(ch0), buffer[1]=G(ch1),
    // buffer[2]=B(ch2), buffer[3]=A(ch3)
#if defined(PORT_PIXEL_ORDER_BGRA)
    // BGRA order: store as [ch2, ch1, ch0, ch3] = [B, G, R, A]
    float32_t q1_0_arr[4] = { paintingData.gradient[2][b00][0],
                              paintingData.gradient[1][b00][0],
                              paintingData.gradient[0][b00][0],
                              paintingData.gradient[3][b00][0] };
    float32_t q1_1_arr[4] = { paintingData.gradient[2][b00][1],
                              paintingData.gradient[1][b00][1],
                              paintingData.gradient[0][b00][1],
                              paintingData.gradient[3][b00][1] };

    float32_t q2_0_arr[4] = { paintingData.gradient[2][b10][0],
                              paintingData.gradient[1][b10][0],
                              paintingData.gradient[0][b10][0],
                              paintingData.gradient[3][b10][0] };
    float32_t q2_1_arr[4] = { paintingData.gradient[2][b10][1],
                              paintingData.gradient[1][b10][1],
                              paintingData.gradient[0][b10][1],
                              paintingData.gradient[3][b10][1] };

    float32_t q3_0_arr[4] = { paintingData.gradient[2][b01][0],
                              paintingData.gradient[1][b01][0],
                              paintingData.gradient[0][b01][0],
                              paintingData.gradient[3][b01][0] };
    float32_t q3_1_arr[4] = { paintingData.gradient[2][b01][1],
                              paintingData.gradient[1][b01][1],
                              paintingData.gradient[0][b01][1],
                              paintingData.gradient[3][b01][1] };

    float32_t q4_0_arr[4] = { paintingData.gradient[2][b11][0],
                              paintingData.gradient[1][b11][0],
                              paintingData.gradient[0][b11][0],
                              paintingData.gradient[3][b11][0] };
    float32_t q4_1_arr[4] = { paintingData.gradient[2][b11][1],
                              paintingData.gradient[1][b11][1],
                              paintingData.gradient[0][b11][1],
                              paintingData.gradient[3][b11][1] };
#else
    // RGBA order: store as [ch0, ch1, ch2, ch3] = [R, G, B, A]
    float32_t q1_0_arr[4] = { paintingData.gradient[0][b00][0],
                              paintingData.gradient[1][b00][0],
                              paintingData.gradient[2][b00][0],
                              paintingData.gradient[3][b00][0] };
    float32_t q1_1_arr[4] = { paintingData.gradient[0][b00][1],
                              paintingData.gradient[1][b00][1],
                              paintingData.gradient[2][b00][1],
                              paintingData.gradient[3][b00][1] };

    float32_t q2_0_arr[4] = { paintingData.gradient[0][b10][0],
                              paintingData.gradient[1][b10][0],
                              paintingData.gradient[2][b10][0],
                              paintingData.gradient[3][b10][0] };
    float32_t q2_1_arr[4] = { paintingData.gradient[0][b10][1],
                              paintingData.gradient[1][b10][1],
                              paintingData.gradient[2][b10][1],
                              paintingData.gradient[3][b10][1] };

    float32_t q3_0_arr[4] = { paintingData.gradient[0][b01][0],
                              paintingData.gradient[1][b01][0],
                              paintingData.gradient[2][b01][0],
                              paintingData.gradient[3][b01][0] };
    float32_t q3_1_arr[4] = { paintingData.gradient[0][b01][1],
                              paintingData.gradient[1][b01][1],
                              paintingData.gradient[2][b01][1],
                              paintingData.gradient[3][b01][1] };

    float32_t q4_0_arr[4] = { paintingData.gradient[0][b11][0],
                              paintingData.gradient[1][b11][0],
                              paintingData.gradient[2][b11][0],
                              paintingData.gradient[3][b11][0] };
    float32_t q4_1_arr[4] = { paintingData.gradient[0][b11][1],
                              paintingData.gradient[1][b11][1],
                              paintingData.gradient[2][b11][1],
                              paintingData.gradient[3][b11][1] };
#endif

    float32x4_t q1_0 = vld1q_f32(q1_0_arr);
    float32x4_t q1_1 = vld1q_f32(q1_1_arr);
    float32x4_t q2_0 = vld1q_f32(q2_0_arr);
    float32x4_t q2_1 = vld1q_f32(q2_1_arr);
    float32x4_t q3_0 = vld1q_f32(q3_0_arr);
    float32x4_t q3_1 = vld1q_f32(q3_1_arr);
    float32x4_t q4_0 = vld1q_f32(q4_0_arr);
    float32x4_t q4_1 = vld1q_f32(q4_1_arr);

    // Broadcast fractions
    float32x4_t fracX = vdupq_n_f32(fractionX);
    float32x4_t fracY = vdupq_n_f32(fractionY);
    float32x4_t fracX_minus1 = vdupq_n_f32(fractionX - 1.0f);
    float32x4_t fracY_minus1 = vdupq_n_f32(fractionY - 1.0f);

    // Compute u and v for first interpolation
    float32x4_t u = vaddq_f32(vmulq_f32(fracX, q1_0), vmulq_f32(fracY, q1_1));
    float32x4_t v =
        vaddq_f32(vmulq_f32(fracX_minus1, q2_0), vmulq_f32(fracY, q2_1));

    // a = lerp(sx, u, v) = u + sx * (v - u)
    float32x4_t sx_vec = vdupq_n_f32(sx);
    float32x4_t a = vaddq_f32(u, vmulq_f32(sx_vec, vsubq_f32(v, u)));

    // Compute u and v for second interpolation
    u = vaddq_f32(vmulq_f32(fracX, q3_0), vmulq_f32(fracY_minus1, q3_1));
    v = vaddq_f32(vmulq_f32(fracX_minus1, q4_0), vmulq_f32(fracY_minus1, q4_1));

    // b = lerp(sx, u, v)
    float32x4_t b = vaddq_f32(u, vmulq_f32(sx_vec, vsubq_f32(v, u)));

    // result = lerp(sy, a, b)
    float32x4_t sy_vec = vdupq_n_f32(sy);
    float32x4_t result = vaddq_f32(a, vmulq_f32(sy_vec, vsubq_f32(b, a)));

    // Return SIMD register directly - no memory store!
    return result;
}

#else
// Fallback C implementation for other architectures
// Returns std::array<float, 4> for non-SIMD architectures
ALWAYS_INLINE SimdFloat4 FilterTurbulence::noise2D(
    const PaintingData& paintingData, const StitchData& stitchData,
    const Unit::FloatPoint& noiseVector)
{
    // Calculate noise positions
    float posX = noiseVector.x() + s_perlinNoise;
    float posY = noiseVector.y() + s_perlinNoise;

    int indexX = static_cast<int>(posX);
    int nextIndexX = indexX + 1;
    float fractionX = posX - indexX;

    int indexY = static_cast<int>(posY);
    int nextIndexY = indexY + 1;
    float fractionY = posY - indexY;

    // Handle stitching
    if (paintingData.stitchTiles) {
        if (indexX >= stitchData.wrapX)
            indexX -= stitchData.width;
        if (nextIndexX >= stitchData.wrapX)
            nextIndexX -= stitchData.width;
        if (indexY >= stitchData.wrapY)
            indexY -= stitchData.height;
        if (nextIndexY >= stitchData.wrapY)
            nextIndexY -= stitchData.height;
    }

    // Apply block mask
    indexX &= s_blockMask;
    nextIndexX &= s_blockMask;
    indexY &= s_blockMask;
    nextIndexY &= s_blockMask;

    // Get lattice indices
    int latticeIndex = paintingData.latticeSelector[indexX];
    int nextLatticeIndex = paintingData.latticeSelector[nextIndexX];

    // Get gradient indices
    int b00 = paintingData.latticeSelector[latticeIndex + indexY];
    int b10 = paintingData.latticeSelector[nextLatticeIndex + indexY];
    int b01 = paintingData.latticeSelector[latticeIndex + nextIndexY];
    int b11 = paintingData.latticeSelector[nextLatticeIndex + nextIndexY];

    // Smooth curves
    float sx = smoothCurve(fractionX);
    float sy = smoothCurve(fractionY);

    // Pre-compute common values
    float fracX_minus1 = fractionX - 1.0f;
    float fracY_minus1 = fractionY - 1.0f;

    // Process all 4 channels and store in correct pixel order
    // BGRA: buffer[0]=B(ch2), buffer[1]=G(ch1), buffer[2]=R(ch0),
    // buffer[3]=A(ch3) RGBA: buffer[0]=R(ch0), buffer[1]=G(ch1),
    // buffer[2]=B(ch2), buffer[3]=A(ch3)
    std::array<float, 4> result;

    // Helper lambda to compute noise for a single channel
    auto computeChannelNoise = [&](int channel) -> float {
        const auto& g00 = paintingData.gradient[channel][b00];
        const auto& g10 = paintingData.gradient[channel][b10];
        const auto& g01 = paintingData.gradient[channel][b01];
        const auto& g11 = paintingData.gradient[channel][b11];

        float u = fractionX * g00[0] + fractionY * g00[1];
        float v = fracX_minus1 * g10[0] + fractionY * g10[1];
        float a = linearInterpolation(sx, u, v);

        u = fractionX * g01[0] + fracY_minus1 * g01[1];
        v = fracX_minus1 * g11[0] + fracY_minus1 * g11[1];
        float b = linearInterpolation(sx, u, v);

        return linearInterpolation(sy, a, b);
    };

#if defined(PORT_PIXEL_ORDER_BGRA)
    // BGRA order: [B, G, R, A] = [ch2, ch1, ch0, ch3]
    result[0] = computeChannelNoise(2); // B
    result[1] = computeChannelNoise(1); // G
    result[2] = computeChannelNoise(0); // R
    result[3] = computeChannelNoise(3); // A
#else
    // RGBA order: [R, G, B, A] = [ch0, ch1, ch2, ch3]
    result[0] = computeChannelNoise(0); // R
    result[1] = computeChannelNoise(1); // G
    result[2] = computeChannelNoise(2); // B
    result[3] = computeChannelNoise(3); // A
#endif

    return result;
}
#endif

// ============================================================================
// SIMD-optimized toIntBasedColorComponents implementation
// Converts SimdFloat4 to uint8_t buffer using SIMD pack operations
// Writes directly to buffer - no intermediate array needed
// ============================================================================

#if defined(STARFISH_X86) || defined(STARFISH_X86_64)
// SSE/SSE2 implementation for x86/x86_64
// Uses pack instructions to convert float -> int32 -> int16 -> uint8
ALWAYS_INLINE void FilterTurbulence::toIntBasedColorComponents(
    SimdFloat4 floatComponents, uint8_t* buffer)
{
#if defined(PORT_CANVAS_NEEDS_PREMULTIPLIED_ALPHA)
    // Premultiplied alpha: multiply R, G, B by A (alpha is in component 3)
    // floatComponents layout: [B, G, R, A] or [R, G, B, A] depending on
    // platform Alpha is always in component 3
    __m128 alpha = _mm_shuffle_ps(floatComponents, floatComponents,
                                  _MM_SHUFFLE(3, 3, 3, 3));
    __m128 premultiplied = _mm_mul_ps(floatComponents, alpha);
    // Keep alpha unchanged: blend original alpha with premultiplied RGB
    // premultiplied = [B*A, G*A, R*A, A*A] -> need [B*A, G*A, R*A, A]
    // Use SSE2 compatible blend: mask for component 3 (highest 32 bits)
    __m128i mask =
        _mm_set_epi32(-1, 0, 0, 0); // All 1s in component 3, 0 elsewhere
    floatComponents = _mm_or_ps(
        _mm_andnot_ps(_mm_castsi128_ps(mask),
                      premultiplied), // premultiplied where mask is 0
        _mm_and_ps(_mm_castsi128_ps(mask),
                   floatComponents) // floatComponents where mask is 1 (alpha)
    );
#endif

    // Multiply by 255.0f
    __m128 scaled = _mm_mul_ps(floatComponents, _mm_set1_ps(255.0f));

    // Convert float to int32 (uses SSE2: _mm_cvtps_epi32)
    __m128i i32 = _mm_cvtps_epi32(scaled);

    // Pack int32 to int16 with saturation (uses SSE2: _mm_packs_epi32)
    // This packs 4 x int32 into 8 x int16 (with saturation to [-32768, 32767])
    __m128i i16 = _mm_packs_epi32(i32, _mm_setzero_si128());

    // Pack int16 to uint8 with unsigned saturation (uses SSE2:
    // _mm_packus_epi16) This packs 8 x int16 into 16 x uint8 (with saturation
    // to [0, 255])
    __m128i u8 = _mm_packus_epi16(i16, _mm_setzero_si128());

    // Store the lower 4 bytes directly to buffer
    // _mm_cvtsi128_si32 extracts the lower 32 bits as an int
    *reinterpret_cast<uint32_t*>(buffer) = _mm_cvtsi128_si32(u8);
}

#elif defined(STARFISH_ARM64)
// NEON implementation for ARM64 (ARMv8)
// Uses saturating narrowing instructions for efficient conversion
ALWAYS_INLINE void FilterTurbulence::toIntBasedColorComponents(
    SimdFloat4 floatComponents, uint8_t* buffer)
{
#if defined(PORT_CANVAS_NEEDS_PREMULTIPLIED_ALPHA)
    // Premultiplied alpha: multiply R, G, B by A (alpha is in component 3)
    // floatComponents layout: [B, G, R, A] or [R, G, B, A] depending on
    // platform Alpha is always in component 3
    // vdupq_laneq_f32 is ARM64 only
    float32x4_t alpha = vdupq_laneq_f32(floatComponents, 3);
    float32x4_t premultiplied = vmulq_f32(floatComponents, alpha);
    // Keep alpha unchanged: blend original alpha with premultiplied RGB
    // premultiplied = [B*A, G*A, R*A, A*A] -> need [B*A, G*A, R*A, A]
    // Use vsetq_lane_f32 to restore the original alpha value
    floatComponents =
        vsetq_lane_f32(vgetq_lane_f32(floatComponents, 3), premultiplied, 3);
#endif

    // Multiply by 255.0f
    float32x4_t scaled = vmulq_n_f32(floatComponents, 255.0f);

    // Convert float to int32 (rounds towards zero)
    int32x4_t i32 = vcvtq_s32_f32(scaled);

    // Saturating narrow int32 -> int16 (saturates to [-32768, 32767])
    int16x4_t i16 = vqmovn_s32(i32);

    // Saturating narrow int16 -> uint8 (saturates to [0, 255])
    uint8x8_t u8 = vqmovun_s16(vcombine_s16(i16, vdup_n_s16(0)));

    // Extract the 4 bytes we need
    *reinterpret_cast<uint32_t*>(buffer) =
        vget_lane_u32(vreinterpret_u32_u8(u8), 0);
}

#elif defined(STARFISH_ARM) && defined(STARFISH_ARM_NEON)
// NEON implementation for ARM32 (ARMv7)
// Uses saturating narrowing instructions for efficient conversion
ALWAYS_INLINE void FilterTurbulence::toIntBasedColorComponents(
    SimdFloat4 floatComponents, uint8_t* buffer)
{
#if defined(PORT_CANVAS_NEEDS_PREMULTIPLIED_ALPHA)
    // Premultiplied alpha: multiply R, G, B by A (alpha is in component 3)
    // floatComponents layout: [B, G, R, A] or [R, G, B, A] depending on
    // platform Alpha is always in component 3
    // ARM32 compatible: use vgetq_lane_f32 + vdupq_n_f32 instead of
    // vdupq_laneq_f32
    float alpha = vgetq_lane_f32(floatComponents, 3);
    float32x4_t alphaVec = vdupq_n_f32(alpha);
    float32x4_t premultiplied = vmulq_f32(floatComponents, alphaVec);
    // Keep alpha unchanged: blend original alpha with premultiplied RGB
    // premultiplied = [B*A, G*A, R*A, A*A] -> need [B*A, G*A, R*A, A]
    // Use vsetq_lane_f32 to restore the original alpha value
    floatComponents = vsetq_lane_f32(alpha, premultiplied, 3);
#endif

    // Multiply by 255.0f
    float32x4_t scaled = vmulq_n_f32(floatComponents, 255.0f);

    // Convert float to int32 (rounds towards zero)
    int32x4_t i32 = vcvtq_s32_f32(scaled);

    // Saturating narrow int32 -> int16 (saturates to [-32768, 32767])
    int16x4_t i16 = vqmovn_s32(i32);

    // Saturating narrow int16 -> uint8 (saturates to [0, 255])
    uint8x8_t u8 = vqmovun_s16(vcombine_s16(i16, vdup_n_s16(0)));

    // Extract the 4 bytes we need
    *reinterpret_cast<uint32_t*>(buffer) =
        vget_lane_u32(vreinterpret_u32_u8(u8), 0);
}

#else
// Fallback C implementation for other architectures
// Writes directly to buffer in the correct pixel order
ALWAYS_INLINE void FilterTurbulence::toIntBasedColorComponents(
    SimdFloat4 floatComponents, uint8_t* buffer)
{
#if defined(PORT_CANVAS_NEEDS_PREMULTIPLIED_ALPHA)
    // Premultiplied alpha: multiply R, G, B by A (alpha is in component 3)
    // floatComponents layout: [B, G, R, A] or [R, G, B, A] depending on
    // platform Alpha is always in component 3
    float alpha = floatComponents[3];
    floatComponents[0] *= alpha;
    floatComponents[1] *= alpha;
    floatComponents[2] *= alpha;
    // Keep alpha unchanged
#endif

    // floatComponents is already in the correct pixel order
    // BGRA: [B, G, R, A], RGBA: [R, G, B, A]
    buffer[0] =
        (uint8_t)clamp(static_cast<int>(floatComponents[0] * 255), 0, 255);
    buffer[1] =
        (uint8_t)clamp(static_cast<int>(floatComponents[1] * 255), 0, 255);
    buffer[2] =
        (uint8_t)clamp(static_cast<int>(floatComponents[2] * 255), 0, 255);
    buffer[3] =
        (uint8_t)clamp(static_cast<int>(floatComponents[3] * 255), 0, 255);
}
#endif

// ============================================================================
// SIMD-optimized calculateTurbulenceValueForPoint implementation
// Processes turbulenceFunctionResult using SIMD intrinsics
// Writes directly to buffer - no intermediate array needed
// ============================================================================

#if defined(STARFISH_X86) || defined(STARFISH_X86_64)
// SSE/SSE2 implementation for x86/x86_64
// noise2D returns __m128 directly - no memory conversion needed!
ALWAYS_INLINE void FilterTurbulence::calculateTurbulenceValueForPoint(
    const PaintingData& paintingData, StitchData stitchData,
    const Unit::FloatPoint& point, uint8_t* buffer)
{
    // Initialize turbulenceFunctionResult to zero using SSE
    __m128 turbulenceResult = _mm_setzero_ps();

    Unit::FloatPoint noiseVector(point.x() * paintingData.baseFrequencyX,
                                 point.y() * paintingData.baseFrequencyY);
    float ratio = 1;

    for (int octave = 0; octave < paintingData.numOctaves; ++octave) {
        // noise2D returns __m128 directly - no memory store/load!
        __m128 noiseVec = noise2D(paintingData, stitchData, noiseVector);

        // Broadcast ratio
        __m128 ratioVec = _mm_set1_ps(ratio);

        if (paintingData.type ==
            SVGFETurbulenceElement::SVG_TURBULENCE_TYPE_FRACTALNOISE) {
            // turbulenceResult += noise / ratio
            __m128 noiseDivRatio = _mm_div_ps(noiseVec, ratioVec);
            turbulenceResult = _mm_add_ps(turbulenceResult, noiseDivRatio);
        } else {
            // abs(noise) using SSE: clear sign bit
            // Create mask with all bits set except sign bit
            __m128 absMask = _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF));
            __m128 absNoise = _mm_and_ps(noiseVec, absMask);

            // turbulenceResult += abs(noise) / ratio
            __m128 noiseDivRatio = _mm_div_ps(absNoise, ratioVec);
            turbulenceResult = _mm_add_ps(turbulenceResult, noiseDivRatio);
        }

        noiseVector.setX(noiseVector.x() * 2);
        noiseVector.setY(noiseVector.y() * 2);
        ratio *= 2;

        if (paintingData.stitchTiles) {
            stitchData.width *= 2;
            stitchData.wrapX = 2 * stitchData.wrapX - s_perlinNoise;
            stitchData.height *= 2;
            stitchData.wrapY = 2 * stitchData.wrapY - s_perlinNoise;
        }
    }

    // Apply fractalNoise adjustment: result * 0.5 + 0.5
    if (paintingData.type ==
        SVGFETurbulenceElement::SVG_TURBULENCE_TYPE_FRACTALNOISE) {
        __m128 half = _mm_set1_ps(0.5f);
        turbulenceResult = _mm_add_ps(_mm_mul_ps(turbulenceResult, half), half);
    }

    // Write directly to buffer - pass SIMD register directly
    toIntBasedColorComponents(turbulenceResult, buffer);
}

#elif defined(STARFISH_ARM64)
// NEON implementation for ARM64 (ARMv8)
// vdivq_f32 is only available on ARM64, not on ARM32 (ARMv7)
// noise2D returns float32x4_t directly - no memory conversion needed!
ALWAYS_INLINE void FilterTurbulence::calculateTurbulenceValueForPoint(
    const PaintingData& paintingData, StitchData stitchData,
    const Unit::FloatPoint& point, uint8_t* buffer)
{
    // Initialize turbulenceFunctionResult to zero using NEON
    float32x4_t turbulenceResult = vdupq_n_f32(0.0f);

    Unit::FloatPoint noiseVector(point.x() * paintingData.baseFrequencyX,
                                 point.y() * paintingData.baseFrequencyY);
    float ratio = 1;

    for (int octave = 0; octave < paintingData.numOctaves; ++octave) {
        // noise2D returns float32x4_t directly - no memory store/load!
        float32x4_t noiseVec = noise2D(paintingData, stitchData, noiseVector);

        // Broadcast ratio
        float32x4_t ratioVec = vdupq_n_f32(ratio);

        if (paintingData.type ==
            SVGFETurbulenceElement::SVG_TURBULENCE_TYPE_FRACTALNOISE) {
            // turbulenceResult += noise / ratio
            float32x4_t noiseDivRatio = vdivq_f32(noiseVec, ratioVec);
            turbulenceResult = vaddq_f32(turbulenceResult, noiseDivRatio);
        } else {
            // abs(noise) using NEON: vabsq_f32
            float32x4_t absNoise = vabsq_f32(noiseVec);

            // turbulenceResult += abs(noise) / ratio
            float32x4_t noiseDivRatio = vdivq_f32(absNoise, ratioVec);
            turbulenceResult = vaddq_f32(turbulenceResult, noiseDivRatio);
        }

        noiseVector.setX(noiseVector.x() * 2);
        noiseVector.setY(noiseVector.y() * 2);
        ratio *= 2;

        if (paintingData.stitchTiles) {
            stitchData.width *= 2;
            stitchData.wrapX = 2 * stitchData.wrapX - s_perlinNoise;
            stitchData.height *= 2;
            stitchData.wrapY = 2 * stitchData.wrapY - s_perlinNoise;
        }
    }

    // Apply fractalNoise adjustment: result * 0.5 + 0.5
    if (paintingData.type ==
        SVGFETurbulenceElement::SVG_TURBULENCE_TYPE_FRACTALNOISE) {
        float32x4_t half = vdupq_n_f32(0.5f);
        turbulenceResult = vaddq_f32(vmulq_f32(turbulenceResult, half), half);
    }

    // Write directly to buffer - pass SIMD register directly
    toIntBasedColorComponents(turbulenceResult, buffer);
}

#elif defined(STARFISH_ARM) && defined(STARFISH_ARM_NEON)
// NEON implementation for ARM32 (ARMv7)
// vdivq_f32 is NOT available on ARM32, use reciprocal approximation instead
// noise2D returns float32x4_t directly - no memory conversion needed!
ALWAYS_INLINE void FilterTurbulence::calculateTurbulenceValueForPoint(
    const PaintingData& paintingData, StitchData stitchData,
    const Unit::FloatPoint& point, uint8_t* buffer)
{
    // Initialize turbulenceFunctionResult to zero using NEON
    float32x4_t turbulenceResult = vdupq_n_f32(0.0f);

    Unit::FloatPoint noiseVector(point.x() * paintingData.baseFrequencyX,
                                 point.y() * paintingData.baseFrequencyY);
    float ratio = 1;

    for (int octave = 0; octave < paintingData.numOctaves; ++octave) {
        // noise2D returns float32x4_t directly - no memory store/load!
        float32x4_t noiseVec = noise2D(paintingData, stitchData, noiseVector);

        // Broadcast ratio
        float32x4_t ratioVec = vdupq_n_f32(ratio);

        // ARM32: Use reciprocal approximation for division
        // recip = vrecpeq_f32(ratioVec) gives an approximate reciprocal
        // One Newton-Raphson iteration for better accuracy: recip =
        // vrecpsq_f32(ratioVec, recip) * recip
        float32x4_t recip = vrecpeq_f32(ratioVec);
        recip = vmulq_f32(recip, vrecpsq_f32(ratioVec, recip));

        if (paintingData.type ==
            SVGFETurbulenceElement::SVG_TURBULENCE_TYPE_FRACTALNOISE) {
            // turbulenceResult += noise / ratio = noise * recip
            float32x4_t noiseDivRatio = vmulq_f32(noiseVec, recip);
            turbulenceResult = vaddq_f32(turbulenceResult, noiseDivRatio);
        } else {
            // abs(noise) using NEON: vabsq_f32
            float32x4_t absNoise = vabsq_f32(noiseVec);

            // turbulenceResult += abs(noise) / ratio = abs(noise) * recip
            float32x4_t noiseDivRatio = vmulq_f32(absNoise, recip);
            turbulenceResult = vaddq_f32(turbulenceResult, noiseDivRatio);
        }

        noiseVector.setX(noiseVector.x() * 2);
        noiseVector.setY(noiseVector.y() * 2);
        ratio *= 2;

        if (paintingData.stitchTiles) {
            stitchData.width *= 2;
            stitchData.wrapX = 2 * stitchData.wrapX - s_perlinNoise;
            stitchData.height *= 2;
            stitchData.wrapY = 2 * stitchData.wrapY - s_perlinNoise;
        }
    }

    // Apply fractalNoise adjustment: result * 0.5 + 0.5
    if (paintingData.type ==
        SVGFETurbulenceElement::SVG_TURBULENCE_TYPE_FRACTALNOISE) {
        float32x4_t half = vdupq_n_f32(0.5f);
        turbulenceResult = vaddq_f32(vmulq_f32(turbulenceResult, half), half);
    }

    // Write directly to buffer - pass SIMD register directly
    toIntBasedColorComponents(turbulenceResult, buffer);
}

#else
// Fallback C implementation for other architectures
ALWAYS_INLINE void FilterTurbulence::calculateTurbulenceValueForPoint(
    const PaintingData& paintingData, StitchData stitchData,
    const Unit::FloatPoint& point, uint8_t* buffer)
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

    toIntBasedColorComponents(turbulenceFunctionResult, buffer);
}
#endif

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
        xposition = xposition + ctx.width * normalizedSubRegion.x();
        width = xposition + ctx.width * normalizedSubRegion.width();
        yposition = yposition + ctx.height * normalizedSubRegion.y();
        height = yposition + ctx.height * normalizedSubRegion.height();
    }

    auto stitchData = computeStitching(
        Unit::IntSize(width / ctx.viewportScaleX, height / ctx.viewportScaleY),
        baseFrequencyX, baseFrequencyY,
        e->stitchTiles()->animVal() ==
            SVGFETurbulenceElement::SVG_STITCHTYPE_STITCH);
    auto paintingData = initPaintingData(
        e->type()->animVal(), baseFrequencyX, baseFrequencyY,
        e->numOctaves()->animVal(), e->seed()->animVal(),
        e->stitchTiles()->animVal() ==
            SVGFETurbulenceElement::SVG_STITCHTYPE_STITCH,
        Unit::IntSize(width / ctx.viewportScaleX, height / ctx.viewportScaleY));

    unsigned char* data = (unsigned char*)outputSource->data();
    if (height * width > 100 * 100 && height > 20) {
        WebView* webView = e->webView();
        struct Params {
            unsigned char* data;
            size_t width;
            size_t startY;
            size_t endY;
            size_t stride;
            size_t xposition;
            size_t yposition;
            FilterTurbulence::StitchData* stitchData;
            FilterTurbulence::PaintingData* paintingData;
            Filter::FilterApplyContext* ctx;
        };

        auto worker = [](void* data) -> void* {
            auto params = (Params*)data;
            auto buffer = params->data;
            for (uint y = params->startY; y < params->endY; y++) {
                for (uint x = 0; x < params->width; x++) {
                    int offset = y * params->stride + x * 4;
                    calculateTurbulenceValueForPoint(
                        *params->paintingData, *params->stitchData,
                        Unit::FloatPoint((x + params->xposition) /
                                             params->ctx->viewportScaleX,
                                         (y + params->yposition) /
                                             params->ctx->viewportScaleY),
                        buffer + offset);
                }
            }
            return nullptr;
        };

        ParallelJobExecutor<Params>* parallelJobExecutor =
            new ParallelJobExecutor<Params>(
                webView, worker, std::min(size_t(6), numberOfCores()));

        size_t num = parallelJobExecutor->numberOfThread();

        const size_t blockHeight = height / num;
        const size_t jobsWithExtra = height % num;
        size_t currentY = 0;
        for (size_t i = 0; i < num; ++i) {
            auto& params = parallelJobExecutor->parameters(i);

            size_t startY = !i ? 0 : currentY;
            currentY += blockHeight;
            size_t endY = i == num - 1 ? height : currentY;

            params.width = width;
            params.stride = ctx.stride;
            params.startY = startY;
            params.endY = endY;
            params.data = data;
            params.xposition = xposition;
            params.yposition = yposition;
            params.stitchData = &stitchData;
            params.paintingData = &paintingData;
            params.ctx = &ctx;
        }

        parallelJobExecutor->execute();
    } else {
        // single threaded
        for (uint y = 0; y < height; y++) {
            for (uint x = 0; x < width; x++) {
                int offset = y * ctx.stride + x * 4;
                calculateTurbulenceValueForPoint(
                    paintingData, stitchData,
                    Unit::FloatPoint((x + xposition) / ctx.viewportScaleX,
                                     (y + yposition) / ctx.viewportScaleY),
                    data + offset);
            }
        }
    }
    // Note: Premultiplied alpha conversion is now integrated into
    // toIntBasedColorComponents via PORT_CANVAS_NEEDS_PREMULTIPLIED_ALPHA

    filter()->registerOutput(ctx, this, outputSource);
}

} // namespace Starfish
