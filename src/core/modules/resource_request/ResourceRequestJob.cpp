/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/fileapi/Blob.h"
#include "platform/file/FileIO.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/modules/resource_request/ResourceRequestJob.h"
#include "core/modules/resource_request/NetworkURLWorkerHelper.h"
#include "core/modules/threading/ThreadPool.h"
#include "core/util/URL.h"

namespace StarFish {

ResourceRequestJobInterface* ResourceRequestJobDelegateFactory::createJob(
    ResourceRequest* proxy)
{
    if (proxy->url()->isFileURL()) {
        return new FileURLResourceRequestJobDelegate(proxy);
    } else if (proxy->url()->isDataURL()) {
        return new DataURLResourceRequestJobDelegate(proxy);
    } else if (proxy->url()->isBlobURL()) {
        return new BlobURLResourceRequestJobDelegate(proxy);
    } else if (proxy->url()->isNetworkURL()) {
        return new NetworkURLResourceRequestJobDelegate(proxy);
    }
    STARFISH_ASSERT_NOT_REACHED();
    return nullptr;
}

FileURLResourceRequestJobDelegate::FileURLResourceRequestJobDelegate(
    ResourceRequest* proxy)
    : m_orgProxy(proxy)
{
}

void FileURLResourceRequestJobDelegate::send(String* body)
{
    STARFISH_ASSERT(m_orgProxy->m_url->isFileURL());
    // this area doesn't require lock.
    // reading file does not require thread
    String* path = m_orgProxy->m_url->getUrlPathString();
    String* filePath = path->substring(7, path->length() - 7);

    if (m_orgProxy->m_isSync) {
        worker(m_orgProxy, filePath);
    } else {
        size_t handle = m_orgProxy->starFish()->messageLoop()->addIdler(
            [](size_t handle, void* data, void* data1) {
                ResourceRequest* request = (ResourceRequest*)data;
                request->removeIdlerHandle(handle);
                FileURLResourceRequestJobDelegate::worker(
                    (ResourceRequest*)data, (String*)data1);
            },
            m_orgProxy, filePath);
        m_orgProxy->pushIdlerHandle(handle);
    }
}

void FileURLResourceRequestJobDelegate::worker(ResourceRequest* res,
                                               String* filePath)
{
    FileIO* fio = FileIO::create();
    if (fio->open(filePath)) {
        res->m_status = 200;
        res->changeReadyState(ResourceRequest::HEADERS_RECEIVED, true);
        res->changeReadyState(ResourceRequest::LOADING, true);
        size_t responseLength = fio->length();
        res->m_response.resize(responseLength);
        fio->read(res->m_response.data(), sizeof(const char), responseLength);
        fio->close();
        res->handleResponseEOF();
    } else {
        STARFISH_LOG_INFO("failed to open %s\n",
                          res->m_url->urlString()->utf8Data());
        res->m_status = 0;
        res->handleError(ResourceRequest::ERROR);
    }
    delete fio;
}

DataURLResourceRequestJobDelegate::DataURLResourceRequestJobDelegate(
    ResourceRequest* proxy)
    : m_orgProxy(proxy)
{
}

void DataURLResourceRequestJobDelegate::send(String* body)
{
    STARFISH_ASSERT(m_orgProxy->m_url->isDataURL());
    // this area doesn't require lock.
    // reading url does not require thread
    if (m_orgProxy->m_isSync) {
        worker(m_orgProxy, m_orgProxy->m_url->urlString());
    } else {
        size_t handle = m_orgProxy->starFish()->messageLoop()->addIdler(
            [](size_t handle, void* data, void* data1) {
                ResourceRequest* request = (ResourceRequest*)data;
                request->removeIdlerHandle(handle);
                DataURLResourceRequestJobDelegate::worker(
                    (ResourceRequest*)data, (String*)data1);
            },
            m_orgProxy, m_orgProxy->m_url->urlString());
        m_orgProxy->pushIdlerHandle(handle);
    }
}

void DataURLResourceRequestJobDelegate::worker(ResourceRequest* res,
                                               String* url)
{
    res->m_status = 200;

    size_t idxColon = url->indexOf(':');
    size_t idx = url->indexOf(',');

    if (idx != SIZE_MAX && idxColon != SIZE_MAX && idxColon < idx) {
        String* sub =
            url->substring(idxColon + 1, idx - idxColon - 1)->toLower();
        size_t base64 = sub->find(";base64");

        if (base64 == sub->length() - 7) {
            sub = sub->substring(0, base64);
            res->m_responseHeaderData =
                "Content-Transfer-Encoding:base64\r\nContent-Type:";
        } else {
            res->m_responseHeaderData = "Content-Type:";
        }

        for (size_t i = 0; i < sub->length(); i++) {
            res->m_responseHeaderData.push_back((char)sub->charAt(i));
        }
    }
    res->changeReadyState(ResourceRequest::HEADERS_RECEIVED, true);

    res->changeReadyState(ResourceRequest::LOADING, true);

    // TODO filter url string correctly according RFC 3986
    String* decodedURL = decodeURL(url, idx + 1);
    const char* utf8Data = decodedURL->utf8Data();

    size_t len = strlen(utf8Data);
    for (size_t i = 0; i < len; i++) {
        res->m_response.push_back(utf8Data[i]);
    }

    res->handleResponseEOF();
}

BlobURLResourceRequestJobDelegate::BlobURLResourceRequestJobDelegate(
    ResourceRequest* proxy)
    : m_orgProxy(proxy)
{
}

void BlobURLResourceRequestJobDelegate::send(String* body)
{
    STARFISH_ASSERT(m_orgProxy->m_url->isBlobURL());
    // this area doesn't require lock.
    // reading url does not require thread
    if (m_orgProxy->m_isSync) {
        worker(m_orgProxy, m_orgProxy->m_url->urlString());
    } else {
        size_t handle = m_orgProxy->starFish()->messageLoop()->addIdler(
            [](size_t handle, void* data, void* data1) {
                ResourceRequest* request = (ResourceRequest*)data;
                request->removeIdlerHandle(handle);
                BlobURLResourceRequestJobDelegate::worker(
                    (ResourceRequest*)data, (String*)data1);
            },
            m_orgProxy, m_orgProxy->m_url->urlString());
        m_orgProxy->pushIdlerHandle(handle);
    }
}

void BlobURLResourceRequestJobDelegate::worker(ResourceRequest* res,
                                               String* url)
{
    res->m_status = 200;

    BlobURLStore store;
    if (!StarFish::stringToBlobURLString(url, store)) {
        res->handleError(ResourceRequest::ERROR);
        return;
    }

    if (!res->starFish()->isValidBlobURL(store)) {
        res->handleError(ResourceRequest::ERROR);
        return;
    }

    res->m_responseMimeType = ((Blob*)store.m_blob)->type();
    res->changeReadyState(ResourceRequest::HEADERS_RECEIVED, true);

    res->changeReadyState(ResourceRequest::LOADING, true);
    char* buf = (char*)((Blob*)store.m_blob)->data();
    res->m_response.assign(buf, &buf[((Blob*)store.m_blob)->size()]);

    res->handleResponseEOF();
}

NetworkURLResourceRequestJobDelegate::NetworkURLResourceRequestJobDelegate(
    ResourceRequest* proxy)
    : m_orgProxy(proxy)
{
}

void NetworkURLResourceRequestJobDelegate::send(String* body)
{
    STARFISH_ASSERT(m_orgProxy->m_url->isNetworkURL());
    NetworkURLWorkerData* data = new (NoGC) NetworkURLWorkerData;
    m_orgProxy->m_activeNetworkURLWorkerData = data;
    {
        CURL* curl = curl_easy_init();
        STARFISH_ASSERT(curl);

        data->request = m_orgProxy;
        data->curl = curl;
        data->isSync = m_orgProxy->m_isSync;
        data->isAborted = false;
        data->res = -1;

#ifdef STARFISH_TIZEN_WEARABLE
        connection_h connection;
        int conn_err;
        conn_err = connection_create(&connection);
        char* proxy_address = NULL;
        if (conn_err == CONNECTION_ERROR_NONE) {
            connection_get_proxy(connection, CONNECTION_ADDRESS_FAMILY_IPV4,
                                 &proxy_address);
            if (proxy_address) {
                curl_easy_setopt(curl, CURLOPT_PROXY, proxy_address);
                free(proxy_address);
            }
            connection_destroy(connection);
        } else {
            STARFISH_LOG_INFO(
                "got error while opening tizen network connection\n");
        }
#endif
        const char* url = m_orgProxy->m_url->urlString()->utf8Data();
        curl_easy_setopt(curl, CURLOPT_URL, url);
        STARFISH_LOG_INFO("sending network request to %s\n", url);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS,
                         static_cast<unsigned long>(m_orgProxy->m_timeout));

        std::string headerText;
        struct curl_slist* list = NULL;
        // list = curl_slist_append(list, "Accept:text/plain");
        list = curl_slist_append(list, "Accept-Charset:utf-8");
        headerText = "Accept-Language:";
        headerText += m_orgProxy->starFish()->locale().getName();
        headerText.replace(headerText.begin(), headerText.end(), '_', '-');
        list = curl_slist_append(list, headerText.data());
        list = curl_slist_append(list, "Connection:keep-alive");
        list = curl_slist_append(
            list, "User-Agent: " USER_AGENT(APP_CODE_NAME, VERSION));

        if (!m_orgProxy->m_document->documentURI()->isNetworkURL()) {
            list = curl_slist_append(list, "Origin:null");
        } else {
            headerText = "Host:";
            headerText += m_orgProxy->m_url->hostname()->utf8Data();
            list = curl_slist_append(list, headerText.data());
            headerText = "Referer:";
            headerText += m_orgProxy->m_document->urlString()->utf8Data();
            list = curl_slist_append(list, headerText.data());
        }

        for (size_t i = 0; i < m_orgProxy->m_requestHeaders.size(); i++) {
            headerText =
                std::string(m_orgProxy->m_requestHeaders[i].first->utf8Data()) +
                ":";
            headerText += m_orgProxy->m_requestHeaders[i].second->utf8Data();
            list = curl_slist_append(list, headerText.data());
        }

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
        data->headerList = list;

        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
        // TODO: we should prevent infinite redirect
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 128);

        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, curlProgressCallback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, data);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

        curl_easy_setopt(curl, CURLOPT_HEADERDATA, data);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, curlWriteHeaderCallback);

        curl_easy_setopt(curl, CURLOPT_WRITEDATA, m_orgProxy);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);

