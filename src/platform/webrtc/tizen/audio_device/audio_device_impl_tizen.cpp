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

#include "audio_device_impl_tizen.h"

#include <stddef.h>

#include "api/scoped_refptr.h"
#include "audio_device_core_tizen.h"
#include "modules/audio_device/audio_device_config.h" // IWYU pragma: keep
#include "modules/audio_device/audio_device_generic.h"
#include "modules/audio_device/dummy/audio_device_dummy.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/ref_counted_object.h"
#include "system_wrappers/include/metrics.h"

#define CHECKinitialized_()  \
    {                        \
        if (!initialized_) { \
            return -1;       \
        }                    \
    }

#define CHECKinitialized__BOOL() \
    {                            \
        if (!initialized_) {     \
            return false;        \
        }                        \
    }

namespace webrtc {

rtc::scoped_refptr<AudioDeviceModule> AudioDeviceModuleImplTizen::Create(
    AudioLayer audio_layer, TaskQueueFactory* task_queue_factory,
    bool bypass_voice_processing)
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;
    return AudioDeviceModuleImplTizen::CreateForTest(
        audio_layer, task_queue_factory, bypass_voice_processing);
}

// static
rtc::scoped_refptr<AudioDeviceModuleForTest>
AudioDeviceModuleImplTizen::CreateForTest(AudioLayer audio_layer,
                                          TaskQueueFactory* task_queue_factory,
                                          bool bypass_voice_processing)
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;

    // The "AudioDeviceModule::kWindowsCoreAudio2" audio layer has its own
    // dedicated factory method which should be used instead.
    if (audio_layer == AudioDeviceModule::kWindowsCoreAudio2) {
        RTC_LOG(LS_ERROR)
            << "Use the CreateWindowsCoreAudioAudioDeviceModule() "
               "factory method instead for this option.";
        return nullptr;
    }

    // Create the generic reference counted (platform independent)
    // implementation.
    auto audio_device = rtc::make_ref_counted<AudioDeviceModuleImplTizen>(
        audio_layer, task_queue_factory, bypass_voice_processing);

    // Ensure that the current platform is supported.
    if (audio_device->CheckPlatform() == -1) {
        return nullptr;
    }

    // Create the platform-dependent implementation.
    if (audio_device->CreatePlatformSpecificObjects() == -1) {
        return nullptr;
    }

    // Ensure that the generic audio buffer can communicate with the platform
    // specific parts.
    if (audio_device->AttachAudioBuffer() == -1) {
        return nullptr;
    }

    return audio_device;
}

AudioDeviceModuleImplTizen::AudioDeviceModuleImplTizen(
    AudioLayer audio_layer, TaskQueueFactory* task_queue_factory,
    bool bypass_voice_processing)
    : audio_layer_(audio_layer)
    , audio_device_buffer_(task_queue_factory)
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;
}

int32_t AudioDeviceModuleImplTizen::CheckPlatform()
{
    RTC_DLOG(LS_INFO) << __FUNCTION__;
    // Ensure that the current platform is supported
    PlatformType platform(kPlatformNotSupported);
    platform = kPlatfromTizen;
    RTC_LOG(LS_INFO) << "current platform is Tizen";
    platform_type_ = platform;
    return 0;
}

int32_t AudioDeviceModuleImplTizen::CreatePlatformSpecificObjects()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    AudioLayer audio_layer(PlatformAudioLayer());
    if (audio_layer == kPlatformDefaultAudio) {
        audio_device_.reset(new AudioDeviceTizenCore());
        RTC_LOG(LS_INFO) << "Tizen OS APIs will be utilized.";
    }
    if (!audio_device_) {
        RTC_LOG(LS_ERROR)
            << "Failed to create the platform specific ADM implementation.";
        return -1;
    }
    return 0;
}

int32_t AudioDeviceModuleImplTizen::AttachAudioBuffer()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    audio_device_->AttachAudioBuffer(&audio_device_buffer_);
    return 0;
}

AudioDeviceModuleImplTizen::~AudioDeviceModuleImplTizen()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
}

