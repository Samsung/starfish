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

#ifndef SRC_TIZEN_VIDEO_CAPTURE_HELP_FUNCTIONS_H_
#define SRC_TIZEN_VIDEO_CAPTURE_HELP_FUNCTIONS_H_

#include <camera.h>

#include "modules/video_capture/video_capture_defines.h"

#define ENUM_CASE(x) \
    case x:          \
        return #x;

namespace webrtc {
namespace videocapturemodule {
    const char* GetCameraName(camera_device_e device);
    const char* GetCameraId(camera_device_e device);

    VideoType CameraPixelFormatToVideoType(camera_pixel_format_e format);
    camera_pixel_format_e VideoTypeToCameraPixelFormat(VideoType type);
    int32_t CameraAttrFpsToInt(camera_attr_fps_e fps);
    camera_attr_fps_e IntToCameraAttrFps(int fps);

    const char* CameraPixelFormatToString(camera_pixel_format_e format);
    const char* VideoTypeToString(VideoType type);
} // namespace videocapturemodule
} // namespace webrtc
#endif // SRC_TIZEN_VIDEO_CAPTURE_HELP_FUNCTIONS_H_

#endif
