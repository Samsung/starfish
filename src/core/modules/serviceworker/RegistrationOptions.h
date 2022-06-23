/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishRegistrationOptions__
#define __StarfishRegistrationOptions__

// NOTE: RegistrationOptions is needed for all builds.
#include "core/modules/worker/WorkerType.h"
#include "core/modules/serviceworker/ServiceWorkerUpdateViaCache.h"

namespace Starfish {

class String;

struct RegistrationOptions : public gc {
    RegistrationOptions();

    String* scope() const;
    void setScope(String* pScope);

    String* type() const
    {
        return WorkerTypeUtils::workerTypeToString(m_type);
    }

    void setType(String* type)
    {
        auto maybeType = WorkerTypeUtils::stringToWorkerType(type);
        if (maybeType) {
            m_type = maybeType.value();
        }
    }

    String* updateViaCache() const
    {
        return UpdateViaCacheUtils::updateViaCacheToString(m_updateViaCache);
    };

    void setUpdateViaCache(String* updateViaCache)
    {
        auto maybeUpdateViaCache =
            UpdateViaCacheUtils::stringToUpdateViaCache(updateViaCache);
        if (maybeUpdateViaCache) {
            m_updateViaCache = maybeUpdateViaCache.value();
        }
    };

    String* m_scope;
    WorkerType m_type = WorkerType::Classic;
    ServiceWorkerUpdateViaCache m_updateViaCache =
        ServiceWorkerUpdateViaCache::Imports;
};
} // namespace Starfish

#endif
