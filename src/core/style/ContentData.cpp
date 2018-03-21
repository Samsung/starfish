/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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
#include "core/style/ContentData.h"
#include "core/style/CounterStyle.h"

namespace StarFish {

bool CounterContentData::equals(const CounterContentData* b) const
{
    if (!id()->equals(b->id())) {
        return false;
    }
    if (separator().hasValue() != b->separator().hasValue()) {
        return false;
    }
    if (separator().hasValue() &&
        !separator().getValue()->equals(b->separator().getValue())) {
        return false;
    }
    STARFISH_ASSERT(counterStyle());
    STARFISH_ASSERT(b->counterStyle());
    return counterStyle()->equals(b->counterStyle());
}
} /* namespace StarFish */
