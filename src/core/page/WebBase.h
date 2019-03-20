/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishWebBase__
#define __StarfishWebBase__

#include "binding/StarfishHoldable.h"

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
    ShouldOverrideUrlLoading,
};
}

namespace std {
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
}

namespace Starfish {

class MessageLoop;
class Console;
class Inspector;
class Blob;
class WebView;

class WebBase : public StarfishHoldable, public gc {
public:
    virtual ~WebBase()
    {
    }
    virtual MessageLoop* messageLoop() const = 0;
    virtual Console* console() const = 0;

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
        return reinterpret_cast<WebView*>(this);
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
    BlobURLStore findBlobURL(Blob* ptr);
    void clearBlobURLStore();

protected:
    WebBase(Starfish* starfish);

    unsigned int m_seed;
    GCUnorderedSet<BlobURLStore> m_urlBlobStore;
};
}

#endif
