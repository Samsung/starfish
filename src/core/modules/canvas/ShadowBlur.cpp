/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#include "ShadowBlur.h"

#define BOUND_CHECK(idx, maxIdx)                       \
    if ((int)(idx) < 0 || (int)(idx) >= (int)maxIdx) { \
        break;                                         \
    }

#define ROUND(v) (int)((v) + 0.5)
#define IDEAL_VALUE 3

namespace StarFish {

const float ShadowBlur::RADIUS_LIMIT = 250.0f;

ShadowBlur::ShadowBlur(uint8_t* source, const size_t& width,
                       const size_t& height, const size_t& stride)
    : m_width(width)
    , m_height(height)
    , m_stride(stride)
    , m_source(source)
    , m_workspace(new uint8_t[m_stride * m_height])
{
    // TODO
    STARFISH_RELEASE_ASSERT(m_stride == m_width * 4);
}

ShadowBlur::~ShadowBlur()
{
}

ALWAYS_INLINE unsigned readOne(uint32_t* src, size_t idx)
{
    return ((uint8_t*)src)[idx];
}

ALWAYS_INLINE void writeOne(uint32_t* dst, size_t idx, unsigned v)
{
    ((uint8_t*)dst)[idx] = v;
}

static void boxBlurH(uint8_t* src, uint8_t* dest, size_t w, size_t h,
                     size_t stride, int r)
{
    double iarr = 1.0 / (r + r + 1.0);
    size_t maxIdx = w * h;
    uint32_t* u32Src = (uint32_t*)src;
    uint32_t* u32Dest = (uint32_t*)dest;

    for (size_t i = 0; i < h; i++) {
        int ti = i * w;
        int li = ti;
        int ri = ti + r;

        unsigned fv0 = readOne(&u32Src[ti], 0);
        unsigned lv0 = readOne(&u32Src[ti + w - 1], 0);
        unsigned val0 = (r + 1) * fv0;

        unsigned fv1 = readOne(&u32Src[ti], 1);
        unsigned lv1 = readOne(&u32Src[ti + w - 1], 1);
        unsigned val1 = (r + 1) * fv1;

        unsigned fv2 = readOne(&u32Src[ti], 2);
        unsigned lv2 = readOne(&u32Src[ti + w - 1], 2);
        unsigned val2 = (r + 1) * fv2;

        unsigned fv3 = readOne(&u32Src[ti], 3);
        unsigned lv3 = readOne(&u32Src[ti + w - 1], 3);
        unsigned val3 = (r + 1) * fv3;

        for (int j = 0; j < r; j++) {
            BOUND_CHECK(ti + j, maxIdx);

            val0 += readOne(&u32Src[ti + j], 0);
            val1 += readOne(&u32Src[ti + j], 1);
            val2 += readOne(&u32Src[ti + j], 2);
            val3 += readOne(&u32Src[ti + j], 3);
        }

        for (int j = 0; j <= r; j++) {
            BOUND_CHECK(ri, maxIdx);
            BOUND_CHECK(ti, maxIdx);

            val0 += readOne(&u32Src[ri], 0) - fv0;
            val1 += readOne(&u32Src[ri], 1) - fv1;
            val2 += readOne(&u32Src[ri], 2) - fv2;
            val3 += readOne(&u32Src[ri], 3) - fv3;
            ri++;

            writeOne((uint32_t*)&u32Dest[ti], 0, ROUND(val0 * iarr));
            writeOne((uint32_t*)&u32Dest[ti], 1, ROUND(val1 * iarr));
            writeOne((uint32_t*)&u32Dest[ti], 2, ROUND(val2 * iarr));
            writeOne((uint32_t*)&u32Dest[ti], 3, ROUND(val3 * iarr));
            ti++;
        }
        for (int j = r + 1; j < (int)(w - r); j++) {
            BOUND_CHECK(ri, maxIdx);
            BOUND_CHECK(li, maxIdx);
            BOUND_CHECK(ti, maxIdx);

            val0 += readOne(&u32Src[ri], 0) - readOne(&u32Src[li], 0);
            val1 += readOne(&u32Src[ri], 1) - readOne(&u32Src[li], 1);
            val2 += readOne(&u32Src[ri], 2) - readOne(&u32Src[li], 2);
            val3 += readOne(&u32Src[ri], 3) - readOne(&u32Src[li], 3);
            ri++;
            li++;

            writeOne((uint32_t*)&u32Dest[ti], 0, ROUND(val0 * iarr));
            writeOne((uint32_t*)&u32Dest[ti], 1, ROUND(val1 * iarr));
            writeOne((uint32_t*)&u32Dest[ti], 2, ROUND(val2 * iarr));
            writeOne((uint32_t*)&u32Dest[ti], 3, ROUND(val3 * iarr));
            ti++;
        }
        for (int j = w - r; j < (int)w; j++) {
            BOUND_CHECK(li, maxIdx);
            BOUND_CHECK(ti, maxIdx);

            val0 += lv0 - readOne(&u32Src[li], 0);
            val1 += lv1 - readOne(&u32Src[li], 1);
            val2 += lv2 - readOne(&u32Src[li], 2);
            val3 += lv3 - readOne(&u32Src[li], 3);
            li++;

            writeOne((uint32_t*)&u32Dest[ti], 0, ROUND(val0 * iarr));
            writeOne((uint32_t*)&u32Dest[ti], 1, ROUND(val1 * iarr));
            writeOne((uint32_t*)&u32Dest[ti], 2, ROUND(val2 * iarr));
            writeOne((uint32_t*)&u32Dest[ti], 3, ROUND(val3 * iarr));
            ti++;
        }
    }
}

static void boxBlurT(uint8_t* src, uint8_t* dest, size_t w, size_t h,
                     size_t stride, int r)
{
    double iarr = 1.0 / (r + r + 1.0);
    size_t maxIdx = w * h;
    uint32_t* u32Src = (uint32_t*)src;
    uint32_t* u32Dest = (uint32_t*)dest;

    for (int i = 0; i < (int)w; i++) {
        int ti = i;
        int li = ti;
        int ri = ti + r * w;

        unsigned fv0 = readOne(&u32Src[ti], 0);
        unsigned lv0 = readOne(&u32Src[ti + w * (h - 1)], 0);
        unsigned val0 = (r + 1) * fv0;

        unsigned fv1 = readOne(&u32Src[ti], 1);
        unsigned lv1 = readOne(&u32Src[ti + w * (h - 1)], 1);
        unsigned val1 = (r + 1) * fv1;

        unsigned fv2 = readOne(&u32Src[ti], 2);
        unsigned lv2 = readOne(&u32Src[ti + w * (h - 1)], 2);
        unsigned val2 = (r + 1) * fv2;

        unsigned fv3 = readOne(&u32Src[ti], 3);
        unsigned lv3 = readOne(&u32Src[ti + w * (h - 1)], 3);
        unsigned val3 = (r + 1) * fv3;

        for (int j = 0; j < r; j++) {
            BOUND_CHECK(ti + j * w, maxIdx);
            val0 += readOne(&u32Src[ti + j * w], 0);
            val1 += readOne(&u32Src[ti + j * w], 1);
            val2 += readOne(&u32Src[ti + j * w], 2);
            val3 += readOne(&u32Src[ti + j * w], 3);
        }
        for (int j = 0; j <= r; j++) {
            BOUND_CHECK(ri, maxIdx);
            BOUND_CHECK(ti, maxIdx);

            val0 += readOne(&u32Src[ri], 0) - fv0;
            val1 += readOne(&u32Src[ri], 1) - fv1;
            val2 += readOne(&u32Src[ri], 2) - fv2;
            val3 += readOne(&u32Src[ri], 3) - fv3;
            writeOne(&u32Dest[ti], 0, ROUND(val0 * iarr));
            writeOne(&u32Dest[ti], 1, ROUND(val1 * iarr));
            writeOne(&u32Dest[ti], 2, ROUND(val2 * iarr));
            writeOne(&u32Dest[ti], 3, ROUND(val3 * iarr));
            ri += w;
            ti += w;
        }

        for (int j = r + 1; j < (int)(h - r); j++) {
            BOUND_CHECK(ri, maxIdx);
            BOUND_CHECK(li, maxIdx);

            val0 += readOne(&u32Src[ri], 0) - readOne(&u32Src[li], 0);
            val1 += readOne(&u32Src[ri], 1) - readOne(&u32Src[li], 1);
            val2 += readOne(&u32Src[ri], 2) - readOne(&u32Src[li], 2);
            val3 += readOne(&u32Src[ri], 3) - readOne(&u32Src[li], 3);

            writeOne(&u32Dest[ti], 0, ROUND(val0 * iarr));
            writeOne(&u32Dest[ti], 1, ROUND(val1 * iarr));
            writeOne(&u32Dest[ti], 2, ROUND(val2 * iarr));
            writeOne(&u32Dest[ti], 3, ROUND(val3 * iarr));

            li += w;
            ri += w;
            ti += w;
        }

        for (int j = h - r; j < (int)h; j++) {
            BOUND_CHECK(li, maxIdx);
            BOUND_CHECK(ti, maxIdx);

            val0 += lv0 - readOne(&u32Src[li], 0);
            val1 += lv1 - readOne(&u32Src[li], 1);
            val2 += lv2 - readOne(&u32Src[li], 2);
            val3 += lv3 - readOne(&u32Src[li], 3);

            writeOne(&u32Dest[ti], 0, val0 * iarr);
            writeOne(&u32Dest[ti], 1, val1 * iarr);
            writeOne(&u32Dest[ti], 2, val2 * iarr);
            writeOne(&u32Dest[ti], 3, val3 * iarr);

            li += w;
            ti += w;
        }
    }
}

static void boxBlur(uint8_t* src, uint8_t* dest, size_t w, size_t h,
                    size_t stride, int r)
{
    memcpy(dest, src, stride * h);
    boxBlurH(dest, src, w, h, stride, r);
    boxBlurT(src, dest, w, h, stride, r);
}

static void gaussBlur(uint8_t* src, uint8_t* dest, size_t w, size_t h,
                      size_t stride, int* bxs)
{
    boxBlur(src, dest, w, h, stride, (bxs[0] - 1) / 2);
    boxBlur(dest, src, w, h, stride, (bxs[1] - 1) / 2);
    boxBlur(src, dest, w, h, stride, (bxs[2] - 1) / 2);
}

static void boxesForGauss(int* bxs, int n, int r)
{
    double sigma = r * 1.0;
    double wIdeal = sqrt((IDEAL_VALUE * sigma * sigma / n) + 1);

    int wl = floor(wIdeal);
    if (wl % 2 == 0) {
        wl--;
    }
    int wu = wl + 2;

    double mIdeal = (double)(IDEAL_VALUE * sigma * sigma - n * wl * wl -
                             4 * n * wl - 3 * n) /
                    (-4 * wl - 4);

    int m = floor(mIdeal);

    for (int i = 0; i < n; i++) {
        bxs[i] = (i < m) ? wl : wu;
    }
}

void ShadowBlur::process(float radius)
{
    int r = ceill(radius);
    r = std::min(r, (int)RADIUS_LIMIT);
    int bxs[3] = {
        0,
    };

    boxesForGauss(bxs, 3, r);

    gaussBlur(m_source, m_workspace.get(), m_width, m_height, m_stride, bxs);
    memcpy(m_source, m_workspace.get(), m_stride * m_height);
}
}
