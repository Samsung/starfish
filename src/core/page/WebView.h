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

#ifndef __StarfishWebView__
#define __StarfishWebView__

#include "binding/ScriptWrappable.h"
#include "core/page/WebBase.h"
#include "core/page/RenderResult.h"
#include "platform/public/ScreenInfo.h"

namespace LWE {
enum class IdleModeJob;
}

namespace Starfish {

enum StarfishStartUpFlag {
    enableComputedStyleDump = 1 << 1,
    enableFrameTreeDump = 1 << 2,
    enableStackingContextDump = 1 << 3,
    enableHitTestDump = 1 << 4,
    enableDebugGraphicsLayer = 1 << 5,
    enableDebugRepaintRegion = 1 << 6,
    enableRegressionTest = 1 << 7,
};

enum StarfishDeviceKind {
    deviceKindUseMouse = 0,
    deviceKindUseTouchScreen = 1 << 0,
};

#ifdef STARFISH_ENABLE_TEST
enum StarfishTestCompatibleMode {
    Normal = 0,
    ChromiumLayout,
};
#endif

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
class PlatformWindow;
class MessageLoop;
class Timer;
class Thread;
class ThreadPool;
class Mutex;
class Inspector;
class MouseData;
class TouchData;
class PlatformKeyEventData;
class EventTarget;
class Scrolling;
class NativeImageData;

#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
class Avplay;
#endif
#ifdef STARFISH_ENABLE_TTS
class TTS;
#endif
union FontFamilyData;

enum class TouchEventKind;
enum class KeyEventKind;
enum class MouseEventKind;
enum class CompositionEventKind;
enum class HistoryManagerAction;

class WebView : public WebBase {
    friend class BrowsingContext;
    friend class StackingContext;
    friend class PlatformWindow;
    friend class Timer;
    friend class AnimationExecutor;
    friend class ResourceLoader;
    friend class FileURLResourceRequestJobDelegate; // Custom file IO
public:
    static WebView* create(
        Starfish* starfish, const char* locale, const char* timezoneID,
        uint32_t windowInitalWidth, uint32_t windowInitalHeight,
        uint32_t defaultFontSize, String* defaultFontName,
        const ScreenInfo& info,
        String* customUserAgentString = String::emptyString,
        String* builtinPolyfillPathString = String::emptyString);
    void destroy();

    bool isWebView() const override
    {
        return true;
    }

    PlatformWindow* platformWindow()
    {
        return m_platformWindow;
    }

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

    void loadHTMLDocument(String* filePath); // navigate function helper
    void navigate(ResourceURL* url, HistoryManagerAction type,
                  ReferrerURL* referrerURL);

    ScriptEngineInstance* scriptEngineInstance()
    {
        return m_scriptEngineInstance;
    }

    void ensureScriptEngineInstance();
    void removeScriptEngineInstance();

    void addJavaScriptNativeInterface(
        String* exposedObjectName, String* jsFunctionName, void* scriptObject,
        Escargot::ScriptNativeFunctionPointer scriptNativeFunctionPointer);
    void removeJavaScriptNativeInterface(String* exposedObjectName,
                                         String* jsFunctionName);
    void applyJavaScriptNativeInterface(ScriptBindingInstance* instance);

    BlobURLStore addMediaSourceInBlobURLStore(MediaSource* ptr);
    void removeMediaSourceFromBlobURLStore(MediaSource* ptr);
    bool isValidMediaSourceBlobURL(BlobURLStore ptr);
    bool isValidMediaSourceBlobURL(MediaSource* ptr);
    BlobURLStore findMediaSourceBlobURL(MediaSource* ptr);
    void clearMediaSourceBlobURLStore();

    void layoutIfNeeded(bool shouldCareStackingContextNow = true);
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
    void pause();
    void resume();
    void resize(uint32_t w, uint32_t h);

    bool isActive()
    {
        return m_isActive;
    }

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

    void setNeedsFullRepainting();

