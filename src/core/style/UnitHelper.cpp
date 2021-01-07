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

#include "StarfishConfig.h"
#include "UnitHelper.h"

namespace Starfish {
namespace UnitHelper {
    float convertFromCmToPx(float value)
    {
        return value * UNIT_PX_PER_CM;
    }

    float convertFromMmToPx(float value)
    {
        return value * UNIT_PX_PER_MM;
    }

    float convertFromInToPx(float value)
    {
        return value * UNIT_PX_PER_IN;
    }

    float convertFromPcToPx(float value)
    {
        return value * UNIT_PX_PER_PC;
    }

    float convertFromPtToPx(float value)
    {
        return value * UNIT_PX_PER_PT;
    }

    float convertFromPxToPt(float value)
    {
        return value / UNIT_PX_PER_PT;
    }

    float convertFromGradToDeg(float value)
    {
        return value * 360 / 400;
    }

    float convertFromRadToDeg(float value)
    {
        return value * 180 / PI;
    }

    float convertFromTurnToDeg(float value)
    {
        return value * 360;
    }

    float convertFromDegToRad(float value)
    {
        return value * static_cast<float>(M_PI) / 180.0f;
    }
} // namespace UnitHelper
} // namespace Starfish
