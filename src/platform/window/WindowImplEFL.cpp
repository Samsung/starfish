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

#include "core/dom/Element.h"
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
#include "core/modules/threading/Thread.h"
#include "core/dom/CompositionEvent.h"

#include <Elementary.h>
#include <Evas_Engine_Buffer.h>
#if defined(STARFISH_TIZEN_3_0) || defined(STARFISH_TIZEN_OBS)
#include <Ecore.h>
#else
#include <Ecore_X.h>
#endif
#include <Ecore_Input.h>
#include <Ecore_Input_Evas.h>
#include <Ecore_IMF.h>
#include <Ecore_IMF_Evas.h>

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
        m_isMouseLbuttonDown = false;
        m_isKeyDown = false;
        m_lastClickedTimestamp = 0;
        m_clickedCount = 0;
        m_canRendering = true;
        m_imfContext = nullptr;
        m_lastKeyPressedTimestamp = 0;
        m_offsetYDueToSoftwareKeyboard = 0;

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
        return height - m_offsetYDueToSoftwareKeyboard;
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

    virtual void showSoftwareKeyboardIfPossible()
    {
        evas_object_focus_set(m_mainBox, EINA_TRUE);
    }
    virtual void hideSoftwareKeyboardIfPossible()
    {
        evas_object_focus_set(m_mainBox, EINA_FALSE);
        ecore_imf_context_hide(m_imfContext);
    }

    void adjustOffsetYDueToFocusChanging()
    {
        int x, y, w, h;
        ecore_imf_context_input_panel_geometry_get(m_imfContext, &x, &y, &w,
                                                   &h);

        if (h != m_offsetYDueToSoftwareKeyboard) {
            m_offsetYDueToSoftwareKeyboard = h;
            onResize();

            if (webView()->hasFocus()) {
                Node* nd = webView()->focusedNode();
                Node* e = nd->nearestParentElement();
                if (e->isElement()) {
                    // FIXME (enable this)
                    // e->asElement()->scrollIntoView(true);
                }
            }
        }
    }

    virtual bool isIMEEnabledNow()
    {
        return evas_object_focus_get(m_mainBox) == EINA_TRUE;
    }

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
    Ecore_Event_Handler* m_desktopMouseWheelEventHandler;
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

    Ecore_IMF_Context* m_imfContext;

    float m_lastMouseX, m_lastMouseY;
    bool m_isMouseLbuttonDown;
    bool m_isKeyDown;
    bool m_canRendering;
    uint32_t m_lastClickedTimestamp;
    uint32_t m_clickedCount;
    uint32_t m_lastKeyPressedTimestamp;
    int m_offsetYDueToSoftwareKeyboard;
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
            wnd->onResize();
            return ECORE_CALLBACK_CANCEL;
        },
        user_data);
}

