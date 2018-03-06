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

#ifndef __StarFishCubicBezier__
#define __StarFishCubicBezier__

#include "core/animation/AnimationTimingFunction.h"

namespace StarFish {

class CubicBeizer : public AnimationTimingFunction {
public:
    CubicBeizer(float X1, float Y1, float X2, float Y2);
    float getValue(float x);

    static void* operator new(size_t size)
    {
        return GC_MALLOC_ATOMIC(size);
    }

private:
    float m_coff1;
    float m_coff2;
    float m_coff3;
};
}
#endif
