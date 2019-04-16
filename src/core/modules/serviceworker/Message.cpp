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

#ifdef STARFISH_ENABLE_SERVICE_WORKER

#include "StarfishConfig.h"

#include <iostream>

#include "core/util/Id.h"
#include "core/util/Archiver.h"
#include "core/util/Archivable.h"
#include "core/modules/serviceworker/Message.h"

#include "core/dom/ExecutionContext.h"
#include "core/util/Id.h"
#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/ServiceWorkerJobData.h"
#include "core/modules/serviceworker/ServiceWorkerJob.h"

#include "core/modules/serviceworker/ServiceWorkerRegistrationData.h"
#include "core/modules/serviceworker/ServiceWorkerData.h"

namespace Starfish {

Message::Message(const char* msgname)
{
    STARFISH_ASSERT(msgname != nullptr);
    name = msgname;
}

void Message::addParam(Archivable* param)
{
    STARFISH_ASSERT(param != nullptr);
    params.push_back(param);
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

    ar.Member("name") & name;

    // array
    ar.Member("params");
    {
        size_t nParams = params.size();

        ar.StartArray(&nParams);

        if (ar.IsReader()) {
            params.resize(nParams);
        }

        for (size_t i = 0; i < nParams; i++) {
            archive(ar, params[i]);
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

void Message::archive(Archiver& ar, Archivable*& archivable)
{
    if (!ar.IsReader() && archivable == nullptr) {
        // empty object
        ar.StartObject();
        ar.EndObject();
        return;
    }

    std::string id = ar.IsReader() ? "" : archivable->archiveId();

    ar.StartObject();

    ar.Member("_archiveId") & id;

    if (id == "ServiceWorkerJobData") {
        if (ar.IsReader()) {
            archivable = new ServiceWorkerJobData;
            STARFISH_ASSERT(archivable != nullptr);
        }
        archivable->archive(ar);
    } else if (id == "ServiceWorkerRegistrationData") {
        if (ar.IsReader()) {
            archivable = new ServiceWorkerRegistrationData;
            STARFISH_ASSERT(archivable != nullptr);
        }
        archivable->archive(ar);
    } else if (id == "ServiceWorkerData") {
        if (ar.IsReader()) {
            archivable = new ServiceWorkerData;
            STARFISH_ASSERT(archivable != nullptr);
        }
        archivable->archive(ar);
    }

    ar.EndObject();
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
