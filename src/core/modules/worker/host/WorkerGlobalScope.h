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

#if defined(STARFISH_WEBWORKER_HOST) && !defined(__StarfishWorkerGlobalScope__)
#define __StarfishWorkerGlobalScope__

#include "core/dom/EventTarget.h"
#include "core/page/GlobalScope.h"

namespace Starfish {

class WebWorker;
class WorkerScriptController;
class WorkerLocation;
class WorkerNavigator;
class ErrorEventInit;
class ResourceURL;

class WorkerGlobalScope : public EventTarget, public GlobalScope {
public:
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isWorkerGlobalScope() const override;

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

    ScriptBindingInstance* scriptBindingInstance()
    {
        return m_scriptBindingInstance;
    }

    void dispatchErrorEvent(ErrorEventInit& errorInfo);

    void dispose();

    void importScripts(GCVector<String*>& urls);

protected:
    WorkerGlobalScope(WebWorker* webWorker, ResourceURL* url, String* charSet);
    WebWorker* m_webWorker;
    ScriptBindingInstance* m_scriptBindingInstance;
    ExecutionContext* m_executionContext;
    WorkerScriptController* m_workerScriptController;
    WorkerLocation* m_workerLocation;
    WorkerNavigator* m_workerNavigator;

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
    }

private:
    void importScript(ResourceURL* url);
};
}

#endif
