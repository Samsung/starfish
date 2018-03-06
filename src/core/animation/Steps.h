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
