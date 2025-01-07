/*
 * Copyright (c) 2025-present Samsung Electronics Co., Ltd
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
#include "Starfish.h"

#include "core/style/StrokeLineJoin.h"

namespace Starfish {
String* strokeLineJoinToString(StrokeLineJoin lineJoin)
{
    if (lineJoin == StrokeLineJoin::Round) {
        return String::createASCIIStringWithNoGC("round");
    } else if (lineJoin == StrokeLineJoin::Bevel) {
        return String::createASCIIString("bevel");
    } else {
        STARFISH_RELEASE_ASSERT(lineJoin == StrokeLineJoin::Miter);
        return String::createASCIIString("miter");
    }
}

bool stringToStrokeLineJoin(String* lineJoin, StrokeLineJoin& out)
{
    if (lineJoin->equals("round", 5) == true) {
        out = StrokeLineJoin::Round;
        return true;
    } else if (lineJoin->equals("bevel", 5) == true) {
        out = StrokeLineJoin::Bevel;
        return true;
    } else if (lineJoin->equals("miter", 5) == true) {
        out = StrokeLineJoin::Miter;
        return true;
    }

    return false;
}
} // namespace Starfish
