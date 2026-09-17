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

// ceil(2^32 / d): (n * m) >> 32 equals n / d while n * d < 2^32, and a box
// sum never exceeds 255 * d with d <= RADIUS_LIMIT + 1.
static inline uint64_t reciprocalOf(unsigned d)
{
    return ((1ull << 32) + d - 1) / d;
}

// One box pass along the rows. Each output is the mean of the window
// [x - left, x + right), with the row's first and last pixel repeated beyond
// its ends.
static void boxBlurRows(const uint8_t* src, uint8_t* dst, unsigned kernelSize,
                        int left, int right, int stride, int width, int height)
{
    const uint64_t m = reciprocalOf(kernelSize);
    const int last = width - 1;
    for (int y = 0; y < height; ++y) {
        const uint8_t* s = src + y * stride;
        uint8_t* d = dst + y * stride;
        uint32_t sum[4] = { 0, 0, 0, 0 };
        for (int i = -left; i < right; ++i) {
            const uint8_t* p = s + std::min(std::max(i, 0), last) * 4;
            sum[0] += p[0];
            sum[1] += p[1];
            sum[2] += p[2];
            sum[3] += p[3];
        }
        for (int x = 0; x < width; ++x) {
            d[0] = static_cast<uint8_t>((sum[0] * m) >> 32);
            d[1] = static_cast<uint8_t>((sum[1] * m) >> 32);
            d[2] = static_cast<uint8_t>((sum[2] * m) >> 32);
            d[3] = static_cast<uint8_t>((sum[3] * m) >> 32);
            d += 4;
            const uint8_t* out = s + std::max(x - left, 0) * 4;
            const uint8_t* in = s + std::min(x + right, last) * 4;
            sum[0] += in[0] - out[0];
            sum[1] += in[1] - out[1];
            sum[2] += in[2] - out[2];
            sum[3] += in[3] - out[3];
        }
    }
}

// The same pass along the columns, walked row by row: the window sums of
// every column are carried in `sums`, so memory is read and written in row
// order instead of striding down one column at a time.
static void boxBlurColumns(const uint8_t* src, uint8_t* dst,
                           unsigned kernelSize, int top, int bottom, int stride,
                           int width, int height, uint32_t* sums)
{
    const uint64_t m = reciprocalOf(kernelSize);
    const int last = height - 1;
    const int n = width * 4;
    memset(sums, 0, sizeof(uint32_t) * n);
    for (int i = -top; i < bottom; ++i) {
        const uint8_t* s = src + std::min(std::max(i, 0), last) * stride;
        for (int k = 0; k < n; ++k) {
            sums[k] += s[k];
        }
    }
    for (int y = 0; y < height; ++y) {
        uint8_t* d = dst + y * stride;
        for (int k = 0; k < n; ++k) {
            d[k] = static_cast<uint8_t>((sums[k] * m) >> 32);
        }
        const uint8_t* out = src + std::max(y - top, 0) * stride;
        const uint8_t* in = src + std::min(y + bottom, last) * stride;
        for (int k = 0; k < n; ++k) {
            sums[k] += in[k] - out[k];
        }
    }
}

inline void standardBoxBlur(uint8_t* fromBuffer, uint8_t* toBuffer,
                            unsigned kernelSizeX, unsigned kernelSizeY,
                            int stride, int imageWidth, int imageHeight)
{
    int dxLeft = 0;
    int dxRight = 0;
    int dyLeft = 0;
    int dyRight = 0;
    std::unique_ptr<uint32_t[]> columnSums(new uint32_t[imageWidth * 4]);

    for (int i = 0; i < 3; ++i) {
        if (kernelSizeX) {
            kernelPosition(i, kernelSizeX, dxLeft, dxRight);
            boxBlurRows(fromBuffer, toBuffer, kernelSizeX, dxLeft, dxRight,
                        stride, imageWidth, imageHeight);
            std::swap(fromBuffer, toBuffer);
        }

        if (kernelSizeY) {
            kernelPosition(i, kernelSizeY, dyLeft, dyRight);
            boxBlurColumns(fromBuffer, toBuffer, kernelSizeY, dyLeft, dyRight,
                           stride, imageWidth, imageHeight, columnSums.get());
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
    if (stdDeviation <= 0 || !m_width || !m_height) {
        return;
    }

    LongTaskFinder timer("ShadowBlur::process", 1);
    int kernelSize = computeKernelSizeAtStdDeviation(stdDeviation);
    standardBoxBlur(m_source, m_workspace.get(), kernelSize, kernelSize,
                    m_stride, m_width, m_height);
}
} // namespace Starfish
