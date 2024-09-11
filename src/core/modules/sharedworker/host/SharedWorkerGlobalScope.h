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
#ifndef __StarfishSharedWorkerGlobalScope__
#define __StarfishSharedWorkerGlobalScope__

#include "core/modules/worker/WorkerGlobalScope.h"

namespace Starfish {

class WebWorker;
class ResourceURL;
class String;
class ScriptBindingInstance;
class SharedWorkerMessagePortConnection;
class MessagePortConnectionInfo;

class SharedWorkerGlobalScope final : public WorkerGlobalScope {
public:
    using PostTaskCallback = void (*)(SharedWorkerGlobalScope*, void*);

    SharedWorkerGlobalScope(WebWorker* webWorker, ResourceURL* url,
                            String* charSet);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(SharedWorkerGlobalScope)

    void initJavaScriptGlobalBinding(
        ScriptExecutionState state,
        ScriptBindingInstance* scriptBindingInstance) override;

    void dispose() override;

    void initialize(const std::string& name, size_t sharedWorkerKey);

    void postTask(PostTaskCallback task, void* data);

    void requestConnection(MessagePortConnectionInfo* info);

    MessageEvent* createConnectMessageEvent(MessagePort* messagePort);

    void closeConnection(uint32_t pid);

    void close();

    String* name()
    {
        return m_name;
    }

#define VIRTUAL
#define OVERRIDE
    DECLARE_EVENT_LISTENER(connect);
#undef VIRTUAL
#undef OVERRIDE

private:
    SharedWorkerMessagePortConnection* createMessagePortConnection(
        MessagePortConnectionInfo* info, MessagePort* messagePort);

    String* m_name;
    size_t m_sharedWorkerKey;
    GCVector<SharedWorkerMessagePortConnection*> m_connections;
};

} // namespace Starfish

#endif
#endif
