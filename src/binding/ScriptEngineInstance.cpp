/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"
#include "binding/ScriptEngineInstance.h"
#include "binding/ScriptBindingInstance.h"

#include "core/modules/message_loop/MessageLoop.h"

#ifdef STARFISH_TIZEN
#include <app_common.h>
#endif
#include <EscargotPublic.h>

namespace Starfish {

ScriptEngineInstance::ScriptEngineInstance(const char* locale,
                                           const char* timezone)
    : m_inDrainMicroTaskQueue(false)
    , m_macroTaskCounter(0)
{
#ifdef STARFISH_TIZEN
    // add argument for CodeCache directory
    auto cachePath = app_get_cache_path();
    m_engineInstance =
        Escargot::VMInstanceRef::create(locale, timezone, cachePath);
    free(cachePath);
#else
    m_engineInstance = Escargot::VMInstanceRef::create(locale, timezone);
#endif
    if (m_engineInstance->isCodeCacheEnabled()) {
#if defined(STARFISH_64)
        m_engineInstance->setMaxCompiledByteCodeSize(1024 * 1024 * 8 * 2);
#else
        m_engineInstance->setMaxCompiledByteCodeSize(1024 * 1024 * 8);
#endif
        m_engineInstance->setCodeCacheMinSourceLength(1024);
        m_engineInstance->setCodeCacheMaxCacheCount(16);
        m_engineInstance->setCodeCacheShouldLoadFunctionOnScriptLoading(true);
    } else {
        m_engineInstance->setMaxCompiledByteCodeSize(1024 * 1024 * 4);
    }

    size_t c = m_engineInstance->config();
    c = c & (~static_cast<size_t>(Escargot::VMInstanceRef::ConfigFlag::
                                      CompressCompressibleStringsWhileGC));
    m_engineInstance->setConfig(c);
}

void ScriptEngineInstance::dispose()
{
    m_engineInstance = nullptr;
}

void ScriptEngineInstance::enterIdleMode()
{
    m_engineInstance->enterIdleMode();
}

void ScriptEngineInstance::drainMicroTaskQueue()
{
    if (m_inDrainMicroTaskQueue) {
        return;
    }
    m_inDrainMicroTaskQueue = true;
    auto vm = engineInstance();
    while (vm->hasPendingJob()) {
        auto jobResult = vm->executePendingJob();
        if (jobResult.error) {
            STARFISH_LOG_ERROR("Uncaught Error in JS job");
        }
    }
    m_inDrainMicroTaskQueue = false;
}

void MicroTaskExecutionManager::forceInvokeDrainMicroTaskQueue()
{
    m_engine->drainMicroTaskQueue();
    m_fired = true;
}

MicroTaskExecutionManager::~MicroTaskExecutionManager()
{
    // drainMicroTaskQueue must be called while macroTaskCounter > 0
    // because JS code executed during drain (e.g. Promise reactions)
    // can call queueMicrotask(), which asserts macroTaskCounter > 0
    if (!m_fired && m_engine->macroTaskCounter() == 1) {
        m_engine->drainMicroTaskQueue();
    }
    m_engine->macroTaskCounter()--;

    // DIAGNOSTIC: the whole nested-Manager stack for this turn just
    // unwound (counter back to 0), so nothing else on this call stack
    // is going to drain the queue. If a job is still pending here, it
    // was either enqueued after a forceInvokeDrainMicroTaskQueue() call
    // (m_fired skip above) or leaked from some JS-entry path that never
    // opened a MicroTaskExecutionManager at all. Either way it will now
    // sit stuck until some unrelated future Manager happens to close at
    // counter==1 - remove once the leaking call site is found.
    STARFISH_ASSERT(!(m_engine->macroTaskCounter() == 0 &&
                      m_engine->engineInstance()->hasPendingJob()));
}
} // namespace Starfish