int32_t AudioDeviceModuleImplTizen::ActiveAudioLayer(
    AudioLayer* audioLayer) const
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    AudioLayer activeAudio;
    if (audio_device_->ActiveAudioLayer(activeAudio) == -1) {
        return -1;
    }
    *audioLayer = activeAudio;
    return 0;
}

int32_t AudioDeviceModuleImplTizen::Init()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    if (initialized_)
        return 0;
    RTC_CHECK(audio_device_);
    AudioDeviceGeneric::InitStatus status = audio_device_->Init();
    RTC_HISTOGRAM_ENUMERATION(
        "WebRTC.Audio.InitializationResult", static_cast<int>(status),
        static_cast<int>(AudioDeviceGeneric::InitStatus::NUM_STATUSES));
    if (status != AudioDeviceGeneric::InitStatus::OK) {
        RTC_LOG(LS_ERROR) << "Audio device initialization failed.";
        return -1;
    }
    initialized_ = true;
    return 0;
}

int32_t AudioDeviceModuleImplTizen::Terminate()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    if (!initialized_)
        return 0;
    if (audio_device_->Terminate() == -1) {
        return -1;
    }
    initialized_ = false;
    return 0;
}

bool AudioDeviceModuleImplTizen::Initialized() const
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << ": " << initialized_;
    return initialized_;
}

int32_t AudioDeviceModuleImplTizen::InitSpeaker()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized_();
    return audio_device_->InitSpeaker();
}

int32_t AudioDeviceModuleImplTizen::InitMicrophone()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized_();
    return audio_device_->InitMicrophone();
}

int32_t AudioDeviceModuleImplTizen::SpeakerVolumeIsAvailable(bool* available)
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized_();
    bool isAvailable = false;
    if (audio_device_->SpeakerVolumeIsAvailable(isAvailable) == -1) {
        return -1;
    }
    *available = isAvailable;
    RTC_LOG(LS_INFO) << "output: " << isAvailable;
    return 0;
}

int32_t AudioDeviceModuleImplTizen::SetSpeakerVolume(uint32_t volume)
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << "(" << volume << ")";
    CHECKinitialized_();
    return audio_device_->SetSpeakerVolume(volume);
}

int32_t AudioDeviceModuleImplTizen::SpeakerVolume(uint32_t* volume) const
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized_();
    uint32_t level = 0;
    if (audio_device_->SpeakerVolume(level) == -1) {
        return -1;
    }
    *volume = level;
    RTC_LOG(LS_INFO) << "output: " << *volume;
    return 0;
}

bool AudioDeviceModuleImplTizen::SpeakerIsInitialized() const
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized__BOOL();
    bool isInitialized = audio_device_->SpeakerIsInitialized();
    RTC_LOG(LS_INFO) << "output: " << isInitialized;
    return isInitialized;
}

bool AudioDeviceModuleImplTizen::MicrophoneIsInitialized() const
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized__BOOL();
    bool isInitialized = audio_device_->MicrophoneIsInitialized();
    RTC_LOG(LS_INFO) << "output: " << isInitialized;
    return isInitialized;
}

int32_t AudioDeviceModuleImplTizen::MaxSpeakerVolume(uint32_t* maxVolume) const
{
    CHECKinitialized_();
    uint32_t maxVol = 0;
    if (audio_device_->MaxSpeakerVolume(maxVol) == -1) {
        return -1;
    }
    *maxVolume = maxVol;
    return 0;
}

int32_t AudioDeviceModuleImplTizen::MinSpeakerVolume(uint32_t* minVolume) const
{
    CHECKinitialized_();
    uint32_t minVol = 0;
    if (audio_device_->MinSpeakerVolume(minVol) == -1) {
        return -1;
    }
    *minVolume = minVol;
    return 0;
}

int32_t AudioDeviceModuleImplTizen::SpeakerMuteIsAvailable(bool* available)
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized_();
    bool isAvailable = false;
    if (audio_device_->SpeakerMuteIsAvailable(isAvailable) == -1) {
        return -1;
    }
    *available = isAvailable;
    RTC_LOG(LS_INFO) << "output: " << isAvailable;
    return 0;
}

