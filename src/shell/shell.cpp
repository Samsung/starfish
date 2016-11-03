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

#include "StarFishConfig.h"
#include "dom/Document.h"
#include "StarFish.h"

#include "dom/binding/ScriptBindingInstance.h"
#include "platform/message_loop/MessageLoop.h"
#include "platform/multimedia/Demuxer.h"
#include "StarFishPublic.h"

#include <pthread.h>
#include <Elementary.h>


/*
#include <player.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}
*/
/*
// mobile
#include <media/player.h>
#include <media/media_format.h>
#include <media/media_packet.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

media_format_h mediaFormat;
*/

// player_h player;

using namespace StarFish;

/*
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

class DemuxerMemorySource : public DemuxerSource {
public:
    DemuxerMemorySource()
    {
        readPos = 0;
    }

    virtual int64_t onSeek(int64_t position, SeekWhence whence)
    {
        if (whence == DemuxerSource::SeekWhenceLookSize) {
            return data.size();
        } else if (whence == DemuxerSource::SeekWhenceSet) {
            // STARFISH_LOG_INFO("onSeek DemuxerSource::SeekWhenceSet %d\n", (int)position);
            STARFISH_ASSERT((int)position <= (int)data.size());
            readPos = position;
            return readPos;
        } else if (whence == DemuxerSource::SeekWhenceCurrent) {
            readPos = readPos + position;
            return readPos;
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    virtual void onRead(size_t sizeWantToRead, size_t& sizeSuccessToRead, int& error, uint8_t* buffer)
    {
        error = 0;
        int readed = 0;
        int end = readPos + sizeWantToRead;
        if ((int)end > (int)data.size()) {
            end = data.size();
            error = 1;
        }
        memcpy(buffer, data.data() + readPos, end - readPos);
        sizeSuccessToRead = end - readPos;
        readPos = end;

        // STARFISH_LOG_INFO("onRead pos %d readed %d\n", (int)(readPos - sizeSuccessToRead), (int)sizeSuccessToRead);
    }

    size_t readPos;
    std::vector<uint8_t> data;
};

class DemuxerFileSource : public DemuxerSource {
public:
    DemuxerFileSource(std::string fileName, size_t start = 0, size_t end = 0)
    {
        m_fp = fopen(fileName.c_str(), "rb");
        m_start = start;
        m_end = end;
        fseek(m_fp, start, SEEK_SET);
    }

    virtual int64_t onSeek(int64_t position, SeekWhence whence)
    {
        if (whence == DemuxerSource::SeekWhenceLookSize) {
            int64_t before = ftell(m_fp);
            fseek(m_fp, 0L, SEEK_END);
            int64_t sz = ftell(m_fp);

            fseek(m_fp, before, SEEK_SET);
            STARFISH_LOG_INFO("onSeek DemuxerSource::SeekWhenceLookSize\n");
            return sz - m_start > m_end ? m_end : sz - m_start;
        } else if (whence == DemuxerSource::SeekWhenceSet) {
            STARFISH_LOG_INFO("onSeek DemuxerSource::SeekWhenceSet %d\n", (int)position);
            fseek(m_fp, position + m_start, SEEK_SET);
            return ftell(m_fp) - m_start;
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    virtual void onRead(size_t sizeWantToRead, size_t& sizeSuccessToRead, int& error, uint8_t* buffer)
    {
        int pos = ftell(m_fp);
        if ((size_t)pos + sizeWantToRead > (size_t)m_end) {
            sizeWantToRead = m_end - pos;
        }
        int read = fread(buffer, 1, sizeWantToRead, m_fp);
        sizeSuccessToRead = (size_t)read;
        if (sizeSuccessToRead == 0) {
            error = -1;
        } else {
            error = 0;
        }
        STARFISH_LOG_INFO("onRead pos %d readed %d\n", (int)pos, (int)read);
    }

    FILE* m_fp;
    int64_t m_start, m_end;
};

void copyFileContent(FILE* fp, int start, int end, std::vector<uint8_t>& data)
{
    uint8_t* buf = new uint8_t[end - start];
    fseek(fp, start, SEEK_SET);
    int ret = fread(buf, 1, end - start, fp);

    data.insert(data.end(), &buf[0], &buf[end - start]);
    delete[] buf;
}

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')

class DemuxerTestClient : public DemuxerClient {
public:
    size_t nalSize = 0;
    virtual bool onDetectPacket(const MediaPacket& packet)
    {
        // for (size_t i = 0; i < 32; i ++) { printf("%x ", packet.m_data[i]); } puts("");
        MediaPacket newP;
        newP.m_data = new uint8_t[packet.m_dataSize];
        memcpy(newP.m_data, packet.m_data, packet.m_dataSize);
        newP.m_dataSize = packet.m_dataSize;
        newP.m_pts = packet.m_pts;
        packets.push_back(newP);
        return false;
    }

    std::vector<MediaPacket> packets;
};

void testDemuxer()
{

    Demuxer* demuxer = Demuxer::createMP4Demuxer();
    DemuxerTestClient* c1 = new DemuxerTestClient();

    demuxer->addClient(c1);

    auto ptr = new DemuxerMemorySource();

    FILE* fp = fopen("ob360.mp4", "rb");

    copyFileContent(fp, 0, 65870855, ptr->data);

    if (!demuxer->findStreamInfo(ptr, String::fromUTF8("video/mp4"))) {
        puts("fail0");
        // ptr->m_debug++;
    }
    demuxer->findStreamPacket(ptr);

    printf("avformat setting start\n");
    av_register_all();
    printf("av_register_all done\n");
    avcodec_register_all();
    printf("avcodec_register_all\n");
    avformat_network_init();
    printf("avformat_network_init done\n");

    AVFormatContext* fc = avformat_alloc_context();
    fc->iformat = av_find_input_format("mp4");
    avformat_open_input(&fc, "ob360.mp4", nullptr, nullptr);
    avcodec_open2(fc->streams[0]->codec, avcodec_find_decoder(fc->streams[0]->codec->codec_id), nullptr);
    AVFrame* frame = av_frame_alloc();
    for (size_t i = 0; i < c1->packets.size() && i < 100; i ++) {
        // printf("packet %d\n", (int)i);
        AVPacket pkt;
        av_init_packet(&pkt);
        pkt.data = c1->packets[i].m_data;
        pkt.size = c1->packets[i].m_dataSize;
        int got_picture;
        while (pkt.size > 0) {
            auto len = avcodec_decode_video2(fc->streams[0]->codec, frame, &got_picture, &pkt);
            printf("avcodec_decode_video2 ret %d got %d\n", len, got_picture);
            if (len < 0) {
                fprintf(stderr, "Error while decoding frame\n");
                exit(1);
            }
            pkt.size -= len;
            pkt.data += len;
        }
    }

}
*/

