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
class ImageData;
class LineBreakIteratorPool;
class ThreadPool;
class Console;
class Inspector;

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
             const char* timezoneID, void* platformHandle, int w, int h,
             float defaultFontSizeMultiplier, ScreenInfo& info,
             const char* localStorageFilePath,
             const char* m_cookieStoreFilePath);
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

    Font* fetchFont(String* familyName, float size,
                    char style = FontStyle::FontStyleNormal,
                    char weight = FontWeight::FontWeightNormal)
    {
        Font* f = nullptr;
        f = m_fontSelector.loadFont(familyName, size, style, weight);
        return f;
    }

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

    String* localStorageFilePath()
    {
        return m_localStorageFilePath;
    }

    void addPointerInRootSet(void* ptr);
    void removePointerFromRootSet(void* ptr);
#ifndef NDEBUG
    size_t countPointersInRootSet(void* ptr);
#endif

    Console* console()
    {
        return m_console;
    }

#if defined(STARFISH_ENABLE_INSPECTOR)
    Inspector* inspector()
    {
        return m_inspector;
    }

    void setupInspector(uint32_t portNumber = 23888);
#endif

#if defined(PORT_GRAPHIC_BACKEND_GENERAL_BUFFER)
    void registerFrameBuffer(void* framBuffer)
    {
        m_frameBufffer = framBuffer;
    }
    void* frameBuffer()
    {
        return m_frameBufffer;
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
    StarFishDeviceKind m_deviceKind;
    MessageLoop* m_messageLoop;
    Timer* m_timer;
    void* m_nativeHandle;
    PlatformWindow* m_platformWindow;
    FontSelector m_fontSelector;
    ThreadPool* m_threadPool;
    Console* m_console;
#if defined(STARFISH_ENABLE_INSPECTOR)
    Inspector* m_inspector;
#endif
    size_t m_enterCount;
    ScreenInfo m_screenInfo;
    String* m_localStorageFilePath;
    String* m_cookieStoreFilePath;
#ifdef PORT_GRAPHIC_BACKEND_GENERAL_BUFFER
    int m_width;
    int m_height;
    void* m_frameBufffer;
#endif

    GCUnorderedMap<void*, size_t> m_rootMap;
    AtomicStringMap m_atomicStringMap;
    GCUnorderedMap<String*, size_t> m_caseInsensitiveAttrSet;

private:
    void initCookieSession();
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
