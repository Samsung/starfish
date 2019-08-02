/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#include <curl/curl.h>

#include "Starfish.h"
#include "core/dom/ExecutionContext.h"
#include "core/xml/FormData.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/modules/resource_request/ResourceRequestJob.h"
#include "core/modules/resource_request/NetworkURLResourceRequestJobDelegate.h"
#include "core/modules/threading/ThreadPool.h"
#include "core/util/URL.h"
#include "core/page/WebBase.h"
#include "core/dom/WebOrigin.h"
#include "core/fetch/Body.h"
#include "core/fetch/Response.h"
#include "core/fetch/FetchUtils.h"
#include "core/csp/ContentSecurityPolicy.h"
#include "platform/network/http/HTTPResponse.h"

namespace Starfish {

class ActiveResourceRequestTracker : public ResourceRequestClient {
public:
    virtual void onProgressEvent(ResourceRequest* request,
                                 bool isExplicitAction) override
    {
        if (request->progressState() == ProgressState::LoadStart) {
            request->executionContext()->addActiveResourceRequests(request);
        } else if (request->progressState() == ProgressState::LoadEnd) {
            request->executionContext()->removeActiveResourceRequests(request);
        }
    }

    virtual void onReadyStateChange(ResourceRequest* request,
                                    bool isExplicitAction) override
    {
        if (request->isError()) {
            struct Param : public gc {
                RequestErrorType errorCode;
                String* url;
            };
            Param* p = new Param;
            p->errorCode = request->errorType();
            p->url = request->url()->urlString();
            request->webBase()->callPublicWebViewHandler(OnReceivedError, p);
        }
    }
};

ResourceRequest::ResourceRequest(ExecutionContext* executionContext)
    : m_executionContext(executionContext)
    , m_requestData(nullptr)
    , m_preflightRequestData(nullptr)
    , m_requestWebOrigin(nullptr)
    , m_requestHeaders(nullptr)
    , m_responseData(nullptr)
    , m_responseHeaders(nullptr)
    , m_readyState(ReadyState::Unset)
    , m_progressState(ProgressState::None)
    , m_bodyType(BodyType::Empty)
    , m_timeout(0)
    , m_activeNetworkURLWorkerData(nullptr)
    , m_mutex(new Mutex())
    , m_lastLocation("")
    , m_lastEffectiveURL("")
    , m_jobDelegate(nullptr)
    , m_pendingOnHeaderReceivedEventIdlerHandle(MessageLoopInvalidID)
    , m_pendingOnProgressEventIdlerHandle(MessageLoopInvalidID)
    , m_loaded(0)
    , m_total(0)
    , m_abortRequestState(AbortRequestType::NoPendingRequest)
    , m_requestError(RequestErrorType::NoError)
{
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            // STARFISH_LOG_INFO("ResourceRequest::~ResourceRequest
            // %p\n", obj);
            ResourceRequest* nr = (ResourceRequest*)obj;
            std::string().swap(nr->m_lastLocation);
            std::string().swap(nr->m_lastEffectiveURL);
        },
        NULL, NULL, NULL);

    initVariables();
    addResourceRequestClient(new ActiveResourceRequestTracker());
}

void ResourceRequest::initVariables()
{
    m_requestError = RequestErrorType::NoError;
    m_contentLanguage = String::emptyString;
    std::string().swap(m_lastLocation);
    std::string().swap(m_lastEffectiveURL);
    m_containsBase64Content = false;
    m_didSend = false;
    m_total = 0;
    m_loaded = 0;
}