static KeyValue ecoreEventKeyToKeyValue(const char* ecoreKeyString,
                                        bool isShiftPressed)
{
    if (strcmp("Left", ecoreKeyString) == 0) {
        return KeyValue::ArrowLeftKey;
    } else if (strcmp("Right", ecoreKeyString) == 0) {
        return KeyValue::ArrowRightKey;
    } else if (strcmp("Up", ecoreKeyString) == 0) {
        return KeyValue::ArrowUpKey;
    } else if (strcmp("Down", ecoreKeyString) == 0) {
        return KeyValue::ArrowDownKey;
    } else if (strcmp("space", ecoreKeyString) == 0) {
        return KeyValue::SpaceKey;
    } else if (strcmp("Return", ecoreKeyString) == 0) {
        return KeyValue::EnterKey;
    } else if (strcmp("BackSpace", ecoreKeyString) == 0) {
        return KeyValue::BackspaceKey;
    } else if (strcmp("Escape", ecoreKeyString) == 0) {
        return KeyValue::EscapeKey;
    } else if (strcmp("minus", ecoreKeyString) == 0) {
        if (isShiftPressed) {
            return KeyValue::MinusMarkKey;
        } else {
            return KeyValue::UnderScoreMarkKey;
        }
    } else if (strcmp("equal", ecoreKeyString) == 0) {
        if (isShiftPressed) {
            return KeyValue::PlusMarkKey;
        } else {
            return KeyValue::EqualitySignKey;
        }
    } else if (strcmp("bracketleft", ecoreKeyString) == 0) {
        if (isShiftPressed) {
            return KeyValue::LeftCurlyBracketMarkKey;
        } else {
            return KeyValue::LeftSquareBracketKey;
        }
    } else if (strcmp("bracketright", ecoreKeyString) == 0) {
        if (isShiftPressed) {
            return KeyValue::RightCurlyBracketMarkKey;
        } else {
            return KeyValue::RightSquareBracketKey;
        }
    } else if (strcmp("semicolon", ecoreKeyString) == 0) {
        if (isShiftPressed) {
            return KeyValue::ColonMarkKey;
        } else {
            return KeyValue::SemiColonMarkKey;
        }
    } else if (strcmp("apostrophe", ecoreKeyString) == 0) {
        if (isShiftPressed) {
            return KeyValue::DoubleQuoteMarkKey;
        } else {
            return KeyValue::SingleQuoteMarkKey;
        }
    } else if (strcmp("comma", ecoreKeyString) == 0) {
        if (isShiftPressed) {
            return KeyValue::LessThanMarkKey;
        } else {
            return KeyValue::CommaMarkKey;
        }
    } else if (strcmp("period", ecoreKeyString) == 0) {
        if (isShiftPressed) {
            return KeyValue::GreaterThanSignKey;
        } else {
            return KeyValue::PeriodKey;
        }
    } else if (strcmp("slash", ecoreKeyString) == 0) {
        if (isShiftPressed) {
            return KeyValue::QuestionMarkKey;
        } else {
            return KeyValue::SlashKey;
        }
    } else if (strlen(ecoreKeyString) == 1) {
        char ch = ecoreKeyString[0];
        if (ch >= '0' && ch <= '9') {
            if (isShiftPressed) {
                switch (ch) {
                case '1':
                    return KeyValue::ExclamationMarkKey;
                case '2':
                    return KeyValue::AtMarkKey;
                case '3':
                    return KeyValue::SharpMarkKey;
                case '4':
                    return KeyValue::DollarMarkKey;
                case '5':
                    return KeyValue::PercentMarkKey;
                case '6':
                    return KeyValue::CaretMarkKey;
                case '7':
                    return KeyValue::AmpersandMarkKey;
                case '8':
                    return KeyValue::AsteriskMarkKey;
                case '9':
                    return KeyValue::LeftParenthesisMarkKey;
                case '0':
                    return KeyValue::RightParenthesisMarkKey;
                }
            }
            return (KeyValue)(KeyValue::Digit0Key + ch - '0');
        } else if (ch >= 'a' && ch <= 'z') {
            int kv = KeyValue::LowerAKey + ch - 'a';
            if (isShiftPressed) {
                kv -= ('z' - 'a');
            }
            return (KeyValue)kv;
        }
    }
    STARFISH_LOG_ERROR("WindowImplEFL - unimplemented key %s\n",
                       ecoreKeyString);
    return KeyValue::UnidentifiedKey;
}

static void setModifiersToKeyboardData(Ecore_Event_Key* d, KeyboardData& k)
{
    if (d->modifiers == 1 || d->keycode == 50 || d->keycode == 62) {
        k.setShiftKey();
    } else if (d->modifiers == 2 || d->keycode == 37) {
        k.setCtrlKey();
    } else if (d->modifiers == 4 || d->keycode == 64) {
        k.setAltKey();
    }
}

static void setModifiersToKeyboardData(Evas_Modifier* d, KeyboardData& k)
{
    if ((evas_key_modifier_is_set(d, "Shift_L") == EINA_TRUE) ||
        (evas_key_modifier_is_set(d, "Shift_R") == EINA_TRUE)) {
        k.setShiftKey();
    } else if ((evas_key_modifier_is_set(d, "Control_L") == EINA_TRUE) ||
               (evas_key_modifier_is_set(d, "Control_R") == EINA_TRUE)) {
        k.setCtrlKey();
    } else if ((evas_key_modifier_is_set(d, "Alt_L") == EINA_TRUE) ||
               (evas_key_modifier_is_set(d, "Alt_R") == EINA_TRUE)) {
        k.setAltKey();
    }
}

static void setRepeatToKeyboardData(WindowImplEFL* window, uint32_t timestamp,
                                    KeyboardData& k)
{
    if (!window->m_isKeyDown) {
        window->m_lastKeyPressedTimestamp = timestamp;
        return;
    }

    if (timestamp - window->m_lastKeyPressedTimestamp < REPEAT_DURATION) {
        k.setRepeat();
    }
}

