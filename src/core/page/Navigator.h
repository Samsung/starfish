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

#ifndef __StarfishNavigator__
#define __StarfishNavigator__

#include "binding/ScriptWrappable.h"
#include "core/page/NavigatorMixin.h"

namespace Starfish {

class Starfish;
class Geolocation;
#ifdef STARFISH_ENABLE_SERVICE_WORKER
class ServiceWorkerContainer;
#endif

class Navigator : public ScriptWrappable, public NavigatorMixin {
public:
    Navigator(Document* document);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isNavigator() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;

    Geolocation* geolocation();

    void dispose();

    bool cookieEnabled()
    {
        return true;
    }

    bool javaEnabled()
    {
        return false;
    }

protected:
    Geolocation* m_geolocation;
#ifdef STARFISH_ENABLE_SERVICE_WORKER
public:
    ServiceWorkerContainer* serviceWorker();

protected:
    ServiceWorkerContainer* m_serviceWorker;
#endif
};
}
#endif
