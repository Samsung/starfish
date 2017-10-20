/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#ifdef STARFISH_TIZEN_TV

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/util/URL.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLVideoElement.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/mediasource/MediaSource.h"
#include "core/modules/mediasource/SourceBuffer.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/threading/Locker.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "platform/multimedia/MediaPlayerTizenTV.h"
#include "platform/window/PlatformWindow.h"

#include <Elementary.h>

#include <media/player.h>
#include <media/player_product.h>

namespace StarFish {

#define STARFISH_VIDEO_MAX_WIDTH 1920
#define STARFISH_VIDEO_MAX_HEIGHT 1080
#define STARFISH_VIDEO_DEFAULT_FRAMERATE_NUM 2997
#define STARFISH_VIDEO_DEFAULT_FRAMERATE_DEN 100

void MediaPlayerTizenTV::printNativePlayerError(int errorCode)
{
    switch (errorCode) {
#define GEN_ERROR_PRINTS(errorenum)           \
    case errorenum:                           \
        PLAYER_LOGE("ERR: %s\n", #errorenum); \
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
        GEN_ERROR_PRINTS(PLAYER_ERROR_STREAMING_PLAYER)
        GEN_ERROR_PRINTS(PLAYER_ERROR_AUDIO_CODEC_NOT_SUPPORTED)
        GEN_ERROR_PRINTS(PLAYER_ERROR_VIDEO_CODEC_NOT_SUPPORTED)
        GEN_ERROR_PRINTS(PLAYER_ERROR_NO_AUTH)
        GEN_ERROR_PRINTS(PLAYER_ERROR_GENEREIC)
        GEN_ERROR_PRINTS(PLAYER_ERROR_DRM_INFO)
        GEN_ERROR_PRINTS(PLAYER_ERROR_SYNC_PLAY_NETWORK_EXCEPTION)
        GEN_ERROR_PRINTS(PLAYER_ERROR_SYNC_PLAY_SERVER_DOWN)
        GEN_ERROR_PRINTS(PLAYER_ERROR_NOT_SUPPORTED_FORMAT)
#undef GEN_ERROR_PRINTS
    default:
        PLAYER_LOGI("Unknown error\n");
        return;
    }
}

void MediaPlayerTizenTV::setNativePlayerDefaultOptions(ResourceURL* url)
{
    if (m_container->isHTMLVideoElement()) {
        player_display_video_at_paused_state(m_nativePlayer, true);
        player_display_h displayHandle = GET_DISPLAY(
            (Evas_Object*)m_container->starFish()->platformWindow()->unwrap());
        player_set_display(m_nativePlayer, PLAYER_DISPLAY_TYPE_OVERLAY,
                           displayHandle);
        player_set_display_mode(m_nativePlayer, PLAYER_DISPLAY_MODE_DST_ROI);
        // NOTE: Do not edit `player_set_display_roi_area` parameter
        player_set_display_roi_area(m_nativePlayer, 0, 0, 1, 1);
    } else {
        player_set_display(m_nativePlayer, PLAYER_DISPLAY_TYPE_NONE, nullptr);
    }
}

void MediaPlayerTizenTV::drawVideo(Canvas* canvas, const LayoutRect& videoRect,
                                   const LayoutRect& absVideoRect)
{
    if (!alive()) {
        return;
    }
    player_state_e state = PLAYER_STATE_NONE;
    player_get_state(m_nativePlayer, &state);
    if (state < PLAYER_STATE_READY) {
        return;
    }
    canvas->punchHole(Unit::Rect(videoRect.x(), videoRect.y(),
                                 videoRect.width(), videoRect.height()));
    player_set_display_roi_area(
        m_nativePlayer, absVideoRect.x().toInt(), absVideoRect.y().toInt(),
        absVideoRect.width().toInt(), absVideoRect.height().toInt());
}

static void seekedCallback(void* data)
{
    PLAYER_LOGI("player_set_play_position_cb\n");
    MediaPlayerTizen* self = (MediaPlayerTizen*)data;
    self->handleSeeked();
}

void MediaPlayerTizenTV::seekOperation(int timeInMS)
{
    PLAYER_LOGI("MediaPlayerTizenTV::seekOperation() (time: %d)\n", timeInMS);
    int ret = player_set_play_position(m_nativePlayer, timeInMS, false,
                                       seekedCallback, this);
    if (ret != PLAYER_ERROR_NONE) {
        // Failed immediately
        PLAYER_LOGI(
            "MediaPlayerTizenTV::seekOperation() player_set_position_async "
            "failed immediately (IGNORE) : ");
        printNativePlayerError(ret);
        m_foundError = true;
        handleSeeked();
        return;
    }
    if (m_audioStream) {
        Locker<Mutex> locker(*m_fillBufferMutex);
        m_audioStream->setLastSubmittedDTS(timeInMS);
        m_audioStream->setWaitingDemuxer(true);
    }
    if (m_videoStream) {
        Locker<Mutex> locker(*m_fillBufferMutex);
        m_videoStream->setLastSubmittedDTS(timeInMS);
        m_videoStream->setWaitingDemuxer(true);
    }
    if (activeSourceBuffer(StreamTypeAudio)) {
        activeSourceBuffer(StreamTypeAudio)->clearPacketAccessCache();
        fillBufferIfNeeded(StreamTypeAudio);
    }
    if (activeSourceBuffer(StreamTypeVideo)) {
        activeSourceBuffer(StreamTypeVideo)->clearPacketAccessCache();
        fillBufferIfNeeded(StreamTypeVideo);
    }
}

static void preparedCallback(void* data)
{
    PLAYER_LOGI("MediaPlayerTizen::player_prepare_async_cb (MSE)\n");
    MediaPlayerTizen* self = (MediaPlayerTizen*)data;
    self->handlePrepared();
}

#ifdef STARFISH_RUN_MSE_THREAD
static void* threadFillingBuffer(void* data)
{
    MediaPlayerTizenTV* self = (MediaPlayerTizenTV*)data;
    bool* playerDeadFlag = self->m_playerDeadFlag;
    while (!(*playerDeadFlag)) {
        if (self->playbackState() != MediaPlayer::PLAYBACK_STATE_END) {
            MediaStream* audioStream = self->currentStream(StreamTypeAudio);
            MediaStream* videoStream = self->currentStream(StreamTypeVideo);
            if (!audioStream->waitingDemuxer() && audioStream->needPacket()) {
                PLAYER_LOGI("[AUDIO] Player need data\n");
                self->fillBuffer(audioStream);
            }
            if (!videoStream->waitingDemuxer() && videoStream->needPacket()) {
                PLAYER_LOGI("[VIDEO] Player need data\n");
                self->fillBuffer(videoStream);
            }
        }
        sleep(1);
    }
    free(playerDeadFlag);
    return nullptr;
}
#endif

void MediaPlayerTizenTV::prepareMediaSource()
{
    PLAYER_LOGI("MediaPlayerTizenTV::prepareMediaSource\n");
    initAudioStreamInfo();
    if (m_foundError) {
        return;
    }
    initVideoStreamInfo();
    if (m_foundError) {
        return;
    }

    m_container->mediaPlayerNotifyUpdateReadyStateItsContainer(
        HTMLMediaElement::HAVE_METADATA);
    openPreparingMode();
    int ret = player_prepare_async(m_nativePlayer, preparedCallback, this);
    if (ret != PLAYER_ERROR_NONE) {
        PLAYER_LOGE(
            "MediaPlayerTizenTV:: player_prepare_async return error !!!\n");
        printNativePlayerError(ret);
        m_foundError = true;
        handlePrepared();
    }
#ifdef STARFISH_RUN_MSE_THREAD
    else {
        m_playerDeadFlag = (bool*)malloc(sizeof(bool));
        *m_playerDeadFlag = false;
        Thread* t = new Thread(m_container->starFish());
        t->run(m_container->starFish()->messageLoop(), threadFillingBuffer,
               this);
    }
#endif
    PLAYER_LOGI("MediaPlayerTizenTV::prepareMediaSource end\n");
}

#define RETURN_WHEN_PLAYER_ERROR(...) \
    if (ret != PLAYER_ERROR_NONE) {   \
        PLAYER_LOGE(__VA_ARGS__);     \
        printNativePlayerError(ret);  \
        handlePlayerError();          \
        return;                       \
    }

void MediaPlayerTizenTV::initVideoStreamInfo(size_t initSegmentIndex)
{
    SourceBuffer* sb = m_activeMediaSource->activeVideoSourceBuffer();
    if (!sb) {
        return;
    }

    // set video options
    PLAYER_LOGI("MediaPlayerTizenTV::initVideoStreamInfo\n");

    m_videoStream = new MediaStream(StreamTypeVideo);
    if (m_container) {
        m_videoStream->setLastSubmittedDTS(
            m_container->defaultPlaybackStartPosition() * 1000);
    }
    if (!m_videoStream->createMediaFormat()) {
        handlePlayerError();
        return;
    }
    media_format_h mediaFormat = m_videoStream->mediaFormat();
    auto mediaFormatExtra = m_videoStream->videoFormatExtra();
    // Get info from demuxer
    StreamInfo* info = sb->streamInfo(
        initSegmentIndex, m_activeMediaSource->activeVideoStreamIndex());
    // Get mimetype
    if (info->isCodec(MediaCodecVideoH264)) {
        media_format_set_video_mime(mediaFormat, MEDIA_FORMAT_H264_SP);
    } else if (info->isCodec(MediaCodecVideoVP9)) {
        media_format_set_video_mime(mediaFormat, MEDIA_FORMAT_VP9);
    } else {
        // TODO
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    m_videoWidth = info->videoWidth();
    m_videoHeight = info->videoHeight();
    media_format_set_video_width(mediaFormat, m_videoWidth);
    media_format_set_video_height(mediaFormat, m_videoHeight);
    mediaFormatExtra->max_width = STARFISH_VIDEO_MAX_WIDTH;
    mediaFormatExtra->max_height = STARFISH_VIDEO_MAX_HEIGHT;
    m_videoStream->setMaxBufferSize(
        (m_videoWidth * m_videoHeight * 30 * 2 * 7) / 100 / 8 * 5);

    if (info->videoHasFramerate() && info->videoFramerate().isValid()) {
        mediaFormatExtra->framerate_num = info->videoFramerate().m_num;
        mediaFormatExtra->framerate_den = info->videoFramerate().m_den;
    } else {
        mediaFormatExtra->framerate_num = STARFISH_VIDEO_DEFAULT_FRAMERATE_NUM;
        mediaFormatExtra->framerate_den = STARFISH_VIDEO_DEFAULT_FRAMERATE_DEN;
    }
    mediaFormatExtra->hdr_mode = MEDIA_STREAM_HDR_TYPE_MATROSKA;
    mediaFormatExtra->hdr_info = std::string().c_str();
    mediaFormatExtra->is_framerate_changed = false;
    // TODO PLAYER_DRM_TYPE_EME
    mediaFormatExtra->drm_type = PLAYER_DRM_TYPE_NONE;

    // Extra data
    mediaFormatExtra->extradata_size = info->m_extraData.size();
    if (mediaFormatExtra->extradata_size != 0) {
        mediaFormatExtra->codec_extradata =
            (unsigned char*)malloc(mediaFormatExtra->extradata_size);
        memcpy(mediaFormatExtra->codec_extradata, info->m_extraData.data(),
               mediaFormatExtra->extradata_size);
    }

    PLAYER_LOGI("Video Info-----------------------------\n");
    PLAYER_LOGI("> codec     : %s\n", info->codecString());
    PLAYER_LOGI("> framerate : %d/%d\n", mediaFormatExtra->framerate_num,
                mediaFormatExtra->framerate_den);
    PLAYER_LOGI("> size      : %dx%d\n", info->videoWidth(),
                info->videoHeight());
    PLAYER_LOGI("> max_buffer: %llu\n", m_videoStream->maxBufferSize());
    PLAYER_LOGI("---------------------------------------\n");

    player_set_media_stream_buffer_min_threshold(m_nativePlayer,
                                                 PLAYER_STREAM_TYPE_VIDEO, 100);
    int ret = player_set_media_stream_buffer_status_cb_ex(
        m_nativePlayer, PLAYER_STREAM_TYPE_VIDEO,
        [](player_media_stream_buffer_status_e status, unsigned long long bytes,
           void* user_data) {
            MediaPlayerTizenTV* self = (MediaPlayerTizenTV*)user_data;
            self->handlePlayerBuffer(StreamTypeVideo, bytes);
        },
        this);
    RETURN_WHEN_PLAYER_ERROR(
        "ERROR: player_set_media_stream_buffer_status_cb_ex\n");

    media_format_set_extra(mediaFormat, mediaFormatExtra);
    ret = player_set_media_stream_info(m_nativePlayer, PLAYER_STREAM_TYPE_VIDEO,
                                       mediaFormat);
    RETURN_WHEN_PLAYER_ERROR("ERROR: player_set_media_stream_info\n");

    // TODO Replace test value to real estimate value
    ret = player_set_media_stream_buffer_max_size(
        m_nativePlayer, PLAYER_STREAM_TYPE_VIDEO,
        m_videoStream->maxBufferSize());
    RETURN_WHEN_PLAYER_ERROR(
        "ERROR: player_set_media_stream_buffer_max_size\n");
    m_videoStream->setInitSegmentIndex(initSegmentIndex);
}

void MediaPlayerTizenTV::initAudioStreamInfo(size_t initSegmentIndex)
{
    SourceBuffer* sb = m_activeMediaSource->activeAudioSourceBuffer();
    if (!sb) {
        return;
    }
    // set audio options
    PLAYER_LOGI("MediaPlayerTizenTV::initAudioStreamInfo\n");

    m_audioStream = new MediaStream(StreamTypeAudio);
    if (m_container) {
        m_audioStream->setLastSubmittedDTS(
            m_container->defaultPlaybackStartPosition() * 1000);
    }
    if (!m_audioStream->createMediaFormat()) {
        handlePlayerError();
        return;
    }
    media_format_h mediaFormat = m_audioStream->mediaFormat();
    auto mediaFormatExtra = m_audioStream->audioFormatExtra();

    // Get info from demuxer
    StreamInfo* info = sb->streamInfo(
        initSegmentIndex, m_activeMediaSource->activeAudioStreamIndex());

    if (info->isCodec(MediaCodecAudioAAC)) {
        media_format_set_audio_mime(mediaFormat, MEDIA_FORMAT_AAC);
    } else if (info->isCodec(MediaCodecAudioVorbis)) {
        media_format_set_audio_mime(mediaFormat, MEDIA_FORMAT_VORBIS);
    } else {
        // TODO
        media_format_set_audio_mime(mediaFormat, MEDIA_FORMAT_MP3);
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    media_format_set_audio_channel(mediaFormat, (int)info->audioChannels());
    media_format_set_audio_samplerate(mediaFormat,
                                      (int)info->audioSampleRate());
    // media_format_set_audio_avg_bps(m_audioFormat, audioCodecCtx->bit_rate);

    mediaFormatExtra->extradata_size = info->m_extraData.size();
    if (mediaFormatExtra->extradata_size != 0) {
        mediaFormatExtra->codec_extradata =
            (unsigned char*)malloc(mediaFormatExtra->extradata_size);
        memcpy(mediaFormatExtra->codec_extradata, info->m_extraData.data(),
               mediaFormatExtra->extradata_size);
    }
    // TODO PLAYER_DRM_TYPE_EME
    mediaFormatExtra->drm_type = PLAYER_DRM_TYPE_NONE;

    PLAYER_LOGI("Audio Info-----------------------------\n");
    PLAYER_LOGI("> codec     : %s\n", info->codecString());
    PLAYER_LOGI("> channels  : %d\n", (int)info->audioChannels());
    PLAYER_LOGI("> sample_rate : %d\n", (int)info->audioSampleRate());
    PLAYER_LOGI("> extradata : %d\n", (int)info->m_extraData.size());
    PLAYER_LOGI("---------------------------------------\n");

    player_set_media_stream_buffer_min_threshold(m_nativePlayer,
                                                 PLAYER_STREAM_TYPE_AUDIO, 100);
    int ret = player_set_media_stream_buffer_status_cb_ex(
        m_nativePlayer, PLAYER_STREAM_TYPE_AUDIO,
        [](player_media_stream_buffer_status_e status, unsigned long long bytes,
           void* user_data) {
            MediaPlayerTizenTV* self = (MediaPlayerTizenTV*)user_data;
            self->handlePlayerBuffer(StreamTypeAudio, bytes);
        },
        this);
    RETURN_WHEN_PLAYER_ERROR(
        "ERROR: player_set_media_stream_buffer_status_cb_ex\n");

    media_format_set_extra(mediaFormat, mediaFormatExtra);
    ret = player_set_media_stream_info(m_nativePlayer, PLAYER_STREAM_TYPE_AUDIO,
                                       mediaFormat);
    RETURN_WHEN_PLAYER_ERROR("ERROR: player_set_media_stream_info\n");

    ret = player_set_media_stream_buffer_max_size(
        m_nativePlayer, PLAYER_STREAM_TYPE_AUDIO,
        m_audioStream->maxBufferSize());
    RETURN_WHEN_PLAYER_ERROR(
        "ERROR: player_set_media_stream_buffer_max_size\n");

    m_audioStream->setInitSegmentIndex(initSegmentIndex);
}
#undef RETURN_WHEN_PLAYER_ERROR

MediaPlayer* MediaPlayer::create(HTMLMediaElement* element)
{
    return new MediaPlayerTizenTV(element);
}
}

#endif
#endif
