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

#include "core/style/StrokeLineCap.h"

namespace Starfish {
String* strokeLineCapToString(StrokeLineCap lineCap)
{
    if (lineCap == StrokeLineCap::Round) {
        return String::createASCIIString("round");
    } else if (lineCap == StrokeLineCap::Square) {
        return String::createASCIIString("square");
    } else {
        STARFISH_RELEASE_ASSERT(lineCap == StrokeLineCap::Butt);
        return String::createASCIIString("butt");
    }
}

bool stringToStrokeLineCap(String* lineCap, StrokeLineCap& out)
{
    if (lineCap->equals("round", 5) == true) {
        out = StrokeLineCap::Round;
        return true;
    } else if (lineCap->equals("square", 6) == true) {
        out = StrokeLineCap::Square;
        return true;
    } else if (lineCap->equals("butt", 4) == true) {
        out = StrokeLineCap::Butt;
        return true;
    }

    return false;
}
} // namespace Starfish
