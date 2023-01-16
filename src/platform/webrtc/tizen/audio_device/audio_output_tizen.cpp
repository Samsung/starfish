/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#if defined(STARFISH_ENABLE_WEBRTC) && defined(STARFISH_TIZEN_PROD_TV) && \
    defined(false)

#include "audio_output_tizen.h"

#include "audio_utility_tizen.h"
#include "rtc_base/logging.h"

namespace webrtc {

AudioOutput::AudioOutput(uint8_t channels)
    : device_(nullptr)
    , state_(kClosed)
    , sampleType_(AUDIO_SAMPLE_TYPE_S16_LE)
    , sampleRate_(44100)
    , channels_(channels)
    , ptrAudioBuffer_(nullptr)
    , playoutFramesIn10MS_(0)
    , playoutBufferSizeIn10MS_(0)
    , playoutBuffer_(nullptr)
    , playoutFramesLeft_(0)
{
}

AudioOutput::~AudioOutput()
{
    Close();
}

int32_t AudioOutput::Open()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    if (state_ == kOpen || state_ == kPlaying) {
        RTC_LOG(LS_INFO) << "Audio output is already opened";
        return 0;
    }

    int result = audio_out_create_new(sampleRate_, IntToAudioChannel(channels_),
                                      sampleType_, &device_);
    if (result != AUDIO_IO_ERROR_NONE) {
        RTC_LOG(LS_ERROR) << "audio_out_create_new(rate: " << sampleRate_
                          << ", channels: " << channels_
                          << ") failed, error: " << get_error_message(result);
        return -1;
    }

    state_ = kOpen;
    playoutFramesIn10MS_ = sampleRate_ / 100;
    playoutBufferSizeIn10MS_ =
        FramesToBufferSize(playoutFramesIn10MS_, sampleType_, channels_);

    return 0;
}

int32_t AudioOutput::Start()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    if (state_ == kClosed) {
        RTC_LOG(LS_ERROR) << "Audio output is not opened";
        return -1;
    }

    if (state_ == kPlaying) {
        RTC_LOG(LS_INFO) << "Audio output is already playing";
        return 0;
    }

    playoutFramesLeft_ = 0;
    if (!playoutBuffer_) {
        playoutBuffer_ = new int8_t[playoutBufferSizeIn10MS_];
    }
    if (!playoutBuffer_) {
        RTC_LOG(LS_ERROR) << "Failed to alloc playout buf";
        return -1;
    }

    int result = audio_out_set_stream_cb(
        device_,
        [](audio_out_h handle, size_t nbytes, void* user_data) {
            AudioOutput* output = static_cast<AudioOutput*>(user_data);
            output->WriteAudioData(nbytes);
        },
        this);
    if (result != AUDIO_IO_ERROR_NONE) {
        RTC_LOG(LS_ERROR) << "audio_out_set_stream_cb() failed, error: "
                          << get_error_message(result);
        return -1;
    }

    result = audio_out_prepare(device_);
    if (result != AUDIO_IO_ERROR_NONE) {
        RTC_LOG(LS_ERROR) << "audio_out_prepare() failed, error: "
                          << get_error_message(result);
        audio_out_unset_stream_cb(device_);
        return -1;
    }

    state_ = kPlaying;
    return 0;
}

int32_t AudioOutput::Stop()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    if (state_ != kPlaying) {
        RTC_LOG(LS_INFO) << "Audio output is not playing";
        return 0;
    }

    int result = audio_out_unprepare(device_);
    if (result != AUDIO_IO_ERROR_NONE) {
        RTC_LOG(LS_ERROR) << "audio_out_unprepare() failed, error: "
                          << get_error_message(result);
        return -1;
    }

    result = audio_out_unset_stream_cb(device_);
    if (result != AUDIO_IO_ERROR_NONE) {
        RTC_LOG(LS_ERROR) << "audio_out_unset_stream_cb() failed, error: "
                          << get_error_message(result);
    }

    playoutFramesLeft_ = 0;
    if (playoutBuffer_) {
        delete[] playoutBuffer_;
        playoutBuffer_ = nullptr;
    }
    state_ = kOpen;

    return 0;
}

void AudioOutput::Close()
{
    if (state_ == kClosed) {
        RTC_LOG(LS_INFO) << "Audio output is already closed";
        return;
    }

    if (state_ == kPlaying) {
        Stop();
    }

    if (device_) {
        int result = audio_out_destroy(device_);
        if (result != AUDIO_IO_ERROR_NONE) {
            RTC_LOG(LS_ERROR) << "audio_out_destroy() failed, error: "
                              << get_error_message(result);
        }
        device_ = nullptr;
    }
    state_ = kClosed;
}

uint32_t AudioOutput::GetSampleRate()
{
    return sampleRate_;
}

void AudioOutput::SetAudioBuffer(AudioDeviceBuffer* buffer)
{
    ptrAudioBuffer_ = buffer;
}

void AudioOutput::WriteAudioData(size_t nbytes)
{
    if (!ptrAudioBuffer_) {
        RTC_LOG(LS_ERROR) << "Audio buffer is invalid";
        return;
    }

    uint32_t framesRemaining =
        BufferSizeToFrames(nbytes, sampleType_, channels_);
    while (framesRemaining > 0) {
        if (playoutFramesLeft_ <= 0) {
            ptrAudioBuffer_->RequestPlayoutData(playoutFramesIn10MS_);
            playoutFramesLeft_ =
                ptrAudioBuffer_->GetPlayoutData(playoutBuffer_);
            RTC_DCHECK_EQ(playoutFramesLeft_, playoutFramesIn10MS_);
        }

        uint32_t framesWritten = (framesRemaining < playoutFramesLeft_)
                                     ? framesRemaining
                                     : playoutFramesLeft_;
        uint32_t size =
            FramesToBufferSize(playoutFramesLeft_, sampleType_, channels_);
        size_t bytesWritten =
            FramesToBufferSize(framesWritten, sampleType_, channels_);
        int result = audio_out_write(
            device_, &playoutBuffer_[playoutBufferSizeIn10MS_ - size],
            bytesWritten);
        if (result < 0) {
            RTC_LOG(LS_ERROR) << "audio_out_write() failed, error: "
                              << get_error_message(result);
            continue;
        }

        RTC_DCHECK_EQ(result, bytesWritten);
        playoutFramesLeft_ -= framesWritten;
        framesRemaining -= framesWritten;
    }
}

} // namespace webrtc

#endif
