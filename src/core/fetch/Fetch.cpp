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

#include "StarfishConfig.h"
#include "binding/ScriptWrappable.h"
#include "core/page/Window.h"
#include "core/fetch/Fetch.h"
#include "core/fetch/Response.h"
#include "platform/network/http/HTTPStatus.h"

namespace Starfish {

class FetchResourceRequestClient : public ResourceRequestClient {
public:
    FetchResourceRequestClient(Fetch* fetch)
        : m_fetch(fetch)
    {
    }

    void onProgressEvent(ResourceRequest* request, bool isExplicitAction)
    {
        ProgressState progState = request->progressState();

        if (progState == ProgressState::InError) {
            m_fetch->fail();
        }
    }

    void onReadyStateChange(ResourceRequest* request, bool fromExplicit)
    {
        if (fromExplicit) {
            if (request->readyState() == ReadyState::Done) {
                if (!request->isError()) {
                    m_fetch->success(request);

                    request->response().clear();
                    request->response().shrink_to_fit();
                }
            }
        }
    }

private:
    Fetch* m_fetch;
};

Fetch::Fetch(Window* window, Request* request, Promise* promise)
    : WindowHoldable(window)
    , m_request(request)
    , m_response(nullptr)
    , m_resourceRequest(new ResourceRequest(window->document()))
    , m_promise(promise)
{
    m_resourceRequest->addResourceRequestClient(
        new FetchResourceRequestClient(this));
}

void* Fetch::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(Fetch));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(Fetch)] = { 0 };
        Fetch::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(Fetch));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void Fetch::start()
{
    m_response = new Response(window()->document());
    m_resourceRequest->open(m_request->requestData());
    m_resourceRequest->send();
}

void Fetch::success(ResourceRequest* request)
{
    auto status = request->status();
    m_response->setStatus(status);
    m_response->setStatusText(httpStatusCodeToText(status));
    m_response->setOk(m_response->ok());
    m_response->setRedirected(request->isRedirected());
    m_response->setType(request->responseType());
    m_response->setMimeType(request->responseMimeType());
    m_response->setUrl(request->url()->urlString());
    m_response->pushResponseData(request);

    m_promise->fulfill(m_response->scriptValue());
}

void Fetch::fail()
{
    auto error =
        scriptError(scriptBindingInstance(), String::fromUTF8("NetworkError"));
    m_promise->reject(createScriptValue(error));
}

Promise* Fetch::fetch(Window* window, RequestInfo& info)
{
    Promise* promise = new Promise(window->scriptBindingInstance());
    Fetch* f = new Fetch(window, new Request(window, info), promise);
    f->start();

    return promise;
}

Promise* Fetch::fetch(Window* window, RequestInfo& info, RequestInit& init)
{
    Promise* promise = new Promise(window->scriptBindingInstance());
    Fetch* f = new Fetch(window, new Request(window, info, init), promise);
    f->start();

    return promise;
}
}
