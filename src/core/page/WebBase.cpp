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

WebBase::WebBase(Starfish* starfish, MessageLoop* messageLoop, Timer* timer,
                 const char* locale, const char* timezoneID,
                 String* customUserAgentString)
    : StarfishHoldable(starfish)
    , m_timezoneID(String::fromUTF8(timezoneID, strlen(timezoneID)))
    , m_customUserAgentString(customUserAgentString)
    , m_messageLoop(messageLoop)
    , m_timer(timer)
    , m_console(new Console(this))
    , m_webSecurityMode(LWE::WebSecurityMode::Enable)
    , m_useHttp2(false)
    , m_videoOverlayEnabled(false)
{
    STARFISH_ASSERT(starfish != nullptr && locale != nullptr &&
                    timezoneID != nullptr && customUserAgentString != nullptr);
    UErrorCode err = U_ZERO_ERROR;
    char buf[512];
    auto len = uloc_getName(locale, buf, sizeof(buf), &err);
    if (U_FAILURE(err) || len <= 0) {
        // uloc_getName() reports success for an empty locale, so an embedder
        // without a system language (e.g. dali-extension web-engine-lwe when
        // vconf is unreachable) would surface an empty navigator.language.
        // HTML NavigatorLanguage must expose a well-formed language tag, so
        // fall back to the ICU default locale.
        STARFISH_LOG_ERROR(
            "there is an error while parsing locale %s. use default instead",
            locale);
        const char* defaultLocale = uloc_getDefault();
        if (defaultLocale != nullptr) {
            err = U_ZERO_ERROR;
            len = uloc_getName(defaultLocale, buf, sizeof(buf), &err);
        } else {
            len = 0;
        }
        if (U_FAILURE(err) || len <= 0) {
            // The default locale itself can be empty; still keep a
            // well-formed language tag.
            buf[0] = 'e';
            buf[1] = 'n';
            len = 2;
        }
    }
    buf[len] = 0;
    m_locale = buf;

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
    // Exactly 8-4-4-4-12 hex digits; sscanf alone would also take signs,
    // spaces and short groups.
    if (strlen(str) != 36) {
        return false;
    }
    for (size_t i = 0; i < 36; i++) {
        bool isDash = i == 8 || i == 13 || i == 18 || i == 23;
        if (isDash ? str[i] != '-' : !isxdigit((unsigned char)str[i])) {
            return false;
        }
    }

    unsigned int a, b0, b1, c0, c1, d;
    if (sscanf(str, "%8X-%4X-%4X-%4X-%4X%8X", &a, &b0, &b1, &c0, &c1, &d) !=
        6) {
        return false;
    }
    store.m_blob = nullptr;
    store.m_a = a;
    store.m_b = (b0 << 16) | b1;
    store.m_c = (c0 << 16) | c1;
    store.m_d = d;
    return true;
}

String* WebBase::blobURLStoreToString(BlobURLStore store, String* origin)
{
    UTF8StringDataNonGCStd url = "blob:";
    url += origin->toUTF8NonGCString();
    url += "/";

    char buf[40];
    snprintf(buf, sizeof(buf), "%08x-%04x-%04x-%04x-%04x%08x",
             (unsigned)store.m_a, (unsigned)(store.m_b >> 16),
             (unsigned)(store.m_b & 0xFFFF), (unsigned)(store.m_c >> 16),
             (unsigned)(store.m_c & 0xFFFF), (unsigned)store.m_d);
    url += buf;
    return String::createASCIIString(url.data(), url.size());
}

BlobURLStore WebBase::createBlobURLStore(void* ptr)
{
    BlobURLStore a;
    a.m_blob = ptr;
    std::uniform_int_distribution<uint32_t> distribution;
    a.m_a = distribution(randomEngine());
    // A version 4 UUID (RFC 4122 4.4): fixed version and variant bits.
    a.m_b = (distribution(randomEngine()) & 0xFFFF0FFF) | 0x00004000;
    a.m_c = (distribution(randomEngine()) & 0x3FFFFFFF) | 0x80000000;
    a.m_d = distribution(randomEngine());
    return a;
}

GCUnorderedSet<BlobURLStore>::iterator WebBase::findBlobURLStoreByPointer(
    GCUnorderedSet<BlobURLStore>& set, void* ptr)
{
    // shortcut: linear scan, entries are keyed by the random UUID; add a
    // pointer index if pages start holding many blob URLs at once.
    auto iter = set.begin();
    while (iter != set.end() && iter->m_blob != ptr) {
        ++iter;
    }
    return iter;
}

BlobURLStore WebBase::addBlobInBlobURLStore(Blob* ptr)
{
    STARFISH_ASSERT(findBlobURLStoreByPointer(m_urlBlobStore, ptr) ==
                    m_urlBlobStore.end());
    BlobURLStore a = createBlobURLStore(ptr);
    m_urlBlobStore.insert(a);
    return a;
}

void WebBase::removeBlobFromBlobURLStore(Blob* ptr)
{
    auto iter = findBlobURLStoreByPointer(m_urlBlobStore, ptr);
    STARFISH_ASSERT(iter != m_urlBlobStore.end());
    if (iter != m_urlBlobStore.end()) {
        m_urlBlobStore.erase(iter);
    }
}

bool WebBase::isValidBlobURL(BlobURLStore& store)
{
    auto iter = m_urlBlobStore.find(store);
    if (iter == m_urlBlobStore.end()) {
        return false;
    }
    store.m_blob = iter->m_blob;
    return true;
}

bool WebBase::isValidBlobURL(Blob* ptr)
{
    return findBlobURLStoreByPointer(m_urlBlobStore, ptr) !=
           m_urlBlobStore.end();
}

Optional<BlobURLStore> WebBase::findBlobURL(Blob* ptr)
{
    auto iter = findBlobURLStoreByPointer(m_urlBlobStore, ptr);
    if (iter != m_urlBlobStore.end()) {
        return *iter;
    } else {
        return nullptr;
    }
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
    StarfishPubicWebViewHandlerKind handlerKind, void* param, bool sync)
{
    auto it = m_publicWebViewHandlers.find(handlerKind);
    if (it == m_publicWebViewHandlers.end()) {
        return;
    }

    if (sync) {
        (it->second)(param);
    } else {
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
                auto it =
                    e->webBase->m_publicWebViewHandlers.find(e->handlerKind);
                if (it != e->webBase->m_publicWebViewHandlers.end()) {
                    (it->second)(e->param);
                }
            },
            env);
    }
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
