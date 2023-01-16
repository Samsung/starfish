
/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#if defined(STARFISH_ENABLE_WEBRTC) && defined(STARFISH_TIZEN_PROD_TV) && \
    defined(false)

#include "audio_utility_tizen.h"

#include "rtc_base/logging.h"
namespace webrtc {

audio_channel_e IntToAudioChannel(uint8_t channel)
{
    switch (channel) {
    case 1:
        return AUDIO_CHANNEL_MONO;
    case 2:
        return AUDIO_CHANNEL_STEREO;
    case 3:
        return AUDIO_CHANNEL_MULTI_3;
    case 4:
        return AUDIO_CHANNEL_MULTI_4;
    case 5:
        return AUDIO_CHANNEL_MULTI_5;
    case 6:
        return AUDIO_CHANNEL_MULTI_6;
    case 7:
        return AUDIO_CHANNEL_MULTI_7;
    case 8:
        return AUDIO_CHANNEL_MULTI_8;
    default:
        RTC_LOG(LS_ERROR) << "Unsupported audio channel";
        return AUDIO_CHANNEL_MONO;
    }
}

uint8_t AudioChannelToInt(audio_channel_e channel)
{
    switch (channel) {
    case AUDIO_CHANNEL_MONO:
        return 1;
    case AUDIO_CHANNEL_STEREO:
        return 2;
    case AUDIO_CHANNEL_MULTI_3:
        return 3;
    case AUDIO_CHANNEL_MULTI_4:
        return 4;
    case AUDIO_CHANNEL_MULTI_5:
        return 5;
    case AUDIO_CHANNEL_MULTI_6:
        return 6;
    case AUDIO_CHANNEL_MULTI_7:
        return 7;
    case AUDIO_CHANNEL_MULTI_8:
        return 8;
    default:
        RTC_LOG(LS_ERROR) << "Unsupported audio channel " << channel;
        return 0;
    }
}

int SampleTypeToBytes(audio_sample_type_e type)
{
    switch (type) {
    case AUDIO_SAMPLE_TYPE_U8:
        return 1;
    case AUDIO_SAMPLE_TYPE_S16_LE:
        return 2;
    case AUDIO_SAMPLE_TYPE_S24_LE:
        return 3;
    case AUDIO_SAMPLE_TYPE_S24_32_LE:
    case AUDIO_SAMPLE_TYPE_S32_LE:
        return 4;
    }

    RTC_LOG(LS_ERROR) << "Unsupported sample type: " << type;
    return 0;
}

int FramesToBufferSize(int frames, audio_sample_type_e type, int channels)
{
    return frames * SampleTypeToBytes(type) * channels;
}

int BufferSizeToFrames(int size, audio_sample_type_e type, int channels)
{
    return size / (channels * SampleTypeToBytes(type));
}

} // namespace webrtc

#endif
