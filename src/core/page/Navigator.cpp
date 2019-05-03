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

#include "StarfishConfig.h"
#include "Navigator.h"
#include "Starfish.h"
#include "core/dom/Document.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/location/Geolocation.h"
#ifdef STARFISH_ENABLE_SERVICE_WORKER
#include "core/modules/serviceworker/ServiceWorkerContainer.h"
#endif

namespace Starfish {

Navigator::Navigator(Document* document)
    : ScriptWrappable(this)
    , NavigatorMixin(document->executionContext())
    , m_geolocation(nullptr)
#ifdef STARFISH_ENABLE_SERVICE_WORKER
    , m_serviceWorker(nullptr)
#endif
{
}

Geolocation* Navigator::geolocation()
{
    if (m_geolocation == nullptr) {
        m_geolocation = Geolocation::create(executionContext()->document());
    }
    return m_geolocation;
}

#ifdef STARFISH_ENABLE_SERVICE_WORKER
ServiceWorkerContainer* Navigator::serviceWorker()
{
    if (m_serviceWorker == nullptr) {
        m_serviceWorker = new ServiceWorkerContainer(executionContext());
    }
    return m_serviceWorker;
}
#endif

void Navigator::dispose()
{
    if (m_geolocation) {
        m_geolocation->dispose();
    }

#ifdef STARFISH_ENABLE_SERVICE_WORKER
    if (m_serviceWorker) {
        m_serviceWorker->dispose();
    }
#endif
}

ScriptBindingInstance* Navigator::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

} // namespace Starfish
