/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
}
