/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#ifndef __StarfishVideoCapturer__
#define __StarfishVideoCapturer__

#include "api/scoped_refptr.h"
#include "api/video/video_frame.h"
#include "api/video/video_source_interface.h"

#include "media/base/video_adapter.h"
#include "media/base/video_broadcaster.h"
#include "modules/video_capture/video_capture.h"

namespace Starfish {
class VideoCapturer : public rtc::VideoSourceInterface<webrtc::VideoFrame>,
                      public rtc::VideoSinkInterface<webrtc::VideoFrame> {
public:
    virtual ~VideoCapturer();

    static VideoCapturer* create(size_t width, size_t height, size_t targetFps,
                                 size_t captureDeviceIndex);

    // Impl rtc::VideoSourceInterface
    void AddOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                         const rtc::VideoSinkWants& wants) override;
    void RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink) override;

    // Impl rtc::VideoSinkInterface
    void OnFrame(const webrtc::VideoFrame& frame) override;

private:
    VideoCapturer();
    bool init(size_t width, size_t height, size_t targetFps,
              size_t captureDeviceIndex);
    void destroy();
    rtc::VideoSinkWants sinkWants();
    void updateVideoAdapter();

    rtc::scoped_refptr<webrtc::VideoCaptureModule> m_vcm{ nullptr };
    webrtc::VideoCaptureCapability m_capability;

    rtc::VideoBroadcaster m_broadcaster;
    cricket::VideoAdapter m_videoAdapter;
};
} // namespace Starfish

#endif
#endif
