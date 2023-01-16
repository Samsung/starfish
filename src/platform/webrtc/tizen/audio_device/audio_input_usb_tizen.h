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

#ifndef SRC_TIZEN_AUDIO_DEVICE_AUDIO_INPUT_USB_TIZEN_H_
#define SRC_TIZEN_AUDIO_DEVICE_AUDIO_INPUT_USB_TIZEN_H_

#include "audio_input_builtin_tizen.h"

namespace webrtc {
class AudioInputUsb : public AudioInputBuiltin {
public:
    AudioInputUsb(int id, uint8_t channel);
    ~AudioInputUsb();
    int32_t Open() override;
    void Close() override;

private:
    const int id_;
    bool device_added_for_stream_routing_;
    sound_device_list_h deviceList_;
    sound_stream_info_h streamInfo_;
    sound_device_h soundDevice_;
};
} // namespace webrtc
#endif // SRC_TIZEN_AUDIO_DEVICE_AUDIO_INPUT_USB_TIZEN_H_

#endif
