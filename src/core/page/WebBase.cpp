/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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
#include "PlatformIntegrationData.h"
#include "core/page/WebBase.h"
#include "core/fileapi/Blob.h"
#include "core/modules/message_loop/Timer.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/threading/ThreadPool.h"
#include "core/extra/Console.h"
#include "core/util/RandomEngine.h"

namespace Starfish {

WebBase::WebBase(Starfish* starfish, const char* locale, const char* timezoneID,
                 String* customUserAgentString)
    : StarfishHoldable(starfish)
    , m_timezoneID(String::fromUTF8(timezoneID, strlen(timezoneID)))
    , m_customUserAgentString(customUserAgentString)
    , m_messageLoop(new MessageLoop())
    , m_timer(new Timer(this))
    , m_console(new Console(this))
    , m_webSecurityMode(LWE::WebSecurityMode::Enable)
    , m_useHttp2(false)
{
    STARFISH_ASSERT(starfish != nullptr && locale != nullptr &&
                    timezoneID != nullptr && customUserAgentString != nullptr);
    UErrorCode err = U_ZERO_ERROR;
    char buf[512];
    auto len = uloc_getName(locale, buf, sizeof(buf), &err);
    if (U_FAILURE(err)) {
        STARFISH_LOG_ERROR(
            "there is an error whild parsing locale %s. use default instead\n",
            locale);
        m_locale = uloc_getDefault();
    } else {
        buf[len] = 0;
        m_locale = buf;
    }

#ifndef STARFISH_THREAD_POOL_SIZE
#define STARFISH_THREAD_POOL_SIZE 6
#endif
    m_threadPool = new ThreadPool(STARFISH_THREAD_POOL_SIZE, m_messageLoop);
}

bool WebBase::stringToBlobURLString(String* url, BlobURLStore& store)
{
    size_t idx = url->lastIndexOf('/');
    if (idx == SIZE_MAX) {
        return false;
    }

    idx++;
    if (idx >= url->length()) {
        return false;
    }
    String* uuid = url->substring(idx, url->length() - idx);

    auto utf8Data = uuid->toUTF8NonGCString();
    const char* str = utf8Data.data();
    if (strlen(str) != 36) {
        return false;
    }

    unsigned int a0, a1, a2, a3, a4, a5, a6, a7;
    sscanf(str, "%04X%04X-%04X-%04X-%04X-%04X%04X%04X", &a0, &a1, &a2, &a3, &a4,
           &a5, &a6, &a7);

    union {
        struct {
            uint16_t a;
            uint16_t b;
        } tiny;
        uint32_t big;
    } spliter;

#ifdef STARFISH_64
    union {
        struct {
            uint16_t a;
            uint16_t b;
            uint16_t c;
            uint16_t d;
        } tiny;
        uint64_t big;
    } spliter64;
#endif

#ifdef STARFISH_64
    spliter.tiny.a = a0;
    spliter.tiny.b = a1;
    store.m_a = spliter.big;

    spliter.tiny.a = a2;
    spliter.tiny.b = a3;
    store.m_b = spliter.big;

    spliter64.tiny.a = a4;
    spliter64.tiny.b = a5;
    spliter64.tiny.c = a6;
    spliter64.tiny.d = a7;
    store.m_blob = (void*)spliter64.big;
#else
    spliter.tiny.a = a0;
    spliter.tiny.b = a1;
    store.m_a = spliter.big;

    spliter.tiny.a = a2;
    spliter.tiny.b = a3;
    store.m_b = spliter.big;

    spliter.tiny.a = a4;
    spliter.tiny.b = a5;
    store.m_c = spliter.big;

    spliter.tiny.a = a6;
    spliter.tiny.b = a7;
    store.m_blob = (void*)spliter.big;
#endif

    return true;
}

String* WebBase::blobURLStoreToString(BlobURLStore store, String* origin)
{
    UTF8StringDataNonGCStd url = "blob:";
    url += origin->toUTF8NonGCString();
    url += "/";

    union {
        struct {
            uint16_t a;
            uint16_t b;
        } tiny;
        uint32_t big;
    } spliter;

#ifdef STARFISH_64
    union {
        struct {
            uint16_t a;
            uint16_t b;
            uint16_t c;
            uint16_t d;
        } tiny;
        uint64_t big;
    } spliter64;
#endif

    char buf[32];
#ifdef STARFISH_64
    spliter.big = store.m_a;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.tiny.a);
    url += buf;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.tiny.b);
    url += buf;
    url += "-";

    spliter.big = store.m_b;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.tiny.a);
    url += buf;
    url += "-";
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.tiny.b);
    url += buf;
    url += "-";

    spliter64.big = (uint64_t)store.m_blob;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter64.tiny.a);
    url += buf;
    url += "-";

    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter64.tiny.b);
    url += buf;

    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter64.tiny.c);
    url += buf;

    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter64.tiny.d);
    url += buf;
