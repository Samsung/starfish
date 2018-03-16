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
#include "StarFish.h"
#include "core/dom/HTMLListContainer.h"

namespace StarFish {
int32_t HTMLListContainer::start()
{
    Nullable<String*> v = getAttribute(starFish()->staticStrings()->m_start);
    if (v.hasValue()) {
        return String::parseInt(v.getValue());
    }
    return 1;
}

void HTMLListContainer::setStart(int32_t v)
{
    setAttribute(starFish()->staticStrings()->m_start, String::fromInt(v));
}
}
