/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifdef STARFISH_ENABLE_MULTIMEDIA
#ifdef STARFISH_TIZEN

#include "StarFishConfig.h"
#include "util/URL.h"
#include "dom/Document.h"
#include "dom/HTMLVideoElement.h"
#include "MediaPlayerTizen.h"
#include "platform/message_loop/MessageLoop.h"
#include "platform/canvas/Canvas.h"
#include "platform/threading/Thread.h"
#include "extra/MediaSource.h"
#include "extra/Blob.h"

namespace StarFish {

static void mediaPlayerErrorCallback(int errorCode, void *user_data)
{
    switch (errorCode) {
#define GEN_ERROR_PRINTS(errorenum) \
    case errorenum: \
        STARFISH_LOG_INFO("mediaPlayerErrorCallback() : %s\n", #errorenum); \
        return;
        GEN_ERROR_PRINTS(PLAYER_ERROR_OUT_OF_MEMORY)
        GEN_ERROR_PRINTS(PLAYER_ERROR_INVALID_PARAMETER)
        GEN_ERROR_PRINTS(PLAYER_ERROR_NO_SUCH_FILE)
        GEN_ERROR_PRINTS(PLAYER_ERROR_INVALID_OPERATION)
        GEN_ERROR_PRINTS(PLAYER_ERROR_FILE_NO_SPACE_ON_DEVICE)
        GEN_ERROR_PRINTS(PLAYER_ERROR_FEATURE_NOT_SUPPORTED_ON_DEVICE)
        GEN_ERROR_PRINTS(PLAYER_ERROR_SEEK_FAILED)
        GEN_ERROR_PRINTS(PLAYER_ERROR_INVALID_STATE)
        GEN_ERROR_PRINTS(PLAYER_ERROR_NOT_SUPPORTED_FILE)
        GEN_ERROR_PRINTS(PLAYER_ERROR_INVALID_URI)
        GEN_ERROR_PRINTS(PLAYER_ERROR_SOUND_POLICY)
        GEN_ERROR_PRINTS(PLAYER_ERROR_CONNECTION_FAILED)
        GEN_ERROR_PRINTS(PLAYER_ERROR_VIDEO_CAPTURE_FAILED)
        GEN_ERROR_PRINTS(PLAYER_ERROR_DRM_EXPIRED)
        GEN_ERROR_PRINTS(PLAYER_ERROR_DRM_NO_LICENSE)
        GEN_ERROR_PRINTS(PLAYER_ERROR_DRM_FUTURE_USE)
        GEN_ERROR_PRINTS(PLAYER_ERROR_DRM_NOT_PERMITTED)
        GEN_ERROR_PRINTS(PLAYER_ERROR_RESOURCE_LIMIT)
        GEN_ERROR_PRINTS(PLAYER_ERROR_PERMISSION_DENIED)
#undef GEN_ERROR_PRINTS
    default:
        STARFISH_LOG_INFO("mediaPlayerErrorCallback() : Unknown error\n");
        return;
    }
}

MediaPlayerTizen::MediaPlayerTizen(HTMLMediaElement* element)
    : MediaPlayer(element)
    , m_inPrepare(false)
    , m_activeMediaSource(nullptr)
    , m_canvasSurface(nullptr)
{
    player_create(&m_nativePlayer);
    player_set_error_cb(m_nativePlayer, mediaPlayerErrorCallback, this);
    player_set_completed_cb(m_nativePlayer, [](void* data) {
        MediaPlayerTizen* player = (MediaPlayerTizen*)data;
        player->m_starFish->messageLoop()->addIdlerWithNoGCRootingInOtherThread([](size_t, void* data) {
            MediaPlayerTizen* player = (MediaPlayerTizen*)data;
            player->m_starFish->removePointerFromRootSet(player);
            player_stop(player->m_nativePlayer);
            player->m_playbackState = MediaPlayer::PLAYBACK_STATE_END;
            if (player->m_container) {
                player->m_container->dispatchPauseEventNow();
                player->m_container->dispatchEndedEventNow();
            }
        }, data);
    }, this);

    initDisplay();

    GC_REGISTER_FINALIZER_NO_ORDER(this, [] (void* obj, void* cd) {
        MediaPlayerTizen* player = (MediaPlayerTizen*)obj;
        player->unprepareOperation();
    }, NULL, NULL, NULL);
}

void MediaPlayerTizen::close()
{
    unprepareOperation();
    if (m_nativePlayer)
        player_destroy(m_nativePlayer);
    m_nativePlayer = nullptr;
}

void MediaPlayerTizen::initDisplay()
{
    m_canvasSurface = CanvasSurface::create(m_container->document()->window(), 1, 1);
    m_canvasSurface->clear();
}

void MediaPlayerTizen::setNativeOptions(URL* url)
{
    player_display_h displayHandle = GET_DISPLAY(m_canvasSurface->unwrap());
    player_display_type_e displayType = PLAYER_DISPLAY_TYPE_EVAS;
    player_display_mode_e displayMode = PLAYER_DISPLAY_MODE_ORIGIN_OR_LETTER;

    player_set_display(m_nativePlayer, displayType, displayHandle);
    player_set_display_mode(m_nativePlayer, displayMode);
}

void MediaPlayerTizen::prepare(URL* url)
{
    if (url->isBlobURL()) {
        BlobURLStore store;
        if (!StarFish::stringToBlobURLString(url->urlString(), store)) {
            STARFISH_LOG_ERROR("MediaPlayerTizen::prepare, seturl, FAIL - INVALID BLOB URL\n");
            return;
        }
        if (m_starFish->isValidBlobURL(store)) {
            player_set_memory_buffer(m_nativePlayer, ((Blob *)store.m_blob)->data(), ((Blob *)store.m_blob)->size());
        } else if (m_starFish->isValidMediaSourceBlobURL(store)) {
            // player_set_uri(m_nativePlayer, "demuxer://aaaa");
        } else {
            // fire eror
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    } else {
        player_set_uri(m_nativePlayer, url->urlString()->utf8Data());
    }

    setNativeOptions(url);

    m_starFish->addPointerInRootSet(this);
    player_prepare_async(m_nativePlayer, [](void *user_data) {
        MediaPlayerTizen* self = (MediaPlayerTizen*)user_data;
        STARFISH_ASSERT(!isMainThread());
        self->m_starFish->messageLoop()->addIdlerWithNoGCRootingInOtherThread([](size_t, void* user_data) {
            MediaPlayerTizen* self = (MediaPlayerTizen*)user_data;
            self->m_starFish->removePointerFromRootSet(self);

            if (!self->m_nativePlayer)
                return;

            char* videoCodec = nullptr;
            char* audioCodec = nullptr;
            player_get_codec_info(self->m_nativePlayer, &audioCodec, &videoCodec);

            if (videoCodec) {
                self->m_hasVideo = true;
                int width = 1;
                int height = 1;
                player_get_video_size(self->m_nativePlayer, &width, &height);
                STARFISH_ASSERT(width > 0);
                STARFISH_ASSERT(height > 0);
                self->m_videoWidth = (unsigned long)width;
                self->m_videoHeight = (unsigned long)height;
                if (self->m_canvasSurface) {
                    self->m_canvasSurface->resize(self->m_videoWidth, self->m_videoHeight);
                }
            }

            STARFISH_LOG_INFO("MediaPlayerTizen::prepare ok %s %s %d %d\n", videoCodec, audioCodec, (int)self->m_videoWidth, (int)self->m_videoHeight);

            free(videoCodec);
            free(audioCodec);

            self->m_container->mediaPlayerNotifyUpdateReadyStateItsContainer(HTMLMediaElement::HAVE_METADATA);
            self->m_container->mediaPlayerNotifyUpdateReadyStateItsContainer(HTMLMediaElement::HAVE_FUTURE_DATA);

            self->processNextOperationQueueInContainer();
        }, user_data);
    }, this);
}

void MediaPlayerTizen::pauseOperation()
{
    m_starFish->removePointerFromRootSet(this);
    player_pause(m_nativePlayer);
    if (m_container) {
        m_container->dispatchPauseEvent();
    }
}

void MediaPlayerTizen::unprepareOperation()
{
    if (m_nativePlayer) {
        player_unprepare(m_nativePlayer);
        if (m_container->isHTMLVideoElement() && m_container->frame()) {
            m_container->setNeedsLayout();
        }

        if (m_activeMediaSource) {
            // m_activeMediaSource->close();
        }
        m_activeMediaSource = nullptr;

        m_container->mediaPlayerNotifyUpdateReadyStateItsContainer(HTMLMediaElement::HAVE_NOTHING);
    }
}

void MediaPlayerTizen::drawVideo(Canvas* canvas, const LayoutRect& videoRect, const LayoutRect& absVideoRect)
{
    canvas->setColor(Color(0, 0, 0, 255));
    canvas->drawRect(videoRect);
    canvas->drawImage(m_canvasSurface, Rect(videoRect.x(), videoRect.y(), videoRect.width(), videoRect.height()));
}

#if !defined(STARFISH_TIZEN_TV)
MediaPlayer* MediaPlayer::create(HTMLMediaElement* element)
{
    return new MediaPlayerTizen(element);
}
#endif

}

#endif
#endif /* STARFISH_ENABLE_MULTIMEDIA */