void ResourceRequest::clearIdlers()
{
    auto iter2 = m_requstedIdlers.begin();
    auto messageLoop = webBase()->messageLoop();
    while (iter2 != m_requstedIdlers.end()) {
        messageLoop->removeIdler(*iter2);
        iter2++;
    }
    m_requstedIdlers.clear();

    if (m_pendingOnHeaderReceivedEventIdlerHandle != MessageLoopInvalidID) {
        messageLoop->removeIdlerWithNoGCRooting(
            m_pendingOnHeaderReceivedEventIdlerHandle);
        m_pendingOnHeaderReceivedEventIdlerHandle = MessageLoopInvalidID;
    }

    if (m_pendingOnProgressEventIdlerHandle != MessageLoopInvalidID) {
        messageLoop->removeIdlerWithNoGCRooting(
            m_pendingOnProgressEventIdlerHandle);
        m_pendingOnProgressEventIdlerHandle = MessageLoopInvalidID;
    }

    if (m_activeNetworkURLWorkerData) {
        m_activeNetworkURLWorkerData->isAborted = true;
        m_activeNetworkURLWorkerData = nullptr;
    }
}

void ResourceRequest::handleResponseEOF()
{
    changeProgress(ProgressState::Progress, true);
    changeReadyState(ReadyState::Done, true);
    changeProgress(ProgressState::Load, true);
    changeProgress(ProgressState::LoadEnd, true);
}

void ResourceRequest::handleError(ProgressState error,
                                  RequestErrorType errorType)
{
    m_requestError = errorType;
    changeReadyState(ReadyState::Done, true);
    changeProgress(ProgressState::Progress, true);
    changeProgress(error, true);
    changeProgress(ProgressState::LoadEnd, true);
}

void ResourceRequest::handleResponseEOFwithPreflightRequestRedirected()
{
    changeReadyState(ReadyState::Done, true);
    changeProgress(ProgressState::Progress, true);
    changeProgress(ProgressState::InError, true);
    changeProgress(ProgressState::LoadEnd, true);
}

void ResourceRequest::changeReadyState(ReadyState readyState,
                                       bool isExplicitAction)
{
    STARFISH_ASSERT(isMainThread());
    if (!isError() && readyState == ReadyState::Loading &&
        m_readyState == ReadyState::Opened) {
        changeReadyState(ReadyState::HeadersReceived, true);
    }

    if (!isError() && readyState == ReadyState::Done &&
        m_readyState == ReadyState::HeadersReceived) {
        changeReadyState(ReadyState::Loading, true);
    } else if (!isError() && readyState == ReadyState::Done &&
               m_readyState == ReadyState::Opened) {
        changeReadyState(ReadyState::HeadersReceived, true);
        changeReadyState(ReadyState::Loading, true);
    }
    if (readyState == ReadyState::HeadersReceived) {
        auto& headerMap = m_responseHeaders->httpHeaderMap()->headerMap();
        auto it = headerMap.find(HTTPHeaderMap::kContentType);
        if (it != headerMap.end()) {
            m_responseData->m_mimeType =
                String::fromUTF8(it->second.data(), it->second.size());
        }

        it = headerMap.find(HTTPHeaderMap::kContentLanguage);
        if (it != headerMap.end()) {
            size_t pos = it->second.find(";");
            if (pos != std::string::npos) {
                m_contentLanguage =
                    String::fromUTF8(it->second.data(), it->second.size());
            } else {
                auto str = it->second.substr(0, pos);
                m_contentLanguage = String::fromUTF8(str.data(), str.size());
            }
        }

        it = headerMap.find(HTTPHeaderMap::kContentTransferEncoding);
        if (it != headerMap.end()) {
            std::string part = it->second;
            std::transform(part.begin(), part.end(), part.begin(), tolower);
            if (part.compare("base64") == 0) {
                m_containsBase64Content = true;
            }
        }

        std::vector<std::string> values;
        m_responseHeaders->httpHeaderMap()->extractHeaderListValues(
            values, HTTPHeaderMap::kAccessControlExposeHeaders);

        for (const auto& value : values) {
            m_responseData->m_corsExposedHeaderNameList.push_back(
                String::fromUTF8(value.data(), value.size()));
        }

        // TODO : https://fetch.spec.whatwg.org/#ref-for-concept-response-type
        ResourceURL* resURL = nullptr;
        if (m_lastLocation == "") {
            resURL = m_requestData->m_url;
        } else {
            resURL = new ResourceURL(
                String::createASCIIString(m_lastLocation.data(),
                                          m_lastLocation.size()),
                String::createASCIIString(m_lastEffectiveURL.data(),
                                          m_lastEffectiveURL.size()));
        }

        auto resWebOrigin = WebOrigin::createDocumentOrigin(resURL);
        if (!executionContext()->webOrigin()->isSameOrigin(resWebOrigin) &&
            !resWebOrigin->isOpaque()) {
            m_responseData->m_type = ResponseType::Cors;
        } else {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        }

    } else if (readyState == ReadyState::Done) {
        if (m_containsBase64Content) {
            m_responseData->m_responseBody =
                parseBase64String(m_responseData->m_responseBody, 0,
                                  m_responseData->m_responseBody.size());
        }
    }

    if (readyState != m_readyState) {
        m_readyState = readyState;
        for (size_t i = 0; i < m_clients.size(); i++) {
            m_clients[i]->onReadyStateChange(this, isExplicitAction);
        }
    }

    if (m_readyState == ReadyState::Done) {
        webBase()->messageLoop()->addIdler(
            globalScope(),
            [](size_t, void* data, void* data2) {
                ResourceRequest* self = (ResourceRequest*)data2;
                ((ExecutionContext*)data)->removePointerFromRootSet(data2);
            },
            executionContext(), this);
        if (m_jobDelegate) {
            m_jobDelegate = nullptr;
        }
    }
}

