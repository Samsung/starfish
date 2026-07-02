/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifdef STARFISH_ENABLE_MULTIMEDIA
#if !defined(STARFISH_USE_MOCK_MEDIAPLAYER) && defined(STARFISH_TIZEN)
#if !defined(STARFISH_TIZEN_PROD_TV) || \
    defined(STARFISH_TIZEN_USERAPP_SDK_API_ONLY)

#include <algorithm>

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/util/URL.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLVideoElement.h"
#include "core/fileapi/Blob.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"
#include "core/modules/mediasource/MediaSource.h"
#include "core/modules/mediasource/SourceBuffer.h"
#include "core/modules/threading/Thread.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "platform/multimedia/MediaPlayerTizen.h"
#include "core/modules/renderer/Renderer.h"

namespace Starfish {

#define STARFISH_VIDEO_MAX_WIDTH 1920
#define STARFISH_VIDEO_MAX_HEIGHT 1080
#define STARFISH_VIDEO_DEFAULT_FRAMERATE_NUM 2997
#define STARFISH_VIDEO_DEFAULT_FRAMERATE_DEN 100
#define STARFISH_MSE_SUBMIT_BYTES_RATE 0.3

// Render the video on a HW overlay plane (player_set_ecore_wl_display OVERLAY +
// DST_ROI) with a punch-hole in the web content, instead of decode-to-texture +
// GL compositing. The video then bypasses the web compositor entirely, so the
// per-video-frame whole-page recomposite (the playback stutter) disappears.
// Controlled at runtime via the public LWE Settings
// (Settings::SetVideoOverlayEnabled), read off the owning WebView.
bool MediaPlayerTizen::videoOverlayEnabled()
{
    // STARFISH_VIDEO_OVERLAY=1 (or =0) overrides the app setting, so the
    // overlay path can be toggled per-device without an app update.
    static int envOverride = []() {
        const char* v = getenv("STARFISH_VIDEO_OVERLAY");
        return v && *v ? (atoi(v) != 0 ? 1 : 0) : -1;
    }();
    if (envOverride >= 0) {
        return envOverride == 1;
    }
    return m_container && m_container->webView() &&
           m_container->webView()->videoOverlayEnabled();
}

void MediaPlayerSourceStream::initFormatExtraForAudio()
{
}

void MediaPlayerSourceStream::initFormatExtraForVideo()
{
}

void MediaPlayerSourceStream::createMediaFormatStreamType()
{
}

void MediaPlayerSourceStream::releaseMediaFormatStreamType()
{
}

int MediaPlayerTizen::playerSetPlayPosition(int& timeInMS)
{
    return player_set_play_position(m_nativePlayer, timeInMS, true,
                                    seekedCallback, this);
}

void MediaPlayerTizen::initCanvasSurface()
{
}

void MediaPlayerTizen::disposePlayer()
{
    dispose();
}

void MediaPlayerTizen::setNativePlayerDisplayMode()
{
    if (videoOverlayEnabled()) {
        // HW overlay plane + DST_ROI. The hole is punched (and ROI tracked) in
        // didDrawVideo()/punchHole().
        player_set_display_mode(m_nativePlayer, PLAYER_DISPLAY_MODE_DST_ROI);
        m_lastAbsoluteROIArea = LayoutRect(0, 0, 1, 1);
        m_lastVideoSourceROI = { { 0.0, 0.0, 1.0, 1.0 } };
        player_set_display_roi_area(m_nativePlayer, 0, 0, 1, 1);
#if defined(STARFISH_TIZEN_USERAPP_SDK_API_ONLY)
        void* elmWindowHandle =
            m_container->webView()->publicLayerUserDataMap()
                ["__internalLWEWebViewEFLNativeWindowEvasObject"];
        player_set_display(m_nativePlayer, PLAYER_DISPLAY_TYPE_OVERLAY,
                           GET_DISPLAY(elmWindowHandle));
#else
        void* ecoreWaylandHandle =
            m_container->webView()->publicLayerUserDataMap()
                ["__internalLWEWebViewEFLEcoreWaylandHandle"];
        auto width = m_container->webView()->renderer()->width();
        auto height = m_container->webView()->renderer()->height();
        player_set_ecore_wl_display(m_nativePlayer, PLAYER_DISPLAY_TYPE_OVERLAY,
                                    ecoreWaylandHandle, 0, 0, width, height);
#endif
        player_set_display_visible(m_nativePlayer, true);
        m_overlayPlaneVisible = true;
    } else {
        setNativePlayerDisplayModeWithGL();
    }
}

// Toggle the HW overlay plane on/off, pushing to the player only on a change.
// Used to hide a video that has scrolled fully off-screen: the plane is not
// clipped or culled by the web compositor, so it must be parked explicitly.
void MediaPlayerTizen::setOverlayPlaneVisible(bool visible)
{
    if (m_overlayPlaneVisible == visible) {
        return;
    }
    int ret = player_set_display_visible(m_nativePlayer, visible);
    if (ret == PLAYER_ERROR_NONE) {
        m_overlayPlaneVisible = visible;
    } else {
        PLAYER_LOGE("**ERROR: player_set_display_visible %x -> ", ret);
    }
}

void MediaPlayerTizen::hideVideoOverlay()
{
    if (!videoOverlayEnabled()) {
        return;
    }
    setOverlayPlaneVisible(false);
}

void MediaPlayerTizen::setPlayerDisplayVideoAtPausedState(int& ret)
{
}
void MediaPlayerTizen::punchHole(Compositor* canvas,
                                 const LayoutRect& videoRect,
                                 const LayoutRect& absVideoRect)
{
    if (!videoOverlayEnabled()) {
        return;
    }
    canvas->punchHole(Unit::Rect(videoRect.x(), videoRect.y(),
                                 videoRect.width(), videoRect.height()));

    // The HW plane is a separate layer behind the (transparent) page, so it is
    // NOT clipped by the page: whatever rect it is given paints straight
    // through the transparent background, including the part that scrolled out
    // of its scroll container (e.g. a rounded card). The page clips the web
    // content correctly, but the overlay must be confined here to match. The
    // visible region is the video rect intersected with the compositor's
    // current clip rect (logical-screen space, same as absVideoRect); it falls
    // back to the page viewport when the compositor does not report a clip.
    // In PLAYER_DISPLAY_MODE_DST_ROI the frame is scaled to fit the display
    // ROI, so shrinking the display ROI to that intersection confines the
    // plane; the source is cropped to the same part (player_set_video_roi_area,
    // ratio-based) so the visible portion keeps its aspect instead of being
    // squeezed. The display ROI is clamped even when the source crop is
    // unavailable -- otherwise the full-size frame paints over the invisible
    // region. A fully scrolled-out video is hidden outright via
    // player_set_display_visible, because the compositor culls the off-screen
    // stacking context and stops calling this method, so the plane would
    // otherwise freeze at its last on-screen ROI.
    Unit::Rect clipRect = canvas->currentClipRect();
    LayoutUnit clipX(0);
    LayoutUnit clipY(0);
    LayoutUnit clipMaxX(m_container->webView()->renderer()->width());
    LayoutUnit clipMaxY(m_container->webView()->renderer()->height());
    if (!clipRect.isEmpty()) {
        clipX = LayoutUnit(clipRect.x());
        clipY = LayoutUnit(clipRect.y());
        clipMaxX = LayoutUnit(clipRect.maxX());
        clipMaxY = LayoutUnit(clipRect.maxY());
    }
    LayoutUnit visibleX = std::max(absVideoRect.x(), clipX);
    LayoutUnit visibleY = std::max(absVideoRect.y(), clipY);
    LayoutUnit visibleMaxX = std::min(absVideoRect.maxX(), clipMaxX);
    LayoutUnit visibleMaxY = std::min(absVideoRect.maxY(), clipMaxY);
    if (absVideoRect.width() <= 0 || absVideoRect.height() <= 0 ||
        visibleMaxX <= visibleX || visibleMaxY <= visibleY) {
        setOverlayPlaneVisible(false);
        return;
    }
    setOverlayPlaneVisible(true);

    LayoutRect roiArea(visibleX, visibleY, visibleMaxX - visibleX,
                       visibleMaxY - visibleY);
    double videoWidth = absVideoRect.width().toDouble();
    double videoHeight = absVideoRect.height().toDouble();
    std::array<double, 4> sourceROI{
        { (visibleX - absVideoRect.x()).toDouble() / videoWidth,
          (visibleY - absVideoRect.y()).toDouble() / videoHeight,
          (visibleMaxX - visibleX).toDouble() / videoWidth,
          (visibleMaxY - visibleY).toDouble() / videoHeight }
    };

    if (!m_videoSourceROIUnsupported && sourceROI != m_lastVideoSourceROI) {
        int ret =
            player_set_video_roi_area(m_nativePlayer, sourceROI[0],
                                      sourceROI[1], sourceROI[2], sourceROI[3]);
        if (ret == PLAYER_ERROR_NONE) {
            m_lastVideoSourceROI = sourceROI;
        } else {
            PLAYER_LOGE("**ERROR: player_set_video_roi_area %x -> ", ret);
            m_videoSourceROIUnsupported = true;
        }
    }

    if (m_lastAbsoluteROIArea != roiArea) {
        // TODO consider LWE::WebView x, y
        int ret = player_set_display_roi_area(
            m_nativePlayer, roiArea.x().toInt(), roiArea.y().toInt(),
            roiArea.width().toInt(), roiArea.height().toInt());
        if (ret == PLAYER_ERROR_NONE) {
            m_lastAbsoluteROIArea = roiArea;
        } else {
            PLAYER_LOGE("**ERROR: player_set_display_roi_area %x -> ", ret);
        }
    }
}

void MediaPlayerTizen::updateAudioStreamInfo(MediaPlayerSourceStream* audio,
                                             size_t pastInitIndex,
                                             size_t newInitIndex)
{
}

void MediaPlayerTizen::videoFramerateChanged(MediaPlayerSourceStream* stream,
                                             int num, int den)
{
}

void MediaPlayerTizen::setMediaFormatExtraForVideo(media_format_h& mediaFormat,
                                                   StreamInfo* info)
{
    if (info->duration() > 0) {
        media_format_set_video_frame_rate(mediaFormat,
                                          (1000 / info->duration()));
    }
}
void MediaPlayerTizen::setMediaFormatExtraForAudio(media_format_h& mediaFormat,
                                                   StreamInfo* info)
{
}
} // namespace Starfish

#endif
#endif
#endif
