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

#ifdef STARFISH_WEBWORKER_HOST

#include "StarfishConfig.h"
#include "binding/ScriptWrappable.h"
#include "core/modules/worker/host/WorkerScriptController.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {

class WorkerScriptControllerClient : public ResourceRequestClient {
public:
    WorkerScriptControllerClient(WorkerScriptController* workerScriptController)
        : m_workerScriptController(workerScriptController)
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
            if (!request->isError() && request->status() == 200) {
                auto response = request->response();
                String* text =
                    String::fromUTF8(response.data(), response.size());
                m_workerScriptController->evaluate(text);
            }
        }
    }

private:
    WorkerScriptController* m_workerScriptController;
};

WorkerScriptController::WorkerScriptController(
    ExecutionContext* executionContext)
    : m_resourceRequest(new ResourceRequest(executionContext))
{
    STARFISH_ASSERT(executionContext != nullptr);

    m_resourceRequest->addResourceRequestClient(
        new WorkerScriptControllerClient(this));
}

ScriptBindingInstance* WorkerScriptController::scriptBindingInstance()
{
    return m_resourceRequest->executionContext()->scriptBindingInstance();
}

void WorkerScriptController::evaluate(ResourceURL* resourceURL)
{
    STARFISH_ASSERT(resourceURL != nullptr);
    RequestData* requestData = new RequestData();
    requestData->m_url = resourceURL;
    requestData->m_destination = RequestDestination::Script;

    m_resourceRequest->open(requestData);
    m_resourceRequest->send();
}

void WorkerScriptController::evaluate(String* string)
{
    STARFISH_ASSERT(string != nullptr);
    evaluateString(scriptBindingInstance(), string);
}
}

#endif /* STARFISH_WEBWORKER_HOST */
