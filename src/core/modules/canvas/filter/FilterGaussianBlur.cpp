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

#include "core/dom/svg/SVGFEGaussianBlurElement.h"
#include "core/modules/canvas/filter/Filter.h"
#include "core/modules/canvas/filter/FilterGaussianBlur.h"
#include "core/page/WebView.h"
#include "core/layout/svg/FrameSVGBox.h"
#include "core/layout/svg/FrameSVGSVGBox.h"

#if defined(STARFISH_X86) || defined(STARFISH_X86_64)
#include <emmintrin.h>
#elif defined(STARFISH_ARM_NEON)
#include <arm_neon.h>
#endif

namespace Starfish {

inline void kernelPosition(int blurIteration, unsigned& radius, int& deltaLeft,
                           int& deltaRight)
{
    // Check http://www.w3.org/TR/SVG/filters.html#feGaussianBlurElement for
    // details.
    switch (blurIteration) {
    case 0:
        if (!(radius % 2)) {
            deltaLeft = radius / 2 - 1;
            deltaRight = radius - deltaLeft;
        } else {
            deltaLeft = radius / 2;
            deltaRight = radius - deltaLeft;
        }
        break;
    case 1:
        if (!(radius % 2)) {
            deltaLeft++;
            deltaRight--;
        }
        break;
    case 2:
        if (!(radius % 2)) {
            deltaRight++;
            radius++;
        }
        break;
    }
}

inline void boxBlurAlphaOnly(uint8_t* srcData, uint8_t* dstData,
                             const unsigned dx, const int dxLeft,
                             const int dxRight, const int stride,
                             const int strideLine, const int effectWidth,
                             const int effectHeight, const int maxKernelSize)
{
    // Memory alignment is: RGBA, zero-index based.
    const int channel = 3;

    for (int y = 0; y < effectHeight; ++y) {
        int line = y * strideLine;
        int sum = 0;

        // Fill the kernel.
        for (int i = 0; i < maxKernelSize; ++i) {
            unsigned offset = line + i * stride;
            const uint8_t* srcPtr = srcData + offset;
            sum += srcPtr[channel];
        }

        // Blurring.
        for (int x = 0; x < effectWidth; ++x) {
            unsigned pixelByteOffset = line + x * stride + channel;
            uint8_t* dstPtr = dstData + pixelByteOffset;
            *dstPtr = static_cast<uint8_t>(sum / dx);

            // Shift kernel.
            if (x >= dxLeft) {
                unsigned leftOffset = pixelByteOffset - dxLeft * stride;
                const uint8_t* srcPtr = srcData + leftOffset;
                sum -= *srcPtr;
            }

            if (x + dxRight < effectWidth) {
                unsigned rightOffset = pixelByteOffset + dxRight * stride;
                const uint8_t* srcPtr = srcData + rightOffset;
                sum += *srcPtr;
            }
        }
    }
}

#if defined(STARFISH_X86)
// SSE2 SIMD implementation for x86 (32-bit)
inline void boxBlurSIMD(uint8_t* srcData, uint8_t* dstData, unsigned dx,
                        int dxLeft, int dxRight, int stride, int strideLine,
                        int effectWidth, int effectHeight, bool alphaImage,
                        SVGFEGaussianBlurElement::EdgeMode edgeMode)
{
    const int maxKernelSize = std::min(dxRight, effectWidth);

    if (alphaImage) {
        return boxBlurAlphaOnly(srcData, dstData, dx, dxLeft, dxRight, stride,
                                strideLine, effectWidth, effectHeight,
                                maxKernelSize);
    }

    for (int y = 0; y < effectHeight; ++y) {
        int line = y * strideLine;
        __m128i vsum = _mm_setzero_si128();

        if (edgeMode == SVGFEGaussianBlurElement::EdgeMode::SVG_EDGEMODE_NONE) {
            for (int i = 0; i < maxKernelSize; ++i) {
                unsigned offset = line + i * stride;
                const uint8_t* srcPtr = srcData + offset;
                __m128i pixel = _mm_cvtsi32_si128(
                    *reinterpret_cast<const int32_t*>(srcPtr));
                __m128i pixel16 = _mm_unpacklo_epi8(pixel, _mm_setzero_si128());
                __m128i pixel32 =
                    _mm_unpacklo_epi16(pixel16, _mm_setzero_si128());
                vsum = _mm_add_epi32(vsum, pixel32);
            }

            for (int x = 0; x < effectWidth; ++x) {
                unsigned pixelByteOffset = line + x * stride;
                uint8_t* dstPtr = dstData + pixelByteOffset;

                // Extract and divide each channel (SSE2 doesn't have
                // _mm_div_epi32)
                int sumR = _mm_cvtsi128_si32(_mm_shuffle_epi32(
                    vsum, _MM_SHUFFLE(0, 0, 0, STARFISH_PIXEL_R_INDEX)));
                int sumG = _mm_cvtsi128_si32(_mm_shuffle_epi32(
                    vsum, _MM_SHUFFLE(0, 0, 0, STARFISH_PIXEL_G_INDEX)));
                int sumB = _mm_cvtsi128_si32(_mm_shuffle_epi32(
                    vsum, _MM_SHUFFLE(0, 0, 0, STARFISH_PIXEL_B_INDEX)));
                int sumA = _mm_cvtsi128_si32(_mm_shuffle_epi32(
                    vsum, _MM_SHUFFLE(0, 0, 0, STARFISH_PIXEL_A_INDEX)));
                dstPtr[STARFISH_PIXEL_R_INDEX] =
                    static_cast<uint8_t>(sumR / static_cast<int>(dx));
                dstPtr[STARFISH_PIXEL_G_INDEX] =
                    static_cast<uint8_t>(sumG / static_cast<int>(dx));
                dstPtr[STARFISH_PIXEL_B_INDEX] =
                    static_cast<uint8_t>(sumB / static_cast<int>(dx));
                dstPtr[STARFISH_PIXEL_A_INDEX] =
                    static_cast<uint8_t>(sumA / static_cast<int>(dx));

                if (x >= dxLeft) {
                    unsigned leftOffset = pixelByteOffset - dxLeft * stride;
                    const uint8_t* srcPtr = srcData + leftOffset;
                    __m128i pixel = _mm_cvtsi32_si128(
                        *reinterpret_cast<const int32_t*>(srcPtr));
                    __m128i pixel16 =
                        _mm_unpacklo_epi8(pixel, _mm_setzero_si128());
                    __m128i pixel32 =
                        _mm_unpacklo_epi16(pixel16, _mm_setzero_si128());
                    vsum = _mm_sub_epi32(vsum, pixel32);
                }

                if (x + dxRight < effectWidth) {
                    unsigned rightOffset = pixelByteOffset + dxRight * stride;
                    const uint8_t* srcPtr = srcData + rightOffset;
                    __m128i pixel = _mm_cvtsi32_si128(
                        *reinterpret_cast<const int32_t*>(srcPtr));
                    __m128i pixel16 =
                        _mm_unpacklo_epi8(pixel, _mm_setzero_si128());
                    __m128i pixel32 =
                        _mm_unpacklo_epi16(pixel16, _mm_setzero_si128());
                    vsum = _mm_add_epi32(vsum, pixel32);
                }
            }
        } else {
            const uint8_t* edgeValueLeft = srcData + line;
            const uint8_t* edgeValueRight =
                srcData + (line + (effectWidth - 1) * stride);

            for (int i = dxLeft * -1; i < dxRight; ++i) {
                unsigned offset = line + i * stride;
                const uint8_t* srcPtr = srcData + offset;
                __m128i pixel;

                if (i < 0) {
                    pixel = _mm_cvtsi32_si128(
                        *reinterpret_cast<const int32_t*>(edgeValueLeft));
                } else if (i >= effectWidth) {
                    pixel = _mm_cvtsi32_si128(
                        *reinterpret_cast<const int32_t*>(edgeValueRight));
                } else {
                    pixel = _mm_cvtsi32_si128(
                        *reinterpret_cast<const int32_t*>(srcPtr));
                }
                __m128i pixel16 = _mm_unpacklo_epi8(pixel, _mm_setzero_si128());
                __m128i pixel32 =
                    _mm_unpacklo_epi16(pixel16, _mm_setzero_si128());
                vsum = _mm_add_epi32(vsum, pixel32);
            }

            for (int x = 0; x < effectWidth; ++x) {
                unsigned pixelByteOffset = line + x * stride;
                uint8_t* dstPtr = dstData + pixelByteOffset;

                // Extract and divide each channel (SSE2 doesn't have
                // _mm_div_epi32)
                int sumR = _mm_cvtsi128_si32(_mm_shuffle_epi32(
                    vsum, _MM_SHUFFLE(0, 0, 0, STARFISH_PIXEL_R_INDEX)));
                int sumG = _mm_cvtsi128_si32(_mm_shuffle_epi32(
                    vsum, _MM_SHUFFLE(0, 0, 0, STARFISH_PIXEL_G_INDEX)));
                int sumB = _mm_cvtsi128_si32(_mm_shuffle_epi32(
                    vsum, _MM_SHUFFLE(0, 0, 0, STARFISH_PIXEL_B_INDEX)));
                int sumA = _mm_cvtsi128_si32(_mm_shuffle_epi32(
                    vsum, _MM_SHUFFLE(0, 0, 0, STARFISH_PIXEL_A_INDEX)));
                dstPtr[STARFISH_PIXEL_R_INDEX] =
                    static_cast<uint8_t>(sumR / static_cast<int>(dx));
                dstPtr[STARFISH_PIXEL_G_INDEX] =
                    static_cast<uint8_t>(sumG / static_cast<int>(dx));
                dstPtr[STARFISH_PIXEL_B_INDEX] =
                    static_cast<uint8_t>(sumB / static_cast<int>(dx));
                dstPtr[STARFISH_PIXEL_A_INDEX] =
                    static_cast<uint8_t>(sumA / static_cast<int>(dx));

                __m128i leftPixel;
                if (x < dxLeft) {
                    leftPixel = _mm_cvtsi32_si128(
                        *reinterpret_cast<const int32_t*>(edgeValueLeft));
                } else {
                    unsigned leftOffset = pixelByteOffset - dxLeft * stride;
                    const uint8_t* srcPtr = srcData + leftOffset;
                    leftPixel = _mm_cvtsi32_si128(
                        *reinterpret_cast<const int32_t*>(srcPtr));
                }
                __m128i leftPixel16 =
                    _mm_unpacklo_epi8(leftPixel, _mm_setzero_si128());
                __m128i leftPixel32 =
                    _mm_unpacklo_epi16(leftPixel16, _mm_setzero_si128());
                vsum = _mm_sub_epi32(vsum, leftPixel32);

                __m128i rightPixel;
                if (x + dxRight >= effectWidth) {
                    rightPixel = _mm_cvtsi32_si128(
                        *reinterpret_cast<const int32_t*>(edgeValueRight));
                } else {
                    unsigned rightOffset = pixelByteOffset + dxRight * stride;
                    const uint8_t* srcPtr = srcData + rightOffset;
                    rightPixel = _mm_cvtsi32_si128(
                        *reinterpret_cast<const int32_t*>(srcPtr));
                }
                __m128i rightPixel16 =
                    _mm_unpacklo_epi8(rightPixel, _mm_setzero_si128());
                __m128i rightPixel32 =
                    _mm_unpacklo_epi16(rightPixel16, _mm_setzero_si128());
                vsum = _mm_add_epi32(vsum, rightPixel32);
            }
        }
    }
}
#elif defined(STARFISH_X86_64)
inline void boxBlurSIMD(uint8_t* srcData, uint8_t* dstData, unsigned dx,
                        int dxLeft, int dxRight, int stride, int strideLine,
                        int effectWidth, int effectHeight, bool alphaImage,
                        SVGFEGaussianBlurElement::EdgeMode edgeMode)
{
    const int maxKernelSize = std::min(dxRight, effectWidth);

    if (alphaImage) {
        return boxBlurAlphaOnly(srcData, dstData, dx, dxLeft, dxRight, stride,
                                strideLine, effectWidth, effectHeight,
                                maxKernelSize);
    }

    __m128i vdx = _mm_set1_epi32(static_cast<int>(dx));

    for (int y = 0; y < effectHeight; ++y) {
        int line = y * strideLine;
        __m128i vsum = _mm_setzero_si128();

        if (edgeMode == SVGFEGaussianBlurElement::EdgeMode::SVG_EDGEMODE_NONE) {
            for (int i = 0; i < maxKernelSize; ++i) {
                unsigned offset = line + i * stride;
                const uint8_t* srcPtr = srcData + offset;
                __m128i pixel = _mm_cvtsi32_si128(
                    *reinterpret_cast<const int32_t*>(srcPtr));
                __m128i pixel16 = _mm_unpacklo_epi8(pixel, _mm_setzero_si128());
                __m128i pixel32 =
                    _mm_unpacklo_epi16(pixel16, _mm_setzero_si128());
                vsum = _mm_add_epi32(vsum, pixel32);
            }

            for (int x = 0; x < effectWidth; ++x) {
                unsigned pixelByteOffset = line + x * stride;
                uint8_t* dstPtr = dstData + pixelByteOffset;

                // Extract and divide each channel (SSE2 doesn't have
                // _mm_div_epi32)
                int sumR = _mm_cvtsi128_si32(_mm_shuffle_epi32(
                    vsum, _MM_SHUFFLE(0, 0, 0, STARFISH_PIXEL_R_INDEX)));
                int sumG = _mm_cvtsi128_si32(_mm_shuffle_epi32(
                    vsum, _MM_SHUFFLE(0, 0, 0, STARFISH_PIXEL_G_INDEX)));
                int sumB = _mm_cvtsi128_si32(_mm_shuffle_epi32(
                    vsum, _MM_SHUFFLE(0, 0, 0, STARFISH_PIXEL_B_INDEX)));
                int sumA = _mm_cvtsi128_si32(_mm_shuffle_epi32(
                    vsum, _MM_SHUFFLE(0, 0, 0, STARFISH_PIXEL_A_INDEX)));
                dstPtr[STARFISH_PIXEL_R_INDEX] =
                    static_cast<uint8_t>(sumR / static_cast<int>(dx));
                dstPtr[STARFISH_PIXEL_G_INDEX] =
                    static_cast<uint8_t>(sumG / static_cast<int>(dx));
                dstPtr[STARFISH_PIXEL_B_INDEX] =
                    static_cast<uint8_t>(sumB / static_cast<int>(dx));
                dstPtr[STARFISH_PIXEL_A_INDEX] =
                    static_cast<uint8_t>(sumA / static_cast<int>(dx));

                if (x >= dxLeft) {
                    unsigned leftOffset = pixelByteOffset - dxLeft * stride;
                    const uint8_t* srcPtr = srcData + leftOffset;
                    __m128i pixel = _mm_cvtsi32_si128(
                        *reinterpret_cast<const int32_t*>(srcPtr));
                    __m128i pixel16 =
                        _mm_unpacklo_epi8(pixel, _mm_setzero_si128());
                    __m128i pixel32 =
                        _mm_unpacklo_epi16(pixel16, _mm_setzero_si128());
                    vsum = _mm_sub_epi32(vsum, pixel32);
                }

                if (x + dxRight < effectWidth) {
                    unsigned rightOffset = pixelByteOffset + dxRight * stride;
                    const uint8_t* srcPtr = srcData + rightOffset;
                    __m128i pixel = _mm_cvtsi32_si128(
                        *reinterpret_cast<const int32_t*>(srcPtr));
                    __m128i pixel16 =
                        _mm_unpacklo_epi8(pixel, _mm_setzero_si128());
                    __m128i pixel32 =
                        _mm_unpacklo_epi16(pixel16, _mm_setzero_si128());
                    vsum = _mm_add_epi32(vsum, pixel32);
                }
            }
        } else {
            const uint8_t* edgeValueLeft = srcData + line;
            const uint8_t* edgeValueRight =
                srcData + (line + (effectWidth - 1) * stride);

            for (int i = dxLeft * -1; i < dxRight; ++i) {
                unsigned offset = line + i * stride;
                const uint8_t* srcPtr = srcData + offset;
                __m128i pixel;

                if (i < 0) {
                    pixel = _mm_cvtsi32_si128(
                        *reinterpret_cast<const int32_t*>(edgeValueLeft));
                } else if (i >= effectWidth) {
                    pixel = _mm_cvtsi32_si128(
                        *reinterpret_cast<const int32_t*>(edgeValueRight));
                } else {
                    pixel = _mm_cvtsi32_si128(
                        *reinterpret_cast<const int32_t*>(srcPtr));
                }
                __m128i pixel16 = _mm_unpacklo_epi8(pixel, _mm_setzero_si128());
                __m128i pixel32 =
                    _mm_unpacklo_epi16(pixel16, _mm_setzero_si128());
                vsum = _mm_add_epi32(vsum, pixel32);
            }

            for (int x = 0; x < effectWidth; ++x) {
                unsigned pixelByteOffset = line + x * stride;
                uint8_t* dstPtr = dstData + pixelByteOffset;

                // Extract and divide each channel (SSE2 doesn't have
                // _mm_div_epi32)
                int sumR = _mm_cvtsi128_si32(_mm_shuffle_epi32(
                    vsum, _MM_SHUFFLE(0, 0, 0, STARFISH_PIXEL_R_INDEX)));
                int sumG = _mm_cvtsi128_si32(_mm_shuffle_epi32(
                    vsum, _MM_SHUFFLE(0, 0, 0, STARFISH_PIXEL_G_INDEX)));
                int sumB = _mm_cvtsi128_si32(_mm_shuffle_epi32(
                    vsum, _MM_SHUFFLE(0, 0, 0, STARFISH_PIXEL_B_INDEX)));
                int sumA = _mm_cvtsi128_si32(_mm_shuffle_epi32(
                    vsum, _MM_SHUFFLE(0, 0, 0, STARFISH_PIXEL_A_INDEX)));
                dstPtr[STARFISH_PIXEL_R_INDEX] =
                    static_cast<uint8_t>(sumR / static_cast<int>(dx));
                dstPtr[STARFISH_PIXEL_G_INDEX] =
                    static_cast<uint8_t>(sumG / static_cast<int>(dx));
                dstPtr[STARFISH_PIXEL_B_INDEX] =
                    static_cast<uint8_t>(sumB / static_cast<int>(dx));
                dstPtr[STARFISH_PIXEL_A_INDEX] =
                    static_cast<uint8_t>(sumA / static_cast<int>(dx));

                __m128i leftPixel;
                if (x < dxLeft) {
                    leftPixel = _mm_cvtsi32_si128(
                        *reinterpret_cast<const int32_t*>(edgeValueLeft));
                } else {
                    unsigned leftOffset = pixelByteOffset - dxLeft * stride;
                    const uint8_t* srcPtr = srcData + leftOffset;
                    leftPixel = _mm_cvtsi32_si128(
                        *reinterpret_cast<const int32_t*>(srcPtr));
                }
                __m128i leftPixel16 =
                    _mm_unpacklo_epi8(leftPixel, _mm_setzero_si128());
                __m128i leftPixel32 =
                    _mm_unpacklo_epi16(leftPixel16, _mm_setzero_si128());
                vsum = _mm_sub_epi32(vsum, leftPixel32);

                __m128i rightPixel;
                if (x + dxRight >= effectWidth) {
                    rightPixel = _mm_cvtsi32_si128(
                        *reinterpret_cast<const int32_t*>(edgeValueRight));
                } else {
                    unsigned rightOffset = pixelByteOffset + dxRight * stride;
                    const uint8_t* srcPtr = srcData + rightOffset;
                    rightPixel = _mm_cvtsi32_si128(
                        *reinterpret_cast<const int32_t*>(srcPtr));
                }
                __m128i rightPixel16 =
                    _mm_unpacklo_epi8(rightPixel, _mm_setzero_si128());
                __m128i rightPixel32 =
                    _mm_unpacklo_epi16(rightPixel16, _mm_setzero_si128());
                vsum = _mm_add_epi32(vsum, rightPixel32);
            }
        }
    }
}
#elif defined(STARFISH_ARM) && defined(STARFISH_ARM_NEON)
inline void boxBlurSIMD(uint8_t* srcData, uint8_t* dstData, unsigned dx,
                        int dxLeft, int dxRight, int stride, int strideLine,
                        int effectWidth, int effectHeight, bool alphaImage,
                        SVGFEGaussianBlurElement::EdgeMode edgeMode)
{
    const int maxKernelSize = std::min(dxRight, effectWidth);

    if (alphaImage) {
        return boxBlurAlphaOnly(srcData, dstData, dx, dxLeft, dxRight, stride,
                                strideLine, effectWidth, effectHeight,
                                maxKernelSize);
    }

    int32x4_t vdx = vdupq_n_s32(static_cast<int>(dx));

    for (int y = 0; y < effectHeight; ++y) {
        int line = y * strideLine;
        int32x4_t vsum = vdupq_n_s32(0);

        if (edgeMode == SVGFEGaussianBlurElement::EdgeMode::SVG_EDGEMODE_NONE) {
            for (int i = 0; i < maxKernelSize; ++i) {
                unsigned offset = line + i * stride;
                const uint8_t* srcPtr = srcData + offset;
                uint8x8_t pixel8 = vld1_u8(srcPtr);
                uint16x8_t pixel16 = vmovl_u8(pixel8);
                int32x4_t pixel32 =
                    vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(pixel16)));
                vsum = vaddq_s32(vsum, pixel32);
            }

            for (int x = 0; x < effectWidth; ++x) {
                unsigned pixelByteOffset = line + x * stride;
                uint8_t* dstPtr = dstData + pixelByteOffset;

                int sumR = vgetq_lane_s32(vsum, STARFISH_PIXEL_R_INDEX);
                int sumG = vgetq_lane_s32(vsum, STARFISH_PIXEL_G_INDEX);
                int sumB = vgetq_lane_s32(vsum, STARFISH_PIXEL_B_INDEX);
                int sumA = vgetq_lane_s32(vsum, STARFISH_PIXEL_A_INDEX);
                dstPtr[STARFISH_PIXEL_R_INDEX] =
                    static_cast<uint8_t>(sumR / static_cast<int>(dx));
                dstPtr[STARFISH_PIXEL_G_INDEX] =
                    static_cast<uint8_t>(sumG / static_cast<int>(dx));
                dstPtr[STARFISH_PIXEL_B_INDEX] =
                    static_cast<uint8_t>(sumB / static_cast<int>(dx));
                dstPtr[STARFISH_PIXEL_A_INDEX] =
                    static_cast<uint8_t>(sumA / static_cast<int>(dx));

                if (x >= dxLeft) {
                    unsigned leftOffset = pixelByteOffset - dxLeft * stride;
                    const uint8_t* srcPtr = srcData + leftOffset;
                    uint8x8_t pixel8 = vld1_u8(srcPtr);
                    uint16x8_t pixel16 = vmovl_u8(pixel8);
                    int32x4_t pixel32 =
                        vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(pixel16)));
                    vsum = vsubq_s32(vsum, pixel32);
                }

