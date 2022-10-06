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
#include "core/modules/serviceworker/client/ServiceWorkerProcessManager.h"
#include "core/modules/serviceworker/client/RegistrationManager.h"

namespace Starfish {

RegistrationManager::RegistrationManager(ServiceWorkerOption* option)
    : m_registrationStore(
          new RegistrationStoreLocalStorage(option->localStorageRootDir()))
{
    m_registrationStore->loadRegistrationList(m_registrationList);
}

bool RegistrationManager::isActivatedRegistration(String* scope)
{
    auto hashValue = scope->hashValue();
    TRACE(CLIENT, hashValue);

    auto itr = m_registrationList.find(hashValue);
    return itr != m_registrationList.end();
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
