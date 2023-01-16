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

#ifndef SRC_TIZEN_AUDIO_DEVICE_AUDIO_INPUT_BUILTIN_TIZEN_H_
#define SRC_TIZEN_AUDIO_DEVICE_AUDIO_INPUT_BUILTIN_TIZEN_H_

#include <audio_io.h>

#include "audio_input_tizen.h"

namespace webrtc {
class AudioInputBuiltin : public AudioInput {
public:
    AudioInputBuiltin(uint8_t channels);
    virtual ~AudioInputBuiltin() override;
    virtual int32_t Open() override;
    int32_t Start() override;
    int32_t Stop() override;
    virtual void Close() override;
    uint32_t GetSampleRate() override;
    void SetAudioBuffer(AudioDeviceBuffer* audioBuffer) override;

protected:
    audio_in_h GetDeviceHandle();
    DeviceState state_;

private:
    void ReadAudioData();

    audio_in_h device_;
    const audio_sample_type_e sampleType_;
    const uint32_t sampleRate_;
    const uint8_t channels_;

    AudioDeviceBuffer* ptrAudioBuffer_;
    uint32_t recordingFramesIn10MS_;
    uint32_t recordingBufferSizeIn10MS_;
    int8_t* recordingBuffer_;
    uint32_t recordingFramesLeft_;
};
} // namespace webrtc
#endif // SRC_TIZEN_AUDIO_DEVICE_AUDIO_INPUT_BUILTIN_TIZEN_H_

#endif
