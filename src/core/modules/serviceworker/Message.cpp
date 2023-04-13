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

#ifdef STARFISH_ENABLE_SERVICE_WORKER

#include "StarfishConfig.h"

#include "core/util/Id.h"
#include "core/util/Archiver.h"
#include "core/util/Archivable.h"
#include "core/modules/serviceworker/Task.h"
#include "core/modules/serviceworker/Message.h"
#include "core/dom/DOMException.h"

#include "core/dom/ExecutionContext.h"
#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/ExceptionData.h"
#include "core/modules/serviceworker/ServiceWorkerJobData.h"
#include "core/modules/serviceworker/ServiceWorkerJob.h"

#include "core/modules/serviceworker/ServiceWorkerRegistrationData.h"
#include "core/modules/serviceworker/ServiceWorkerData.h"
#include "core/modules/serviceworker/ServiceWorkerRequest.h"
#include "core/modules/serviceworker/MessageServiceWorker.h"
#include "core/modules/serviceworker/FetchEventData.h"
#include "core/modules/serviceworker/util/Trace.h"

namespace Starfish {

Message::Message(const char* msgname)
{
    STARFISH_ASSERT(msgname != nullptr);
    m_name = String::createASCIIString(msgname, strlen(msgname));
}

std::string Message::name()
{
    std::string str = CSTR(m_name);
    return str;
}

void Message::addParam(NULLABLE Archivable* param)
{
    m_params.push_back(param);
}

class ArchiverObjectScope {
public:
    ArchiverObjectScope(Archiver& ar)
    {
        m_archiver = &ar;
        m_archiver->StartObject();
    }
    ~ArchiverObjectScope()
    {
        m_archiver->EndObject();
    }
    Archiver* m_archiver{ nullptr };
};

void Message::archive(Archiver& ar)
{
    Archiver::ExecuteScope scope(&ar, "Message");
    /*
     * NOTE: Although using a binary serialization format with IDL is a better
     * approach, we go with the existing solution, json, first. Managing
     * ServiceWorker life-cycle isn't performance sensitive. Thus, we can accept
     * the time required from parsing/unpacking json. However, if Fetch, Cache,
     * and Resource loader are involved, for better memory efficiency and speed,
     * we may consider using a serialization solution or a comprehensive IPC/RPC
     * solution later.
     */
    ArchiverObjectScope objectScope(ar);

    ar.Member("name") & m_name;

    ar.Member("params");
    {
        size_t nParams = m_params.size();

        ar.StartArray(&nParams);

        if (ar.IsReader()) {
            m_params.resize(nParams);
        }

        for (size_t i = 0; i < nParams; i++) {
            archive(ar, &m_params[i]);
        }

        ar.EndArray();
    }
}

void Message::init()
{
    // NOTE: Registering a pair of a key string and a factory
    // method per an archivable is worth considering.
    Archiver::setArchivableHandler(Message::archive);
}

NULLABLE Archivable* Message::param(size_t index)
{
    STARFISH_ASSERT(m_params.size() > index);

    auto archivable = m_params[index];

    return archivable;
}

template <typename T>
static void archiveIfMatched(const char* archiveId, std::string& id,
                             Archiver& ar, Archivable** archivable_,
                             bool& isAlreadyArchived)
{
    STARFISH_ASSERT(archiveId != nullptr);

    if (isAlreadyArchived == false && (id == archiveId)) {
        if (ar.IsReader()) {
            (*archivable_) = new T;
        }
        (*archivable_)->archive(ar);
        isAlreadyArchived = true;
    }
}

void Message::archive(Archiver& ar, Archivable** archivable_)
{
    ArchiverObjectScope objectScope(ar);

    std::string id;

    if (ar.IsReader()) {
        if (ar.HasMember("_archiveId") == false) {
            // unknown object
            return;
        }
    } else {
        if (*archivable_ == nullptr) {
            // empty object or unknown
            id = TypeName::Null;
            ar.Member("_archiveId") & id;
            return;
        }
    }

    bool isAlreadyArchived = false;

    id = ar.IsReader() ? "" : (*archivable_)->archiveId();

    ar.Member("_archiveId") & id;

    Archiver::ExecuteScope scope(&ar, id.c_str());

    if (id == TypeName::Null) {
        isAlreadyArchived = true;
    } else if (id == TypeName::String) {
        if (ar.IsReader()) {
            (*archivable_) =
                new StringArchivable(id.c_str(), String::emptyString);
        }
        (*archivable_)->archive(ar);
        isAlreadyArchived = true;
    } else if (id == TypeName::Integer) {
        if (ar.IsReader()) {
            (*archivable_) = new IntegerArchivable(id.c_str());
        }
        (*archivable_)->archive(ar);
        isAlreadyArchived = true;
    }

// TODO: Use `switch-case` instead of `if-else`
#define ARCHIVE(NAME) \
    archiveIfMatched<NAME>(#NAME, id, ar, archivable_, isAlreadyArchived);

    // TODO: We could somehow remove the macro to register types.
    ARCHIVE(ServiceWorkerRequest);
    ARCHIVE(ServiceWorkerJobData);
    ARCHIVE(ServiceWorkerRegistrationData);
    ARCHIVE(ServiceWorkerData);
    ARCHIVE(ExceptionData);
    ARCHIVE(UpdateRegistrationState);
    ARCHIVE(UpdateWorkerStateData);
    ARCHIVE(ContextRequestData);
    ARCHIVE(FetchEventRequestData);
    ARCHIVE(FetchEventResponseData);

#undef ARCHIVE

    if (isAlreadyArchived == false) {
        STARFISH_LOG_ERROR(
            "[%s] '%s' isn't un/archived because it's unknown type. It may need"
            " to be registered using the above `ARCHIVE` macro.",
            ar.IsReader() ? "Reader" : "Writer", id.c_str());
    }
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
