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
#include "Window.h"

#include "dom/binding/ScriptBindingInstance.h"
#include "dom/HTMLDocument.h"
#include "layout/FrameTreeBuilder.h"
#include "platform/canvas/font/Font.h"
#include "platform/message_loop/MessageLoop.h"
#include "animation/Animation.h"

#include "extra/History.h"
#include "extra/Navigator.h"
#include "extra/Location.h"
#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
#include "extra/WebApis.h"
#endif

#include "layout/Frame.h"
#include "layout/FrameBox.h"
#include "layout/FrameBlockBox.h"

#include <Elementary.h>
#include <Evas_Engine_Buffer.h>
#if defined(STARFISH_TIZEN_3_0) || defined(STARFISH_TIZEN_OBS)
#include <Ecore.h>
#else
#include <Ecore_X.h>
#endif
#include <Ecore_Input.h>
#include <Ecore_Input_Evas.h>

#ifdef STARFISH_TIZEN_WEARABLE
#include <efl_extension.h>
#include <tizen.h>
#endif

#ifdef STARFISH_ENABLE_TEST
#include <sys/ioctl.h>
#include <net/if.h>
bool g_fireOnloadEvent = false;
bool g_forceRendering = false;
#endif

#ifndef STARFISH_TIZEN_WEARABLE_LIB
extern "C" Ecore_Evas* ecore_evas_ecore_evas_get(const Evas* e);
extern "C" Ecore_Window ecore_evas_window_get(const Ecore_Evas* e);
#endif

// #define STARFISH_ENABLE_TIMER

namespace StarFish {

struct IdlerData {
    void (*m_fn)(void*);
    void* m_data;
};

class WindowImplEFL : public Window {
public:
    WindowImplEFL(StarFish* sf) : Window(sf)
    {
        m_mainBox = nullptr;
        m_dummyBox = nullptr;
        m_dummyBoxClipper = nullptr;
        m_renderingAnimator = nullptr;
        m_renderingIdlerData = nullptr;
        m_animationExecutor = nullptr;

        GC_REGISTER_FINALIZER_NO_ORDER(
            this,
            [](void* obj, void* cd) {
                STARFISH_LOG_INFO("WindowImplEFL::~WindowImplEFL\n");
            },
            NULL, NULL, NULL);
    }

    virtual int width()
    {
#ifdef STARFISH_ENABLE_TEST
        if (getenv("SCREEN_SHOT_WIDTH") &&
            strlen(getenv("SCREEN_SHOT_WIDTH"))) {
            return atoi(getenv("SCREEN_SHOT_WIDTH"));
        }
#endif
        WindowImplEFL* eflWindow = (WindowImplEFL*)this;
        int width;
        evas_object_geometry_get(eflWindow->m_window, NULL, NULL, &width, NULL);
        return width;
    }

    virtual int height()
    {
#ifdef STARFISH_ENABLE_TEST
        if (getenv("SCREEN_SHOT_HEIGHT") &&
            strlen(getenv("SCREEN_SHOT_HEIGHT"))) {
            return atoi(getenv("SCREEN_SHOT_HEIGHT"));
        }
#endif
        WindowImplEFL* eflWindow = (WindowImplEFL*)this;
        int height;
        evas_object_geometry_get(eflWindow->m_window, NULL, NULL, NULL,
                                 &height);
        return height;
    }

    virtual void resizeTo(int w, int h)
    {
        evas_object_resize(m_window, w, h);
    }

    virtual void* unwrap()
    {
        return (void*)m_window;
    }

    void clearEFLResources()
    {
        clearStackingContext(false);
    }

    bool m_isActive;
    uintptr_t m_handle;
    Evas_Object* m_window;
    Evas_Object* m_canvasAdpater;
    std::vector<Evas_Object*> m_objectList;
    std::vector<Evas_Object*> m_surfaceList;
    Evas_Object* m_mainBox;
    Evas_Object* m_dummyBox;
    Evas_Object* m_dummyBoxClipper;

    Ecore_Event_Handler* m_desktopMouseDownEventHandler;
    Ecore_Event_Handler* m_desktopMouseMoveEventHandler;
    Ecore_Event_Handler* m_desktopMouseUpEventHandler;
    Ecore_Event_Handler* m_desktopKeyDownEventHandler;
    Ecore_Event_Handler* m_desktopKeyUpEventHandler;

    void (*m_mobileMouseDownEventHandler)(void* data, Evas* evas,
                                          Evas_Object* obj, void* event_info);
    void (*m_mobileMouseMoveEventHandler)(void* data, Evas* evas,
                                          Evas_Object* obj, void* event_info);
    void (*m_mobileMouseUpEventHandler)(void* data, Evas* evas,
                                        Evas_Object* obj, void* event_info);
    void (*m_mobileClickEventHandler)(void* data, Evas_Object* obj,
                                      void* event_info);

    Ecore_Animator* m_renderingAnimator;
    IdlerData* m_renderingIdlerData;

    float m_lastMouseX, m_lastMouseY;

    AnimationExecutor* m_animationExecutor;
};

class CanvasSurfaceEFL : public CanvasSurface {
public:
    CanvasSurfaceEFL(Window* wnd, size_t w, size_t h)
    {
        m_window = (WindowImplEFL*)wnd;
        m_image =
            evas_object_image_add(evas_object_evas_get(m_window->m_window));
        evas_object_image_size_set(m_image, w, h);
        evas_object_image_filled_set(m_image, EINA_TRUE);
        evas_object_image_colorspace_set(
            m_image, Evas_Colorspace::EVAS_COLORSPACE_ARGB8888);
        evas_object_image_alpha_set(m_image, EINA_TRUE);
        evas_object_anti_alias_set(m_image, EINA_TRUE);
        STARFISH_RELEASE_ASSERT(evas_object_image_colorspace_get(m_image) ==
                                EVAS_COLORSPACE_ARGB8888);
        m_width = w;
        m_height = h;
        // STARFISH_LOG_INFO("create CanvasSurfaceEFL %p %p\n", this, m_image);

        STARFISH_ASSERT(evas_object_visible_get(m_image) == EINA_FALSE);
        GC_REGISTER_FINALIZER_NO_ORDER(this,
                                       [](void* obj, void* cd) {
                                           CanvasSurfaceEFL* s =
                                               (CanvasSurfaceEFL*)obj;
                                           // STARFISH_LOG_INFO("release
                                           // CanvasSurfaceEFL %p\n", s);
                                           s->detachNativeBuffer();
                                       },
                                       NULL, NULL, NULL);
    }

    void detachNative(Evas_Object* image)
    {
        if (!image) {
            return;
        }
        CanvasSurfaceEFL* s = (CanvasSurfaceEFL*)this;
        // STARFISH_LOG_INFO("detach CanvasSurfaceEFL NativeBuffer %p\n",
        //                   image);
        // evas_object_image_size_set(image, 0, 0);
        evas_object_hide(image);
        STARFISH_RELEASE_ASSERT(evas_object_ref_get(image) == 0);
        evas_object_del(image);
        /*
        auto iter = std::find(s->m_window->m_surfaceList.begin(),
        s->m_window->m_surfaceList.end(), s->m_image);
        if (s->m_window->m_surfaceList.end() != iter) {
            s->m_window->m_surfaceList.erase(iter);
        }
        */
    }

    virtual void detachNativeBuffer()
    {
        detachNative(m_image);
        m_image = nullptr;
    }

    virtual void resize(size_t w, size_t h)
    {
        STARFISH_ASSERT(m_image);
        evas_object_image_size_set(m_image, w, h);
    }

    virtual void* unwrap()
    {
        return m_image;
    }

    virtual size_t width()
    {
        return m_width;
    }

    virtual size_t height()
    {
        return m_height;
    }

