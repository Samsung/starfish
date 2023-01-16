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

#include "device_info_tizen.h"

#include <map>

#include "absl/strings/match.h"
#include "help_functions.h"
#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_defines.h"
#include "modules/video_capture/video_capture_impl.h"
#include "rtc_base/logging.h"

namespace webrtc {
namespace videocapturemodule {

    DeviceInfoTizen::DeviceInfoTizen()
    {
    } // namespace videocapturemodule

    DeviceInfoTizen::~DeviceInfoTizen()
    {
    }

    uint32_t DeviceInfoTizen::NumberOfDevices()
    {
        camera_h camera_handle;
        int err = camera_create(CAMERA_DEVICE_CAMERA0, &camera_handle);
        if (err != CAMERA_ERROR_NONE) {
            RTC_LOG(LS_ERROR)
                << "Cannot create camera, error : " << get_error_message(err);
            return 0;
        }
        int device_count = 0;
        err = camera_get_device_count(camera_handle, &device_count);
        if (err != CAMERA_ERROR_NONE) {
            RTC_LOG(LS_ERROR) << "Cannot read camera count, error : "
                              << get_error_message(err);
        }
        camera_destroy(camera_handle);
        return device_count;
    }

    int32_t DeviceInfoTizen::GetDeviceName(
        uint32_t deviceNumber, char* deviceNameUTF8, uint32_t deviceNameLength,
        char* deviceUniqueIdUTF8, uint32_t deviceUniqueIdUTF8Length,
        char* /*productUniqueIdUTF8*/, uint32_t /*productUniqueIdUTF8Length*/)
    {
        uint32_t count = 0;
        bool found = false;
        camera_h handle = nullptr;
        const char* deviceName;
        const char* deviceId;
        std::vector<camera_device_e> devices = {
            CAMERA_DEVICE_CAMERA0, CAMERA_DEVICE_CAMERA1, CAMERA_DEVICE_CAMERA2,
            CAMERA_DEVICE_CAMERA3, CAMERA_DEVICE_CAMERA4, CAMERA_DEVICE_CAMERA5,
            CAMERA_DEVICE_CAMERA6, CAMERA_DEVICE_CAMERA7, CAMERA_DEVICE_CAMERA8,
            CAMERA_DEVICE_CAMERA9
        };
        for (auto device : devices) {
            if (camera_create(device, &handle) == CAMERA_ERROR_NONE) {
                if (count == deviceNumber) {
                    deviceName = GetCameraName(device);
                    deviceId = GetCameraId(device);
                    found = true;
                } else {
                    count++;
                }
                camera_destroy(handle);
                if (found)
                    break;
            }
        }
        if (!found) {
            RTC_LOG(LS_INFO) << "No matching device found";
            return -1;
        }
        if (deviceNameLength > strlen(deviceName)) {
            memset(deviceNameUTF8, 0, deviceNameLength);
            memcpy(deviceNameUTF8, deviceName, strlen(deviceName));
        } else {
            RTC_LOG(LS_ERROR) << "deviceNameUTF8 passed is too small";
            return -1;
        }

        if (deviceUniqueIdUTF8Length > strlen(deviceId)) {
            memset(deviceUniqueIdUTF8, 0, deviceUniqueIdUTF8Length);
            memcpy(deviceUniqueIdUTF8, deviceId, strlen(deviceId));
        } else {
            RTC_LOG(LS_ERROR) << "deviceUniqueIdUTF8 passed is too small";
            return -1;
        }

        return 0;
    }

