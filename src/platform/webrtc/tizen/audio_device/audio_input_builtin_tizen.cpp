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

#include "audio_input_builtin_tizen.h"

#include "audio_utility_tizen.h"
#include "rtc_base/logging.h"

namespace webrtc {

AudioInputBuiltin::AudioInputBuiltin(uint8_t channels)
    : state_(kClosed)
    , device_(nullptr)
    , sampleType_(AUDIO_SAMPLE_TYPE_S16_LE)
    , sampleRate_(48000)
    , channels_(channels)
    , ptrAudioBuffer_(nullptr)
    , recordingFramesIn10MS_(0)
    , recordingBufferSizeIn10MS_(0)
    , recordingBuffer_(nullptr)
    , recordingFramesLeft_(0)
{
}

AudioInputBuiltin::~AudioInputBuiltin()
{
    Close();
}

int32_t AudioInputBuiltin::Open()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    if (state_ == kOpen || state_ == kRecording) {
        RTC_LOG(LS_INFO) << "Audio input is already opened";
        return 0;
    }

    if (!GetDeviceHandle()) {
        return -1;
    }

    state_ = kOpen;
    return 0;
}

int32_t AudioInputBuiltin::Start()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    if (state_ == kClosed) {
        RTC_LOG(LS_ERROR) << "Audio input is not opened";
        return -1;
    }

    if (state_ == kRecording) {
        RTC_LOG(LS_INFO) << "Audio input is already recording";
        return 0;
    }

    recordingFramesLeft_ = recordingFramesIn10MS_;
    if (!recordingBuffer_) {
        recordingBuffer_ = new int8_t[recordingBufferSizeIn10MS_];
    }
    if (!recordingBuffer_) {
        RTC_LOG(LS_ERROR) << "Failed to alloc recording buf";
        return -1;
    }

    int result = audio_in_set_stream_cb(
        device_,
        [](audio_in_h handle, size_t nbytes, void* user_data) {
            AudioInputBuiltin* input =
                static_cast<AudioInputBuiltin*>(user_data);
            input->ReadAudioData();
        },
        this);
    if (result != AUDIO_IO_ERROR_NONE) {
        RTC_LOG(LS_ERROR) << "audio_in_set_stream_cb() failed, error: "
                          << get_error_message(result);
        return -1;
    }

    result = audio_in_prepare(device_);
    if (result != AUDIO_IO_ERROR_NONE) {
        RTC_LOG(LS_ERROR) << "audio_in_prepare() failed, error: "
                          << get_error_message(result);
        audio_in_unset_stream_cb(device_);
        return -1;
    }

    state_ = kRecording;
    return 0;
}

int32_t AudioInputBuiltin::Stop()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    if (state_ != kRecording) {
        RTC_LOG(LS_INFO) << "Audio input is not recording";
        return 0;
    }

    int result = audio_in_unprepare(device_);
    if (result != AUDIO_IO_ERROR_NONE) {
        RTC_LOG(LS_ERROR) << "audio_in_unprepare() failed, error: "
                          << get_error_message(result);
        return -1;
    }

    result = audio_in_unset_stream_cb(device_);
    if (result != AUDIO_IO_ERROR_NONE) {
        RTC_LOG(LS_ERROR) << "audio_in_unset_stream_cb() failed, error: "
                          << get_error_message(result);
    }

    recordingFramesLeft_ = 0;
    if (recordingBuffer_) {
        delete[] recordingBuffer_;
        recordingBuffer_ = nullptr;
    }
    state_ = kOpen;

    return 0;
}

void AudioInputBuiltin::Close()
{
    if (state_ == kClosed) {
        RTC_LOG(LS_INFO) << "Audio input is already closed";
        return;
    }

    if (state_ == kRecording) {
        Stop();
    }

    if (device_) {
        int result = audio_in_destroy(device_);
        if (result != AUDIO_IO_ERROR_NONE) {
            RTC_LOG(LS_ERROR) << "audio_in_destroy() failed, error: "
                              << get_error_message(result);
        }
        device_ = nullptr;
    }
    state_ = kClosed;
}

uint32_t AudioInputBuiltin::GetSampleRate()
{
    return sampleRate_;
}

void AudioInputBuiltin::SetAudioBuffer(AudioDeviceBuffer* audioBuffer)
{
    ptrAudioBuffer_ = audioBuffer;
}

audio_in_h AudioInputBuiltin::GetDeviceHandle()
{
    if (device_) {
        return device_;
    }

    int result = audio_in_create(sampleRate_, IntToAudioChannel(channels_),
                                 sampleType_, &device_);
    if (result != AUDIO_IO_ERROR_NONE) {
        RTC_LOG(LS_ERROR) << "audio_in_create(rate: " << sampleRate_
                          << ", channels: " << channels_
                          << ") failed, error: " << get_error_message(result);
        return nullptr;
    }

    recordingFramesIn10MS_ = sampleRate_ / 100;
    recordingBufferSizeIn10MS_ =
        SampleTypeToBytes(sampleType_) * recordingFramesIn10MS_ * channels_;
    return device_;
}

void AudioInputBuiltin::ReadAudioData()
{
    if (!ptrAudioBuffer_) {
        RTC_LOG(LS_ERROR) << "Audio buffer is invalid";
        return;
    }

    unsigned int bytes_read = 0;
    const void* loc_buff = nullptr;
    int result = audio_in_peek(device_, &loc_buff, &bytes_read);
    if (result != AUDIO_IO_ERROR_NONE) {
        RTC_LOG(LS_ERROR) << "audio_in_peek() failed, error: "
                          << get_error_message(result);
        return;
    }

    int number_of_frames =
        BufferSizeToFrames(bytes_read, sampleType_, channels_);
    int offset = 0;
    while ((number_of_frames + recordingFramesLeft_) >=
           recordingFramesIn10MS_) {
        int left_size =
            FramesToBufferSize(recordingFramesLeft_, sampleType_, channels_);
        int copy_size = recordingBufferSizeIn10MS_ - left_size;
        memcpy(&recordingBuffer_[left_size], (int8_t*)loc_buff + offset,
               copy_size);
        number_of_frames =
            number_of_frames - (recordingFramesIn10MS_ - recordingFramesLeft_);
        offset += (recordingFramesIn10MS_ - recordingFramesLeft_) * channels_ *
                  SampleTypeToBytes(sampleType_);
        recordingFramesLeft_ = 0;
        ptrAudioBuffer_->SetRecordedBuffer(recordingBuffer_,
                                           recordingFramesIn10MS_);
        ptrAudioBuffer_->SetVQEData(10 * 1000 / sampleRate_,
                                    10 * 1000 / sampleRate_);
        ptrAudioBuffer_->DeliverRecordedData();
    }
    if (number_of_frames > 0) {
        int left_size =
            FramesToBufferSize(recordingFramesLeft_, sampleType_, channels_);
        int copy_size =
            FramesToBufferSize(number_of_frames, sampleType_, channels_);
        memcpy(&recordingBuffer_[left_size], (int8_t*)loc_buff + offset,
               copy_size);
        recordingFramesLeft_ += number_of_frames;
    }
    result = audio_in_drop(device_);
    if (result != AUDIO_IO_ERROR_NONE) {
        RTC_LOG(LS_ERROR) << "audio_in_drop() failed, error: "
                          << get_error_message(result);
    }
}

} // namespace webrtc

#endif