    virtual void clear()
    {
        void* address = evas_object_image_data_get(m_image, EINA_TRUE);
        size_t end = m_width * m_height * sizeof(uint32_t);
        memset(address, 0xff, end);
        evas_object_image_data_set(m_image, address);
    }

protected:
    WindowImplEFL* m_window;
    Evas_Object* m_image;
    size_t m_width;
    size_t m_height;
};

CanvasSurface* CanvasSurface::create(Window* wnd, size_t w, size_t h)
{
    return new CanvasSurfaceEFL(wnd, w, h);
}

static void mainRenderingFunction(Evas_Object* o, Evas_Object_Box_Data* priv,
                                  void* user_data)
{
    ecore_animator_add(
        [](void* user_data) -> Eina_Bool {
            WindowImplEFL* wnd = (WindowImplEFL*)user_data;
            wnd->setNeedsLayout();
            return ECORE_CALLBACK_CANCEL;
        },
        user_data);
}

Window* Window::create(StarFish* sf, void* win, int width, int height)
{
    auto wnd = new WindowImplEFL(sf);
    wnd->m_starFish = sf;
    wnd->m_window = (Evas_Object*)win;

#ifndef STARFISH_TIZEN_WEARABLE_LIB
    Evas* e = evas_object_evas_get(wnd->m_window);
    Ecore_Evas* ee = ecore_evas_ecore_evas_get(e);
    Ecore_Window ew = ecore_evas_window_get(ee);
    wnd->m_handle = (uintptr_t)ew;

    wnd->m_mainBox = elm_box_add(wnd->m_window);
    evas_object_size_hint_weight_set(wnd->m_mainBox, EVAS_HINT_EXPAND,
                                     EVAS_HINT_EXPAND);
    elm_win_resize_object_add(wnd->m_window, wnd->m_mainBox);
    elm_box_layout_set(wnd->m_mainBox, mainRenderingFunction, wnd, NULL);
    evas_object_show(wnd->m_mainBox);
#ifdef STARFISH_ENABLE_TEST
    {
        const char* path = getenv("SCREEN_SHOT");
        const char* hide = getenv("HIDE_WINDOW");
        if ((path && strlen(path)) || (hide && strlen(hide))) {
            evas_object_hide(wnd->m_window);
        } else {
            evas_object_show(wnd->m_window);
        }
    }
#else
    evas_object_show(wnd->m_window);
#endif
    /*
    evas_event_callback_add(e, EVAS_CALLBACK_RENDER_FLUSH_POST,
        [](void *data,
            Evas *e, void *event_info) {
        }, wnd);
    */

    wnd->m_desktopMouseDownEventHandler = ecore_event_handler_add(
        ECORE_EVENT_MOUSE_BUTTON_DOWN,
        [](void* data, int type, void* event) -> Eina_Bool {
            Window* sf = (Window*)data;
            Ecore_Event_Mouse_Button* d = (Ecore_Event_Mouse_Button*)event;
            StarFishEnterer enter(sf->m_starFish);
            sf->dispatchMouseEvent(d->x, d->y, Window::MouseEventDown);
            return EINA_TRUE;
        },
        wnd);

    wnd->m_desktopMouseUpEventHandler = ecore_event_handler_add(
        ECORE_EVENT_MOUSE_BUTTON_UP,
        [](void* data, int type, void* event) -> Eina_Bool {
            Window* sf = (Window*)data;
            Ecore_Event_Mouse_Button* d = (Ecore_Event_Mouse_Button*)event;
            StarFishEnterer enter(sf->m_starFish);
            sf->dispatchMouseEvent(d->x, d->y, Window::MouseEventUp);
            return EINA_TRUE;
        },
        wnd);

    wnd->m_desktopMouseMoveEventHandler = ecore_event_handler_add(
        ECORE_EVENT_MOUSE_MOVE,
        [](void* data, int type, void* event) -> Eina_Bool {
            Window* sf = (Window*)data;
            Ecore_Event_Mouse_Move* d = (Ecore_Event_Mouse_Move*)event;
            StarFishEnterer enter(sf->m_starFish);
            sf->dispatchMouseEvent(d->x, d->y, Window::MouseEventMove);
            return EINA_TRUE;
        },
        wnd);

    wnd->m_desktopKeyDownEventHandler = ecore_event_handler_add(
        ECORE_EVENT_KEY_DOWN,
        [](void* data, int type, void* event) -> Eina_Bool {
            Window* sf = (Window*)data;
            Ecore_Event_Key* d = (Ecore_Event_Key*)event;
            StarFishEnterer enter(sf->m_starFish);
            sf->dispatchKeyEvent(String::createASCIIString(d->keyname),
                                 Window::KeyEventDown);
            return EINA_TRUE;
        },
        wnd);

    wnd->m_desktopKeyUpEventHandler = ecore_event_handler_add(
        ECORE_EVENT_KEY_UP,
        [](void* data, int type, void* event) -> Eina_Bool {
            Window* sf = (Window*)data;
            Ecore_Event_Key* d = (Ecore_Event_Key*)event;
            StarFishEnterer enter(sf->m_starFish);
            sf->dispatchKeyEvent(String::createASCIIString(d->keyname),
                                 Window::KeyEventUp);
            return EINA_TRUE;
        },
        wnd);

#else
    Evas* e = evas_object_evas_get(wnd->m_window);
    wnd->m_mainBox = elm_box_add(wnd->m_window);
    evas_object_size_hint_weight_set(wnd->m_mainBox, EVAS_HINT_EXPAND,
                                     EVAS_HINT_EXPAND);
    elm_win_resize_object_add(wnd->m_window, wnd->m_mainBox);
    elm_box_layout_set(wnd->m_mainBox, mainRenderingFunction, wnd, NULL);
    evas_object_show(wnd->m_mainBox);

    wnd->m_dummyBox = elm_button_add(wnd->m_window);
    int w, h;
    evas_object_geometry_get(wnd->m_window, &w, &h, NULL, NULL);
    evas_object_resize(wnd->m_dummyBox, width, height);
    evas_object_move(wnd->m_dummyBox, 0, 0);
    evas_object_show(wnd->m_dummyBox);

    wnd->m_dummyBoxClipper = evas_object_rectangle_add(e);
    evas_object_move(wnd->m_dummyBoxClipper, 0, 0);
    evas_object_resize(wnd->m_dummyBoxClipper, width, height);
    evas_object_clip_set(wnd->m_dummyBox, wnd->m_dummyBoxClipper);
    evas_object_color_set(wnd->m_dummyBoxClipper, 0, 0, 0, 0);
    evas_object_show(wnd->m_dummyBoxClipper);

    evas_object_show(wnd->m_window);

    wnd->m_mobileMouseDownEventHandler =
        [](void* data, Evas* evas, Evas_Object* obj, void* event_info) -> void {
        WindowImplEFL* sf = (WindowImplEFL*)data;
        Evas_Event_Mouse_Down* ev = (Evas_Event_Mouse_Down*)event_info;
        sf->m_lastMouseX = ev->canvas.x;
        sf->m_lastMouseY = ev->canvas.y;
        StarFishEnterer enter(sf->m_starFish);
        sf->dispatchTouchEvent(ev->canvas.x, ev->canvas.y,
                               Window::TouchEventDown);
        return;
    };
    evas_object_event_callback_add(wnd->m_dummyBox, EVAS_CALLBACK_MOUSE_DOWN,
                                   wnd->m_mobileMouseDownEventHandler, wnd);

    wnd->m_mobileMouseMoveEventHandler =
        [](void* data, Evas* evas, Evas_Object* obj, void* event_info) -> void {
        WindowImplEFL* sf = (WindowImplEFL*)data;
        Evas_Event_Mouse_Move* ev = (Evas_Event_Mouse_Move*)event_info;
        sf->m_lastMouseX = ev->cur.canvas.x;
        sf->m_lastMouseY = ev->cur.canvas.y;
        StarFishEnterer enter(sf->m_starFish);
        sf->dispatchTouchEvent(ev->cur.canvas.x, ev->cur.canvas.y,
                               Window::TouchEventMove);
        return;
    };
    evas_object_event_callback_add(wnd->m_dummyBox, EVAS_CALLBACK_MOUSE_MOVE,
                                   wnd->m_mobileMouseMoveEventHandler, wnd);

    wnd->m_mobileMouseUpEventHandler =
        [](void* data, Evas* evas, Evas_Object* obj, void* event_info) -> void {
        WindowImplEFL* sf = (WindowImplEFL*)data;
        sf->starFish()->messageLoop()->addIdler(
            [](size_t a, void* data) {
                ((Window*)data)
                    ->dispatchTouchEvent(0, 0, Window::TouchEventCancel);
            },
            sf);
        return;
    };
    evas_object_event_callback_add(wnd->m_dummyBox, EVAS_CALLBACK_MOUSE_UP,
                                   wnd->m_mobileMouseUpEventHandler, wnd);

    wnd->m_mobileClickEventHandler = [](void* data, Evas_Object* obj,
                                        void* event_info) -> void {
        WindowImplEFL* sf = (WindowImplEFL*)data;
        StarFishEnterer enter(sf->m_starFish);
        sf->dispatchTouchEvent(sf->m_lastMouseX, sf->m_lastMouseY,
                               Window::TouchEventUp);
    };
    evas_object_smart_callback_add(wnd->m_dummyBox, "clicked",
                                   wnd->m_mobileClickEventHandler, wnd);
#endif
    wnd->m_animationExecutor = new AnimationExecutor(wnd);
    return wnd;
}

Window::Window(StarFish* starFish)
    : m_starFish(starFish)
    , m_scriptBindingInstance(nullptr)
    , m_history(nullptr)
    , m_navigator(nullptr)
    , m_location(nullptr)
    , m_document(nullptr)
#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
    , m_webapis(nullptr)
#endif
    , m_rootStackingContext(nullptr)
    , m_touchDownPoint(0, 0)
    , m_ctrlKeyDown(0)
    , m_shiftKeyDown(0)
    , m_altKeyDown(0)
    , m_metaKeyDown(0)
{
    initFlags();
}

void Window::initFlags()
{
    m_timeoutCounter = 0;
    m_requestAnimationFrameCounter = 1;

    m_needsRendering = false;
    m_inRendering = false;
    m_needsStyleRecalc = false;
    m_needsStyleRecalcForWholeDocument = false;
    m_needsFrameTreeBuild = false;
    m_needsLayout = false;
    m_needsPainting = false;
    m_needsComposite = false;

    m_hasRootElementBackground = false;
    m_hasBodyElementBackground = false;
    m_isRunning = true;
    m_pendingStyleSheetCount = 0;
    m_lastRenderingTime = 0;

    m_activeNodeWithTouchDown = nullptr;
}

Window::~Window()
{
    STARFISH_LOG_INFO("Window::~Window\n");

    WindowImplEFL* eflWindow = (WindowImplEFL*)this;

    if (eflWindow->m_dummyBoxClipper) {
        evas_object_del(eflWindow->m_dummyBoxClipper);
        eflWindow->m_dummyBoxClipper = nullptr;
    }

    if (eflWindow->m_dummyBox) {
        evas_object_del(eflWindow->m_dummyBox);
        eflWindow->m_dummyBox = nullptr;
    }

    if (eflWindow->m_mainBox) {
        elm_win_resize_object_del(eflWindow->m_window, eflWindow->m_mainBox);
        evas_object_del(eflWindow->m_mainBox);
        eflWindow->m_mainBox = nullptr;
    }

#ifndef STARFISH_TIZEN_WEARABLE
    ecore_event_handler_del(eflWindow->m_desktopMouseDownEventHandler);
    ecore_event_handler_del(eflWindow->m_desktopMouseUpEventHandler);
    ecore_event_handler_del(eflWindow->m_desktopMouseMoveEventHandler);
    ecore_event_handler_del(eflWindow->m_desktopKeyDownEventHandler);
    ecore_event_handler_del(eflWindow->m_desktopKeyUpEventHandler);
#endif

#ifdef STARFISH_TIZEN_WEARABLE
    evas_object_event_callback_del(eflWindow->m_dummyBox,
                                   EVAS_CALLBACK_MOUSE_DOWN,
                                   eflWindow->m_mobileMouseDownEventHandler);
    evas_object_event_callback_del(eflWindow->m_dummyBox,
                                   EVAS_CALLBACK_MOUSE_MOVE,
                                   eflWindow->m_mobileMouseMoveEventHandler);
    evas_object_event_callback_del(eflWindow->m_dummyBox,
                                   EVAS_CALLBACK_MOUSE_UP,
                                   eflWindow->m_mobileMouseUpEventHandler);
    evas_object_smart_callback_del(eflWindow->m_dummyBox, "clicked",
                                   eflWindow->m_mobileClickEventHandler);

    if (eflWindow->m_animationExecutor->isAlive()) {
        eflWindow->m_animationExecutor->stopIfNeeds();
    }
    eflWindow->m_animationExecutor = nullptr;
#endif
}

void Window::navigate(URL* url)
{
    close();
    initFlags();
    STARFISH_LOG_INFO("Window::navigate %s\n", url->urlString()->utf8Data());

    WindowImplEFL* eflWindow = (WindowImplEFL*)this;
    eflWindow->m_isActive = true;

    m_scriptBindingInstance = new ScriptBindingInstance();
    StarFishEnterer enter(m_starFish);
    m_scriptBindingInstance->initBinding(m_starFish);
    initScriptWrappable(this);

    m_history = new History(m_starFish);
    m_navigator = new Navigator(m_starFish);
    m_location = new LocationObj(m_starFish);
#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
    m_webapis = new webapis(m_starFish);
#endif
    m_document = new HTMLDocument(this, scriptBindingInstance(), url,
                                  String::createASCIIString("UTF-8"), true);
    m_document->open();
}

void Window::setHistory(URL* url)
{
    if (!m_history) {
        m_history = new History(m_starFish);
    }
    m_history->setHistory(String::emptyString, String::emptyString, url);
}

void Window::navigateAsync(URL* url)
{
    starFish()->messageLoop()->addIdlerWithNoScriptInstanceEntering(
        [](size_t a, void* data, void* data2) {
            ((Window*)data2)->setHistory((URL*)data);
            ((Window*)data2)->navigate((URL*)data);
        },
        url, this);
}

void Window::navigateAsyncWithoutSetHistory(URL* url)
{
    starFish()->messageLoop()->addIdlerWithNoScriptInstanceEntering(
        [](size_t a, void* data, void* data2) {
            ((Window*)data2)->navigate((URL*)data);
        },
        url, this);
}

#ifdef STARFISH_ENABLE_TEST
static Evas_Object* g_imgBufferForScreehShot;
static CanvasSurface* g_surfaceForScreehShot;
#endif

Canvas* preparePainting(WindowImplEFL* eflWindow, bool forPainting)
{
#ifdef STARFISH_ENABLE_TEST
    {
        const char* path = getenv("SCREEN_SHOT");
        if (path && strlen(path) && g_fireOnloadEvent) {
            g_surfaceForScreehShot = CanvasSurface::create(
                eflWindow, eflWindow->width(), eflWindow->height());
            g_imgBufferForScreehShot =
                (Evas_Object*)g_surfaceForScreehShot->unwrap();
            return Canvas::create(g_surfaceForScreehShot);
        }
    }
#endif
    int width, height;
    evas_object_geometry_get(eflWindow->m_window, NULL, NULL, &width, &height);
    Evas* evas = evas_object_evas_get(eflWindow->m_window);
    struct dummy {
        void* a;
        void* b;
        int w;
        int h;
        std::vector<Evas_Object*>* objList;
        std::vector<Evas_Object*>* surfaceList;
    };
    dummy* d = new dummy;
    d->a = evas;
    d->b = nullptr;
    if (!forPainting) {
        d->b = nullptr;
    }
    d->w = width;
    d->h = height;
    d->objList = &eflWindow->m_objectList;
    d->surfaceList = &eflWindow->m_surfaceList;
    auto iter = eflWindow->m_objectList.begin();
    while (iter != eflWindow->m_objectList.end()) {
        evas_object_del(*iter);
        iter++;
    }
    eflWindow->m_objectList.clear();
    eflWindow->m_objectList.shrink_to_fit();

    iter = eflWindow->m_surfaceList.begin();
    while (iter != eflWindow->m_surfaceList.end()) {
        evas_object_hide(*iter);
        iter++;
    }
    eflWindow->m_surfaceList.clear();
    eflWindow->m_surfaceList.shrink_to_fit();

    Canvas* canvas = Canvas::createDirect(d);
    delete d;

    return canvas;
}

void Window::paintWindowBackground(Canvas* canvas)
{
#ifdef STARFISH_TIZEN
    if (!document()->m_tizenWidgetTransparentBackground) {
        canvas->clearColor(Color(255, 255, 255, 255));
    }
#else
    canvas->clearColor(Color(255, 255, 255, 255));
#endif

    if (m_hasRootElementBackground || m_hasBodyElementBackground) {
        WindowImplEFL* eflWindow = (WindowImplEFL*)this;
        int width, height;
        evas_object_geometry_get(eflWindow->m_window, NULL, NULL, &width,
                                 &height);
        LayoutRect colorRect(0, 0, width, height);
        if (m_hasRootElementBackground) {
            FrameBox* rootRect =
                document()->rootElement()->frame()->asFrameBox();
            LayoutLocation rootRectPos =
                rootRect->absolutePoint(document()->frame()->asFrameBox());
            LayoutRect imgRect(rootRectPos.x() + rootRect->borderLeft(),
                               rootRectPos.y() + rootRect->borderTop(),
                               rootRect->width() - rootRect->borderWidth(),
                               rootRect->height() - rootRect->borderHeight());

            FrameBox::paintBackground(canvas,
                                      document()->rootElement()->style(),
                                      imgRect, colorRect, true);
        } else {
            LayoutRect imgRect(0, 0, width, height);
            if (document()->rootElement()->body()->frame()) {
                FrameBox* bodyRect =
                    document()->rootElement()->body()->frame()->asFrameBox();
                imgRect.setHeight(bodyRect->height() +
                                  bodyRect->marginHeight());
            }

            FrameBox::paintBackground(
                canvas, document()->rootElement()->body()->style(), imgRect,
                colorRect, true);
        }
    }
}

void Window::layoutIfNeeds()
{
    if (m_needsStyleRecalc || m_needsStyleRecalcForWholeDocument) {
        if (m_needsStyleRecalcForWholeDocument) {
#ifdef STARFISH_ENABLE_TIMER
            Timer t("parse sheet");
#endif
            document()->styleResolver()->sheets()[0]->parseSheetIfneeds();
            document()->styleResolver()->sheets()[0]->sortRulesBySpecificity();

            document()->styleResolver()->removeAllRules();
            for (size_t i = 1; i < document()->styleResolver()->sheets().size();
                 i++) {
                document()->styleResolver()->sheets()[i]->parseSheetIfneeds();
                for (size_t j = 0;
                     j <
                     document()->styleResolver()->sheets()[i]->rules().size();
                     j++) {
                    document()->styleResolver()->allRules()->addRule(
                        document()->styleResolver()->sheets()[i]->rules()[j]);
                }
            }
            document()->styleResolver()->allRules()->sortRulesBySpecificity();
        }

// resolve style
#ifdef STARFISH_ENABLE_TIMER
        Timer t("resolve style");
#endif
        document()->styleResolver()->resolveDOMStyle(
            m_document, m_needsStyleRecalcForWholeDocument);
        m_needsStyleRecalc = false;
        m_needsStyleRecalcForWholeDocument = false;

#ifdef STARFISH_ENABLE_TEST
        if (m_starFish->startUpFlag() &
            StarFishStartUpFlag::enableComputedStyleDump) {
            // dump style
            document()->styleResolver()->dumpDOMStyle(m_document);
        }
#endif
    }

    if (m_needsFrameTreeBuild) {
        if (m_document->frame()) {
            clearStackingContext(true);

// create frame tree
#ifdef STARFISH_ENABLE_TIMER
            Timer t("create frame tree");
#endif
            FrameTreeBuilder::buildFrameTree(m_document);
            m_needsFrameTreeBuild = false;
        }
    }

    if (m_needsLayout) {
// lay out frame tree
#ifdef STARFISH_ENABLE_TIMER
        Timer t("lay out frame tree");
#endif
        clearStackingContext(true);

        LayoutContext ctx(starFish(), m_document->frame()
                                          ->asFrameBox()
                                          ->asFrameBlockBox()
                                          ->asFrameDocument());
        m_document->frame()->layout(ctx,
                                    Frame::LayoutWantToResolve::ResolveAll);

#ifndef NDEBUG
        {
            LayoutContext ctx(starFish(), m_document->frame()
                                              ->asFrameBox()
                                              ->asFrameBlockBox()
                                              ->asFrameDocument());
            m_document->frame()->layout(ctx,
                                        Frame::LayoutWantToResolve::ResolveAll);
        }
#endif
        {
#ifdef STARFISH_ENABLE_TIMER
            Timer t("computeStackingContextProperties");
#endif
            m_document->frame()->asFrameBox()->iterateChildBoxes(
                [](FrameBox* box) -> bool {
                    box->establishesStackingContextIfNeeds();
                    return true;
                },
                nullptr, nullptr);
            if (m_document->frame()->firstChild()) {
                m_rootStackingContext = m_document->frame()
                                            ->firstChild()
                                            ->asFrameBox()
                                            ->stackingContext();
                m_rootStackingContext->computeStackingContextProperties();
            }

            // STARFISH_LOG_INFO("computeStackingContextProperties end composite
            // %d\n", (int)m_rootStackingContext->needsOwnBuffer());
        }
        m_needsLayout = false;
#ifdef STARFISH_ENABLE_TEST
        if (m_starFish->startUpFlag() &
            StarFishStartUpFlag::enableFrameTreeDump) {
            FrameTreeBuilder::dumpFrameTree(m_document);
        }
#endif
    }
}

void Window::markHasPendingStyleSheet()
{
    STARFISH_LOG_INFO("Window::markHasPendingStyleSheet\n");
    m_pendingStyleSheetCount++;
}

void Window::unmarkHasPendingStyleSheet()
{
    STARFISH_LOG_INFO("Window::unmarkHasPendingStyleSheet\n");
    if (m_pendingStyleSheetCount > 0) {
        m_pendingStyleSheetCount--;
        setNeedsRendering();
    }
}

void Window::rendering()
{
    WindowImplEFL* eflWindow = (WindowImplEFL*)this;
    if (m_pendingStyleSheetCount && document() &&
        document()->resourceLoader()->isDocumentInOpenState() &&
        ((timestamp() - document()->resourceLoader()->documentOpenTime()) <
         1000)) {
        m_needsRendering = false;
        setTimeout([](Window* wnd, void* data) { wnd->setNeedsRendering(); },
                   100, nullptr);

        Canvas* canvas = preparePainting(eflWindow, true);
#ifndef STARFISH_TIZEN
        canvas->clearColor(Color(255, 255, 255, 255));
#endif
        return;
    }

    if (!m_needsRendering) {
        return;
    }

    uint64_t currentTick = tickCount();
    m_lastRenderingTime = currentTick;
    m_inRendering = true;
    STARFISH_RELEASE_ASSERT(eflWindow->m_isActive);
#ifdef STARFISH_ENABLE_TIMER
    Timer renderingTimer("Window::rendering");
#endif
    layoutIfNeeds();

    {
        size_t bufSiz = m_backStackingContextBufferUpWhileReCompsite.size();
        for (size_t i = 0; i < bufSiz; i++) {
            m_backStackingContextBufferUpWhileReCompsite[i]
                ->detachNativeBuffer();
        }
        m_backStackingContextBufferUpWhileReCompsite.clear();
    }

    if (m_needsPainting) {
#ifdef STARFISH_ENABLE_TIMER
        Timer t("painting");
#endif
        // painting
        Canvas* canvas = preparePainting(eflWindow, true);

        if (m_document->frame()->firstChild()) {
            m_needsComposite = m_rootStackingContext->needsOwnBuffer();
        } else {
            m_needsComposite = false;
        }

        if (!m_needsComposite) {
            paintWindowBackground(canvas);
        }

        {
            PaintingContext ctx(canvas);
            ctx.m_paintingStage = PaintingStageEnd;
            m_document->frame()->paint(ctx);
        }
        m_needsPainting = false;

        delete canvas;
#ifdef STARFISH_TIZEN_WEARABLE
        evas_object_raise(eflWindow->m_dummyBox);
#endif

#ifdef STARFISH_ENABLE_TEST
        if (m_starFish->startUpFlag() &
            StarFishStartUpFlag::enableStackingContextDump) {
            if (m_document->frame()->firstChild()) {
                STARFISH_ASSERT(m_document->frame()
                                    ->firstChild()
                                    ->asFrameBox()
                                    ->isRootElement());
                StackingContext* ctx = m_document->frame()
                                           ->firstChild()
                                           ->asFrameBox()
                                           ->stackingContext();

                std::function<void(StackingContext*, int)> dumpSC =
                    [&dumpSC](StackingContext* ctx, int depth) {
                        for (int i = 0; i < depth; i++) {
                            printf("  ");
                        }

                        auto fr = ctx->visibleRect();

                        std::string className;
                        for (unsigned i = 0; i < ctx->owner()
                                                     ->node()
                                                     ->asElement()
                                                     ->asHTMLElement()
                                                     ->classNames()
                                                     .size();
                             i++) {
                            className += ctx->owner()
                                             ->node()
                                             ->asElement()
                                             ->asHTMLElement()
                                             ->classNames()[i]
                                             ->utf8Data();
                            className += " ";
                        }

                        printf(
                            "StackingContext[%p, node %p %s id:%s className:%s "
                            ", frame %p, buf %p %d %d %d %d]\n",
                            ctx, ctx->owner()->node(),
                            ctx->owner()->node()->localName()->utf8Data(),
                            ctx->owner()
                                ->node()
                                ->asElement()
                                ->asHTMLElement()
                                ->id()
                                ->utf8Data(),
                            className.data(), ctx->owner(), ctx->buffer(),
                            (int)fr.x(), (int)fr.y(), (int)fr.width(),
                            (int)fr.height());

                        auto iter = ctx->childContexts().begin();
                        while (iter != ctx->childContexts().end()) {
                            int32_t num = iter->first;

                            for (int i = 0; i < depth + 1; i++) {
                                printf("  ");
                            }

                            printf("z-index: %d\n", (int)num);

                            auto iter2 = iter->second->begin();
                            while (iter2 != iter->second->end()) {
                                dumpSC(*iter2, depth + 2);
                                iter2++;
                            }

                            iter++;
                        }
                    };

                dumpSC(ctx, 0);
            }
        }
#endif
    }

    if (m_needsComposite) {
#ifdef STARFISH_ENABLE_TIMER
        Timer t("composite");
#endif
        if (m_document->frame()->firstChild() &&
            m_rootStackingContext->needsOwnBuffer()) {
            Canvas* canvas = preparePainting(eflWindow, false);
            paintWindowBackground(canvas);
            m_document->frame()
                ->firstChild()
                ->asFrameBox()
                ->stackingContext()
                ->compositeStackingContext(canvas);

            delete canvas;
#ifdef STARFISH_TIZEN_WEARABLE
            evas_object_raise(eflWindow->m_dummyBox);
#endif
        }
        m_needsComposite = false;
    }

    m_needsRendering = false;
    m_inRendering = false;

#ifdef STARFISH_ENABLE_TEST
    {
        const char* path = getenv("SCREEN_SHOT");
        if (path && strlen(path) && g_fireOnloadEvent) {
            evas_object_image_save(g_imgBufferForScreehShot, path, NULL, NULL);
            // int writeImage(char* filename, int width, int height, void
            // *buffer)
            // writeImage(path, width(), height(),
            // evas_object_image_data_get(g_imgBufferForScreehShot,
            // EINA_FALSE));
            if (getenv("EXIT_AFTER_SCREEN_SHOT") &&
                strlen(getenv("EXIT_AFTER_SCREEN_SHOT"))) {
                exit(0);
            }

            g_surfaceForScreehShot->detachNativeBuffer();
            g_surfaceForScreehShot = nullptr;
        }
    }
#endif
}

void Window::clearStackingContext(bool backupBuffer)
{
    if (m_rootStackingContext) {
        StackingContext* ctx = m_rootStackingContext;
        std::function<void(StackingContext*)> clearSC =
            [&](StackingContext* ctx) {
                if (backupBuffer) {
                    if (ctx->needsOwnBuffer() && ctx->buffer()) {
                        m_backStackingContextBufferUpWhileReCompsite.push_back(
                            ctx->buffer());
                    }
                    ctx->owner()->clearStackingContextIfNeeds(false);
                } else {
                    ctx->owner()->clearStackingContextIfNeeds();
                }
                auto iter = ctx->childContexts().begin();
                while (iter != ctx->childContexts().end()) {
                    auto iter2 = iter->second->begin();
                    while (iter2 != iter->second->end()) {
                        clearSC(*iter2);
                        iter2++;
                    }
                    iter++;
                }
            };
        clearSC(ctx);
        m_rootStackingContext = nullptr;
    }
}

#ifdef STARFISH_ENABLE_TEST
void Window::setNetworkState(bool state)
{
    int sockfd;
    struct ifreq ifr;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0) {
        return;
    }