                if (x + dxRight < effectWidth) {
                    unsigned rightOffset = pixelByteOffset + dxRight * stride;
                    const uint8_t* srcPtr = srcData + rightOffset;
                    uint8x8_t pixel8 = vld1_u8(srcPtr);
                    uint16x8_t pixel16 = vmovl_u8(pixel8);
                    int32x4_t pixel32 =
                        vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(pixel16)));
                    vsum = vaddq_s32(vsum, pixel32);
                }
            }
        } else {
            const uint8_t* edgeValueLeft = srcData + line;
            const uint8_t* edgeValueRight =
                srcData + (line + (effectWidth - 1) * stride);

            for (int i = dxLeft * -1; i < dxRight; ++i) {
                unsigned offset = line + i * stride;
                const uint8_t* srcPtr = srcData + offset;
                uint8x8_t pixel8;

                if (i < 0) {
                    pixel8 = vld1_u8(edgeValueLeft);
                } else if (i >= effectWidth) {
                    pixel8 = vld1_u8(edgeValueRight);
                } else {
                    pixel8 = vld1_u8(srcPtr);
                }
                uint16x8_t pixel16 = vmovl_u8(pixel8);
                int32x4_t pixel32 =
                    vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(pixel16)));
                vsum = vaddq_s32(vsum, pixel32);
            }

            for (int x = 0; x < effectWidth; ++x) {
                unsigned pixelByteOffset = line + x * stride;
                uint8_t* dstPtr = dstData + pixelByteOffset;

                int sumR = vgetq_lane_s32(vsum, STARFISH_PIXEL_R_INDEX);
                int sumG = vgetq_lane_s32(vsum, STARFISH_PIXEL_G_INDEX);
                int sumB = vgetq_lane_s32(vsum, STARFISH_PIXEL_B_INDEX);
                int sumA = vgetq_lane_s32(vsum, STARFISH_PIXEL_A_INDEX);
                dstPtr[STARFISH_PIXEL_R_INDEX] =
                    static_cast<uint8_t>(sumR / static_cast<int>(dx));
                dstPtr[STARFISH_PIXEL_G_INDEX] =
                    static_cast<uint8_t>(sumG / static_cast<int>(dx));
                dstPtr[STARFISH_PIXEL_B_INDEX] =
                    static_cast<uint8_t>(sumB / static_cast<int>(dx));
                dstPtr[STARFISH_PIXEL_A_INDEX] =
                    static_cast<uint8_t>(sumA / static_cast<int>(dx));

                int32x4_t leftPixel;
                if (x < dxLeft) {
                    uint8x8_t pixel8 = vld1_u8(edgeValueLeft);
                    uint16x8_t pixel16 = vmovl_u8(pixel8);
                    leftPixel =
                        vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(pixel16)));
                } else {
                    unsigned leftOffset = pixelByteOffset - dxLeft * stride;
                    const uint8_t* srcPtr = srcData + leftOffset;
                    uint8x8_t pixel8 = vld1_u8(srcPtr);
                    uint16x8_t pixel16 = vmovl_u8(pixel8);
                    leftPixel =
                        vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(pixel16)));
                }
                vsum = vsubq_s32(vsum, leftPixel);

                int32x4_t rightPixel;
                if (x + dxRight >= effectWidth) {
                    uint8x8_t pixel8 = vld1_u8(edgeValueRight);
                    uint16x8_t pixel16 = vmovl_u8(pixel8);
                    rightPixel =
                        vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(pixel16)));
                } else {
                    unsigned rightOffset = pixelByteOffset + dxRight * stride;
                    const uint8_t* srcPtr = srcData + rightOffset;
                    uint8x8_t pixel8 = vld1_u8(srcPtr);
                    uint16x8_t pixel16 = vmovl_u8(pixel8);
                    rightPixel =
                        vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(pixel16)));
                }
                vsum = vaddq_s32(vsum, rightPixel);
            }
        }
    }
}
#elif defined(STARFISH_ARM64) && defined(STARFISH_ARM_NEON)
inline void boxBlurSIMD(uint8_t* srcData, uint8_t* dstData, unsigned dx,
                        int dxLeft, int dxRight, int stride, int strideLine,
                        int effectWidth, int effectHeight, bool alphaImage,
                        SVGFEGaussianBlurElement::EdgeMode edgeMode)
{
    const int maxKernelSize = std::min(dxRight, effectWidth);

    if (alphaImage) {
        return boxBlurAlphaOnly(srcData, dstData, dx, dxLeft, dxRight, stride,
                                strideLine, effectWidth, effectHeight,
                                maxKernelSize);
    }

    int32x4_t vdx = vdupq_n_s32(static_cast<int>(dx));

    for (int y = 0; y < effectHeight; ++y) {
        int line = y * strideLine;
        int32x4_t vsum = vdupq_n_s32(0);

        if (edgeMode == SVGFEGaussianBlurElement::EdgeMode::SVG_EDGEMODE_NONE) {
            for (int i = 0; i < maxKernelSize; ++i) {
                unsigned offset = line + i * stride;
                const uint8_t* srcPtr = srcData + offset;
                uint8x8_t pixel8 = vld1_u8(srcPtr);
                uint16x8_t pixel16 = vmovl_u8(pixel8);
                int32x4_t pixel32 =
                    vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(pixel16)));
                vsum = vaddq_s32(vsum, pixel32);
            }

            for (int x = 0; x < effectWidth; ++x) {
                unsigned pixelByteOffset = line + x * stride;
                uint8_t* dstPtr = dstData + pixelByteOffset;

                int sumR = vgetq_lane_s32(vsum, STARFISH_PIXEL_R_INDEX);
                int sumG = vgetq_lane_s32(vsum, STARFISH_PIXEL_G_INDEX);
                int sumB = vgetq_lane_s32(vsum, STARFISH_PIXEL_B_INDEX);
                int sumA = vgetq_lane_s32(vsum, STARFISH_PIXEL_A_INDEX);
                dstPtr[STARFISH_PIXEL_R_INDEX] =
                    static_cast<uint8_t>(sumR / static_cast<int>(dx));
                dstPtr[STARFISH_PIXEL_G_INDEX] =
                    static_cast<uint8_t>(sumG / static_cast<int>(dx));
                dstPtr[STARFISH_PIXEL_B_INDEX] =
                    static_cast<uint8_t>(sumB / static_cast<int>(dx));
                dstPtr[STARFISH_PIXEL_A_INDEX] =
                    static_cast<uint8_t>(sumA / static_cast<int>(dx));

                if (x >= dxLeft) {
                    unsigned leftOffset = pixelByteOffset - dxLeft * stride;
                    const uint8_t* srcPtr = srcData + leftOffset;
                    uint8x8_t pixel8 = vld1_u8(srcPtr);
                    uint16x8_t pixel16 = vmovl_u8(pixel8);
                    int32x4_t pixel32 =
                        vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(pixel16)));
                    vsum = vsubq_s32(vsum, pixel32);
                }

                if (x + dxRight < effectWidth) {
                    unsigned rightOffset = pixelByteOffset + dxRight * stride;
                    const uint8_t* srcPtr = srcData + rightOffset;
                    uint8x8_t pixel8 = vld1_u8(srcPtr);
                    uint16x8_t pixel16 = vmovl_u8(pixel8);
                    int32x4_t pixel32 =
                        vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(pixel16)));
                    vsum = vaddq_s32(vsum, pixel32);
                }
            }
        } else {
            const uint8_t* edgeValueLeft = srcData + line;
            const uint8_t* edgeValueRight =
                srcData + (line + (effectWidth - 1) * stride);

            for (int i = dxLeft * -1; i < dxRight; ++i) {
                unsigned offset = line + i * stride;
                const uint8_t* srcPtr = srcData + offset;
                uint8x8_t pixel8;

                if (i < 0) {
                    pixel8 = vld1_u8(edgeValueLeft);
                } else if (i >= effectWidth) {
                    pixel8 = vld1_u8(edgeValueRight);
                } else {
                    pixel8 = vld1_u8(srcPtr);
                }
                uint16x8_t pixel16 = vmovl_u8(pixel8);
                int32x4_t pixel32 =
                    vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(pixel16)));
                vsum = vaddq_s32(vsum, pixel32);
            }

            for (int x = 0; x < effectWidth; ++x) {
                unsigned pixelByteOffset = line + x * stride;
                uint8_t* dstPtr = dstData + pixelByteOffset;

                int sumR = vgetq_lane_s32(vsum, STARFISH_PIXEL_R_INDEX);
                int sumG = vgetq_lane_s32(vsum, STARFISH_PIXEL_G_INDEX);
                int sumB = vgetq_lane_s32(vsum, STARFISH_PIXEL_B_INDEX);
                int sumA = vgetq_lane_s32(vsum, STARFISH_PIXEL_A_INDEX);
                dstPtr[STARFISH_PIXEL_R_INDEX] =
                    static_cast<uint8_t>(sumR / static_cast<int>(dx));
                dstPtr[STARFISH_PIXEL_G_INDEX] =
                    static_cast<uint8_t>(sumG / static_cast<int>(dx));
                dstPtr[STARFISH_PIXEL_B_INDEX] =
                    static_cast<uint8_t>(sumB / static_cast<int>(dx));
                dstPtr[STARFISH_PIXEL_A_INDEX] =
                    static_cast<uint8_t>(sumA / static_cast<int>(dx));

                int32x4_t leftPixel;
                if (x < dxLeft) {
                    uint8x8_t pixel8 = vld1_u8(edgeValueLeft);
                    uint16x8_t pixel16 = vmovl_u8(pixel8);
                    leftPixel =
                        vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(pixel16)));
                } else {
                    unsigned leftOffset = pixelByteOffset - dxLeft * stride;
                    const uint8_t* srcPtr = srcData + leftOffset;
                    uint8x8_t pixel8 = vld1_u8(srcPtr);
                    uint16x8_t pixel16 = vmovl_u8(pixel8);
                    leftPixel =
                        vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(pixel16)));
                }
                vsum = vsubq_s32(vsum, leftPixel);

                int32x4_t rightPixel;
                if (x + dxRight >= effectWidth) {
                    uint8x8_t pixel8 = vld1_u8(edgeValueRight);
                    uint16x8_t pixel16 = vmovl_u8(pixel8);
                    rightPixel =
                        vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(pixel16)));
                } else {
                    unsigned rightOffset = pixelByteOffset + dxRight * stride;
                    const uint8_t* srcPtr = srcData + rightOffset;
                    uint8x8_t pixel8 = vld1_u8(srcPtr);
                    uint16x8_t pixel16 = vmovl_u8(pixel8);
                    rightPixel =
                        vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(pixel16)));
                }
                vsum = vaddq_s32(vsum, rightPixel);
            }
        }
    }
}
#else
inline void boxBlurScalar(uint8_t* srcData, uint8_t* dstData, unsigned dx,
                          int dxLeft, int dxRight, int stride, int strideLine,
                          int effectWidth, int effectHeight, bool alphaImage,
                          SVGFEGaussianBlurElement::EdgeMode edgeMode)
{
    const int maxKernelSize = std::min(dxRight, effectWidth);

    if (alphaImage) {
        return boxBlurAlphaOnly(srcData, dstData, dx, dxLeft, dxRight, stride,
                                strideLine, effectWidth, effectHeight,
                                maxKernelSize);
    }

    for (int y = 0; y < effectHeight; ++y) {
        int line = y * strideLine;
        int sumR = 0, sumG = 0, sumB = 0, sumA = 0;

        if (edgeMode == SVGFEGaussianBlurElement::EdgeMode::SVG_EDGEMODE_NONE) {
            for (int i = 0; i < maxKernelSize; ++i) {
                unsigned offset = line + i * stride;
                const uint8_t* srcPtr = srcData + offset;
                sumR += srcPtr[STARFISH_PIXEL_R_INDEX];
                sumG += srcPtr[STARFISH_PIXEL_G_INDEX];
                sumB += srcPtr[STARFISH_PIXEL_B_INDEX];
                sumA += srcPtr[STARFISH_PIXEL_A_INDEX];
            }

            for (int x = 0; x < effectWidth; ++x) {
                unsigned pixelByteOffset = line + x * stride;
                uint8_t* dstPtr = dstData + pixelByteOffset;

                dstPtr[STARFISH_PIXEL_R_INDEX] =
                    static_cast<uint8_t>(sumR / dx);
                dstPtr[STARFISH_PIXEL_G_INDEX] =
                    static_cast<uint8_t>(sumG / dx);
                dstPtr[STARFISH_PIXEL_B_INDEX] =
                    static_cast<uint8_t>(sumB / dx);
                dstPtr[STARFISH_PIXEL_A_INDEX] =
                    static_cast<uint8_t>(sumA / dx);

                if (x >= dxLeft) {
                    unsigned leftOffset = pixelByteOffset - dxLeft * stride;
                    const uint8_t* srcPtr = srcData + leftOffset;
                    sumR -= srcPtr[STARFISH_PIXEL_R_INDEX];
                    sumG -= srcPtr[STARFISH_PIXEL_G_INDEX];
                    sumB -= srcPtr[STARFISH_PIXEL_B_INDEX];
                    sumA -= srcPtr[STARFISH_PIXEL_A_INDEX];
                }

                if (x + dxRight < effectWidth) {
                    unsigned rightOffset = pixelByteOffset + dxRight * stride;
                    const uint8_t* srcPtr = srcData + rightOffset;
                    sumR += srcPtr[STARFISH_PIXEL_R_INDEX];
                    sumG += srcPtr[STARFISH_PIXEL_G_INDEX];
                    sumB += srcPtr[STARFISH_PIXEL_B_INDEX];
                    sumA += srcPtr[STARFISH_PIXEL_A_INDEX];
                }
            }
        } else {
            const uint8_t* edgeValueLeft = srcData + line;
            const uint8_t* edgeValueRight =
                srcData + (line + (effectWidth - 1) * stride);

            for (int i = dxLeft * -1; i < dxRight; ++i) {
                unsigned offset = line + i * stride;
                const uint8_t* srcPtr = srcData + offset;

                if (i < 0) {
                    sumR += edgeValueLeft[STARFISH_PIXEL_R_INDEX];
                    sumG += edgeValueLeft[STARFISH_PIXEL_G_INDEX];
                    sumB += edgeValueLeft[STARFISH_PIXEL_B_INDEX];
                    sumA += edgeValueLeft[STARFISH_PIXEL_A_INDEX];
                } else if (i >= effectWidth) {
                    sumR += edgeValueRight[STARFISH_PIXEL_R_INDEX];
                    sumG += edgeValueRight[STARFISH_PIXEL_G_INDEX];
                    sumB += edgeValueRight[STARFISH_PIXEL_B_INDEX];
                    sumA += edgeValueRight[STARFISH_PIXEL_A_INDEX];
                } else {
                    sumR += srcPtr[STARFISH_PIXEL_R_INDEX];
                    sumG += srcPtr[STARFISH_PIXEL_G_INDEX];
                    sumB += srcPtr[STARFISH_PIXEL_B_INDEX];
                    sumA += srcPtr[STARFISH_PIXEL_A_INDEX];
                }
            }

            for (int x = 0; x < effectWidth; ++x) {
                unsigned pixelByteOffset = line + x * stride;
                uint8_t* dstPtr = dstData + pixelByteOffset;

                dstPtr[STARFISH_PIXEL_R_INDEX] =
                    static_cast<uint8_t>(sumR / dx);
                dstPtr[STARFISH_PIXEL_G_INDEX] =
                    static_cast<uint8_t>(sumG / dx);
                dstPtr[STARFISH_PIXEL_B_INDEX] =
                    static_cast<uint8_t>(sumB / dx);
                dstPtr[STARFISH_PIXEL_A_INDEX] =
                    static_cast<uint8_t>(sumA / dx);

                if (x < dxLeft) {
                    sumR -= edgeValueLeft[STARFISH_PIXEL_R_INDEX];
                    sumG -= edgeValueLeft[STARFISH_PIXEL_G_INDEX];
                    sumB -= edgeValueLeft[STARFISH_PIXEL_B_INDEX];
                    sumA -= edgeValueLeft[STARFISH_PIXEL_A_INDEX];
                } else {
                    unsigned leftOffset = pixelByteOffset - dxLeft * stride;
                    const uint8_t* srcPtr = srcData + leftOffset;
                    sumR -= srcPtr[STARFISH_PIXEL_R_INDEX];
                    sumG -= srcPtr[STARFISH_PIXEL_G_INDEX];
                    sumB -= srcPtr[STARFISH_PIXEL_B_INDEX];
                    sumA -= srcPtr[STARFISH_PIXEL_A_INDEX];
                }

                if (x + dxRight >= effectWidth) {
                    sumR += edgeValueRight[STARFISH_PIXEL_R_INDEX];
                    sumG += edgeValueRight[STARFISH_PIXEL_G_INDEX];
                    sumB += edgeValueRight[STARFISH_PIXEL_B_INDEX];
                    sumA += edgeValueRight[STARFISH_PIXEL_A_INDEX];
                } else {
                    unsigned rightOffset = pixelByteOffset + dxRight * stride;
                    const uint8_t* srcPtr = srcData + rightOffset;
                    sumR += srcPtr[STARFISH_PIXEL_R_INDEX];
                    sumG += srcPtr[STARFISH_PIXEL_G_INDEX];
                    sumB += srcPtr[STARFISH_PIXEL_B_INDEX];
                    sumA += srcPtr[STARFISH_PIXEL_A_INDEX];
                }
            }
        }
    }
}
#endif

