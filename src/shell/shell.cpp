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

#include <pthread.h>
#include <Elementary.h>

using namespace StarFish;

bool hasEnding(std::string const &fullString, std::string const &ending)
{
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

int main(int argc, char *argv[])
{
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

    StarFish::StarFish* sf = new StarFish::StarFish((StarFish::StarFishStartUpFlag)flag, "en-us", "Asia/Seoul", nullptr, width, height);


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
            ecore_idler_add([](void *data) -> Eina_Bool {
                Pass* p = (Pass*)data;

                if (strncmp(p->buf, "!exit", 5) == 0) {
                    delete p->sf;

                    GC_gcollect_and_unmap();
                    GC_gcollect_and_unmap();
                    GC_gcollect_and_unmap();
                    GC_gcollect_and_unmap();
                    exit(-1);
                }

                ScriptBindingInstanceEnterer enter(p->sf->scriptBindingInstance());
                p->sf->evaluate(String::fromUTF8(p->buf));

                delete [] p->buf;
                delete p;
                return ECORE_CALLBACK_CANCEL;
            }, pass);
            ecore_thread_main_loop_end();
        }
        return NULL;
    }, sf);

    sf->run();
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
