#ifndef __StarFishNetworkRequest__
#define __StarFishNetworkRequest__

#include <curl/curl.h>

#include "platform/threading/Mutex.h"
#include "platform/threading/Locker.h"

namespace StarFish {

class Document;
class NetworkRequest;

class NetworkRequestClient : public gc {
public:
    virtual ~NetworkRequestClient() { }
    virtual void onProgressEvent(NetworkRequest* request, bool isExplicitAction) { }
    virtual void onReadyStateChange(NetworkRequest* request, bool isExplicitAction) { }
};

class NetworkRequest : public gc_cleanup {
    friend class XMLHttpRequest;
public:
    enum MethodType {
        UNKNOWN_METHOD,
        POST_METHOD,
        GET_METHOD
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
        DONE
    };

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
    ~NetworkRequest();
    void open(MethodType method, String* url, bool async, String* userName = String::emptyString, String* password = String::emptyString);
    void abort(bool isExplicitAction = true);
    void send(String* body = String::emptyString);

    void setTimeout(uint32_t ms)
    {
        m_timeout = ms;
    }

    const uint32_t& timeout()
    {
        return m_timeout;
    }

    ReadyState readyState()
    {
        return m_readyState;
    }

    ProgressState progressState()
    {
        return m_progressState;
    }

    size_t loaded()
    {
        return m_loaded;
    }

    size_t total()
    {
        return m_total;
    }

    int status()
    {
        return m_status;
    }

    bool isSync()
    {
        return m_isSync;
    }

    Document* document()
    {
        return m_document;
    }

    StarFish* starFish()
    {
        return m_starFish;
    }

    const std::vector<char>& responseData()
    {
        return m_response;
    }

    void addNetworkRequestClient(NetworkRequestClient* client)
    {
        m_clients.push_back(client);
    }

    // this is for XHR
    // direct reference of NetworkRequest with client
    // cause gc-cycle
    void addNetworkRequestClientInNonGCArea(NetworkRequestClient* client)
    {
        m_clientsWithNoGC.push_back(client);
    }

protected:
    void initVariables();
    void clearIdlers();
    static void fileWorker(NetworkRequest* res, String* filePath);
    static void dataURLWorker(NetworkRequest* res, String* url);
    static int curlProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);
    static size_t curlWriteCallback(void* ptr, size_t size, size_t nmemb, void* data);
    static size_t curlWriteHeaderCallback(void* ptr, size_t size, size_t nmemb, void* data);
    static void* networkWorker(void*);
    void changeReadyState(ReadyState readyState, bool isExplicitAction);
    void changeProgress(ProgressState progress, bool isExplicitAction);
    bool m_isAborted;
    bool m_isSync;
    bool m_isReceivedHeader;
    bool m_didSend;
    StarFish* m_starFish;
    Document* m_document;
    URL m_url;
    // FIXME should we store userName, password in m_url?
    String* m_userName;
    String* m_password;
    ReadyState m_readyState;
    ProgressState m_progressState;
    MethodType m_method;
    ResponseType m_responseType;
    int m_status;
    uint32_t m_timeout;
    Mutex m_mutex;
    std::vector<char> m_response;

    std::vector<size_t, gc_allocator<size_t>> m_requstedIdlers;
    void pushIdlerHandle(size_t handle)
    {
        m_requstedIdlers.push_back(handle);
    }

    void removeIdlerHandle(size_t handle)
    {
        m_requstedIdlers.erase(std::find(m_requstedIdlers.begin(), m_requstedIdlers.end(), handle));
    }

    volatile size_t m_pendingOnHeaderReceivedEventIdlerHandle;
    volatile size_t m_pendingOnProgressEventIdlerHandle;
    // progress event data
    volatile size_t m_loaded;
    volatile size_t m_total;

    volatile size_t m_pendingNetworkWorkerEndIdlerHandle;

    std::vector<NetworkRequestClient*, gc_allocator<NetworkRequestClient*>> m_clients;
    std::vector<NetworkRequestClient*> m_clientsWithNoGC;
};
}

#endif
