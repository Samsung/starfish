/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#ifdef PORT_GRAPHIC_BACKEND_EFL
#include "StarFish.h"

#include "core/dom/MouseEvent.h"
#include "core/dom/TouchEvent.h"
#include "core/dom/KeyboardEvent.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "platform/window/PlatformWindow.h"

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

#ifndef STARFISH_TIZEN_WEARABLE_LIB
extern "C" Ecore_Evas* ecore_evas_ecore_evas_get(const Evas* e);
extern "C" Ecore_Window ecore_evas_window_get(const Ecore_Evas* e);
#endif

#ifdef STARFISH_ENABLE_TEST
extern bool g_fireOnloadEvent;
extern Evas_Object* g_imgBufferForScreehShot;
extern StarFish::CanvasSurface* g_surfaceForScreehShot;
#endif

// #define STARFISH_ENABLE_TIMER
namespace StarFish {

struct IdlerData {
    void (*m_fn)(void*);
    void* m_data;
};

class WindowImplEFL : public PlatformWindow {
public:
    WindowImplEFL(StarFish* sf)
        : PlatformWindow(sf)
    {
        m_mainBox = nullptr;
        m_dummyBox = nullptr;
        m_dummyBoxClipper = nullptr;
        m_renderingAnimator = nullptr;
        m_renderingIdlerData = nullptr;
        m_isMouseLbuttonDown = false;

        GC_REGISTER_FINALIZER_NO_ORDER(
            this,
            [](void* obj, void* cd) {
                STARFISH_LOG_INFO("WindowImplEFL::~WindowImplEFL\n");
            },
            NULL, NULL, NULL);
    }

    virtual int32_t width() override
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

    virtual int32_t height() override
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

    virtual void clearResources();
    virtual Canvas* preparePainting(bool forPainting);

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
    bool m_isMouseLbuttonDown;
};

class CanvasSurfaceEFL : public CanvasSurface {
public:
    CanvasSurfaceEFL(PlatformWindow* wnd, size_t w, size_t h)
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

