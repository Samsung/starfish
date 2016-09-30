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
#ifdef STARFISH_TIZEN_MOBILE

#include "StarFishConfig.h"
#include "MediaPlayer.h"
#include "util/URL.h"
#include "dom/Document.h"
#include "dom/HTMLVideoElement.h"
#include "platform/message_loop/MessageLoop.h"
#include "platform/canvas/Canvas.h"
#include "platform/threading/Thread.h"
#include "extra/MediaSource.h"

#include <media/player.h>

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

class MediaPlayerTizen : public MediaPlayer {
public:
    MediaPlayerTizen(HTMLMediaElement* element)
        : MediaPlayer(element)
        , m_isURISetted(false)
    {
        player_create(&m_nativePlayer);
        player_set_error_cb(m_nativePlayer, mediaPlayerErrorCallback, this);
        player_set_completed_cb(m_nativePlayer, [](void* data) {
            MediaPlayerTizen* player = (MediaPlayerTizen*)data;
            player->m_starFish->messageLoop()->addIdlerWithNoGCRootingInOtherThread([](size_t, void* data) {
                MediaPlayerTizen* player = (MediaPlayerTizen*)data;
                player->m_starFish->removePointerFromRootSet(player);
                player_stop(player->m_nativePlayer);
            }, data);
        }, this);

        m_canvasSurface = CanvasSurface::create(element->document()->window(), 1, 1);
        m_canvasSurface->clear();

        GC_REGISTER_FINALIZER_NO_ORDER(this, [] (void* obj, void* cd) {
            MediaPlayerTizen* player = (MediaPlayerTizen*)obj;
            player_destroy(player->m_nativePlayer);
        }, NULL, NULL, NULL);
    }

