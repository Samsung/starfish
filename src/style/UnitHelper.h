/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

#ifndef __StarFishUnitHelper__
#define __StarFishUnitHelper__

#include "StarFish.h"
#include <math.h>

namespace StarFish {

// https://www.w3.org/TR/CSS2/syndata.html#value-def-length
const float unitPxPerCm = 96 / 2.54;
const float unitPxPerMm = unitPxPerCm / 10;
const float unitPxPerIn = 96;
const float unitPxPerPc = unitPxPerIn / 6;
const float unitPxPerPt = unitPxPerIn / 72;

float convertFromCmToPx(float value);
float convertFromMmToPx(float value);
float convertFromInToPx(float value);
float convertFromPcToPx(float value);
float convertFromPtToPx(float value);
float convertFromPxToPt(float value);

// https://www.w3.org/TR/css3-values/#angle-value
const float pi = static_cast<float>(M_PI);

float convertFromGradToDeg(float value);
float convertFromRadToDeg(float value);
float convertFromTurnToDeg(float value);

}

#endif