inline void boxBlur(uint8_t* srcData, uint8_t* dstData, unsigned dx, int dxLeft,
                    int dxRight, int stride, int strideLine, int effectWidth,
                    int effectHeight, bool alphaImage,
                    SVGFEGaussianBlurElement::EdgeMode edgeMode)
{
#if defined(STARFISH_X86) || defined(STARFISH_X86_64) || \
    defined(STARFISH_ARM_NEON)
    boxBlurSIMD(srcData, dstData, dx, dxLeft, dxRight, stride, strideLine,
                effectWidth, effectHeight, alphaImage, edgeMode);
#else
    boxBlurScalar(srcData, dstData, dx, dxLeft, dxRight, stride, strideLine,
                  effectWidth, effectHeight, alphaImage, edgeMode);
#endif
}

// Box-average |factor| x |factor| source pixels into one destination pixel.
// The buffers hold premultiplied RGBA, where a component-wise average is a
// valid downscale.
void filterDownsampleRGBA(const uint8_t* src, int srcWidth, int srcHeight,
                          int srcStride, uint8_t* dst, int dstWidth,
                          int dstHeight, int dstStride, int factor)
{
    for (int y = 0; y < dstHeight; ++y) {
        uint8_t* dstRow = dst + y * dstStride;
        int sourceYBegin = y * factor;
        int sourceYEnd = std::min(sourceYBegin + factor, srcHeight);
        for (int x = 0; x < dstWidth; ++x) {
            int sourceXBegin = x * factor;
            int sourceXEnd = std::min(sourceXBegin + factor, srcWidth);
            unsigned sum[4] = { 0, 0, 0, 0 };
            unsigned count = 0;
            for (int sy = sourceYBegin; sy < sourceYEnd; ++sy) {
                const uint8_t* srcPixel =
                    src + sy * srcStride + sourceXBegin * 4;
                for (int sx = sourceXBegin; sx < sourceXEnd;
                     ++sx, srcPixel += 4) {
                    sum[0] += srcPixel[0];
                    sum[1] += srcPixel[1];
                    sum[2] += srcPixel[2];
                    sum[3] += srcPixel[3];
                    count++;
                }
            }
            uint8_t* dstPixel = dstRow + x * 4;
            if (!count) {
                dstPixel[0] = dstPixel[1] = dstPixel[2] = dstPixel[3] = 0;
                continue;
            }
            dstPixel[0] = static_cast<uint8_t>(sum[0] / count);
            dstPixel[1] = static_cast<uint8_t>(sum[1] / count);
            dstPixel[2] = static_cast<uint8_t>(sum[2] / count);
            dstPixel[3] = static_cast<uint8_t>(sum[3] / count);
        }
    }
}

