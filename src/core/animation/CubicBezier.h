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