    memset(&ifr, 0, sizeof ifr);
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ);

    if (state) {
        ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
    } else {
        ifr.ifr_flags |= ~IFF_RUNNING;
        // ifr.ifr_flags |= ~IFF_UP;
    }

    ioctl(sockfd, SIOCSIFFLAGS, &ifr);
}

void Window::screenShot(std::string filePath)
{
    bool oldNeedsPainting = m_needsPainting;
    bool oldOnLoad = g_fireOnloadEvent;
    g_fireOnloadEvent = true;
    g_forceRendering = true;
    setNeedsPainting();
    setenv("SCREEN_SHOT", filePath.data(), 1);
    rendering();
    setenv("SCREEN_SHOT", "", 1);
    g_fireOnloadEvent = oldOnLoad;
    g_forceRendering = false;

    m_needsPainting = oldNeedsPainting;
    setNeedsRendering();
}

void Window::forceDisableOnloadCapture()
{
    setenv("SCREEN_SHOT", "", 1);
}

void Window::simulateClick(float x, float y)
{
    dispatchTouchEvent(x, y, Window::TouchEventDown);
    dispatchTouchEvent(x, y, Window::TouchEventUp);
}

void Window::simulateVisibilitychange(bool show)
{
    if (show) {
        m_starFish->resume();
    } else {
        m_starFish->pause();
    }
}
#endif

