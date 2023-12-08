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

#include "StarfishConfig.h"
#include "core/animation/CubicBezier.h"
#include <cmath>

namespace Starfish {

static constexpr double bezierEpsilon = 1e-4;

CubicBezier* CubicBezier::createCubicBezier(EaseType type)
{
    switch (type) {
    case EaseType::EASE:
        return new CubicBezier(0.25, 1.0, 0.25, 1.0, type);
    case EaseType::LINEAR:
        return new CubicBezier(0.0, 0.0, 1.0, 1.0, type);
    case EaseType::EASE_IN:
        return new CubicBezier(0.42, 0, 1, 1, type);
    case EaseType::EASE_OUT:
        return new CubicBezier(0.0, 0.0, 0.58, 1.0, type);
    case EaseType::EASE_IN_OUT:
        return new CubicBezier(0.42, 0.0, 0.58, 1.0, type);
    default:
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
}

CubicBezier::CubicBezier(double X1, double Y1, double X2, double Y2,
                         EaseType type)
    : m_easeType(type)
{
    STARFISH_ASSERT(0 <= X1 && X1 <= 1);
    STARFISH_ASSERT(0 <= X2 && X2 <= 1);

    m_coffY3 = 3.0 * Y1;
    m_coffY2 = 3.0 * (Y2 - Y1) - m_coffY3;
    m_coffY1 = 1 - m_coffY3 - m_coffY2;

    m_coffX3 = 3.0 * X1;
    m_coffX2 = 3.0 * (X2 - X1) - m_coffX3;
    m_coffX1 = 1 - m_coffX3 - m_coffX2;

    if (X1 > 0) {
        m_startGradient = Y1 / X1;
    } else if (!Y1 && X2 > 0) {
        m_startGradient = Y2 / X2;
    } else {
        m_startGradient = 0;
    }

    if (X2 < 1) {
        m_endGradient = (Y2 - 1) / (X2 - 1);
    } else if (X2 == 1 && X1 < 1) {
        m_endGradient = (Y1 - 1) / (X1 - 1);
    } else {
        m_endGradient = 0;
    }
}

double CubicBezier::getValue(double t)
{
    if (t < 0.0) {
        return 0.0 + m_startGradient * t;
    } else if (t > 1.0) {
        return 1.0 + m_endGradient * (t - 1.0);
    }

    double x = getCurveX(t, bezierEpsilon);
    return ((m_coffY1 * x + m_coffY2) * x + m_coffY3) * x;
}

double CubicBezier::getValueX(double t)
{
    return ((m_coffX1 * t + m_coffX2) * t + m_coffX3) * t;
}

double CubicBezier::curveDerivativeX(double t)
{
    return (3.0 * m_coffX1 * t + 2.0 * m_coffX2) * t + m_coffX3;
}

double CubicBezier::getCurveX(double x, double epsilon)
{
    double t0;
    double t1;
    double t2;
    double x2;
    double d2;
    int i;
    // First try a few iterations of Newton's method -- normally very fast.
    for (t2 = x, i = 0; i < 8; i++) {
        x2 = getValueX(t2) - x;
        if (fabs(x2) < epsilon) {
            return t2;
        }
        d2 = curveDerivativeX(t2);
        if (fabs(d2) < 1e-6) {
            break;
        }
        t2 = t2 - x2 / d2;
    }
    // Fall back to the bisection method for reliability.
    t0 = 0.0;
    t1 = 1.0;
    t2 = x;
    while (t0 < t1) {
        x2 = getValueX(t2);
        if (fabs(x2 - x) < epsilon) {
            return t2;
        }
        if (x > x2) {
            t0 = t2;
        } else {
            t1 = t2;
        }
        t2 = (t1 - t0) * 0.5 + t0;
    }
    // Failure.
    return t2;
}
} // namespace Starfish
