
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

#include "video_capture_module_tizen.h"

#include <media_packet.h>

#include <map>

#include "api/scoped_refptr.h"
#include "help_functions.h"
#include "media/base/video_common.h"
#include "modules/video_capture/video_capture.h"
#include "rtc_base/logging.h"
#include "rtc_base/ref_counted_object.h"

namespace webrtc {
namespace videocapturemodule {

    VideoCaptureModuleTizen::VideoCaptureModuleTizen()
        : device_(nullptr)
        , width_(0)
        , height_(0)
        , rate_(0)
        , videoType_(VideoType::kI420)
        , capturing_(false)
        , captureType_(kNone)
    {
    }

    int32_t VideoCaptureModuleTizen::Init(const char* deviceUniqueIdUTF8)
    {
        bool found = false;
        int length = strlen((const char*)deviceUniqueIdUTF8);
        _deviceUniqueId = new (std::nothrow) char[length + 1];
        if (_deviceUniqueId) {
            memcpy(_deviceUniqueId, deviceUniqueIdUTF8, length + 1);
        }

        std::vector<camera_device_e> devices = {
            CAMERA_DEVICE_CAMERA0, CAMERA_DEVICE_CAMERA1, CAMERA_DEVICE_CAMERA2,
            CAMERA_DEVICE_CAMERA3, CAMERA_DEVICE_CAMERA4, CAMERA_DEVICE_CAMERA5,
            CAMERA_DEVICE_CAMERA6, CAMERA_DEVICE_CAMERA7, CAMERA_DEVICE_CAMERA8,
            CAMERA_DEVICE_CAMERA9
        };
        camera_h handle = nullptr;
        for (auto device : devices) {
            if (camera_create(device, &handle) == CAMERA_ERROR_NONE) {
                if (strncmp(GetCameraId(device), deviceUniqueIdUTF8, length) ==
                    0) {
                    found = true;
                    break;
                }
                camera_destroy(handle);
            }
        }

        if (!found) {
            RTC_LOG(LS_ERROR) << "No matching device found";
            return -1;
        }
        device_ = handle;
        return 0;
    }

    VideoCaptureModuleTizen::~VideoCaptureModuleTizen()
    {
        StopCapture();
        if (device_) {
            camera_destroy(device_);
        }
    }

    int32_t VideoCaptureModuleTizen::StartCapture(
        const VideoCaptureCapability& capability)
    {
        if (device_ == nullptr) {
            RTC_LOG(LS_ERROR) << "camera is not created";
            return -1;
        }

        if (capturing_) {
            if (capability.width == width_ && capability.height == height_ &&
                capability.videoType == videoType_) {
                return 0;
            } else {
                if (StopCapture() != 0) {
                    RTC_LOG(LS_ERROR) << "Cannot stop capturing";
                    return -1;
                }
            }
        }

        int result = CAMERA_ERROR_NONE;
        result = camera_set_preview_resolution(device_, capability.width,
                                               capability.height);
        if (result != CAMERA_ERROR_NONE) {
            RTC_LOG(LS_ERROR)
                << "Cannot set video size(" << capability.width << ", "
                << capability.height
                << ") to camera, error: " << get_error_message(result);
            return -1;
        }
        width_ = capability.width;
        height_ = capability.height;

        camera_pixel_format_e format =
            VideoTypeToCameraPixelFormat(capability.videoType);
        if (format != CAMERA_PIXEL_FORMAT_INVALID) {
            result = camera_set_preview_format(device_, format);
            if (result != CAMERA_ERROR_NONE) {
                RTC_LOG(LS_ERROR)
                    << "Cannot set format " << CameraPixelFormatToString(format)
                    << " to camera, error: " << get_error_message(result);
                return -1;
            }
        } else {
            RTC_LOG(LS_ERROR) << "Unsupported video type "
                              << VideoTypeToString(capability.videoType);
            return -1;
        }
        videoType_ = capability.videoType;

        camera_attr_fps_e fps = IntToCameraAttrFps(capability.maxFPS);
        if (fps > 0) {
            result = camera_attr_set_preview_fps(device_, fps);
            if (result != CAMERA_ERROR_NONE) {
                RTC_LOG(LS_ERROR)
                    << "Cannot set fps " << capability.maxFPS
                    << " to camera, error: " << get_error_message(result);
                return -1;
            }
        } else {
            RTC_LOG(LS_ERROR) << "Unsupported fps " << capability.maxFPS;
            return -1;
        }
        rate_ = capability.maxFPS;

        if (captureType_ == kNone) {
            result = camera_set_media_packet_preview_cb(
                device_, OnCapturedWithMediaPacket, this);
            if (result != CAMERA_ERROR_NONE) {
                RTC_LOG(LS_INFO)
                    << "Cannot set media packet preview callback, error: "
                    << get_error_message(result);
                result = camera_set_preview_cb(device_, OnCaptured, this);
                if (result != CAMERA_ERROR_NONE) {
                    RTC_LOG(LS_ERROR) << "Cannot set preview callback, error: "
                                      << get_error_message(result);
                    return -1;
                }
                captureType_ = kFrame;
            } else {
                captureType_ = kMediaPacket;
            }
        }

        result = camera_start_preview(device_);
        if (result != CAMERA_ERROR_NONE) {
            RTC_LOG(LS_ERROR)
                << "Cannot satrt preview, error: " << get_error_message(result);
            StopCapture();
            return -1;
        }

        capturing_ = true;
        return 0;
    }