// Bilinear expansion back to the full resolution. The source is already
// blurred, so interpolating between its samples reproduces a smooth gradient
// rather than the reduced sampling grid.
void filterUpsampleRGBA(const uint8_t* src, int srcWidth, int srcHeight,
                        int srcStride, uint8_t* dst, int dstWidth,
                        int dstHeight, int dstStride, int factor)
{
    const int fixedOne = 256;
    for (int y = 0; y < dstHeight; ++y) {
        // Sample at the center of the source texel covering this row.
        int sourceYFixed =
            ((y * 2 + 1) * fixedOne) / (2 * factor) - fixedOne / 2;
        if (sourceYFixed < 0) {
            sourceYFixed = 0;
        }
        int sourceY = sourceYFixed / fixedOne;
        int weightY = sourceYFixed - sourceY * fixedOne;
        if (sourceY >= srcHeight - 1) {
            sourceY = srcHeight - 1;
            weightY = 0;
        }
        const uint8_t* sourceRowTop = src + sourceY * srcStride;
        const uint8_t* sourceRowBottom =
            src + std::min(sourceY + 1, srcHeight - 1) * srcStride;
        uint8_t* dstRow = dst + y * dstStride;
        for (int x = 0; x < dstWidth; ++x) {
            int sourceXFixed =
                ((x * 2 + 1) * fixedOne) / (2 * factor) - fixedOne / 2;
            if (sourceXFixed < 0) {
                sourceXFixed = 0;
            }
            int sourceX = sourceXFixed / fixedOne;
            int weightX = sourceXFixed - sourceX * fixedOne;
            if (sourceX >= srcWidth - 1) {
                sourceX = srcWidth - 1;
                weightX = 0;
            }
            int sourceXNext = std::min(sourceX + 1, srcWidth - 1);
            uint8_t* dstPixel = dstRow + x * 4;
            for (int channel = 0; channel < 4; ++channel) {
                int topLeft = sourceRowTop[sourceX * 4 + channel];
                int topRight = sourceRowTop[sourceXNext * 4 + channel];
                int bottomLeft = sourceRowBottom[sourceX * 4 + channel];
                int bottomRight = sourceRowBottom[sourceXNext * 4 + channel];
                int top = topLeft * (fixedOne - weightX) + topRight * weightX;
                int bottom =
                    bottomLeft * (fixedOne - weightX) + bottomRight * weightX;
                dstPixel[channel] = static_cast<uint8_t>(
                    (top * (fixedOne - weightY) + bottom * weightY) /
                    (fixedOne * fixedOne));
            }
        }
    }
}

