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

class CubicBezier : public AnimationTimingFunction {
public:
    CubicBezier(float X1, float Y1, float X2, float Y2);
    float getValue(float x);

    static void* operator new(size_t size)
    {
        return GC_MALLOC_ATOMIC(size);
    }

    bool isCubicBezier() const override
    {
        return true;
    }

    String* toString() const override
    {
        float x1 = m_coffX3 / 3.0;
        float y1 = m_coffY3 / 3.0;
        float x2 = (m_coffX2 + m_coffX3) / 3.0 + x1;
        float y2 = (m_coffY2 + m_coffY3) / 3.0 + y1;

        StringBuilder builder;
        builder.appendString("cubic-bezier(");
        builder.appendString(String::fromFloat(x1));
        builder.appendString(", ");
        builder.appendString(String::fromFloat(y1));
        builder.appendString(", ");
        builder.appendString(String::fromFloat(x2));
        builder.appendString(", ");
        builder.appendString(String::fromFloat(y2));
        builder.appendString(")");
        return builder.finalize();
    }

private:
    float m_coffX1;
    float m_coffX2;
    float m_coffX3;
    float m_coffY1;
    float m_coffY2;
    float m_coffY3;
};
}
#endif
