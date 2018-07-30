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

#ifndef __StarFishWebView__
#define __StarFishWebView__

#include "binding/StarFishHoldable.h"
#include "browser/history/HistoryManager.h"
#include "core/page/RenderResult.h"

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
class AnimationExecutor;

class WebView : public StarFishHoldable, public gc {
    friend class BrowsingContext;
    friend class StackingContext;
    friend class PlatformWindow;
    friend class WindowImplEFL;
    friend class AnimationExecutor;
    friend class ResourceLoader;

public:
    static WebView* create(StarFish* starFish);
    void close();

    BrowsingContext* mainBrowsingContext()
    {
        return m_topLevelBrowsingContext;
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

    void navigate(ResourceURL* url, HistoryManager::Action type,
                  ResourceURL* referrerURL);
    ScriptEngineInstance* scriptEngineInstance()
    {
        return m_scriptEngineInstance;
    }

    void createScriptEngineInstance()
    {
        if (!m_scriptEngineInstance) {
            m_scriptEngineInstance = new ScriptEngineInstance(m_starFish);
        }
    }

    void removeScriptEngineInstance()
    {
        if (m_scriptEngineInstance) {
            m_scriptEngineInstance->dispose();

            delete m_scriptEngineInstance;
            m_scriptEngineInstance = nullptr;
        }
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

    void layoutIfNeeds();
    void clearStackingContext();
    StackingContext* rootStackingContext()
    {
        return m_rootStackingContext;
    }

    bool didCompositeBefore()
    {
        return m_didCompositeBefore;
    }

    bool hasFocus();
    Node* focusedNode();
    BrowsingContext* focusedBrowsingContext();
    void blur();

    void setNeedsComputeStackingContextProperties()
    {
        if (!m_needsComputeStackingContextProperties) {
            m_needsComputeStackingContextProperties = true;
            setNeedsRendering();
        }
    }

    void setNeedsEstablishesStackingContext()
    {
        if (!m_needsEstablishesStackingContext) {
            m_needsEstablishesStackingContext = true;
            setNeedsRendering();
        }
    }

    void markNeedsPaintingConsiderInRendering()
    {
        if (!m_needsPainting) {
            m_needsPainting = true;
        }
        if (!m_inRendering) {
            setNeedsRendering();
        }
    }

    void markNeedsCompositeConsiderInRendering()
    {
        if (!m_needsComposite) {
            m_needsComposite = true;
        }
        if (!m_inRendering) {
            setNeedsRendering();
        }
    }

    bool inRendering()
    {
        return m_inRendering;
    }

    void onIdle();

    typedef bool (*DidLayoutCallback)(void*); // return true cause relayout
    void addDidLayoutCallback(DidLayoutCallback cb, void* data);

    typedef void (*DidRenderingCallback)(void*);
    void addDidRenderingCallback(BrowsingContext* ctx, DidRenderingCallback cb,
                                 void* data);

    bool hasActiveAnimationExecutor()
    {
        return m_activeAnimationExecutor.size();
    }

    bool needsRendering()
    {
        return m_needsRendering;
    }

    void addPaintingDirtyArea(const LayoutRect& rt)
    {
        m_paintingDirtyRect.unite(rt);
    }

    PrevDrawnStackingContextInfoMap& prevDrawnStackingContextInfo()
    {
        return m_prevDrawnStackingContextInfo;
    }

private:
    WebView(StarFish* starFish);

    void initRenderingFlags();

    RenderResult rendering(
        bool force = false); // returns did painting | did compositing
    void setNeedsRendering();
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

    void initStorage();

    BrowsingContext* m_topLevelBrowsingContext;

    ScriptEngineInstance* m_scriptEngineInstance;

    StorageNamespaceProvider* m_storageNamespaceProvider;

    StorageNamespace* m_localStorageNamespace;
    StorageNamespace* m_sessionStorageNamespace;

    HistoryManager* m_historyManager;
    unsigned int m_seed;

    GCUnorderedSet<BlobURLStore> m_urlBlobStore;
    GCUnorderedSet<BlobURLStore> m_urlMediaSourceBlobStore;

    PrevDrawnStackingContextInfoMap m_prevDrawnStackingContextInfo;

    uint64_t m_lastRenderingTime;
    uint64_t m_navigateStartingTime;
    uint32_t m_currentActiveAnimatorCount;
    bool m_inRendering;
    bool m_needsRendering;
    bool m_needsEstablishesStackingContext;
    bool m_needsComputeStackingContextProperties;
    bool m_needsPainting;
    bool m_needsComposite;
    bool m_didCompositeBefore; // last state of enabling composite
    LayoutRect m_paintingDirtyRect;

    GCVector<BrowsingContext*> m_browsingContextsNeedsLayout;
    StackingContext* m_rootStackingContext;
    GCVector<std::pair<DidLayoutCallback, void*>> m_didLayoutCallbacks;
    GCVector<BrowsingContext*> m_browsingContextsHasPendingAnimation;
    GCVector<AnimationExecutor*> m_activeAnimationExecutor;
    size_t m_activeAnimatorForAnimationExecutor;
    GCVector<std::tuple<BrowsingContext*, DidRenderingCallback, void*>>
        m_didRenderingCallbacks;
};
}

#endif