void Window::setNeedsRenderingSlowCase()
{
    STARFISH_ASSERT(!m_needsRendering);
    m_needsRendering = true;

    IdlerData* id = new (NoGC) IdlerData;
    id->m_fn = [](void* data) -> void {
        Window* wnd = (Window*)data;
        wnd->rendering();
    };
    id->m_data = this;

    ((WindowImplEFL*)this)->m_renderingIdlerData = id;
    ((WindowImplEFL*)this)->m_renderingAnimator = ecore_animator_add(
        [](void* data) -> Eina_Bool {
            IdlerData* id = (IdlerData*)data;
            Window* wnd = (Window*)id->m_data;
            StarFishEnterer enter(wnd->m_starFish);
            id->m_fn(id->m_data);
            ((WindowImplEFL*)wnd)->m_renderingAnimator = nullptr;
            ((WindowImplEFL*)wnd)->m_renderingIdlerData = nullptr;
            GC_FREE(id);
            return ECORE_CALLBACK_CANCEL;
        },
        id);

    ((WindowImplEFL*)this)->m_animationExecutor->startIfNeeds();
}

void Window::setWholeDocumentNeedsStyleRecalc()
{
    m_needsStyleRecalcForWholeDocument = true;
    setNeedsRendering();
}

struct TimeoutData {
    Window* m_window;
    uint32_t m_id;
    Ecore_Timer* m_timerID;
    void* m_data;
    WindowSetTimeoutHandler m_handler;
};

