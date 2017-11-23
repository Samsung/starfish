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

#ifndef __StarFishSteps__
#define __StarFishSteps__

#include "core/animation/AnimationTimingFunction.h"

namespace StarFish {

class Steps : public AnimationTimingFunction {
public:
    Steps(size_t numberOfSteps, bool isEndDirection)
        : m_isEndDirection(isEndDirection)
        , m_numberOfSteps(numberOfSteps)
    {
    }

    float getValue(float t)
    {
        if (m_isEndDirection)
            return floor(m_numberOfSteps * t) / m_numberOfSteps;
        return std::min(1.0, ((double)floor(m_numberOfSteps * t) + 1) /
                                 m_numberOfSteps);
    }

    static void* operator new(size_t size)
    {
        return GC_MALLOC_ATOMIC(size);
    }

private:
    bool m_isEndDirection;
    size_t m_numberOfSteps;
};
}
#endif
