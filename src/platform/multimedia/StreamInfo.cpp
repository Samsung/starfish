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

#ifdef STARFISH_ENABLE_MULTIMEDIA

#include "StarFishConfig.h"
#include "platform/multimedia/StreamInfo.h"

namespace StarFish {

MediaCodec suggestAudioCodecFromString(String* name)
{
    if (name->startsWith("vorbis")) {
        return MediaCodecAudioVorbis;
    }
    if (name->startsWith("mp3")) {
        return MediaCodecAudioMP3;
    }
    if (name->startsWith("aac") || name->startsWith("mp4a.40")) {
        return MediaCodecAudioAAC;
    }
    return MediaCodecUnknown;
}

MediaCodec suggestVideoCodecFromString(String* name)
{
    if (name->startsWith("vp9")) {
        return MediaCodecVideoVP9;
    }
    if (name->startsWith("avc1") || name->startsWith("avc3")) {
        return MediaCodecVideoH264;
    }
    if (name->startsWith("hev1") || name->startsWith("hvc1")) {
        return MediaCodecVideoHEVC;
    }
    return MediaCodecUnknown;
}

const char* mediaCodecToString(MediaCodec codec)
{
    switch (codec) {
    case MediaCodecAudioVorbis:
        return "vorbis";
    case MediaCodecAudioAAC:
        return "aac";
    case MediaCodecAudioMP3:
        return "mp3";
    case MediaCodecVideoVP9:
        return "vp9";
    case MediaCodecVideoH264:
        return "h264";
    case MediaCodecVideoHEVC:
        return "hevc";
    default:
        break;
    }
    return "unknown";
}

StreamInfo::StreamInfo()
    : m_streamIndex(SIZE_MAX)
    , m_timescale(1)
    , m_rawDuration(0)
    , m_mediaTime(0)
    , m_codec(MediaCodecUnknown)
    , m_type(StreamTypeUnknown)
{
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            STARFISH_LOG_INFO("StreamInfo::~StreamInfo\n");
            StreamInfo* self = (StreamInfo*)obj;
            std::vector<uint8_t>().swap(self->m_extraData);
        },
        NULL, NULL, NULL);
}

static int64_t inline avGCD(int64_t a, int64_t b)
{
    if (b) {
        return avGCD(b, a % b);
    }
    return a;
}

// chromium-efl (mp4_stream_parser.cc)
Framerate Framerate::createFromLL(int64_t num, int64_t den)
{
    const int64_t max = 30000;
    int64_t a0num = 0;
    int64_t a0den = 1;
    int64_t a1num = 1;
    int64_t a1den = 0;
    int sign = (num < 0) ^ (den < 0);
    int64_t gcd = avGCD(std::llabs(num), std::llabs(den));

    if (gcd) {
        num = std::llabs(num) / gcd;
        den = std::llabs(den) / gcd;
    }
    if (num <= max && den <= max) {
        a1num = num;
        a1den = den;
        den = 0;
    }

    while (den) {
        uint64_t x = num / den;
        int64_t next_den = num - den * x;
        int64_t a2n = x * a1num + a0num;
        int64_t a2d = x * a1den + a0den;

        if (a2n > max || a2d > max) {
            if (a1num) {
                x = (max - a0num) / a1num;
            }
            if (a1den) {
                x = std::min((int64_t)x, (max - a0den) / a1den);
            }
            if (den * (2 * (int64_t)x * a1den + a0den) > num * a1den) {
                a1num = x * a1num + a0num;
                a1den = x * a1den + a0den;
            }
            break;
        }

        a0num = a1num;
        a0den = a1den;
        a1num = a2n;
        a1den = a2d;
        num = den;
        den = next_den;
    }

    return { (int)(sign ? -a1num : a1num), (int)a1den };
}
}
#endif /* STARFISH_ENABLE_MULTIMEDIA */
