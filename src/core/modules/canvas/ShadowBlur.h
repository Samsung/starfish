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

#ifndef __StarFishShadowBlur__
#define __StarFishShadowBlur__

#include "core/style/Unit.h"

namespace StarFish {
class ShadowBlur {
public:
    ShadowBlur(unsigned char* source, int& width, int& height, int& stride,
               int channel);
    ~ShadowBlur();
    bool process(float radius);

    static const float RADIUS_LIMIT;

private:
    bool init();
    void dispose();
    void gaussBlur(unsigned* src, unsigned* dest, int w, int h, int* bxs);
    void boxesForGauss(int* bxs, int n, int r);
    void boxBlur(unsigned* src, unsigned* dest, int w, int h, int r);
    void boxBlurH(unsigned* src, unsigned* dest, int w, int h, int r);
    void boxBlurT(unsigned* src, unsigned* dest, int w, int h, int r);

    int sourceBufferLength()
    {
        return m_height * m_stride;
    }

    int colorBufferLength()
    {
        return m_height * m_width;
    }

    LayoutRect rect;
    int m_width;
    int m_height;
    int m_stride;
    int m_channel;
    bool m_initialized;
    unsigned char* m_source;
    unsigned* m_alpha;
    unsigned* m_red;
    unsigned* m_green;
    unsigned* m_blue;
};
}
#endif
