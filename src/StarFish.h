/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#ifndef __StarFish__
#define __StarFish__

#include "platform/public/ScreenInfo.h"
#include "StaticStrings.h"
namespace StarFish {

class MessageLoop;
class Timer;
class Window;
class PlatformWindow;
class NativeImageData;
class LineBreakIteratorPool;
class Thread;
class ThreadPool;
class Console;
class Inspector;
class Mutex;
union FontFamilyData;
#if defined(STARFISH_ENABLE_HTTPCACHE)
class HTTPCache;
#endif
#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
class Avplay;
#endif
#ifdef STARFISH_ENABLE_TTS
class TTS;
#endif
enum StarFishStartUpFlag {
    enableComputedStyleDump = 1 << 1,
    enableFrameTreeDump = 1 << 2,
    enableStackingContextDump = 1 << 3,
    enableHitTestDump = 1 << 4,
    enableDebugGraphicsLayer = 1 << 5,
    enableDebugRepaintRegion = 1 << 6,
    enableRegressionTest = 1 << 7,
};

enum StarFishDeviceKind {
    deviceKindUseMouse = 0,
    deviceKindUseTouchScreen = 1 << 0,
};

#ifdef STARFISH_ENABLE_TEST
enum StarFishTestCompatibleMode {
    Normal = 0,
    ChromiumLayout,
};
#endif

void addGCCollectionListener(void (*fn)(GC_EventType));

// you must call close function
// ctor of StarFish class is NOT THREAD-SAFE
class StarFish : public gc {
    friend class AtomicString;
    friend class StaticStrings;
    friend class HTMLDocument;                      // m_caseInsensitiveAttrSet
    friend class FileURLResourceRequestJobDelegate; // Custom file IO
public:
    StarFish(const char* locale, const char* timezoneID, int w, int h,
             float defaultFontSizeMultiplier, String* defaultFontName,
             const ScreenInfo& info, const char* localStorageFilePath,
             const char* cookieStoreFilePath,
             const char* httpCacheDirectorypath,
             String* customUserAgentString = String::emptyString,
             String* builtinPolyfillPathString = String::emptyString);

    void run();

    PlatformWindow* platformWindow()
    {
        return m_platformWindow;
    }

    void loadHTMLDocument(String* filePath);

    void resume();
    void pause();
    void close();

    String* evaluate(String* s);

    StaticStrings* staticStrings()
    {
        return m_staticStrings;
    }

    MessageLoop* messageLoop()
    {
        return m_messageLoop;
    }

    Timer* timer()
    {
        return m_timer;
    }

    StarFishStartUpFlag startUpFlag()
    {
        return (StarFishStartUpFlag)m_startUpFlag;
    }

    StarFishDeviceKind deviceKind()
    {
        return m_deviceKind;
    }

    const icu::Locale& locale()
    {
        return m_locale;
    }

    LineBreakIteratorPool* lineBreakIteratorPool()
    {
        return m_lineBreakIteratorPool;
    }

    String* timezoneID()
    {
        return m_timezoneID;
    }

    ThreadPool* threadPool()
    {
        return m_threadPool;
    }

    float defaultFontSizeMultiplier()
    {
        return m_defaultFontSizeMultiplier;
    }

    ScreenInfo& screenInfo()
    {
        return m_screenInfo;
    }

    PlatformFontSelector* platformFontSelector()
    {
        return m_platformFontSelector;
    }

    PlatformFontCache* platformFontCache()
    {
        return m_platformFontCache;
    }

    String* localStorageFilePath()
    {
        return m_localStorageFilePath;
    }

#ifdef STARFISH_ENABLE_HTTPCACHE
    HTTPCache* httpCache()
    {
        return m_httpCache;
    }

#endif
    String* customUserAgentString()
    {
        return m_customUserAgentString;
    }

    void setCustomUserAgentString(String* customUserAgentString)
    {
        m_customUserAgentString = customUserAgentString;
    }

    String* builtinPolyfillPathString()
    {
        return m_builtinPolyfillPathString;
    }

    String* userAgent();

    Console* console()
    {
        return m_console;
    }

#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
    Avplay* avplay()
    {
        return m_avplay;
    }
#endif

#if defined(STARFISH_ENABLE_INSPECTOR)
    Inspector* inspector()
    {
        return m_inspector;
    }

    void setupInspector(uint32_t portNumber = 23888);
#endif

    void addActiveThread(Thread* thread);
    void removeActiveThread(Thread* thread);
    void joinAllActiveThread();

#ifdef STARFISH_ENABLE_TTS
    TTS* tts()
    {
        return m_tts;
    }
#endif
    void addPointerInRootSet(void* ptr);
    void removePointerFromRootSet(void* ptr);
#ifndef NDEBUG
    size_t countPointersInRootSet(void* ptr);
#endif

