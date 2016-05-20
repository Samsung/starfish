#include "StarFishConfig.h"
#include "NetworkRequest.h"

#include "platform/file_io/FileIO.h"
#include "platform/window/Window.h"
#include "dom/Document.h"
#include "platform/message_loop/MessageLoop.h"

#ifdef STARFISH_TIZEN_WEARABLE
#include <net_connection.h>
#endif

namespace StarFish {

class ActiveNetworkRequestTracker : public NetworkRequestClient {
public:
    virtual void onProgressEvent(NetworkRequest* request, bool isExplicitAction)
    {
        if (request->progressState() == NetworkRequest::LOADSTART) {
            request->document()->m_activeNetworkRequests.push_back(request);
        } else if (request->progressState() == NetworkRequest::LOADEND) {
            std::vector<NetworkRequest*, gc_allocator<NetworkRequest*>>& v = request->document()->m_activeNetworkRequests;
            auto iter = std::find(v.begin(), v.end(), request);
            if (iter != v.end())
                v.erase(iter);
        }
    }
};

NetworkRequest::NetworkRequest(Document* document)
    : m_starFish(document->window()->starFish())
    , m_document(document)
    , m_url(document->documentURI()) // FIXME implement empty url
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
{
    initVariables();
    addNetworkRequestClient(new ActiveNetworkRequestTracker());
}


NetworkRequest::~NetworkRequest()
{
    clearIdlers();
}

void NetworkRequest::initVariables()
{
    m_userName = String::emptyString;
    m_password = String::emptyString;
    m_response.clear();
    m_isAborted = false;
    m_isSync = false;
    m_isReceivedHeader = false;
    m_didSend = false;
    m_total = 0;
    m_loaded = 0;
    m_status = 0;
    m_timeout = 0;

}

void NetworkRequest::clearIdlers()
{
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

    if (m_pendingNetworkWorkerEndIdlerHandle != SIZE_MAX) {
        m_starFish->messageLoop()->removeIdlerWithNoGCRooting(m_pendingNetworkWorkerEndIdlerHandle);
        m_pendingNetworkWorkerEndIdlerHandle = SIZE_MAX;
    }
}

void NetworkRequest::changeReadyState(ReadyState readyState, bool isExplicitAction)
{
    Locker<Mutex> locker(m_mutex);

    if (readyState != m_readyState) {
        m_readyState = readyState;
        for (size_t i = 0; i < m_clients.size(); i ++) {
            m_clients[i]->onReadyStateChange(this, isExplicitAction);
        }

        for (size_t i = 0; i < m_clientsWithNoGC.size(); i ++) {
            m_clientsWithNoGC[i]->onReadyStateChange(this, isExplicitAction);
        }
    }
}

void NetworkRequest::changeProgress(ProgressState progress, bool isExplicitAction)
{
    Locker<Mutex> locker(m_mutex);
    if (m_progressState != progress || (progress == PROGRESS)) {
        m_progressState = progress;
        for (size_t i = 0; i < m_clients.size(); i ++) {
            m_clients[i]->onProgressEvent(this, isExplicitAction);
        }

        for (size_t i = 0; i < m_clientsWithNoGC.size(); i ++) {
            m_clientsWithNoGC[i]->onProgressEvent(this, isExplicitAction);
        }
    }
}

void NetworkRequest::open(MethodType method, String* url, bool async, String* userName, String* password)
{
    bool shouldAbort = false;
    {
        Locker<Mutex> locker(m_mutex);
        STARFISH_ASSERT(!(!async && m_timeout != 0));
        shouldAbort = m_progressState >= LOADSTART;
    }
    if (shouldAbort) {
        abort(true);
    }
    {
        initVariables();
        Locker<Mutex> locker(m_mutex);
        m_method = method;
        m_url = URL(m_starFish->window()->document()->documentURI().urlString(), url);
        m_userName = userName;
        m_password = password;
        m_isSync = !async;
        changeReadyState(OPENED, true);
    }
}

void NetworkRequest::abort(bool isExplicitAction)
{
    Locker<Mutex> locker(m_mutex);
    clearIdlers();

    m_isAborted = true;
    if (m_readyState >= UNSENT) {
        auto theStatusWas = m_progressState;
        if (m_readyState == OPENED && m_didSend) {
            changeProgress(ABORT, false);
        } else {
            changeProgress(ABORT, isExplicitAction);
        }

        if (theStatusWas == LOADEND && isExplicitAction) {
            changeReadyState(DONE, false);
        } else {
            changeReadyState(DONE, m_didSend);
        }
        changeProgress(LOADEND, true);
        changeReadyState(UNSENT, false);
    }
}

struct NetworkWorkerData {
    NetworkRequest* request;
    CURL* curl;
    curl_slist* headerList;
};


int NetworkRequest::curlProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
    NetworkRequest* request = (NetworkRequest*)clientp;
    Locker<Mutex> locker(request->m_mutex);
    // check abort
    if (request->m_isAborted)
        return 1;

