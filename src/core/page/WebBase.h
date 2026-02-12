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

#ifndef __StarfishWebBase__
#define __StarfishWebBase__

#include "binding/StarfishHoldable.h"

namespace LWE {
enum class WebSecurityMode;
}

namespace Starfish {
struct BlobURLStore {
#ifdef STARFISH_32
    void* m_blob;
    uint32_t m_a;
    uint32_t m_b;
    uint32_t m_c;
#else
    void* m_blob;
    uint32_t m_a;
    uint32_t m_b;
#endif
};

enum StarfishPubicWebViewHandlerKind {
    OnPageStarted,
    OnPageLoaded,
    OnPageParsed,
    OnLoadResource,
    OnReceivedError,
    OnProgressChanged,
    OnDownloadStart,
    OnIdle,
    ShouldOverrideUrlLoading,
    DebuggerShouldInit,
    DebuggerShouldContinueWaiting,
};
} // namespace Starfish

namespace std {
template <>
struct hash<Starfish::StarfishPubicWebViewHandlerKind> {
    size_t operator()(Starfish::StarfishPubicWebViewHandlerKind const& x) const
    {
        return std::hash<uint32_t>()((uint32_t)x);
    }
};

template <>
struct equal_to<Starfish::StarfishPubicWebViewHandlerKind> {
    bool operator()(Starfish::StarfishPubicWebViewHandlerKind const& a,
                    Starfish::StarfishPubicWebViewHandlerKind const& b) const
    {
        return a == b;
    }
};

template <>
struct hash<Starfish::BlobURLStore> {
    size_t operator()(Starfish::BlobURLStore const& x) const
    {
        return (size_t)x.m_blob;
    }
};

template <>
struct equal_to<Starfish::BlobURLStore> {
    bool operator()(Starfish::BlobURLStore const& a,
                    Starfish::BlobURLStore const& b) const
    {
        return a.m_blob == b.m_blob;
    }
};
} // namespace std

namespace Starfish {

class MessageLoop;
class Console;
class Inspector;
class Blob;
class WebView;
class WebWorker;
class ThreadPool;
class Timer;

class WebBase : public StarfishHoldable, public gc {
public:
    virtual ~WebBase()
    {
    }

    virtual bool isWebView() const
    {
        return false;
    }

    virtual bool isWebWorker() const
    {
        return false;
    }

    WebView* asWebView()
    {
        STARFISH_ASSERT(isWebView());
        return reinterpret_cast<WebView*>(this);
    }

    WebWorker* asWebWorker()
    {
        STARFISH_ASSERT(isWebWorker());
        return reinterpret_cast<WebWorker*>(this);
    }

#if defined(STARFISH_ENABLE_INSPECTOR)
    virtual Inspector* inspector() const = 0;
#endif

    virtual void setNeedsRendering() = 0;
    virtual uint64_t lastRenderingTick() = 0;

    static bool stringToBlobURLString(String* url, BlobURLStore& result);
    static String* blobURLStoreToString(BlobURLStore store, String* origin);

    BlobURLStore addBlobInBlobURLStore(Blob* ptr);
    void removeBlobFromBlobURLStore(Blob* ptr);
    bool isValidBlobURL(BlobURLStore ptr);
    bool isValidBlobURL(Blob* ptr);
    Optional<BlobURLStore> findBlobURL(Blob* ptr);
    void clearBlobURLStore();

    Console* console() const
    {
        return m_console;
    }

    const std::string& locale()
    {
        return m_locale;
    }

    String* timezoneID()
    {
        return m_timezoneID;
    }

    String* customUserAgentString()
    {
        return m_customUserAgentString;
    }

    void setCustomUserAgentString(String* customUserAgentString)
    {
        m_customUserAgentString = customUserAgentString;
    }

    void setProxyURL(const std::string& url)
    {
        m_proxyURL = url;
    }

    const std::string& proxyURL() const
    {
        return m_proxyURL;
    }

    String* userAgent();

    void registerPublicWebViewHandler(
        StarfishPubicWebViewHandlerKind handlerKind,
        std::function<void(void*)> handler);
    bool containsPublicWebViewHandler(
        StarfishPubicWebViewHandlerKind handlerKind);
    void callPublicWebViewHandler(StarfishPubicWebViewHandlerKind handlerKind,
                                  void* data, bool sync = false);

    void registerCustomFileResourceRequestCallbacks(
        std::function<const char*(const char* path)> resolveFilePathCallback,
        std::function<void*(const char* path)> fileOpenCallback,
        std::function<size_t(uint8_t* destBuffer, size_t size, void* handle)>
            fileReadCallback,
        std::function<long int(void* handle)> fileLengthCallback,
        std::function<void(void* handle)> fileCloseCallback)
    {
        m_resolveFilePathCallback = resolveFilePathCallback;
        m_fileOpenCallback = fileOpenCallback;
        m_fileReadCallback = fileReadCallback;
        m_fileLengthCallback = fileLengthCallback;
        m_fileCloseCallback = fileCloseCallback;
    }

    MessageLoop* messageLoop()
    {
        return m_messageLoop;
    }

    Timer* timer()
    {
        return m_timer;
    }

    ThreadPool* threadPool()
    {
        return m_threadPool;
    }

    LWE::WebSecurityMode getWebSecurityMode() const;
    void setWebSecurityMode(LWE::WebSecurityMode value);

    void setUseHttp2(bool b)
    {
        m_useHttp2 = b;
    }

    bool useHttp2()
    {
        return m_useHttp2;
    }

    std::mt19937& randomEngine();

protected:
    WebBase(Starfish* starfish, MessageLoop* messageLoop, Timer* timer,
            const char* locale, const char* timezoneID,
            String* customUserAgentString);

    GCUnorderedSet<BlobURLStore> m_urlBlobStore;

    // options
    std::string m_locale;
    String* m_timezoneID;
    String* m_customUserAgentString;
    std::string m_proxyURL;
    std::unordered_map<StarfishPubicWebViewHandlerKind,
                       std::function<void(void*)>>
        m_publicWebViewHandlers;

    MessageLoop* m_messageLoop;
    Timer* m_timer;
    ThreadPool* m_threadPool;

    Console* m_console;

    LWE::WebSecurityMode m_webSecurityMode;
    bool m_useHttp2;

public:
    // function sets for implementing custom file IO for resource request
    std::function<const char*(const char* path)> m_resolveFilePathCallback;
    std::function<void*(const char* path)> m_fileOpenCallback;
    std::function<size_t(uint8_t* destBuffer, size_t size, void* handle)>
        m_fileReadCallback;
    std::function<long int(void* handle)> m_fileLengthCallback;
    std::function<void(void* handle)> m_fileCloseCallback;
    // <----
};
} // namespace Starfish

#endif