bool hasEnding(std::string const &fullString, std::string const &ending)
{
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

void test(size_t, void* data)
{
    StarFish::StarFish* sf2 = (StarFish::StarFish*)data;
    STARFISH_LOG_INFO("asdf\n");
    ecore_timer_add(0.0001, [](void* data)->Eina_Bool {
        StarFish::StarFish* sf2 = (StarFish::StarFish*)data;
        sf2->messageLoop()->addIdler(test, sf2);
        return ECORE_CALLBACK_CANCEL;
    }, sf2);
}
/*
static void printNativePlayerError(int errorCode)
{
    switch (errorCode) {
#define GEN_ERROR_PRINTS(errorenum) \
    case errorenum: \
        printf("%s\n", #errorenum); \
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
        printf("Unknown error\n");
        return;
    }
}
*/
int main(int argc, char *argv[])
{
    /*
    // elm_config_accel_preference_set("opengl");
    elm_init(0, 0);
    elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
    Evas_Object* wnd = elm_win_add(NULL, "", ELM_WIN_BASIC);

    elm_win_autodel_set(wnd, EINA_TRUE);
    Evas_Object* box = elm_box_add(wnd);
    evas_object_size_hint_weight_set (box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(wnd, box);
    evas_object_show(box);

    evas_object_resize(wnd, 600, 600);
    evas_object_show(wnd);

    Evas_Object* background = elm_bg_add(wnd);
    evas_object_size_hint_weight_set(background, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(wnd, background);
    elm_bg_color_set(background, 0x00, 0xff, 0x00);
    evas_object_show(background);

    Evas_Object* eo = evas_object_rectangle_add(evas_object_evas_get(wnd));
    evas_object_resize(eo, 200, 200);
    evas_object_color_set(eo, 200, 0, 200, 200);
    evas_object_show(eo);

    eo = evas_object_rectangle_add(evas_object_evas_get(wnd));
    evas_object_resize(eo, 100, 100);
    evas_object_move(eo, 50, 50);
    int a=0,r=0,g=0,b=0;
    evas_color_argb_premul(a, &r, &g, &b);
    printf("%d %d %d %d",a,r,g,b);
    evas_object_color_set(eo, r, g, b, a);
    // evas_object_render_op_set(eo, Evas_Render_Op::EVAS_RENDER_);
    evas_object_render_op_set(eo, EVAS_RENDER_COPY);
    evas_object_show(eo);

    elm_run();
    return 0;
*/
#ifndef NDEBUG
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
#endif

    // setenv("ELM_ENGINE", "gl", 1);
    elm_init(0, 0);
    elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
    // tv player test
/*
    Evas_Object* wndObj = elm_win_add(NULL, "StarFish", ELM_WIN_BASIC);
    elm_win_title_set(wndObj, "StarFish");
    elm_win_autodel_set(wndObj, EINA_TRUE);
    evas_object_resize(wndObj, 1280, 720);
    evas_object_show(wndObj);

    Evas_Object* eo = evas_object_rectangle_add(evas_object_evas_get(wndObj));
    evas_object_resize(eo, 1280, 720);
    evas_object_render_op_set(eo, EVAS_RENDER_COPY);
    evas_object_color_set(eo, 0, 0, 0, 0);
    evas_object_show(eo);

    player_create(&player);

    player_set_error_cb(player, [](int error_code, void *user_data) {
        printNativePlayerError(error_code);
    },nullptr);
    int ret;

    player_display_h display_handle = GET_DISPLAY(elm_win_xwindow_get(wndObj));
    player_display_type_e display_type = PLAYER_DISPLAY_TYPE_X11;
    player_display_mode_e display_mode = PLAYER_DISPLAY_MODE_DST_ROI;
    player_display_roi_mode_e roi_mode = PLAYER_DISPLAY_ROI_MODE_LETTER_BOX;
    player_set_display(player, (player_display_type_e) display_type, display_handle);
    player_set_display_mode(player, display_mode);
    player_set_x11_display_roi_mode(player, roi_mode);

    // player_set_uri(player, "car.mp4");

    player_set_uri(player, "external_demuxer://aaaa");

    printf("avformat setting start\n");
    av_register_all();
    printf("av_register_all done\n");
    avcodec_register_all();
    printf("avcodec_register_all\n");
    avformat_network_init();
    printf("avformat_network_init done\n");

    AVFormatContext* fc = avformat_alloc_context();
    fc->iformat = av_find_input_format("mp4");
    avformat_open_input(&fc, "car.mp4", nullptr, nullptr);

    player_video_stream_info_s* videoInfo =
        (player_video_stream_info_s*) malloc(sizeof(player_video_stream_info_s));
    memset(videoInfo, 0, sizeof (player_video_stream_info_s));
    videoInfo->mime = "video/x-h264";
    videoInfo->width = 854;
    videoInfo->height = 480;
    // 23.976
    videoInfo->framerate_den = 1000;
    videoInfo->framerate_num = 23976;
    videoInfo->codec_extradata = fc->streams[0]->codec->extradata;
    videoInfo->extradata_size = fc->streams[0]->codec->extradata_size;

    if(int ret = player_set_video_stream_info(player, videoInfo) != PLAYER_ERROR_NONE) {
        printf(" *** ERROR %x\n", ret);
        return 0;
    }

    player_set_buffer_need_video_data_cb(player, [](unsigned int size, void *d)
    {
        static bool once = false;
        if (once)
            return;
        once = true;
        puts("thread start");
        Demuxer* demuxer = Demuxer::createMP4Demuxer();

        class DemuxerClientTemp : public DemuxerClient {
        public:
            virtual void onDetectVideoStream(const VideoStreamInfo& info)
            {

            }
            virtual void onDetectAudioStream(const AudioStreamInfo& info)
            {

            }
            virtual void onDetectPacket(const MediaPacket& packet)
            {
                int ret = player_submit_packet(player, packet.m_data, packet.m_dataSize, packet.m_pts, PLAYER_TRACK_TYPE_VIDEO);
                if (ret != PLAYER_ERROR_NONE) {
                    printf("**ERROR: player_submit_packet %x", ret);
                }
                printf("push packet %d %p %d\n", (int)packet.m_pts, packet.m_data, (int)packet.m_dataSize);
            }

            DemuxerClientTemp()
            {
            }
        };


        demuxer->addClient(new DemuxerClientTemp());

        auto ptr = new DemuxerMemorySource();
        FILE* fp = fopen("car.mp4", "rb");

        copyFileContent(fp, 0, 708, ptr->data);
        if (!demuxer->findStreamInfo(ptr, String::fromUTF8("video/mp4"))) {
            puts("fail0");
            // ptr->m_debug++;
        }


        auto ptr2 = new DemuxerMemorySource();
        // 1184-727646, 19723193
        // copyFileContent(fp, 1184, 19723193, ptr2->data);
        // 5931518-6320465
        // 1450907
        copyFileContent(fp, 1450907, 6320465 + 1, ptr2->data);
        demuxer->findStreamPacket(ptr2);

        puts("thread end");
    }, nullptr);

    int err = player_prepare_async(player, [](void *user_data) {
        puts("prepared");
        player_start(player);
    }, nullptr);
    if (err != PLAYER_ERROR_NONE) {
        return 0;
    }
    player_set_x11_display_dst_roi(player, 0, 0, 512, 288);
    // free(videoInfo);
     */
    /*
    // mobile test
    ret = player_set_display(player, PLAYER_DISPLAY_TYPE_OVERLAY, GET_DISPLAY(wndObj));
    STARFISH_ASSERT(ret == 0);

    // player_set_uri(player, "car.mp4");
    puts("0");
    media_format_create(&mediaFormat);
    ret = media_format_set_video_mime(mediaFormat, media_format_mimetype_e::MEDIA_FORMAT_H264_MP);
    STARFISH_ASSERT(ret == 0);
    ret = media_format_set_video_width(mediaFormat, 854);
    STARFISH_ASSERT(ret == 0);
    ret = media_format_set_video_height(mediaFormat, 480);
    STARFISH_ASSERT(ret == 0);
    // ret = media_format_set_video_avg_bps(mediaFormat, 867 * 1024);
    // STARFISH_ASSERT(ret == 0);

    ret = player_set_media_stream_buffer_status_cb(player, PLAYER_STREAM_TYPE_VIDEO, [](player_media_stream_buffer_status_e status, void *user_data){}, nullptr);
    STARFISH_ASSERT(ret == 0);
    ret = player_set_media_stream_buffer_status_cb(player, PLAYER_STREAM_TYPE_AUDIO, [](player_media_stream_buffer_status_e status, void *user_data){}, nullptr);
    STARFISH_ASSERT(ret == 0);
    ret = player_set_media_stream_seek_cb(player, PLAYER_STREAM_TYPE_VIDEO, [](unsigned long long offset, void *user_data){}, nullptr);
    STARFISH_ASSERT(ret == 0);
    ret = player_set_media_stream_seek_cb(player, PLAYER_STREAM_TYPE_AUDIO, [](unsigned long long offset, void *user_data){}, nullptr);
    STARFISH_ASSERT(ret == 0);

    ret = player_set_media_packet_video_frame_decoded_cb(player, [](media_packet_h pkt, void *user_data) {
        puts("decoded");
    }, nullptr);
    STARFISH_ASSERT(ret == 0);

    puts("1");
    ret = player_set_media_stream_info(player, PLAYER_STREAM_TYPE_VIDEO, mediaFormat);
    STARFISH_ASSERT(ret == 0);

    puts("2");
    ret = player_prepare_async(player, [](void *user_data) {
        puts("prepare");
        player_h player = (player_h)user_data;
        player_start(player);
    }, player);
    printNativePlayerError(ret);
    STARFISH_ASSERT(ret == 0);

    pthread_t thread;
    pthread_create(&thread, nullptr, [](void* data) -> void* {
        player_h player = (player_h)data;
        puts("thread start");

        Demuxer* demuxer = Demuxer::createMP4Demuxer();

        class DemuxerClientTemp : public DemuxerClient {
        public:
            virtual void onDetectVideoStream(const VideoStreamInfo& info)
            {

            }
            virtual void onDetectAudioStream(const AudioStreamInfo& info)
            {

            }
            virtual void onDetectPacket(const MediaPacket& packet)
            {
                int got_picture;
                AVPacket pkt;
                av_init_packet(&pkt);
                pkt.data = packet.m_data;
                pkt.size = packet.m_dataSize;
                auto ret2 = avcodec_decode_video2(fc->streams[0]->codec, frame, &got_picture, &pkt);
                printf("avcodec_decode_video2 ret %d got %d\n", ret2, got_picture);

                // char error[128];
                // av_strerror(-1052488119, error, 128);
                // puts(error);
                if (!got_picture)
                    return;
                media_packet_h p = 0;
                media_packet_create_alloc(mediaFormat, nullptr, nullptr, &p);

                int ret;
                ret = media_packet_set_pts(p, packet.m_pts / 1090);
                STARFISH_ASSERT(ret == 0);
                void* data;
                ret = media_packet_get_buffer_data_ptr(p, &data);
                STARFISH_ASSERT(ret == 0);
                memcpy(data, packet.m_data, packet.m_dataSize);
                ret = media_packet_set_buffer_size(p, packet.m_dataSize);
                STARFISH_ASSERT(ret == 0);


                ret = player_push_media_stream(player, p);
                printf("push packet %d %p %d\n", (int)packet.m_pts, packet.m_data, (int)packet.m_dataSize);
                STARFISH_ASSERT(ret == 0);
                media_packet_destroy(p);
            }

            DemuxerClientTemp(player_h p)
            {
                player = p;

                av_register_all();
                avcodec_register_all();

                avformat_open_input(&fc, "car.mp4", nullptr, nullptr);

                avformat_find_stream_info(fc, nullptr);

                avcodec_open2(fc->streams[0]->codec, avcodec_find_decoder(fc->streams[0]->codec->codec_id), nullptr);



                frame = av_frame_alloc();
            }
            player_h player;
            AVFormatContext* fc;
            AVFrame *frame;
        };


        demuxer->addClient(new DemuxerClientTemp(player));
        // demuxer->addClient(new DemuxerClientTemp());

        auto ptr = new DemuxerMemorySource();
        FILE* fp = fopen("car.mp4", "rb");

        copyFileContent(fp, 0, 708, ptr->data);
        if (!demuxer->findStreamInfo(ptr, String::fromUTF8("video/mp4"))) {
            puts("fail0");
            // ptr->m_debug++;
        }


        auto ptr2 = new DemuxerMemorySource();
        // 1184-727646
        copyFileContent(fp, 1184, 727646 + 1, ptr2->data);
        demuxer->findStreamPacket(ptr2);

        puts("thread end");
        return nullptr;
    }, player);

    // player_start(player);


    puts("elm_run");
    elm_run();
    return 0;
    */
    // -------------------------------------------------------------------

    // GC_disable();
    int flag = 0;

    if (argc == 1) {
        puts("please specify file path");
        return -1;
    }
    // printf("%d", (int)sizeof (StarFish::ComputedStyle));

    std::string screenShot;
    int width = 360, height = 360;
    for (int i = 2; i < argc; i ++) {
        if (strcmp(argv[i], "--dump-computed-style") == 0) {
            flag |= StarFish::enableComputedStyleDump;
        } else if (strcmp(argv[i], "--dump-frame-tree") == 0) {
            flag |= StarFish::enableFrameTreeDump;
        } else if (strcmp(argv[i], "--dump-stacking-context") == 0) {
            flag |= StarFish::enableStackingContextDump;
        } else if (strcmp(argv[i], "--dump-hittest") == 0) {
            flag |= StarFish::enableHitTestDump;
        } else if (strcmp(argv[i], "--pixel-test") == 0) {
#ifdef STARFISH_ENABLE_TEST
            g_enablePixelTest = true;
            setenv("PIXEL_TEST", "1", 1);
#endif
        } else if (strstr(argv[i], "--width=") == argv[i]) {
            width = std::atoi(argv[i] + strlen("--width="));
        } else if (strstr(argv[i], "--height=") == argv[i]) {
            height = std::atoi(argv[i] + strlen("--height="));
        } else if (strcmp(argv[i], "--regression-test") == 0) {
            flag |= StarFish::enableRegressionTest;
        } else if (strstr(argv[i], "--screen-shot=") == argv[i]) {
            screenShot = argv[i] + strlen("--screen-shot=");
            setenv("SCREEN_SHOT_FILE", screenShot.c_str(), 1);
        } else if (strstr(argv[i], "--screen-shot-width=") == argv[i]) {
            setenv("SCREEN_SHOT_WIDTH", argv[i] + strlen("--screen-shot-width="), 1);
        } else if (strstr(argv[i], "--screen-shot-height=") == argv[i]) {
            setenv("SCREEN_SHOT_HEIGHT", argv[i] + strlen("--screen-shot-height="), 1);
        } else if (strcmp(argv[i], "--hide-window") == 0) {
            // regression test, pixel test only
            setenv("HIDE_WINDOW", "1", 1);
        } else if (strcmp(argv[i], "--mem-log-dump") == 0) {
#ifdef STARFISH_ENABLE_TEST
            g_memLogDump = true;
#endif
        }
    }

    if (screenShot.length()) {
        // screenShot = std::string("shot:delay=0.5:file=") + screenShot;
        // setenv("ELM_ENGINE", screenShot.data(), 1);
        setenv("SCREEN_SHOT", screenShot.data(), 1);
        setenv("EXIT_AFTER_SCREEN_SHOT", "1", 1);
    }

    elm_init(0, 0);
    elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

    StarFish::StarFish* sf = new StarFish::StarFish((StarFish::StarFishStartUpFlag)flag, "ko-KR", "Asia/Seoul", nullptr, width, height, 1);

    // testDemuxer();

#if defined(STARFISH_ENABLE_INSPECTOR)
    sf->setupInspector();
#endif
    sf->loadHTMLDocument(String::createASCIIString(argv[1]));

    pthread_t t;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(&t, &attr, [](void* data) -> void* {
        char buf[1024];
        sleep(1);
        while (1) {
            fgets(buf, 1024, stdin);
            struct Pass {
                StarFish::StarFish* sf;
                char* buf;
            };
            char* b = new char[1024];
            Pass* pass = new Pass;
            pass->buf = b;
            pass->sf = (StarFish::StarFish*)data;
            memcpy(b, buf, sizeof buf);
            ecore_thread_main_loop_begin();
            ecore_animator_add([](void *data) -> Eina_Bool {
                Pass* p = (Pass*)data;

                if (strncmp(p->buf, "!exit", 5) == 0) {
                    delete p->sf;

                    GC_gcollect_and_unmap();
                    GC_gcollect_and_unmap();
                    GC_gcollect_and_unmap();
                    GC_gcollect_and_unmap();
                    exit(-1);
                }

                StarFishEnterer enter(p->sf);
                String* str = p->sf->evaluate(String::fromUTF8(p->buf));
                puts(str->utf8Data());

                delete [] p->buf;
                delete p;
                return ECORE_CALLBACK_CANCEL;
            }, pass);
            ecore_thread_main_loop_end();
        }
        return NULL;
    }, sf);

    // sf->messageLoop()->addIdler(test, sf);

    sf->run();
    // delete sf;

    return 0;
}

/*
#include <Elementary.h>

int main()
{
    elm_init(0, 0);
    elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
    Evas_Object* wnd = elm_win_add(NULL, "", ELM_WIN_BASIC);

    elm_win_autodel_set(wnd, EINA_TRUE);
    Evas_Object* box = elm_box_add(wnd);
    evas_object_size_hint_weight_set (box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(wnd, box);
    evas_object_show(box);

    evas_object_show(wnd);
    elm_run();
    return 0;
}
*/
