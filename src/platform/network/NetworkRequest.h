#ifndef __StarFishNetworkRequest__
#define __StarFishNetworkRequest__


#include "platform/threading/Mutex.h"
#include "platform/threading/Locker.h"

namespace StarFish {

class NetworkRequest : public gc_cleanup {
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

    NetworkRequest(StarFish* starFish);
    ~NetworkRequest();
    void open(MethodType method, String* url, bool async);
    void abort();
    void send(String* body);

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

protected:
    static void fileWorker(NetworkRequest* res, String* filePath);
    static void* networkWorker(void*);
    void changeReadyState(ReadyState readyState);
    void changeProgress(ProgressState progress);
    bool m_isAborted;
    bool m_isSended;
    bool m_isSync;
    StarFish* m_starFish;
    URL m_url;
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

    size_t m_pendingOnHeaderReceivedEventIdlerHandle;
    size_t m_pendingOnProgressEventIdlerHandle;
    // progress event data
    size_t m_loaded;
    size_t m_total;

    size_t m_pendingNetworkWorkerEndIdlerHandle;

    void (*m_onReadyStateChangeListener)(NetworkRequest*, ReadyState);
    void (*m_onProgressListener)(NetworkRequest*, ProgressState);
};
}

#endif