int32_t AudioDeviceModuleImplTizen::SetSpeakerMute(bool enable)
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << "(" << enable << ")";
    CHECKinitialized_();
    return audio_device_->SetSpeakerMute(enable);
}

int32_t AudioDeviceModuleImplTizen::SpeakerMute(bool* enabled) const
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized_();
    bool muted = false;
    if (audio_device_->SpeakerMute(muted) == -1) {
        return -1;
    }
    *enabled = muted;
    RTC_LOG(LS_INFO) << "output: " << muted;
    return 0;
}

int32_t AudioDeviceModuleImplTizen::MicrophoneMuteIsAvailable(bool* available)
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized_();
    bool isAvailable = false;
    if (audio_device_->MicrophoneMuteIsAvailable(isAvailable) == -1) {
        return -1;
    }
    *available = isAvailable;
    RTC_LOG(LS_INFO) << "output: " << isAvailable;
    return 0;
}

int32_t AudioDeviceModuleImplTizen::SetMicrophoneMute(bool enable)
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << "(" << enable << ")";
    CHECKinitialized_();
    return (audio_device_->SetMicrophoneMute(enable));
}

int32_t AudioDeviceModuleImplTizen::MicrophoneMute(bool* enabled) const
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized_();
    bool muted = false;
    if (audio_device_->MicrophoneMute(muted) == -1) {
        return -1;
    }
    *enabled = muted;
    RTC_LOG(LS_INFO) << "output: " << muted;
    return 0;
}

int32_t AudioDeviceModuleImplTizen::MicrophoneVolumeIsAvailable(bool* available)
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized_();
    bool isAvailable = false;
    if (audio_device_->MicrophoneVolumeIsAvailable(isAvailable) == -1) {
        return -1;
    }
    *available = isAvailable;
    RTC_LOG(LS_INFO) << "output: " << isAvailable;
    return 0;
}

int32_t AudioDeviceModuleImplTizen::SetMicrophoneVolume(uint32_t volume)
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << "(" << volume << ")";
    CHECKinitialized_();
    return (audio_device_->SetMicrophoneVolume(volume));
}

int32_t AudioDeviceModuleImplTizen::MicrophoneVolume(uint32_t* volume) const
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized_();
    uint32_t level = 0;
    if (audio_device_->MicrophoneVolume(level) == -1) {
        return -1;
    }
    *volume = level;
    RTC_LOG(LS_INFO) << "output: " << *volume;
    return 0;
}

int32_t AudioDeviceModuleImplTizen::StereoRecordingIsAvailable(
    bool* available) const
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized_();
    bool isAvailable = false;
    if (audio_device_->StereoRecordingIsAvailable(isAvailable) == -1) {
        return -1;
    }
    *available = isAvailable;
    RTC_LOG(LS_INFO) << "output: " << isAvailable;
    return 0;
}

int32_t AudioDeviceModuleImplTizen::SetStereoRecording(bool enable)
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << "(" << enable << ")";
    CHECKinitialized_();
    if (audio_device_->RecordingIsInitialized()) {
        RTC_LOG(LS_ERROR)
            << "unable to set stereo mode after recording is initialized";
        return -1;
    }
    if (audio_device_->SetStereoRecording(enable) == -1) {
        if (enable) {
            RTC_LOG(LS_WARNING) << "failed to enable stereo recording";
        }
        return -1;
    }
    int8_t nChannels(1);
    if (enable) {
        nChannels = 2;
    }
    audio_device_buffer_.SetRecordingChannels(nChannels);
    return 0;
}

int32_t AudioDeviceModuleImplTizen::StereoRecording(bool* enabled) const
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized_();
    bool stereo = false;
    if (audio_device_->StereoRecording(stereo) == -1) {
        return -1;
    }
    *enabled = stereo;
    RTC_LOG(LS_INFO) << "output: " << stereo;
    return 0;
}

