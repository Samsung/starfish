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

#ifndef __StarfishResourceRequest__
#define __StarfishResourceRequest__

#include "binding/DocumentHoldable.h"
#include "platform/network/http/HTTPUtil.h"
#include "core/util/URL.h"
#include "core/modules/threading/Mutex.h"
#include "core/modules/threading/Semaphore.h"
#include "core/modules/threading/Locker.h"
#include "core/modules/resource_request/ResourceRequestJob.h"
#include "core/modules/resource_request/NetworkURLResourceRequestJobDelegate.h"
#include "core/fetch/RequestData.h"

namespace Starfish {

class Document;
class ResourceRequest;
class FormDataSetItem;
class WebOrigin;

typedef std::vector<char> EntityBody;

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

class ResourceRequest : public gc,
                        public DocumentHoldable,
                        public ResourceRequestJobInterface {
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

public:
    virtual ~ResourceRequest()
    {
    }

    ResourceRequest(Document* document);
    void open(RequestData* reqData, bool async);
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
        return m_status;
    }

    bool isSync() const
    {
        return m_isSync;
    }

    const HeaderMap& responseHeaderMap()
    {
        return m_responseHeaderMap;
    }

    // Reading response is only safe when onProgress callback fired | request
    // ended
    EntityBody& response()
    {
        return m_response;
    }

    BodyType bodyType()
    {
        return m_bodyType;
    }

    ResponseType responseType()
    {
        return m_responseType;
    }

    String* responseMimeType()
    {
        return m_responseMimeType;
    }

    String* contentLanguage()
    {
        return m_contentLanguage;
    }

    std::string lastEffectiveURL()
    {
        return m_lastEffectiveURL;
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
        if (!m_requestData) {
            return new ResourceURL(String::emptyString);
        }
        return m_requestData->m_url;
    }

    ReferrerURL* referrer()
    {
        if (!m_requestData) {
            return new ReferrerURL(String::emptyString);
        }
        return m_requestData->m_referrer;
    }

    String* method()
    {
        if (!m_requestData) {
            return String::emptyString;
        }
        return m_requestData->m_method;
    }

    RequestMode requestMode()
    {
        if (!m_requestData) {
            return RequestMode::CORS;
        }
        return m_requestData->m_mode;
    }

    RequestDestination requestDestination()
    {
        if (!m_requestData) {
            return RequestDestination::Empty;
        }
        return m_requestData->m_destination;
    }

    bool isSubresourceRequest()
    {
        if (!m_requestData) {
            return false;
        }

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
        return m_gotError;
    }

    void setRequestHeader(String* h, String* c);

    RequestCredentials requestCredentials()
    {
        if (!m_requestData) {
            return RequestCredentials::Omit;
        }
        return m_requestData->m_credentials;
    }

    void setRequestCredentials(RequestCredentials value)
    {
        if (m_requestData) {
            m_requestData->m_credentials = value;
        }
    }

    static EncodeType toEncodeType(String* input);
    static String* encodeType(EncodeType input);

    String* encodeFormDataSet(GCVector<FormDataSetItem*>* formDataSet,
                              EncodeType formEnctype);
    ResourceURL* mutateActionURL(ResourceURL* url,
                                 FormSubmitData* formSubmitData);

    void setCorsFlag(bool value)
    {
        m_corsFlag = value;
    }
    bool corsFlag()
    {
        return m_corsFlag;
    }

protected:
    void pareseHeader(const char* header, size_t len);
    void initVariables();
    void clearIdlers();

    template <typename StrType>
    static EntityBody parseBase64String(const StrType& str, size_t startAt,
                                        size_t endAt);
    void changeReadyState(ReadyState readyState, bool isExplicitAction);
    void changeProgress(ProgressState progress, bool isExplicitAction);
    void handleResponseEOF();
    void handleError(ProgressState error);
    void handleConnectError();

    void pushIdlerHandle(size_t handle)
    {
        m_requstedIdlers.push_back(handle);
    }

    void removeIdlerHandle(size_t handle)
    {
        m_requstedIdlers.erase(std::find(m_requstedIdlers.begin(),
                                         m_requstedIdlers.end(), handle));
    }

    bool m_isSync;
    bool m_didSend;
    bool m_gotError;
    bool m_containsBase64Content;

    RequestData* m_requestData;
    WebOrigin* m_requestWebOrigin;
    bool m_corsFlag; // https://fetch.spec.whatwg.org/#main-fetch

    ReadyState m_readyState;
    ProgressState m_progressState;

    BodyType m_bodyType;

    ResponseType m_responseType;
    uint16_t m_status;
    uint32_t m_timeout;
    NetworkURLWorkerData* m_activeNetworkURLWorkerData;
    Mutex* m_mutex;
    String* m_responseMimeType;
    String* m_contentLanguage;
    std::string m_lastEffectiveURL;
    EntityBody m_response;
    GCVector<size_t> m_requstedIdlers;
    GCVector<std::pair<String*, String*>> m_requestHeaders;

    ResourceRequestJobInterface* m_networkRequestJobDelegate;

    volatile size_t m_pendingOnHeaderReceivedEventIdlerHandle;
    volatile size_t m_pendingOnProgressEventIdlerHandle;
    // progress event data
    volatile size_t m_loaded;
    volatile size_t m_total;

    GCVector<ResourceRequestClient*> m_clients;

    HeaderMap m_responseHeaderMap;
};
}

#endif
