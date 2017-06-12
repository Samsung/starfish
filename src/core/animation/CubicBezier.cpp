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
float CubicBeizer::A(float aA1, float aA2)
{
    return 1.0 - 3.0 * aA2 + 3.0 * aA1;
}

float CubicBeizer::B(float aA1, float aA2)
{
    return 3.0 * aA2 - 6.0 * aA1;
}

float CubicBeizer::C(float aA1)
{
    return 3.0 * aA1;
}

// Returns x(t) given t, x1, and x2, or y(t) given t, y1, and y2.
float CubicBeizer::calcBezier(float aT, float aA1, float aA2)
{
    return ((A(aA1, aA2) * aT + B(aA1, aA2)) * aT + C(aA1)) * aT;
}

// Returns dx/dt given t, x1, and x2, or dy/dt given t, y1, and y2.
float CubicBeizer::getSlope(float aT, float aA1, float aA2)
{
    return 3.0 * A(aA1, aA2) * aT * aT + 2.0 * B(aA1, aA2) * aT + C(aA1);
}

float CubicBeizer::binarySubdivide(float aX, float aA, float aB, float mX1,
                                   float mX2)
{
    float currentX = 0, currentT = 0;
    int i = 0;
    do {
        currentT = aA + (aB - aA) / 2.0;
        currentX = calcBezier(currentT, mX1, mX2) - aX;
        if (currentX > 0.0) {
            aB = currentT;
        } else {
            aA = currentT;
        }
    } while (std::abs(currentX) > SUBDIVISION_PRECISION &&
             ++i < SUBDIVISION_MAX_ITERATIONS);
    return currentT;
}

float CubicBeizer::newtonRaphsonIterate(float aX, float aGuessT, float mX1,
                                        float mX2)
{
    for (int i = 0; i < NEWTON_ITERATIONS; ++i) {
        float currentSlope = getSlope(aGuessT, mX1, mX2);
        if (currentSlope == 0.0) {
            return aGuessT;
        }
        float currentX = calcBezier(aGuessT, mX1, mX2) - aX;
        aGuessT -= currentX / currentSlope;
    }
    return aGuessT;
}

CubicBeizer::CubicBeizer(float mX1, float mY1, float mX2, float mY2)
{
    if (!(0 <= mX1 && mX1 <= 1 && 0 <= mX2 && mX2 <= 1)) {
        // throw new Error('bezier x values must be in [0, 1] range');
    }

    m_mX1 = mX1;
    m_mY1 = mY1;
    m_mX2 = mX2;
    m_mY2 = mY2;

    // Precompute samples table
    if (mX1 != mY1 || mX2 != mY2) {
        for (int i = 0; i < kSplineTableSize; ++i) {
            m_sampleValues[i] = calcBezier(i * kSampleStepSize, mX1, mX2);
        }
    }
}

float CubicBeizer::getTForX(float aX)
{
    float intervalStart = 0.0;
    int currentSample = 1;
    int lastSample = kSplineTableSize - 1;

    for (; currentSample != lastSample && m_sampleValues[currentSample] <= aX;
         ++currentSample) {
        intervalStart += kSampleStepSize;
    }
    --currentSample;

    // Interpolate to provide an initial guess for t
    float dist =
        (aX - m_sampleValues[currentSample]) /
        (m_sampleValues[currentSample + 1] - m_sampleValues[currentSample]);
    float guessForT = intervalStart + dist * kSampleStepSize;

    float initialSlope = getSlope(guessForT, m_mX1, m_mX2);
    if (initialSlope >= NEWTON_MIN_SLOPE) {
        return newtonRaphsonIterate(aX, guessForT, m_mX1, m_mX2);
    } else if (initialSlope == 0.0) {
        return guessForT;
    } else {
        return binarySubdivide(aX, intervalStart,
                               intervalStart + kSampleStepSize, m_mX1, m_mX2);
    }
}

float CubicBeizer::getValue(float x)
{
    if (m_mX1 == m_mY1 && m_mX2 == m_mY2) {
        return x; // linear
    }
    return calcBezier(getTForX(x), m_mY1, m_mY2);
}
}