    int32_t DeviceInfoTizen::CreateCapabilityMap(const char* deviceUniqueIdUTF8)
    {
        bool found = false;
        const int32_t deviceUniqueIdUTF8Length = strlen(deviceUniqueIdUTF8);
        if (deviceUniqueIdUTF8Length >= kVideoCaptureUniqueNameLength) {
            RTC_LOG(LS_ERROR) << "Device name too long";
            return -1;
        }
        RTC_LOG(LS_INFO) << "CreateCapabilityMap called for device "
                         << deviceUniqueIdUTF8;
        std::vector<camera_device_e> devices = {
            CAMERA_DEVICE_CAMERA0, CAMERA_DEVICE_CAMERA1, CAMERA_DEVICE_CAMERA2,
            CAMERA_DEVICE_CAMERA3, CAMERA_DEVICE_CAMERA4, CAMERA_DEVICE_CAMERA5,
            CAMERA_DEVICE_CAMERA6, CAMERA_DEVICE_CAMERA7, CAMERA_DEVICE_CAMERA8,
            CAMERA_DEVICE_CAMERA9
        };
        camera_h handle = nullptr;
        for (auto device : devices) {
            if (camera_create(device, &handle) == CAMERA_ERROR_NONE) {
                if (strncmp(GetCameraId(device), deviceUniqueIdUTF8,
                            deviceUniqueIdUTF8Length) == 0) {
                    found = true;
                    break;
                }
                camera_destroy(handle);
            }
        }

        if (!found) {
            RTC_LOG(LS_INFO) << "No matching device found";
            return -1;
        }

        _captureCapabilities.clear();
        int size = FillCapabilities(handle);
        camera_destroy(handle);
        // Store the new used device name
        _lastUsedDeviceNameLength = deviceUniqueIdUTF8Length;
        _lastUsedDeviceName = static_cast<char*>(
            realloc(_lastUsedDeviceName, _lastUsedDeviceNameLength + 1));
        memcpy(_lastUsedDeviceName, deviceUniqueIdUTF8,
               _lastUsedDeviceNameLength + 1);

        RTC_LOG(LS_INFO) << "CreateCapabilityMap "
                         << _captureCapabilities.size();
        return size;
    }

    int32_t DeviceInfoTizen::FillCapabilities(camera_h handle)
    {
        std::vector<camera_pixel_format_e> formats;
        camera_foreach_supported_preview_format(
            handle,
            [](camera_pixel_format_e format, void* user_data) -> bool {
                std::vector<camera_pixel_format_e>* formats =
                    static_cast<std::vector<camera_pixel_format_e>*>(user_data);
                formats->push_back(format);
                return true;
            },
            &formats);
        std::vector<std::pair<int, int>> resolutions;
        camera_foreach_supported_preview_resolution(
            handle,
            [](int width, int height, void* user_data) -> bool {
                std::vector<std::pair<int, int>>* resolutions =
                    static_cast<std::vector<std::pair<int, int>>*>(user_data);
                resolutions->push_back(std::pair<int, int>(width, height));
                return true;
            },
            &resolutions);
        for (auto format : formats) {
            VideoType type = CameraPixelFormatToVideoType(format);
            if (type == VideoType::kUnknown) {
                continue;
            }

            for (auto resolution : resolutions) {
                VideoCaptureCapability cap;
                cap.width = resolution.first;
                cap.height = resolution.second;
                cap.videoType = type;

                camera_attr_fps_e maxFPS = CAMERA_ATTR_FPS_AUTO;
                camera_attr_foreach_supported_fps_by_resolution(
                    handle, resolution.first, resolution.second,
                    [](camera_attr_fps_e fps, void* user_data) -> bool {
                        camera_attr_fps_e* maxFPS =
                            static_cast<camera_attr_fps_e*>(user_data);
                        if (*maxFPS < fps) {
                            *maxFPS = fps;
                        }
                        return true;
                    },
                    &maxFPS);
                cap.maxFPS = CameraAttrFpsToInt(maxFPS);
                _captureCapabilities.push_back(cap);
                RTC_LOG(LS_INFO)
                    << "Camera capability, width: " << cap.width
                    << " height: " << cap.height
                    << " type: " << VideoTypeToString(cap.videoType)
                    << " fps: " << cap.maxFPS;
            }
        }
        return _captureCapabilities.size();
    }

    int32_t DeviceInfoTizen::Init()
    {
        return 0;
    }

    int32_t DeviceInfoTizen::DisplayCaptureSettingsDialogBox(
        const char* /*deviceUniqueIdUTF8*/, const char* /*dialogTitleUTF8*/,
        void* /*parentWindow*/, uint32_t /*positionX*/, uint32_t /*positionY*/)
    {
        return -1;
    }

