/*
 * Copyright (c) 2022-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#ifdef STARFISH_ENABLE_SERVICE_WORKER

#include "StarfishConfig.h"

#include "core/modules/serviceworker/WorkerConfig.h"
#include "core/modules/serviceworker/ServiceWorkerOption.h"
#include "core/modules/serviceworker/RegistrationStore.h"
#include "core/modules/serviceworker/ServiceWorkerData.h"
#include "core/modules/serviceworker/util/LocalStorageHelper.h"
#include "core/modules/serviceworker/client/ServiceWorkerClientConnection.h"
#include "core/modules/serviceworker/client/ServiceWorkerProcessManager.h"
#include "core/modules/serviceworker/client/RegistrationManager.h"

namespace Starfish {

RegistrationManager::RegistrationManager(ServiceWorkerOption* option)
    : m_registrationStore(
          new RegistrationStoreLocalStorage(option->dataDirectoryPath()))
{
    refreshRegistrationList();

    option->addOnChangeDataDirectoryPathCallback(
        [this](const std::string& path) { refreshRegistrationList(path); });
}

void RegistrationManager::refreshRegistrationList(const std::string path)
{
    if (!path.empty()) {
        LocalStorageHelper::File::mkdirIfNotExists(path);
        m_registrationStore->setWorkingPath(path);
    }
    m_registrationStore->loadRegistrationList();
}

bool RegistrationManager::isActivatedRegistration(String* scope)
{
    return m_registrationStore->hasRegistraionSW(scope);
}

void RegistrationManager::startRegisteredServiceWorkerContext(
    ServiceWorkerClientConnection* connection, Id<GlobalScope> id,
    String* scope)
{
    TRACE(CLIENT);

    auto scopeHash = scope->hashValue();
    auto storeData = m_registrationStore->findRegistraionStoreData(scopeHash);
    STARFISH_ASSERT(storeData.hasValue());

    auto worker = new ServiceWorkerData();
    worker->clientContextId = id;
    worker->scopeURL = storeData->scopeURL;
    worker->scriptURL = storeData->scriptURL;

    connection->startServiceWorkerContext(worker);
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
