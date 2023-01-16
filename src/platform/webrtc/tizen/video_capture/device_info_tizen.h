/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#if defined(STARFISH_ENABLE_WEBRTC) && defined(STARFISH_TIZEN_PROD_TV)

#ifndef SRC_TIZEN_VIDEO_CAPTURE_DEVICE_INFO_TIZEN_H_
#define SRC_TIZEN_VIDEO_CAPTURE_DEVICE_INFO_TIZEN_H_

#include <camera.h>

#include "modules/video_capture/device_info_impl.h"

namespace webrtc {
namespace videocapturemodule {

    class DeviceInfoTizen : public DeviceInfoImpl {
    public:
        DeviceInfoTizen();
        ~DeviceInfoTizen() override;
        uint32_t NumberOfDevices() override;
        int32_t GetDeviceName(uint32_t deviceNumber, char* deviceNameUTF8,
                              uint32_t deviceNameLength,
                              char* deviceUniqueIdUTF8,
                              uint32_t deviceUniqueIdUTF8Length,
                              char* productUniqueIdUTF8 = 0,
                              uint32_t productUniqueIdUTF8Length = 0) override;
        /*
         * Fills the membervariable _captureCapabilities with capabilites for
         * the given device name.
         */
        int32_t CreateCapabilityMap(const char* deviceUniqueIdUTF8) override
            RTC_EXCLUSIVE_LOCKS_REQUIRED(_apiLock);
        int32_t Init() override;
        int32_t DisplayCaptureSettingsDialogBox(
            const char* /*deviceUniqueIdUTF8*/, const char* /*dialogTitleUTF8*/,
            void* /*parentWindow*/, uint32_t /*positionX*/,
            uint32_t /*positionY*/) override;
        int32_t GetBestMatchedCapability(
            const char* deviceUniqueIdUTF8,
            const VideoCaptureCapability& requested,
            VideoCaptureCapability& resulting) override;

    private:
        int32_t FillCapabilities(camera_h handle)
            RTC_EXCLUSIVE_LOCKS_REQUIRED(_apiLock);
    };

} // namespace videocapturemodule
} // namespace webrtc
#endif // SRC_TIZEN_VIDEO_CAPTURE_DEVICE_INFO_TIZEN_H_

#endif
