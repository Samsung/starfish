#include "StarFishConfig.h"
#include "NetworkRequest.h"

#include <curl/curl.h>

#include "platform/file_io/FileIO.h"
#include "platform/window/Window.h"
#include "dom/Document.h"
#include "platform/message_loop/MessageLoop.h"

namespace StarFish {

NetworkRequest::NetworkRequest(StarFish* starFish)
    : m_isAborted(false)
    , m_isSended(false)
    , m_isSync(false)
    , m_starFish(starFish)
    , m_url(String::emptyString, String::emptyString)
    , m_readyState(UNSENT)
    , m_progressState(NONE)
    , m_method(UNKNOWN_METHOD)
    , m_responseType(DEFAULT_RESPONSE)
    , m_status(0)
    , m_timeout(0)
    , m_pendingOnHeaderReceivedEventIdlerHandle(SIZE_MAX)
    , m_pendingOnProgressEventIdlerHandle(SIZE_MAX)
    , m_loaded(0)
    , m_total(0)
    , m_pendingNetworkWorkerEndIdlerHandle(SIZE_MAX)
    , m_onReadyStateChangeListener(nullptr)
    , m_onProgressListener(nullptr)
{

}

NetworkRequest::~NetworkRequest()
{
    STARFISH_ASSERT(m_pendingOnHeaderReceivedEventIdlerHandle == SIZE_MAX);
    STARFISH_ASSERT(m_pendingOnProgressEventIdlerHandle == SIZE_MAX);
    STARFISH_ASSERT(m_pendingNetworkWorkerEndIdlerHandle == SIZE_MAX);
    STARFISH_ASSERT(m_requstedIdlers.size() == 0);
}

void NetworkRequest::changeReadyState(ReadyState readyState)
{
    Locker<Mutex> locker(m_mutex);

    if (readyState != m_readyState) {
        m_readyState = readyState;
        if (m_onReadyStateChangeListener) {
            m_onReadyStateChangeListener(this, readyState);
        }
    }
}

void NetworkRequest::changeProgress(ProgressState progress)
{
    Locker<Mutex> locker(m_mutex);
    if (m_progressState != progress || (progress == PROGRESS)) {
        m_progressState = progress;
        if (m_onProgressListener) {
            m_onProgressListener(this, progress);
        }
    }
}

void NetworkRequest::open(MethodType method, String* url, bool async)
{
    bool shouldAbort = false;
    {
        Locker<Mutex> locker(m_mutex);
        STARFISH_ASSERT(!(!async && m_timeout != 0));
        shouldAbort = m_progressState >= LOADSTART;
    }
    if (shouldAbort) {
        abort();
    }
    {
        Locker<Mutex> locker(m_mutex);
        m_method = method;
        m_url = URL(m_starFish->window()->document()->documentURI().urlString(), url);
        m_isSync = async;
        changeReadyState(OPENED);
    }
}

void NetworkRequest::abort()
{
    Locker<Mutex> locker(m_mutex);
    auto iter2 = m_requstedIdlers.begin();
    while (iter2 != m_requstedIdlers.end()) {
        m_starFish->messageLoop()->removeIdler(*iter2);
        iter2++;
    }
    m_requstedIdlers.clear();

    if (m_pendingOnHeaderReceivedEventIdlerHandle != SIZE_MAX) {
        m_starFish->messageLoop()->removeIdlerWithNoGCRooting(m_pendingOnHeaderReceivedEventIdlerHandle);
        m_pendingOnHeaderReceivedEventIdlerHandle = SIZE_MAX;
    }

    if (m_pendingOnProgressEventIdlerHandle != SIZE_MAX) {
        m_starFish->messageLoop()->removeIdlerWithNoGCRooting(m_pendingOnProgressEventIdlerHandle);
        m_pendingOnProgressEventIdlerHandle = SIZE_MAX;
    }

    m_isAborted = true;
    if (!(((m_readyState == UNSENT || m_readyState == OPENED) && !m_isSended) || m_readyState == DONE)) {
        changeReadyState(DONE);
        changeProgress(ABORT);
        changeProgress(LOADEND);
    }
    changeReadyState(UNSENT);
}

struct NetworkWorkerData {
    NetworkRequest* request;
    CURL* curl;
};

void NetworkRequest::send(String* body)
{
    Locker<Mutex> locker(m_mutex);
    STARFISH_ASSERT(!m_isSended);
    m_isSended = true;
    changeProgress(LOADSTART);
    if (m_url.isFileURL()) {

        String* path = m_url.urlStringWithoutSearchPart();
        String* filePath = path->substring(7, path->length() - 7);

        if (m_isSync) {
            fileWorker(this, filePath);
        } else {
            pushIdlerHandle(m_starFish->messageLoop()->addIdler([](size_t handle, void* data, void* data1) {
                Resource* request = (Resource*)data;
                // this area doesn't require lock.
                // reading file is not require thread
                request->removeIdlerHandle(handle);
                fileWorker((NetworkRequest*)data, (String*)data1);
            }, this, filePath));
        }

    } else {
        CURL* curl = curl_easy_init();
        STARFISH_ASSERT(curl);

        NetworkWorkerData* data = new(NoGC) NetworkWorkerData;
        data->request = this;
        data->curl = curl;

        curl_easy_setopt(curl, CURLOPT_URL, m_url.urlString()->utf8Data());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, static_cast<unsigned long>(m_timeout));
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, [](void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
        {
            NetworkRequest* request = (NetworkRequest*)clientp;
            // check abort
            if (request->m_isAborted)
                return 1;

            Locker<Mutex> locker(request->m_mutex);
            request->m_loaded = static_cast<uint32_t>(dlnow);
            request->m_total = static_cast<uint32_t>(dltotal);
            if (request->m_pendingOnProgressEventIdlerHandle == SIZE_MAX) {
                request->m_pendingOnProgressEventIdlerHandle = request->m_starFish->messageLoop()->addIdlerWithNoGCRooting([](size_t handle, void* data) {
                    NetworkRequest* request = (NetworkRequest*)data;
                    Locker<Mutex> locker(request->m_mutex);
                    STARFISH_ASSERT(handle == request->m_pendingOnProgressEventIdlerHandle);
                    request->changeReadyState(LOADING);
                    request->changeProgress(PROGRESS);
                    request->m_pendingOnProgressEventIdlerHandle = SIZE_MAX;
                }, request);
            }
            return 0;
        });
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

        curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, [](void* ptr, size_t size, size_t nmemb, void* data)
        {
            NetworkRequest* request = (NetworkRequest*)data;
            Locker<Mutex> locker(request->m_mutex);
            size_t realSize = size * nmemb;

            const char* memPtr = (const char*)ptr;

            request->m_response.insert(request->m_response.end(), memPtr, memPtr + realSize);
            return realSize;
        });