int32_t AudioDeviceModuleImplTizen::StereoPlayoutIsAvailable(
    bool* available) const
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized_();
    bool isAvailable = false;
    if (audio_device_->StereoPlayoutIsAvailable(isAvailable) == -1) {
        return -1;
    }
    *available = isAvailable;
    RTC_LOG(LS_INFO) << "output: " << isAvailable;
    return 0;
}

int32_t AudioDeviceModuleImplTizen::SetStereoPlayout(bool enable)
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << "(" << enable << ")";
    CHECKinitialized_();
    if (audio_device_->PlayoutIsInitialized()) {
        RTC_LOG(LS_ERROR)
            << "unable to set stereo mode while playing side is initialized";
        return -1;
    }
    if (audio_device_->SetStereoPlayout(enable)) {
        RTC_LOG(LS_WARNING) << "stereo playout is not supported";
        return -1;
    }
    int8_t nChannels(1);
    if (enable) {
        nChannels = 2;
    }
    audio_device_buffer_.SetPlayoutChannels(nChannels);
    return 0;
}

int32_t AudioDeviceModuleImplTizen::StereoPlayout(bool* enabled) const
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized_();
    bool stereo = false;
    if (audio_device_->StereoPlayout(stereo) == -1) {
        return -1;
    }
    *enabled = stereo;
    RTC_LOG(LS_INFO) << "output: " << stereo;
    return 0;
}

int32_t AudioDeviceModuleImplTizen::PlayoutIsAvailable(bool* available)
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized_();
    bool isAvailable = false;
    if (audio_device_->PlayoutIsAvailable(isAvailable) == -1) {
        return -1;
    }
    *available = isAvailable;
    RTC_LOG(LS_INFO) << "output: " << isAvailable;
    return 0;
}

int32_t AudioDeviceModuleImplTizen::RecordingIsAvailable(bool* available)
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized_();
    bool isAvailable = false;
    if (audio_device_->RecordingIsAvailable(isAvailable) == -1) {
        return -1;
    }
    *available = isAvailable;
    RTC_LOG(LS_INFO) << "output: " << isAvailable;
    return 0;
}

int32_t AudioDeviceModuleImplTizen::MaxMicrophoneVolume(
    uint32_t* maxVolume) const
{
    CHECKinitialized_();
    uint32_t maxVol(0);
    if (audio_device_->MaxMicrophoneVolume(maxVol) == -1) {
        return -1;
    }
    *maxVolume = maxVol;
    return 0;
}

int32_t AudioDeviceModuleImplTizen::MinMicrophoneVolume(
    uint32_t* minVolume) const
{
    CHECKinitialized_();
    uint32_t minVol(0);
    if (audio_device_->MinMicrophoneVolume(minVol) == -1) {
        return -1;
    }
    *minVolume = minVol;
    return 0;
}

int16_t AudioDeviceModuleImplTizen::PlayoutDevices()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized_();
    uint16_t nPlayoutDevices = audio_device_->PlayoutDevices();
    RTC_LOG(LS_INFO) << "output: " << nPlayoutDevices;
    return (int16_t)(nPlayoutDevices);
}

int32_t AudioDeviceModuleImplTizen::SetPlayoutDevice(uint16_t index)
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << "(" << index << ")";
    CHECKinitialized_();
    return audio_device_->SetPlayoutDevice(index);
}

int32_t AudioDeviceModuleImplTizen::SetPlayoutDevice(WindowsDeviceType device)
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized_();
    return audio_device_->SetPlayoutDevice(device);
}

int32_t AudioDeviceModuleImplTizen::PlayoutDeviceName(
    uint16_t index, char name[kAdmMaxDeviceNameSize],
    char guid[kAdmMaxGuidSize])
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << "(" << index << ", ...)";
    CHECKinitialized_();
    if (name == NULL) {
        return -1;
    }
    if (audio_device_->PlayoutDeviceName(index, name, guid) == -1) {
        return -1;
    }
    if (name != NULL) {
        RTC_LOG(LS_INFO) << "output: name = " << name;
    }
    if (guid != NULL) {
        RTC_LOG(LS_INFO) << "output: guid = " << guid;
    }
    return 0;
}