        auto iter = std::find(s->m_window->m_surfaceList.begin(),
                              s->m_window->m_surfaceList.end(), s->m_image);
        if (s->m_window->m_surfaceList.end() != iter) {
            s->m_window->m_surfaceList.erase(iter);
        }
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

CanvasSurface* CanvasSurface::create(PlatformWindow* wnd, size_t w, size_t h)
{
    return new CanvasSurfaceEFL(wnd, w, h);
}

static void mainRenderingFunction(Evas_Object* o, Evas_Object_Box_Data* priv,
                                  void* user_data)
{
    ecore_animator_add(
        [](void* user_data) -> Eina_Bool {
            WindowImplEFL* wnd = (WindowImplEFL*)user_data;
            wnd->webView()->mainBrowsingContext()->window()->resize(
                wnd->width(), wnd->height());
            return ECORE_CALLBACK_CANCEL;
        },
        user_data);
}

PlatformWindow* PlatformWindow::create(StarFish* sf, void* win, int width,
                                       int height)
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
            WindowImplEFL* sf = (WindowImplEFL*)data;
            Ecore_Event_Mouse_Button* d = (Ecore_Event_Mouse_Button*)event;
            // We care just left button now
            if (d->buttons == 1) {
                StarFishEnterer enter(sf->starFish());
                MouseData mdata(MouseData::MouseButtonValue::LeftButton,
                                MouseData::MouseButtonsValue::LeftButtonDown,
                                d->x, d->y);
                sf->dispatchMouseEvent(PlatformWindow::MouseEventDown, mdata);
                sf->m_isMouseLbuttonDown = true;
            }
            return EINA_TRUE;
        },
        wnd);

    wnd->m_desktopMouseUpEventHandler = ecore_event_handler_add(
        ECORE_EVENT_MOUSE_BUTTON_UP,
        [](void* data, int type, void* event) -> Eina_Bool {
            WindowImplEFL* sf = (WindowImplEFL*)data;
            Ecore_Event_Mouse_Button* d = (Ecore_Event_Mouse_Button*)event;
            // We care just left button now
            if (d->buttons == 1) {
                StarFishEnterer enter(sf->starFish());
                MouseData mdata(MouseData::MouseButtonValue::NoButton,
                                MouseData::MouseButtonsValue::NoButtonDown,
                                d->x, d->y);
                sf->dispatchMouseEvent(PlatformWindow::MouseEventUp, mdata);
                sf->m_isMouseLbuttonDown = false;
            }
            return EINA_TRUE;
        },
        wnd);

    wnd->m_desktopMouseMoveEventHandler = ecore_event_handler_add(
        ECORE_EVENT_MOUSE_MOVE,
        [](void* data, int type, void* event) -> Eina_Bool {
            WindowImplEFL* sf = (WindowImplEFL*)data;
            Ecore_Event_Mouse_Move* d = (Ecore_Event_Mouse_Move*)event;
            StarFishEnterer enter(sf->m_starFish);
            unsigned char buttons =
                sf->m_isMouseLbuttonDown
                    ? MouseData::MouseButtonsValue::LeftButtonDown
                    : 0;
            MouseData mdata(0, buttons, d->x, d->y);
            sf->dispatchMouseEvent(PlatformWindow::MouseEventMove, mdata);
            return EINA_TRUE;
        },
        wnd);

    wnd->m_desktopKeyDownEventHandler = ecore_event_handler_add(
        ECORE_EVENT_KEY_DOWN,
        [](void* data, int type, void* event) -> Eina_Bool {
            PlatformWindow* sf = (PlatformWindow*)data;
            Ecore_Event_Key* d = (Ecore_Event_Key*)event;
            StarFishEnterer enter(sf->m_starFish);
            sf->dispatchKeyEvent(String::createASCIIString(d->keyname),
                                 PlatformWindow::KeyEventDown);
            return EINA_TRUE;
        },
        wnd);

    wnd->m_desktopKeyUpEventHandler = ecore_event_handler_add(
        ECORE_EVENT_KEY_UP,
        [](void* data, int type, void* event) -> Eina_Bool {
            PlatformWindow* sf = (PlatformWindow*)data;
            Ecore_Event_Key* d = (Ecore_Event_Key*)event;
            StarFishEnterer enter(sf->m_starFish);
            sf->dispatchKeyEvent(String::createASCIIString(d->keyname),
                                 PlatformWindow::KeyEventUp);
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
        PlatformWindow* sf = (PlatformWindow*)data;
        Evas_Event_Mouse_Down* ev = (Evas_Event_Mouse_Down*)event_info;
        ((WindowImplEFL*)sf)->m_lastMouseX = ev->canvas.x;
        ((WindowImplEFL*)sf)->m_lastMouseY = ev->canvas.y;
        StarFishEnterer enter(sf->m_starFish);
        TouchData data(ev->canvas.x, ev->canvas.y);
        sf->dispatchTouchEvent(Window::TouchEventStart, &data, 1);
        return;
    };
    evas_object_event_callback_add(wnd->m_dummyBox, EVAS_CALLBACK_MOUSE_DOWN,
                                   wnd->m_mobileMouseDownEventHandler, wnd);

    wnd->m_mobileMouseMoveEventHandler =
        [](void* data, Evas* evas, Evas_Object* obj, void* event_info) -> void {
        PlatformWindow* sf = (PlatformWindow*)data;
        Evas_Event_Mouse_Move* ev = (Evas_Event_Mouse_Move*)event_info;
        ((WindowImplEFL*)sf)->m_lastMouseX = ev->cur.canvas.x;
        ((WindowImplEFL*)sf)->m_lastMouseY = ev->cur.canvas.y;
        StarFishEnterer enter(sf->m_starFish);
        TouchData data(ev->cur.canvas.x, ev->cur.canvas.y);
        sf->dispatchTouchEvent(Window::TouchEventMove, &data, 1);
        return;
    };
    evas_object_event_callback_add(wnd->m_dummyBox, EVAS_CALLBACK_MOUSE_MOVE,
                                   wnd->m_mobileMouseMoveEventHandler, wnd);

    wnd->m_mobileMouseUpEventHandler =
        [](void* data, Evas* evas, Evas_Object* obj, void* event_info) -> void {
        PlatformWindow* sf = (PlatformWindow*)data;
        sf->starFish()->messageLoop()->addIdler(
            sf->webView()->mainBrowsingContext(),
            [](size_t a, void* data) {
                ((PlatformWindow*)data)
                    ->dispatchTouchEvent(Window::TouchEventCancel, nullptr, 0);
            },
            sf);
        return;
    };
    evas_object_event_callback_add(wnd->m_dummyBox, EVAS_CALLBACK_MOUSE_UP,
                                   wnd->m_mobileMouseUpEventHandler, wnd);

    wnd->m_mobileClickEventHandler = [](void* data, Evas_Object* obj,
                                        void* event_info) -> void {
        PlatformWindow* sf = (PlatformWindow*)data;
        StarFishEnterer enter(sf->m_starFish);
        TouchData data(((WindowImplEFL*)sf)->m_lastMouseX,
                       ((WindowImplEFL*)sf)->m_lastMouseY);
        sf->dispatchTouchEvent(Window::TouchEventEnd, &data, 1);
    };
    evas_object_smart_callback_add(wnd->m_dummyBox, "clicked",
                                   wnd->m_mobileClickEventHandler, wnd);
#endif
    return wnd;
}

