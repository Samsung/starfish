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
#ifdef STARFISH_TIZEN_TV

extern "C" {
#include <libavformat/avformat.h>
}

#include "StarFishConfig.h"
#include "util/URL.h"
#include "dom/Document.h"
#include "dom/HTMLVideoElement.h"
#include "MediaPlayerTizen.h"
#include "platform/canvas/Canvas.h"
#include "platform/message_loop/MessageLoop.h"
#include "extra/MediaSource.h"
#include "extra/SourceBuffer.h"

#include <Elementary.h>

#include <media/player.h>
// #include <media/player_product.h>


namespace StarFish {

struct FFMpegIOContext {
    FFMpegIOContext(const std::vector<uint8_t, gc_allocator<uint8_t>>& buf)
        : m_readPos(0)
        , m_buffer(buf)
    { }
    size_t m_readPos;
    const std::vector<uint8_t, gc_allocator<uint8_t>>& m_buffer;
};

static int FFMpegIOContextReadCallback(void *opaque, uint8_t *buf, int buf_size)
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

static int64_t FFMpegIOContextSeekCallback(void *opaque, int64_t offset, int whence)
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


class MediaPlayerTizenTV : public MediaPlayerTizen {
public:
    MediaPlayerTizenTV(HTMLMediaElement* element)
        : MediaPlayerTizen(element)
    {
        m_lastVideoPts = m_lastAudioPts = 0;
    }

    virtual void initDisplay()
    {
    }

    virtual void setNativePlayerDefaultOptions(URL* url)
    {
        if (m_container->isHTMLVideoElement() && m_container->frame()) {
            player_display_h displayHandle = GET_DISPLAY(elm_win_xwindow_get((Evas_Object*) m_container->document()->window()->unwrap()));
            player_display_type_e displayType = PLAYER_DISPLAY_TYPE_X11;
            player_display_mode_e displayMode = PLAYER_DISPLAY_MODE_DST_ROI;
            player_display_roi_mode_e roiMode = PLAYER_DISPLAY_ROI_MODE_LETTER_BOX;

            player_set_display(m_nativePlayer, displayType, displayHandle);
            player_set_display_mode(m_nativePlayer, displayMode);
            player_set_x11_display_roi_mode(m_nativePlayer, roiMode);
            // TODO #include <media/player_product.h>
            // player_display_video_at_paused_state(m_nativePlayer, TRUE);

            if (url->isNetworkURL()) {
                // TODO #include <media/player_product.h>
                // player_set_streaming_type(m_nativePlayer, const_cast<char*>("FFMPEG_HTTP"));
            }
        }
    }

