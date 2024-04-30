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

#if defined(STARFISH_ENABLE_WORKER) && \
    !defined(__StarfishDedicatedWorkerGlobalScope__)
#define __StarfishDedicatedWorkerGlobalScope__

#include "core/dom/StructuredSerializeOptions.h"
#include "core/modules/worker/host/WorkerGlobalScope.h"

namespace Starfish {

class WorkerHost;
class WorkerObjectProxy;

class DedicatedWorkerGlobalScope final : public WorkerGlobalScope {
public:
    DedicatedWorkerGlobalScope(WebWorker* webWorker, ResourceURL* url,
                               String* charSet);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(DedicatedWorkerGlobalScope)

    void initJavaScriptGlobalBinding(
        ScriptExecutionState state,
        ScriptBindingInstance* scriptBindingInstance) override;

    void initialize(WorkerHost* workerHost,
                    WorkerObjectProxy* workerObjectProxy);

    String* name()
    {
        return m_name;
    }

    void postMessage(ScriptValue message,
                     const GCAtomicVector<ScriptObject>& transfer);

    void postMessage(ScriptValue message,
                     const StructuredSerializeOptions& options =
                         StructuredSerializeOptions());

    void close();

    void dispose() override;

    DEFINE_GETTER(WorkerObjectProxy*, workerObjectProxy);

#define VIRTUAL
#define OVERRIDE
    DECLARE_EVENT_LISTENER(message);
    DECLARE_EVENT_LISTENER(messageerror);
#undef VIRTUAL
#undef OVERRIDE

private:
    WorkerObjectProxy* m_workerObjectProxy;
    String* m_name;
    bool m_wasTerminated;
};

} // namespace Starfish

#endif
