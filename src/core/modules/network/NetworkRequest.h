/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef __StarFishNetworkRequest__
#define __StarFishNetworkRequest__

#include "binding/DocumentHoldable.h"
#include "core/util/URL.h"
#include "core/modules/threading/Mutex.h"
#include "core/modules/threading/Semaphore.h"
#include "core/modules/threading/Locker.h"

namespace StarFish {

class Document;
class NetworkRequest;
class NetworkWorkerHelper;

typedef std::vector<char> NetworkRequestResponse;
typedef std::string NetworkRequestResponseHeader;

class NetworkRequestClient : public gc {
public:
    virtual ~NetworkRequestClient()
    {
    }
    virtual void onProgressEvent(NetworkRequest* request, bool isExplicitAction)
    {
    }
    virtual void onReadyStateChange(NetworkRequest* request,
                                    bool isExplicitAction)
    {
    }
};

struct NetworkWorkerData {
    NetworkRequest* request;
    NetworkWorkerHelper* networkWorker;
    CURL* curl;
    curl_slist* headerList;
    bool isSync;
    bool isAborted;
    long responseCode;
    int res;
};

class NetworkWorkerHelper : public gc {
public:
    NetworkWorkerHelper()
    {
    }
    virtual ~NetworkWorkerHelper()
    {
    }
    void* networkWorker(void* data);

protected:
    virtual void responseHandlerWrapper(int res, NetworkWorkerData* requestData)
    {
    }
    static void responseHandler(size_t handle, void* requestData);
};

class AsyncNetworkWorkHelper : public NetworkWorkerHelper {
protected:
    virtual void responseHandlerWrapper(int res,
                                        NetworkWorkerData* requestData);
};

class SyncNetworkWorkHelper : public NetworkWorkerHelper {
protected:
    virtual void responseHandlerWrapper(int res,
                                        NetworkWorkerData* requestData);
};

class NetworkRequest : public gc, public DocumentHoldable {
    friend class XMLHttpRequest;
    friend class NetworkWorkerHelper;
    friend class AsyncNetworkWorkHelper;
    friend void NetworkRequestFileWorker(NetworkRequest* res, String* filePath);
    friend void NetworkRequestDataURLWorker(NetworkRequest* res, String* url);
    friend void NetworkRequestBlobURLWorker(NetworkRequest* res, String* url);

public:
    enum MethodType { UNKNOWN_METHOD, POST_METHOD, GET_METHOD };

    enum ResponseType {
        TEXT_RESPONSE,
        ARRAY_BUFFER_RESPONSE,
        BLOB_RESPONSE,
        DOCUMENT_RESPONSE,
        JSON_RESPONSE,
        DEFAULT_RESPONSE
    };

    enum ReadyState { UNSENT, OPENED, HEADERS_RECEIVED, LOADING, DONE };

    enum ProgressState {
        NONE,
        LOADSTART,
        PROGRESS,
        LOAD,
        ERROR,
        ABORT,
        TIMEOUT,
        LOADEND,
    };

    NetworkRequest(Document* document);
    void open(MethodType method, String* url, bool async,
              String* userName = String::emptyString,
              String* password = String::emptyString);
    void abort(bool isExplicitAction = true);
    void send(String* body = String::emptyString);

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

    const NetworkRequestResponseHeader& responseHeaderData()
    {
        return m_responseHeaderData;
    }

    NetworkRequestResponse& response()
    {
        return m_response;
    }

    String* responseMimeType()
    {
        return m_responseMimeType;
    }

    void addNetworkRequestClient(NetworkRequestClient* client)
    {
        m_clients.push_back(client);
    }

    void clearNetworkRequestClient()
    {
        m_clients.clear();
    }

    URL* url()
    {
        return m_url;
    }

    void setRequestHeader(String* h, String* c);

protected:
    void pareseHeader(const char* header, size_t len);
    void initVariables();
    void clearIdlers();
    static void fileWorker(NetworkRequest* res, String* filePath);
    static void dataURLWorker(NetworkRequest* res, String* url);
    static void blobURLWorker(NetworkRequest* res, String* url);
    static int curlProgressCallback(void* clientp, curl_off_t dltotal,
                                    curl_off_t dlnow, curl_off_t ultotal,
                                    curl_off_t ulnow);
    static size_t curlWriteCallback(void* ptr, size_t size, size_t nmemb,
                                    void* data);
    static size_t curlWriteHeaderCallback(void* ptr, size_t size, size_t nmemb,
                                          void* data);
    static void* networkWorker(void*);

    template <typename StrType>
    static NetworkRequestResponse parseBase64String(const StrType& str,
                                                    size_t startAt,
                                                    size_t endAt);
    void changeReadyState(ReadyState readyState, bool isExplicitAction);
    void changeProgress(ProgressState progress, bool isExplicitAction);
    void handleResponseEOF();
    void handleError(ProgressState error);
    bool m_isSync;
    bool m_didSend;
    bool m_gotError;
    bool m_containsBase64Content;
    URL* m_url;
    ReadyState m_readyState;
    ProgressState m_progressState;
    MethodType m_method;
    ResponseType m_responseType;
    uint16_t m_status;
    uint32_t m_timeout;
    NetworkWorkerData* m_activeNetworkWorkerData;
    Mutex* m_mutex;
    String* m_responseMimeType;
    NetworkRequestResponse m_response;
    NetworkRequestResponseHeader m_responseHeaderData;
    GCVector<size_t> m_requstedIdlers;
    GCVector<std::pair<String*, String*>> m_requestHeaders;
    void pushIdlerHandle(size_t handle)
    {
        m_requstedIdlers.push_back(handle);
    }

    void removeIdlerHandle(size_t handle)
    {
        m_requstedIdlers.erase(std::find(m_requstedIdlers.begin(),
                                         m_requstedIdlers.end(), handle));
    }

    volatile size_t m_pendingOnHeaderReceivedEventIdlerHandle;
    volatile size_t m_pendingOnProgressEventIdlerHandle;
    // progress event data
    volatile size_t m_loaded;
    volatile size_t m_total;

    volatile size_t m_pendingNetworkWorkerEndIdlerHandle;

    GCVector<NetworkRequestClient*> m_clients;
};

inline void NetworkRequestFileWorker(NetworkRequest* res, String* filePath)
{
    NetworkRequest::fileWorker(res, filePath);
}

inline void NetworkRequestDataURLWorker(NetworkRequest* res, String* url)
{
    NetworkRequest::dataURLWorker(res, url);
}

inline void NetworkRequestBlobURLWorker(NetworkRequest* res, String* url)
{
    NetworkRequest::blobURLWorker(res, url);
}
}

#endif
