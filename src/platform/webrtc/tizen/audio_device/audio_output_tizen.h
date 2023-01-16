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

#ifndef SRC_TIZEN_AUDIO_DEVICE_AUDIO_OUTPUT_TIZEN_H_
#define SRC_TIZEN_AUDIO_DEVICE_AUDIO_OUTPUT_TIZEN_H_

#include <audio_io.h>

#include "modules/audio_device/audio_device_buffer.h"

namespace webrtc {

class AudioOutput {
public:
    enum DeviceState {
        kOpen,
        kPlaying,
        kClosed,
    };

    AudioOutput(uint8_t channels);
    ~AudioOutput();
    int32_t Open();
    int32_t Start();
    int32_t Stop();
    void Close();
    uint32_t GetSampleRate();
    void SetAudioBuffer(AudioDeviceBuffer* buffer);

private:
    void WriteAudioData(size_t nbytes);

    audio_out_h device_;
    DeviceState state_;
    const audio_sample_type_e sampleType_;
    const uint32_t sampleRate_;
    const uint8_t channels_;

    AudioDeviceBuffer* ptrAudioBuffer_;
    uint32_t playoutFramesIn10MS_;
    uint32_t playoutBufferSizeIn10MS_;
    int8_t* playoutBuffer_;
    uint32_t playoutFramesLeft_;
};

} // namespace webrtc

#endif // SRC_TIZEN_AUDIO_DEVICE_AUDIO_OUTPUT_TIZEN_H_

#endif