PlatformWindow::~PlatformWindow()
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

#endif
}

void WebView::setNeedsRenderingSlowCase()
{
    STARFISH_ASSERT(!m_needsRendering);
    m_needsRendering = true;

    IdlerData* id = new (NoGC) IdlerData;
    id->m_fn = [](void* data) -> void {
        PlatformWindow* wnd = (PlatformWindow*)data;
        wnd->rendering();
    };
    id->m_data = starFish()->platformWindow();

    ((WindowImplEFL*)starFish()->platformWindow())->m_renderingIdlerData = id;
    ((WindowImplEFL*)starFish()->platformWindow())->m_renderingAnimator =
        ecore_animator_add(
            [](void* data) -> Eina_Bool {
                IdlerData* id = (IdlerData*)data;
                PlatformWindow* wnd = (PlatformWindow*)id->m_data;
                StarFishEnterer enter(wnd->starFish());
                id->m_fn(id->m_data);
                ((WindowImplEFL*)wnd)->m_renderingAnimator = nullptr;
                ((WindowImplEFL*)wnd)->m_renderingIdlerData = nullptr;
                GC_FREE(id);
                return ECORE_CALLBACK_CANCEL;
            },
            id);
}

Canvas* WindowImplEFL::preparePainting(bool forPainting)
{
#ifdef STARFISH_ENABLE_TEST
    {
        const char* path = getenv("SCREEN_SHOT");
        if (path && strlen(path) && g_fireOnloadEvent) {
            g_surfaceForScreehShot =
                CanvasSurface::create(this, width(), height());
            g_imgBufferForScreehShot =
                (Evas_Object*)g_surfaceForScreehShot->unwrap();
            return Canvas::create(g_surfaceForScreehShot);
        }
    }
#endif
    int width, height;
    evas_object_geometry_get(m_window, NULL, NULL, &width, &height);
    Evas* evas = evas_object_evas_get(m_window);
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
    d->objList = &m_objectList;
    d->surfaceList = &m_surfaceList;
    auto iter = m_objectList.begin();
    while (iter != m_objectList.end()) {
        evas_object_del(*iter);
        iter++;
    }
    m_objectList.clear();
    m_objectList.shrink_to_fit();

    iter = m_surfaceList.begin();
    while (iter != m_surfaceList.end()) {
        evas_object_hide(*iter);
        iter++;
    }
    m_surfaceList.clear();
    m_surfaceList.shrink_to_fit();

    Canvas* canvas = Canvas::createDirect(d);
    delete d;

    return canvas;
}

void WindowImplEFL::clearResources()
{
    if (m_renderingAnimator) {
        ecore_animator_del(m_renderingAnimator);
        GC_FREE(m_renderingIdlerData);
    }

    webView()->clearStackingContext(false);

    m_objectList.clear();
    m_objectList.shrink_to_fit();
    m_surfaceList.clear();
    m_surfaceList.shrink_to_fit();
}
}
#endif
