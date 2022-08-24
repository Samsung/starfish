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
#include "core/modules/serviceworker/ErrorData.h"
#include "core/modules/serviceworker/ServiceWorkerJobData.h"
#include "core/modules/serviceworker/ServiceWorkerJob.h"

#include "core/modules/serviceworker/ServiceWorkerRegistrationData.h"
#include "core/modules/serviceworker/ServiceWorkerData.h"
#include "core/modules/serviceworker/ServiceWorkerRequest.h"
#include "core/modules/serviceworker/ErrorData.h"
#include "core/modules/serviceworker/MessageServiceWorker.h"
#include "core/modules/serviceworker/ServiceWorkerFetchTask.h"

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

void Message::archive(Archiver& ar)
{
    /*
     * NOTE: Although using a binary serialization format with IDL is a better
     * approach, we go with the existing solution, json, first. Managing
     * ServiceWorker life-cycle isn't performance sensitive. Thus, we can accept
     * the time required from parsing/unpacking json. However, if Fetch, Cache,
     * and Resource loader are involved, for better memory efficiency and speed,
     * we may consider using a serialization solution or a comprehensive IPC/RPC
     * solution later.
     */
    ar.StartObject();

    ar.Member("name") & m_name;

    // array
    ar.Member("params");
    {
        size_t nParams = m_params.size();

        ar.StartArray(&nParams);

        if (ar.IsReader()) {
            m_params.resize(nParams);
        }

        for (size_t i = 0; i < nParams; i++) {
            archive(ar, m_params[i]);
        }

        ar.EndArray();
    }

    ar.EndObject();
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
                             Archiver& ar, Archivable*& archivable,
                             bool& isAlreadyArchived)
{
    STARFISH_ASSERT(archiveId != nullptr);

    if (isAlreadyArchived == false && (id == archiveId)) {
        if (ar.IsReader()) {
            archivable = new T;
        }
        archivable->archive(ar);
        isAlreadyArchived = true;
    }
}

void Message::archive(Archiver& ar, Archivable*& archivable)
{
    if ((ar.IsReader() == false) && archivable == nullptr) {
        // empty object
        ar.StartObject();
        ar.EndObject();
        return;
    }

    std::string id = ar.IsReader() ? "" : archivable->archiveId();

    ar.StartObject();

    ar.Member("_archiveId") & id;

    bool isAlreadyArchived = false;

    if (id == TypeName::String) {
        if (ar.IsReader()) {
            archivable = new StringArchivable(id.c_str(), String::emptyString);
        }
        archivable->archive(ar);
        isAlreadyArchived = true;
    }

#define ARCHIVE(NAME) \
    archiveIfMatched<NAME>(#NAME, id, ar, archivable, isAlreadyArchived);

    // TODO: We could somehow remove the macro to register types.
    ARCHIVE(ServiceWorkerRequest);
    ARCHIVE(ServiceWorkerJobData);
    ARCHIVE(ServiceWorkerRegistrationData);
    ARCHIVE(ServiceWorkerData);
    ARCHIVE(ErrorData);
    ARCHIVE(UpdateWorkerStateData);
    ARCHIVE(ContextRequestData);
    ARCHIVE(FetchEventData);
    ARCHIVE(FireEventRequestData);

#undef ARCHIVE

    if (isAlreadyArchived == false) {
        STARFISH_LOG_ERROR(
            "'%s' isn't archived because it's unknow type. It may need to be "
            "registered using the above `ARCHIVE` macro",
            id.c_str());
    }

    ar.EndObject();
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
