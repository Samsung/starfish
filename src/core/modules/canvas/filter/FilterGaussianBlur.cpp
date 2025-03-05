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

inline void boxBlurAlphaOnly(uint8_t* srcData, uint8_t* dstData, unsigned dx,
                             int& dxLeft, int& dxRight, int& stride,
                             int& strideLine, int& effectWidth,
                             int& effectHeight, const int& maxKernelSize)
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

inline void boxBlur(uint8_t* srcData, uint8_t* dstData, unsigned dx, int dxLeft,
                    int dxRight, int stride, int strideLine, int effectWidth,
                    int effectHeight, bool alphaImage,
                    SVGFEGaussianBlurElement::EdgeMode edgeMode)
{
    const int maxKernelSize = std::min(dxRight, effectWidth);

    if (alphaImage)
        return boxBlurAlphaOnly(srcData, dstData, dx, dxLeft, dxRight, stride,
                                strideLine, effectWidth, effectHeight,
                                maxKernelSize);

    // Concerning the array width/length: it is Element size + Margin + Border.
    // The number of pixels will be
    // P = width * height * channels.
    for (int y = 0; y < effectHeight; ++y) {
        int line = y * strideLine;
        int sumR = 0, sumG = 0, sumB = 0, sumA = 0;

        if (edgeMode == SVGFEGaussianBlurElement::EdgeMode::SVG_EDGEMODE_NONE) {
            // Fill the kernel.
            for (int i = 0; i < maxKernelSize; ++i) {
                unsigned offset = line + i * stride;
                const uint8_t* srcPtr = srcData + offset;
#if defined(PORT_PIXEL_ORDER_RGBA)
                sumR += *srcPtr++;
                sumG += *srcPtr++;
                sumB += *srcPtr++;
                sumA += *srcPtr;
#else
                sumB += *srcPtr++;
                sumG += *srcPtr++;
                sumR += *srcPtr++;
                sumA += *srcPtr;
#endif
            }

            // Blurring.
            for (int x = 0; x < effectWidth; ++x) {
                unsigned pixelByteOffset = line + x * stride;
                uint8_t* dstPtr = dstData + pixelByteOffset;

#if defined(PORT_PIXEL_ORDER_RGBA)
                *dstPtr++ = static_cast<uint8_t>(sumR / dx);
                *dstPtr++ = static_cast<uint8_t>(sumG / dx);
                *dstPtr++ = static_cast<uint8_t>(sumB / dx);
                *dstPtr = static_cast<uint8_t>(sumA / dx);
#else
                *dstPtr++ = static_cast<uint8_t>(sumB / dx);
                *dstPtr++ = static_cast<uint8_t>(sumG / dx);
                *dstPtr++ = static_cast<uint8_t>(sumR / dx);
                *dstPtr = static_cast<uint8_t>(sumA / dx);
#endif

                // Shift kernel.
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
            // FIXME: Add support for 'wrap' here.
            // Get edge values for edgeMode 'duplicate'.
            const uint8_t* edgeValueLeft = srcData + line;
            const uint8_t* edgeValueRight =
                srcData + (line + (effectWidth - 1) * stride);

            // Fill the kernel.
            for (int i = dxLeft * -1; i < dxRight; ++i) {
                // Is this right for negative values of 'i'?
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
#if defined(PORT_PIXEL_ORDER_RGBA)
                    sumR += *srcPtr++;
                    sumG += *srcPtr++;
                    sumB += *srcPtr++;
                    sumA += *srcPtr;
#else
                    sumB += *srcPtr++;
                    sumG += *srcPtr++;
                    sumR += *srcPtr++;
                    sumA += *srcPtr;
#endif
                }
            }

            // Blurring.
            for (int x = 0; x < effectWidth; ++x) {
                unsigned pixelByteOffset = line + x * stride;
                uint8_t* dstPtr = dstData + pixelByteOffset;

#if defined(PORT_PIXEL_ORDER_RGBA)
                *dstPtr++ = static_cast<uint8_t>(sumR / dx);
                *dstPtr++ = static_cast<uint8_t>(sumG / dx);
                *dstPtr++ = static_cast<uint8_t>(sumB / dx);
                *dstPtr = static_cast<uint8_t>(sumA / dx);
#else
                *dstPtr++ = static_cast<uint8_t>(sumB / dx);
                *dstPtr++ = static_cast<uint8_t>(sumG / dx);
                *dstPtr++ = static_cast<uint8_t>(sumR / dx);
                *dstPtr = static_cast<uint8_t>(sumA / dx);
#endif
                // Shift kernel.
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
    : FilterPrimitive(filter, element)
{
    STARFISH_ASSERT(filter);
    STARFISH_ASSERT(element->isSVGFEGaussianBlurElement());

    auto kernel = FilterGaussianBlur::computeKernelSize(
        element->asSVGFEGaussianBlurElement()->stdDeviationX()->baseVal(),
        element->asSVGFEGaussianBlurElement()->stdDeviationY()->baseVal());
    float kernelX = kernel.first;
    float kernelY = kernel.second;

    SVGFEGaussianBlurElement* ele = element->asSVGFEGaussianBlurElement();
    if ((SVGFEGaussianBlurElement::EdgeMode)ele->edgeMode()->baseVal() ==
        SVGFEGaussianBlurElement::EdgeMode::SVG_EDGEMODE_NONE) {
        filter->setBias(-kernelX, -kernelY, kernelX * 3, kernelY * 3);
    }
}

void* FilterGaussianBlur::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FilterGaussianBlur));
    static bool typeInited = false;
    static GC_descr descr;
    if (typeInited == false) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(FilterGaussianBlur)] = { 0 };
        STARFISH_ASSERT(obj_bitmap != nullptr);
        descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(FilterGaussianBlur));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void FilterGaussianBlur::apply(size_t x, size_t y, size_t width, size_t height,
                               Filter::FilterApplyContext& ctx)
{
    STARFISH_ASSERT(element()->isSVGFEGaussianBlurElement());
    SVGFEGaussianBlurElement* ele = element()->asSVGFEGaussianBlurElement();

    float stdDeviationX = ele->stdDeviationX()->baseVal();
    if (stdDeviationX <= 0) {
        return;
    }
    float stdDeviationY = ele->stdDeviationY()->baseVal();
    if (stdDeviationY < 0) {
        return;
    }
    auto kernelSize = computeKernelSize(stdDeviationX * ctx.viewportScaleX,
                                        stdDeviationY * ctx.viewportScaleY);

    String* sourceNameStr = ele->in1()->baseVal();
    auto inputSource = ctx.sourceGraphic();

    std::shared_ptr<Filter::FilterApplyContext::FilterSourceBuffer>
        outputBuffer(new Filter::FilterApplyContext::FilterSourceBuffer(
            ctx.src, ctx.stride * ctx.height, true));

    standardBoxBlur(
        inputSource->data(), outputBuffer->data(), kernelSize.first,
        kernelSize.second, ctx.stride, ctx.width, ctx.height, ctx.isAlphaImage,
        (SVGFEGaussianBlurElement::EdgeMode)ele->edgeMode()->baseVal());

    // update source graphic for next filter
    ctx.updateSourceGraphic(outputBuffer);
}

} // namespace Starfish
