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

#if defined(STARFISH_ENABLE_SHARED_WORKER) && defined(STARFISH_WEBWORKER_HOST)

#include "StarfishConfig.h"

#include "core/modules/worker/WorkerConfig.h"
#include "core/modules/worker/util/Trace.h"
#include "core/modules/worker/util/network/SocketNN.h"
#include "core/modules/sharedworker/IPCMessageHandler.h"
#include "core/modules/sharedworker/IPCSerializer.h"
#include "core/modules/sharedworker/SharedWorkerMessage.h"
#include "core/modules/sharedworker/SharedWorkerMessagePortConnection.h"
#include "core/modules/sharedworker/host/SharedWorkerThread.h"
#include "core/modules/sharedworker/host/SharedWorkerAgent.h"
#include "core/modules/sharedworker/host/SharedWorkerAgentServer.h"

namespace Starfish {

SharedWorkerAgentServer::SharedWorkerAgentServer(PerProcess* perProcess,
                                                 const std::string& address)
    : IPCConnection(perProcess, address, SocketNN::kReplyProtocol)
    , m_messageHandler(new IPCMessageHandler())
{
    initMessageReceiveHandlers();
}

SharedWorkerAgentServer::~SharedWorkerAgentServer() = default;

void SharedWorkerAgentServer::start()
{
    bind();
}

void SharedWorkerAgentServer::onReceived(Socket* socket, const char* data,
                                         size_t len)
{
    m_messageHandler->onReceiveMessage(data, len);
}

#if defined(STARFISH_ENABLE_TEST)
static void onSharedWorkerMessageTest(IPCMessageDeserializer* deserializer)
{
    SharedWorkerMessage::SharedWorkerMessageTest message;
    message.deserialize(deserializer);
}
#endif

static void onRequestSharedWorkerMessage(IPCMessageDeserializer* deserializer)
{
    SharedWorkerMessage::RequestGetSharedWorker message;
    message.deserialize(deserializer);
    if (deserializer->isError()) {
        TRACE(SHAREDWORKER,
              "failed to deserialize RequestGetSharedWorker message");
        return;
    }

    TRACE(SHAREDWORKER, message.sharedWorkerKey(), message.clientID(),
          message.name(), message.workerHostInitData().baseURL,
          message.workerHostInitData().url);

    SharedWorkerAgent::instance()->connectWorkerThread(message);
}

void SharedWorkerAgentServer::responseShareWorkerConnection(
    SharedWorkerMessagePortConnection* connection)
{
    SharedWorkerMessage::ResponseGetSharedWorker message(connection);
    m_messageHandler->sendMessage(this, message);
}

void SharedWorkerAgentServer::initMessageReceiveHandlers()
{
#if defined(STARFISH_ENABLE_TEST)
    m_messageHandler->setMessageReceiveHandler(
        SharedWorkerMessage::SharedWorkerMessageTest::messageID(),
        onSharedWorkerMessageTest);
#endif

    m_messageHandler->setMessageReceiveHandler(
        SharedWorkerMessage::RequestGetSharedWorker::messageID(),
        onRequestSharedWorkerMessage);
}

} // namespace Starfish
#endif // #ifdef STARFISH_WEBWORKER_HOST
