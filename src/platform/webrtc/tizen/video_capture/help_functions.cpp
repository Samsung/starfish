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

#include "help_functions.h"

namespace webrtc {
namespace videocapturemodule {
    const char* GetCameraName(camera_device_e device)
    {
        switch (device) {
            ENUM_CASE(CAMERA_DEVICE_CAMERA0);
            ENUM_CASE(CAMERA_DEVICE_CAMERA1);
            ENUM_CASE(CAMERA_DEVICE_CAMERA2);
            ENUM_CASE(CAMERA_DEVICE_CAMERA3);
            ENUM_CASE(CAMERA_DEVICE_CAMERA4);
            ENUM_CASE(CAMERA_DEVICE_CAMERA5);
            ENUM_CASE(CAMERA_DEVICE_CAMERA6);
            ENUM_CASE(CAMERA_DEVICE_CAMERA7);
            ENUM_CASE(CAMERA_DEVICE_CAMERA8);
            ENUM_CASE(CAMERA_DEVICE_CAMERA9);
        default:
            break;
        }
        return "Unknown";
    }

    const char* GetCameraId(camera_device_e device)
    {
        return GetCameraName(device);
    }

    VideoType CameraPixelFormatToVideoType(camera_pixel_format_e format)
    {
        switch (format) {
        case CAMERA_PIXEL_FORMAT_ARGB:
            return VideoType::kARGB;
        case CAMERA_PIXEL_FORMAT_RGB565:
            return VideoType::kRGB565;
        case CAMERA_PIXEL_FORMAT_YV12:
            return VideoType::kYV12;
        case CAMERA_PIXEL_FORMAT_UYVY:
            return VideoType::kUYVY;
        case CAMERA_PIXEL_FORMAT_MJPEG:
            return VideoType::kMJPEG;
        case CAMERA_PIXEL_FORMAT_YUYV:
            return VideoType::kYUY2;
        case CAMERA_PIXEL_FORMAT_NV12:
            return VideoType::kI420;
        default:
            return VideoType::kUnknown;
        }
    }

    camera_pixel_format_e VideoTypeToCameraPixelFormat(VideoType type)
    {
        switch (type) {
        case VideoType::kI420:
            return CAMERA_PIXEL_FORMAT_NV12;
        case VideoType::kARGB:
            return CAMERA_PIXEL_FORMAT_ARGB;
        case VideoType::kRGB565:
            return CAMERA_PIXEL_FORMAT_RGB565;
        case VideoType::kYV12:
            return CAMERA_PIXEL_FORMAT_YV12;
        case VideoType::kUYVY:
            return CAMERA_PIXEL_FORMAT_UYVY;
        case VideoType::kMJPEG:
            return CAMERA_PIXEL_FORMAT_MJPEG;
        case VideoType::kYUY2:
            return CAMERA_PIXEL_FORMAT_YUYV;
        default:
            return CAMERA_PIXEL_FORMAT_INVALID;
        }
    }

    int32_t CameraAttrFpsToInt(camera_attr_fps_e fps)
    {
        switch (fps) {
        case CAMERA_ATTR_FPS_7:
            return 7;
        case CAMERA_ATTR_FPS_8:
            return 8;
        case CAMERA_ATTR_FPS_15:
            return 15;
        case CAMERA_ATTR_FPS_20:
            return 20;
        case CAMERA_ATTR_FPS_24:
            return 24;
        case CAMERA_ATTR_FPS_25:
            return 25;
        case CAMERA_ATTR_FPS_30:
            return 30;
        case CAMERA_ATTR_FPS_60:
            return 60;
        case CAMERA_ATTR_FPS_90:
            return 90;
        case CAMERA_ATTR_FPS_120:
            return 120;
        default:
            return 0;
        }
    }

    camera_attr_fps_e IntToCameraAttrFps(int fps)
    {
        switch (fps) {
        case 7:
            return CAMERA_ATTR_FPS_7;
        case 8:
            return CAMERA_ATTR_FPS_8;
        case 15:
            return CAMERA_ATTR_FPS_15;
        case 20:
            return CAMERA_ATTR_FPS_20;
        case 24:
            return CAMERA_ATTR_FPS_24;
        case 25:
            return CAMERA_ATTR_FPS_25;
        case 30:
            return CAMERA_ATTR_FPS_30;
        case 60:
            return CAMERA_ATTR_FPS_60;
        case 90:
            return CAMERA_ATTR_FPS_90;
        case 120:
            return CAMERA_ATTR_FPS_120;
        default:
            break;
        }
        return CAMERA_ATTR_FPS_AUTO;
    }

    const char* CameraPixelFormatToString(camera_pixel_format_e format)
    {
        switch (format) {
            ENUM_CASE(CAMERA_PIXEL_FORMAT_INVALID);
            ENUM_CASE(CAMERA_PIXEL_FORMAT_NV12);
            ENUM_CASE(CAMERA_PIXEL_FORMAT_NV12T);
            ENUM_CASE(CAMERA_PIXEL_FORMAT_NV16);
            ENUM_CASE(CAMERA_PIXEL_FORMAT_NV21);
            ENUM_CASE(CAMERA_PIXEL_FORMAT_YUYV);
            ENUM_CASE(CAMERA_PIXEL_FORMAT_UYVY);
            ENUM_CASE(CAMERA_PIXEL_FORMAT_422P);
            ENUM_CASE(CAMERA_PIXEL_FORMAT_I420);
            ENUM_CASE(CAMERA_PIXEL_FORMAT_YV12);
            ENUM_CASE(CAMERA_PIXEL_FORMAT_RGB565);
            ENUM_CASE(CAMERA_PIXEL_FORMAT_RGB888);
            ENUM_CASE(CAMERA_PIXEL_FORMAT_RGBA);
            ENUM_CASE(CAMERA_PIXEL_FORMAT_ARGB);
            ENUM_CASE(CAMERA_PIXEL_FORMAT_JPEG);
            ENUM_CASE(CAMERA_PIXEL_FORMAT_H264);
            ENUM_CASE(CAMERA_PIXEL_FORMAT_INVZ);
            ENUM_CASE(CAMERA_PIXEL_FORMAT_MJPEG);
        default:
            break;
        }
        return "Unknown";
    }

    const char* VideoTypeToString(VideoType type)
    {
        switch (type) {
            ENUM_CASE(VideoType::kUnknown);
            ENUM_CASE(VideoType::kI420);
            ENUM_CASE(VideoType::kIYUV);
            ENUM_CASE(VideoType::kRGB24);
            ENUM_CASE(VideoType::kARGB);
            ENUM_CASE(VideoType::kRGB565);
            ENUM_CASE(VideoType::kYUY2);
            ENUM_CASE(VideoType::kYV12);
            ENUM_CASE(VideoType::kUYVY);
            ENUM_CASE(VideoType::kMJPEG);
            ENUM_CASE(VideoType::kBGRA);
        default:
            break;
        }
        return "Unknown";
    }

} // namespace videocapturemodule
} // namespace webrtc

#endif
