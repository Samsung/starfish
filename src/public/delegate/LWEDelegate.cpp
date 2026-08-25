/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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
#include "Starfish.h"

#include "public/contract/LWEDelegate.h"
#include "ThreadedCallHelper.h"

#include "core/page/WebBase.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/dom/ExecutionContext.h"

#include "core/page/Window.h"

#include "binding/ScriptEngineInstance.h"

#include <EscargotPublic.h>

#if defined(STARFISH_WINDOWS)
#include <fontconfig/fontconfig.h>
#include <windows.h>
#include "WindowsFontconfigConfig.h"
#endif

using namespace Escargot;

namespace LWEDelegate {

Starfish::Starfish* g_starfishInstance;
Starfish::Starfish** g_starfishInstanceHolder;
static void StarfishGCMemoryLogger(void* data)
{
    STARFISH_LOG_INFO("Done GC: HeapSize: [%f MB , %f MB]",
                      GC_get_memory_use() / 1024.f / 1024.f,
                      GC_get_heap_size() / 1024.f / 1024.f);
}

#if defined(STARFISH_WINDOWS)
// Fontconfig has no usable system-wide configuration on Windows. Rather than
// shipping fonts.conf/conf.d next to this library and pointing FONTCONFIG_FILE
// at it, load the flattened build-time snapshot of that same config (see
// tool/build/gen_windows_fontconfig_config.py) straight from memory: its own
// <dir>/<cachedir> entries already resolve through Windows-native tokens
// (WINDOWSFONTDIR, LOCAL_APPDATA_FONTCONFIG_CACHE, ...), so nothing needs to
// be located or deployed at runtime.
static void loadWindowsFontconfigConfig()
{
    // The generated chunks are stored as separate literals (never
    // concatenated by the compiler into one, to stay under MSVC's
    // 65535-byte string literal limit -- see the generator), so they need to
    // be joined back into one buffer here instead.
    std::string xml;
    for (size_t i = 0; i < Starfish::g_windowsFontconfigConfigXmlChunkCount;
         ++i) {
        xml += Starfish::g_windowsFontconfigConfigXmlChunks[i];
    }

    FcConfig* config = FcConfigCreate();
    if (!FcConfigParseAndLoadFromMemory(
            config, reinterpret_cast<const FcChar8*>(xml.c_str()), FcTrue)) {
        STARFISH_LOG_ERROR(
            "Failed to parse the embedded Fontconfig configuration");
    }
    FcConfigBuildFonts(config);
    FcConfigSetCurrent(config);
    // FcConfigSetCurrent() takes its own reference; release ours.
    FcConfigDestroy(config);
}
#endif

void LWE::Initialize(const char* storageDirectoryPath, uint32_t option)
{
    STARFISH_RELEASE_ASSERT(!IsInitialized());

    std::string backend = STARFISH_BACKEND_STR;
    // Extract thread mode from option flags.
    // If PreferSeparateThread is set, use a separate thread; otherwise prefer
    // the process main thread. Note: Some backends ignore this preference and
    // always use thread mode.
    bool isThreadMode = option & kInitializeOptionPreferSeparateThread;
    // Apply PreferIncrementalGC
    if (option & kInitializeOptionPreferIncrementalGC) {
#if defined(OS_POSIX)
        setenv("GC_ENABLE_INCREMENTAL", "1", 1);
#endif
    }
    Starfish::StarfishRendererType rendererType =
        Starfish::StarfishRendererType::kOpenGL;
    if (backend == "uv_cairo_gl" || backend == "flutter" ||
        backend == "uv_worker") {
        // These backends always require thread mode regardless of preference
        isThreadMode = true;
    }
    if (backend == "glib_headless") {
        rendererType = Starfish::StarfishRendererType::kHeadless;
    }
#ifdef STARFISH_ENABLE_TEST
    setenv("BACKEND", backend.data(), TRUE);
#endif

    // Diagnostic only (never used to gate runWithProcessMainThreadPausedSync
    // -- see MessageLoop::isCallerInsideBackendEventLoop()'s doc comment).
    // This is the right place to check it because LWE::Initialize() always
    // runs on whatever thread the embedder calls it from -- typically the
    // process main thread, alongside setting up its other UI components --
    // regardless of isThreadMode. In isolated thread mode, MessageLoop::init()
    // itself instead runs on the freshly created dedicated LWE thread, which
    // can't tell us anything about the process main thread.
    STARFISH_LOG_INFO(
        "LWE::Initialize() called with isThreadMode=%d, "
        "isCallerInsideBackendEventLoop=%d",
        isThreadMode, Starfish::MessageLoop::isCallerInsideBackendEventLoop());

    ThreadedCallHelper::Instance()->Initialize(isThreadMode);

    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([&]() -> void {
#if defined(STARFISH_WINDOWS)
        loadWindowsFontconfigConfig();
#endif
        Starfish::StarfishConfiguration config;
        config.storageDirectoryPath = storageDirectoryPath;
        config.gcFrequency = BDWGC_FREE_SPACE_DIVISOR;
        config.isThreadMode = isThreadMode;
        config.backend = STARFISH_BACKEND_STR;
        config.rendererType = rendererType;

        Starfish::staticallyInitScriptEngine();

        g_starfishInstanceHolder = reinterpret_cast<Starfish::Starfish**>(
            GC_MALLOC_UNCOLLECTABLE(sizeof(Starfish::Starfish**)));
        g_starfishInstance = *g_starfishInstanceHolder =
            new Starfish::Starfish(config);
        // add gc event listener
        Escargot::Memory::removeGCEventListener(
            Escargot::Memory::RECLAIM_END, StarfishGCMemoryLogger, nullptr);
        Escargot::Memory::addGCEventListener(Escargot::Memory::RECLAIM_END,
                                             StarfishGCMemoryLogger, nullptr);
    });
}

void LWE::Finalize()
{
    STARFISH_RELEASE_ASSERT(IsInitialized());

    ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync([]() -> void {
        STARFISH_RELEASE_ASSERT(g_starfishInstance->webViewInstanceCount() ==
                                0);

        g_starfishInstance->destroy();
        g_starfishInstance = nullptr;
        GC_FREE(g_starfishInstanceHolder);
        g_starfishInstanceHolder = nullptr;

        clearStack<ELABORATE_CLEAR_STACK_SIZE>();

        // do implicit calling GC funciton takes a lots time
        // we should remove this if possible
        Starfish::Starfish::doFullGCWithoutSeeingStack();
        Starfish::Starfish::doFullGCWithoutSeeingStack();
        Starfish::Starfish::doFullGCWithoutSeeingStack();

        Starfish::staticallyDestroyScriptEngine();
    });
}

bool LWE::IsInitialized()
{
    return g_starfishInstance;
}

void LWE::SetGCFrequency(unsigned char freq)
{
    STARFISH_RELEASE_ASSERT(IsInitialized());
    g_starfishInstance->setGCFrequency(freq);
}

unsigned char LWE::GetGCFrequency()
{
    STARFISH_RELEASE_ASSERT(IsInitialized());
    return g_starfishInstance->gcFrequency();
}

void LWE::GetVersion(int* major, int* minor, int* patch)
{
    STARFISH_RELEASE_ASSERT(IsInitialized());
    g_starfishInstance->version(major, minor, patch);
}

bool LWE::IsUsingSeparateThread()
{
    return ThreadedCallHelper::Instance()->isThreadMode();
}

} // namespace LWEDelegate