    int32_t VideoCaptureModuleTizen::StopCapture()
    {
        int result = CAMERA_ERROR_NONE;
        if (captureType_ == kMediaPacket) {
            result = camera_unset_media_packet_preview_cb(device_);
            if (result != CAMERA_ERROR_NONE) {
                RTC_LOG(LS_ERROR)
                    << "Cannot unset media packet preview callback, error: "
                    << get_error_message(result);
                return -1;
            } else {
                captureType_ = kNone;
            }
        } else if (captureType_ == kFrame) {
            result = camera_unset_preview_cb(device_);
            if (result != CAMERA_ERROR_NONE) {
                RTC_LOG(LS_ERROR) << "Cannot unset preview callback, error: "
                                  << get_error_message(result);
                return -1;
            } else {
                captureType_ = kNone;
            }
        }

        if (capturing_) {
            result = camera_stop_preview(device_);
            if (result != CAMERA_ERROR_NONE) {
                RTC_LOG(LS_ERROR) << "Cannot stop preview, error: "
                                  << get_error_message(result);
                return -1;
            }

            capturing_ = false;
        }
        return 0;
    }

    bool VideoCaptureModuleTizen::CaptureStarted()
    {
        return capturing_;
    }

    int32_t VideoCaptureModuleTizen::CaptureSettings(
        VideoCaptureCapability& settings)
    {
        settings.width = width_;
        settings.height = height_;
        settings.maxFPS = rate_;
        settings.videoType = videoType_;

        return 0;
    }

    void VideoCaptureModuleTizen::OnCapturedWithMediaPacket(
        media_packet_h packet, void* data)
    {
        VideoCaptureModuleTizen* self =
            static_cast<VideoCaptureModuleTizen*>(data);
        media_format_h format = nullptr;
        media_format_mimetype_e mime;
        int width, height, avgBps, maxBps;
        media_packet_get_format(packet, &format);
        media_format_get_video_info(format, &mime, &width, &height, &avgBps,
                                    &maxBps);
        media_format_unref(format);

        VideoCaptureCapability frameInfo;
        frameInfo.width = width;
        frameInfo.height = height;

        switch (mime) {
        case MEDIA_FORMAT_NV12: {
            int strideY, strideUV;
            void *dataY, *dataUV;
            media_packet_get_video_plane_data_ptr(packet, 0, &dataY);
            media_packet_get_video_stride_width(packet, 0, &strideY);
            media_packet_get_video_plane_data_ptr(packet, 1, &dataUV);
            media_packet_get_video_stride_width(packet, 1, &strideUV);
            frameInfo.videoType = VideoType::kI420;
            self->IncomingNV12Frame(static_cast<uint8_t*>(dataY), strideY,
                                    static_cast<uint8_t*>(dataUV), strideUV,
                                    frameInfo);
            break;
        }
        default:
            RTC_LOG(LS_ERROR) << "Unsupported format " << mime;
            break;
        }

        media_packet_destroy(packet);
    }

    void VideoCaptureModuleTizen::OnCaptured(camera_preview_data_s* frame,
                                             void* data)
    {
        VideoCaptureModuleTizen* self =
            static_cast<VideoCaptureModuleTizen*>(data);
        VideoCaptureCapability frameInfo;
        frameInfo.width = frame->width;
        frameInfo.height = frame->height;
        frameInfo.videoType = CameraPixelFormatToVideoType(frame->format);
        if (frameInfo.videoType == VideoType::kUnknown) {
            RTC_LOG(LS_ERROR) << "Unknown format";
            return;
        }

        switch (frame->format) {
        case CAMERA_PIXEL_FORMAT_NV12: {
            // The width and height of frame are aligned by 512 on RB5
            // For example, the frame size is 1280 * 720, the real buffer is
            // allocated with 1536 * 1024 int strideY = ceilf(frame->width /
            // 512.0) * 512;
            int strideY = frame->width;
            int strideUV = strideY;
            self->IncomingNV12Frame(
                static_cast<uint8_t*>(frame->data.double_plane.y), strideY,
                static_cast<uint8_t*>(frame->data.double_plane.uv), strideUV,
                frameInfo, frame->timestamp);
            break;
        }
        case CAMERA_PIXEL_FORMAT_YUYV:
            self->IncomingFrame(frame->data.single_plane.yuv,
                                frame->data.single_plane.size, frameInfo,
                                frame->timestamp);
            break;
        case CAMERA_PIXEL_FORMAT_MJPEG:
            self->IncomingFrame(frame->data.encoded_plane.data,
                                frame->data.encoded_plane.size, frameInfo,
                                frame->timestamp);
            break;
        default:
            RTC_LOG(LS_ERROR) << "Unsupported format "
                              << CameraPixelFormatToString(frame->format);
            break;
        }
    }
} // namespace videocapturemodule
} // namespace webrtc

#endif
