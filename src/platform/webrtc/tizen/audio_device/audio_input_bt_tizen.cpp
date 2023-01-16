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

#include "audio_input_bt_tizen.h"

namespace webrtc {
AudioInputBt::AudioInputBt()
{
}

AudioInputBt::~AudioInputBt()
{
    Close();
}

int32_t AudioInputBt::Open()
{
    return -1;
}

int32_t AudioInputBt::Start()
{
    return -1;
}

int32_t AudioInputBt::Stop()
{
    return -1;
}

void AudioInputBt::Close()
{
}

uint32_t AudioInputBt::GetSampleRate()
{
    return 0;
}

void AudioInputBt::SetAudioBuffer(AudioDeviceBuffer* audioBuffer)
{
}
} // namespace webrtc

#endif
