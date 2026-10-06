/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#ifndef __StarfishPulseSimple__
#define __StarfishPulseSimple__

#include <cstddef>
#include <cstdint>

namespace Starfish {

// PulseAudio Simple API is loaded at runtime to avoid a libpulse-dev build
// dependency. These declarations match pulse/simple.h and pulse/sample.h.
enum pa_sample_format_t { PA_SAMPLE_S16LE = 3 };
enum pa_stream_direction_t { PA_STREAM_PLAYBACK = 1 };

struct pa_sample_spec {
    pa_sample_format_t format;
    uint32_t rate;
    uint8_t channels;
};

struct pa_simple;
struct pa_channel_map;
struct pa_buffer_attr {
    uint32_t maxlength;
    uint32_t tlength;
    uint32_t prebuf;
    uint32_t minreq;
    uint32_t fragsize;
};

struct PulseSimpleApi {
    pa_simple* (*pa_simple_new)(const char*, const char*, pa_stream_direction_t,
                                const char*, const char*, const pa_sample_spec*,
                                const pa_channel_map*, const pa_buffer_attr*,
                                int*);
    int (*pa_simple_write)(pa_simple*, const void*, size_t, int*);
    int (*pa_simple_drain)(pa_simple*, int*);
    int (*pa_simple_flush)(pa_simple*, int*);
    uint64_t (*pa_simple_get_latency)(pa_simple*, int*);
    void (*pa_simple_free)(pa_simple*);
};

PulseSimpleApi* loadPulseSimple(void*& handleOut);

} // namespace Starfish

#endif