    virtual void processOperationQueue(MediaPlayerOperationQueueData* data)
    {
        auto type = data->eventType();
        STARFISH_LOG_INFO("MediaPlayerTizen::processOperationQueue %d\n", (int)type);
        player_state_e state;
        player_get_state(m_nativePlayer, &state);
        if (type == MediaPlayerOperationQueueData::SetURLEventType) {
            URL* url = ((MediaPlayerOperationQueueDataSetURL*)data)->m_url;
            if (state == PLAYER_STATE_PLAYING) {
                pauseOperation();
            }

            if (state != PLAYER_STATE_IDLE) {
                player_unprepare(m_nativePlayer);
                if (m_container->isHTMLVideoElement() && m_container->frame()) {
                    m_container->setNeedsLayout();
                }
            }

            player_display_h displayHandle = GET_DISPLAY((Evas_Object*)m_canvasSurface->unwrap());
            player_display_type_e displayType = PLAYER_DISPLAY_TYPE_EVAS;
            player_display_mode_e displayMode = PLAYER_DISPLAY_MODE_ORIGIN_OR_LETTER;

            player_set_display(m_nativePlayer, displayType, displayHandle);
            player_set_display_mode(m_nativePlayer, displayMode);

            m_hasVideo = false;
            if (url) {
                player_set_uri(m_nativePlayer, url->urlString()->utf8Data());
                m_isURISetted = true;
            } else {
                m_isURISetted = false;
            }
            processNextOperationQueue();
        } else if (type == MediaPlayerOperationQueueData::RequestPrepareEventType) {
            if (state == PLAYER_STATE_IDLE) {
                if (m_isURISetted) {
                    m_starFish->addPointerInRootSet(this);
                    player_prepare_async(m_nativePlayer, [](void *user_data) {
                        MediaPlayerTizen* self = (MediaPlayerTizen*)user_data;
                        STARFISH_ASSERT(!isMainThread());
                        self->m_starFish->messageLoop()->addIdlerWithNoGCRootingInOtherThread([](size_t, void* user_data) {
                            MediaPlayerTizen* self = (MediaPlayerTizen*)user_data;
                            self->m_starFish->removePointerFromRootSet(self);

                            char* videoCodec = nullptr;
                            char* audioCodec = nullptr;
                            player_get_codec_info(self->m_nativePlayer, &videoCodec, &audioCodec);

                            if (videoCodec) {
                                self->m_hasVideo = true;
                                int width = 1;
                                int height = 1;
                                player_get_video_size(self->m_nativePlayer, &width, &height);
                                STARFISH_ASSERT(width > 0);
                                STARFISH_ASSERT(height > 0);
                                self->m_videoWidth = (unsigned long)width;
                                self->m_videoHeight = (unsigned long)height;
                                if (self->m_container->isHTMLVideoElement() && self->m_container->frame()) {
                                    self->m_container->setNeedsLayout();
                                }
                                self->m_canvasSurface->resize(self->m_videoWidth, self->m_videoHeight);
                            }

                            STARFISH_LOG_INFO("MediaPlayerTizen::processOperationQueue::player_prepare_async ok %s %s %d %d\n", videoCodec, audioCodec, (int)self->m_videoWidth, (int)self->m_videoHeight);

                            free(videoCodec);
                            free(audioCodec);

                            self->processNextOperationQueue();
                        }, user_data);
                    }, this);
                    return;
                } else {
                    // ignore command
                }
            }
            processNextOperationQueue();
        } else if (type == MediaPlayerOperationQueueData::RequestPlayEventType) {
            if (state == PLAYER_STATE_READY || state == PLAYER_STATE_PAUSED) {
                m_starFish->addPointerInRootSet(this);
                player_start(m_nativePlayer);
                STARFISH_LOG_INFO("MediaPlayerTizen::processOperationQueue::player_start\n");
            } else if (state == PLAYER_STATE_IDLE) {
                prependToOperationQueue(new MediaPlayerOperationQueueDataRequestPlay(this));
                prependToOperationQueue(new MediaPlayerOperationQueueDataRequestPrepare(this));
            } else if (state == PLAYER_STATE_PLAYING) {
                // ignore command
            } else {
                STARFISH_LOG_INFO("invalid state %d\n", (int)state);
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            processNextOperationQueue();
        } else if (type == MediaPlayerOperationQueueData::RequestPauseEventType) {
            if (state == PLAYER_STATE_PLAYING) {
                pauseOperation();
            }
            processNextOperationQueue();
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    void pauseOperation()
    {
        m_starFish->removePointerFromRootSet(this);
        player_pause(m_nativePlayer);
    }

    virtual unsigned long videoWidth()
    {
        if (m_hasVideo) {
            return m_videoWidth;
        } else
            return STARFISH_VIDEO_WIDTH_WHEN_VIDEO_NOT_EXISTS;
    }

    virtual unsigned long videoHeight()
    {
        if (m_hasVideo) {
            return m_videoHeight;
        } else
            return STARFISH_VIDEO_HEIGHT_WHEN_VIDEO_NOT_EXISTS;
    }

    virtual double currentTime()
    {
        int s;
        int ret = player_get_play_position(m_nativePlayer, &s);
        if (ret)
            return 0;
        return s / 1000.0;
    }

    virtual void drawVideo(Canvas* canvas, const LayoutRect& videoRect, const LayoutRect& absVideoRect)
    {
        canvas->setColor(Color(0, 0, 0, 255));
        canvas->drawRect(videoRect);
        canvas->drawImage(m_canvasSurface, Rect(videoRect.x(), videoRect.y(), videoRect.width(), videoRect.height()));
    }

    bool m_isURISetted;
    CanvasSurface* m_canvasSurface;
    player_h m_nativePlayer;
    unsigned long m_videoWidth, m_videoHeight;
};

MediaPlayer* MediaPlayer::create(HTMLMediaElement* element)
{
    return new MediaPlayerTizen(element);
}

}

#endif /* STARFISH_TIZEN_MOBILE */
#endif /* STARFISH_ENABLE_MULTIMEDIA */
