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

#include "StarFishConfig.h"
#include "core/animation/CubicBezier.h"
#include <cmath>

namespace StarFish {
CubicBeizer::CubicBeizer(float X1, float Y1, float X2, float Y2)
{
    STARFISH_ASSERT(0 <= X1 && X1 <= 1);
    STARFISH_ASSERT(0 <= X2 && X2 <= 1);

    m_coff3 = 3.0 * Y1;
    m_coff2 = 3.0 * (Y2 - Y1) - m_coff3;
    m_coff1 = 1 - m_coff3 - m_coff2;
}

float CubicBeizer::getValue(float x)
{
    float square = x * x;
    float cube = square * x;

    return (m_coff1 * cube) + (m_coff2 * square) + (m_coff3 * x);
}
}
