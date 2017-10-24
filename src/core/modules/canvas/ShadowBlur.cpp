/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "StarFishConfig.h"
#include "ShadowBlur.h"

#define BOUND_CHECK(idx)                         \
    if (idx < 0 || idx >= colorBufferLength()) { \
        break;                                   \
    }

#define ROUND(v) (int)((v) + 0.5)
#define IDEAL_VALUE 3

namespace StarFish {

const float ShadowBlur::RADIUS_LIMIT = 250.0f;

ShadowBlur::ShadowBlur(unsigned char* source, int& width, int& height,
                       int& stride, int channel)
    : m_width(width)
    , m_height(height)
    , m_stride(stride)
    , m_channel(channel)
    , m_initialized(false)
    , m_source(source)
    , m_alpha(nullptr)
    , m_red(nullptr)
    , m_green(nullptr)
    , m_blue(nullptr)
{
    m_initialized = init();
}

ShadowBlur::~ShadowBlur()
{
    dispose();
}

bool ShadowBlur::init()
{
    if (m_channel != 4) {
        // currently, only support ARGB32
        return false;
    }

    int len = colorBufferLength();

    m_alpha = (unsigned*)(malloc(sizeof(unsigned) * len));
    m_red = (unsigned*)(malloc(sizeof(unsigned) * len));
    m_green = (unsigned*)(malloc(sizeof(unsigned) * len));
    m_blue = (unsigned*)(malloc(sizeof(unsigned) * len));

    if (!m_alpha || !m_red || !m_green || !m_blue) {
        dispose();
        return false;
    }

    int j = 0;
    for (int i = 0; i < sourceBufferLength(); i += m_channel, j++) {
        m_alpha[j] = m_source[i + 3];
        m_red[j] = m_source[i + 2];
        m_green[j] = m_source[i + 1];
        m_blue[j] = m_source[i];
    }

    return true;
}

void ShadowBlur::dispose()
{
    free(m_alpha);
    free(m_red);
    free(m_green);
    free(m_blue);
}

bool ShadowBlur::process(float radius)
{
    int len = colorBufferLength();

    unsigned* newAlpha = (unsigned*)(malloc(sizeof(unsigned) * len));
    unsigned* newRed = (unsigned*)(malloc(sizeof(unsigned) * len));
    unsigned* newGreen = (unsigned*)(malloc(sizeof(unsigned) * len));
    unsigned* newBlue = (unsigned*)(malloc(sizeof(unsigned) * len));

    if (!newAlpha || !newRed || !newGreen || !newBlue) {
        return false;
    }

    int r = ceill(radius);
    r = std::min(r, (int)RADIUS_LIMIT);
    int bxs[3] = {
        0,
    };

    boxesForGauss(bxs, 3, r);

    gaussBlur(m_alpha, newAlpha, m_width, m_height, bxs);
    gaussBlur(m_red, newRed, m_width, m_height, bxs);
    gaussBlur(m_green, newGreen, m_width, m_height, bxs);
    gaussBlur(m_blue, newBlue, m_width, m_height, bxs);

    int j = 0;
    for (int i = 0; i < sourceBufferLength(); i += m_channel, j++) {
        m_source[i + 3] = (newAlpha[j] <= 255) ? newAlpha[j] : 255;
        m_source[i + 2] = (newRed[j] <= 255) ? newRed[j] : 255;
        m_source[i + 1] = (newGreen[j] <= 255) ? newGreen[j] : 255;
        m_source[i] = (newBlue[j] <= 255) ? newBlue[j] : 255;
    }

    free(newAlpha);
    free(newRed);
    free(newGreen);
    free(newBlue);

    return true;
}

void ShadowBlur::gaussBlur(unsigned* src, unsigned* dest, int w, int h,
                           int* bxs)
{
    boxBlur(src, dest, w, h, (bxs[0] - 1) / 2);
    boxBlur(dest, src, w, h, (bxs[1] - 1) / 2);
    boxBlur(src, dest, w, h, (bxs[2] - 1) / 2);
}

void ShadowBlur::boxesForGauss(int* bxs, int n, int r)
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

void ShadowBlur::boxBlur(unsigned* src, unsigned* dest, int w, int h, int r)
{
    for (int i = 0; i < w * h; i++) {
        dest[i] = src[i];
    }
    boxBlurH(dest, src, w, h, r);
    boxBlurT(src, dest, w, h, r);
}

void ShadowBlur::boxBlurH(unsigned* src, unsigned* dest, int w, int h, int r)
{
    double iarr = 1.0 / (r + r + 1.0);
    for (int i = 0; i < h; i++) {
        int ti = i * w;
        int li = ti;
        int ri = ti + r;
        unsigned fv = src[ti];
        unsigned lv = src[ti + w - 1];
        unsigned val = (r + 1) * fv;

        for (int j = 0; j < r; j++) {
            BOUND_CHECK(ti + j);
            val += src[ti + j];
        }
        for (int j = 0; j <= r; j++) {
            BOUND_CHECK(ri);
            BOUND_CHECK(ti);
            val += src[ri++] - fv;
            dest[ti++] = ROUND(val * iarr);
        }
        for (int j = r + 1; j < w - r; j++) {
            BOUND_CHECK(ri);
            BOUND_CHECK(li);
            BOUND_CHECK(ti);
            val += src[ri++] - src[li++];
            dest[ti++] = ROUND(val * iarr);
        }
        for (int j = w - r; j < w; j++) {
            BOUND_CHECK(li);
            BOUND_CHECK(ti);
            val += lv - src[li++];
            dest[ti++] = ROUND(val * iarr);
        }
    }
}

void ShadowBlur::boxBlurT(unsigned* src, unsigned* dest, int w, int h, int r)
{
    double iarr = 1.0 / (r + r + 1.0);

    for (int i = 0; i < w; i++) {
        int ti = i;
        int li = ti;
        int ri = ti + r * w;
        unsigned fv = src[ti];
        unsigned lv = src[ti + w * (h - 1)];
        unsigned val = (r + 1) * fv;

        for (int j = 0; j < r; j++) {
            BOUND_CHECK(ti + j * w);
            val += src[ti + j * w];
        }
        for (int j = 0; j <= r; j++) {
            BOUND_CHECK(ri);
            BOUND_CHECK(ti);
            val += src[ri] - fv;
            dest[ti] = ROUND(val * iarr);
            ri += w;
            ti += w;
        }
        for (int j = r + 1; j < h - r; j++) {
            BOUND_CHECK(ri);
            BOUND_CHECK(li);
            val += src[ri] - src[li];
            dest[ti] = ROUND(val * iarr);
            li += w;
            ri += w;
            ti += w;
        }
        for (int j = h - r; j < h; j++) {
            BOUND_CHECK(li);
            BOUND_CHECK(ti);
            val += lv - src[li];
            dest[ti] = ROUND(val * iarr);
            li += w;
            ti += w;
        }
    }
}
}