    int32_t DeviceInfoTizen::GetBestMatchedCapability(
        const char* deviceUniqueIdUTF8, const VideoCaptureCapability& requested,
        VideoCaptureCapability& resulting)
    {
        if (!deviceUniqueIdUTF8)
            return -1;

        MutexLock lock(&_apiLock);
        if (!absl::EqualsIgnoreCase(
                deviceUniqueIdUTF8,
                absl::string_view(_lastUsedDeviceName,
                                  _lastUsedDeviceNameLength))) {
            if (-1 == CreateCapabilityMap(deviceUniqueIdUTF8)) {
                return -1;
            }
        }

        int32_t bestformatIndex = -1;
        int32_t bestWidth = 0;
        int32_t bestHeight = 0;
        int32_t bestFrameRate = 0;
        VideoType bestVideoType = VideoType::kUnknown;

        const int32_t numberOfCapabilies =
            static_cast<int32_t>(_captureCapabilities.size());

        for (int32_t tmp = 0; tmp < numberOfCapabilies;
             ++tmp) // Loop through all capabilities
        {
            VideoCaptureCapability& capability = _captureCapabilities[tmp];

            const int32_t diffWidth = capability.width - requested.width;
            const int32_t diffHeight = capability.height - requested.height;
            const int32_t diffFrameRate = capability.maxFPS - requested.maxFPS;

            const int32_t currentbestDiffWith = bestWidth - requested.width;
            const int32_t currentbestDiffHeight = bestHeight - requested.height;
            const int32_t currentbestDiffFrameRate =
                bestFrameRate - requested.maxFPS;

            if ((diffHeight >= 0 &&
                 diffHeight <=
                     abs(currentbestDiffHeight)) // Height better or equalt
                                                 // that previouse.
                || (currentbestDiffHeight < 0 &&
                    diffHeight >= currentbestDiffHeight)) {
                if (diffHeight ==
                    currentbestDiffHeight) // Found best height. Care about the
                                           // width)
                {
                    if ((diffWidth >= 0 &&
                         diffWidth <=
                             abs(currentbestDiffWith)) // Width better or equal
                        || (currentbestDiffWith < 0 &&
                            diffWidth >= currentbestDiffWith)) {
                        if (diffWidth == currentbestDiffWith &&
                            diffHeight == currentbestDiffHeight) // Same size as
                                                                 // previously
                        {
                            // Also check the best frame rate if the diff is the
                            // same as previouse
                            if (((diffFrameRate >= 0 &&
                                  diffFrameRate <=
                                      currentbestDiffFrameRate) // Frame rate to
                                                                // high but
                                                                // better match
                                                                // than
                                                                // previouse and
                                                                // we have not
                                                                // selected IUV
                                 ||
                                 (currentbestDiffFrameRate < 0 &&
                                  diffFrameRate >=
                                      currentbestDiffFrameRate)) // Current
                                                                 // frame rate
                                                                 // is lower
                                                                 // than
                                                                 // requested.
                                                                 // This is
                                                                 // better.
                            ) {
                                if ((currentbestDiffFrameRate ==
                                     diffFrameRate) // Same frame rate as
                                                    // previous  or frame rate
                                                    // allready good enough
                                    || (currentbestDiffFrameRate >= 0)) {
                                    if (bestVideoType != requested.videoType) {
                                        if (capability.videoType ==
                                            requested.videoType) {
                                            bestVideoType =
                                                capability.videoType;
                                            bestformatIndex = tmp;
                                        } else {
                                            if (capability.videoType ==
                                                    VideoType::kI420 ||
                                                capability.videoType ==
                                                    VideoType::kYUY2 ||
                                                capability.videoType ==
                                                    VideoType::kYV12) {
                                                bestVideoType =
                                                    capability.videoType;
                                                bestformatIndex = tmp;
                                            }
                                        }
                                    }
                                } else // Better frame rate
                                {
                                    bestWidth = capability.width;
                                    bestHeight = capability.height;
                                    bestFrameRate = capability.maxFPS;
                                    bestVideoType = capability.videoType;
                                    bestformatIndex = tmp;
                                }
                            }
                        } else // Better width than previously
                        {
                            bestWidth = capability.width;
                            bestHeight = capability.height;
                            bestFrameRate = capability.maxFPS;
                            bestVideoType = capability.videoType;
                            bestformatIndex = tmp;
                        }
                    }  // else width no good
                } else // Better height
                {
                    bestWidth = capability.width;
                    bestHeight = capability.height;
                    bestFrameRate = capability.maxFPS;
                    bestVideoType = capability.videoType;
                    bestformatIndex = tmp;
                }
            } // else height not good
        }     // end for

        RTC_LOG(LS_INFO) << "Best camera format: " << bestWidth << "x"
                         << bestHeight << "@" << bestFrameRate
                         << "fps, color format: "
                         << static_cast<int>(bestVideoType);

        // Copy the capability
        if (bestformatIndex < 0)
            return -1;
        resulting = _captureCapabilities[bestformatIndex];
        return bestformatIndex;
    }
} // namespace videocapturemodule
} // namespace webrtc

#endif