// A blur wide enough to average many source pixels into each output pixel can
// be computed on a downscaled copy: the detail the downscale throws away is
// detail the blur would have erased anyway. Each doubling of the factor cuts
// the work by 4x, which is what makes large decorative blurs affordable on a
// device - on a Family Hub the glow behind the Bixby prompt costs ~59 ms per
// frame at full resolution.
static int computeDownsampleFactor(unsigned kernelSizeX, unsigned kernelSizeY,
                                   int width, int height)
{
    // Keep this many samples across the kernel after downscaling, so the blur
    // still has a smooth profile, and keep the reduced image big enough that
    // its edges stay meaningful.
    const unsigned minKernelSizeAfterScale = 8;
    const int minDimensionAfterScale = 16;
    const int maxFactor = 4;

    // A zero kernel size means "do not blur along this axis"; it must not
    // decide the factor.
    unsigned kernelSize = std::min(kernelSizeX ? kernelSizeX : kernelSizeY,
                                   kernelSizeY ? kernelSizeY : kernelSizeX);
    if (!kernelSize) {
        return 1;
    }

    int factor = 1;
    while (factor < maxFactor &&
           kernelSize / (unsigned)(factor * 2) >= minKernelSizeAfterScale &&
           width / (factor * 2) >= minDimensionAfterScale &&
           height / (factor * 2) >= minDimensionAfterScale) {
        factor *= 2;
    }
    return factor;
}

