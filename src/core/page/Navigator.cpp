/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#include "Navigator.h"
#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/modules/location/Geolocation.h"
#include <sys/utsname.h>

namespace StarFish {

Navigator::Navigator(Document* document)
    : ScriptWrappable(this)
    , DocumentHoldable(document)
    , m_geolocation(nullptr)
{
}

Geolocation* Navigator::geolocation()
{
    if (m_geolocation == nullptr) {
        m_geolocation = Geolocation::create(document());
    }
    return m_geolocation;
}

void Navigator::dispose()
{
    if (m_geolocation) {
        m_geolocation->dispose();
    }
}

ScriptBindingInstance* Navigator::scriptBindingInstance()
{
    return document()->scriptBindingInstance();
}

String* Navigator::userAgent()
{
    return starFish()->userAgent();
}

String* Navigator::platform()
{
    // Unix-like systems
    struct utsname osname;
    StringBuilder platformName;
    if (uname(&osname) == 0) {
        platformName.appendString(osname.sysname);
        platformName.appendString(String::spaceString);
        platformName.appendString(osname.machine);
    }
    return platformName.finalize();
}

String* Navigator::language()
{
    return String::fromUTF8(starFish()->locale().getName());
}
}