    virtual void prepareMediaSource()
    {
        STARFISH_LOG_INFO("prepareMediaSource\n");
        player_set_uri(m_nativePlayer, "external_demuxer://aaaa");

        av_register_all();
        avcodec_register_all();
        avformat_network_init();

        // set video options
        player_video_stream_info_s videoInfo;
        if (m_activeMediaSource->activeVideoSourceBuffer()) {
            STARFISH_LOG_INFO("MSE set Video\n");
            VideoStreamInfo* info = (VideoStreamInfo*)(m_activeMediaSource->activeVideoSourceBuffer()->streamInfo()[m_activeMediaSource->activeVideoStreamInSourceBuffer()]);
            memset(&videoInfo, 0, sizeof(player_video_stream_info_s));

            const char* mediaFormat = "";
            if (strstr(info->m_codecName, "h264")) {
                videoInfo.mime = "video/x-h264";
                // TODO find container type
                mediaFormat = "mp4";
            } else if (strstr(info->m_codecName, "vp9")) {
                videoInfo.mime = "video/x-vp9";
                // TODO find container type
                mediaFormat = "webm";
            } else {
                // TODO
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            videoInfo.width = info->m_width;
            videoInfo.height = info->m_height;
            videoInfo.framerate_den = info->m_timeBaseDen;
            videoInfo.framerate_num = info->m_timeBaseNum;

            uint8_t* bufferForIO = (uint8_t*)av_malloc(4096);
            FFMpegIOContext ctx(m_activeMediaSource->activeVideoSourceBuffer()->bufferHeader());
            AVIOContext* ioContext = avio_alloc_context(bufferForIO, 4096, 0, &ctx, FFMpegIOContextReadCallback, nullptr, FFMpegIOContextSeekCallback);
            AVFormatContext* fc = avformat_alloc_context();
            fc->flags = AVFMT_FLAG_CUSTOM_IO;
            fc->pb = ioContext;
            fc->iformat = av_find_input_format(mediaFormat);
            STARFISH_RELEASE_ASSERT(avformat_open_input(&fc, NULL, NULL, NULL) == 0);
            videoInfo.codec_extradata = fc->streams[m_activeMediaSource->activeVideoStreamInSourceBuffer()]->codec->extradata;
            videoInfo.extradata_size = fc->streams[m_activeMediaSource->activeVideoStreamInSourceBuffer()]->codec->extradata_size;

            int ret = player_set_video_stream_info(m_nativePlayer, &videoInfo);
            STARFISH_RELEASE_ASSERT(ret == 0);

            avformat_close_input(&fc);
            av_free(bufferForIO);
            av_free(ioContext);
        }

        // set audio options
        player_audio_stream_info_s audioInfo;
        if (m_activeMediaSource->activeAudioSourceBuffer()) {
            STARFISH_LOG_INFO("MSE set Audio\n");
            memset(&audioInfo, 0, sizeof(player_audio_stream_info_s));

            StreamInfo* info = (m_activeMediaSource->activeAudioSourceBuffer()->streamInfo()[m_activeMediaSource->activeAudioStreamInSourceBuffer()]);

            const char* mediaFormat = "";
            if (strstr(info->m_codecName, "aac")) {
                audioInfo.mime = "audio/mpeg";
                // TODO find container type
                mediaFormat = "mp4";
            } else if (strstr(info->m_codecName, "vorbis")) {
                audioInfo.mime = "audio/x-vorbis";
                // TODO find container type
                mediaFormat = "webm";
            } else {
                // TODO
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }

            uint8_t* bufferForIO = (uint8_t*)av_malloc(4096);
            FFMpegIOContext ctx(m_activeMediaSource->activeAudioSourceBuffer()->bufferHeader());
            AVIOContext* ioContext = avio_alloc_context(bufferForIO, 4096, 0, &ctx, FFMpegIOContextReadCallback, nullptr, FFMpegIOContextSeekCallback);
            AVFormatContext* fc = avformat_alloc_context();
            fc->flags = AVFMT_FLAG_CUSTOM_IO;
            fc->pb = ioContext;
            fc->iformat = av_find_input_format(mediaFormat);
            STARFISH_RELEASE_ASSERT(avformat_open_input(&fc, NULL, NULL, NULL) == 0);

            AVStream* audioStream = fc->streams[m_activeMediaSource->activeAudioStreamInSourceBuffer()];
            AVCodecContext* audioCodecCtx = fc->streams[m_activeMediaSource->activeAudioStreamInSourceBuffer()]->codec;
            audioInfo.channels = audioCodecCtx->channels;
            audioInfo.sample_rate = audioCodecCtx->sample_rate;
            audioInfo.bit_rate = audioCodecCtx->bit_rate;
            audioInfo.version = 2;
            audioInfo.user_info = 0;
            audioInfo.codec_extradata = audioCodecCtx->extradata;
            audioInfo.extradata_size = audioCodecCtx->extradata_size;

            int ret = player_set_audio_stream_info(m_nativePlayer, &audioInfo);
            STARFISH_RELEASE_ASSERT(ret == 0);

            avformat_close_input(&fc);
            av_free(bufferForIO);
            av_free(ioContext);
        }

        player_set_buffer_need_video_data_cb(m_nativePlayer, [](unsigned int size, void *user_data)
        {
            STARFISH_LOG_INFO("videoPlayerBufferNeedVideoDataCB called\n");
            MediaPlayerTizenTV* self = (MediaPlayerTizenTV*)user_data;
            uint64_t ptsStart = self->m_lastVideoPts;
            uint64_t ptsNow = self->m_lastVideoPts;
            while (ptsNow - ptsStart < 1000) {
                MediaPacket* packet = self->m_activeMediaSource->activeVideoSourceBuffer()->findProperMediaPacket(self->m_activeMediaSource->activeVideoStreamIndex(), self->m_lastVideoPts);
                if (!packet) {
                    break;
                }
                ptsNow = packet->m_pts;
                self->m_lastVideoPts = packet->m_pts + 1;
                int ret = player_submit_packet(self->m_nativePlayer, packet->m_data, packet->m_dataSize, packet->m_pts, PLAYER_TRACK_TYPE_VIDEO);

                if (ret != PLAYER_ERROR_NONE) {
                    printf("**ERROR: player_submit_packet %x", ret);
                }
                // printf("push packet %d %p %d\n", (int)packet->m_pts, packet->m_data, (int)packet->m_dataSize);
            }
            // STARFISH_LOG_INFO("videoPlayerBufferNeedVideoDataCB called end\n");
        }, this);
        player_set_buffer_need_audio_data_cb(m_nativePlayer, [](unsigned int size, void *user_data)
        {
            STARFISH_LOG_INFO("videoPlayerBufferNeedAudioDataCB called\n");
            MediaPlayerTizenTV* self = (MediaPlayerTizenTV*)user_data;
            uint64_t ptsStart = self->m_lastAudioPts;
            uint64_t ptsNow = self->m_lastAudioPts;
            while (ptsNow - ptsStart < 1000) {
                MediaPacket* packet = self->m_activeMediaSource->activeAudioSourceBuffer()->findProperMediaPacket(self->m_activeMediaSource->activeAudioStreamIndex(), self->m_lastAudioPts);
                if (!packet) {
                    break;
                }
                ptsNow = packet->m_pts;
                self->m_lastAudioPts = packet->m_pts + 1;
                int ret = player_submit_packet(self->m_nativePlayer, packet->m_data, packet->m_dataSize, packet->m_pts, PLAYER_TRACK_TYPE_AUDIO);

                if (ret != PLAYER_ERROR_NONE) {
                    printf("**ERROR: player_submit_packet %x", ret);
                }
                // printf("push packet %d %p %d\n", (int)packet->m_pts, packet->m_data, (int)packet->m_dataSize);
            }
        }, this);

        openPreparingMode();

        int nativeResult = player_prepare_async(m_nativePlayer, [](void *user_data) {
            STARFISH_LOG_INFO("MediaPlayerPlayerMSEprepared\n");
            MediaPlayerTizenTV* self = (MediaPlayerTizenTV*)user_data;
            self->compleatePrepare();
        }, this);

        if (nativeResult != PLAYER_ERROR_NONE) {
            STARFISH_LOG_ERROR("player_prepare_async return error !!!\n");
            STARFISH_ASSERT_NOT_REACHED();

            STARFISH_ASSERT(m_inPrepare);
            closePreparingMode();
        }

        STARFISH_LOG_INFO("prepareMediaSourceEnd\n");
    }

    virtual void drawVideo(Canvas* canvas, const LayoutRect& videoRect, const LayoutRect& absVideoRect)
    {
        canvas->punchHole(Rect(videoRect.x(), videoRect.y(), videoRect.width(), videoRect.height()));
        player_set_x11_display_dst_roi(m_nativePlayer, absVideoRect.x(), absVideoRect.y(), absVideoRect.width(), absVideoRect.height());
    }

    virtual double currentTime()
    {
        int s;
        int ret = player_get_position(m_nativePlayer, &s);
        if (ret)
            return 0;
        return s / 1000.0;
    }

    uint64_t m_lastVideoPts;
    uint64_t m_lastAudioPts;
};

MediaPlayer* MediaPlayer::create(HTMLMediaElement* element)
{
    return new MediaPlayerTizenTV(element);
}

}

#endif
#endif
