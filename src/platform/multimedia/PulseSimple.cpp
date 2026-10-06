/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_LINUX) && (defined(STARFISH_USE_FFMPEG_MEDIAPLAYER) || \
                                defined(STARFISH_ENABLE_WEBAUDIO))

#include "StarfishConfig.h"
#include "platform/multimedia/PulseSimple.h"

#include <dlfcn.h>

namespace Starfish {

PulseSimpleApi* loadPulseSimple(void*& handleOut)
{
    struct State {
        void* handle{ nullptr };
        PulseSimpleApi api{};

        State()
        {
            handle = dlopen("libpulse-simple.so.0", RTLD_NOW | RTLD_GLOBAL);
            if (!handle) {
                return;
            }
            api.pa_simple_new = reinterpret_cast<decltype(api.pa_simple_new)>(
                dlsym(handle, "pa_simple_new"));
            api.pa_simple_write =
                reinterpret_cast<decltype(api.pa_simple_write)>(
                    dlsym(handle, "pa_simple_write"));
            api.pa_simple_drain =
                reinterpret_cast<decltype(api.pa_simple_drain)>(
                    dlsym(handle, "pa_simple_drain"));
            api.pa_simple_get_latency =
                reinterpret_cast<decltype(api.pa_simple_get_latency)>(
                    dlsym(handle, "pa_simple_get_latency"));
            api.pa_simple_flush =
                reinterpret_cast<decltype(api.pa_simple_flush)>(
                    dlsym(handle, "pa_simple_flush"));
            api.pa_simple_free = reinterpret_cast<decltype(api.pa_simple_free)>(
                dlsym(handle, "pa_simple_free"));
            if (!api.pa_simple_new || !api.pa_simple_write ||
                !api.pa_simple_free || !api.pa_simple_flush) {
                dlclose(handle);
                handle = nullptr;
            }
        }
    };
    static State state;
    handleOut = state.handle;
    return state.handle ? &state.api : nullptr;
}

} // namespace Starfish

#endif
