/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"
#include <curl/curl.h>

#include "Starfish.h"
#include "core/dom/ExecutionContext.h"
#include "core/fileapi/Blob.h"
#include "platform/file/File.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/modules/resource_request/ResourceRequestJob.h"
#include "core/modules/resource_request/NetworkURLResourceRequestJobDelegate.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/util/URL.h"
#include "core/page/WebBase.h"

namespace Starfish {

// [uriReserved] ; / ? : @ & = + $ ,
inline static bool isURIReservedOrSharp(char16_t ch)
{
    return ch == ';' || ch == '/' || ch == '?' || ch == ':' || ch == '@' ||
           ch == '&' || ch == '=' || ch == '+' || ch == '$' || ch == ',' ||
           ch == '#';
}

inline static bool isDecimalDigit(char16_t ch)
{
    return ('0' <= ch && ch <= '9');
}

inline static bool isHexadecimalDigit(char16_t ch)
{
    return isDecimalDigit(ch) || ('A' <= ch && ch <= 'F') ||
           ('a' <= ch && ch <= 'f');
}

inline static bool twocharToHexaDecimal(char16_t ch1, char16_t ch2,
                                        unsigned char* res)
{
    if (!isHexadecimalDigit(ch1) || !isHexadecimalDigit(ch2))
        return false;
    *res = (((ch1 & 0x10) ? (ch1 & 0xf) : ((ch1 & 0xf) + 9)) << 4) |
           ((ch2 & 0x10) ? (ch2 & 0xf) : ((ch2 & 0xf) + 9));
    return true;
}

inline static bool codeUnitToHexaDecimal(const UTF16StringDataNonGCStd& str,
                                         size_t start, unsigned char* res)
{
    STARFISH_ASSERT(str.length() > start + 2);
    if (str[start] != '%')
        return false;
    bool succeed = twocharToHexaDecimal(str[start + 1], str[start + 2], res);
    // The two most significant bits of res should be 10.
    return succeed && (*res & 0xC0) == 0x80;
}

static Nullable<String*> decodeURI(String* uriString, bool noComponent = true)
{
    UTF16StringDataNonGCStd unescaped;
    auto u16String = uriString->toUTF16NonGCString();
    size_t strLen = u16String.length();

    for (size_t i = 0; i < strLen; i++) {
        char16_t t = u16String[i];
        if (t != '%') {
            unescaped += t;
        } else {
            size_t start = i;
            if (i + 2 >= strLen) {
                // error
                return Nullable<String*>();
            }
            char16_t next = u16String[i + 1];
            char16_t nextnext = u16String[i + 2];

            // char to hex
            unsigned char b = 0;
            if (!twocharToHexaDecimal(next, nextnext, &b)) {
                // error
                return Nullable<String*>();
            }
            i += 2;

            // most significant bit in b is 0
            if (!(b & 0x80)) {
                // let C be the character with code unit value B.
                // if C is not in reservedSet, then let S be the String
                // containing only the character C.
                // else, C is in reservedSet, Let S be the substring of string
                // from position start to position k included.
                const char16_t c = b & 0x7f;
                if (noComponent && isURIReservedOrSharp(c)) {
                    unescaped += u16String[start];
                    unescaped += u16String[start + 1];
                    unescaped += u16String[start + 2];
                } else {
                    unescaped += c;
                }
            } else { // most significant bit in b is 1
                unsigned char b_tmp = b;
                int n = 1;
                while (n < 5) {
                    b_tmp <<= 1;
                    if ((b_tmp & 0x80) == 0) {
                        break;
                    }
                    n++;
                }
                if (n == 1 || n == 5 || (i + (3 * (n - 1)) >= strLen)) {
                    // error
                    return Nullable<String*>();
                }
                unsigned char octets[4];
                octets[0] = b;

                int j = 1;
                while (j < n) {
                    if (!codeUnitToHexaDecimal(u16String, ++i,
                                               &b)) { // "%XY" type
                        // error
                        return Nullable<String*>();
                    }
                    i += 2;
                    octets[j] = b;
                    j++;
                }
                STARFISH_ASSERT(n == 2 || n == 3 || n == 4);
                unsigned int v = 0;
                if (n == 2) {
                    v = (octets[0] & 0x1F) << 6 | (octets[1] & 0x3F);
                    if ((octets[0] == 0xC0) || (octets[0] == 0xC1)) {
                        // error
                        return Nullable<String*>();
                    }
                } else if (n == 3) {
                    v = (octets[0] & 0x0F) << 12 | (octets[1] & 0x3F) << 6 |
                        (octets[2] & 0x3F);
                    if ((0xD800 <= v && v <= 0xDFFF) ||
                        ((octets[0] == 0xE0) &&
                         ((octets[1] < 0xA0) || (octets[1] > 0xBF)))) {
                        // error
                        return Nullable<String*>();
                    }
                } else if (n == 4) {
                    v = (octets[0] & 0x07) << 18 | (octets[1] & 0x3F) << 12 |
                        (octets[2] & 0x3F) << 6 | (octets[3] & 0x3F);
                    if ((octets[0] == 0xF0) &&
                        ((octets[1] < 0x90) || (octets[1] > 0xBF))) {
                        // error
                        return Nullable<String*>();
                    }
                }
                if (v >= 0x10000) {
                    const char16_t l = (((v - 0x10000) & 0x3ff) + 0xdc00);
                    const char16_t h =
                        ((((v - 0x10000) >> 10) & 0x3ff) + 0xd800);
                    unescaped += h;
                    unescaped += l;
                } else {
                    const char16_t l = v & 0xFFFF;
                    unescaped += l;
                }
            }
        }
    }
    return Nullable<String*>(
        String::fromUTF16(unescaped.data(), unescaped.size()));
}

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
    } else if (proxy->url()->isHTTPFamilyURL()) {
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
    STARFISH_ASSERT(m_orgProxy->url()->isFileURL());
    // this area doesn't require lock.
    // reading file does not require thread
    String* path = m_orgProxy->url()->getUrlPathString();
    auto decodedPath = decodeURI(path);
    if (decodedPath.hasValue()) {
        path = decodedPath.getValue();
    } else {
        auto s = m_orgProxy->url()->urlString()->toUTF8NonGCString();
        STARFISH_LOG_INFO("failed to open %s\n", s.data());
        m_orgProxy->m_responseData->m_status = 0;
        m_orgProxy->handleError(ProgressState::InError,
                                RequestErrorType::BadURLError);
        return;
    }
#if defined(OS_WINDOWS)
    String* filePath = path->substring(8, path->length() - 8);
#else
    String* filePath = path->substring(7, path->length() - 7);
#endif

    if (m_orgProxy->isSync()) {
        worker(m_orgProxy, filePath);
    } else {
        size_t handle = m_orgProxy->webBase()->messageLoop()->addIdler(
            m_orgProxy->globalScope(),
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

void FileURLResourceRequestJobDelegate::worker(ResourceRequest* request,
                                               String* filePath)
{
    std::string u8Path = filePath->toUTF8NonGCString();
    if (request->webBase()->m_resolveFilePathCallback) {
        // custom I/O path
        u8Path = request->webBase()->m_resolveFilePathCallback(u8Path.data());

        auto handle = request->webBase()->m_fileOpenCallback(u8Path.data());
        if (!handle) {
            auto s = request->url()->urlString()->toUTF8NonGCString();
            STARFISH_LOG_INFO("failed to open %s\n", s.data());
            request->m_responseData->m_status = 0;
            request->handleError(ProgressState::InError,
                                 RequestErrorType::FileError);
        }

        request->m_responseData->m_status = 200;
        request->changeReadyState(ReadyState::HeadersReceived, true);
        request->changeReadyState(ReadyState::Loading, true);
        size_t responseLength =
            request->webBase()->m_fileLengthCallback(handle);
        request->response().resize(responseLength);
        request->webBase()->m_fileReadCallback(
            (uint8_t*)request->response().data(), request->response().size(),
            handle);
        request->webBase()->m_fileCloseCallback(handle);
        request->handleResponseEOF();
        return;
    }
    auto fio = File::open(u8Path, File::Read);
    if (fio) {
        request->m_responseData->m_status = 200;
        request->changeReadyState(ReadyState::HeadersReceived, true);
        request->changeReadyState(ReadyState::Loading, true);
        size_t responseLength = fio->size();
        request->response().resize(responseLength);
        fio->read(request->response().data(), sizeof(const char),
                  responseLength);
        fio.reset();
        request->handleResponseEOF();
    } else {
        auto s = request->url()->urlString()->toUTF8NonGCString();
        STARFISH_LOG_INFO("failed to open %s\n", s.data());
        request->m_responseData->m_status = 0;
        request->handleError(ProgressState::InError,
                             RequestErrorType::FileError);
    }
}

DataURLResourceRequestJobDelegate::DataURLResourceRequestJobDelegate(
    ResourceRequest* proxy)
    : m_orgProxy(proxy)
{
}

void DataURLResourceRequestJobDelegate::send(String* body, bool allowCache)
{
    STARFISH_ASSERT(m_orgProxy->url()->isDataURL());
    // this area doesn't require lock.
    // reading url does not require thread
    if (m_orgProxy->isSync()) {
        worker(m_orgProxy, m_orgProxy->url()->urlString());
    } else {
        size_t handle = m_orgProxy->webBase()->messageLoop()->addIdler(
            m_orgProxy->globalScope(),
            [](size_t handle, void* data, void* data1) {
                ResourceRequest* request = (ResourceRequest*)data;
                request->removeIdlerHandle(handle);
                DataURLResourceRequestJobDelegate::worker(
                    (ResourceRequest*)data, (String*)data1);
            },
            m_orgProxy, m_orgProxy->url()->urlString());
        m_orgProxy->pushIdlerHandle(handle);
    }
}

void DataURLResourceRequestJobDelegate::worker(ResourceRequest* request,
                                               String* url)
{
    request->m_responseData->m_status = 200;

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
            request->m_containsBase64Content = true;
        }
    }

    request->m_responseData->m_mimeType = mimeType;
    request->changeReadyState(ReadyState::HeadersReceived, true);

    request->changeReadyState(ReadyState::Loading, true);

    // TODO filter url string correctly according RFC 3986
    auto decodedURL =
        decodeURI(url->substring(idx + 1, url->length() - idx - 1), false);
    if (!decodedURL.hasValue()) {
        auto s = request->url()->urlString()->toUTF8NonGCString();
        request->m_responseData->m_status = 0;
        request->handleError(ProgressState::InError,
                             RequestErrorType::BadURLError);
        return;
    }
    UTF8StringDataNonGCStd utf8Data =
        decodedURL.getValue()->toUTF8NonGCString();

    size_t len = utf8Data.length();
    for (size_t i = 0; i < len; i++) {
        request->response().push_back(utf8Data[i]);
    }

    request->handleResponseEOF();
}

AboutURLResourceRequestJobDelegate::AboutURLResourceRequestJobDelegate(
    ResourceRequest* proxy)
    : m_orgProxy(proxy)
{
}

void AboutURLResourceRequestJobDelegate::send(String* body, bool allowCache)
{
    STARFISH_ASSERT(m_orgProxy->url()->isAboutURL());
    // this area doesn't require lock.
    if (m_orgProxy->isSync()) {
        worker(m_orgProxy, m_orgProxy->url()->urlString());
    } else {
        size_t handle = m_orgProxy->webBase()->messageLoop()->addIdler(
            m_orgProxy->globalScope(),
            [](size_t handle, void* data, void* data1) {
                ResourceRequest* request = (ResourceRequest*)data;
                request->removeIdlerHandle(handle);
                AboutURLResourceRequestJobDelegate::worker(
                    (ResourceRequest*)data, (String*)data1);
            },
            m_orgProxy, m_orgProxy->url()->urlString());
        m_orgProxy->pushIdlerHandle(handle);
    }
}

void AboutURLResourceRequestJobDelegate::worker(ResourceRequest* request,
                                                String* url)
{
    if (request->url()->pathname()->equalsIgnoreCase("blank")) {
        request->m_responseData->m_status = 200;
    } else {
        request->m_responseData->m_status = 404;
    }

    request->changeReadyState(ReadyState::HeadersReceived, true);
    request->changeReadyState(ReadyState::Loading, true);
    request->handleResponseEOF();
}

JavaScriptURLResourceRequestJobDelegate::
    JavaScriptURLResourceRequestJobDelegate(ResourceRequest* proxy)
    : m_orgProxy(proxy)
{
}

void JavaScriptURLResourceRequestJobDelegate::send(String* body,
                                                   bool allowCache)
{
    STARFISH_ASSERT(m_orgProxy->url()->isJavascriptURL());
    // this area doesn't require lock.
    if (m_orgProxy->isSync()) {
        worker(m_orgProxy, m_orgProxy->url()->urlString());
    } else {
        size_t handle = m_orgProxy->webBase()->messageLoop()->addIdler(
            m_orgProxy->globalScope(),
            [](size_t handle, void* data, void* data1) {
                ResourceRequest* request = (ResourceRequest*)data;
                request->removeIdlerHandle(handle);
                AboutURLResourceRequestJobDelegate::worker(
                    (ResourceRequest*)data, (String*)data1);
            },
            m_orgProxy, m_orgProxy->url()->urlString());
        m_orgProxy->pushIdlerHandle(handle);
    }
}

void JavaScriptURLResourceRequestJobDelegate::worker(ResourceRequest* request,
                                                     String* url)
{
    request->m_responseData->m_status = 200;
    request->changeReadyState(ReadyState::HeadersReceived, true);
    request->changeReadyState(ReadyState::Loading, true);
    request->handleResponseEOF();
}

UnknownURLResourceRequestJobDelegate::UnknownURLResourceRequestJobDelegate(
    ResourceRequest* proxy)
    : m_orgProxy(proxy)
{
}

void UnknownURLResourceRequestJobDelegate::send(String* body, bool allowCache)
{
    // this area doesn't require lock.
    if (m_orgProxy->isSync()) {
        worker(m_orgProxy, m_orgProxy->url()->urlString());
    } else {
        size_t handle = m_orgProxy->webBase()->messageLoop()->addIdler(
            m_orgProxy->globalScope(),
            [](size_t handle, void* data, void* data1) {
                ResourceRequest* request = (ResourceRequest*)data;
                request->removeIdlerHandle(handle);
                UnknownURLResourceRequestJobDelegate::worker(
                    (ResourceRequest*)data, (String*)data1);
            },
            m_orgProxy, m_orgProxy->url()->urlString());
        m_orgProxy->pushIdlerHandle(handle);
    }
}

void UnknownURLResourceRequestJobDelegate::worker(ResourceRequest* request,
                                                  String* url)
{
    request->m_responseData->m_status = 404;
    request->changeReadyState(ReadyState::HeadersReceived, true);
    request->changeReadyState(ReadyState::Loading, true);
    request->handleResponseEOF();
}

BlobURLResourceRequestJobDelegate::BlobURLResourceRequestJobDelegate(
    ResourceRequest* proxy)
    : m_orgProxy(proxy)
{
}

void BlobURLResourceRequestJobDelegate::send(String* body, bool allowCache)
{
    STARFISH_ASSERT(m_orgProxy->url()->isBlobURL());
    // this area doesn't require lock.
    // reading url does not require thread
    if (m_orgProxy->isSync()) {
        worker(m_orgProxy, m_orgProxy->url()->urlString());
    } else {
        size_t handle = m_orgProxy->webBase()->messageLoop()->addIdler(
            m_orgProxy->globalScope(),
            [](size_t handle, void* data, void* data1) {
                ResourceRequest* request = (ResourceRequest*)data;
                request->removeIdlerHandle(handle);
                BlobURLResourceRequestJobDelegate::worker(
                    (ResourceRequest*)data, (String*)data1);
            },
            m_orgProxy, m_orgProxy->url()->urlString());
        m_orgProxy->pushIdlerHandle(handle);
    }
}

void BlobURLResourceRequestJobDelegate::worker(ResourceRequest* request,
                                               String* url)
{
    request->m_responseData->m_status = 200;

    BlobURLStore store;
    if (!WebBase::stringToBlobURLString(url, store)) {
        request->handleError(ProgressState::InError,
                             RequestErrorType::BadURLError);
        return;
    }

    if (!request->executionContext()->webBase()->isValidBlobURL(store)) {
        request->handleError(ProgressState::InError,
                             RequestErrorType::BadURLError);
        return;
    }

    request->m_responseData->m_mimeType = ((Blob*)store.m_blob)->type();
    request->changeReadyState(ReadyState::HeadersReceived, true);

    request->changeReadyState(ReadyState::Loading, true);
    char* buf = (char*)((Blob*)store.m_blob)->data();
    request->response().assign(buf, &buf[((Blob*)store.m_blob)->size()]);

    request->handleResponseEOF();
}

} // namespace Starfish
