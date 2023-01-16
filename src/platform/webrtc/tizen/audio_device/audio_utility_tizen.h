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

#ifndef SRC_TIZEN_AUDIO_DEVICE_AUDIO_UTILITY_TIZEN_H_
#define SRC_TIZEN_AUDIO_DEVICE_AUDIO_UTILITY_TIZEN_H_

#include <audio_io.h>
#include <stdint.h>

namespace webrtc {
audio_channel_e IntToAudioChannel(uint8_t channel);
uint8_t AudioChannelToInt(audio_channel_e channel);

int SampleTypeToBytes(audio_sample_type_e type);
int FramesToBufferSize(int frames, audio_sample_type_e type, int channels);
int BufferSizeToFrames(int size, audio_sample_type_e type, int channels);
} // namespace webrtc

#endif // SRC_TIZEN_AUDIO_DEVICE_AUDIO_UTILITY_TIZEN_H_

#endif
