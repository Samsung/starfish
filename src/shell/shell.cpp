#include "StarFishConfig.h"
#include "dom/Document.h"
#include "StarFish.h"

#include <pthread.h>
#include <Elementary.h>

using namespace StarFish;

int main(int argc, char *argv[])
{
#ifndef NDEBUG
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
#endif
    // GC_disable();
    int flag = 0;

    const char* path = "";
    for (int i = 2; i < argc; i ++) {
        if (strcmp(argv[i], "--dump-computed-style") == 0) {
            flag |= StarFish::enableComputedStyleDump;
        } else if (strcmp(argv[i], "--dump-frame-tree") == 0) {
            flag |= StarFish::enableFrameTreeDump;
        } else if (strcmp(argv[i], "--dump-hittest") == 0) {
            flag |= StarFish::enableHitTestDump;
        } else if (strcmp(argv[i], "--enable-black-theme") == 0) {
            flag |= StarFish::enableBlackTheme;
        } else if (strstr(argv[i], "--working-directory=") == argv[i]) {
            path = argv[i] + strlen("--working-directory=");
        } else if (strcmp(argv[i], "--pixel-test") == 0) {
            g_enablePixelTest = true;
        } else if (strcmp(argv[i], "--regression-test") == 0) {
            flag |= StarFish::enableRegressionTest;
        }
    }

    printf("running StarFish (working directory = %s)\n", path);
    StarFish::StarFish* sf = new StarFish::StarFish((StarFish::StarFishStartUpFlag)flag, String::fromUTF8(path));
    if (argc == 1) {
        puts("please specify xml path");
        return -1;
    }
    sf->window()->loadXMLDocument(String::createASCIIString(argv[1]));

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
