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
#include "core/modules/serviceworker/MessageParam.h"
#include "core/modules/serviceworker/Message.h"

#include "core/dom/ExecutionContext.h"
#include "core/util/Id.h"
#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/ServiceWorkerJobData.h"
#include "core/modules/serviceworker/ServiceWorkerJob.h"

namespace Starfish {

Message::Message(const char* msgname)
{
    name = msgname;
}

void Message::addParam(MessageParam* param)
{
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

void Message::archive(Archiver& ar, MessageParam*& param)
{
    std::string paramType = ar.IsReader() ? "" : param->paramType();

    ar.StartObject();
    ar.Member("_paramType") & paramType;

    // NOTE: using factory and flyweight here is considerable.
    if (paramType == "ServiceWorkerJobData") {
        if (ar.IsReader()) {
            param = new ServiceWorkerJobData;
        }
        param->archive(ar);
    }

    ar.EndObject();
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