uint32_t Window::setTimeout(WindowSetTimeoutHandler handler, uint32_t delay,
                            void* data)
{
    WindowImplEFL* eflWindow = (WindowImplEFL*)this;
    STARFISH_RELEASE_ASSERT(eflWindow->m_isActive);

    TimeoutData* td = new (NoGC) TimeoutData;
    td->m_window = this;
    uint32_t id = ++m_timeoutCounter;
    td->m_id = id;
    td->m_data = data;
    td->m_handler = handler;
    td->m_timerID =
        ecore_timer_add(delay / 1000.0,
                        [](void* data) -> Eina_Bool {
                            TimeoutData* td = (TimeoutData*)data;
                            StarFishEnterer enter(td->m_window->m_starFish);
                            Window* wnd = td->m_window;
                            uint32_t id = td->m_id;
                            td->m_handler(td->m_window, td->m_data);
                            auto iter = wnd->m_timeoutHandler.find(id);
                            if (iter != wnd->m_timeoutHandler.end()) {
                                wnd->m_timeoutHandler.erase(iter);
                                GC_FREE(td);
                            }
                            return ECORE_CALLBACK_DONE;
                        },
                        td);

    m_timeoutHandler.insert(std::make_pair(id, td));

    return id;
}

void Window::clearTimeout(uint32_t id)
{
    WindowImplEFL* eflWindow = (WindowImplEFL*)this;
    STARFISH_RELEASE_ASSERT(eflWindow->m_isActive);

    auto handlerData = m_timeoutHandler.find(id);
    if (handlerData != m_timeoutHandler.end()) {
        TimeoutData* td = (TimeoutData*)handlerData->second;
        ecore_timer_del(td->m_timerID);
        GC_FREE(td);
        m_timeoutHandler.erase(handlerData);
    }
}

