/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */
/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#if defined(STARFISH_ENABLE_WEBRTC)

#include "StarfishConfig.h"
#include "Starfish.h"

#include "platform/webrtc/VideoCapturer.h"

#include "api/scoped_refptr.h"
#include "api/video/i420_buffer.h"
#include "api/video/video_frame_buffer.h"
#include "api/video/video_rotation.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "modules/video_capture/video_capture_factory.h"

#ifdef STARFISH_TIZEN_PROD_TV
// TODO
#include "platform/webrtc/tizen/video_capture/video_capture_factory_tizen.h"
#endif

namespace Starfish {

VideoCapturer::VideoCapturer()
{
}

bool VideoCapturer::init(size_t width, size_t height, size_t targetFps,
                         size_t captureDeviceIndex)
{
#ifdef STARFISH_TIZEN_PROD_TV
    std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> deviceInfo(
        webrtc::VideoCaptureFactoryTizen::CreateDeviceInfo());
#else
    std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> deviceInfo(
        webrtc::VideoCaptureFactory::CreateDeviceInfo());
#endif

    char deviceName[256];
    char uniqueName[256];
    if (deviceInfo->GetDeviceName(static_cast<uint32_t>(captureDeviceIndex),
                                  deviceName, sizeof(deviceName), uniqueName,
                                  sizeof(uniqueName)) != 0) {
        destroy();
        return false;
    }

#ifdef STARFISH_TIZEN_PROD_TV
    m_vcm = webrtc::VideoCaptureFactoryTizen::Create(uniqueName);
#else
    m_vcm = webrtc::VideoCaptureFactory::Create(uniqueName);
#endif

    if (!m_vcm) {
        return false;
    }
    m_vcm->RegisterCaptureDataCallback(this);

    webrtc::VideoCaptureCapability capability;
    capability.width = static_cast<int32_t>(width);
    capability.height = static_cast<int32_t>(height);
    capability.maxFPS = static_cast<int32_t>(targetFps);
    capability.videoType = webrtc::VideoType::kUnknown;
    deviceInfo->GetBestMatchedCapability(m_vcm->CurrentDeviceName(), capability,
                                         m_capability);

    if (m_vcm->StartCapture(m_capability) != 0) {
        destroy();
        return false;
    }

    STARFISH_ASSERT(m_vcm->CaptureStarted());
    return true;
}

VideoCapturer* VideoCapturer::create(size_t width, size_t height,
                                     size_t targetFps,
                                     size_t captureDeviceIndex)
{
    std::unique_ptr<VideoCapturer> videoCapturer(new VideoCapturer());
    if (!videoCapturer->init(width, height, targetFps, captureDeviceIndex)) {
        STARFISH_LOG_WARN(
            "Failed to create VcmCapturer(w =%zu, h=%zu, fps=%zu)", width,
            height, targetFps);
        return nullptr;
    }
    return videoCapturer.release();
}

void VideoCapturer::destroy()
{
    if (!m_vcm) {
        return;
    }

    m_vcm->StopCapture();
    m_vcm->DeRegisterCaptureDataCallback();
    // Release reference to VCM.
    m_vcm = nullptr;
}

VideoCapturer::~VideoCapturer()
{
    destroy();
}

void VideoCapturer::AddOrUpdateSink(
    rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
    const rtc::VideoSinkWants& wants)
{
    m_broadcaster.AddOrUpdateSink(sink, wants);
    updateVideoAdapter();
}

void VideoCapturer::RemoveSink(
    rtc::VideoSinkInterface<webrtc::VideoFrame>* sink)
{
    m_broadcaster.RemoveSink(sink);
    updateVideoAdapter();
}

void VideoCapturer::updateVideoAdapter()
{
    rtc::VideoSinkWants wants = m_broadcaster.wants();
    // TODO : Activate this part when m94 is applied.
    /*
    m_videoAdapter.OnSinkWants(wants);

    if (0 < wants.resolutions.size()) {
        auto size = wants.resolutions.at(0);
        std::pair<int, int> target_aspect_ratiot(size.width, size.height);
        m_videoAdapter.OnOutputFormatRequest(target_aspect_ratiot,
                                             wants.max_pixel_count,
                                             wants.max_framerate_fps);
    }
    */
}

void VideoCapturer::OnFrame(const webrtc::VideoFrame& frame)
{
    int croppedWidth = 0;
    int croppedHeight = 0;
    int outWidth = 0;
    int outHeight = 0;

    if (!m_videoAdapter.AdaptFrameResolution(
            frame.width(), frame.height(), frame.timestamp_us() * 1000,
            &croppedWidth, &croppedHeight, &outWidth, &outHeight)) {
        // Drop frame in order to respect frame rate constraint.
        return;
    }

    if (outHeight != frame.height() || outWidth != frame.width()) {
        // Video adapter has requested a down-scale. Allocate a new buffer
        // and return scaled version.
        rtc::scoped_refptr<webrtc::I420Buffer> scaledBuffer =
            webrtc::I420Buffer::Create(outWidth, outHeight);
        scaledBuffer->ScaleFrom(*frame.video_frame_buffer()->ToI420());
        m_broadcaster.OnFrame(webrtc::VideoFrame::Builder()
                                  .set_video_frame_buffer(scaledBuffer)
                                  .set_rotation(webrtc::kVideoRotation_0)
                                  .set_timestamp_us(frame.timestamp_us())
                                  .set_id(frame.id())
                                  .build());
    } else {
        // No adaptations needed, just return the frame as is.
        m_broadcaster.OnFrame(frame);
    }
}

rtc::VideoSinkWants VideoCapturer::sinkWants()
{
    return m_broadcaster.wants();
}
} // namespace Starfish

#endif
