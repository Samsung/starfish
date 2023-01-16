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

// This file contains interfaces used for creating the VideoCaptureModule
// and DeviceInfo.

#ifndef SRC_TIZEN_VIDEO_CAPTURE_VIDEO_CAPTURE_FACTORY_TIZEN_H_
#define SRC_TIZEN_VIDEO_CAPTURE_VIDEO_CAPTURE_FACTORY_TIZEN_H_

#include "api/scoped_refptr.h"
#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_defines.h"

namespace webrtc {

class VideoCaptureFactoryTizen {
public:
    // Create a video capture module object
    // id - unique identifier of this video capture module object.
    // deviceUniqueIdUTF8 - name of the device.
    //                      Available names can be found by using GetDeviceName
    static rtc::scoped_refptr<VideoCaptureModule> Create(
        const char* deviceUniqueIdUTF8);

    static VideoCaptureModule::DeviceInfo* CreateDeviceInfo();

private:
    ~VideoCaptureFactoryTizen();
};

} // namespace webrtc

#endif // MODULES_VIDEO_CAPTURE_VIDEO_CAPTURE_FACTORY_H_

#endif
