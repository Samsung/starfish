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

#ifndef SRC_TIZEN_AUDIO_DEVICE_AUDIO_DEVICE_CORE_TIZEN_H_
#define SRC_TIZEN_AUDIO_DEVICE_AUDIO_DEVICE_CORE_TIZEN_H_

#include <audio_io.h>

#include "audio_input_tizen.h"
#include "audio_output_tizen.h"
#include "modules/audio_device/audio_device_generic.h"

namespace webrtc {

struct DeviceInfo {
    int id;
    sound_device_type_e type;
    std::string name;
};

class AudioDeviceTizenCore : public AudioDeviceGeneric {
public:
    AudioDeviceTizenCore();
    ~AudioDeviceTizenCore();
    // Retrieve the currently utilized audio layer
    virtual int32_t ActiveAudioLayer(
        AudioDeviceModule::AudioLayer& audioLayer) const override;

    // Main initializaton and termination
    virtual InitStatus Init() override;
    virtual int32_t Terminate() override;
    virtual bool Initialized() const override;

    // Device enumeration
    virtual int16_t PlayoutDevices() override;
    virtual int16_t RecordingDevices() override;
    virtual int32_t PlayoutDeviceName(uint16_t index,
                                      char name[kAdmMaxDeviceNameSize],
                                      char guid[kAdmMaxGuidSize]) override;
    virtual int32_t RecordingDeviceName(uint16_t index,
                                        char name[kAdmMaxDeviceNameSize],
                                        char guid[kAdmMaxGuidSize]) override;

    // Device selection
    virtual int32_t SetPlayoutDevice(uint16_t index) override;
    virtual int32_t SetPlayoutDevice(
        AudioDeviceModule::WindowsDeviceType device) override;
    virtual int32_t SetRecordingDevice(uint16_t index) override;
    virtual int32_t SetRecordingDevice(
        AudioDeviceModule::WindowsDeviceType device) override;

    // Audio transport initialization
    virtual int32_t PlayoutIsAvailable(bool& available) override;
    virtual int32_t InitPlayout() override;
    virtual bool PlayoutIsInitialized() const override;
    virtual int32_t RecordingIsAvailable(bool& available) override;
    virtual int32_t InitRecording() override;
    virtual bool RecordingIsInitialized() const override;

    // Audio transport control
    virtual int32_t StartPlayout() override;
    virtual int32_t StopPlayout() override;
    virtual bool Playing() const override;
    virtual int32_t StartRecording() override;
    virtual int32_t StopRecording() override;
    virtual bool Recording() const override;

    // Audio mixer initialization
    virtual int32_t InitSpeaker() override;
    virtual bool SpeakerIsInitialized() const override;
    virtual int32_t InitMicrophone() override;
    virtual bool MicrophoneIsInitialized() const override;

    // Speaker volume controls
    virtual int32_t SpeakerVolumeIsAvailable(bool& available) override;
    virtual int32_t SetSpeakerVolume(uint32_t volume) override;
    virtual int32_t SpeakerVolume(uint32_t& volume) const override;
    virtual int32_t MaxSpeakerVolume(uint32_t& maxVolume) const override;
    virtual int32_t MinSpeakerVolume(uint32_t& minVolume) const override;

    // Microphone volume controls
    virtual int32_t MicrophoneVolumeIsAvailable(bool& available) override;
    virtual int32_t SetMicrophoneVolume(uint32_t volume) override;
    virtual int32_t MicrophoneVolume(uint32_t& volume) const override;
    virtual int32_t MaxMicrophoneVolume(uint32_t& maxVolume) const override;
    virtual int32_t MinMicrophoneVolume(uint32_t& minVolume) const override;

    // Speaker mute control
    virtual int32_t SpeakerMuteIsAvailable(bool& available) override;
    virtual int32_t SetSpeakerMute(bool enable) override;
    virtual int32_t SpeakerMute(bool& enabled) const override;

    // Microphone mute control
    virtual int32_t MicrophoneMuteIsAvailable(bool& available) override;
    virtual int32_t SetMicrophoneMute(bool enable) override;
    virtual int32_t MicrophoneMute(bool& enabled) const override;

    // Stereo support
    virtual int32_t StereoPlayoutIsAvailable(bool& available) override;
    virtual int32_t SetStereoPlayout(bool enable) override;
    virtual int32_t StereoPlayout(bool& enabled) const override;
    virtual int32_t StereoRecordingIsAvailable(bool& available) override;
    virtual int32_t SetStereoRecording(bool enable) override;
    virtual int32_t StereoRecording(bool& enabled) const override;

    // Delay information and control
    virtual int32_t PlayoutDelay(uint16_t& delayMS) const override;
    virtual void AttachAudioBuffer(AudioDeviceBuffer* audioBuffer) override;

    int32_t SetAudioDeviceSink(AudioDeviceSink* sink) override
    {
        return -1;
    }

private:
    bool GetAudioDevices(bool playout, std::vector<DeviceInfo>& devices);
    int32_t InitAuidoInput();
    int32_t InitAudioOutput();
    int32_t DeviceName(DeviceInfo& info, char name[kAdmMaxDeviceNameSize],
                       char guid[kAdmMaxGuidSize]);

    bool initialized_;
    bool playing_;
    bool recording_;
    int outputDevieId_;
    int inputDeviceId_;
    bool playIsInitialized_;
    bool recordingIsInitialized_;
    uint8_t playChannels_;
    uint8_t recordingChannels_;
    AudioDeviceBuffer* ptrAudioBuffer_;
    std::unique_ptr<AudioOutput> audioOutput_;
    std::unique_ptr<AudioInput> audioInput_;
};

} // namespace webrtc

#endif // SRC_TIZEN_AUDIO_DEVICE_AUDIO_DEVICE_CORE_TIZEN_H_

#endif