int32_t AudioDeviceModuleImplTizen::RecordingDeviceName(
    uint16_t index, char name[kAdmMaxDeviceNameSize],
    char guid[kAdmMaxGuidSize])
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << "(" << index << ", ...)";
    CHECKinitialized_();
    if (name == NULL) {
        return -1;
    }
    if (audio_device_->RecordingDeviceName(index, name, guid) == -1) {
        return -1;
    }
    if (name != NULL) {
        RTC_LOG(LS_INFO) << "output: name = " << name;
    }
    if (guid != NULL) {
        RTC_LOG(LS_INFO) << "output: guid = " << guid;
    }
    return 0;
}

int16_t AudioDeviceModuleImplTizen::RecordingDevices()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized_();
    uint16_t nRecordingDevices = audio_device_->RecordingDevices();
    RTC_LOG(LS_INFO) << "output: " << nRecordingDevices;
    return (int16_t)nRecordingDevices;
}

int32_t AudioDeviceModuleImplTizen::SetRecordingDevice(uint16_t index)
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << "(" << index << ")";
    CHECKinitialized_();
    return audio_device_->SetRecordingDevice(index);
}

int32_t AudioDeviceModuleImplTizen::SetRecordingDevice(WindowsDeviceType device)
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized_();
    return audio_device_->SetRecordingDevice(device);
}

int32_t AudioDeviceModuleImplTizen::InitPlayout()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized_();
    if (PlayoutIsInitialized()) {
        return 0;
    }
    int32_t result = audio_device_->InitPlayout();
    RTC_LOG(LS_INFO) << "output: " << result;
    RTC_HISTOGRAM_BOOLEAN("WebRTC.Audio.InitPlayoutSuccess",
                          static_cast<int>(result == 0));
    return result;
}

int32_t AudioDeviceModuleImplTizen::InitRecording()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized_();
    if (RecordingIsInitialized()) {
        return 0;
    }
    int32_t result = audio_device_->InitRecording();
    RTC_LOG(LS_INFO) << "output: " << result;
    RTC_HISTOGRAM_BOOLEAN("WebRTC.Audio.InitRecordingSuccess",
                          static_cast<int>(result == 0));
    return result;
}

bool AudioDeviceModuleImplTizen::PlayoutIsInitialized() const
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized__BOOL();
    return audio_device_->PlayoutIsInitialized();
}

bool AudioDeviceModuleImplTizen::RecordingIsInitialized() const
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized__BOOL();
    return audio_device_->RecordingIsInitialized();
}

int32_t AudioDeviceModuleImplTizen::StartPlayout()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized_();
    if (Playing()) {
        return 0;
    }
    audio_device_buffer_.StartPlayout();
    int32_t result = audio_device_->StartPlayout();
    RTC_LOG(LS_INFO) << "output: " << result;
    RTC_HISTOGRAM_BOOLEAN("WebRTC.Audio.StartPlayoutSuccess",
                          static_cast<int>(result == 0));
    return result;
}

int32_t AudioDeviceModuleImplTizen::StopPlayout()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized_();
    int32_t result = audio_device_->StopPlayout();
    audio_device_buffer_.StopPlayout();
    RTC_LOG(LS_INFO) << "output: " << result;
    RTC_HISTOGRAM_BOOLEAN("WebRTC.Audio.StopPlayoutSuccess",
                          static_cast<int>(result == 0));
    return result;
}

bool AudioDeviceModuleImplTizen::Playing() const
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized__BOOL();
    return audio_device_->Playing();
}

int32_t AudioDeviceModuleImplTizen::StartRecording()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized_();
    if (Recording()) {
        return 0;
    }
    audio_device_buffer_.StartRecording();
    int32_t result = audio_device_->StartRecording();
    RTC_LOG(LS_INFO) << "output: " << result;
    RTC_HISTOGRAM_BOOLEAN("WebRTC.Audio.StartRecordingSuccess",
                          static_cast<int>(result == 0));
    return result;
}