        curl_easy_setopt(curl, CURLOPT_HEADERDATA, this);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, [] (void* ptr, size_t size, size_t nmemb, void* data) -> size_t {
            // TODO read header info
            size_t realsize = size * nmemb;
            NetworkRequest* request = (NetworkRequest*)data;
            Locker<Mutex> locker(request->m_mutex);

            STARFISH_ASSERT(request->m_pendingOnHeaderReceivedEventIdlerHandle == SIZE_MAX);
            request->m_pendingOnHeaderReceivedEventIdlerHandle = request->m_starFish->messageLoop()->addIdlerWithNoGCRooting([](size_t handle, void* data) {
                NetworkRequest* request = (NetworkRequest*)data;
                Locker<Mutex> locker(request->m_mutex);
                STARFISH_ASSERT(handle == request->m_pendingOnHeaderReceivedEventIdlerHandle);
                request->changeReadyState(HEADERS_RECEIVED);
                request->m_pendingOnHeaderReceivedEventIdlerHandle = SIZE_MAX;
            }, request);

            return realsize;
        });

        if (m_method == POST_METHOD) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body->utf8Data());
        } else {
            STARFISH_ASSERT(m_method == GET_METHOD);
        }


        m_starFish->addPointerInRootSet(this);
        pthread_t thread;
        pthread_create(&thread, NULL, networkWorker, data);
    }
}

void NetworkRequest::fileWorker(NetworkRequest* res, String* filePath)
{
    FileIO* fio = FileIO::create();
    if (fio->open(filePath)) {
        res->m_status = 200;
        res->changeReadyState(HEADERS_RECEIVED);
        res->changeReadyState(LOADING);
        size_t responseLength = fio->length();
        res->m_response.resize(responseLength);
        fio->read(res->m_response.data(), sizeof(const char), responseLength);
        fio->close();
        fio->close();

        res->changeReadyState(DONE);
        res->changeProgress(PROGRESS);
        res->changeProgress(LOAD);
        res->changeProgress(LOADEND);
    } else {
        res->m_status = 0;
        res->changeReadyState(DONE);
        res->changeProgress(ERROR);
    }
    delete fio;
}

void* NetworkRequest::networkWorker(void* data)
{
    NetworkWorkerData* requestData = (NetworkWorkerData*)data;
    CURL* curl = requestData->curl;

    auto res = curl_easy_perform(curl);
    if (res == 0) {
        Locker<Mutex> locker(requestData->request->m_mutex);
        requestData->request->m_pendingNetworkWorkerEndIdlerHandle = requestData->request->m_starFish->messageLoop()->addIdlerWithNoGCRooting([](size_t handle, void* data) {
            NetworkWorkerData* requestData = (NetworkWorkerData*)data;
            Locker<Mutex> locker(requestData->request->m_mutex);
            requestData->request->m_pendingNetworkWorkerEndIdlerHandle = SIZE_MAX;

            requestData->request->changeProgress(LOAD);
            requestData->request->changeProgress(LOADEND);
            requestData->request->changeReadyState(DONE);

            requestData->request->m_starFish->removePointerFromRootSet(requestData->request);
        }, requestData->request);

    } else if (res == CURLE_ABORTED_BY_CALLBACK) {

    } else {
        // handle error
        Locker<Mutex> locker(requestData->request->m_mutex);
        requestData->request->m_pendingNetworkWorkerEndIdlerHandle = requestData->request->m_starFish->messageLoop()->addIdlerWithNoGCRooting([](size_t handle, void* data) {
            NetworkWorkerData* requestData = (NetworkWorkerData*)data;
            Locker<Mutex> locker(requestData->request->m_mutex);
            requestData->request->m_pendingNetworkWorkerEndIdlerHandle = SIZE_MAX;

            requestData->request->changeProgress(ERROR);
            requestData->request->changeProgress(LOADEND);
            requestData->request->changeReadyState(DONE);

            requestData->request->m_starFish->removePointerFromRootSet(requestData->request);
        }, requestData->request);
    }

    curl_easy_cleanup(curl);
    return NULL;
}


}
