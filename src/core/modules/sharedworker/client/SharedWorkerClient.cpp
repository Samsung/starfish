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
#include "core/modules/sharedworker/IPCSerializer.h"
#include "core/modules/sharedworker/client/SharedWorkerProcessManager.h"
#include "core/modules/sharedworker/client/SharedWorkerClient.h"

namespace Starfish {

SharedWorkerClient::SharedWorkerClient(PerProcess* perProcess,
                                       const std::string& ipcAddress)
    : IPCConnection(perProcess, ipcAddress, SocketNN::kRequestProtocol)
    , m_messageHandler(new IPCMessageHandler())
{
    initMessageReceiveHandlers();
}

SharedWorkerClient::~SharedWorkerClient() = default;

void SharedWorkerClient::start()
{
    connect();

#if defined(STARFISH_ENABLE_TEST)
    SharedWorkerMessage::SharedWorkerMessageTest message;
    m_messageHandler->sendMessage(this, message);
#endif
}

void SharedWorkerClient::onReceived(Socket* socket, const char* data,
                                    size_t len)
{
    m_messageHandler->onReceiveMessage(data, len);
}

void SharedWorkerClient::requestConnection(SharedWorker* sharedWorker)
{
    SharedWorkerMessage::RequestGetSharedWorker message(sharedWorker);
    m_messageHandler->sendMessage(this, message);
}

void SharedWorkerClient::requestClose(SharedWorker* sharedWorker)
{
    // TODO: Request a close to the server.
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

} // namespace Starfish

#endif
