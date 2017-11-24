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
#include "binding/DocumentHoldable.h"

namespace StarFish {

class StarFish;
class Geolocation;

class Navigator : public ScriptWrappable, public DocumentHoldable {
public:
    Navigator(Document* document);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isNavigator() const override;
    virtual ScriptBindingInstance* scriptBindingInstance();

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
        return userAgent();
    }

    String* platform();

    String* product()
    {
        return String::createASCIIString(PRODUCT_NAME);
    }

    String* vendor()
    {
        return String::createASCIIString(VENDOR_NAME);
    }

    String* vendorSub()
    {
        return String::emptyString;
    }

    String* userAgent();

    Geolocation* geolocation();

    void dispose();

protected:
    Geolocation* m_geolocation;
};
}
#endif
