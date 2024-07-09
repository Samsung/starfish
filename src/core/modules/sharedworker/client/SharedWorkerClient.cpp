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

#include "core/modules/worker/WorkerConfig.h"
#include "core/modules/worker/util/network/SocketNN.h"
#include "core/modules/sharedworker/SharedWorker.h"
#include "core/modules/sharedworker/SharedWorkerMessage.h"
#include "core/modules/sharedworker/IPCMessageSerializer.h"
#include "core/modules/sharedworker/SharedWorkerMessagePortConnection.h"
#include "core/modules/sharedworker/client/SharedWorkerProcessManager.h"
#include "core/modules/sharedworker/client/SharedWorkerClient.h"

namespace Starfish {

SharedWorkerClient::SharedWorkerClient(PerProcess* perProcess,
                                       const std::string& ipcAddress)
    : IPCConnection(perProcess, ipcAddress, SocketNN::kBusProtocol)
    , m_messageHandler(new IPCMessageHandler())
    , m_requestFlag(false)
{
    initMessageReceiveHandlers();
}

SharedWorkerClient::~SharedWorkerClient()
{
    for (const auto& iter : m_requestMessages) {
        free(iter.first);
    }

    m_requestMessages.clear();
    m_requestMessages.shrink_to_fit();
}

void SharedWorkerClient::start()
{
    connect();
}

void SharedWorkerClient::onReceived(Socket* socket, const char* data,
                                    size_t len)
{
    m_requestFlag = false;
    sendPendingMessage();

    m_messageHandler->onReceiveMessage(data, len);
}

void SharedWorkerClient::requestConnection(SharedWorker* sharedWorker)
{
    SharedWorkerMessage::RequestGetSharedWorker message(sharedWorker);
    TRACE(SHAREDWORKER, message.clientID());
    sendMessage(message);
}

void SharedWorkerClient::requestClose()
{
    SharedWorkerMessage::RequestCloseSharedWorker message;
    sendMessage(message, true);
}

static void onResponseGetSharedWorker(IPCMessageDeserializer* deserializer)
{
    SharedWorkerMessage::ResponseGetSharedWorker message;
    message.deserialize(deserializer);
    if (deserializer->isError()) {
        TRACE(SHAREDWORKER,
              "failed to deserializer ResponseGetSharedWorker message");
        return;
    }

    SharedWorkerProcessManager::instance()->startMessagePortConnection(message);
}

void SharedWorkerClient::initMessageReceiveHandlers()
{
    m_messageHandler->setMessageReceiveHandler(
        SharedWorkerMessage::ResponseGetSharedWorker::messageID(),
        onResponseGetSharedWorker);
}

void SharedWorkerClient::sendMessage(IPCMessage& message, bool force)
{
    Nullable<IPCMessageSerializer*> serializer =
        m_messageHandler->serialize(message);

    if (force) {
        send(serializer->data(), serializer->size());
        return;
    }

    if (m_requestFlag) {
        void* buffer = malloc(serializer->size());
        memcpy(buffer, serializer->data(), serializer->size());
        m_requestMessages.push_back({ buffer, serializer->size() });
    } else {
        send(serializer->data(), serializer->size());
    }

    m_requestFlag = true;
}

void SharedWorkerClient::sendPendingMessage()
{
    STARFISH_ASSERT(!m_requestFlag);

    if (m_requestMessages.empty()) {
        return;
    }

    auto message = m_requestMessages.front();
    send(static_cast<const char*>(message.first), message.second);
    free(message.first);
    m_requestMessages.pop_front();

    m_requestFlag = true;
}

} // namespace Starfish

#endif