    bool inRendering()
    {
        return m_inRendering;
    }

    void onIdle();
    void clearDrawnBuffers();

    bool hasActiveAnimationExecutor()
    {
        return m_activeAnimationExecutor.size();
    }

    GCVector<AnimationExecutor*>& activeAnimationExecutor()
    {
        return m_activeAnimationExecutor;
    }

    bool needsComposite()
    {
        return m_needsComposite;
    }

    bool needsRendering()
    {
        return m_needsRendering;
    }

    bool needsContinuousRendering()
    {
        return m_needsContinuousRendering;
    }

    PrevDrawnStackingContextInfoMap& prevDrawnStackingContextInfo()
    {
        return m_prevDrawnStackingContextInfo;
    }

    StarfishStartUpFlag startUpFlag()
    {
        return (StarfishStartUpFlag)m_startUpFlag;
    }

    StarfishDeviceKind deviceKind()
    {
        return m_deviceKind;
    }

    uint32_t defaultFontSize() const
    {
        return m_defaultFontSize;
    }
    void setDefaultFontSize(uint32_t size);

    const ScreenInfo& screenInfo() const
    {
        return m_screenInfo;
    }

    ScreenInfo& mutableScreenInfo()
    {
        return m_screenInfo;
    }

    String* builtinPolyfillPathString()
    {
        return m_builtinPolyfillPathString;
    }

#ifdef STARFISH_ENABLE_TEST
    void setTestCompatibleMode(StarfishTestCompatibleMode mode)
    {
        m_testCompatibleMode = mode;
    }

    StarfishTestCompatibleMode testCompatibleMode()
    {
        return (StarfishTestCompatibleMode)m_testCompatibleMode;
    }
#endif

#if defined(STARFISH_ENABLE_INSPECTOR)
    Inspector* inspector() const override
    {
        return m_inspector;
    }

    void setupInspector(uint32_t portNumber = 23888);
#endif

    GCVector<Thread*>& parallelJobExecutorThreadPool()
    {
        return m_parallelJobExecutorThreadPool;
    }

    String* evaluateJavaScript(String* s);
    void evaluateJavaScript(String* s, std::function<void(std::string)> cb);

    std::unordered_map<std::string, void*>& publicLayerUserDataMap()
    {
        return m_publicLayerUserDataMap;
    }

#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
    Avplay* avplay()
    {
        return m_avplay;
    }
#endif
#ifdef STARFISH_ENABLE_TTS
    TTS* tts() const
    {
        return m_tts;
    }
#endif
    PlatformFontSelector* platformFontSelector()
    {
        return m_platformFontSelector;
    }

    PlatformFontCache* platformFontCache()
    {
        return m_platformFontCache;
    }

    FontFamilyData* initialFontFamilyDatas()
    {
        return m_initialFontFamilyDatas;
    }

    uint64_t lastRenderingTick() override
    {
        return m_lastRenderingTick;
    }

    const RepaintRegion& repaintRegionInRendering()
    {
        return m_repaintRegionInRendering;
    }

    GCUnorderedSet<Scrolling*>& activeScrollingSet()
    {
        return m_activeScrollingSet;
    }

    void dispatchTouchEvent(TouchEventKind kind, TouchData* touches,
                            size_t touchCount);
    void dispatchMouseEvent(MouseEventKind kind, MouseData data);
    void dispatchMouseWheelEvent(
        float screenX, float screenY, int z,
        bool isVerticalWheelEvent); // z : -1(up, left) or 1(down, right)
    void dispatchKeyEvent(KeyEventKind kind, PlatformKeyEventData data);
    void dispatchCompositionEvent(CompositionEventKind kind, String* data,
                                  Nullable<Node*> node);
    // starting global pointing Intercept must use default event.
    void addGlobalPointingEventInterceptListener(EventTarget* node);
    void removeGlobalPointingEventInterceptListener(EventTarget* node);

