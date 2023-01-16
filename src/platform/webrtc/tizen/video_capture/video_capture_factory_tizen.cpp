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

#include "video_capture_factory_tizen.h"

#include "device_info_tizen.h"
#include "modules/video_capture/video_capture_impl.h"
#include "video_capture_module_tizen.h"
#include "rtc_base/ref_counted_object.h"

namespace webrtc {

rtc::scoped_refptr<VideoCaptureModule> VideoCaptureFactoryTizen::Create(
    const char* deviceUniqueIdUTF8)
{
    auto implementation =
        rtc::make_ref_counted<videocapturemodule::VideoCaptureModuleTizen>();

    if (implementation->Init(deviceUniqueIdUTF8) != 0) {
        return nullptr;
    }

    return implementation;
}

VideoCaptureModule::DeviceInfo* VideoCaptureFactoryTizen::CreateDeviceInfo()
{
    return new videocapturemodule::DeviceInfoTizen();
}

} // namespace webrtc
#endif
