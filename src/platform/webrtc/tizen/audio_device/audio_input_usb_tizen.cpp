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

#include "audio_input_usb_tizen.h"

#include "audio_utility_tizen.h"
#include "rtc_base/logging.h"

namespace webrtc {
AudioInputUsb::AudioInputUsb(int id, uint8_t channel)
    : AudioInputBuiltin(channel)
    , id_(id)
    , device_added_for_stream_routing_(false)
    , deviceList_(nullptr)
    , streamInfo_(nullptr)
    , soundDevice_(nullptr)
{
}

AudioInputUsb::~AudioInputUsb()
{
    Close();
}

int32_t AudioInputUsb::Open()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    if (state_ == kOpen || state_ == kRecording) {
        RTC_LOG(LS_INFO) << "Audio input is already opened";
        return 0;
    }

    int result = sound_manager_create_stream_information(
        SOUND_STREAM_TYPE_MEDIA_EXTERNAL_ONLY, nullptr, nullptr, &streamInfo_);
    if (result != SOUND_MANAGER_ERROR_NONE) {
        RTC_LOG(LS_ERROR)
            << "sound_manager_create_stream_information() failed, error: "
            << get_error_message(result);
        return -1;
    }

    result = sound_manager_get_device_list(SOUND_DEVICE_IO_DIRECTION_IN_MASK,
                                           &deviceList_);
    if (result != SOUND_MANAGER_ERROR_NONE) {
        RTC_LOG(LS_ERROR) << "sound_manager_get_device_list() falied, error: "
                          << get_error_message(result);
        Close();
        return -1;
    }

    sound_device_h device;
    while (!sound_manager_get_next_device(deviceList_, &device)) {
        int id;
        if (sound_manager_get_device_id(device, &id)) {
            RTC_LOG(LS_ERROR) << "Failed to get device Id";
            continue;
        }
        if (id_ == id) {
            soundDevice_ = device;
            break;
        }
    }
    if (soundDevice_ == nullptr) {
        RTC_LOG(LS_ERROR)
            << "Sound device with the mentioned ID does not exist";
        Close();
        return -1;
    }

    result =
        sound_manager_add_device_for_stream_routing(streamInfo_, soundDevice_);
    if (result != SOUND_MANAGER_ERROR_NONE) {
        RTC_LOG(LS_ERROR)
            << "sound_manager_add_device_for_stream_routing() failed, error: "
            << get_error_message(result);
        Close();
        return -1;
    }

    result = sound_manager_apply_stream_routing(streamInfo_);
    if (result != SOUND_MANAGER_ERROR_NONE) {
        RTC_LOG(LS_ERROR)
            << "sound_manager_apply_stream_routing() failed, error: "
            << get_error_message(result);
        Close();
        return -1;
    }
    device_added_for_stream_routing_ = true;

    audio_in_h handle = GetDeviceHandle();
    if (!handle) {
        Close();
        return -1;
    }

    result = audio_in_set_sound_stream_info(handle, streamInfo_);
    if (result != SOUND_MANAGER_ERROR_NONE) {
        RTC_LOG(LS_ERROR) << "audio_in_set_sound_stream_info() failed, error: "
                          << get_error_message(result);
        Close();
        return -1;
    }

    state_ = kOpen;
    return 0;
}

void AudioInputUsb::Close()
{
    if (state_ == kClosed) {
        RTC_LOG(LS_INFO) << "Audio input is already closed";
        return;
    }

    if (state_ == kRecording) {
        Stop();
    }

    int result = 0;
    if (device_added_for_stream_routing_) {
        result = sound_manager_remove_device_for_stream_routing(streamInfo_,
                                                                soundDevice_);
        if (result != SOUND_MANAGER_ERROR_NONE) {
            RTC_LOG(LS_ERROR) << "sound_manager_remove_device_for_stream_"
                                 "routing() failed, error: "
                              << get_error_message(result);
        }
        device_added_for_stream_routing_ = false;
    }
    soundDevice_ = nullptr;

    if (deviceList_) {
        result = sound_manager_free_device_list(deviceList_);
        if (result != SOUND_MANAGER_ERROR_NONE) {
            RTC_LOG(LS_ERROR)
                << "sound_manager_free_device_list() failed, error: "
                << get_error_message(result);
        }
        deviceList_ = nullptr;
    }

    if (streamInfo_) {
        result = sound_manager_destroy_stream_information(streamInfo_);
        if (result != SOUND_MANAGER_ERROR_NONE) {
            RTC_LOG(LS_ERROR)
                << "sound_manager_destroy_stream_information() failed, error: "
                << get_error_message(result);
        }
        streamInfo_ = nullptr;
    }

    AudioInputBuiltin::Close();
}
} // namespace webrtc

#endif
