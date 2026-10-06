/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/modules/webaudio/render/AudioDecoder.h"
#include "core/modules/webaudio/render/AudioBufferData.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace Starfish {

static uint16_t read16(const uint8_t* p)
{
    return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}

static uint32_t read32(const uint8_t* p)
{
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) |
           (static_cast<uint32_t>(p[3]) << 24);
}

static uint64_t read64(const uint8_t* p)
{
    return static_cast<uint64_t>(read32(p)) |
           (static_cast<uint64_t>(read32(p + 4)) << 32);
}

static bool tagEquals(const uint8_t* p, const char* tag)
{
    return memcmp(p, tag, 4) == 0;
}

struct WaveFormat {
    uint16_t encoding{ 0 };
    uint16_t channels{ 0 };
    uint16_t bitsPerSample{ 0 };
    uint16_t validBits{ 0 };
    uint16_t blockAlign{ 0 };
    uint32_t sampleRate{ 0 };
};

bool isMalformedWaveAudio(const uint8_t* bytes, size_t length)
{
    if (length < 12 || !tagEquals(bytes, "RIFF") ||
        !tagEquals(bytes + 8, "WAVE")) {
        return false;
    }
    uint64_t riffEnd = static_cast<uint64_t>(read32(bytes + 4)) + 8;
    if (riffEnd < 12 || riffEnd > length) {
        return true;
    }
    size_t end = static_cast<size_t>(riffEnd);
    for (size_t offset = 12; offset < end;) {
        if (end - offset < 8) {
            return true;
        }
        size_t payload = offset + 8;
        uint32_t size = read32(bytes + offset + 4);
        if (size > end - payload || (size & 1) > end - payload - size) {
            return true;
        }
        offset = payload + size + (size & 1);
    }
    return false;
}

static bool parseFormat(const uint8_t* p, size_t size, WaveFormat& format)
{
    if (size < 16) {
        return false;
    }
    format.encoding = read16(p);
    format.channels = read16(p + 2);
    format.sampleRate = read32(p + 4);
    format.blockAlign = read16(p + 12);
    format.bitsPerSample = read16(p + 14);
    format.validBits = format.bitsPerSample;

    if (format.encoding == 0xfffe) {
        // WAVE_FORMAT_EXTENSIBLE subtype GUID: PCM or IEEE float.
        static const uint8_t guidTail[] = {
            0x00, 0x00, 0x10, 0x00, 0x80, 0x00,
            0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71
        };
        if (size < 40 || read16(p + 16) < 22 ||
            memcmp(p + 28, guidTail, sizeof(guidTail)) != 0 ||
            read16(p + 26) != 0) {
            return false;
        }
        format.encoding = read16(p + 24);
        format.validBits = read16(p + 18);
    }

    if (format.channels < 1 || format.channels > 32 ||
        format.sampleRate < 3000 || format.sampleRate > 768000 ||
        format.bitsPerSample % 8 || !format.bitsPerSample ||
        !format.validBits || format.validBits > format.bitsPerSample ||
        format.blockAlign != format.channels * (format.bitsPerSample / 8)) {
        return false;
    }
    if (format.encoding == 1) {
        return format.bitsPerSample == 8 || format.bitsPerSample == 16 ||
               format.bitsPerSample == 24 || format.bitsPerSample == 32;
    }
    if (format.encoding == 3) {
        return (format.bitsPerSample == 32 || format.bitsPerSample == 64) &&
               format.validBits == format.bitsPerSample;
    }
    return false;
}

