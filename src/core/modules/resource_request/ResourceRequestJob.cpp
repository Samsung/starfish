/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/page/WebView.h"
#include "core/dom/Document.h"
#include "core/fileapi/Blob.h"
#include "platform/file/File.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/modules/resource_request/ResourceRequestJob.h"
#include "core/modules/resource_request/NetworkURLResourceRequestJobDelegate.h"
#include "core/modules/message_loop/MessageLoop.h"
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
    } else if (proxy->url()->isAboutURL()) {
        return new AboutURLResourceRequestJobDelegate(proxy);
    } else if (proxy->url()->isNetworkURL()) {
        return new NetworkURLResourceRequestJobDelegate(proxy);
    } else if (proxy->url()->isJavascriptURL()) {
        return new JavaScriptURLResourceRequestJobDelegate(proxy);
    } else {
        return new UnknownURLResourceRequestJobDelegate(proxy);
    }
}

FileURLResourceRequestJobDelegate::FileURLResourceRequestJobDelegate(
    ResourceRequest* proxy)
    : m_orgProxy(proxy)
{
}

void FileURLResourceRequestJobDelegate::send(String* body, bool allowCache)
{
    STARFISH_ASSERT(m_orgProxy->m_url->isFileURL());
    // this area doesn't require lock.
    // reading file does not require thread
    String* path = m_orgProxy->m_url->getUrlPathString();
#if defined(OS_WINDOWS)
    String* filePath = path->substring(8, path->length() - 8);
#else
    String* filePath = path->substring(7, path->length() - 7);
#endif

    if (m_orgProxy->m_isSync) {
        worker(m_orgProxy, filePath);
    } else {
        size_t handle = m_orgProxy->starFish()->messageLoop()->addIdler(
            m_orgProxy->document()->browsingContext(),
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
    File* fio = File::create();
    if (fio->open(filePath, File::Read)) {
        res->m_status = 200;
        res->changeReadyState(ResourceRequest::HEADERS_RECEIVED, true);
        res->changeReadyState(ResourceRequest::LOADING, true);
        size_t responseLength = fio->size();
        res->m_response.resize(responseLength);
        fio->read(res->m_response.data(), sizeof(const char), responseLength);
        fio->close();
        res->handleResponseEOF();
    } else {
        auto s = res->m_url->urlString()->toUTF8NonGCString();
        STARFISH_LOG_INFO("failed to open %s\n", s.data());
        res->m_status = 0;
        res->handleError(ResourceRequest::IN_ERROR);
    }
    delete fio;
}

DataURLResourceRequestJobDelegate::DataURLResourceRequestJobDelegate(
    ResourceRequest* proxy)
    : m_orgProxy(proxy)
{
}

void DataURLResourceRequestJobDelegate::send(String* body, bool allowCache)
{
    STARFISH_ASSERT(m_orgProxy->m_url->isDataURL());
    // this area doesn't require lock.
    // reading url does not require thread
    if (m_orgProxy->m_isSync) {
        worker(m_orgProxy, m_orgProxy->m_url->urlString());
    } else {
        size_t handle = m_orgProxy->starFish()->messageLoop()->addIdler(
            m_orgProxy->document()->browsingContext(),
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

    String* mimeType = String::emptyString;
    if (idx != SIZE_MAX && idxColon != SIZE_MAX && idxColon < idx) {
        String* sub =
            url->substring(idxColon + 1, idx - idxColon - 1)->toASCIILower();
        mimeType = sub;
        size_t base64 = sub->find(";base64");

        if (base64 == sub->length() - 7) {
            sub = sub->substring(0, base64);
            mimeType = sub;
            res->m_containsBase64Content = true;
        }
    }

    res->m_responseMimeType = mimeType;
    res->changeReadyState(ResourceRequest::HEADERS_RECEIVED, true);

    res->changeReadyState(ResourceRequest::LOADING, true);

    // TODO filter url string correctly according RFC 3986
    String* decodedURL = decodeURL(url, idx + 1);
    UTF8StringDataNonGCStd utf8Data = decodedURL->toUTF8NonGCString();

    size_t len = utf8Data.length();
    for (size_t i = 0; i < len; i++) {
        res->m_response.push_back(utf8Data[i]);
    }

    res->handleResponseEOF();
}

AboutURLResourceRequestJobDelegate::AboutURLResourceRequestJobDelegate(
    ResourceRequest* proxy)
    : m_orgProxy(proxy)
{
}

void AboutURLResourceRequestJobDelegate::send(String* body, bool allowCache)
{
    STARFISH_ASSERT(m_orgProxy->m_url->isAboutURL());
    // this area doesn't require lock.
    if (m_orgProxy->m_isSync) {
        worker(m_orgProxy, m_orgProxy->m_url->urlString());
    } else {
        size_t handle = m_orgProxy->starFish()->messageLoop()->addIdler(
            m_orgProxy->document()->browsingContext(),
            [](size_t handle, void* data, void* data1) {
                ResourceRequest* request = (ResourceRequest*)data;
                request->removeIdlerHandle(handle);
                AboutURLResourceRequestJobDelegate::worker(
                    (ResourceRequest*)data, (String*)data1);
            },
            m_orgProxy, m_orgProxy->m_url->urlString());
        m_orgProxy->pushIdlerHandle(handle);
    }
}

void AboutURLResourceRequestJobDelegate::worker(ResourceRequest* res,
                                                String* url)
{
    if (res->url()->pathname()->equalsIgnoreCase("blank")) {
        res->m_status = 200;
    } else {
        res->m_status = 404;
    }

    res->changeReadyState(ResourceRequest::HEADERS_RECEIVED, true);
    res->changeReadyState(ResourceRequest::LOADING, true);
    res->handleResponseEOF();
}

JavaScriptURLResourceRequestJobDelegate::
    JavaScriptURLResourceRequestJobDelegate(ResourceRequest* proxy)
    : m_orgProxy(proxy)
{
}

void JavaScriptURLResourceRequestJobDelegate::send(String* body,
                                                   bool allowCache)
{
    STARFISH_ASSERT(m_orgProxy->m_url->isJavascriptURL());
    // this area doesn't require lock.
    if (m_orgProxy->m_isSync) {
        worker(m_orgProxy, m_orgProxy->m_url->urlString());
    } else {
        size_t handle = m_orgProxy->starFish()->messageLoop()->addIdler(
            m_orgProxy->document()->browsingContext(),
            [](size_t handle, void* data, void* data1) {
                ResourceRequest* request = (ResourceRequest*)data;
                request->removeIdlerHandle(handle);
                AboutURLResourceRequestJobDelegate::worker(
                    (ResourceRequest*)data, (String*)data1);
            },
            m_orgProxy, m_orgProxy->m_url->urlString());
        m_orgProxy->pushIdlerHandle(handle);
    }
}

void JavaScriptURLResourceRequestJobDelegate::worker(ResourceRequest* res,
                                                     String* url)
{
    res->m_status = 200;
    res->changeReadyState(ResourceRequest::HEADERS_RECEIVED, true);
    res->changeReadyState(ResourceRequest::LOADING, true);
    res->handleResponseEOF();
}

UnknownURLResourceRequestJobDelegate::UnknownURLResourceRequestJobDelegate(
    ResourceRequest* proxy)
    : m_orgProxy(proxy)
{
}

void UnknownURLResourceRequestJobDelegate::send(String* body, bool allowCache)
{
    // this area doesn't require lock.
    if (m_orgProxy->m_isSync) {
        worker(m_orgProxy, m_orgProxy->m_url->urlString());
    } else {
        size_t handle = m_orgProxy->starFish()->messageLoop()->addIdler(
            m_orgProxy->document()->browsingContext(),
            [](size_t handle, void* data, void* data1) {
                ResourceRequest* request = (ResourceRequest*)data;
                request->removeIdlerHandle(handle);
                UnknownURLResourceRequestJobDelegate::worker(
                    (ResourceRequest*)data, (String*)data1);
            },
            m_orgProxy, m_orgProxy->m_url->urlString());
        m_orgProxy->pushIdlerHandle(handle);
    }
}

void UnknownURLResourceRequestJobDelegate::worker(ResourceRequest* res,
                                                  String* url)
{
    res->m_status = 404;
    res->changeReadyState(ResourceRequest::HEADERS_RECEIVED, true);
    res->changeReadyState(ResourceRequest::LOADING, true);
    res->handleResponseEOF();
}

BlobURLResourceRequestJobDelegate::BlobURLResourceRequestJobDelegate(
    ResourceRequest* proxy)
    : m_orgProxy(proxy)
{
}

void BlobURLResourceRequestJobDelegate::send(String* body, bool allowCache)
{
    STARFISH_ASSERT(m_orgProxy->m_url->isBlobURL());
    // this area doesn't require lock.
    // reading url does not require thread
    if (m_orgProxy->m_isSync) {
        worker(m_orgProxy, m_orgProxy->m_url->urlString());
    } else {
        size_t handle = m_orgProxy->starFish()->messageLoop()->addIdler(
            m_orgProxy->document()->browsingContext(),
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
    if (!WebView::stringToBlobURLString(url, store)) {
        res->handleError(ResourceRequest::IN_ERROR);
        return;
    }

    if (!res->document()->webView()->isValidBlobURL(store)) {
        res->handleError(ResourceRequest::IN_ERROR);
        return;
    }

    res->m_responseMimeType = ((Blob*)store.m_blob)->type();
    res->changeReadyState(ResourceRequest::HEADERS_RECEIVED, true);

    res->changeReadyState(ResourceRequest::LOADING, true);
    char* buf = (char*)((Blob*)store.m_blob)->data();
    res->m_response.assign(buf, &buf[((Blob*)store.m_blob)->size()]);

    res->handleResponseEOF();
}

String* decodeURL(String* src, size_t idx)
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
