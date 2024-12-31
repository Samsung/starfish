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

enum class CubicBezierEaseType : uint8_t {
    Linear,
    Ease,
    Easein,
    EaseOut,
    EaseInout,
    Custom,
};

class CubicBezier : public TimingFunction {
public:
    static CubicBezier* createCubicBezier(CubicBezierEaseType type);

    CubicBezier(double X1, double Y1, double X2, double Y2,
                CubicBezierEaseType type = CubicBezierEaseType::Custom);
    double getValue(double x) override;

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
        if (m_easeType == CubicBezierEaseType::Custom) {
            double x1 = m_coffX3 / 3.0;
            double y1 = m_coffY3 / 3.0;
            double x2 = (m_coffX2 + m_coffX3) / 3.0 + x1;
            double y2 = (m_coffY2 + m_coffY3) / 3.0 + y1;

            builder.appendString("cubic-bezier(");
            builder.appendString(String::fromDouble(x1));
            builder.appendString(", ");
            builder.appendString(String::fromDouble(y1));
            builder.appendString(", ");
            builder.appendString(String::fromDouble(x2));
            builder.appendString(", ");
            builder.appendString(String::fromDouble(y2));
            builder.appendString(")");
        } else if (m_easeType == CubicBezierEaseType::Ease) {
            builder.appendString("ease");
        } else if (m_easeType == CubicBezierEaseType::Easein) {
            builder.appendString("ease-in");
        } else if (m_easeType == CubicBezierEaseType::EaseOut) {
            builder.appendString("ease-out");
        } else if (m_easeType == CubicBezierEaseType::EaseInout) {
            builder.appendString("ease-in-out");
        } else if (m_easeType == CubicBezierEaseType::Linear) {
            builder.appendString("linear");
        }

        return builder.finalize();
    }

    double getCurveX(double x, double epsilon);

    double getValueX(double x);

    double curveDerivativeX(double t);

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
    double m_coffX1;
    double m_coffX2;
    double m_coffX3;
    double m_coffY1;
    double m_coffY2;
    double m_coffY3;

    double m_startGradient;
    double m_endGradient;

    CubicBezierEaseType m_easeType;
};
} // namespace Starfish
#endif
