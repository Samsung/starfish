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

#include "StarfishConfig.h"
#include "binding/ScriptWrappable.h"
#include "core/modules/worker/host/WorkerScriptController.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/DOMException.h"

namespace Starfish {

class WorkerScriptControllerClient : public ResourceRequestClient {
public:
    WorkerScriptControllerClient(WorkerScriptController* workerScriptController)
        : m_workerScriptController(workerScriptController)
        , m_scriptLoadResult(ScriptLoadResult::NotHandled)
    {
        STARFISH_ASSERT(workerScriptController != nullptr);
    }

    void onProgressEvent(ResourceRequest* request, bool isExplicitAction)
    {
        STARFISH_ASSERT(request != nullptr);
        ProgressState progState = request->progressState();
    }

    void onReadyStateChange(ResourceRequest* request, bool fromExplicit)
    {
        STARFISH_ASSERT(request != nullptr);

        if (!fromExplicit) {
            return;
        }

        if (request->readyState() == ReadyState::Done) {
            if (request->isError() == false && request->status() == 200) {
                auto& response = request->response();
                String* text =
                    String::fromUTF8(response.data(), response.size());
                if (m_workerScriptController->evaluatefromString(text)) {
                    m_scriptLoadResult = ScriptLoadResult::Success;
                } else {
                    m_scriptLoadResult = ScriptLoadResult::ScriptError;
                }

                response.clear();
                response.shrink_to_fit();

            } else {
                m_scriptLoadResult = ScriptLoadResult::NetworkError;
            }
        }
    }

    ScriptLoadResult scriptLoadResult()
    {
        return m_scriptLoadResult;
    }

private:
    WorkerScriptController* m_workerScriptController;
    ScriptLoadResult m_scriptLoadResult;
};

WorkerScriptController::WorkerScriptController(
    ExecutionContext* executionContext)
    : m_executionContext(executionContext)
{
    STARFISH_ASSERT(executionContext != nullptr);
}

ScriptBindingInstance* WorkerScriptController::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

ScriptLoadResult WorkerScriptController::loadJavaScript(
    ResourceURL* resourceURL)
{
    STARFISH_ASSERT(resourceURL != nullptr);
    RequestData* requestData = new RequestData();
    requestData->m_url = resourceURL;
    requestData->m_destination = RequestDestination::Script;
    requestData->m_syncLevel = RequestSyncLevel::AlwaysSync;

    ResourceRequest* resourceRequest = new ResourceRequest(executionContext());
    WorkerScriptControllerClient* client =
        new WorkerScriptControllerClient(this);
    resourceRequest->addResourceRequestClient(client);
    resourceRequest->open(requestData, new HeadersData());
    resourceRequest->send();

    return client->scriptLoadResult();
}

bool WorkerScriptController::evaluatefromString(String* string)
{
    STARFISH_ASSERT(string != nullptr);
    bool result = false;
    evaluateString(scriptBindingInstance(), string, String::emptyString,
                   &result);
    return result;
}
} // namespace Starfish

#endif /* STARFISH_WEBWORKER_HOST */