inline void standardBoxBlur(uint8_t* fromBuffer, uint8_t* toBuffer,
                            unsigned kernelSizeX, unsigned kernelSizeY,
                            int stride, int imageWidth, int imageHeight,
                            bool alphaImage,
                            SVGFEGaussianBlurElement::EdgeMode edgeMode)
{
    int dxLeft = 0;
    int dxRight = 0;
    int dyLeft = 0;
    int dyRight = 0;

    for (int i = 0; i < 3; ++i) {
        if (kernelSizeX) {
            kernelPosition(i, kernelSizeX, dxLeft, dxRight);
            boxBlur(fromBuffer, toBuffer, kernelSizeX, dxLeft, dxRight, 4,
                    stride, imageWidth, imageHeight, alphaImage, edgeMode);
            std::swap(fromBuffer, toBuffer);
        }

        if (kernelSizeY) {
            kernelPosition(i, kernelSizeY, dyLeft, dyRight);
            boxBlur(fromBuffer, toBuffer, kernelSizeY, dyLeft, dyRight, stride,
                    4, imageHeight, imageWidth, alphaImage, edgeMode);
            std::swap(fromBuffer, toBuffer);
        }
    }
}

static inline float gaussianKernelFactor()
{
    return 3 / 4.f * sqrtf(2 * M_PI);
}