#else
    spliter.big = store.m_a;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.tiny.a);
    url += buf;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.tiny.b);
    url += buf;
    url += "-";

    spliter.big = store.m_b;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.tiny.a);
    url += buf;
    url += "-";
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.tiny.b);
    url += buf;
    url += "-";

    spliter.big = store.m_c;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.tiny.a);
    url += buf;
    url += "-";

    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.tiny.b);
    url += buf;

    spliter.big = (uint32_t)store.m_blob;
    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.tiny.a);
    url += buf;

    snprintf(buf, sizeof(buf), "%04X", (unsigned)spliter.tiny.b);
    url += buf;
#endif
    return String::createASCIIString(url.data(), url.size());
}

BlobURLStore WebBase::addBlobInBlobURLStore(Blob* ptr)
{
#ifndef NDEBUG
    {
        BlobURLStore s;
        s.m_blob = ptr;
        STARFISH_ASSERT(m_urlBlobStore.find(s) == m_urlBlobStore.end());
    }
#endif
    BlobURLStore a;
    a.m_blob = ptr;

    std::uniform_int_distribution<uint32_t> distribution;
#ifdef STARFISH_32
    a.m_a = distribution(randomEngine());
    a.m_b = distribution(randomEngine());
    a.m_c = distribution(randomEngine());
#else
    a.m_a = distribution(randomEngine());
    a.m_b = distribution(randomEngine());
#endif

    m_urlBlobStore.insert(a);

    return a;
}

void WebBase::removeBlobFromBlobURLStore(Blob* ptr)
{
#ifndef NDEBUG
    {
        BlobURLStore s;
        s.m_blob = ptr;
        STARFISH_ASSERT(m_urlBlobStore.find(s) != m_urlBlobStore.end());
    }
#endif
    BlobURLStore s;
    s.m_blob = ptr;
    m_urlBlobStore.erase(s);
}

bool WebBase::isValidBlobURL(BlobURLStore ptr)
{
    auto iter = m_urlBlobStore.find(ptr);
#ifdef STARFISH_32
    return iter != m_urlBlobStore.end() && ptr.m_a == iter->m_a &&
           ptr.m_b == iter->m_b && ptr.m_c == iter->m_c;
#else
    return iter != m_urlBlobStore.end() && ptr.m_a == iter->m_a &&
           ptr.m_b == iter->m_b;
#endif
}

bool WebBase::isValidBlobURL(Blob* ptr)
{
    BlobURLStore s;
    s.m_blob = ptr;
    auto iter = m_urlBlobStore.find(s);
    return iter != m_urlBlobStore.end();
}

BlobURLStore WebBase::findBlobURL(Blob* ptr)
{
    BlobURLStore s;
    s.m_blob = ptr;
    auto iter = m_urlBlobStore.find(s);
    return *iter;
}

void WebBase::clearBlobURLStore()
{
    m_urlBlobStore.clear();
    GCUnorderedSet<BlobURLStore>().swap(m_urlBlobStore);
}

String* WebBase::userAgent()
{
    String* custom = customUserAgentString();
    if (custom->length()) {
        return custom;
    }
    return String::createASCIIString(USER_AGENT(STARFISH_NAME, VERSION));
}

void WebBase::registerPublicWebViewHandler(
    StarfishPubicWebViewHandlerKind handlerKind,
    std::function<void(void*)> handler)
{
    auto it = m_publicWebViewHandlers.find(handlerKind);
    if (it == m_publicWebViewHandlers.end()) {
        m_publicWebViewHandlers.insert(std::make_pair(handlerKind, handler));
    } else {
        it->second = handler;
    }
}

bool WebBase::containsPublicWebViewHandler(
    StarfishPubicWebViewHandlerKind handlerKind)
{
    auto it = m_publicWebViewHandlers.find(handlerKind);
    if (it != m_publicWebViewHandlers.end()) {
        return true;
    }

    return false;
}

void WebBase::callPublicWebViewHandler(
    StarfishPubicWebViewHandlerKind handlerKind, void* param)
{
    auto it = m_publicWebViewHandlers.find(handlerKind);
    if (it == m_publicWebViewHandlers.end()) {
        return;
    }

    struct Env : public gc {
        WebBase* webBase;
        StarfishPubicWebViewHandlerKind handlerKind;
        void* param;
    };
    Env* env = new Env();
    env->webBase = this;
    env->handlerKind = handlerKind;
    env->param = param;

    messageLoop()->addIdler(
        nullptr,
        [](size_t, void* env) {
            Env* e = (Env*)env;
            auto it = e->webBase->m_publicWebViewHandlers.find(e->handlerKind);
            if (it != e->webBase->m_publicWebViewHandlers.end()) {
                (it->second)(e->param);
            }
        },
        env);
}

std::mt19937& WebBase::randomEngine()
{
    return RandomEngine::instance().mt19937();
}

LWE::WebSecurityMode WebBase::getWebSecurityMode() const
{
    return m_webSecurityMode;
}

void WebBase::setWebSecurityMode(LWE::WebSecurityMode value)
{
    m_webSecurityMode = value;
}
} // namespace Starfish
