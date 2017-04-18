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

#ifndef __StarFishNavigator__
#define __StarFishNavigator__

#include "binding/ScriptWrappable.h"
#include "platform/location/Geolocation.h"

namespace StarFish {

class StarFish;
class Geolocation;

class Navigator : public ScriptWrappable {
public:
    Navigator(StarFish* starFish);
    StarFish* starFish()
    {
        return m_starFish;
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual bool isNavigator() const
    {
        return true;
    }

    String* appCodeName()
    {
        return String::createASCIIString(APP_CODE_NAME);
    }

    String* appName()
    {
        return String::createASCIIString(APP_NAME);
    }

    String* appVersion()
    {
        return String::createASCIIString(USER_AGENT(APP_NAME, VERSION));
    }

    String* vendor()
    {
        return String::createASCIIString(VENDOR_NAME);
    }

    String* userAgent()
    {
        return String::createASCIIString(USER_AGENT(APP_CODE_NAME, VERSION));
    }

    Geolocation* geolocation()
    {
        if (m_geolocation == nullptr) {
            m_geolocation = Geolocation::create(m_starFish);
        }
        return m_geolocation;
    }

    void close()
    {
        if (m_geolocation) {
            m_geolocation->close();
        }
    }

protected:
    StarFish* m_starFish;
    Geolocation* m_geolocation;
};
}

#endif
