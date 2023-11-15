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

#ifdef STARFISH_ENABLE_WORKER

#include <EscargotPublic.h>

#include "StarfishConfig.h"
#include "binding/ScriptEngineInstance.h"
#include "binding/ScriptWrappable.h"
#include "core/modules/message_loop/Timer.h"
#include "core/modules/message_loop/RunLoop.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/threading/ThreadPool.h"
#include "core/modules/worker/host/WorkerGlobalScope.h"
#include "core/modules/worker/host/DedicatedWorkerGlobalScope.h"
#include "core/modules/worker/host/WorkerScriptController.h"
#include "core/modules/worker/util/Trace.h"
#include "core/modules/serviceworker/host/ServiceWorkerGlobalScope.h"
#include "core/modules/worker/host/WebWorker.h"

namespace Starfish {

WebWorker::WebWorker(Starfish* starfish, const char* locale,
                     const char* timezoneID, String* customUserAgentString)
    : WebBase(starfish, MessageLoop::createForWorker(RunLoop::create()),
              Timer::createForWorker(this), locale, timezoneID,
              customUserAgentString)
{
}

WebWorker::WebWorker(WebBase* webBase, RunLoop* runLoop)
    : WebBase(webBase->starfish(), MessageLoop::createForWorker(runLoop),
              Timer::createForWorker(this), webBase->locale().c_str(),
              webBase->timezoneID()->toUTF8NonGCString().c_str(),
              webBase->customUserAgentString())
{
}

WebWorker::~WebWorker()
{
    if (m_workerGlobalScope != nullptr) {
        m_workerGlobalScope->dispose();
        m_workerGlobalScope = nullptr;
    }

    if (m_threadPool != nullptr) {
        m_threadPool->destroy();
        m_threadPool = nullptr;
    }

    if (m_messageLoop != nullptr) {
        m_messageLoop->destroy();
        m_messageLoop = nullptr;
    }

    if (m_timer != nullptr) {
        m_timer->clear(nullptr);
        m_timer->destroy();
        m_timer = nullptr;
    }
}

void WebWorker::destroy()
{
    TRACE_SCOPE(SVCWORKER);
    this->~WebWorker();
}

void WebWorker::ensureScriptEngineInstance()
{
    STARFISH_ASSERT(isMainThread() == true);

    if (!m_scriptEngineInstance) {
        m_scriptEngineInstance = new ScriptEngineInstance(
            locale().data(), timezoneID()->toUTF8NonGCString().data());
    }
}

void WebWorker::removeScriptEngineInstance()
{
    STARFISH_ASSERT(isMainThread() == true);

    if (m_scriptEngineInstance) {
        m_scriptEngineInstance->dispose();

        delete m_scriptEngineInstance;
        m_scriptEngineInstance = nullptr;
    }
}

template <typename GlobalScopeType>
GlobalScopeType* WebWorker::createGlobalScope(ResourceURL* scriptURL)
{
    TRACE_SCOPE(SVCWORKER);
    STARFISH_ASSERT(scriptURL != nullptr);

    clearBlobURLStore();

    clearStack<ELABORATE_CLEAR_STACK_SIZE>();

    if (m_workerGlobalScope != nullptr) {
        m_workerGlobalScope->dispose();
    }

    removeScriptEngineInstance();
    ensureScriptEngineInstance();

    GlobalScopeType* globalScope = new GlobalScopeType(
        this, scriptURL, String::createASCIIString("UTF-8"));
    m_workerGlobalScope = globalScope;

    return globalScope;
}

template DedicatedWorkerGlobalScope* WebWorker::createGlobalScope(
    ResourceURL* scriptURL);
#if defined(STARFISH_SERVICE_WORKER_HOST)
template ServiceWorkerGlobalScope* WebWorker::createGlobalScope(
    ResourceURL* scriptURL);
#endif

} // namespace Starfish

#endif /* STARFISH_ENABLE_WORKER */