    void setBaseBackgroundColor(Unit::Color color)
    {
        m_baseBackgroundColor = color;
    }

    Unit::Color baseBackgroundColor()
    {
        return m_baseBackgroundColor;
    }

    void setNeedsDownloadWebFontsEarly(bool b)
    {
        m_needsDownloadWebFontsEarly = b;
    }

    bool needsDownloadWebFontsEarly()
    {
        return m_needsDownloadWebFontsEarly;
    }

    void setNeedsDownScaleImageResourceLargerThan(uint32_t demention)
    {
        m_needsDownScaleImageResourceLargerThan = demention;
    }

    uint32_t needsDownScaleImageResourceLargerThan()
    {
        return m_needsDownScaleImageResourceLargerThan;
    }

    void setScrollbarVisible(bool visible)
    {
        m_scrollbarVisible = visible;
    }

    bool scrollbarVisible()
    {
        return m_scrollbarVisible;
    }

    void setUseExternalPopup(bool useExternalPopup)
    {
        m_useExternalPopup = useExternalPopup;
    }

    bool useExternalPopup()
    {
        return m_useExternalPopup;
    }

    void setBaseForegroundColor(Unit::Color color)
    {
        m_baseForegroundColor = color;
    }

    Unit::Color baseForegroundColor()
    {
        return m_baseForegroundColor;
    }

    void setIdleModeJob(LWE::IdleModeJob job)
    {
        m_idleModeJob = job;
    }

    LWE::IdleModeJob idleModeJob()
    {
        return m_idleModeJob;
    }

    void setIdleModeCheckIntervalInMS(uint32_t i);
    uint32_t idleModeCheckIntervalInMS()
    {
        return m_idleModeCheckIntervalInMS;
    }

    bool didFirstRenderingAfterWakeup()
    {
        return m_didFirstRenderingAfterWakeup;
    }
#if defined(STARFISH_ENABLE_MULTI_THREAD_IMAGE_DECODING)
    ThreadPool* imageDecodeThreadPool()
    {
        return m_imageDecodeThreadPool;
    }
#endif
    // active image URLs functions
    // active image URLs are updated while painting(in rendering)
    void clearActiveImageURLsInRenderingSet();
    void putURLIntoActiveImageURLsInRenderingSet(const std::string& url);
    bool isThereURLInActiveImageURLsInRenderingSet(const std::string& url);
    bool areThereMoreThanThreeImageURLsInRenderingSet();

    void accessActiveImageURLsInRenderingSet(
        void (*callback)(const std::string& url, NULLABLE void* data),
        NULLABLE void* data);

    void putImageIntoBoxShadowCache(FrameBox* box, size_t idx,
                                    NativeImageData* image);
    Nullable<NativeImageData*> isThereImageInBoxShadowCache(FrameBox* box,
                                                            size_t idx);

private:
    WebView(Starfish* starfish, const char* locale, const char* timezoneID,
            uint32_t w, uint32_t h, uint32_t defaultFontSize,
            String* defaultFontName, const ScreenInfo& info,
            String* customUserAgentString, String* builtinPolyfillPathString);

    void initRenderingFlags();
    void enterIdleMode();

    RenderResult rendering(
        bool force = false); // returns did painting | did compositing
    void setNeedsRendering() override;
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

    void computeLayoutPaintingDirty();

    void initStorage();

    void navigateCrossDocument(ResourceURL* url, HistoryManagerAction type,
                               ReferrerURL* referrerURL);

    void navigateSameDocument(ResourceURL* url, HistoryManagerAction type,
                              ReferrerURL* referrerURL);

    PlatformWindow* m_platformWindow;
    BrowsingContext* m_topLevelBrowsingContext;

    ScriptEngineInstance* m_scriptEngineInstance;

    StorageNamespaceProvider* m_storageNamespaceProvider;

    StorageNamespace* m_localStorageNamespace;
    StorageNamespace* m_sessionStorageNamespace;

    HistoryManager* m_historyManager;

