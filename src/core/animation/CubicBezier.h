/*
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

#ifndef __StarfishCubicBezier__
#define __StarfishCubicBezier__

#include "core/animation/TimingFunction.h"

namespace Starfish {

class CubicBezier : public TimingFunction {
public:
    enum class EaseType {
        LINEAR,
        EASE,
        EASE_IN,
        EASE_OUT,
        EASE_IN_OUT,
        CUSTOM
    };

    static CubicBezier* createCubicBezier(EaseType type);

    CubicBezier(float X1, float Y1, float X2, float Y2,
                EaseType type = EaseType::CUSTOM);
    float getValue(float x) override;

    TimingFunctionType timingFunctionType()
    {
        return TimingFunctionType::CUBIC_BEZIER;
    }

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
        StringBuilder builder;
        if (m_easeType == EaseType::CUSTOM) {
            float x1 = m_coffX3 / 3.0;
            float y1 = m_coffY3 / 3.0;
            float x2 = (m_coffX2 + m_coffX3) / 3.0 + x1;
            float y2 = (m_coffY2 + m_coffY3) / 3.0 + y1;

            builder.appendString("cubic-bezier(");
            builder.appendString(String::fromFloat(x1));
            builder.appendString(", ");
            builder.appendString(String::fromFloat(y1));
            builder.appendString(", ");
            builder.appendString(String::fromFloat(x2));
            builder.appendString(", ");
            builder.appendString(String::fromFloat(y2));
            builder.appendString(")");
        } else if (m_easeType == EaseType::EASE) {
            builder.appendString("ease");
        } else if (m_easeType == EaseType::EASE_IN) {
            builder.appendString("ease-in");
        } else if (m_easeType == EaseType::EASE_OUT) {
            builder.appendString("ease-out");
        } else if (m_easeType == EaseType::EASE_IN_OUT) {
            builder.appendString("ease-in-out");
        } else if (m_easeType == EaseType::LINEAR) {
            builder.appendString("linear");
        }

        return builder.finalize();
    }
    float getCurveX(float x, float epsilon);
    float getValueX(float x);
    float curveDerivativeX(float t);

    bool operator==(const CubicBezier& b) const
    {
        if (m_coffX1 != b.m_coffX1) {
            return false;
        }
        if (m_coffX2 != b.m_coffX2) {
            return false;
        }
        if (m_coffX3 != b.m_coffX3) {
            return false;
        }
        if (m_coffY1 != b.m_coffY1) {
            return false;
        }
        if (m_coffY2 != b.m_coffY2) {
            return false;
        }
        if (m_coffY3 != b.m_coffY3) {
            return false;
        }

        if (m_startGradient != b.m_startGradient) {
            return false;
        }
        if (m_endGradient != b.m_endGradient) {
            return false;
        }
        return true;
    }

private:
    float m_coffX1;
    float m_coffX2;
    float m_coffX3;
    float m_coffY1;
    float m_coffY2;
    float m_coffY3;

    float m_startGradient;
    float m_endGradient;

    EaseType m_easeType;
};
} // namespace Starfish
#endif
