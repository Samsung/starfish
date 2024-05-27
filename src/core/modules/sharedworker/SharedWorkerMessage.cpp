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
#include "core/dom/ExecutionContext.h"
#include "core/modules/sharedworker/SharedWorker.h"
#include "core/modules/sharedworker/IPCSerializer.h"
#include "core/modules/sharedworker/SharedWorkerMessagePortConnection.h"
#include "core/modules/sharedworker/SharedWorkerMessage.h"

namespace Starfish {

namespace SharedWorkerMessage {

#if defined(STARFISH_ENABLE_TEST)
    IPCMessageSerializer* SharedWorkerMessageTest::serialize()
    {
        IPCMessageSerializer* serializer =
            new IPCMessageSerializer(messageID());
        serializer->writeUInt32(123456789);
        serializer->writeString("Hello World");
        serializer->writeBool(true);
        serializer->writeSize(128);

        if (serializer->isError()) {
            STARFISH_LOG_ERROR("failed to serialize the IPC test message\n");
        }

        return serializer;
    }

    void SharedWorkerMessageTest::deserialize(
        IPCMessageDeserializer* deserializer)
    {
        uint32_t uint32Value = deserializer->readUInt32();
        STARFISH_ASSERT(uint32Value == 123456789);

        std::string stringValue = deserializer->readString();
        STARFISH_ASSERT(stringValue == "Hello World");

        bool boolValue = deserializer->readBool();
        STARFISH_ASSERT(boolValue == true);

        size_t sizeValue = deserializer->readSize();
        STARFISH_ASSERT(sizeValue == 128);

        STARFISH_ASSERT(!deserializer->isError());
    }

#endif

    // RequestGetSharedWorker
    RequestGetSharedWorker::RequestGetSharedWorker(SharedWorker* sharedWorker)
    {
        ResourceURL* url = sharedWorker->scriptURL();
        WebBase* webBase = sharedWorker->executionContext()->webBase();

        m_clientID = sharedWorker->clientID();
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
        : m_identifier(connection->identifier())
        , m_clientID(connection->clientID())
        , m_ipcAddress(connection->ipcAddress())
    {
    }

    IPCMessageSerializer* ResponseGetSharedWorker::serialize()
    {
        IPCMessageSerializer* serializer =
            new IPCMessageSerializer(messageID());

        serializer->writeUInt32(m_identifier);
        serializer->writeUInt32(m_clientID);
        serializer->writeString(m_ipcAddress);

        return serializer;
    }

    void ResponseGetSharedWorker::deserialize(
        IPCMessageDeserializer* deserializer)
    {
        m_identifier = deserializer->readUInt32();
        m_clientID = deserializer->readUInt32();
        m_ipcAddress = deserializer->readString();
    }

} // namespace SharedWorkerMessage
} // namespace Starfish

#endif