void ResourceRequest::changeProgress(ProgressState progress,
                                     bool isExplicitAction)
{
    STARFISH_ASSERT(isMainThread());
    if (m_progressState != progress || (progress == ProgressState::Progress)) {
        m_progressState = progress;
        for (size_t i = 0; i < m_clients.size(); i++) {
            m_clients[i]->onProgressEvent(this, isExplicitAction);
        }
    }

    if (m_progressState == ProgressState::LoadEnd) {
        if (m_responseData) {
            ResponseBody().swap(m_responseData->m_responseBody);
        }
    }
}

void ResourceRequest::open(RequestData* reqData)
{
    bool shouldAbort = false;
    m_requestData = reqData;
    m_requestHeaders = new HeadersData();

    m_preflightRequestData = new RequestData();
    m_preflightRequestData->m_method = String::createASCIIString("OPTIONS");
    m_requestWebOrigin = WebOrigin::createDocumentOrigin(m_requestData->m_url);

    m_responseData = new ResponseData();
    m_responseHeaders = new HeadersData();

    STARFISH_ASSERT(
        !((m_requestData->m_syncLevel == RequestSyncLevel::AlwaysSync) &&
          m_timeout != 0));
    shouldAbort = m_progressState >= ProgressState::LoadStart;

    if (shouldAbort) {
        abort(true);
    }
    initVariables();

    m_jobDelegate = ResourceRequestJobDelegateFactory::createJob(this);
    changeReadyState(ReadyState::Opened, true);
}

void ResourceRequest::abort(bool isExplicitAction)
{
    clearIdlers();

    if (m_readyState >= ReadyState::Unset) {
        m_requestError = RequestErrorType::UnknownError;
        auto theStatusWas = m_progressState;
        if (m_readyState == ReadyState::Opened && m_didSend) {
            changeProgress(ProgressState::Abort, false);
        } else {
            changeProgress(ProgressState::Abort, isExplicitAction);
        }

        if (theStatusWas == ProgressState::LoadEnd && isExplicitAction) {
            changeReadyState(ReadyState::Done, false);
        } else {
            changeReadyState(ReadyState::Done, m_didSend);
        }
        changeProgress(ProgressState::LoadEnd, true);
        changeReadyState(ReadyState::Unset, false);
    }
}

void ResourceRequest::send(String* body, bool allowCache)
{
    executionContext()->addPointerInRootSet(this);
    m_didSend = true;

    STARFISH_ASSERT(m_jobDelegate);
    m_jobDelegate->send(body, allowCache);

    changeProgress(ProgressState::LoadStart, true);
}

bool ResourceRequest::isSameOriginRequest()
{
    STARFISH_ASSERT(m_requestWebOrigin);
    return executionContext()->webOrigin()->isSameOrigin(m_requestWebOrigin);
}