    request->m_loaded = static_cast<uint32_t>(dlnow);
    request->m_total = static_cast<uint32_t>(dltotal);
    if (request->m_pendingOnProgressEventIdlerHandle == SIZE_MAX && request->m_isReceivedHeader) {
        request->m_pendingOnProgressEventIdlerHandle = request->m_starFish->messageLoop()->addIdlerWithNoGCRootingInOtherThread([](size_t handle, void* data) {
            NetworkRequest* request = (NetworkRequest*)data;
            Locker<Mutex> locker(request->m_mutex);
            STARFISH_ASSERT(handle == request->m_pendingOnProgressEventIdlerHandle);
            request->changeReadyState(LOADING, true);
            request->changeProgress(PROGRESS, true);
            request->m_pendingOnProgressEventIdlerHandle = SIZE_MAX;
        }, request);
    }
    return 0;
}

size_t NetworkRequest::curlWriteCallback(void* ptr, size_t size, size_t nmemb, void* data)
{
    NetworkRequest* request = (NetworkRequest*)data;
    Locker<Mutex> locker(request->m_mutex);
    size_t realSize = size * nmemb;

    const char* memPtr = (const char*)ptr;

    request->m_response.insert(request->m_response.end(), memPtr, memPtr + realSize);
    return realSize;
}

size_t NetworkRequest::curlWriteHeaderCallback(void* ptr, size_t size, size_t nmemb, void* data)
{
    size_t realsize = size * nmemb;
    NetworkRequest* request = (NetworkRequest*)data;
    Locker<Mutex> locker(request->m_mutex);

    // TODO read header info
    if (request->m_pendingOnHeaderReceivedEventIdlerHandle == SIZE_MAX) {
        request->m_isReceivedHeader = true;
        request->m_pendingOnHeaderReceivedEventIdlerHandle = request->m_starFish->messageLoop()->addIdlerWithNoGCRootingInOtherThread([](size_t handle, void* data) {
            NetworkRequest* request = (NetworkRequest*)data;
            Locker<Mutex> locker(request->m_mutex);
            STARFISH_ASSERT(handle == request->m_pendingOnHeaderReceivedEventIdlerHandle);
            request->changeReadyState(HEADERS_RECEIVED, true);
            request->m_pendingOnHeaderReceivedEventIdlerHandle = SIZE_MAX;
        }, request);
    }
    return realsize;
}

