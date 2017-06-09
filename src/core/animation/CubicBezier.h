/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

// This is conversion of bezier-easing project
// I tried to save as much as possible from the original contents.
// Here is original file header information

/**
 * https://github.com/gre/bezier-easing
 * BezierEasing - use bezier curve for transition easing function
 * by Gaëtan Renaudeau 2014 - 2015 – MIT License
 */

#ifndef __StarFishCubicBezier__
#define __StarFishCubicBezier__

namespace StarFish {

// These values are established by empiricism with tests (tradeoff: performance
// VS precision)
#define NEWTON_ITERATIONS 4
#define NEWTON_MIN_SLOPE 0.001
#define SUBDIVISION_PRECISION 0.0000001
#define SUBDIVISION_MAX_ITERATIONS 10
#define kSplineTableSize 11
#define kSampleStepSize 1.0 / (kSplineTableSize - 1.0)

class CubicBeizer : public gc {
public:
    CubicBeizer(float mX1, float mY1, float mX2, float mY2);
    float getValue(float x);

private:
    float getTForX(float aX);
    float A(float aA1, float aA2);
    float B(float aA1, float aA2);
    float C(float aA1);
    float calcBezier(float aT, float aA1, float aA2);
    float getSlope(float aT, float aA1, float aA2);
    float binarySubdivide(float aX, float aA, float aB, float mX1, float mX2);
    float newtonRaphsonIterate(float aX, float aGuessT, float mX1, float mX2);

    float m_mX1;
    float m_mY1;
    float m_mX2;
    float m_mY2;
    float m_sampleValues[kSplineTableSize];
};
}
#endif
