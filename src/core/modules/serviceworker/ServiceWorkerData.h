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

#if defined(STARFISH_ENABLE_SERVICE_WORKER) && \
    !defined(__StarfishServiceWorkerData__)
#define __StarfishServiceWorkerData__

#include "core/modules/worker/WorkerType.h"

namespace Starfish {

enum class ServiceWorkerRunningState {
    Running,
    Terminating,
    NotRunning,
};

class WorkerGlobalScope;
using ScriptResourceMapKey_t = String;   // URL
using ScriptResourceMapValue_t = String; // Responses

struct ScriptResourceMapKeyComparator {
    bool operator()(const ScriptResourceMapKey_t*& lhs,
                    const ScriptResourceMapKey_t*& rhs) const
    {
        // TODO: use this on ServiceWorkerRegistrationKeyComparator
        size_t l1 = lhs->length();
        size_t l2 = rhs->length();
        size_t lmin = std::min(l1, l2);
        size_t c = 0, pos = 0;

        while ((pos < lmin) && (lhs->charAt(c) == rhs->charAt(c))) {
            ++c;
            ++pos;
        }

        if (pos < lmin) {
            return (lhs->charAt(c) < rhs->charAt(c)) ? true : false;
        }

        if (l1 == l2) {
            return true;
        }

        return (l1 < l2) ? true : false;
    }
};

using ScriptResourceMap_t =
    GCMap<ScriptResourceMapKey_t*, ScriptResourceMapValue_t*,
          ScriptResourceMapKeyComparator>;

class ScriptResource : public gc {
public:
    String* script{ String::emptyString };
    String* httpsState{ String::emptyString };
    String* referrerPolicy{ String::emptyString };
};

class ServiceWorkerData : public Archivable {
public:
    String* scriptURL{ String::emptyString };
    String* scopeURL{ String::emptyString };
    ServiceWorkerState state{ ServiceWorkerState::Parsed };
    ServiceWorkerRegistrationId registrationId;
    ServiceWorkerContextId clientContextId;

    // serialize/deserialize
    const char* archiveId() const override;
    void archive(Archiver& ar) override;

    // NOTE: consider seperating ServiceWorker model shared
    // on both client and host.
    DEFINE_GETTER_SETTER(bool, hasPendingEvents, HasPendingEvents);
    DEFINE_GETTER_SETTER(ServiceWorkerRunningState, runningState, RunningState);
    DEFINE_GETTER_SETTER(WorkerGlobalScope*, globalObject, GlobalObject);
    DEFINE_GETTER_SETTER(WorkerType, type, Type);
    DEFINE_GETTER_SETTER(NULLABLE ScriptResourceMap_t*, urlToScriptResourceMap,
                         UrlToScriptResourceMap);
    DEFINE_GETTER_SETTER(bool, skipWaiting, SkipWaiting);

    ScriptResource& scriptResource()
    {
        return m_scriptResource;
    }

private:
    ServiceWorkerRunningState m_runningState{
        ServiceWorkerRunningState::NotRunning
    };
    bool m_hasPendingEvents{ false };
    NULLABLE WorkerGlobalScope* m_globalObject{ nullptr };
    ScriptResourceMap_t* m_urlToScriptResourceMap{ nullptr };
    WorkerType m_type{ WorkerType::Classic };
    ScriptResource m_scriptResource;
    bool m_skipWaiting{ false };
};

} // namespace Starfish

#endif
