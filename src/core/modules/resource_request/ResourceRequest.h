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

#ifndef __StarFishResourceRequest__
#define __StarFishResourceRequest__

#include "binding/DocumentHoldable.h"
#include "platform/network/http/HTTPUtil.h"
#include "core/util/URL.h"
#include "core/modules/threading/Mutex.h"
#include "core/modules/threading/Semaphore.h"
#include "core/modules/threading/Locker.h"
#include "core/modules/resource_request/ResourceRequestJob.h"
#include "core/modules/resource_request/NetworkURLResourceRequestJobDelegate.h"

namespace StarFish {

class Document;
class ResourceRequest;
class FormDataSetItem;

typedef std::vector<char> EntityBody;

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
    friend class NetworkURLResourceRequestJobDelegate;
    friend class UnknownURLResourceRequestJobDelegate;
    friend class EventSource;

public:
    virtual ~ResourceRequest()
    {
    }
    enum MethodType {
        UNKNOWN_METHOD,
        GET_METHOD,
        HEAD_METHOD,
        POST_METHOD,
        PUT_METHOD,
        DELETE_METHOD,
        CONNECT_METHOD,
        OPTION_METHOD,
        TRACE_METHOD,
        PATCH_METHOD
    };

    enum ResponseType {
        TEXT_RESPONSE,
        ARRAY_BUFFER_RESPONSE,
        BLOB_RESPONSE,
        DOCUMENT_RESPONSE,
        JSON_RESPONSE,
        DEFAULT_RESPONSE
    };

    enum ReadyState {
        UNSENT,
        OPENED,
        HEADERS_RECEIVED,
        LOADING,
        DONE,
    };

    enum ProgressState {
        NONE,
        LOADSTART,
        PROGRESS,
        LOAD,
        IN_ERROR,
        ABORT,
        TIMEOUT,
        LOADEND,
    };

    enum EncodeType {
        APPLICATION_X_WWW_FORM_URLENCODED,
        MULTIPART_FORM_DATA,
        TEXT_PLAIN,
        MISSING_OR_INVALID_ENCODETYPE,
    };

    ResourceRequest(Document* document);
    void open(ResourceRequest::MethodType method, ResourceURL* url, bool async,
              ResourceURL* referrer, String* userName = String::emptyString,
              String* password = String::emptyString);
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

    String* responseMimeType()
    {
        return m_responseMimeType;
    }

    String* contentLanguage()
    {
        return m_contentLanguage;
    }

    String* lastLocation()
    {
        return m_lastLocation;
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
        return m_url;
    }

    ResourceURL* referrer()
    {
        return m_referrer;
    }

    bool isError()
    {
        return m_gotError;
    }

    void setRequestHeader(String* h, String* c);

    static ResourceRequest::MethodType toMethodType(String* input);
    static String* methodType(ResourceRequest::MethodType method);
    static ResourceRequest::EncodeType toEncodeType(String* input);
    static String* encodeType(ResourceRequest::EncodeType input);

    String* encodeFormDataSet(GCVector<FormDataSetItem*>* formDataSet,
                              ResourceRequest::EncodeType formEnctype);
    ResourceURL* mutateActionURL(ResourceURL* url,
                                 FormSubmitData* formSubmitData);

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
    ResourceURL* m_url;
    ResourceURL* m_referrer;
    ReadyState m_readyState;
    ProgressState m_progressState;
    MethodType m_method;
    ResponseType m_responseType;
    uint16_t m_status;
    uint32_t m_timeout;
    NetworkURLWorkerData* m_activeNetworkURLWorkerData;
    Mutex* m_mutex;
    String* m_responseMimeType;
    String* m_contentLanguage;
    String* m_lastLocation;
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