extern "C" {
uint32_t EXPORT_UNMANAGED_API LWEDelegate_GetAbiEpoch()
{
    return LWEDelegate::kDelegateAbiEpoch;
}

void EXPORT_UNMANAGED_API
LWEDelegate_LWE_Initialize(const char* storageDirectoryPath, uint32_t option)

{
    LWEDelegate::LWE::Initialize(storageDirectoryPath, option);
}

bool EXPORT_UNMANAGED_API LWEDelegate_LWE_IsInitialized()
{
    return LWEDelegate::LWE::IsInitialized();
}

void EXPORT_UNMANAGED_API LWEDelegate_LWE_Finalize()
{
    LWEDelegate::LWE::Finalize();
}

unsigned char EXPORT_UNMANAGED_API LWEDelegate_LWE_GetGCFrequency()
{
    return LWEDelegate::LWE::GetGCFrequency();
}

void EXPORT_UNMANAGED_API LWEDelegate_LWE_SetGCFrequency(unsigned char freq)
{
    LWEDelegate::LWE::SetGCFrequency(freq);
}

void EXPORT_UNMANAGED_API LWEDelegate_LWE_GetVersion(int* major, int* minor,
                                                     int* patch)
{
    LWEDelegate::LWE::GetVersion(major, minor, patch);
}

bool EXPORT_UNMANAGED_API LWEDelegate_LWE_IsUsingSeparateThread()
{
    return LWEDelegate::LWE::IsUsingSeparateThread();
}
}
