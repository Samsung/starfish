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

#include "audio_device_core_tizen.h"

#include <sound_manager.h>

#include "audio_input_bt_tizen.h"
#include "audio_input_builtin_tizen.h"
#include "audio_input_usb_tizen.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"

namespace webrtc {
AudioDeviceTizenCore::AudioDeviceTizenCore()
    : initialized_(false)
    , playing_(false)
    , recording_(false)
    , outputDevieId_(-1)
    , inputDeviceId_(-1)
    , playIsInitialized_(false)
    , recordingIsInitialized_(false)
    , playChannels_(1)
    , recordingChannels_(1)
    , ptrAudioBuffer_(nullptr)
{
}

AudioDeviceTizenCore::~AudioDeviceTizenCore()
{
}

// Retrieve the currently utilized audio layer
int32_t AudioDeviceTizenCore::ActiveAudioLayer(
    AudioDeviceModule::AudioLayer& audioLayer) const
{
    audioLayer = AudioDeviceModule::kPlatformDefaultAudio;
    return 0;
}

// Main initializaton and termination
AudioDeviceTizenCore::InitStatus AudioDeviceTizenCore::Init()
{
    initialized_ = true;
    return AudioDeviceTizenCore::InitStatus::OK;
}

int32_t AudioDeviceTizenCore::Terminate()
{
    initialized_ = false;
    return 0;
}

bool AudioDeviceTizenCore::Initialized() const
{
    return initialized_;
}

// Device enumeration
int16_t AudioDeviceTizenCore::PlayoutDevices()
{
    std::vector<DeviceInfo> devices;
    bool result = GetAudioDevices(true, devices);
    if (!result) {
        return -1;
    }

    return devices.size();
}

int16_t AudioDeviceTizenCore::RecordingDevices()
{
    std::vector<DeviceInfo> devices;
    bool result = GetAudioDevices(false, devices);
    if (!result) {
        return -1;
    }
    return devices.size();
}

int32_t AudioDeviceTizenCore::PlayoutDeviceName(
    uint16_t index, char name[kAdmMaxDeviceNameSize],
    char guid[kAdmMaxGuidSize])
{
    std::vector<DeviceInfo> devices;
    bool result = GetAudioDevices(true, devices);
    if (!result || (index >= devices.size())) {
        return -1;
    }

    return DeviceName(devices[index], name, guid);
}

int32_t AudioDeviceTizenCore::RecordingDeviceName(
    uint16_t index, char name[kAdmMaxDeviceNameSize],
    char guid[kAdmMaxGuidSize])
{
    std::vector<DeviceInfo> devices;
    bool result = GetAudioDevices(false, devices);
    if (!result || (index >= devices.size())) {
        return -1;
    }

    return DeviceName(devices[index], name, guid);
}

// Device selection
int32_t AudioDeviceTizenCore::SetPlayoutDevice(uint16_t index)
{
    std::vector<DeviceInfo> devices;
    bool result = GetAudioDevices(true, devices);
    if (!result || (index >= devices.size())) {
        return -1;
    }

    outputDevieId_ = devices[index].id;
    return 0;
}

int32_t AudioDeviceTizenCore::SetPlayoutDevice(
    AudioDeviceModule::WindowsDeviceType device)
{
    RTC_LOG(LS_ERROR) << "WindowsDeviceType not supported";
    return -1;
}

int32_t AudioDeviceTizenCore::SetRecordingDevice(uint16_t index)
{
    std::vector<DeviceInfo> devices;
    bool result = GetAudioDevices(false, devices);
    if (!result || (index >= devices.size())) {
        return -1;
    }

    inputDeviceId_ = devices[index].id;
    return 0;
}

int32_t AudioDeviceTizenCore::SetRecordingDevice(
    AudioDeviceModule::WindowsDeviceType device)
{
    RTC_LOG(LS_ERROR) << "WindowsDeviceType not supported";
    return -1;
}

// Audio transport initialization
int32_t AudioDeviceTizenCore::PlayoutIsAvailable(bool& available)
{
    available = false;

    // Try to initialize the playout side with mono
    // Assumes that user set num channels after calling this function
    playChannels_ = 1;
    int32_t result = InitPlayout();

    // Cancel effect of initialization
    StopPlayout();

    if (result != -1) {
        available = true;
    } else {
        // It may be possible to play out in stereo
        result = StereoPlayoutIsAvailable(available);
        if (available) {
            // Then set channels to 2 so InitPlayout doesn't fail
            playChannels_ = 2;
        }
    }

    return result;
}

int32_t AudioDeviceTizenCore::InitPlayout()
{
    if (playing_) {
        return -1;
    }

    if (playIsInitialized_) {
        return 0;
    }

    int32_t result = InitAudioOutput();
    if (result != 0) {
        RTC_LOG(LS_ERROR) << "Failed to init audio output";
        return result;
    }

    if (ptrAudioBuffer_) {
        audioOutput_->SetAudioBuffer(ptrAudioBuffer_);
        ptrAudioBuffer_->SetPlayoutSampleRate(audioOutput_->GetSampleRate());
        ptrAudioBuffer_->SetPlayoutChannels(playChannels_);
    }

    playIsInitialized_ = true;
    return 0;
}

bool AudioDeviceTizenCore::PlayoutIsInitialized() const
{
    return playIsInitialized_;
}

int32_t AudioDeviceTizenCore::RecordingIsAvailable(bool& available)
{
    available = false;
    // Try to initialize the playout side
    int32_t result = InitRecording();

    // Cancel effect of initialization
    StopRecording();

    if (result != -1) {
        available = true;
    }

    return result;
}

int32_t AudioDeviceTizenCore::InitRecording()
{
    if (recording_) {
        return -1;
    }

    if (recordingIsInitialized_) {
        return 0;
    }

    int32_t result = InitAuidoInput();
    if (result != 0) {
        RTC_LOG(LS_WARNING) << "InitMicrophone() failed";
        return result;
    }

    if (ptrAudioBuffer_) {
        audioInput_->SetAudioBuffer(ptrAudioBuffer_);
        ptrAudioBuffer_->SetRecordingSampleRate(audioInput_->GetSampleRate());
        ptrAudioBuffer_->SetRecordingChannels(recordingChannels_);
    }

    // Mark recording side as initialized
    recordingIsInitialized_ = true;
    return 0;
}

bool AudioDeviceTizenCore::RecordingIsInitialized() const
{
    return recordingIsInitialized_;
}

// Audio transport control
int32_t AudioDeviceTizenCore::StartPlayout()
{
    if (!playIsInitialized_) {
        return -1;
    }

    if (playing_) {
        return 0;
    }

    int32_t result = audioOutput_->Start();
    if (result != -1) {
        playing_ = true;
    }
    return result;
}

int32_t AudioDeviceTizenCore::StopPlayout()
{
    if (!playIsInitialized_) {
        return 0;
    }

    if (!audioOutput_) {
        return -1;
    }

    playing_ = false;
    playIsInitialized_ = false;
    audioOutput_.reset();
    return 0;
}

bool AudioDeviceTizenCore::Playing() const
{
    return playing_;
}

int32_t AudioDeviceTizenCore::StartRecording()
{
    if (!recordingIsInitialized_) {
        return -1;
    }

    if (recording_) {
        return 0;
    }

    int32_t result = audioInput_->Start();
    if (result != -1) {
        recording_ = true;
    }
    return result;
}

int32_t AudioDeviceTizenCore::StopRecording()
{
    if (!recordingIsInitialized_) {
        return 0;
    }

    if (!audioInput_) {
        return -1;
    }

    recordingIsInitialized_ = false;
    recording_ = false;
    audioInput_.reset();
    return 0;
}

bool AudioDeviceTizenCore::Recording() const
{
    return recording_;
}

// Audio mixer initialization
int32_t AudioDeviceTizenCore::InitSpeaker()
{
    return 0;
}

bool AudioDeviceTizenCore::SpeakerIsInitialized() const
{
    return true;
}

int32_t AudioDeviceTizenCore::InitMicrophone()
{
    return 0;
}

bool AudioDeviceTizenCore::MicrophoneIsInitialized() const
{
    return true;
}

// Speaker volume controls
int32_t AudioDeviceTizenCore::SpeakerVolumeIsAvailable(bool& available)
{
    available = false;
    return 0;
}
int32_t AudioDeviceTizenCore::SetSpeakerVolume(uint32_t volume)
{
    return 0;
}

int32_t AudioDeviceTizenCore::SpeakerVolume(uint32_t& volume) const
{
    return 0;
}

int32_t AudioDeviceTizenCore::MaxSpeakerVolume(uint32_t& maxVolume) const
{
    return 0;
}

int32_t AudioDeviceTizenCore::MinSpeakerVolume(uint32_t& minVolume) const
{
    return 0;
}

// Microphone volume controls
int32_t AudioDeviceTizenCore::MicrophoneVolumeIsAvailable(bool& available)
{
    available = false;
    return 0;
}

int32_t AudioDeviceTizenCore::SetMicrophoneVolume(uint32_t volume)
{
    return 0;
}

int32_t AudioDeviceTizenCore::MicrophoneVolume(uint32_t& volume) const
{
    return 0;
}

int32_t AudioDeviceTizenCore::MaxMicrophoneVolume(uint32_t& maxVolume) const
{
    return 0;
}

int32_t AudioDeviceTizenCore::MinMicrophoneVolume(uint32_t& minVolume) const
{
    return 0;
}

// Speaker mute control
int32_t AudioDeviceTizenCore::SpeakerMuteIsAvailable(bool& available)
{
    available = false;
    return 0;
}

int32_t AudioDeviceTizenCore::SetSpeakerMute(bool enable)
{
    return 0;
}

int32_t AudioDeviceTizenCore::SpeakerMute(bool& enabled) const
{
    return 0;
}

// Microphone mute control
int32_t AudioDeviceTizenCore::MicrophoneMuteIsAvailable(bool& available)
{
    available = false;
    return 0;
}

int32_t AudioDeviceTizenCore::SetMicrophoneMute(bool enable)
{
    return 0;
}

int32_t AudioDeviceTizenCore::MicrophoneMute(bool& enabled) const
{
    return 0;
}

// Stereo support
int32_t AudioDeviceTizenCore::StereoPlayoutIsAvailable(bool& available)
{
    if (playIsInitialized_ && (2 == playChannels_)) {
        available = true;
        return 0;
    }

    bool playIsInitialized = playIsInitialized_;
    bool playing = playing_;
    int playChannels = playChannels_;

    available = false;

    // Stop/uninitialize playing if initialized
    if (playIsInitialized_) {
        StopPlayout();
    }

    // Try init in stereo;
    playChannels_ = 2;
    if (InitPlayout() == 0) {
        available = true;
    }
    StopPlayout();

    // Recover previous states
    playChannels_ = playChannels;
    if (playIsInitialized) {
        InitPlayout();
    }
    if (playing) {
        StartPlayout();
    }

    return 0;
}

int32_t AudioDeviceTizenCore::SetStereoPlayout(bool enable)
{
    if (enable)
        playChannels_ = 2;
    else
        playChannels_ = 1;

    return 0;
}

int32_t AudioDeviceTizenCore::StereoPlayout(bool& enabled) const
{
    if (playChannels_ == 2)
        enabled = true;
    else
        enabled = false;

    return 0;
}

int32_t AudioDeviceTizenCore::StereoRecordingIsAvailable(bool& available)
{
    // If we already have initialized in stereo it's obviously available
    if (recordingIsInitialized_ && (2 == recordingChannels_)) {
        available = true;
        return 0;
    }

    // Save rec states and the number of rec channels
    bool recordingIsInitialized = recordingIsInitialized_;
    bool recording = recording_;
    int recordingChannels = recordingChannels_;

    available = false;

    // Stop/uninitialize recording if initialized (and possibly started)
    if (recordingIsInitialized_) {
        StopRecording();
    }

    // Try init in stereo;
    recordingChannels_ = 2;
    if (InitRecording() == 0) {
        available = true;
    }

    // Stop/uninitialize recording
    StopRecording();

    // Recover previous states
    recordingChannels_ = recordingChannels;
    if (recordingIsInitialized) {
        InitRecording();
    }
    if (recording) {
        StartRecording();
    }

    return 0;
}

int32_t AudioDeviceTizenCore::SetStereoRecording(bool enable)
{
    if (enable)
        recordingChannels_ = 2;
    else
        recordingChannels_ = 1;

    return 0;
}

int32_t AudioDeviceTizenCore::StereoRecording(bool& enabled) const
{
    if (recordingChannels_ == 2)
        enabled = true;
    else
        enabled = false;
    return 0;
}

// Delay information and control
int32_t AudioDeviceTizenCore::PlayoutDelay(uint16_t& delayMS) const
{
    return 0;
}

void AudioDeviceTizenCore::AttachAudioBuffer(AudioDeviceBuffer* audioBuffer)
{
    ptrAudioBuffer_ = audioBuffer;
    // Inform the AudioBuffer about default settings for this implementation.
    // Set all values to zero here since the actual settings will be done by
    // InitPlayout and InitRecording later.
    ptrAudioBuffer_->SetRecordingSampleRate(0);
    ptrAudioBuffer_->SetPlayoutSampleRate(0);
    ptrAudioBuffer_->SetRecordingChannels(0);
    ptrAudioBuffer_->SetPlayoutChannels(0);
}

// ============================================================================
//                                 Private Methods
// ============================================================================

bool AudioDeviceTizenCore::GetAudioDevices(bool playout,
                                           std::vector<DeviceInfo>& devices)
{
    sound_device_list_h list;
    int result = sound_manager_get_device_list(
        playout ? SOUND_DEVICE_IO_DIRECTION_OUT_MASK
                : SOUND_DEVICE_IO_DIRECTION_IN_MASK,
        &list);
    if (result == SOUND_MANAGER_ERROR_NO_DATA) {
        RTC_LOG(LS_INFO) << "No sound device";
        return true;
    }
    if (result != SOUND_MANAGER_ERROR_NONE) {
        RTC_LOG(LS_ERROR) << "Failed to get sound device, error: "
                          << get_error_message(result);
        return false;
    }

    sound_device_h device;
    while (SOUND_MANAGER_ERROR_NONE ==
           sound_manager_get_next_device(list, &device)) {
        int id;
        char* name = nullptr;
        sound_device_type_e type;
        result = sound_manager_get_device_id(device, &id);
        if (result != SOUND_MANAGER_ERROR_NONE) {
            RTC_LOG(LS_ERROR) << "Failed to get device id. error: "
                              << get_error_message(result);
            continue;
        }
        sound_manager_get_device_name(device, &name);
        sound_manager_get_device_type(device, &type);

        DeviceInfo deviceInfo;
        deviceInfo.id = id;
        deviceInfo.type = type;
        deviceInfo.name = (name == nullptr ? "" : name);
        devices.push_back(deviceInfo);
    }

    sound_manager_free_device_list(list);
    return true;
}

int32_t AudioDeviceTizenCore::DeviceName(DeviceInfo& info,
                                         char name[kAdmMaxDeviceNameSize],
                                         char guid[kAdmMaxGuidSize])
{
    if (name == nullptr || info.name.length() == 0) {
        return -1;
    }

    if (info.name.length() >= kAdmMaxDeviceNameSize) {
        RTC_LOG(LS_ERROR) << "Device name is too long";
        return -1;
    }

    memset(name, 0, kAdmMaxDeviceNameSize);
    memcpy(name, info.name.c_str(), info.name.length());

    if (guid != nullptr) {
        memset(guid, 0, kAdmMaxGuidSize);
        sprintf(guid, "%d", info.id);
    }
    return 0;
}

int32_t AudioDeviceTizenCore::InitAuidoInput()
{
    if (inputDeviceId_ == -1) {
        RTC_LOG(LS_ERROR) << "Invalid id of audio input.";
        return -1;
    }

    if (audioInput_ != nullptr) {
        audioInput_.reset();
    }

    std::vector<DeviceInfo> devices;
    bool result = GetAudioDevices(false, devices);
    if (!result) {
        RTC_LOG(LS_ERROR) << "Fail to get audio devices";
        return -1;
    }

    bool found = false;
    sound_device_type_e type;
    for (DeviceInfo deviceInfo : devices) {
        if (inputDeviceId_ == deviceInfo.id) {
            type = deviceInfo.type;
            found = true;
            break;
        }
    }
    if (!found) {
        RTC_LOG(LS_ERROR) << "Cannot find audio input " << inputDeviceId_;
        return -1;
    }

    if (type == SOUND_DEVICE_BUILTIN_MIC) {
        audioInput_ = std::make_unique<AudioInputBuiltin>(recordingChannels_);
    } else if (type == SOUND_DEVICE_USB_AUDIO) {
        audioInput_ =
            std::make_unique<AudioInputUsb>(inputDeviceId_, recordingChannels_);
    } else {
        RTC_LOG(LS_ERROR) << "Unsupported type of audio input: " << type;
        return -1;
    }

    return audioInput_->Open();
}

int32_t AudioDeviceTizenCore::InitAudioOutput()
{
    if (audioOutput_ != nullptr) {
        audioOutput_.reset();
    }

    audioOutput_ = std::make_unique<AudioOutput>(playChannels_);
    return audioOutput_->Open();
}

} // namespace webrtc
#endif
