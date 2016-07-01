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

#include "StarFishConfig.h"
#include "UnitHelper.h"

namespace StarFish {

float convertFromCmToPx(float value)
{
    return value * unitPxPerCm;
}

float convertFromMmToPx(float value)
{
    return value * unitPxPerMm;
}

float convertFromInToPx(float value)
{
    return value * unitPxPerIn;
}

float convertFromPcToPx(float value)
{
    return value * unitPxPerPc;
}

float convertFromPtToPx(float value)
{
    return value * unitPxPerPt;
}

float convertFromPxToPt(float value)
{
    return value / unitPxPerPt;
}

float convertFromGradToDeg(float value)
{
    return value * 360 / 400;
}

float convertFromRadToDeg(float value)
{
    return value * 180 / pi;
}

float convertFromTurnToDeg(float value)
{
    return value * 360;
}

}
