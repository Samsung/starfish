/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishResourceRequest__
#define __StarfishResourceRequest__

#include "core/modules/resource_request/ResourceRequestJob.h"
#include "core/fetch/RequestData.h"
#include "core/fetch/ResponseData.h"
#include "core/fetch/HeadersData.h"

namespace Starfish {

class Document;
class ResourceRequest;
class FormDataSetItem;
class WebOrigin;
class Resource;
class WebBase;
class GlobalScope;
class ExecutionContext;
class Mutex;

struct NetworkURLWorkerData;

enum class BodyType;
enum class ResponseType;

enum class ReadyState : uint8_t {
    Unset,
    Opened,
    HeadersReceived,
    Loading,
    Done,
};

enum class ProgressState {
    None,
    LoadStart,
    Progress,
    Load,
    InError,
    Abort,
    TimeOut,
    LoadEnd,
};

enum class EncodeType {
    ApplicationXWWWFormURLEncoded,
    MultiPartFormData,
    TextPlain,
    MissingOrInvalidEncodeType,
};

enum class AbortRequestType {
    NoPendingRequest,
    AbortWithNoError,
    AbortWithError,
};

enum class RequestErrorType {
    NoError,
    UnknownError,
    HostLookupError,
    UnsupportedAuthSchemeError,
    AuthenticationError,
    ProxyAuthenticationError,
    ConnectError,
    IOError,
    TimeoutError,
    RedirectLoopError,
    UnsupportedSchemeError,
    FailedSSLHandshakeError,
    BadURLError,
    FileError,
    FileNotFoundError,
    TooManyRequestError,
};

class ResourceRequestClient : public gc {
public:
    virtual ~ResourceRequestClient()
    {
    }
    virtual void onProgressEvent(ResourceRequest* request,
                                 bool isExplicitAction)
    {
    }
    virtual void onReadyStateChange(ResourceRequest* request,
                                    bool isExplicitAction)
    {
    }
};

class ResourceRequest : public gc, public ResourceRequestJobInterface {
    friend class XMLHttpRequest;
    friend class NetworkURLWorkerHelper;
    friend class AsyncNetworkWorkHelper;
    friend class FileURLResourceRequestJobDelegate;
    friend class DataURLResourceRequestJobDelegate;
    friend class BlobURLResourceRequestJobDelegate;
    friend class AboutURLResourceRequestJobDelegate;
    friend class JavaScriptURLResourceRequestJobDelegate;
    friend class NetworkURLResourceRequestJobDelegate;
    friend class UnknownURLResourceRequestJobDelegate;
    friend class EventSource;
    friend class Resource;

public:
    virtual ~ResourceRequest()
    {
    }

    ResourceRequest(ExecutionContext* executionContext);
    void open(RequestData* reqData);
    void abort(bool isExplicitAction = true);
    virtual void send(String* body = String::emptyString,
                      bool allowCache = false);

    void setTimeout(uint32_t ms)
    {
        m_timeout = ms;
    }

    uint32_t timeout() const
    {
        return m_timeout;
    }

    ReadyState readyState() const
    {
        return m_readyState;
    }

    ProgressState progressState() const
    {
        return m_progressState;
    }

    size_t loaded() const
    {
        return m_loaded;
    }

    size_t total() const
    {
        return m_total;
    }

    uint16_t status() const
    {
        STARFISH_ASSERT(m_responseData);
        return m_responseData->m_status;
    }

    bool isRedirected() const
    {
        STARFISH_ASSERT(m_responseData);
        return m_responseData->m_redirected;
    }

    bool isSync() const
    {
        STARFISH_ASSERT(m_requestData);
        return m_requestData->m_syncLevel == RequestSyncLevel::AlwaysSync;
    }

    const HeaderMap& responseHeaderMap()
    {
        return m_responseHeaders->httpHeaderMap()->headerMap();
    }

    // Reading response is only safe when onProgress callback fired | request
    // ended
    ResponseBody& response()
    {
        STARFISH_ASSERT(m_responseData);
        return m_responseData->m_responseBody;
    }

    BodyType bodyType()
    {
        return m_bodyType;
    }

    ResponseType responseType()
    {
        STARFISH_ASSERT(m_responseData);
        return m_responseData->m_type;
    }

    String* responseMimeType()
    {
        STARFISH_ASSERT(m_responseData);
        return m_responseData->m_mimeType;
    }

    String* contentLanguage()
    {
        return m_contentLanguage;
    }

    std::string lastLocation()
    {
        return m_lastLocation;
    }

    std::string lastEffectiveURL()
    {
        return m_lastEffectiveURL;
    }

    void setLastEffectiveURL(char* effectiveURL)
    {
        m_lastEffectiveURL = effectiveURL;
    }

    void addResourceRequestClient(ResourceRequestClient* client)
    {
        m_clients.push_back(client);
    }

    void clearResourceRequestClient()
    {
        m_clients.clear();
    }

    ResourceURL* url()
    {
        STARFISH_ASSERT(m_requestData);
        return m_requestData->m_url;
    }

    ReferrerURL* referrer()
    {
        STARFISH_ASSERT(m_requestData);
        return m_requestData->m_referrer;
    }

    String* method()
    {
        STARFISH_ASSERT(m_requestData);
        return m_requestData->m_method;
    }

