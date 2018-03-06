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
