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

#ifndef SRC_TIZEN_AUDIO_DEVICE_AUDIO_INPUT_BT_TIZEN_H_
#define SRC_TIZEN_AUDIO_DEVICE_AUDIO_INPUT_BT_TIZEN_H_

#include "audio_input_tizen.h"

namespace webrtc {
class AudioInputBt : public AudioInput {
public:
    AudioInputBt();
    ~AudioInputBt() override;
    int32_t Open() override;
    int32_t Start() override;
    int32_t Stop() override;
    void Close() override;
    uint32_t GetSampleRate() override;
    void SetAudioBuffer(AudioDeviceBuffer* audioBuffer) override;
};
} // namespace webrtc
#endif // SRC_TIZEN_AUDIO_DEVICE_AUDIO_INPUT_BT_TIZEN_H_

#endif