#ifdef STARFISH_ENABLE_TEST
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif

        if (m_orgProxy->m_method == ResourceRequest::POST_METHOD) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body->utf8Data());
        } else {
            STARFISH_ASSERT(m_orgProxy->m_method ==
                            ResourceRequest::GET_METHOD);
        }
    }

    if (m_orgProxy->m_isSync) {
        data->networkWorker = new SyncNetworkWorkHelper();
        worker(data);
    } else {
        data->networkWorker = new AsyncNetworkWorkHelper();
        m_orgProxy->starFish()->threadPool()->addWork(
            NetworkURLResourceRequestJobDelegate::worker, data);
        // Thread* t = new Thread();
        // t->run(networkWorker, data);
    }
}

void* NetworkURLResourceRequestJobDelegate::worker(void* data)
{
    NetworkURLWorkerData* requestData = (NetworkURLWorkerData*)data;
    return requestData->networkWorker->networkWorker(data);
}

int NetworkURLResourceRequestJobDelegate::curlProgressCallback(
    void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal,
    curl_off_t ulnow)
{
    NetworkURLWorkerData* workerData = (NetworkURLWorkerData*)clientp;
    ResourceRequest* request = workerData->request;
    Locker<Mutex> locker(*request->m_mutex);
    // check abort
    if (workerData->isAborted) {
        return 1;
    }

    request->m_loaded = static_cast<uint32_t>(dlnow);
    request->m_total = static_cast<uint32_t>(dltotal);
    return 0;
}

