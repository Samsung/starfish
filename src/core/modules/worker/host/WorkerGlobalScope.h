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

#if defined(STARFISH_ENABLE_WORKER) && !defined(__StarfishWorkerGlobalScope__)
#define __StarfishWorkerGlobalScope__

#include "core/dom/EventTarget.h"
#include "core/page/GlobalScope.h"
#include "core/fetch/Fetch.h"
#include "core/extra/Performance.h"
#include "core/modules/crypto/Crypto.h"
#include "core/page/WindowOrWorkerGlobalScope.h"

namespace Starfish {

class WebWorker;
class WorkerScriptController;
class WorkerLocation;
class WorkerNavigator;
class ErrorEventInit;
class ResourceURL;

typedef void (*TimerHandler)(void* data);

class WorkerGlobalScope : public EventTarget, public GlobalScope {
public:
    WorkerGlobalScope(WebWorker* webWorker, ResourceURL* url, String* charSet);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isWorkerGlobalScope() const override;

    virtual void initJavaScriptGlobalBinding(
        ScriptExecutionState state,
        ScriptBindingInstance* scriptBindingInstance) = 0;

    DedicatedWorkerGlobalScope* asDedicatedWorkerGlobalScope();

    ExecutionContext* executionContext() const override
    {
        return m_executionContext;
    }

    WebWorker* webWorker()
    {
        return m_webWorker;
    }

    WorkerScriptController* workerScriptController()
    {
        return m_workerScriptController;
    }

    WorkerGlobalScope* self()
    {
        return this;
    }

    WorkerLocation* location()
    {
        return m_workerLocation;
    }

    WorkerNavigator* navigator()
    {
        return m_workerNavigator;
    }

    Crypto* crypto()
    {
        if (!m_crypto) {
            m_crypto = Crypto::create(executionContext());
        }
        return m_crypto;
    }

    bool isClosing() const
    {
        return m_closing;
    }

    ScriptBindingInstance* scriptBindingInstance()
    {
        return m_scriptBindingInstance;
    }

    void dispatchErrorEvent(ErrorEventInit& errorInfo);

    bool terminate();

    virtual void dispose();

    uint32_t setTimeout(TimerHandler handler, int32_t delay, void* data);
    void clearTimeout(int32_t id);
    uint32_t setInterval(TimerHandler handler, int32_t delay, void* data);
    void clearInterval(int32_t id);

    void importScript(ResourceURL* url);
    void importScripts(GCVector<String*>& urls);

    Promise* fetch(RequestInfo& input);
    Promise* fetch(RequestInfo& input, RequestInit& init);

    String* btoa(ExecutionContext* executionContext, String* data);
    String* atob(ExecutionContext* executionContext, String* data);

    void queueMicrotask(ExecutionContext* executionContext,
                        ScriptObject callback);

#ifdef STARFISH_ENABLE_CANVAS
    Promise* createImageBitmap(
        ExecutionContext* executionContext, ImageBitmapSource image,
        ImageBitmapOptions options = ImageBitmapOptions());
    Promise* createImageBitmap(
        ExecutionContext* executionContext, ImageBitmapSource image, int32_t sx,
        int32_t sy, int32_t sw, int32_t sh,
        ImageBitmapOptions options = ImageBitmapOptions());
#endif

    Performance* performance();

    CustomStorage* workerStorage();
    void initCacheStorage();

protected:
    WorkerGlobalScope(WebWorker* webWorker);

    WebWorker* m_webWorker;
    ScriptBindingInstance* m_scriptBindingInstance;
    ExecutionContext* m_executionContext;
    WorkerScriptController* m_workerScriptController;
    WorkerLocation* m_workerLocation;
    WorkerNavigator* m_workerNavigator;
    Crypto* m_crypto;
    std::atomic_bool m_closing;

    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc, GC_WORD_OFFSET(WorkerGlobalScope, m_webWorker));
        GC_set_bit(desc,
                   GC_WORD_OFFSET(WorkerGlobalScope, m_scriptBindingInstance));
        GC_set_bit(desc, GC_WORD_OFFSET(WorkerGlobalScope, m_executionContext));
        GC_set_bit(desc,
                   GC_WORD_OFFSET(WorkerGlobalScope, m_workerScriptController));
        GC_set_bit(desc, GC_WORD_OFFSET(WorkerGlobalScope, m_workerLocation));
        GC_set_bit(desc, GC_WORD_OFFSET(WorkerGlobalScope, m_workerNavigator));
        GC_set_bit(desc, GC_WORD_OFFSET(WorkerGlobalScope, m_crypto));
    }

    void initGlobalScope(ResourceURL* url, String* charSet);

private:
};
} // namespace Starfish

#endif