void NetworkRequest::send(String* body)
{
    bool isFileURL = false;
    bool isSync = false;
    {
        Locker<Mutex> locker(m_mutex);
        changeProgress(LOADSTART, true);
        isFileURL = m_url.isFileURL();
        isSync = m_isSync;
        m_didSend = true;
    }
    if (isFileURL) {
        // this area doesn't require lock.
        // reading file is not require thread
        String* path = m_url.urlStringWithoutSearchPart();
        String* filePath = path->substring(7, path->length() - 7);

        if (m_isSync) {
            fileWorker(this, filePath);
        } else {
            size_t handle = m_starFish->messageLoop()->addIdler([](size_t handle, void* data, void* data1) {
                NetworkRequest* request = (NetworkRequest*)data;
                request->removeIdlerHandle(handle);
                fileWorker((NetworkRequest*)data, (String*)data1);
            }, this, filePath);
            pushIdlerHandle(handle);
        }
    } else {
        NetworkWorkerData* data = new(NoGC) NetworkWorkerData;
        {
            Locker<Mutex> locker(m_mutex);
            CURL* curl = curl_easy_init();
            STARFISH_ASSERT(curl);

            data->request = this;
            data->curl = curl;

#ifdef STARFISH_TIZEN_WEARABLE
            connection_h connection;
            int conn_err;
            conn_err = connection_create(&connection);
            char* proxy_address = NULL;
            if (conn_err == CONNECTION_ERROR_NONE) {
                connection_get_proxy(connection, CONNECTION_ADDRESS_FAMILY_IPV4, &proxy_address);
                if (proxy_address) {
                    STARFISH_LOG_INFO("tizen proxy address is %s\n", proxy_address);
                    curl_easy_setopt(curl, CURLOPT_PROXY, proxy_address);
                    free(proxy_address);
                }
                connection_destroy(connection);
            } else {
                STARFISH_LOG_INFO("got error while opening tizen network connection\n");
            }
#endif

            curl_easy_setopt(curl, CURLOPT_URL, m_url.urlString()->utf8Data());
            curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, static_cast<unsigned long>(m_timeout));

            std::string headerText;
            struct curl_slist* list = NULL;
            list = curl_slist_append(list, "Accept:text/plain");
            list = curl_slist_append(list, "Accept-Charset:utf-8");
            headerText = "Accept-Language:";
            headerText += m_starFish->locale().getName();
            headerText.replace(headerText.begin(), headerText.end(), '_', '-');
            list = curl_slist_append(list, headerText.data());
            list = curl_slist_append(list, "Connection:keep-alive");
            list = curl_slist_append(list, "Origin:null");
            list = curl_slist_append(list, "User-Agent:" USER_AGENT(APP_CODE_NAME, VERSION));

            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
            data->headerList = list;

            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
            // TODO
            // we should prevent infinite redirect
            curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 128);

            curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, curlProgressCallback);
            curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

            curl_easy_setopt(curl, CURLOPT_HEADERDATA, this);
            curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, curlWriteHeaderCallback);

            curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);

            if (m_userName->length()) {
                curl_easy_setopt(curl, CURLOPT_USERNAME, m_userName->utf8Data());
            }
            if (m_password->length()) {
                curl_easy_setopt(curl, CURLOPT_USERPWD, m_password->utf8Data());
            }

            if (m_method == POST_METHOD) {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body->utf8Data());
            } else {
                STARFISH_ASSERT(m_method == GET_METHOD);
            }

            m_starFish->addPointerInRootSet(this);
        }
        if (m_isSync) {
            networkWorker(data);
        } else {
            pthread_t thread;
            pthread_create(&thread, NULL, networkWorker, data);
        }

    }
}

void NetworkRequest::fileWorker(NetworkRequest* res, String* filePath)
{
    FileIO* fio = FileIO::create();
    if (fio->open(filePath)) {
        res->m_status = 200;
        res->changeReadyState(HEADERS_RECEIVED, true);
        res->changeReadyState(LOADING, true);
        size_t responseLength = fio->length();
        res->m_response.resize(responseLength);
        fio->read(res->m_response.data(), sizeof(const char), responseLength);
        fio->close();

        res->changeReadyState(DONE, true);
        res->changeProgress(PROGRESS, true);
        res->changeProgress(LOAD, true);
        res->changeProgress(LOADEND, true);
    } else {
        STARFISH_LOG_INFO("failed to open %s\n", res->m_url.urlString()->utf8Data());
        res->m_status = 0;
        res->changeProgress(ERROR, true);
        res->changeProgress(LOADEND, true);
        res->changeReadyState(DONE, true);
    }
    delete fio;
}

