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

#include "StarfishConfig.h"

#include "core/page/GlobalScope.h"
#include "platform/process/base/ProcessType.h"

#include "core/page/WebView.h"
#include "core/util/Id.h"
#include "core/util/Archivable.h"
#include "core/modules/serviceworker/Message.h"
#include "core/modules/serviceworker/ServiceWorkerProcessInterface.h"
#include "core/modules/networking/Socket.h"
#include "core/modules/threading/IRunnable.h"
#include "core/modules/serviceworker/IORunnable.h"
#include "core/modules/serviceworker/Connection.h"

#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/client/ServiceWorkerClientConnection.h"
#include "core/modules/serviceworker/client/ServiceWorkerProcessManager.h"

#include "platform/process/base/ProcessType.h"
#include "platform/process/base/Process.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/threading/AdaptedThread.h"
#include "core/modules/threading/ThreadPool.h"

#include "core/modules/threading/IRunnable.h"
#include "core/modules/networking/Socket.h"
#include "core/modules/serviceworker/IORunnable.h"

#include "core/modules/serviceworker/ServiceWorkerProcessInterface.h"
#include "core/modules/serviceworker/host/ServiceWorkerHostProcess.h"

#include "core/modules/serviceworker/host/ProgramOptions.h"

#ifdef STARFISH_ENABLE_SERVICE_WORKER

namespace Starfish {

ServiceWorkerProcessManager* ServiceWorkerProcessManager::m_instance = nullptr;

ServiceWorkerProcessManager* ServiceWorkerProcessManager::getInstance()
{
    if (!m_instance) {
        m_instance = new ServiceWorkerProcessManager();
        STARFISH_ASSERT(m_instance);
    }
    return m_instance;
}

void ServiceWorkerProcessManager::init(WebView* webView)
{
    STARFISH_ASSERT(webView != nullptr);

    Message::init();

    m_threadPool = webView->threadPool();

    // start I/O runner
    m_ioRunnable = new IORunnable(m_threadPool->messageLoop());
    m_ioThread = new AdaptedThread(m_threadPool);

    STARFISH_ASSERT(m_ioRunnable != nullptr);
    STARFISH_ASSERT(m_ioThread != nullptr);

    m_ioThread->start(m_ioRunnable);

    // create mock instances
    m_serviceWorkerHostProcess = ServiceWorkerHostProcess::getInstance();
    m_serviceWorkerHostProcess->init(m_threadPool);
}

void ServiceWorkerProcessManager::destroy()
{
    if (m_instance) {
        m_instance->~ServiceWorkerProcessManager();
        m_instance = nullptr;
    }
}

ServiceWorkerProcessManager::ServiceWorkerProcessManager()
    : m_ioThread(nullptr)
    , m_threadPool(nullptr)
    , m_ioRunnable(nullptr)
    , m_serviceWorkerHostProcess(nullptr)
    , m_serviceWorkerClientProcess(nullptr)
{
}

ServiceWorkerProcessManager::~ServiceWorkerProcessManager()
{
    if (m_ioThread) {
        m_ioThread->stop();
    }
}

ServiceWorkerClientConnection* ServiceWorkerProcessManager::getConnection(
    String* originSerialized)
{
    STARFISH_ASSERT(originSerialized != nullptr);
    STARFISH_ASSERT(m_threadPool != nullptr);

    std::string origin = CSTR(originSerialized);

    std::shared_ptr<ProcessData> processData = nullptr;

    std::string encodedOrigin = StringUtils::toBase64(origin);

    // check if a process for this origin exists
    auto it = m_mapOriginToProcessData.find(origin);
    if (it == m_mapOriginToProcessData.end()) {
        processData = std::make_shared<ProcessData>();
        m_mapOriginToProcessData.insert(std::make_pair(origin, processData));

        // TODO: launch a service worker process
        auto po = new ProgramOptions;

        STARFISH_ASSERT(po != nullptr);

        po->set("origin", origin.c_str());
        m_serviceWorkerHostProcess->start(po);

    } else {
        processData = it->second;
    }

    STARFISH_ASSERT(processData != nullptr);

    if (processData->connection == nullptr) {
        processData->connection = new ServiceWorkerClientConnection();

        STARFISH_ASSERT(processData->connection != nullptr);

        std::string address = IPC_PROTOCOL;
        address.append(IPC_ADDRESS_PREFIX);
        address.append(encodedOrigin);

#ifndef NDEBUG
        STARFISH_LOG_WARN("client: connect: %s\n", address.c_str());
        STARFISH_LOG_WARN("client: origin: %s\n", origin.c_str());
#endif
        processData->connection->socket()->connect(address.c_str());
        m_ioRunnable->addClient(processData->connection);
    }

    return processData->connection;
}

void ServiceWorkerProcessManager::registerActiveGlobalScope(
    Id<GlobalScope> id, GlobalScope* globalScope)
{
    STARFISH_ASSERT(globalScope != nullptr);
    m_mapIdToActiveGlobalScope.insert(std::make_pair(id, globalScope));
}

void ServiceWorkerProcessManager::deregisterActiveGlobalScope(
    Id<GlobalScope> id)
{
    m_mapIdToActiveGlobalScope.erase(id);
}

GlobalScope* ServiceWorkerProcessManager::find(Id<GlobalScope> id)
{
    auto it = m_mapIdToActiveGlobalScope.find(id);
    if (it == m_mapIdToActiveGlobalScope.end()) {
        return nullptr;
    }
    return it->second;
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