int32_t AudioDeviceModuleImplTizen::StopRecording()
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized_();
    int32_t result = audio_device_->StopRecording();
    audio_device_buffer_.StopRecording();
    RTC_LOG(LS_INFO) << "output: " << result;
    RTC_HISTOGRAM_BOOLEAN("WebRTC.Audio.StopRecordingSuccess",
                          static_cast<int>(result == 0));
    return result;
}

bool AudioDeviceModuleImplTizen::Recording() const
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized__BOOL();
    return audio_device_->Recording();
}

int32_t AudioDeviceModuleImplTizen::RegisterAudioCallback(
    AudioTransport* audioCallback)
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    return audio_device_buffer_.RegisterAudioCallback(audioCallback);
}

int32_t AudioDeviceModuleImplTizen::PlayoutDelay(uint16_t* delayMS) const
{
    CHECKinitialized_();
    uint16_t delay = 0;
    if (audio_device_->PlayoutDelay(delay) == -1) {
        RTC_LOG(LS_ERROR) << "failed to retrieve the playout delay";
        return -1;
    }
    *delayMS = delay;
    return 0;
}

bool AudioDeviceModuleImplTizen::BuiltInAECIsAvailable() const
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized__BOOL();
    bool isAvailable = audio_device_->BuiltInAECIsAvailable();
    RTC_LOG(LS_INFO) << "output: " << isAvailable;
    return isAvailable;
}

int32_t AudioDeviceModuleImplTizen::EnableBuiltInAEC(bool enable)
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << "(" << enable << ")";
    CHECKinitialized_();
    int32_t ok = audio_device_->EnableBuiltInAEC(enable);
    RTC_LOG(LS_INFO) << "output: " << ok;
    return ok;
}

bool AudioDeviceModuleImplTizen::BuiltInAGCIsAvailable() const
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized__BOOL();
    bool isAvailable = audio_device_->BuiltInAGCIsAvailable();
    RTC_LOG(LS_INFO) << "output: " << isAvailable;
    return isAvailable;
}

int32_t AudioDeviceModuleImplTizen::EnableBuiltInAGC(bool enable)
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << "(" << enable << ")";
    CHECKinitialized_();
    int32_t ok = audio_device_->EnableBuiltInAGC(enable);
    RTC_LOG(LS_INFO) << "output: " << ok;
    return ok;
}

bool AudioDeviceModuleImplTizen::BuiltInNSIsAvailable() const
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized__BOOL();
    bool isAvailable = audio_device_->BuiltInNSIsAvailable();
    RTC_LOG(LS_INFO) << "output: " << isAvailable;
    return isAvailable;
}

int32_t AudioDeviceModuleImplTizen::EnableBuiltInNS(bool enable)
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << "(" << enable << ")";
    CHECKinitialized_();
    int32_t ok = audio_device_->EnableBuiltInNS(enable);
    RTC_LOG(LS_INFO) << "output: " << ok;
    return ok;
}

int32_t AudioDeviceModuleImplTizen::GetPlayoutUnderrunCount() const
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    CHECKinitialized_();
    int32_t underrunCount = audio_device_->GetPlayoutUnderrunCount();
    RTC_LOG(LS_INFO) << "output: " << underrunCount;
    return underrunCount;
}

int32_t AudioDeviceModuleImplTizen::SetAudioDeviceSink(
    AudioDeviceSink* sink) const
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << "(" << sink << ")";
    int32_t ok = audio_device_->SetAudioDeviceSink(sink);
    RTC_LOG(LS_INFO) << "output: " << ok;
    return ok;
}

AudioDeviceModuleImplTizen::PlatformType AudioDeviceModuleImplTizen::Platform()
    const
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    return platform_type_;
}

AudioDeviceModule::AudioLayer AudioDeviceModuleImplTizen::PlatformAudioLayer()
    const
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    return audio_layer_;
}

} // namespace webrtc

#endif