void* NetworkRequest::networkWorker(void* data)
{
    NetworkWorkerData* requestData = (NetworkWorkerData*)data;
    bool isSync;
    CURL* curl = requestData->curl;
    curl_slist* list = requestData->headerList;
    {
        Locker<Mutex> locker(requestData->request->m_mutex);
        isSync = requestData->request->m_isSync;
    }
    auto res = curl_easy_perform(curl);
    // FIXME duplicate code block sync, async
    if (isSync) {
        if (res == 0) {
            Locker<Mutex> locker(requestData->request->m_mutex);
            requestData->request->m_pendingNetworkWorkerEndIdlerHandle = SIZE_MAX;

            long code;
            curl_easy_getinfo(requestData->curl, CURLINFO_RESPONSE_CODE, &code);
            requestData->request->m_status = code;
            requestData->request->changeProgress(LOAD, true);
            requestData->request->changeProgress(LOADEND, true);
            requestData->request->changeReadyState(DONE, true);

            requestData->request->m_starFish->removePointerFromRootSet(requestData->request);

            GC_FREE(requestData);
        } else if (res == CURLE_ABORTED_BY_CALLBACK) {
            requestData->request->m_starFish->removePointerFromRootSet(requestData->request);
            GC_FREE(requestData);
        } else {
            NetworkWorkerData* requestData = (NetworkWorkerData*)data;
            Locker<Mutex> locker(requestData->request->m_mutex);
            requestData->request->m_pendingNetworkWorkerEndIdlerHandle = SIZE_MAX;

            STARFISH_LOG_INFO("failed to open %s\n", requestData->request->m_url.urlString()->utf8Data());
            long code;
            curl_easy_getinfo(requestData->curl, CURLINFO_RESPONSE_CODE, &code);
            requestData->request->m_status = code;
            requestData->request->changeProgress(ERROR, true);
            requestData->request->changeProgress(LOADEND, true);
            requestData->request->changeReadyState(DONE, true);

            requestData->request->m_starFish->removePointerFromRootSet(requestData->request);

            GC_FREE(requestData);
        }
    } else {
        if (res == 0) {
            Locker<Mutex> locker(requestData->request->m_mutex);
            requestData->request->m_pendingNetworkWorkerEndIdlerHandle = requestData->request->m_starFish->messageLoop()->addIdlerWithNoGCRootingInOtherThread([](size_t handle, void* data) {
                NetworkWorkerData* requestData = (NetworkWorkerData*)data;
                Locker<Mutex> locker(requestData->request->m_mutex);
                requestData->request->m_pendingNetworkWorkerEndIdlerHandle = SIZE_MAX;

                long code;
                curl_easy_getinfo(requestData->curl, CURLINFO_RESPONSE_CODE, &code);
                requestData->request->m_status = code;
                requestData->request->changeProgress(LOAD, true);
                requestData->request->changeProgress(LOADEND, true);
                requestData->request->changeReadyState(DONE, true);

                requestData->request->m_starFish->removePointerFromRootSet(requestData->request);

                GC_FREE(requestData);
            }, requestData);

        } else if (res == CURLE_ABORTED_BY_CALLBACK) {
            Locker<Mutex> locker(requestData->request->m_mutex);
            requestData->request->m_pendingNetworkWorkerEndIdlerHandle = requestData->request->m_starFish->messageLoop()->addIdlerWithNoGCRootingInOtherThread([](size_t handle, void* data) {
                NetworkWorkerData* requestData = (NetworkWorkerData*)data;
                requestData->request->m_starFish->removePointerFromRootSet(requestData->request);
                GC_FREE(requestData);
            }, requestData);
        } else {
            // handle error
            Locker<Mutex> locker(requestData->request->m_mutex);
            requestData->request->m_pendingNetworkWorkerEndIdlerHandle = requestData->request->m_starFish->messageLoop()->addIdlerWithNoGCRootingInOtherThread([](size_t handle, void* data) {
                NetworkWorkerData* requestData = (NetworkWorkerData*)data;
                Locker<Mutex> locker(requestData->request->m_mutex);
                requestData->request->m_pendingNetworkWorkerEndIdlerHandle = SIZE_MAX;

                long code;
                curl_easy_getinfo(requestData->curl, CURLINFO_RESPONSE_CODE, &code);
                STARFISH_LOG_INFO("failed to open %s[%d]\n", requestData->request->m_url.urlString()->utf8Data(), (int)code);
                requestData->request->m_status = code;
                requestData->request->changeProgress(ERROR, true);
                requestData->request->changeProgress(LOADEND, true);
                requestData->request->changeReadyState(DONE, true);

                requestData->request->m_starFish->removePointerFromRootSet(requestData->request);

                GC_FREE(requestData);
            }, requestData);
        }
    }

    curl_slist_free_all(list);
    curl_easy_cleanup(curl);
    return NULL;
}


}
