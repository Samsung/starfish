/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_WORKER) && !defined(__StarfishWorker__)
#define __StarfishWorker__

#include "core/dom/StructuredSerializeOptions.h"
#include "core/modules/worker/AbstractWorker.h"

namespace Starfish {

class ExecutionContext;
class ResourceURL;
class WorkerThread;
class WorkerHostProxy;

class Worker : public AbstractWorker {
public:
    Worker(ExecutionContext* executionContext, String* scriptURL,
           const WorkerOptions& workerOptions = {});

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(Worker)

    virtual ExecutionContext* executionContext() const override
    {
        return m_executionContext;
    }

    void postMessage(ScriptValue message,
                     GCAtomicVector<ScriptObject>& transfer);

    void postMessage(ScriptValue message,
                     const StructuredSerializeOptions& options = {});

    void terminate();

    void destroy();

    DEFINE_GETTER(WorkerThread*, workerThread);
    DEFINE_GETTER(WorkerHostProxy*, workerHostProxy);
    DEFINE_GETTER(bool, wasTerminated);

#define VIRTUAL
#define OVERRIDE
    DECLARE_EVENT_LISTENER(message);
    DECLARE_EVENT_LISTENER(messageerror);
#undef VIRTUAL
#undef OVERRIDE

private:
    WorkerThread* m_workerThread;
    WorkerHostProxy* m_workerHostProxy;
    std::atomic_bool m_wasTerminated;
};
} // namespace Starfish
#endif
