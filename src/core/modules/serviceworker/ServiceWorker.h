/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_SERVICE_WORKER) && \
    !defined(__StarfishServiceWorker__)
#define __StarfishServiceWorker__

#include "core/dom/EventTarget.h"
#include "core/fetch/GetSet.h"

namespace Starfish {

enum class ServiceWorkerState {
    Installing,
    Installed,
    Activating,
    Activated,
    Redundant,
};

class ServiceWorkerData : public gc {
public:
    ServiceWorkerData()
        : m_scriptURL(String::emptyString)
        , m_state(ServiceWorkerState::Installing)
    {
    }

    GETTER_SETTER(String*, scriptURL, ScriptURL);
    GETTER_SETTER(ServiceWorkerState, state, State);

protected:
    String* m_scriptURL;
    ServiceWorkerState m_state;
};

class ServiceWorker : public EventTarget {
public:
    ServiceWorker(Document* document)
        : EventTarget(document)
        , m_serviceWorkerData(new ServiceWorkerData())
    {
    }

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(ServiceWorker)

    String* scriptURL() const
    {
        return m_serviceWorkerData->scriptURL();
    }

    String* state() const;

    ServiceWorkerData* data()
    {
        return m_serviceWorkerData;
    }

#define VIRTUAL
#define OVERRIDE
    DECLARE_EVENT_LISTENER(statechange);
#undef VIRTUAL
#undef OVERRIDE

private:
    ServiceWorkerData* m_serviceWorkerData;
};
}

#endif