static float readSample(const uint8_t* p, const WaveFormat& format)
{
    if (format.encoding == 3) {
        if (format.bitsPerSample == 32) {
            uint32_t bits = read32(p);
            float value;
            memcpy(&value, &bits, sizeof(value));
            return value;
        }
        uint64_t bits = read64(p);
        double value;
        memcpy(&value, &bits, sizeof(value));
        return static_cast<float>(value);
    }

    if (format.bitsPerSample == 8) {
        unsigned shift = 8 - format.validBits;
        int midpoint = 1 << (format.validBits - 1);
        return static_cast<float>(static_cast<int>(p[0] >> shift) - midpoint) /
               midpoint;
    }

    int32_t value;
    if (format.bitsPerSample == 16) {
        value = static_cast<int16_t>(read16(p));
    } else if (format.bitsPerSample == 24) {
        value = static_cast<int32_t>(p[0]) | (static_cast<int32_t>(p[1]) << 8) |
                (static_cast<int32_t>(p[2]) << 16);
        if (value & 0x00800000) {
            value -= 1 << 24;
        }
    } else {
        value = static_cast<int32_t>(read32(p));
    }
    // Extensible PCM stores valid bits left-aligned in the container.
    unsigned shift = format.bitsPerSample - format.validBits;
    if (shift) {
        value >>= shift;
    }
    return static_cast<float>(static_cast<double>(value) /
                              std::ldexp(1.0, format.validBits - 1));
}

AudioBufferData* decodeWaveAudio(const uint8_t* bytes, size_t length,
                                 double targetSampleRate)
{
    if (!bytes || length < 12 || !tagEquals(bytes, "RIFF") ||
        !tagEquals(bytes + 8, "WAVE") || !std::isfinite(targetSampleRate) ||
        targetSampleRate < 3000 || targetSampleRate > 768000) {
        return nullptr;
    }

    uint64_t riffEnd = static_cast<uint64_t>(read32(bytes + 4)) + 8;
    if (riffEnd < 12 || riffEnd > length) {
        return nullptr;
    }
    size_t end = static_cast<size_t>(riffEnd);
    const uint8_t* audio = nullptr;
    size_t audioBytes = 0;
    WaveFormat format;
    bool hasFormat = false;
    for (size_t offset = 12; offset < end;) {
        if (end - offset < 8) {
            return nullptr;
        }
        uint32_t chunkBytes = read32(bytes + offset + 4);
        size_t payload = offset + 8;
        if (chunkBytes > end - payload) {
            return nullptr;
        }
        if (tagEquals(bytes + offset, "fmt ") && !hasFormat) {
            hasFormat = parseFormat(bytes + payload, chunkBytes, format);
            if (!hasFormat) {
                return nullptr;
            }
        } else if (tagEquals(bytes + offset, "data") && !audio) {
            audio = bytes + payload;
            audioBytes = chunkBytes;
        }
        size_t padding = chunkBytes & 1;
        if (padding > end - payload - chunkBytes) {
            return nullptr;
        }
        offset = payload + chunkBytes + padding;
    }

    if (!hasFormat || !audio || !audioBytes || audioBytes % format.blockAlign) {
        return nullptr;
    }
    size_t sourceFrames = audioBytes / format.blockAlign;
    double outputLength = std::round(static_cast<double>(sourceFrames) *
                                     targetSampleRate / format.sampleRate);
    if (outputLength < 1 ||
        outputLength > std::numeric_limits<uint32_t>::max()) {
        return nullptr;
    }
    size_t targetFrames = static_cast<size_t>(outputLength);
    AudioBufferData* data =
        AudioBufferData::create(format.channels, targetFrames);
    if (!data) {
        return nullptr;
    }
    size_t sampleBytes = format.bitsPerSample / 8;
    for (size_t frame = 0; frame < targetFrames; frame++) {
        double sourcePosition =
            static_cast<double>(frame) * format.sampleRate / targetSampleRate;
        size_t first =
            std::min(static_cast<size_t>(sourcePosition), sourceFrames - 1);
        size_t second = std::min(first + 1, sourceFrames - 1);
        float fraction = static_cast<float>(sourcePosition - first);
        const uint8_t* firstFrame = audio + first * format.blockAlign;
        const uint8_t* secondFrame = audio + second * format.blockAlign;
        for (size_t channel = 0; channel < format.channels; channel++) {
            float a = readSample(firstFrame + channel * sampleBytes, format);
            float b = readSample(secondFrame + channel * sampleBytes, format);
            data->channel(channel)[frame] = a + (b - a) * fraction;
        }
    }
    return data;
}

} // namespace Starfish

#endif