uint32_t Window::setInterval(WindowSetTimeoutHandler handler, uint32_t delay,
                             void* data)
{
    WindowImplEFL* eflWindow = (WindowImplEFL*)this;
    STARFISH_RELEASE_ASSERT(eflWindow->m_isActive);

    TimeoutData* td = new (NoGC) TimeoutData;
    td->m_window = this;
    uint32_t id = ++m_timeoutCounter;
    td->m_id = id;
    td->m_data = data;
    td->m_handler = handler;
    td->m_timerID =
        ecore_timer_add(delay / 1000.0,
                        [](void* data) -> Eina_Bool {
                            TimeoutData* td = (TimeoutData*)data;
                            StarFishEnterer enter(td->m_window->m_starFish);
                            auto a =
                                td->m_window->m_timeoutHandler.find(td->m_id);
                            td->m_handler(td->m_window, td->m_data);
                            return ECORE_CALLBACK_RENEW;
                        },
                        td);

    m_timeoutHandler.insert(std::make_pair(id, td));
    return id;
}

void Window::clearInterval(uint32_t id)
{
    WindowImplEFL* eflWindow = (WindowImplEFL*)this;
    STARFISH_RELEASE_ASSERT(eflWindow->m_isActive);

    auto handlerData = m_timeoutHandler.find(id);
    if (handlerData != m_timeoutHandler.end()) {
        TimeoutData* td = (TimeoutData*)handlerData->second;
        ecore_timer_del(td->m_timerID);
        GC_FREE(td);
        m_timeoutHandler.erase(handlerData);
    }
}

uint32_t Window::requestAnimationFrame(WindowSetTimeoutHandler handler,
                                       void* data)
{
    WindowImplEFL* eflWindow = (WindowImplEFL*)this;
    STARFISH_RELEASE_ASSERT(eflWindow->m_isActive);

    TimeoutData* td = new (NoGC) TimeoutData;
    td->m_window = this;
    uint32_t id = ++m_requestAnimationFrameCounter;
    td->m_id = id;
    td->m_data = data;
    td->m_handler = handler;
    td->m_timerID = (Ecore_Timer*)ecore_animator_add(
        [](void* data) -> Eina_Bool {
            TimeoutData* td = (TimeoutData*)data;
            StarFishEnterer enter(td->m_window->m_starFish);
            auto a =
                td->m_window->m_requestAnimationFrameHandler.find(td->m_id);
            td->m_handler(td->m_window, td->m_data);
            a = td->m_window->m_requestAnimationFrameHandler.find(td->m_id);
            if (td->m_window->m_requestAnimationFrameHandler.end() != a) {
                td->m_window->m_requestAnimationFrameHandler.erase(a);
            }
            GC_FREE(td);
            return ECORE_CALLBACK_DONE;
        },
        td);

    m_requestAnimationFrameHandler.insert(std::make_pair(id, td));

    return id;
}

void Window::cancelAnimationFrame(uint32_t reqID)
{
    auto handlerData = m_requestAnimationFrameHandler.find(reqID);

    if (handlerData != m_requestAnimationFrameHandler.end()) {
        TimeoutData* td = (TimeoutData*)handlerData->second;
        ecore_animator_del((Ecore_Animator*)td->m_timerID);
        GC_FREE(td);
        m_requestAnimationFrameHandler.erase(handlerData);
    }
}

Node* Window::hitTest(float x, float y)
{
    layoutIfNeeds();

    if (document() && document()->frame()) {
        Frame* frame = document()->frame()->hitTest(x, y, HitTestStageEnd);
        if (!frame) {
            return nullptr;
        }

        while (frame->isAnonymous()) {
            frame = frame->parent();
        }
#ifdef STARFISH_ENABLE_TEST
        if (m_starFish->startUpFlag() &
            StarFishStartUpFlag::enableHitTestDump) {
            printf("hitTest Result-> ");
            frame->node()->dump();
            puts("");
        }
#endif
        return frame->node();
    }

    return nullptr;
}

void Window::setActiveNode(Node* n)
{
    Node* t = n;
    while (t) {
        t->setState(Node::NodeStateActive,
                    Node::ChildrenOrSiblingsAffectedByActive, true);
        m_activeNodes.push_back(t);
        t = t->parentNode();
    }
    m_activeNodeWithTouchDown = n;
}

void Window::releaseActiveNode()
{
    for (size_t i = 0; i < m_activeNodes.size(); i++) {
        m_activeNodes[i]->setState(Node::NodeStateActive,
                                   Node::ChildrenOrSiblingsAffectedByActive,
                                   false);
    }
    m_activeNodes.clear();
    m_activeNodes.shrink_to_fit();
    m_activeNodeWithTouchDown = nullptr;
}