static int clampedToKernelSize(float value)
{
    // Limit the kernel size to 500. A bigger radius won't make a big difference
    // for the result image but
    // inflates the absolute paint rect too much. This is compatible with
    // Firefox' behavior.
    unsigned size = std::max<unsigned>(
        2,
        static_cast<unsigned>(floorf(value * gaussianKernelFactor() + 0.5f)));
    return clampTo<int>(std::min(size, static_cast<unsigned>(500.f)));
}

float computeKernelSizeAtStdDeviation(float stdDeviation)
{
    return clampedToKernelSize(stdDeviation);
}

std::pair<float, float> FilterGaussianBlur::computeKernelSize(
    float stdDeviationX, float stdDeviationY)
{
    if (stdDeviationX <= 0 || stdDeviationY <= 0) {
        return std::make_pair(0, 0);
    }
    return std::make_pair(computeKernelSizeAtStdDeviation(stdDeviationX),
                          computeKernelSizeAtStdDeviation(stdDeviationY));
}

FilterGaussianBlur::FilterGaussianBlur(
    Filter* filter, SVGFilterPrimitiveStandardAttributes* element)
    : FilterPrimitive(filter, element,
                      element->asSVGFEGaussianBlurElement()->in1()->baseVal(),
                      element->output()->baseVal())
{
    STARFISH_ASSERT(filter);
    STARFISH_ASSERT(element->isSVGFEGaussianBlurElement());
}