size_t NetworkURLResourceRequestJobDelegate::curlWriteCallback(void* ptr,
                                                               size_t size,
                                                               size_t nmemb,
                                                               void* data)
{
    ResourceRequest* request = (ResourceRequest*)data;
    Locker<Mutex> locker(*request->m_mutex);

    size_t realSize = size * nmemb;
    const char* memPtr = (const char*)ptr;

    request->m_response.insert(request->m_response.end(), memPtr,
                               memPtr + realSize);

    if (request->m_pendingOnProgressEventIdlerHandle == SIZE_MAX) {
        if (request->isSync()) {
            request->changeReadyState(ResourceRequest::LOADING, true);
            request->changeProgress(ResourceRequest::PROGRESS, true);
        } else {
            request->m_pendingOnProgressEventIdlerHandle =
                request->starFish()
                    ->messageLoop()
                    ->addIdlerWithNoGCRootingInOtherThread(
                        [](size_t handle, void* data) {
                            ResourceRequest* request = (ResourceRequest*)data;
                            Locker<Mutex> locker(*request->m_mutex);
                            {
                                STARFISH_ASSERT(
                                    handle ==
                                    request
                                        ->m_pendingOnProgressEventIdlerHandle);
                                request->m_pendingOnProgressEventIdlerHandle =
                                    SIZE_MAX;
                            }
                            request->changeReadyState(ResourceRequest::LOADING,
                                                      true);
                            request->changeProgress(ResourceRequest::PROGRESS,
                                                    true);
                        },
                        request);
        }
    }

    return realSize;
}