    RequestMode requestMode()
    {
        STARFISH_ASSERT(m_requestData);
        return m_requestData->m_mode;
    }

    RequestDestination requestDestination()
    {
        STARFISH_ASSERT(m_requestData);
        return m_requestData->m_destination;
    }

    bool isSubresourceRequest()
    {
        STARFISH_ASSERT(m_requestData);
        switch (m_requestData->m_destination) {
        case RequestDestination::Audio:
        case RequestDestination::AudioWorkLet:
        case RequestDestination::Font:
        case RequestDestination::Image:
        case RequestDestination::Manifest:
        case RequestDestination::PaintWorkLet:
        case RequestDestination::Script:
        case RequestDestination::Style:
        case RequestDestination::Track:
        case RequestDestination::Video:
        case RequestDestination::Xslt:
            return true;
            break;
        default:
            return false;
            break;
        }
    }

    bool isSameOriginRequest();

    bool isError()
    {
        return (m_requestData && m_requestError != RequestErrorType::NoError);
    }

    RequestErrorType errorType()
    {
        return m_requestError;
    }

    void setRequestHeader(String* name, String* value);
    void deleteRequestHeader(String* name);

    RequestCredentials requestCredentials()
    {
        STARFISH_ASSERT(m_requestData);
        return m_requestData->m_credentials;
    }

    void setRequestCredentials(RequestCredentials value)
    {
        STARFISH_ASSERT(m_requestData);
        m_requestData->m_credentials = value;
    }

    ResponseTainting responseTainting()
    {
        STARFISH_ASSERT(m_requestData);
        return m_requestData->m_responseTainting;
    }

    void setResponseTainting(ResponseTainting value)
    {
        STARFISH_ASSERT(m_requestData);
        m_requestData->m_responseTainting = value;
    }

    bool useCorsPreflightFlag()
    {
        STARFISH_ASSERT(m_requestData);
        return m_requestData->m_useCorsPreflightFlag;
    }

    void setUseCorsPreflightFlag(bool value)
    {
        STARFISH_ASSERT(m_requestData);
        m_requestData->m_useCorsPreflightFlag = value;
    }

    bool unsafeRequestFlag()
    {
        STARFISH_ASSERT(m_requestData);
        return m_requestData->m_unsafeRequestFlag;
    }

    void setUnsafeRequestFlag(bool value)
    {
        STARFISH_ASSERT(m_requestData);
        m_requestData->m_unsafeRequestFlag = value;
    }

    bool isCORSsafelistedResponseHeaderName(const std::string& name);

    static EncodeType toEncodeType(String* input);
    static String* encodeType(EncodeType input);

    String* encodeFormDataSet(GCVector<FormDataSetItem*>* formDataSet,
                              EncodeType formEnctype);
    ResourceURL* mutateActionURL(ResourceURL* url,
                                 FormSubmitData* formSubmitData);

    void requestAbortOnRequestClient(
        AbortRequestType type = AbortRequestType::AbortWithError)
    {
        m_abortRequestState = type;
    }

    ExecutionContext* executionContext()
    {
        return m_executionContext;
    }

    WebBase* webBase();
    GlobalScope* globalScope();
    Starfish* starfish();

protected:
    void pareseHeader(const char* header, size_t len);
    void initVariables();
    void clearIdlers();

    template <typename StrType>
    static ResponseBody parseBase64String(const StrType& str, size_t startAt,
                                          size_t endAt);
    void changeReadyState(ReadyState readyState, bool isExplicitAction);
    void changeProgress(ProgressState progress, bool isExplicitAction);
    void handleResponseEOF();
    void handleError(ProgressState error, RequestErrorType errorType);
    void handleConnectError();
    void handleResponseEOFwithPreflightRequestRedirected();

    void pushIdlerHandle(size_t handle)
    {
        m_requstedIdlers.push_back(handle);
    }

    void removeIdlerHandle(size_t handle)
    {
        m_requstedIdlers.erase(std::find(m_requstedIdlers.begin(),
                                         m_requstedIdlers.end(), handle));
    }

    bool m_didSend;
    bool m_containsBase64Content;

    ExecutionContext* m_executionContext;
    RequestData* m_requestData;
    RequestData* m_preflightRequestData;
    WebOrigin* m_requestWebOrigin;
    HeadersData* m_requestHeaders;

    ResponseData* m_responseData;
    HeadersData* m_responseHeaders;

    ReadyState m_readyState;
    ProgressState m_progressState;

    BodyType m_bodyType;

    uint32_t m_timeout;
    NetworkURLWorkerData* m_activeNetworkURLWorkerData;
    Mutex* m_mutex;
    String* m_contentLanguage;
    std::string m_lastLocation;
    std::string m_lastEffectiveURL;

    GCVector<size_t> m_requstedIdlers;

    ResourceRequestJobInterface* m_jobDelegate;

    volatile size_t m_pendingOnHeaderReceivedEventIdlerHandle;
    volatile size_t m_pendingOnProgressEventIdlerHandle;
    // progress event data
    volatile size_t m_loaded;
    volatile size_t m_total;
    AbortRequestType m_abortRequestState;
    RequestErrorType m_requestError;

    GCVector<ResourceRequestClient*> m_clients;
};
}

#endif