void* FilterGaussianBlur::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FilterGaussianBlur));
    static bool typeInited = false;
    static GC_descr descr;
    if (typeInited == false) {
        GC_word desc[GC_BITMAP_SIZE(FilterGaussianBlur)] = { 0 };
        FilterPrimitive::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(FilterGaussianBlur));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

std::pair<float, float> FilterGaussianBlur::computeStdXY(
    const LayoutSize& targetSize, const std::pair<float, float>& viewportScale)
{
    auto e = element()->asSVGFEGaussianBlurElement();
    float stdX = filter()->resolveFilterPrimitiveValue(
        e->stdDeviationX()->animVal(), targetSize.width(), viewportScale.first);
    float stdY = filter()->resolveFilterPrimitiveValue(
        e->stdDeviationY()->animVal(), targetSize.height(),
        viewportScale.second);

    return std::make_pair(stdX, stdY);
}

Filter::FilterBias FilterGaussianBlur::computeBias(ComputeBiasContext& ctx)
{
    auto e = element()->asSVGFEGaussianBlurElement();
    auto stdXY = computeStdXY(ctx.targetSize, ctx.viewportScale);
    auto kernel =
        FilterGaussianBlur::computeKernelSize(stdXY.first, stdXY.second);

    Unit::Rect newRt;
    if (input()->equals("SourceGraphic")) {
        newRt.setX(ctx.candidateFilterFrameRect.x());
        newRt.setY(ctx.candidateFilterFrameRect.y());
        newRt.setWidth(ctx.candidateFilterFrameRect.width());
        newRt.setHeight(ctx.candidateFilterFrameRect.height());
    } else {
        newRt.setX(ctx.currentVisibleRect.x());
        newRt.setY(ctx.currentVisibleRect.y());
        newRt.setWidth(ctx.currentVisibleRect.width());
        newRt.setHeight(ctx.currentVisibleRect.height());
    }

    newRt.setX(newRt.x() - kernel.first);
    newRt.setY(newRt.y() - kernel.second);
    newRt.setWidth(newRt.width() + kernel.first * 2);
    newRt.setHeight(newRt.height() + kernel.second * 2);

    ctx.currentVisibleRect.setX(ctx.currentVisibleRect.x() - kernel.first);
    ctx.currentVisibleRect.setY(ctx.currentVisibleRect.y() - kernel.second);
    ctx.currentVisibleRect.setWidth(ctx.currentVisibleRect.width() +
                                    kernel.first * 2);
    ctx.currentVisibleRect.setHeight(ctx.currentVisibleRect.height() +
                                     kernel.second * 2);

    return Filter::FilterBias(newRt);
}

void FilterGaussianBlur::apply(const Unit::Rect& subRegionInFloat,
                               Filter::FilterApplyContext& ctx)
{
    STARFISH_ASSERT(element()->isSVGFEGaussianBlurElement());
    SVGFEGaussianBlurElement* ele = element()->asSVGFEGaussianBlurElement();

    auto vm = ctx.target->outmostSVGViewportBox()
                  ->computeTranlateScaleOnPaint()
                  .second;
    auto stdXY = computeStdXY(ctx.target->unadjustedFrameRectByFilter()->size(),
                              std::make_pair(vm.getScaleX(), vm.getScaleY()));
    if (stdXY.first <= 0 || stdXY.second <= 0) {
        return;
    }

    auto kernelSize = computeKernelSize(stdXY.first * ctx.viewportScaleX,
                                        stdXY.second * ctx.viewportScaleY);

    float dpr = element()->webView()->screenInfo().devicePixelRatio;
    kernelSize.first *= dpr;
    kernelSize.second *= dpr;

    std::shared_ptr<Filter::FilterSourceBuffer> inputSource =
        filter()->fetchInputSource(ctx, this);

    // we should copy source buffer since blur function overwrite input buffer
    if (filter()->shouldMaintainSourceBuffer() &&
        inputSource->data() == ctx.src) {
        inputSource = std::shared_ptr<Filter::FilterSourceBuffer>(
            new Filter::FilterSourceBuffer(ctx.src, ctx.stride * ctx.height,
                                           true));
        memcpy(inputSource->data(), ctx.src, inputSource->size());
    }

    std::shared_ptr<Filter::FilterSourceBuffer> outputBuffer(
        new Filter::FilterSourceBuffer(ctx.src, ctx.stride * ctx.height, true));

    auto edgeMode =
        (SVGFEGaussianBlurElement::EdgeMode)ele->edgeMode()->baseVal();
    int factor = computeDownsampleFactor(kernelSize.first, kernelSize.second,
                                         ctx.width, ctx.height);
    if (factor > 1) {
        int reducedWidth = ((int)ctx.width + factor - 1) / factor;
        int reducedHeight = ((int)ctx.height + factor - 1) / factor;
        size_t reducedStride = (size_t)reducedWidth * 4;
        size_t reducedSize = reducedStride * reducedHeight;
        Filter::FilterSourceBuffer reducedInput(nullptr, reducedSize, true);
        Filter::FilterSourceBuffer reducedOutput(nullptr, reducedSize, true);

        filterDownsampleRGBA(inputSource->data(), ctx.width, ctx.height, ctx.stride,
                       reducedInput.data(), reducedWidth, reducedHeight,
                       reducedStride, factor);
        standardBoxBlur(reducedInput.data(), reducedOutput.data(),
                        kernelSize.first / factor, kernelSize.second / factor,
                        reducedStride, reducedWidth, reducedHeight,
                        ctx.isAlphaImage, edgeMode);
        filterUpsampleRGBA(reducedOutput.data(), reducedWidth, reducedHeight,
                     reducedStride, outputBuffer->data(), ctx.width, ctx.height,
                     ctx.stride, factor);
    } else {
        standardBoxBlur(inputSource->data(), outputBuffer->data(),
                        kernelSize.first, kernelSize.second, ctx.stride,
                        ctx.width, ctx.height, ctx.isAlphaImage, edgeMode);
    }

    auto normalizedSubRegion = normalizeSubRegion(subRegionInFloat);

    if (!subRegionCoversAll(normalizedSubRegion)) {
        Canvas* c = Canvas::create(outputBuffer->data(), ctx.width, ctx.height,
                                   ctx.stride, 1);
        c->setCompositeOperator(CanvasCompositeOperator::Copy,
                                BlendMode::Normal);
        c->rect(Unit::Rect(ctx.width * normalizedSubRegion.x(),
                           ctx.height * normalizedSubRegion.y(),
                           ctx.width * normalizedSubRegion.width(),
                           ctx.height * normalizedSubRegion.height()));
        c->rect(Unit::Rect(0, 0, ctx.width, ctx.height));
        c->closePath();
        c->setFillColor(Unit::Color(0, 0, 0, 0));
        c->setFillRule(false);
        c->fill();
        delete c;
    }

    filter()->registerOutput(ctx, this, outputBuffer);
}

} // namespace Starfish
