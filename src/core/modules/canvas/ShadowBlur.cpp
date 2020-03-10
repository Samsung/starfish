/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2010 Igalia, S.L.
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 * Copyright (C) 2015-2016 Apple, Inc. All rights reserved.
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#include "ShadowBlur.h"

#include "core/modules/canvas/image/NativeImageData.h"

namespace Starfish {

const float ShadowBlur::RADIUS_LIMIT = 500.f;

ShadowBlur::ShadowBlur(uint8_t* source, const size_t& width,
                       const size_t& height, const size_t& stride)
    : m_width(width)
    , m_height(height)
    , m_stride(stride)
    , m_source(source)
    , m_workspace(new uint8_t[m_stride * m_height],
                  [](uint8_t* data) { delete[] data; })
{
}

ShadowBlur::~ShadowBlur()
{
}

// the box blur functions belows are import from WebKit project
// webkit/Source/WebCore/platform/graphics/filters/FEGaussianBlur.cpp(6f9b511a115311b13c06eb58038ddc2c78da5531)

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

enum EdgeModeType {
    EDGEMODE_UNKNOWN = 0,
    EDGEMODE_DUPLICATE = 1,
    EDGEMODE_WRAP = 2,
    EDGEMODE_NONE = 3
};

inline void boxBlur(uint8_t* srcData, uint8_t* dstData, unsigned dx, int dxLeft,
                    int dxRight, int stride, int strideLine, int effectWidth,
                    int effectHeight, EdgeModeType edgeMode)
{
    const int maxKernelSize = std::min(dxRight, effectWidth);

    // Concerning the array width/length: it is Element size + Margin + Border.
    // The number of pixels will be
    // P = width * height * channels.
    for (int y = 0; y < effectHeight; ++y) {
        int line = y * strideLine;
        int sumR = 0, sumG = 0, sumB = 0, sumA = 0;

        if (edgeMode == EDGEMODE_NONE) {
            // Fill the kernel.
            for (int i = 0; i < maxKernelSize; ++i) {
                unsigned offset = line + i * stride;
                const uint8_t* srcPtr = srcData + offset;
                sumR += *srcPtr++;
                sumG += *srcPtr++;
                sumB += *srcPtr++;
                sumA += *srcPtr;
            }

            // Blurring.
            for (int x = 0; x < effectWidth; ++x) {
                unsigned pixelByteOffset = line + x * stride;
                uint8_t* dstPtr = dstData + pixelByteOffset;

                *dstPtr++ = static_cast<uint8_t>(sumR / dx);
                *dstPtr++ = static_cast<uint8_t>(sumG / dx);
                *dstPtr++ = static_cast<uint8_t>(sumB / dx);
                *dstPtr = static_cast<uint8_t>(sumA / dx);

                // Shift kernel.
                if (x >= dxLeft) {
                    unsigned leftOffset = pixelByteOffset - dxLeft * stride;
                    const uint8_t* srcPtr = srcData + leftOffset;
                    sumR -= srcPtr[0];
                    sumG -= srcPtr[1];
                    sumB -= srcPtr[2];
                    sumA -= srcPtr[3];
                }

                if (x + dxRight < effectWidth) {
                    unsigned rightOffset = pixelByteOffset + dxRight * stride;
                    const uint8_t* srcPtr = srcData + rightOffset;
                    sumR += srcPtr[0];
                    sumG += srcPtr[1];
                    sumB += srcPtr[2];
                    sumA += srcPtr[3];
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
                    sumR += edgeValueLeft[0];
                    sumG += edgeValueLeft[1];
                    sumB += edgeValueLeft[2];
                    sumA += edgeValueLeft[3];
                } else if (i >= effectWidth) {
                    sumR += edgeValueRight[0];
                    sumG += edgeValueRight[1];
                    sumB += edgeValueRight[2];
                    sumA += edgeValueRight[3];
                } else {
                    sumR += *srcPtr++;
                    sumG += *srcPtr++;
                    sumB += *srcPtr++;
                    sumA += *srcPtr;
                }
            }

            // Blurring.
            for (int x = 0; x < effectWidth; ++x) {
                unsigned pixelByteOffset = line + x * stride;
                uint8_t* dstPtr = dstData + pixelByteOffset;

                *dstPtr++ = static_cast<uint8_t>(sumR / dx);
                *dstPtr++ = static_cast<uint8_t>(sumG / dx);
                *dstPtr++ = static_cast<uint8_t>(sumB / dx);
                *dstPtr = static_cast<uint8_t>(sumA / dx);

                // Shift kernel.
                if (x < dxLeft) {
                    sumR -= edgeValueLeft[0];
                    sumG -= edgeValueLeft[1];
                    sumB -= edgeValueLeft[2];
                    sumA -= edgeValueLeft[3];
                } else {
                    unsigned leftOffset = pixelByteOffset - dxLeft * stride;
                    const uint8_t* srcPtr = srcData + leftOffset;
                    sumR -= srcPtr[0];
                    sumG -= srcPtr[1];
                    sumB -= srcPtr[2];
                    sumA -= srcPtr[3];
                }

                if (x + dxRight >= effectWidth) {
                    sumR += edgeValueRight[0];
                    sumG += edgeValueRight[1];
                    sumB += edgeValueRight[2];
                    sumA += edgeValueRight[3];
                } else {
                    unsigned rightOffset = pixelByteOffset + dxRight * stride;
                    const uint8_t* srcPtr = srcData + rightOffset;
                    sumR += srcPtr[0];
                    sumG += srcPtr[1];
                    sumB += srcPtr[2];
                    sumA += srcPtr[3];
                }
            }
        }
    }
}

inline void standardBoxBlur(uint8_t* fromBuffer, uint8_t* toBuffer,
                            unsigned kernelSizeX, unsigned kernelSizeY,
                            int stride, int imageWidth, int imageHeight,
                            EdgeModeType edgeMode)
{
    int dxLeft = 0;
    int dxRight = 0;
    int dyLeft = 0;
    int dyRight = 0;

    for (int i = 0; i < 3; ++i) {
        if (kernelSizeX) {
            kernelPosition(i, kernelSizeX, dxLeft, dxRight);
            boxBlur(fromBuffer, toBuffer, kernelSizeX, dxLeft, dxRight, 4,
                    stride, imageWidth, imageHeight, edgeMode);
            std::swap(fromBuffer, toBuffer);
        }

        if (kernelSizeY) {
            kernelPosition(i, kernelSizeY, dyLeft, dyRight);
            boxBlur(fromBuffer, toBuffer, kernelSizeY, dyLeft, dyRight, stride,
                    4, imageHeight, imageWidth, edgeMode);
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
    return clampTo<int>(
        std::min(size, static_cast<unsigned>(ShadowBlur::RADIUS_LIMIT)));
}

float ShadowBlur::computeKernelSizeAtStdDeviation(float stdDeviation)
{
    return clampedToKernelSize(stdDeviation);
}

void ShadowBlur::process(float stdDeviation)
{
    if (stdDeviation <= 0) {
        return;
    }

    LongTaskFinder timer(__PRETTY_FUNCTION__, 1);
    float kernelSize = computeKernelSizeAtStdDeviation(stdDeviation);
    standardBoxBlur(m_source, m_workspace.get(), kernelSize, kernelSize,
                    m_stride, m_width, m_height, EDGEMODE_DUPLICATE);
}
}