void Window::setFocusedNode(Node* n)
{
    Node* m = n;
    while (!(m->isElement() && m->asElement()->isFocusable()) &&
           !m->isDocument()) {
        m = m->parentNode();
    }

    if (!m || m->isDocument()) {
        if (!this->document()->bodyElement()) {
            return;
        }
        m = this->document()->bodyElement()->asNode();
    }

    if (m_focusedNode == m) {
        return;
    }

    m->setState(Node::NodeStateFocused, Node::ChildrenOrSiblingsAffectedByFocus,
                true);

    Node* t = m_focusedNode;
    String* eventType;
    Event* e;
    if (t) {
        if (t->isElement() && t->asElement()->isHTMLElement()) {
            eventType = starFish()->staticStrings()->m_blur.localName();
            e = new FocusEvent(eventType, EventInit(false, false));
            EventTarget::dispatchEvent(t->asNode(), e);
        }
        if (t->isElement() && t->asElement()->isHTMLElement() &&
            !t->asElement()->asHTMLElement()->isHTMLBodyElement()) {
            eventType = starFish()->staticStrings()->m_focusout.localName();
            e = new FocusEvent(eventType, EventInit(true, false));
            EventTarget::dispatchEvent(t->asNode(), e);
        }
    }
    m_relatedTarget = t;
    releaseFocusedNode();

    t = m;
    if (t) {
        if (t->isElement() && t->asElement()->isHTMLElement()) {
            eventType = starFish()->staticStrings()->m_focus.localName();
            e = new FocusEvent(eventType, EventInit(false, false));
            EventTarget::dispatchEvent(t->asNode(), e);
        }
        if (t->isElement() && t->asElement()->isHTMLElement() &&
            !t->asElement()->asHTMLElement()->isHTMLBodyElement()) {
            eventType = starFish()->staticStrings()->m_focusin.localName();
            e = new FocusEvent(eventType, EventInit(true, false));
            EventTarget::dispatchEvent(t->asNode(), e);
        }
    }
    m_focusedNode = t;
}

void Window::releaseFocusedNode()
{
    if (m_relatedTarget) {
        m_relatedTarget->setState(Node::NodeStateFocused,
                                  Node::ChildrenOrSiblingsAffectedByFocus,
                                  false);
    }
}

void Window::setActiveNodeWithMouseMove(Node* n)
{
    Node* t = n;

    while (!(t->isElement() && t->asElement()->isHTMLElement()) &&
           !t->isDocument()) {
        t = t->parentNode();
    }

    while (t) {
        t->setState(Node::NodeStateHovered,
                    Node::ChildrenOrSiblingsAffectedByHover, true);
        m_hoveredNodes.push_back(t);
        t = t->parentNode();
    }
    m_activeNodeWithTouchMove = n;
}

void Window::releaseActiveNodeWithMouseMove()
{
    for (size_t i = 0; i < m_hoveredNodes.size(); i++) {
        m_hoveredNodes[i]->setState(Node::NodeStateHovered,
                                    Node::ChildrenOrSiblingsAffectedByHover,
                                    false);
    }
    m_hoveredNodes.clear();
    m_hoveredNodes.shrink_to_fit();
    m_activeNodeWithTouchMove = nullptr;
}

void Window::processUrlFragment(String* name)
{
    Node* n = document()->getElementById(name);
    if (n) {
        setCSSTarget(n);
        return;
    }

    Node* anchor = Traverse::findDescendant(document(), [&](Node* child) {
        if (child->isElement() && child->asElement()->isHTMLElement() &&
            child->asElement()->asHTMLElement()->isHTMLAnchorElement() &&
            child->asElement()
                ->asHTMLElement()
                ->asHTMLAnchorElement()
                ->name()
                .localName()
                ->equals(name)) {
            return true;
        } else {
            return false;
        }
    });

    if (anchor) {
        setCSSTarget(anchor);
    }
}

void Window::setCSSTarget(Node* n)
{
    releaseCSSTarget();

    m_cssTarget = n;
    if (m_cssTarget) {
        m_cssTarget->setState(Node::NodeStateTarget, Node::NotAffected, true);
    }
}

void Window::releaseCSSTarget()
{
    if (m_cssTarget) {
        m_cssTarget->setState(Node::NodeStateTarget, Node::NotAffected, false);
    }
}

void Window::dispatchTouchEvent(float x, float y, TouchEventKind kind)
{
    // STARFISH_LOG_INFO("Window::dispatchTouchEvent %f %f kind %d\n", x, y,
    // (int)kind);
    if (!m_isRunning) {
        return;
    }

    if (kind == TouchEventDown) {
        Node* node = hitTest(x, y);
        if (!node) {
            // STARFISH_LOG_INFO("dispatchTouchEvent: hitTest is NULL\n");
            return;
        }
        m_touchDownPoint = Location(x, y);
        setActiveNode(node);
        setFocusedNode(node);
    } else if (kind == TouchEventMove) {
        if ((starFish()->deviceKind() & deviceKindUseTouchScreen) &&
            m_activeNodeWithTouchDown &&
            ((abs(m_touchDownPoint.x() - x) > 30) ||
             (abs(m_touchDownPoint.y() - y) > 30))) {
            releaseActiveNode();
            m_activeNodeWithTouchDown = nullptr;
        }
    } else if (kind == TouchEventCancel) {
        if (m_activeNodeWithTouchDown) {
            releaseActiveNode();
            m_activeNodeWithTouchDown = nullptr;
        }
    } else {
        STARFISH_ASSERT(kind == TouchEventUp);
        bool shouldCallOnClick = false;
        Node* node = hitTest(x, y);
        if (m_activeNodeWithTouchDown == node) {
            shouldCallOnClick = true;
        }

        Node* t = m_activeNodeWithTouchDown;

        bool shouldDispatchEvent = shouldCallOnClick;
        while (t) {
            if (shouldDispatchEvent &&
                (t->isElement() && t->asElement()->isHTMLElement())) {
                String* eventType =
                    starFish()->staticStrings()->m_click.localName();
                Event* e = new MouseEvent(eventType, EventInit(true, true));
                EventTarget::dispatchEvent(t->asNode(), e);
                shouldDispatchEvent = false;
                break;
            }
            t = t->parentNode();
        }

        if (shouldDispatchEvent) {
            if (t == nullptr) {
                t = m_document;
            }
            String* eventType =
                starFish()->staticStrings()->m_click.localName();
            Event* e = new MouseEvent(eventType, EventInit(true, true));
            EventTarget::dispatchEvent(t->asDocument(), e);
        }

        releaseActiveNode();

        m_activeNodeWithTouchDown = nullptr;
    }
}