    GCUnorderedSet<BlobURLStore> m_urlMediaSourceBlobStore;

    PrevDrawnStackingContextInfoMap m_prevDrawnStackingContextInfo;
    GCVector<StackingContext*> m_stackingContextsNeedsGraphicsBuffer;

    uint64_t m_lastRenderingTick;
    uint64_t m_navigateStartingTime;
    uint32_t m_currentActiveAnimatorCount;
    RepaintRegionTrackerContext m_repaintRegionTrackerContext;
    RepaintRegion m_repaintRegionInRendering;
    bool m_inRendering;
    bool m_needsRendering;
    bool m_needsEstablishesStackingContext;
    bool m_needsComputeStackingContextProperties;
    bool m_needsPainting;
    bool m_needsComposite;
    bool m_needsContinuousRendering;
    bool m_needsFullPainting;
    bool m_didCompositeBefore; // last state of enabling composite
    bool m_isActive; // false means that is paused, then rendering callbacks
                     // will be skipped.
    bool m_inIdleMode;
    bool m_didFirstRenderingAfterWakeup;

    GCVector<BrowsingContext*> m_browsingContextsNeedsLayout;
    GCVector<BrowsingContext*> m_browsingContextsDidLayout;
    StackingContext* m_rootStackingContext;
    GCVector<AnimationExecutor*> m_activeAnimationExecutor;

    GCVector<Thread*> m_parallelJobExecutorThreadPool;

#ifdef STARFISH_ENABLE_TTS
    TTS* m_tts;
#endif
#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
    Avplay* m_avplay;
#endif
#if defined(STARFISH_ENABLE_INSPECTOR)
    Inspector* m_inspector;
#endif
    PlatformFontSelector* m_platformFontSelector;
    PlatformFontCache* m_platformFontCache;
    FontFamilyData* m_initialFontFamilyDatas;

    // when painting tile, each box can be painted multiple times
    template <class T1, class T2>
    struct pair_hash {
        size_t operator()(const std::pair<T1, T2>& pair) const
        {
            return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
        }
    };
    GCUnorderedMap<std::pair<FrameBox*, size_t>, NativeImageData*,
                   pair_hash<FrameBox*, size_t>>
        m_boxShadowCachePerRendering;

    GCVector<EventTarget*> m_globalPointingEventListener;
    GCUnorderedSet<Scrolling*> m_activeScrollingSet;
    Unit::Location m_lastMouseMovePoint;

#if defined(STARFISH_ENABLE_MULTI_THREAD_IMAGE_DECODING)
    ThreadPool* m_imageDecodeThreadPool;
#endif
    std::unordered_set<std::string> m_activeImageURLsInRendering;
    Mutex* m_activeImageURLsInRenderingMutex;

    // options
    uint32_t m_defaultFontSize;
    ScreenInfo m_screenInfo;
    String* m_builtinPolyfillPathString;
    unsigned int m_startUpFlag;
    StarfishDeviceKind m_deviceKind;

    std::unordered_map<std::string, void*> m_publicLayerUserDataMap;

    GCVector<std::tuple<String*, String*, void*,
                        Escargot::ScriptNativeFunctionPointer>>
        m_jsInterfaceList;

#ifdef STARFISH_ENABLE_TEST
    unsigned int m_testCompatibleMode;
#endif
    Unit::Color m_baseBackgroundColor;
    Unit::Color m_baseForegroundColor;
    LWE::IdleModeJob
        m_idleModeJob; // default value is IdleModeJob::IdleModeFull
    uint32_t m_idleModeCheckIntervalInMS; // default value is 3000(ms)
    size_t m_idleCheckTimerID;
    bool m_needsDownloadWebFontsEarly;
    uint32_t m_needsDownScaleImageResourceLargerThan;
    bool m_scrollbarVisible;
    bool m_useExternalPopup;

    static size_t g_fillingGraphicsBufferTileFrameTimeLimitInMS;
};
} // namespace Starfish

#endif