size_t NetworkURLResourceRequestJobDelegate::curlWriteHeaderCallback(
    void* ptr, size_t size, size_t nmemb, void* data)
{
    size_t realsize = size * nmemb;
    NetworkURLWorkerData* request = (NetworkURLWorkerData*)data;
    Locker<Mutex> locker(*request->request->m_mutex);

    size_t realSize = size * nmemb;
    const char* memPtr = (const char*)ptr;

    long code;
    curl_easy_getinfo(request->curl, CURLINFO_RESPONSE_CODE, &code);

    if (request->res != (int)code) {
        request->res = (int)code;
        request->request->m_responseHeaderData.clear();
    }

    request->request->m_responseHeaderData.insert(
        request->request->m_responseHeaderData.end(), memPtr,
        memPtr + realSize);
    return realsize;
}

static String* decodeURL(String* src, size_t idx)
{
    bool gotUTF32Char = false;
    UTF32String ret;

    while (idx < src->length()) {
        char32_t c = src->charAt(idx);
        if (c == '%') {
            char32_t ch = 0;
            bool ok = true;
            for (size_t i = 0; i < 2; i++) {
                if (idx >= src->length()) {
                    ok = false;
                    break;
                }
                idx++;
                c = src->charAt(idx);
                char32_t current = 0;
                if (c < ':') {
                    current = c - 48;
                } else if (c > '@' && c < '[') {
                    current = (c - 'A') + 10;
                } else {
                    current = (c - 'a') + 10;
                }

                if ((16 * (1 - i))) {
                    current = (current * 16);
                }
                ch += current;
            }
            if (ok) {
                if (ch > 127) {
                    gotUTF32Char = true;
                }
                ret += ch;
            }
        } else {
            if (c > 127) {
                gotUTF32Char = true;
            }
            ret += c;
        }
        idx++;
    }
    if (gotUTF32Char) {
        return new StringDataUTF32(std::move(ret));
    } else {
        return String::createASCIIStringFromUTF32Source(ret);
    }
}
}