void Window::dispatchMouseEvent(float x, float y, MouseEventKind kind)
{
    // STARFISH_LOG_INFO("Window::dispatchMouseEvent %f %f kind %d\n", x, y,
    // (int)kind);
    if (!m_isRunning) {
        return;
    }

    if (kind == MouseEventDown) {
        Node* node = hitTest(x, y);
        if (!node) {
            // STARFISH_LOG_INFO("MouseEventDown: hitTest is NULL\n");
            return;
        }
        m_touchDownPoint = Location(x, y);
        setActiveNode(node);
        setFocusedNode(node);
    } else if (kind == MouseEventMove) {
        if ((starFish()->deviceKind() & deviceKindUseTouchScreen) &&
            m_activeNodeWithTouchDown &&
            ((abs(m_touchDownPoint.x() - x) > 30) ||
             (abs(m_touchDownPoint.y() - y) > 30))) {
            releaseActiveNode();
            m_activeNodeWithTouchDown = nullptr;
        }
        Node* node = hitTest(x, y);

        if (!node) {
            return;
        }
        if (m_activeNodeWithTouchMove == nullptr ||
            node != m_activeNodeWithTouchMove) {
            if (m_activeNodeWithTouchMove != nullptr &&
                node != m_activeNodeWithTouchMove) {
                releaseActiveNodeWithMouseMove();
            }
            setActiveNodeWithMouseMove(node);
            if (m_activeNodeWithTouchMove &&
                node == m_activeNodeWithTouchMove) {
                bool shouldDispatchEvent = true;
                Node* t = m_activeNodeWithTouchMove;
                while (t) {
                    if ((t->isElement() && t->asElement()->isHTMLElement())) {
                        String* eventType = starFish()
                                                ->staticStrings()
                                                ->m_mouseover.localName();
                        Event* e =
                            new MouseEvent(eventType, EventInit(true, true));
                        EventTarget::dispatchEvent(t->asNode(), e);
                        shouldDispatchEvent = false;
                        break;
                    }
                    t = t->parentNode();
                }
                if (shouldDispatchEvent) {
                    if (t == nullptr) {
                        t = m_document;
                    }
                    String* eventType =
                        starFish()->staticStrings()->m_mouseover.localName();
                    Event* e = new MouseEvent(eventType, EventInit(true, true));
                    EventTarget::dispatchEvent(t->asDocument(), e);
                    shouldDispatchEvent = false;
                }
            }
        }
    } else if (kind == MouseEventCancel) {
        if (m_activeNodeWithTouchDown) {
            releaseActiveNode();
            m_activeNodeWithTouchDown = nullptr;
        }
    } else {
        STARFISH_ASSERT(kind == MouseEventUp);
        bool shouldCallOnClick = false;
        Node* node = hitTest(x, y);
        if (m_activeNodeWithTouchDown == node) {
            shouldCallOnClick = true;
        }

        Node* t = m_activeNodeWithTouchDown;

        bool shouldDispatchEvent = shouldCallOnClick;
        while (t) {
            if (shouldDispatchEvent &&
                (t->isElement() && t->asElement()->isHTMLElement())) {
                String* eventType =
                    starFish()->staticStrings()->m_click.localName();
                Event* e = new MouseEvent(eventType, EventInit(true, true));
                EventTarget::dispatchEvent(t->asNode(), e);
                shouldDispatchEvent = false;
                break;
            }
            t = t->parentNode();
        }

        if (shouldDispatchEvent) {
            if (t == nullptr) {
                t = m_document;
            }
            String* eventType =
                starFish()->staticStrings()->m_click.localName();
            Event* e = new MouseEvent(eventType, EventInit(true, true));
            EventTarget::dispatchEvent(t->asDocument(), e);
        }

        releaseActiveNode();

        m_activeNodeWithTouchDown = nullptr;
    }
}

void Window::dispatchKeyEvent(String* key, KeyEventKind kind)
{
    String* eventType = String::emptyString;
    if (kind == KeyEventKind::KeyEventUp) {
        eventType = starFish()->staticStrings()->m_keyup.localName();
    } else {
        // kind == KeyEventKind::KeyEventDown
        eventType = starFish()->staticStrings()->m_keydown.localName();
    }
    KeyboardEvent* e = new KeyboardEvent(eventType, key, EventInit(true, true));

    if (e->ctrlKey()) {
        m_ctrlKeyDown = kind == KeyEventKind::KeyEventDown ? m_ctrlKeyDown + 1
                                                           : m_ctrlKeyDown - 1;
        STARFISH_ASSERT(m_ctrlKeyDown >= 0);
    } else if (e->altKey()) {
        m_altKeyDown = kind == KeyEventKind::KeyEventDown ? m_altKeyDown + 1
                                                          : m_altKeyDown - 1;
        STARFISH_ASSERT(m_altKeyDown >= 0);
    } else if (e->shiftKey()) {
        m_shiftKeyDown = kind == KeyEventKind::KeyEventDown
                             ? m_shiftKeyDown + 1
                             : m_shiftKeyDown - 1;
        STARFISH_ASSERT(m_shiftKeyDown >= 0);
    } else if (e->shiftKey()) {
        m_metaKeyDown = kind == KeyEventKind::KeyEventDown ? m_metaKeyDown + 1
                                                           : m_metaKeyDown - 1;
        STARFISH_ASSERT(m_metaKeyDown >= 0);
    }
    if (m_ctrlKeyDown > 0) {
        e->setCtrlKey();
    }
    if (m_altKeyDown > 0) {
        e->setAltKey();
    }
    if (m_shiftKeyDown > 0) {
        e->setShiftKey();
    }
    if (m_metaKeyDown > 0) {
        e->setMetaKey();
    }

    // [Target]
    // 1) currently focused element if possible -> no focus concept
    // or 2) body element if possible
    // or 3) root element
    if (document()->rootElement()) {
        EventTarget::dispatchEvent((document()->bodyElement()
                                        ? document()->bodyElement()->asNode()
                                        : document()->rootElement()->asNode()),
                                   e);
    }
}

// https://html.spec.whatwg.org/multipage/browsers.html#named-access-on-the-window-object
HTMLCollection* Window::namedAccess(String* name)
{
    // TODO
    // when child browser context(ex- iframe) implemented, we should
    // re-implement this block
    if (document()) {
        return document()->namedAccess(name);
    } else {
        return nullptr;
    }
}

void Window::pause()
{
    STARFISH_LOG_INFO("Window::pause\n");
    if (!m_isRunning) {
        return;
    }

    m_isRunning = false;

    document()->setVisibleState(PageVisibilityState::PageVisibilityStateHidden);

    document()->resourceLoader()->cachePruning();
}

void Window::resume()
{
    STARFISH_LOG_INFO("Window::resume\n");
    if (m_isRunning) {
        m_needsRendering = true;
        m_needsPainting = true;
        rendering();
        return;
    }

    m_isRunning = true;
    m_needsRendering = true;
    m_needsPainting = true;
    rendering();

    document()->setVisibleState(
        PageVisibilityState::PageVisibilityStateVisible);
}

void Window::close()
{
    STARFISH_LOG_INFO("Window::close\n");
    clearEventListeners();

    if (m_navigator) {
        m_navigator->close();
    }

    m_focusedNode = nullptr;
    m_relatedTarget = nullptr;
    m_cssTarget = nullptr;

    m_activeNodeWithTouchDown = nullptr;

    m_hoveredNodes.clear();
    m_hoveredNodes.shrink_to_fit();
    m_activeNodeWithTouchMove = nullptr;

    if (m_location) {
        m_location->close();
    }

    if (m_document) {
        StarFishEnterer enter(m_starFish);
        m_document->close();
        delete m_document;
        m_document = nullptr;
    }

    if (m_scriptBindingInstance) {
        if (true) {
            StarFishEnterer enter(m_starFish);
            m_scriptBindingInstance->close();
        }
        delete m_scriptBindingInstance;
        m_scriptBindingInstance = nullptr;
    }

    WindowImplEFL* eflWindow = (WindowImplEFL*)this;
    eflWindow->m_isActive = false;

    auto timerIter = m_timeoutHandler.begin();
    while (timerIter != m_timeoutHandler.end()) {
        TimeoutData* td = (TimeoutData*)timerIter->second;
        ecore_timer_del(td->m_timerID);
        GC_FREE(td);
        timerIter++;
    }
    m_timeoutHandler.clear();

    auto aniIter = m_requestAnimationFrameHandler.begin();
    while (aniIter != m_requestAnimationFrameHandler.end()) {
        TimeoutData* td = (TimeoutData*)aniIter->second;
        ecore_animator_del((Ecore_Animator*)td->m_timerID);
        GC_FREE(td);
        aniIter++;
    }
    m_requestAnimationFrameHandler.clear();

    if (eflWindow->m_renderingAnimator) {
        ecore_animator_del(eflWindow->m_renderingAnimator);
        GC_FREE(eflWindow->m_renderingIdlerData);
    }

    eflWindow->clearEFLResources();

    eflWindow->m_objectList.clear();
    eflWindow->m_objectList.shrink_to_fit();
    eflWindow->m_surfaceList.clear();
    eflWindow->m_surfaceList.shrink_to_fit();

    m_starFish->messageLoop()->clearPendingIdlers();
    m_starFish->clearBlobURLStore();
}
}
