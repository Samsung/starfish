/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_SHARED_WORKER)

#include "StarfishConfig.h"
#include "core/page/WebBase.h"
#include "platform/process/base/Process.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/sharedworker/SharedWorker.h"
#include "core/modules/sharedworker/IPCMessageSerializer.h"
#include "core/modules/sharedworker/SharedWorkerMessagePortConnection.h"
#include "core/modules/sharedworker/SharedWorkerMessage.h"

namespace Starfish {

namespace SharedWorkerMessage {

    // RequestGetSharedWorker
    RequestGetSharedWorker::RequestGetSharedWorker(SharedWorker* sharedWorker)
    {
        ResourceURL* url = sharedWorker->scriptURL();
        WebBase* webBase = sharedWorker->executionContext()->webBase();

        m_clientID = sharedWorker->clientID();
        m_pid = ProcessUtil::getCurrentProcId();
        m_sharedWorkerKey = sharedWorker->sharedWorkerKey();
        m_name = sharedWorker->workerOptions().name()->toUTF8NonGCString();
        m_workerHostInitData.baseURL =
            sharedWorker->scriptURL()->baseURI()->toUTF8NonGCString();
        m_workerHostInitData.url =
            sharedWorker->scriptURL()->urlString()->toUTF8NonGCString();
        m_workerHostInitData.locale = webBase->locale();
        m_workerHostInitData.timezoneID =
            webBase->timezoneID()->toUTF8NonGCString();
        m_workerHostInitData.userAgent =
            webBase->userAgent()->toUTF8NonGCString();
    }

    IPCMessageSerializer* RequestGetSharedWorker::serialize()
    {
        IPCMessageSerializer* serializer =
            new IPCMessageSerializer(messageID());

        serializer->writeUInt32(m_clientID);
        serializer->writeUInt32(m_pid);
        serializer->writeSize(m_sharedWorkerKey);
        serializer->writeString(m_name);
        serializer->writeString(m_workerHostInitData.baseURL);
        serializer->writeString(m_workerHostInitData.url);
        serializer->writeString(m_workerHostInitData.locale);
        serializer->writeString(m_workerHostInitData.timezoneID);
        serializer->writeString(m_workerHostInitData.userAgent);
        return serializer;
    }

    void RequestGetSharedWorker::deserialize(
        IPCMessageDeserializer* deserializer)
    {
        m_clientID = deserializer->readUInt32();
        m_pid = deserializer->readUInt32();
        m_sharedWorkerKey = deserializer->readSize();
        m_name = deserializer->readString();
        m_workerHostInitData.baseURL = deserializer->readString();
        m_workerHostInitData.url = deserializer->readString();
        m_workerHostInitData.locale = deserializer->readString();
        m_workerHostInitData.timezoneID = deserializer->readString();
        m_workerHostInitData.userAgent = deserializer->readString();
    }

    ResponseGetSharedWorker::ResponseGetSharedWorker(
        SharedWorkerMessagePortConnection* connection)
        : m_clientID(connection->clientID())
        , m_pid(connection->pid())
        , m_ipcAddress(connection->ipcAddress())
    {
    }

    IPCMessageSerializer* ResponseGetSharedWorker::serialize()
    {
        IPCMessageSerializer* serializer =
            new IPCMessageSerializer(messageID());

        serializer->writeUInt32(m_clientID);
        serializer->writeUInt32(m_pid);
        serializer->writeString(m_ipcAddress);

        return serializer;
    }

    void ResponseGetSharedWorker::deserialize(
        IPCMessageDeserializer* deserializer)
    {
        m_clientID = deserializer->readUInt32();
        m_pid = deserializer->readUInt32();
        m_ipcAddress = deserializer->readString();
    }

    RequestCloseSharedWorker::RequestCloseSharedWorker()
        : m_pid(ProcessUtil::getCurrentProcId())
    {
    }

    IPCMessageSerializer* RequestCloseSharedWorker::serialize()
    {
        IPCMessageSerializer* serializer =
            new IPCMessageSerializer(messageID());

        serializer->writeUInt32(m_pid);

        return serializer;
    }

    void RequestCloseSharedWorker::deserialize(
        IPCMessageDeserializer* deserializer)
    {
        m_pid = deserializer->readUInt32();
    }

} // namespace SharedWorkerMessage
} // namespace Starfish

#endif
