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

#ifdef STARFISH_WEBWORKER_HOST

#include <EscargotPublic.h>

#include "StarfishConfig.h"
#include "binding/ScriptEngineInstance.h"
#include "binding/ScriptWrappable.h"
#include "core/modules/message_loop/Timer.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/threading/ThreadPool.h"
#include "core/dom/ExecutionContext.h"

#include "core/modules/serviceworker/host/ServiceWorkerGlobalScope.h"
#include "core/modules/worker/host/WorkerScriptController.h"
#include "core/modules/worker/host/WebWorker.h"

namespace Starfish {

WebWorker::WebWorker(Starfish* starfish, const char* locale,
                     const char* timezoneID, String* customUserAgentString)
    : WebBase(starfish, locale, timezoneID, customUserAgentString)
{
    STARFISH_ASSERT(starfish != nullptr);
    STARFISH_ASSERT(locale != nullptr);
    STARFISH_ASSERT(timezoneID != nullptr);
    STARFISH_ASSERT(customUserAgentString != nullptr);
    STARFISH_ASSERT(isMainThread() == true);
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

WebWorker* WebWorker::create(Starfish* starfish, const char* locale,
                             const char* timezoneID,
                             String* customUserAgentString)
{
    STARFISH_ASSERT(starfish != nullptr);
    STARFISH_ASSERT(locale != nullptr);
    STARFISH_ASSERT(timezoneID != nullptr);
    STARFISH_ASSERT(customUserAgentString != nullptr);

    return new WebWorker(starfish, locale, timezoneID, customUserAgentString);
}

void WebWorker::destroy()
{
    this->~WebWorker();
}

void WebWorker::createScriptEngineInstance()
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

WorkerGlobalScope* WebWorker::createGlobalScope(String* scriptURL)
{
    STARFISH_ASSERT(scriptURL != nullptr);

    clearBlobURLStore();

    clearStack<ELABORATE_CLEAR_STACK_SIZE>();

    if (m_workerGlobalScope != nullptr) {
        m_workerGlobalScope->dispose();
    }

    removeScriptEngineInstance();
    createScriptEngineInstance();
    ResourceURL* resourceURL = new ResourceURL(scriptURL);
    m_workerGlobalScope = new ServiceWorkerGlobalScope(
        this, resourceURL, String::createASCIIString("UTF-8"));

    return m_workerGlobalScope;
}

} // namespace Starfish

#endif /* STARFISH_WEBWORKER_HOST */
