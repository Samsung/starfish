/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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
    enableRegressionTest = 1 << 5,
};

enum StarFishDeviceKind {
    deviceKindUseMouse = 0,
    deviceKindUseTouchScreen = 1 << 0,
};

void addGCCollectionListener(void (*fn)(GC_EventType));

// you must call delete
// StarFish::StarFish function is NOT THREAD-SAFE
class StarFish : public gc {
    friend class AtomicString;
    friend class StaticStrings;
    friend class StarFishEnterer;
    friend class HTMLDocument; // m_caseInsensitiveAttrSet

public:
    StarFish(StarFishStartUpFlag flag, const char* locale,
             const char* timezoneID, void* platformHandle, int w, int h, int x,
             int y, float defaultFontSizeMultiplier, String* defaultFontName,
             const ScreenInfo& info, const char* localStorageFilePath,
             const char* cookieStoreFilePath,
             const char* httpCacheDirectorypath,
             String* customUserAgentString = String::emptyString,
             String* builtinPolyfillPathString = String::emptyString);

    ~StarFish();
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

    void* nativeHandle()
    {
        return m_nativeHandle;
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

#if defined(PORT_GRAPHIC_BACKEND_GENERAL_BUFFER)
    void registerFrameBuffer(void* framBuffer1, void* framBuffer2)
    {
        m_frameBufffer1 = framBuffer1;
        m_frameBufffer2 = framBuffer2;
    }
    void* frameBuffer();
    void setNeedsUpdate();
    int frameBufferUpdate();
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

#ifdef TIZEN_DEVICE_API
    void setWidgetContext(const void* widgetContext)
    {
        m_widgetContext = widgetContext;
    }

    const void* widgetContext()
    {
        return m_widgetContext;
    }
#endif
#ifdef STARFISH_TIZEN_WEARABLE
    void enableUpdate()
    {
        m_updateFlag = true;
    }

    bool updateFlag()
    {
        return m_updateFlag;
    }
#endif

protected:
    void enter();
    void exit();

    size_t posPrefix(std::string str, std::string prefix)
    {
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        return str.find(prefix);
    }
    StaticStrings* m_staticStrings;
    icu::Locale m_locale;
    LineBreakIteratorPool* m_lineBreakIteratorPool;
    String* m_timezoneID;
    unsigned int m_startUpFlag;
    float m_defaultFontSizeMultiplier;
    float m_screenScaleRatio;
    StarFishDeviceKind m_deviceKind;
    MessageLoop* m_messageLoop;
    Timer* m_timer;
    void* m_nativeHandle;
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
#ifdef TIZEN_DEVICE_API
    const void* m_widgetContext;
#endif
#ifdef STARFISH_TIZEN_WEARABLE
    bool m_updateFlag;
#endif
    size_t m_enterCount;
    ScreenInfo m_screenInfo;
    String* m_localStorageFilePath;
    String* m_customUserAgentString;
    String* m_builtinPolyfillPathString;

#ifdef PORT_GRAPHIC_BACKEND_GENERAL_BUFFER
    int m_width;
    int m_height;
    int bufferIdx;
    int m_completBufferIdx;
    void* m_frameBufffer1;
    void* m_frameBufffer2;
    bool m_needsUpdate;
    Mutex* m_frameBufferSwitchMutex;
#endif

    GCUnorderedMap<void*, size_t> m_rootMap;
    AtomicStringMap m_atomicStringMap;
    GCUnorderedMap<String*, size_t> m_caseInsensitiveAttrSet;
    GCVector<Thread*> m_activeThreadList;
    Mutex* m_activeThreadListMutex;
#ifdef STARFISH_ENABLE_HTTPCACHE
    HTTPCache* m_httpCache;
#endif
#ifdef STARFISH_ENABLE_TTS
    TTS* m_tts;
#endif
    GCUnorderedMap<String*, float> m_profilingRecods;
    FontFamilyData* m_initialFontFamilyDatas;

private:
    void initNetworkSharedResourceManager(const char* cookieStoreFilePath);
};

class StarFishEnterer {
public:
    StarFishEnterer(StarFish* instance)
        : m_instance(instance)
    {
        m_instance->enter();
    }

    ~StarFishEnterer()
    {
        m_instance->exit();
    }

protected:
    StarFish* m_instance;
};

#ifdef STARFISH_ENABLE_TEST
extern bool g_enablePixelTest;
extern bool g_memLogDump;
#endif
}

#endif
