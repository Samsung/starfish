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

#define BOUND_CHECK(idx, maxIdx) !((idx) < 0 || (int)(idx) >= (int)(maxIdx))

#define ROUND(v) (int)((v) + 0.5)
#define READ_ONE(src, idx) (((uint8_t*)src)[idx])
#define WRITE_ONE(dst, idx, v) (((uint8_t*)dst)[idx] = v)
#define IDEAL_VALUE 3

namespace StarFish {

const float ShadowBlur::RADIUS_LIMIT = 250.0f;

ShadowBlur::ShadowBlur(uint8_t* source, const size_t& width,
                       const size_t& height, const size_t& stride)
    : m_width(width)
    , m_height(height)
    , m_stride(stride)
    , m_source(source)
    , m_workspace(new uint8_t[m_stride * m_height],
                  [](uint8_t* data) { delete[] data; })
{
    // TODO
    STARFISH_RELEASE_ASSERT(m_stride == m_width * 4);
}

ShadowBlur::~ShadowBlur()
{
}

static void boxBlurH(uint8_t* src, uint8_t* dest, size_t w, size_t h,
                     size_t stride, int r)
{
    double iarr = 1.0 / (r + r + 1.0);
    size_t maxIdx = w * h;
    uint32_t* u32Src = (uint32_t*)src;
    uint32_t* u32Dest = (uint32_t*)dest;

    for (size_t i = 0; i < h; ++i) {
        int ti = i * w;
        int li = ti;
        int ri = ti + r;

        const unsigned& fv0 = READ_ONE(&u32Src[ti], 0);
        const unsigned& lv0 = READ_ONE(&u32Src[ti + w - 1], 0);
        unsigned val0 = (r + 1) * fv0;

        const unsigned& fv1 = READ_ONE(&u32Src[ti], 1);
        const unsigned& lv1 = READ_ONE(&u32Src[ti + w - 1], 1);
        unsigned val1 = (r + 1) * fv1;

        const unsigned& fv2 = READ_ONE(&u32Src[ti], 2);
        const unsigned& lv2 = READ_ONE(&u32Src[ti + w - 1], 2);
        unsigned val2 = (r + 1) * fv2;

        const unsigned& fv3 = READ_ONE(&u32Src[ti], 3);
        const unsigned& lv3 = READ_ONE(&u32Src[ti + w - 1], 3);
        unsigned val3 = (r + 1) * fv3;

        for (int j = 0; j < r && BOUND_CHECK(ti + j, maxIdx); ++j) {
            val0 += READ_ONE(&u32Src[ti + j], 0);
            val1 += READ_ONE(&u32Src[ti + j], 1);
            val2 += READ_ONE(&u32Src[ti + j], 2);
            val3 += READ_ONE(&u32Src[ti + j], 3);
        }

        for (int j = 0;
             j <= r && BOUND_CHECK(ri, maxIdx) && BOUND_CHECK(ti, maxIdx);
             ++j) {
            val0 += READ_ONE(&u32Src[ri], 0) - fv0;
            val1 += READ_ONE(&u32Src[ri], 1) - fv1;
            val2 += READ_ONE(&u32Src[ri], 2) - fv2;
            val3 += READ_ONE(&u32Src[ri], 3) - fv3;
            ++ri;

            WRITE_ONE((uint32_t*)&u32Dest[ti], 0, ROUND(val0 * iarr));
            WRITE_ONE((uint32_t*)&u32Dest[ti], 1, ROUND(val1 * iarr));
            WRITE_ONE((uint32_t*)&u32Dest[ti], 2, ROUND(val2 * iarr));
            WRITE_ONE((uint32_t*)&u32Dest[ti], 3, ROUND(val3 * iarr));
            ++ti;
        }
        int limit = (int)(w - r);
        for (int j = r + 1; j < limit && BOUND_CHECK(ri, maxIdx) &&
                            BOUND_CHECK(li, maxIdx) && BOUND_CHECK(ti, maxIdx);
             ++j) {
            val0 += READ_ONE(&u32Src[ri], 0) - READ_ONE(&u32Src[li], 0);
            val1 += READ_ONE(&u32Src[ri], 1) - READ_ONE(&u32Src[li], 1);
            val2 += READ_ONE(&u32Src[ri], 2) - READ_ONE(&u32Src[li], 2);
            val3 += READ_ONE(&u32Src[ri], 3) - READ_ONE(&u32Src[li], 3);
            ++ri;
            ++li;

            WRITE_ONE((uint32_t*)&u32Dest[ti], 0, ROUND(val0 * iarr));
            WRITE_ONE((uint32_t*)&u32Dest[ti], 1, ROUND(val1 * iarr));
            WRITE_ONE((uint32_t*)&u32Dest[ti], 2, ROUND(val2 * iarr));
            WRITE_ONE((uint32_t*)&u32Dest[ti], 3, ROUND(val3 * iarr));
            ++ti;
        }
        for (int j = w - r;
             j < (int)w && BOUND_CHECK(li, maxIdx) && BOUND_CHECK(ti, maxIdx);
             ++j) {
            val0 += lv0 - READ_ONE(&u32Src[li], 0);
            val1 += lv1 - READ_ONE(&u32Src[li], 1);
            val2 += lv2 - READ_ONE(&u32Src[li], 2);
            val3 += lv3 - READ_ONE(&u32Src[li], 3);
            ++li;

            WRITE_ONE((uint32_t*)&u32Dest[ti], 0, ROUND(val0 * iarr));
            WRITE_ONE((uint32_t*)&u32Dest[ti], 1, ROUND(val1 * iarr));
            WRITE_ONE((uint32_t*)&u32Dest[ti], 2, ROUND(val2 * iarr));
            WRITE_ONE((uint32_t*)&u32Dest[ti], 3, ROUND(val3 * iarr));
            ++ti;
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

    for (int i = 0; i < (int)w; ++i) {
        int ti = i;
        int li = ti;
        int ri = ti + r * w;

        const unsigned& fv0 = READ_ONE(&u32Src[ti], 0);
        const unsigned& lv0 = READ_ONE(&u32Src[ti + w * (h - 1)], 0);
        unsigned val0 = (r + 1) * fv0;

        const unsigned& fv1 = READ_ONE(&u32Src[ti], 1);
        const unsigned& lv1 = READ_ONE(&u32Src[ti + w * (h - 1)], 1);
        unsigned val1 = (r + 1) * fv1;

        const unsigned& fv2 = READ_ONE(&u32Src[ti], 2);
        const unsigned& lv2 = READ_ONE(&u32Src[ti + w * (h - 1)], 2);
        unsigned val2 = (r + 1) * fv2;

        const unsigned& fv3 = READ_ONE(&u32Src[ti], 3);
        const unsigned& lv3 = READ_ONE(&u32Src[ti + w * (h - 1)], 3);
        unsigned val3 = (r + 1) * fv3;

        for (int j = 0; j < r && BOUND_CHECK(ti + j * w, maxIdx); ++j) {
            val0 += READ_ONE(&u32Src[ti + j * w], 0);
            val1 += READ_ONE(&u32Src[ti + j * w], 1);
            val2 += READ_ONE(&u32Src[ti + j * w], 2);
            val3 += READ_ONE(&u32Src[ti + j * w], 3);
        }
        for (int j = 0;
             j <= r && BOUND_CHECK(ri, maxIdx) && BOUND_CHECK(ti, maxIdx);
             ++j) {
            val0 += READ_ONE(&u32Src[ri], 0) - fv0;
            val1 += READ_ONE(&u32Src[ri], 1) - fv1;
            val2 += READ_ONE(&u32Src[ri], 2) - fv2;
            val3 += READ_ONE(&u32Src[ri], 3) - fv3;
            WRITE_ONE(&u32Dest[ti], 0, ROUND(val0 * iarr));
            WRITE_ONE(&u32Dest[ti], 1, ROUND(val1 * iarr));
            WRITE_ONE(&u32Dest[ti], 2, ROUND(val2 * iarr));
            WRITE_ONE(&u32Dest[ti], 3, ROUND(val3 * iarr));
            ri += w;
            ti += w;
        }
        int limit = (int)(h - r);
        for (int j = r + 1;
             j < limit && BOUND_CHECK(ri, maxIdx) && BOUND_CHECK(li, maxIdx);
             ++j) {
            val0 += READ_ONE(&u32Src[ri], 0) - READ_ONE(&u32Src[li], 0);
            val1 += READ_ONE(&u32Src[ri], 1) - READ_ONE(&u32Src[li], 1);
            val2 += READ_ONE(&u32Src[ri], 2) - READ_ONE(&u32Src[li], 2);
            val3 += READ_ONE(&u32Src[ri], 3) - READ_ONE(&u32Src[li], 3);

            WRITE_ONE(&u32Dest[ti], 0, ROUND(val0 * iarr));
            WRITE_ONE(&u32Dest[ti], 1, ROUND(val1 * iarr));
            WRITE_ONE(&u32Dest[ti], 2, ROUND(val2 * iarr));
            WRITE_ONE(&u32Dest[ti], 3, ROUND(val3 * iarr));

            li += w;
            ri += w;
            ti += w;
        }

        for (int j = h - r;
             j < (int)h && BOUND_CHECK(li, maxIdx) && BOUND_CHECK(ti, maxIdx);
             ++j) {
            val0 += lv0 - READ_ONE(&u32Src[li], 0);
            val1 += lv1 - READ_ONE(&u32Src[li], 1);
            val2 += lv2 - READ_ONE(&u32Src[li], 2);
            val3 += lv3 - READ_ONE(&u32Src[li], 3);

            WRITE_ONE(&u32Dest[ti], 0, val0 * iarr);
            WRITE_ONE(&u32Dest[ti], 1, val1 * iarr);
            WRITE_ONE(&u32Dest[ti], 2, val2 * iarr);
            WRITE_ONE(&u32Dest[ti], 3, val3 * iarr);

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
        --wl;
    }
    int wu = wl + 2;

    double mIdeal = (double)(IDEAL_VALUE * sigma * sigma - n * wl * wl -
                             4 * n * wl - 3 * n) /
                    (-4 * wl - 4);

    int m = floor(mIdeal);

    for (int i = 0; i < n; ++i) {
        bxs[i] = (i < m) ? wl : wu;
    }
}

void ShadowBlur::process(float radius)
{
    if (radius <= 0) {
        return;
    }
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
