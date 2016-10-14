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

using namespace StarFish;

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
            STARFISH_ASSERT((int)position < (int)data.size());
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
        int readed = 0;
        int end = readPos + sizeWantToRead;
        if ((int)end > (int)data.size()) {
            end = data.size();
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

void testDemuxer()
{
    /*
    Demuxer* demuxer = Demuxer::create();
    auto ptr = new DemuxerMemorySource();
    FILE* fp = fopen("feelings.webm", "rb");

    copyFileContent(fp, 0, 235, ptr->data);
    if (!demuxer->findStreamInfo(ptr, String::fromUTF8("video/webm"))) {
        puts("fail0");
        // ptr->m_debug++;
    }

    auto ptr2 = new DemuxerMemorySource();
    // 2785163-3019334
    copyFileContent(fp, 2785163, 3019334 + 1, ptr2->data);
    demuxer->findStreamPacket(ptr2);

    auto ptr3 = new DemuxerMemorySource();
    // 661985-880943
    copyFileContent(fp, 661985, 880943 + 1, ptr3->data);
    demuxer->findStreamPacket(ptr3);
    fclose(fp);*/

    Demuxer* demuxer = Demuxer::createMP4Demuxer();
    auto ptr = new DemuxerMemorySource();
    FILE* fp = fopen("car.mp4", "rb");

    copyFileContent(fp, 0, 708, ptr->data);
    if (!demuxer->findStreamInfo(ptr, String::fromUTF8("video/mp4"))) {
        puts("fail0");
        // ptr->m_debug++;
    }

    auto ptr2 = new DemuxerMemorySource();
    // 5931518-6320465
    copyFileContent(fp, 5931518, 6320465 + 1, ptr2->data);
    demuxer->findStreamPacket(ptr2);

    auto ptr3 = new DemuxerMemorySource();
    // 1450907-2164184
    copyFileContent(fp, 1450907, 2164184 + 1, ptr3->data);
    demuxer->findStreamPacket(ptr3);

    auto ptr4 = new DemuxerMemorySource();
    FILE* fp2 = fopen("toystory_frag.mp4", "rb");
    copyFileContent(fp2, 0, 33517761, ptr4->data);

    demuxer = Demuxer::createMP4Demuxer();
    demuxer->findStreamInfo(ptr4, String::fromUTF8("video/mp4"));
    demuxer->findStreamPacket(ptr4);
}

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