void ResourceRequest::setRequestHeader(String* name, String* value)
{
    // Do not use HeadersData's append here, becuase name will change to lower
    // in the append. but xhr's behavior of append should not work that way
    m_requestHeaders->httpHeaderMap()->append(name->toUTF8NonGCString(),
                                              value->toUTF8NonGCString());
}

void ResourceRequest::deleteRequestHeader(String* name)
{
    m_requestHeaders->httpHeaderMap()->remove(name->toUTF8NonGCString());
}

bool ResourceRequest::isCORSsafelistedResponseHeaderName(
    const std::string& name)
{
    return FetchUtils::isCORSsafelistedResponseHeaderName(
        name, m_responseData ? &(m_responseData->m_corsExposedHeaderNameList)
                             : nullptr);
}

EncodeType ResourceRequest::toEncodeType(String* input)
{
    String* lowerType = input->toASCIILower();
    if (lowerType->equals("application/x-www-form-urlencoded")) {
        return EncodeType::ApplicationXWWWFormURLEncoded;
    } else if (lowerType->equals("multipart/form-data")) {
        return EncodeType::MultiPartFormData;
    } else if (lowerType->equals("text/plain")) {
        return EncodeType::TextPlain;
    }
    return EncodeType::MissingOrInvalidEncodeType;
}

String* ResourceRequest::encodeType(EncodeType input)
{
    switch (input) {
    case EncodeType::ApplicationXWWWFormURLEncoded:
        return String::createASCIIString("application/x-www-form-urlencoded");
    case EncodeType::MultiPartFormData:
        return String::createASCIIString("multipart/form-data");
    case EncodeType::TextPlain:
        return String::createASCIIString("text/plain");
    default:
        return String::emptyString;
    }
}

// https://www.w3.org/TR/html5/forms.html#application/
// x-www-form-urlencoded-encoding-algorithm
String* ResourceRequest::encodeFormDataSet(
    GCVector<FormDataSetItem*>* formDataSet, EncodeType formEnctype)
{
    String* result = String::createASCIIString("");
    if (formEnctype == EncodeType::ApplicationXWWWFormURLEncoded) {
        for (size_t i = 0; i < formDataSet->size(); i++) {
            FormDataSetItem* item = (*formDataSet)[i];
            String* name =
                ResourceURL::createPercentEncodingString(item->m_name, true);
            String* value =
                ResourceURL::createPercentEncodingString(item->m_value, true);
            String* type = item->m_type;

            if (i == 0 && name->equalsIgnoreCase("isindex") &&
                type->equalsIgnoreCase("text")) {
                result = result->concat(value);
                continue;
            }

            if (name->equalsIgnoreCase("_charset_") &&
                type->equalsIgnoreCase("hidden")) {
                value = String::createASCIIString("UTF-8");
            }

            if (i > 0) {
                result = result->concat(String::createASCIIString("&"));
            }
            result = result->concat(name);
            result = result->concat(String::createASCIIString("="));
            result = result->concat(value);
        }
    } else if (formEnctype == EncodeType::MultiPartFormData) {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    } else if (formEnctype == EncodeType::TextPlain) {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    return result;
}

ResourceURL* ResourceRequest::mutateActionURL(ResourceURL* url,
                                              FormSubmitData* formSubmitData)
{
    String* encodedFormData = encodeFormDataSet(formSubmitData->m_formDataSet,
                                                formSubmitData->m_enctype);
    return url->setSearch(encodedFormData);
}

template <typename StrType>
ResponseBody ResourceRequest::parseBase64String(const StrType& str,
                                                size_t startAt, size_t endAt)
{
    std::string decodedStr = Base64Utils::decodeBase64(
        StrType(str.begin() + startAt, str.begin() + (endAt - startAt)));
    return ResponseBody(decodedStr.begin(), decodedStr.end());
}

WebBase* ResourceRequest::webBase()
{
    return m_executionContext->webBase();
}

GlobalScope* ResourceRequest::globalScope()
{
    return m_executionContext->globalScope();
}

Starfish* ResourceRequest::starfish()
{
    return m_executionContext->webBase()->starfish();
}
}
