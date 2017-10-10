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

extern "C" {
#include <libavformat/avformat.h>
}

#include "StarFishConfig.h"
#include "core/util/URL.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLVideoElement.h"
#include "MediaPlayerTizenTV.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/mediasource/MediaSource.h"
#include "core/modules/mediasource/SourceBuffer.h"
#include "core/modules/threading/Locker.h"

namespace StarFish {

struct FFMpegIOContext {
    FFMpegIOContext(const GCVector<uint8_t>& buf)
        : m_readPos(0)
        , m_buffer(buf)
    {
    }
    size_t m_readPos;
    const GCVector<uint8_t>& m_buffer;
};

static int FFMpegIOContextReadCallback(void* opaque, uint8_t* buf, int buf_size)
{
    FFMpegIOContext* self = (FFMpegIOContext*)opaque;
    int sizeSuccessToRead = buf_size;

    if (sizeSuccessToRead + self->m_readPos > self->m_buffer.size()) {
        sizeSuccessToRead = self->m_buffer.size() - self->m_readPos;
    }

    memcpy(buf, self->m_buffer.data() + self->m_readPos, sizeSuccessToRead);
    self->m_readPos += sizeSuccessToRead;

    return sizeSuccessToRead;
}

static int64_t FFMpegIOContextSeekCallback(void* opaque, int64_t offset,
                                           int whence)
{
    FFMpegIOContext* self = (FFMpegIOContext*)opaque;
    if (whence == AVSEEK_SIZE) {
        return self->m_buffer.size();
    } else if (whence == SEEK_CUR) {
        self->m_readPos += offset;
    } else if (whence == SEEK_SET) {
        self->m_readPos = offset;
    } else if (whence == SEEK_END) {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
        return -1;
    }
    return self->m_readPos;
}

#define STARFISH_VIDEO_MAX_WIDTH 1920
#define STARFISH_VIDEO_MAX_HEIGHT 1080
#define STARFISH_VIDEO_DEFAULT_FRAMERATE_NUM 2997
#define STARFISH_VIDEO_DEFAULT_FRAMERATE_DEN 100

#define RETURN_WHEN_PLAYER_ERROR(...) \
    if (ret != PLAYER_ERROR_NONE) {   \
        PLAYER_LOGE(__VA_ARGS__);     \
        printNativePlayerError(ret);  \
        handlePlayerError();          \
        return;                       \
    }

static int getBufferLevel(uint64_t current, uint64_t total)
{
    return 0;
}

void MediaPlayerTizenTV::setVideoStreamInfoWithGuard(size_t initSegmentIndex)
{
    if (!m_activeMediaSource->activeVideoSourceBuffer()) {
        return;
    }
    Locker<Mutex> locker(*m_mediaFormatMutex);

    // set video options
    PLAYER_LOGI("MediaPlayerTizenTV::setVideoStreamInfoWithGuard\n");

    STARFISH_RELEASE_ASSERT(m_videoFormat == nullptr);
    int ret = media_format_create(&m_videoFormat);
    if (ret != MEDIA_FORMAT_ERROR_NONE) {
        printMediaFormatError(ret);
        handlePlayerError();
        return;
    }
    if (m_videoFormatExtra.codec_extradata != nullptr) {
        free(m_videoFormatExtra.codec_extradata);
    }
    memset(&m_videoFormatExtra, 0,
           sizeof(player_media_stream_video_extra_info_s));

    // Get info from demuxer
    StreamInfo* info =
        m_activeMediaSource->activeVideoSourceBuffer()->streamInfo(
            0, m_activeMediaSource->activeVideoStreamIndex());
    // Get mimetype
    if (info->isCodec(MediaCodecVideoH264)) {
        media_format_set_video_mime(m_videoFormat, MEDIA_FORMAT_H264_SP);
    } else if (info->isCodec(MediaCodecVideoVP9)) {
        media_format_set_video_mime(m_videoFormat, MEDIA_FORMAT_VP9);
    } else {
        // TODO
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    m_videoWidth = info->videoWidth();
    m_videoHeight = info->videoHeight();
    media_format_set_video_width(m_videoFormat, m_videoWidth);
    media_format_set_video_height(m_videoFormat, m_videoHeight);
    m_videoFormatExtra.max_width = STARFISH_VIDEO_MAX_WIDTH;
    m_videoFormatExtra.max_height = STARFISH_VIDEO_MAX_HEIGHT;
    m_videoMaxBufferSize =
        (m_videoWidth * m_videoHeight * 30 * 2 * 7) / 100 / 8 * 5;

    if (info->videoHasFramerate() && info->videoFramerate().isValid()) {
        m_videoFormatExtra.framerate_num = info->videoFramerate().m_num;
        m_videoFormatExtra.framerate_den = info->videoFramerate().m_den;
    } else {
        m_videoFormatExtra.framerate_num = STARFISH_VIDEO_DEFAULT_FRAMERATE_NUM;
        m_videoFormatExtra.framerate_den = STARFISH_VIDEO_DEFAULT_FRAMERATE_DEN;
    }
    m_videoFormatExtra.hdr_mode = MEDIA_STREAM_HDR_TYPE_MATROSKA;
    m_videoFormatExtra.hdr_info = std::string().c_str();
    m_videoFormatExtra.is_framerate_changed = false;
    // TODO PLAYER_DRM_TYPE_EME
    m_videoFormatExtra.drm_type = PLAYER_DRM_TYPE_NONE;

    // Extra data
    m_videoFormatExtra.extradata_size = info->m_extraData.size();
    if (m_videoFormatExtra.extradata_size != 0) {
        m_videoFormatExtra.codec_extradata =
            (unsigned char*)malloc(m_videoFormatExtra.extradata_size);
        memcpy(m_videoFormatExtra.codec_extradata, info->m_extraData.data(),
               m_videoFormatExtra.extradata_size);
    }

    PLAYER_LOGI("Video Info-----------------------------\n");
    PLAYER_LOGI("> codec     : %s\n", info->codecString());
    PLAYER_LOGI("> framerate : %d/%d\n", m_videoFormatExtra.framerate_num,
                m_videoFormatExtra.framerate_den);
    PLAYER_LOGI("> size      : %dx%d\n", info->videoWidth(),
                info->videoHeight());
    PLAYER_LOGI("> max_buffer: %llu\n", m_videoMaxBufferSize);
    PLAYER_LOGI("---------------------------------------\n");

    player_set_media_stream_buffer_min_threshold(m_nativePlayer,
                                                 PLAYER_STREAM_TYPE_VIDEO, 100);
    ret = player_set_media_stream_buffer_status_cb_ex(
        m_nativePlayer, PLAYER_STREAM_TYPE_VIDEO,
        [](player_media_stream_buffer_status_e status, unsigned long long bytes,
           void* user_data) {
            MediaPlayerTizenTV* self = (MediaPlayerTizenTV*)user_data;
            self->handlePlayerBuffer(StreamTypeVideo, bytes);
        },
        this);
    RETURN_WHEN_PLAYER_ERROR(
        "ERROR: player_set_media_stream_buffer_status_cb_ex\n");

    media_format_set_extra(m_videoFormat, &m_videoFormatExtra);
    ret = player_set_media_stream_info(m_nativePlayer, PLAYER_STREAM_TYPE_VIDEO,
                                       m_videoFormat);
    RETURN_WHEN_PLAYER_ERROR("ERROR: player_set_media_stream_info\n");

    // TODO Replace test value to real estimate value
    ret = player_set_media_stream_buffer_max_size(
        m_nativePlayer, PLAYER_STREAM_TYPE_VIDEO, m_videoMaxBufferSize);
    RETURN_WHEN_PLAYER_ERROR(
        "ERROR: player_set_media_stream_buffer_max_size\n");

    m_videoInitSegmentIndex = initSegmentIndex;
}

void MediaPlayerTizenTV::setAudioStreamInfoWithGuard(size_t initSegmentIndex)
{
    if (!m_activeMediaSource->activeAudioSourceBuffer()) {
        return;
    }
    Locker<Mutex> locker(*m_mediaFormatMutex);
    // set audio options
    PLAYER_LOGI("MediaPlayerTizenTV::setAudioStreamInfoWithGuard\n");
    STARFISH_RELEASE_ASSERT(m_audioFormat == nullptr);
    int ret = media_format_create(&m_audioFormat);
    if (ret != MEDIA_FORMAT_ERROR_NONE) {
        printMediaFormatError(ret);
        handlePlayerError();
        return;
    }
    if (m_audioFormatExtra.codec_extradata != nullptr) {
        free(m_audioFormatExtra.codec_extradata);
    }
    memset(&m_audioFormatExtra, 0,
           sizeof(player_media_stream_audio_extra_info_s));

    // Get info from demuxer
    StreamInfo* info =
        m_activeMediaSource->activeAudioSourceBuffer()->streamInfo(
            initSegmentIndex, m_activeMediaSource->activeAudioStreamIndex());

    if (info->isCodec(MediaCodecAudioAAC)) {
        media_format_set_audio_mime(m_audioFormat, MEDIA_FORMAT_AAC);
    } else if (info->isCodec(MediaCodecAudioVorbis)) {
        media_format_set_audio_mime(m_audioFormat, MEDIA_FORMAT_VORBIS);
    } else {
        // TODO
        media_format_set_audio_mime(m_audioFormat, MEDIA_FORMAT_MP3);
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    media_format_set_audio_channel(m_audioFormat, (int)info->audioChannels());
    media_format_set_audio_samplerate(m_audioFormat,
                                      (int)info->audioSampleRate());
    // media_format_set_audio_avg_bps(m_audioFormat, audioCodecCtx->bit_rate);

    m_audioFormatExtra.extradata_size = info->m_extraData.size();
    if (m_audioFormatExtra.extradata_size != 0) {
        m_audioFormatExtra.codec_extradata =
            (unsigned char*)malloc(m_audioFormatExtra.extradata_size);
        memcpy(m_audioFormatExtra.codec_extradata, info->m_extraData.data(),
               m_audioFormatExtra.extradata_size);
    }
    // TODO PLAYER_DRM_TYPE_EME
    m_audioFormatExtra.drm_type = PLAYER_DRM_TYPE_NONE;

    PLAYER_LOGI("Audio Info-----------------------------\n");
    PLAYER_LOGI("> codec     : %s\n", info->codecString());
    PLAYER_LOGI("> channels  : %d\n", (int)info->audioChannels());
    PLAYER_LOGI("> sample_rate : %d\n", (int)info->audioSampleRate());
    PLAYER_LOGI("> extradata : %d\n", (int)info->m_extraData.size());
    PLAYER_LOGI("---------------------------------------\n");

    player_set_media_stream_buffer_min_threshold(m_nativePlayer,
                                                 PLAYER_STREAM_TYPE_AUDIO, 100);
    ret = player_set_media_stream_buffer_status_cb_ex(
        m_nativePlayer, PLAYER_STREAM_TYPE_AUDIO,
        [](player_media_stream_buffer_status_e status, unsigned long long bytes,
           void* user_data) {
            MediaPlayerTizenTV* self = (MediaPlayerTizenTV*)user_data;
            self->handlePlayerBuffer(StreamTypeAudio, bytes);
        },
        this);
    RETURN_WHEN_PLAYER_ERROR(
        "ERROR: player_set_media_stream_buffer_status_cb_ex\n");

    media_format_set_extra(m_audioFormat, &m_audioFormatExtra);
    ret = player_set_media_stream_info(m_nativePlayer, PLAYER_STREAM_TYPE_AUDIO,
                                       m_audioFormat);
    RETURN_WHEN_PLAYER_ERROR("ERROR: player_set_media_stream_info\n");

    ret = player_set_media_stream_buffer_max_size(
        m_nativePlayer, PLAYER_STREAM_TYPE_AUDIO, m_audioMaxBufferSize);
    RETURN_WHEN_PLAYER_ERROR(
        "ERROR: player_set_media_stream_buffer_max_size\n");

    m_audioInitSegmentIndex = initSegmentIndex;
}
}
#undef RETURN_WHEN_PLAYER_ERROR
#endif
#endif
