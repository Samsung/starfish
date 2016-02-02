#include "test.h"

#define NDEBUG
#define STARFISH_TIZEN_WEARABLE
#include <StarFishConfig.h>
#include <StarFish.h>

#include <string>

using namespace StarFish;

StarFish::StarFish* sf;

extern "C" void sfInit(void* win)
{
	char* buf = app_get_shared_resource_path();
	std::string path = buf;
	path = path.substr(0,path.length()-4);
    sf = new StarFish::StarFish(StarFish::enableBlackTheme, String::createASCIIString(std::string(path+"/20160115/").data()), win);
    GC_add_roots(sf, sf + sizeof(StarFish::StarFish));
    dlog_print(DLOG_ERROR, "StarFish", "StarFishInit3");
    path += "./20160115/result.xml";

    sf->loadXMLDocument(String::createASCIIString(path.data()));
}

extern "C" void sfPause()
{
	sf->pause();
}

extern "C" void sfResume()
{
	sf->resume();
}

/*
#include <Elementary.h>

int main()
{
    elm_init(0,0);
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
