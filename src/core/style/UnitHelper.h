/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishUnitHelper__
#define __StarfishUnitHelper__

namespace Starfish {
namespace UnitHelper {
    // https://www.w3.org/TR/CSS2/syndata.html#value-def-length
    const float UNIT_PX_PER_CM = 96 / 2.54;
    const float UNIT_PX_PER_MM = UNIT_PX_PER_CM / 10;
    const float UNIT_PX_PER_IN = 96;
    const float UNIT_PX_PER_PC = UNIT_PX_PER_IN / 6;
    const float UNIT_PX_PER_PT = UNIT_PX_PER_IN / 72;

    float convertFromCmToPx(float value);
    float convertFromPxToCm(float value);
    float convertFromMmToPx(float value);
    float convertFromPxToMm(float value);
    float convertFromInToPx(float value);
    float convertFromPxToIn(float value);
    float convertFromPcToPx(float value);
    float convertFromPxToPc(float value);
    float convertFromPtToPx(float value);
    float convertFromPxToPt(float value);

    // https://www.w3.org/TR/css3-values/#angle-value
    const float PI = static_cast<float>(M_PI);

    float convertFromGradToDeg(float value);
    float convertFromRadToDeg(float value);
    float convertFromTurnToDeg(float value);
    float convertFromDegToRad(float value);
    float convertFromDegToGrad(float value);
} // namespace UnitHelper
} // namespace Starfish

#endif