static const char* getImfMethod()
{
    Eina_List* modules;

    modules = ecore_imf_context_available_ids_get();
    if (!modules)
        return NULL;

    void* module;
    EINA_LIST_FREE(modules, module)
    {
        return (const char*)module;
    }

    return NULL;
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
                if (d->timestamp - sf->m_lastClickedTimestamp >
                    CLICK_REFRESH_DELAY) {
                    sf->m_clickedCount = 1;
                    sf->m_lastClickedTimestamp = d->timestamp;
                } else {
                    sf->m_clickedCount++;
                }
                MouseData mdata(MouseData::MouseButtonValue::LeftButton,
                                MouseData::MouseButtonsValue::LeftButtonDown,
                                d->x, d->y, sf->m_clickedCount);
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
                                d->x, d->y, sf->m_clickedCount);
                sf->dispatchMouseEvent(PlatformWindow::MouseEventUp, mdata);
                sf->m_isMouseLbuttonDown = false;
            }
            return EINA_TRUE;
        },
        wnd);

    wnd->m_desktopMouseWheelEventHandler = ecore_event_handler_add(
        ECORE_EVENT_MOUSE_WHEEL,
        [](void* data, int type, void* event) -> Eina_Bool {
            WindowImplEFL* sf = (WindowImplEFL*)data;
            Ecore_Event_Mouse_Wheel* d = (Ecore_Event_Mouse_Wheel*)event;
            // We only care vertical wheel
            sf->dispatchMouseWheelEvent(d->x, d->y, d->z, true);
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
            MouseData mdata(0, buttons, d->x, d->y, 0);
            sf->dispatchMouseEvent(PlatformWindow::MouseEventMove, mdata);
            return EINA_TRUE;
        },
        wnd);

    wnd->m_desktopKeyDownEventHandler = ecore_event_handler_add(
        ECORE_EVENT_KEY_DOWN,
        [](void* data, int type, void* event) -> Eina_Bool {
            WindowImplEFL* sf = (WindowImplEFL*)data;
            if (evas_object_focus_get(sf->m_mainBox) == EINA_TRUE) {
                return EINA_TRUE;
            }
            Ecore_Event_Key* d = (Ecore_Event_Key*)event;

#ifdef STARFISH_TIZEN_TV
            if ((strcmp(d->key, "XF86Exit") == 0) ||
                (strcmp(d->key, "XF86Back") == 0)) {
                evas_object_del(sf->m_window);
                return EINA_FALSE;
            }
#endif

            auto keyValue = ecoreEventKeyToKeyValue(d->key, d->modifiers & 1);
            KeyboardData kdata(keyValue);
            setRepeatToKeyboardData(sf, d->timestamp, kdata);
            setModifiersToKeyboardData(d, kdata);
            StarFishEnterer enter(sf->m_starFish);
            sf->dispatchKeyEvent(PlatformWindow::KeyEventDown, kdata);
            sf->m_isKeyDown = true;
            return EINA_TRUE;
        },
        wnd);

    wnd->m_desktopKeyUpEventHandler = ecore_event_handler_add(
        ECORE_EVENT_KEY_UP,
        [](void* data, int type, void* event) -> Eina_Bool {
            WindowImplEFL* sf = (WindowImplEFL*)data;
            if (evas_object_focus_get(sf->m_mainBox) == EINA_TRUE) {
                return EINA_TRUE;
            }
            Ecore_Event_Key* d = (Ecore_Event_Key*)event;
            auto keyValue = ecoreEventKeyToKeyValue(d->key, d->modifiers & 1);
            KeyboardData kdata(keyValue);
            setModifiersToKeyboardData(d, kdata);
            StarFishEnterer enter(sf->m_starFish);
            sf->dispatchKeyEvent(PlatformWindow::KeyEventUp, kdata);
            sf->m_isKeyDown = false;
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

    // Rendering control callback
    evas_event_callback_add(evas_object_evas_get(wnd->m_window),
                            EVAS_CALLBACK_RENDER_POST,
                            [](void* data, Evas* e, void* event_info) {
                                WindowImplEFL* wnd = (WindowImplEFL*)data;
                                STARFISH_RELEASE_ASSERT(isMainThread());
                                wnd->m_canRendering = true;
                            },
                            wnd);

    ecore_imf_init();
    // Register IMF callbacks
    if (ecore_imf_context_default_id_get()) {
        wnd->m_imfContext =
            ecore_imf_context_add(ecore_imf_context_default_id_get());
    } else {
        STARFISH_LOG_ERROR(
            "ecore_imf_context_default_id_get returns null.. use fallback "
            "method\n");
        wnd->m_imfContext = ecore_imf_context_add(getImfMethod());
    }

    ecore_imf_context_client_window_set(
        wnd->m_imfContext,
        (void*)ecore_evas_window_get(
            ecore_evas_ecore_evas_get(evas_object_evas_get(wnd->m_window))));
    ecore_imf_context_client_canvas_set(wnd->m_imfContext,
                                        evas_object_evas_get(wnd->m_window));

    ecore_imf_context_retrieve_surrounding_callback_set(
        wnd->m_imfContext,
        [](void* data, Ecore_IMF_Context* ctx, char** text,
           int* cursor_pos) -> Eina_Bool {
            // This callback will be called when the Input Method Context module
            // requests the surrounding context.
            if (text)
                *text = strdup("");
            if (cursor_pos)
                *cursor_pos = 0;
            return EINA_TRUE;
        },
        wnd);

    // register commit event callback
    ecore_imf_context_event_callback_add(
        wnd->m_imfContext, ECORE_IMF_CALLBACK_COMMIT,
        [](void* data, Ecore_IMF_Context* ctx, void* event_info) {
            char* commit_str = (char*)event_info;
            WindowImplEFL* self = (WindowImplEFL*)data;
            STARFISH_LOG_INFO("ECORE_IMF_CALLBACK_COMMIT %s\n", commit_str);

            bool isAllASCII = true;
            String* str = String::fromUTF8(commit_str);
            for (size_t i = 0; i < str->length(); i++) {
                if (str->charAt(i) < 128 &&
                    String::isASCIIPrintableKey(str->charAt(i))) {
                } else {
                    isAllASCII = false;
                    break;
                }
            }

            if (isAllASCII) {
                for (size_t i = 0; i < str->length(); i++) {
                    // ASCII char
                    KeyValue kv = (KeyValue)str->charAt(i);
                    KeyboardData kdata(kv);
                    StarFishEnterer enter(self->m_starFish);
                    self->dispatchKeyEvent(PlatformWindow::KeyEventDown, kdata);
                    self->dispatchKeyEvent(PlatformWindow::KeyEventUp, kdata);
                }
            } else {
                // non-ASCII char
                self->dispatchCompositionEvent(
                    PlatformWindow::CompositionEventEnd,
                    String::fromUTF8(commit_str));
            }

        },
        wnd);

    ecore_imf_context_event_callback_add(
        wnd->m_imfContext, ECORE_IMF_CALLBACK_PREEDIT_END,
        [](void* data, Ecore_IMF_Context* ctx, void* event_info) {
            WindowImplEFL* self = (WindowImplEFL*)data;
            char* str = NULL;
            int cursor_pos;
            ecore_imf_context_preedit_string_get(self->m_imfContext, &str,
                                                 &cursor_pos);
            STARFISH_LOG_INFO("ECORE_IMF_CALLBACK_PREEDIT_END %s %d\n", str,
                              cursor_pos);
            if (str) {
                free(str);
            }
        },
        wnd);

    ecore_imf_context_event_callback_add(
        wnd->m_imfContext, ECORE_IMF_CALLBACK_PREEDIT_START,
        [](void* data, Ecore_IMF_Context* ctx, void* event_info) {
            WindowImplEFL* self = (WindowImplEFL*)data;
            STARFISH_LOG_INFO("ECORE_IMF_CALLBACK_PREEDIT_START\n");
            self->dispatchCompositionEvent(
                PlatformWindow::CompositionEventStart, String::emptyString);
        },
        wnd);

    // register preedit changed event handler
    ecore_imf_context_event_callback_add(
        wnd->m_imfContext, ECORE_IMF_CALLBACK_PREEDIT_CHANGED,
        [](void* data, Ecore_IMF_Context* ctx, void* event_info) {
            WindowImplEFL* self = (WindowImplEFL*)data;
            char* str = NULL;
            int cursor_pos;
            ecore_imf_context_preedit_string_get(self->m_imfContext, &str,
                                                 &cursor_pos);
            STARFISH_LOG_INFO("ECORE_IMF_CALLBACK_PREEDIT_CHANGED %s %d\n", str,
                              cursor_pos);
            if (str) {
                self->dispatchCompositionEvent(
                    PlatformWindow::CompositionEventUpdate,
                    String::fromUTF8(str));
                free(str);
            }
        },
        wnd);

    ecore_imf_context_input_panel_event_callback_add(
        wnd->m_imfContext, ECORE_IMF_INPUT_PANEL_STATE_EVENT,
        [](void* data, Ecore_IMF_Context* ctx, int value) {
            WindowImplEFL* self = (WindowImplEFL*)data;
            if (ecore_imf_context_input_panel_state_get(ctx) ==
                ECORE_IMF_INPUT_PANEL_STATE_HIDE) {
                self->webView()->blur();
                self->adjustOffsetYDueToFocusChanging();
            } else if (ecore_imf_context_input_panel_state_get(ctx) ==
                       ECORE_IMF_INPUT_PANEL_STATE_SHOW) {
                self->adjustOffsetYDueToFocusChanging();
            }
        },
        wnd);

    // register key event handler
    evas_object_event_callback_add(
        wnd->m_mainBox, EVAS_CALLBACK_KEY_DOWN,
        [](void* data, Evas* e, Evas_Object* obj, void* event_info) {
            WindowImplEFL* self = (WindowImplEFL*)data;
            Evas_Event_Key_Down* ev = (Evas_Event_Key_Down*)event_info;
            STARFISH_LOG_INFO("EVAS_CALLBACK_KEY_DOWN for ime object [%s]\n",
                              ev->key);

            if ((strcmp(ev->key, "XF86Exit") == 0) ||
                (strcmp(ev->key, "Select") == 0) ||
                (strcmp(ev->key, "Cancel") == 0)) {
                if (strcmp(ev->key, "Select") == 0) {
                    self->m_starFish->messageLoop()->addIdler(
                        nullptr,
                        [](size_t, void* data) {
                            WindowImplEFL* self = (WindowImplEFL*)data;
                            StarFishEnterer enter(self->m_starFish);
                            KeyValue kv = KeyValue::EnterKey;
                            KeyboardData kdata(kv);
                            self->dispatchKeyEvent(PlatformWindow::KeyEventDown,
                                                   kdata);
                            self->dispatchKeyEvent(PlatformWindow::KeyEventUp,
                                                   kdata);
                            self->hideSoftwareKeyboardIfPossible();
                        },
                        self);
                } else {
                    self->m_starFish->messageLoop()->addIdler(
                        nullptr,
                        [](size_t, void* data) {
                            WindowImplEFL* self = (WindowImplEFL*)data;
                            self->hideSoftwareKeyboardIfPossible();
                        },
                        self);
                }
            }
            /*
            #ifdef STARFISH_TIZEN_TV
                        if (strcmp(ev->key, "Select") == 0) {
                            ev->key = "Return";
                        }
            #endif
            */
            Ecore_IMF_Event_Key_Down ecore_ev;
            ecore_imf_evas_event_key_down_wrap(ev, &ecore_ev);
            if (ecore_imf_context_filter_event(self->m_imfContext,
                                               ECORE_IMF_EVENT_KEY_DOWN,
                                               (Ecore_IMF_Event*)&ecore_ev)) {
                return;
            }
            // process non-char keys
            STARFISH_LOG_INFO("process non-char [%s]\n", ev->key);
            auto keyValue = ecoreEventKeyToKeyValue(
                ev->key, (evas_key_modifier_is_set(ev->modifiers, "Shift_L") ==
                          EINA_TRUE) ||
                             (evas_key_modifier_is_set(
                                  ev->modifiers, "Shift_R") == EINA_TRUE));
            KeyboardData kdata(keyValue);
            setRepeatToKeyboardData(self, ev->timestamp, kdata);
            setModifiersToKeyboardData(ev->modifiers, kdata);
            StarFishEnterer enter(self->m_starFish);
            self->dispatchKeyEvent(PlatformWindow::KeyEventDown, kdata);
            self->m_isKeyDown = true;
        },
        wnd);
    evas_object_event_callback_add(
        wnd->m_mainBox, EVAS_CALLBACK_KEY_UP,
        [](void* data, Evas* e, Evas_Object* obj, void* event_info) {
            WindowImplEFL* self = (WindowImplEFL*)data;
            Evas_Event_Key_Up* ev = (Evas_Event_Key_Up*)event_info;
            /*
#ifdef STARFISH_TIZEN_TV
            if (strcmp(ev->key, "Select") == 0) {
                ev->key = "Return";
            }
#endif
            */
            STARFISH_LOG_INFO("EVAS_CALLBACK_KEY_UP for ime object [%s]\n",
                              ev->key);
            Ecore_IMF_Event_Key_Up ecore_ev;
            ecore_imf_evas_event_key_up_wrap(ev, &ecore_ev);
            if (ecore_imf_context_filter_event(self->m_imfContext,
                                               ECORE_IMF_EVENT_KEY_UP,
                                               (Ecore_IMF_Event*)&ecore_ev)) {
                return;
            }
            // process non-char keys
            auto keyValue = ecoreEventKeyToKeyValue(
                ev->key, (evas_key_modifier_is_set(ev->modifiers, "Shift_L") ==
                          EINA_TRUE) ||
                             (evas_key_modifier_is_set(
                                  ev->modifiers, "Shift_R") == EINA_TRUE));
            KeyboardData kdata(keyValue);
            setModifiersToKeyboardData(ev->modifiers, kdata);
            StarFishEnterer enter(self->m_starFish);
            self->dispatchKeyEvent(PlatformWindow::KeyEventUp, kdata);
            self->m_isKeyDown = false;
        },
        wnd);

    evas_object_event_callback_add(
        wnd->m_mainBox, EVAS_CALLBACK_FOCUS_IN,
        [](void* data, Evas* e, Evas_Object* obj, void* event_info) {
            WindowImplEFL* self = (WindowImplEFL*)data;
            Ecore_IMF_Context* ctx = self->m_imfContext;
            Ecore_IMF_Event_Key_Down ev;
            ecore_imf_evas_event_key_down_wrap((Evas_Event_Key_Down*)event_info,
                                               &ev);
            ecore_imf_context_reset(ctx);
            ecore_imf_context_focus_in(ctx);
            ecore_imf_context_show(ctx);
        },
        wnd);

    evas_object_event_callback_add(
        wnd->m_mainBox, EVAS_CALLBACK_FOCUS_OUT,
        [](void* data, Evas* e, Evas_Object* obj, void* event_info) {
            WindowImplEFL* self = (WindowImplEFL*)data;
            Ecore_IMF_Context* ctx = self->m_imfContext;
            Ecore_IMF_Event_Key_Down ev;

            ecore_imf_evas_event_key_down_wrap((Evas_Event_Key_Down*)event_info,
                                               &ev);
            // ecore_imf_context_reset(ctx);
            ecore_imf_context_focus_out(ctx);
        },
        wnd);

    ecore_imf_context_autocapital_type_set(wnd->m_imfContext,
                                           ECORE_IMF_AUTOCAPITAL_TYPE_NONE);
    ecore_imf_context_prediction_allow_set(wnd->m_imfContext, EINA_FALSE);

    return wnd;
}

PlatformWindow::~PlatformWindow()
{
    STARFISH_LOG_INFO("PlatformWindow::~PlatformWindow\n");

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

    if (eflWindow->m_imfContext) {
        ecore_imf_context_del(eflWindow->m_imfContext);
    }

#ifndef STARFISH_TIZEN_WEARABLE
    ecore_event_handler_del(eflWindow->m_desktopMouseDownEventHandler);
    ecore_event_handler_del(eflWindow->m_desktopMouseUpEventHandler);
    ecore_event_handler_del(eflWindow->m_desktopMouseMoveEventHandler);
    ecore_event_handler_del(eflWindow->m_desktopMouseWheelEventHandler);
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

void WebView::setNeedsRendering()
{
    WindowImplEFL* wnd = (WindowImplEFL*)starFish()->platformWindow();

    // refresh rendering animator
    if (wnd->m_renderingAnimator) {
        ecore_animator_del(wnd->m_renderingAnimator);
    }

    m_needsRendering = true;
    wnd->m_renderingAnimator = ecore_animator_add(
        [](void* data) -> Eina_Bool {
            WindowImplEFL* wnd = (WindowImplEFL*)data;
            if (!wnd->m_canRendering) {
                return ECORE_CALLBACK_RENEW;
            }
            StarFishEnterer enter(wnd->starFish());
            if (wnd->rendering()) {
                wnd->m_canRendering = false;
            } else {
                wnd->m_canRendering = true;
            }
            wnd->m_renderingAnimator = nullptr;
            return ECORE_CALLBACK_CANCEL;
        },
        wnd);
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
            Canvas* c = Canvas::create(g_surfaceForScreehShot);
            c->setViewportWidthAndHeight(width(), height());
            return c;
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
    }

    webView()->clearStackingContext(false);

    m_objectList.clear();
    m_objectList.shrink_to_fit();
    m_surfaceList.clear();
    m_surfaceList.shrink_to_fit();
}
}
#endif
