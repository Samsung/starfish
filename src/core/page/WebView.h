/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishWebView__
#define __StarFishWebView__

#include "binding/StarFishHoldable.h"
#include "browser/history/HistoryManager.h"

namespace StarFish {
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
}

namespace std {
template <>
struct hash<StarFish::BlobURLStore> {
    size_t operator()(StarFish::BlobURLStore const& x) const
    {
        return (size_t)x.m_blob;
    }
};

template <>
struct equal_to<StarFish::BlobURLStore> {
    bool operator()(StarFish::BlobURLStore const& a,
                    StarFish::BlobURLStore const& b) const
    {
        return a.m_blob == b.m_blob;
    }
};
}

namespace StarFish {

class Document;
class BrowsingContext;
class StorageNamespaceProvider;
class StorageNamespace;
class HistoryManager;
class ScriptEngineInstance;
class Blob;
class MediaSource;
class StackingContext;
class CanvasSurface;

class WebView : public StarFishHoldable, public gc {
    friend class BrowsingContext;
    friend class PlatformWindow;

public:
    static WebView* create(StarFish* starFish);

    BrowsingContext* mainBrowsingContext()
    {
        return m_mainBrowsingContext;
    }

    StorageNamespace* localStorageNamespace()
    {
        return m_localStorageNamespace;
    }

    StorageNamespace* sessionStorageNamespace()
    {
        return m_sessionStorageNamespace;
    }

    HistoryManager* historyManager()
    {
        return m_historyManager;
    }

    void navigate(ResourceURL* url, HistoryManager::Action type);
    ScriptEngineInstance* scriptEngineInstance()
    {
        return m_scriptEngineInstance;
    }

    static bool stringToBlobURLString(String* url, BlobURLStore& result);
    static String* blobURLStoreToString(BlobURLStore store, String* origin);

    BlobURLStore addBlobInBlobURLStore(Blob* ptr);
    void removeBlobFromBlobURLStore(Blob* ptr);
    bool isValidBlobURL(BlobURLStore ptr);
    bool isValidBlobURL(Blob* ptr);
    BlobURLStore findBlobURL(Blob* ptr);

    BlobURLStore addMediaSourceInBlobURLStore(MediaSource* ptr);
    void removeMediaSourceFromBlobURLStore(MediaSource* ptr);
    bool isValidMediaSourceBlobURL(BlobURLStore ptr);
    bool isValidMediaSourceBlobURL(MediaSource* ptr);
    BlobURLStore findMediaSourceBlobURL(MediaSource* ptr);
    void clearBlobURLStore();

private:
    WebView(StarFish* starFish);

    void initRenderingFlags();
    void rendering();
    void setNeedsRendering()
    {
        if (m_needsRendering) {
            return;
        }
        setNeedsRenderingSlowCase();
    }
    void setNeedsPainting()
    {
        if (!m_needsPainting) {
            m_needsPainting = true;
            setNeedsRendering();
        }
    }

    void setNeedsComposite()
    {
        if (!m_needsComposite) {
            m_needsComposite = true;
            setNeedsRendering();
        }
    }
    void renderingIfNeeds()
    {
        if (m_needsRendering) {
            rendering();
            m_needsRendering = false;
        }
    }

    void setNeedsRenderingSlowCase();
    void initStorage();

    BrowsingContext* m_mainBrowsingContext;

    ScriptEngineInstance* m_scriptEngineInstance;

    StorageNamespaceProvider* m_storageNamespaceProvider;

    StorageNamespace* m_localStorageNamespace;
    StorageNamespace* m_sessionStorageNamespace;

    HistoryManager* m_historyManager;
    unsigned int m_seed;

    GCUnorderedSet<BlobURLStore> m_urlBlobStore;
    GCUnorderedSet<BlobURLStore> m_urlMediaSourceBlobStore;

    uint64_t m_lastRenderingTime;
    bool m_inRendering;
    bool m_needsRendering;
    bool m_needsPainting;
    bool m_needsComposite;

    StackingContext* m_rootStackingContext;
    GCVector<CanvasSurface*> m_backStackingContextBufferUpWhileReCompsite;
};
}

#endif
