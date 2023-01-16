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

#ifndef SRC_TIZEN_AUDIO_DEVICE_AUDIO_INPUT_TIZEN_H_
#define SRC_TIZEN_AUDIO_DEVICE_AUDIO_INPUT_TIZEN_H_

#include "modules/audio_device/audio_device_buffer.h"

namespace webrtc {
class AudioInput {
public:
    enum DeviceState {
        kOpen,
        kRecording,
        kClosed,
    };

    AudioInput(){};
    virtual ~AudioInput(){};
    virtual int32_t Open() = 0;
    virtual int32_t Start() = 0;
    virtual int32_t Stop() = 0;
    virtual void Close() = 0;
    virtual uint32_t GetSampleRate() = 0;
    virtual void SetAudioBuffer(AudioDeviceBuffer* audioBuffer) = 0;
};
} // namespace webrtc
#endif // SRC_TIZEN_AUDIO_DEVICE_AUDIO_INPUT_TIZEN_H_

#endif
