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

#ifndef SRC_TIZEN_VIDEO_CAPTURE_VIDEO_CAPTURE_TIZEN_H_
#define SRC_TIZEN_VIDEO_CAPTURE_VIDEO_CAPTURE_TIZEN_H_

#include <camera.h>

#include "modules/video_capture/video_capture_defines.h"
#include "video_capture_impl_tizen.h"

namespace webrtc {
namespace videocapturemodule {
    class VideoCaptureModuleTizen : public VideoCaptureImplTizen {
    public:
        VideoCaptureModuleTizen();
        ~VideoCaptureModuleTizen() override;
        int32_t Init(const char* deviceUniqueId);
        int32_t StartCapture(const VideoCaptureCapability& capability) override;
        int32_t StopCapture() override;
        bool CaptureStarted() override;
        int32_t CaptureSettings(VideoCaptureCapability& settings) override;

    private:
        enum CaptureType {
            kNone,
            kMediaPacket,
            kFrame,
        };

        static void OnCapturedWithMediaPacket(media_packet_h packet,
                                              void* data);
        static void OnCaptured(camera_preview_data_s* frame, void* data);

        camera_h device_;
        int32_t width_;
        int32_t height_;
        int32_t rate_;
        VideoType videoType_;
        bool capturing_;
        CaptureType captureType_;
    };
} // namespace videocapturemodule
} // namespace webrtc

#endif // SRC_TIZEN_VIDEO_CAPTURE_VIDEO_CAPTURE_TIZEN_H_

#endif
