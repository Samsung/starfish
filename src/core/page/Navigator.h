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

    bool cookieEnabled()
    {
        return true;
    }

    String* language();

    bool onLine()
    {
        return true;
    }

protected:
    Geolocation* m_geolocation;
};
}
#endif
