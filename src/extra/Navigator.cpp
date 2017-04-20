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
#include "Navigator.h"
#include "platform/location/Geolocation.h"

namespace StarFish {

Navigator::Navigator(StarFish* starFish)
    : ScriptWrappable(this)
    , m_starFish(starFish)
    , m_geolocation(nullptr)
{
}

Geolocation* Navigator::geolocation()
{
    if (m_geolocation == nullptr) {
        m_geolocation = Geolocation::create(m_starFish);
    }
    return m_geolocation;
}

void Navigator::close()
{
    if (m_geolocation) {
        m_geolocation->close();
    }
}
}