    void updateProfileRecord(String* tag, float time)
    {
        auto it = m_profilingRecods.find(tag);
        if (it == m_profilingRecods.end()) {
            m_profilingRecods.insert(std::make_pair(tag, time));
        } else {
            it->second = time;
        }
    }

    float profileRecode(String* tag) const
    {
        auto it = m_profilingRecods.find(tag);
        if (it == m_profilingRecods.end()) {
            return 0;
        }

        return it->second;
    }

    FontFamilyData* initialFontFamilyDatas()
    {
        return m_initialFontFamilyDatas;
    }

#ifdef STARFISH_ENABLE_TEST
    void setTestCompatibleMode(StarFishTestCompatibleMode mode)
    {
        m_testCompatibleMode = mode;
    }

    StarFishTestCompatibleMode testCompatibleMode()
    {
        return (StarFishTestCompatibleMode)m_testCompatibleMode;
    }
#endif

    void setProxyURL(const std::string& url)
    {
        m_proxyURL = url;
    }

    const std::string& proxyURL()
    {
        return m_proxyURL;
    }

    GCVector<Thread*>& parallelJobExecutorThreadPool()
    {
        return m_parallelJobExecutorThreadPool;
    }

    void registerWebViewHandler(const std::string& handlerName,
                                std::function<void(String*, int)> handler);
    void registerWebViewHandler(const std::string& handlerName,
                                std::function<void(void*)> handler);
    bool containsWebViewHandler(const std::string& handlerName);
    void callWebViewHandler(const std::string& handlerName, String* url,
                            int param = 0);
    void callWebViewHandler(const std::string& handlerName, void* data);

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

    std::unordered_map<std::string, void*>& publicLayerUserDataMap()
    {
        return m_publicLayerUserDataMap;
    }

protected:
    StaticStrings* m_staticStrings;
    icu::Locale m_locale;
    std::string m_proxyURL;
    LineBreakIteratorPool* m_lineBreakIteratorPool;
    String* m_timezoneID;
    unsigned int m_startUpFlag;
    float m_defaultFontSizeMultiplier;
    float m_screenScaleRatio;
    StarFishDeviceKind m_deviceKind;
    MessageLoop* m_messageLoop;
    Timer* m_timer;
    PlatformWindow* m_platformWindow;
    PlatformFontSelector* m_platformFontSelector;
    PlatformFontCache* m_platformFontCache;
    ThreadPool* m_threadPool;
    Console* m_console;
#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
    Avplay* m_avplay;
#endif
#if defined(STARFISH_ENABLE_INSPECTOR)
    Inspector* m_inspector;
#endif
    ScreenInfo m_screenInfo;
    String* m_localStorageFilePath;
    String* m_customUserAgentString;
    String* m_builtinPolyfillPathString;

    GCUnorderedMap<void*, size_t> m_rootMap;
    AtomicStringMap m_atomicStringMap;
    GCUnorderedMap<String*, size_t> m_caseInsensitiveAttrSet;
    GCVector<Thread*> m_activeThreadList;
    GCVector<Thread*> m_parallelJobExecutorThreadPool;
    Mutex* m_activeThreadListMutex;
#ifdef STARFISH_ENABLE_HTTPCACHE
    HTTPCache* m_httpCache;
#endif
#ifdef STARFISH_ENABLE_TTS
    TTS* m_tts;
#endif
    GCUnorderedMap<String*, float> m_profilingRecods;
    FontFamilyData* m_initialFontFamilyDatas;
#ifdef STARFISH_ENABLE_TEST
    unsigned int m_testCompatibleMode;
#endif

    std::unordered_map<std::string, std::function<void(String*, int)>>
        m_lweWebViewHandlers;
    std::unordered_map<std::string, std::function<void(void*)>>
        m_lweWebViewHandlersGeneral;

    // function sets for implementing custom file IO for resource request
    std::function<const char*(const char* path)> m_resolveFilePathCallback;
    std::function<void*(const char* path)> m_fileOpenCallback;
    std::function<size_t(uint8_t* destBuffer, size_t size, void* handle)>
        m_fileReadCallback;
    std::function<long int(void* handle)> m_fileLengthCallback;
    std::function<void(void* handle)> m_fileCloseCallback;
    // <----

    std::unordered_map<std::string, void*> m_publicLayerUserDataMap;

private:
    void initNetworkSharedResourceManager(const char* cookieStoreFilePath);
};

#ifdef STARFISH_ENABLE_TEST
extern bool g_enablePixelTest;
extern bool g_memLogDump;
extern bool g_enableDumpAsText;
extern bool g_DumpAsText_Async;
extern int g_referenceTestState; // 0:None, 1:RunningTC, 2:RunningReference
#endif
}

#endif
