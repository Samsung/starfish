/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#include "StarfishConfig.h"
#include "LWEWebView.h"

#if defined(PORT_WEBVIEW_BRIDGE_EFL)

#define STARFISH_ENABLE_PROFILE_TIMER

#include <Elementary.h>
#include <Ecore_Input.h>
#include <Ecore_Input_Evas.h>
#include <Ecore_IMF.h>
#include <Ecore_IMF_Evas.h>
#include <Evas_GL.h>

#ifdef STREAMLINE_PROFILE
#include "streamline_annotate.h"
#else
#define ANNOTATE_SETUP
#define ANNOTATE_CHANNEL_COLOR(channel, color, str)
#define ANNOTATE_CHANNEL_END(channel)
#define ANNOTATE_GREEN 0x00ff001b
#endif

#if defined(PORT_WINDOW_BACKEND_GL)
extern Evas_GL_API* g_evasGLAPI;
extern Evas_GL* g_evasGL;
extern bool g_isEvasGLOnDirectMode;
#endif

namespace LWE {

const int g_arrowKeyDownMinimumDelayInMS = 150;
static int g_arrowKeyDownTimestamp[4];

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

static bool isASCIIPrintableKey(char c)
{
    if (c >= 32 && c <= 126) {
        return true;
    }
    return false;
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
    } else if (strcmp("Tab", ecoreKeyString) == 0) {
        return KeyValue::TabKey;
    } else if (strcmp("BackSpace", ecoreKeyString) == 0) {
        return KeyValue::BackspaceKey;
    } else if (strcmp("Escape", ecoreKeyString) == 0) {
        return KeyValue::EscapeKey;
    } else if (strcmp("Delete", ecoreKeyString) == 0) {
        return KeyValue::DeleteKey;
    } else if (strcmp("at", ecoreKeyString) == 0) {
        return KeyValue::AtMarkKey;
    } else if (strcmp("minus", ecoreKeyString) == 0) {
        if (isShiftPressed) {
            return KeyValue::UnderScoreMarkKey;
        } else {
            return KeyValue::MinusMarkKey;
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
            return (KeyValue)(KeyValue::LowerAKey + ch - 'a');
        } else if (ch >= 'A' && ch <= 'Z') {
            return (KeyValue)(KeyValue::AKey + ch - 'A');
        }
    } else if (strcmp("XF86AudioRaiseVolume", ecoreKeyString) == 0) {
        return KeyValue::TVVolumeUpKey;
    } else if (strcmp("XF86AudioLowerVolume", ecoreKeyString) == 0) {
        return KeyValue::TVVolumeDownKey;
    } else if (strcmp("XF86AudioMute", ecoreKeyString) == 0) {
        return KeyValue::TVMuteKey;
    } else if (strcmp("XF86RaiseChannel", ecoreKeyString) == 0) {
        return KeyValue::TVChannelUpKey;
    } else if (strcmp("XF86LowerChannel", ecoreKeyString) == 0) {
        return KeyValue::TVChannelDownKey;
    } else if (strcmp("XF86AudioRewind", ecoreKeyString) == 0) {
        return KeyValue::MediaTrackPreviousKey;
    } else if (strcmp("XF86AudioNext", ecoreKeyString) == 0) {
        return KeyValue::MediaTrackNextKey;
    } else if (strcmp("XF86AudioPause", ecoreKeyString) == 0) {
        return KeyValue::MediaPauseKey;
    } else if (strcmp("XF86AudioRecord", ecoreKeyString) == 0) {
        return KeyValue::MediaRecordKey;
    } else if (strcmp("XF86AudioPlay", ecoreKeyString) == 0) {
        return KeyValue::MediaPlayKey;
    } else if (strcmp("XF86AudioStop", ecoreKeyString) == 0) {
        return KeyValue::MediaStopKey;
    } else if (strcmp("XF86Info", ecoreKeyString) == 0) {
        return KeyValue::TVInfoKey;
    } else if (strcmp("XF86Back", ecoreKeyString) == 0) {
        return KeyValue::TVReturnKey;
    } else if (strcmp("XF86Red", ecoreKeyString) == 0) {
        return KeyValue::TVRedKey;
    } else if (strcmp("XF86Green", ecoreKeyString) == 0) {
        return KeyValue::TVGreenKey;
    } else if (strcmp("XF86Yellow", ecoreKeyString) == 0) {
        return KeyValue::TVYellowKey;
    } else if (strcmp("XF86Blue", ecoreKeyString) == 0) {
        return KeyValue::TVBlueKey;
    } else if (strcmp("XF86SysMenu", ecoreKeyString) == 0) {
        return KeyValue::TVMenuKey;
    } else if (strcmp("XF86Home", ecoreKeyString) == 0) {
        return KeyValue::TVHomeKey;
    } else if (strcmp("XF86Exit", ecoreKeyString) == 0) {
        return KeyValue::TVExitKey;
    }

    STARFISH_LOG_ERROR("WebViewEFL - unimplemented key %s\n", ecoreKeyString);
    return KeyValue::UnidentifiedKey;
}

const uint32_t CLICK_REFRESH_DELAY = 400;

static void elm_box_layout_cb(Evas_Object* o, Evas_Object_Box_Data* priv,
                              void* user_data)
{
    int x, y, width, height;
    evas_object_geometry_get(o, &x, &y, &width, &height);

    Evas_Object_Box_Option* opt;
    Eina_List* l;
    for (l = priv->children,
        opt = (Evas_Object_Box_Option*)eina_list_data_get(l);
         l; l = eina_list_next(l),
        opt = (Evas_Object_Box_Option*)eina_list_data_get(l)) {
        evas_object_geometry_set(opt->obj, x, y, width, height);
    }
}

class WebViewEFL : public WebView {
public:
    WebViewEFL(void* winArg, unsigned x, unsigned y, unsigned width,
               unsigned height, float devicePixelRatio,
               const char* defaultFontName, const char* locale,
               const char* timezoneID)
        : WebView(nullptr)
        , m_resizeHandler(nullptr)
        , m_keyDownEventHandler(nullptr)
        , m_keyUpEventHandler(nullptr)
#if defined(PORT_WINDOW_BACKEND_GL)
        , m_glSync(nullptr)
#endif
        , m_lastMouseX(0)
        , m_lastMouseY(0)
        , m_isMouseLbuttonDown(false)
        , m_isKeyDown(false)
        , m_isDestroyed(false)
        , m_lastRenderingTime(0)
        , m_lastInputTime(0)
    {
        STARFISH_LOG_INFO("WebViewEFL::WebViewEFL\n");
        Evas_Object* win = (Evas_Object*)winArg;

        m_windowObject = win;

        m_windowDelEventHandler = [](void* data, Evas* e, Evas_Object* obj,
                                     void* event_info) {
            WebViewEFL* wv = (WebViewEFL*)data;
            wv->m_isDestroyed = true;
        };

        evas_object_event_callback_add(m_windowObject, EVAS_CALLBACK_DEL,
                                       m_windowDelEventHandler, this);

        m_nonIMEKeyEventBox = elm_label_add(win);
        evas_object_show(m_nonIMEKeyEventBox);

        m_mainBox = elm_box_add(win);
        evas_object_resize(m_mainBox, width, height);
        evas_object_move(m_mainBox, x, y);
        elm_box_layout_set(m_mainBox, elm_box_layout_cb, NULL, NULL);
        evas_object_show(m_mainBox);

        m_graphicsAdapter =
            evas_object_image_filled_add(evas_object_evas_get(win));
        evas_object_resize(m_graphicsAdapter, width, height);
        evas_object_move(m_graphicsAdapter, x, y);
        evas_object_image_size_set(m_graphicsAdapter, width, height);
        evas_object_image_alpha_set(m_graphicsAdapter, EINA_TRUE);

        elm_box_pack_end(m_mainBox, m_graphicsAdapter);

#if defined(PORT_WINDOW_BACKEND_GL)
        m_glEvasgl = evas_gl_new(evas_object_evas_get(win));
        m_glGlapi = evas_gl_api_get(m_glEvasgl);
        m_isEvasGLOnDirectMode = false;
        // Set a surface config
        m_glCfg = evas_gl_config_new();
        m_glCfg->color_format = EVAS_GL_RGBA_8888;
        m_glCfg->depth_bits = EVAS_GL_DEPTH_NONE;
        m_glCfg->stencil_bits = EVAS_GL_STENCIL_NONE;
        m_glCfg->multisample_bits = EVAS_GL_MULTISAMPLE_NONE;

// we need to set these secret flags reducing memory usage
// see platform/upstream/efl/src/modules/evas/engines/gl_common/evas_gl_core.c
// in tizen
// or ./src/modules/evas/engines/gl_common/evas_gl_core.c in efl git
#define EVAS_GL_OPTIONS_DIRECT_MEMORY_OPTIMIZE (1 << 12)
#define EVAS_GL_OPTIONS_DIRECT_OVERRIDE (1 << 13)
        m_isEvasGLOnDirectMode = true;
        m_glCfg->options_bits = (Evas_GL_Options_Bits)(
            EVAS_GL_OPTIONS_DIRECT | EVAS_GL_OPTIONS_DIRECT_OVERRIDE |
            EVAS_GL_OPTIONS_DIRECT_MEMORY_OPTIMIZE |
            EVAS_GL_OPTIONS_CLIENT_SIDE_ROTATION);
        STARFISH_LOG_INFO("try to use EvasGL direct mode\n");

        // Create a surface and context
        m_glSfc = evas_gl_surface_create(m_glEvasgl, m_glCfg, width, height);
        m_glCtx = evas_gl_context_version_create(
            m_glEvasgl, NULL, Evas_GL_Context_Version::EVAS_GL_GLES_3_X);

        if (m_glCtx == nullptr) {
            STARFISH_LOG_ERROR(
                "failed to create openGL 3.0 context... try to use 2.0 "
                "instead\n");
            m_glCtx = evas_gl_context_version_create(
                m_glEvasgl, NULL, Evas_GL_Context_Version::EVAS_GL_GLES_2_X);
        }
        if (m_glCtx == nullptr) {
            STARFISH_LOG_ERROR("failed to create openGL 2.0 context...\n");
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }

        Evas_Native_Surface ns;
        evas_gl_native_surface_get(m_glEvasgl, m_glSfc, &ns);
        evas_object_image_native_surface_set(m_graphicsAdapter, &ns);
        evas_object_show(m_graphicsAdapter);

        STARFISH_LOG_INFO("WebViewEFL::WebViewEFL::clearEvasGL\n");
        evas_gl_make_current(m_glEvasgl, m_glSfc, m_glCtx);
        m_glGlapi->glClearColor(0, 0, 0, 0);
        m_glGlapi->glClear(GL_COLOR_BUFFER_BIT);
        m_glGlapi->glFlush();

        m_windowShownHandler = [](void* data, Evas* e, Evas_Object* obj,
                                  void* event_info) {
            WebViewEFL* wv = (WebViewEFL*)data;
            STARFISH_LOG_INFO("WebViewEFL::windowShownCallback::clearEvasGL\n");

            evas_object_image_pixels_dirty_set(wv->m_graphicsAdapter,
                                               EINA_TRUE);
            evas_object_image_pixels_get_callback_set(
                wv->m_graphicsAdapter,
                [](void* data, Evas_Object* o) {
                    WebViewEFL* wv = (WebViewEFL*)data;
                    evas_gl_make_current(wv->m_glEvasgl, wv->m_glSfc,
                                         wv->m_glCtx);
                    wv->m_glGlapi->glClearColor(0, 0, 0, 0);
                    wv->m_glGlapi->glClear(GL_COLOR_BUFFER_BIT);
                    wv->m_glGlapi->glFlush();
                },
                wv);
        };
        evas_object_event_callback_add(m_windowObject, EVAS_CALLBACK_SHOW,
                                       m_windowShownHandler, this);
#else
        m_windowShownHandler = [](void* data, Evas* e, Evas_Object* obj,
                                  void* event_info) {
            WebViewEFL* wv = (WebViewEFL*)data;
        };
        evas_object_event_callback_add(m_windowObject, EVAS_CALLBACK_SHOW,
                                       m_windowShownHandler, this);

        evas_object_image_content_hint_set(m_graphicsAdapter,
                                           EVAS_IMAGE_CONTENT_HINT_DYNAMIC);
        evas_object_show(m_graphicsAdapter);
#endif
        m_isKeyDown = false;
        m_lastClickedTimestamp = 0;
        m_clickedCount = 0;
        m_imfContext = nullptr;
        m_lastKeyPressedTimestamp = 0;
        m_offsetYDueToSoftwareKeyboard = 0;

        m_mouseDownEventHandler = [](void* data, Evas* evas, Evas_Object* obj,
                                     void* event_info) -> void {
            WebViewEFL* webView = (WebViewEFL*)data;
            Evas_Event_Mouse_Down* ev = (Evas_Event_Mouse_Down*)event_info;
            // We care just left button now
            int currentPosX = ev->output.x;
            int currentPosY = ev->output.y;

            int x, y;
            evas_object_geometry_get(webView->m_graphicsAdapter, &x, &y, 0, 0);
            currentPosX -= x;
            currentPosY -= y;

            if (ev->button == 1 && (currentPosX >= 0 && currentPosY >= 0)) {
                if (ev->timestamp - webView->m_lastClickedTimestamp >
                    CLICK_REFRESH_DELAY) {
                    webView->m_clickedCount = 1;
                    webView->m_lastClickedTimestamp = ev->timestamp;
                } else {
                    webView->m_clickedCount++;
                }
                webView->FetchWebContainer()->DispatchMouseDownEvent(
                    MouseButtonValue::LeftButton,
                    MouseButtonsValue::LeftButtonDown, currentPosX,
                    currentPosY);
                webView->m_isMouseLbuttonDown = true;
            }

            webView->HideSoftwareKeyboardIfPossible();

            return;
        };
        evas_object_event_callback_add(m_graphicsAdapter,
                                       EVAS_CALLBACK_MOUSE_DOWN,
                                       m_mouseDownEventHandler, this);

        m_mouseUpEventHandler = [](void* data, Evas* evas, Evas_Object* obj,
                                   void* event_info) -> void {
            WebViewEFL* webView = (WebViewEFL*)data;
            Evas_Event_Mouse_Up* ev = (Evas_Event_Mouse_Up*)event_info;
            // We care just left button now
            int currentPosX = ev->output.x;
            int currentPosY = ev->output.y;
            int x, y;
            evas_object_geometry_get(webView->m_graphicsAdapter, &x, &y, 0, 0);
            currentPosX -= x;
            currentPosY -= y;

            if (ev->button == 1 && (currentPosX >= 0 && currentPosY >= 0)) {
                if (ev->timestamp - webView->m_lastClickedTimestamp >
                    CLICK_REFRESH_DELAY) {
                    webView->m_clickedCount = 1;
                    webView->m_lastClickedTimestamp = ev->timestamp;
                } else {
                    webView->m_clickedCount++;
                }
                webView->FetchWebContainer()->DispatchMouseUpEvent(
                    MouseButtonValue::NoButton, MouseButtonsValue::NoButtonDown,
                    currentPosX, currentPosY);
                webView->m_isMouseLbuttonDown = false;
            }
            return;
        };
        evas_object_event_callback_add(m_graphicsAdapter,
                                       EVAS_CALLBACK_MOUSE_UP,
                                       m_mouseUpEventHandler, this);

        m_mouseWheelEventHandler = [](void* data, Evas* evas, Evas_Object* obj,
                                      void* event_info) -> void {
            WebViewEFL* wv = (WebViewEFL*)data;
            Evas_Event_Mouse_Wheel* ev = (Evas_Event_Mouse_Wheel*)event_info;
            // We care just left button now
            int currentPosX = ev->output.x;
            int currentPosY = ev->output.y;
            int x, y;
            evas_object_geometry_get(wv->m_graphicsAdapter, &x, &y, 0, 0);
            currentPosX -= x;
            currentPosY -= y;
            wv->FetchWebContainer()->DispatchMouseWheelEvent(
                currentPosX, currentPosY, ev->z);
            return;
        };
        evas_object_event_callback_add(m_graphicsAdapter,
                                       EVAS_CALLBACK_MOUSE_WHEEL,
                                       m_mouseWheelEventHandler, this);

        m_mouseMoveEventHandler = [](void* data, Evas* evas, Evas_Object* obj,
                                     void* event_info) -> void {
            WebViewEFL* webView = (WebViewEFL*)data;
            Evas_Event_Mouse_Move* ev = (Evas_Event_Mouse_Move*)event_info;
            // We care just left button now
            int currentPosX = ev->cur.output.x;
            int currentPosY = ev->cur.output.y;
            int x, y;
            evas_object_geometry_get(webView->m_graphicsAdapter, &x, &y, 0, 0);
            currentPosX -= x;
            currentPosY -= y;
            unsigned char buttons = webView->m_isMouseLbuttonDown
                                        ? MouseButtonsValue::LeftButtonDown
                                        : 0;
            webView->FetchWebContainer()->DispatchMouseMoveEvent(
                MouseButtonValue::NoButton, (MouseButtonsValue)buttons,
                currentPosX, currentPosY);
            return;
        };
        evas_object_event_callback_add(m_graphicsAdapter,
                                       EVAS_CALLBACK_MOUSE_MOVE,
                                       m_mouseMoveEventHandler, this);

#if !defined(STARFISH_TIZEN_WEARABLE_WIDGET)
        m_keyDownEventHandler = [](void* data, Evas* evas, Evas_Object* obj,
                                   void* event_info) -> void {
            WebViewEFL* webView = (WebViewEFL*)data;
            Evas_Event_Key_Down* ev = (Evas_Event_Key_Down*)event_info;
            STARFISH_LOG_INFO(
                "EVAS_CALLBACK_KEY_DOWN for m_nonIMEKeyEventBox [%s,%d]\n",
                ev->key, (int)ev->keycode);
            if (evas_object_focus_get(webView->m_mainBox) == EINA_TRUE) {
                STARFISH_LOG_INFO(
                    "EVAS_CALLBACK_KEY_DOWN for m_nonIMEKeyEventBox but "
                    "m_mainBox has focus[%s]\n",
                    ev->key);
                return;
            }

            if (webView->m_lastInputTime == 0) {
                ANNOTATE_SETUP;
                ANNOTATE_CHANNEL_COLOR(3000, ANNOTATE_GREEN,
                                       "EVAS_CALLBACK_KEY_DOWN");
                webView->m_lastInputTime = Starfish::longTickCount();
                ANNOTATE_CHANNEL_END(3000);
            }

#ifdef STARFISH_TIZEN_TV
            if ((strncmp(ev->key, "XF86Red", 7) == 0)) {
                ev->key = "Tab";
            } else if ((strncmp(ev->key, "XF86Back", 8) == 0)) {
                ev->key = "Escape";
            }
#endif
            auto keyValue = ecoreEventKeyToKeyValue(
                ev->key, (evas_key_modifier_is_set(ev->modifiers, "Shift_L") ==
                          EINA_TRUE) ||
                             (evas_key_modifier_is_set(
                                  ev->modifiers, "Shift_R") == EINA_TRUE));

            if (keyValue >= ArrowDownKey && keyValue <= ArrowRightKey) {
                unsigned int currentTimestamp = ev->timestamp;
                if (currentTimestamp -
                        g_arrowKeyDownTimestamp[keyValue - ArrowDownKey] <
                    g_arrowKeyDownMinimumDelayInMS) {
                    return;
                }
                g_arrowKeyDownTimestamp[keyValue - ArrowDownKey] =
                    currentTimestamp;
            }

            webView->FetchWebContainer()->DispatchKeyDownEvent(keyValue);
            webView->FetchWebContainer()->DispatchKeyPressEvent(keyValue);
            webView->m_isKeyDown = true;
        };
        evas_object_event_callback_add(m_nonIMEKeyEventBox,
                                       EVAS_CALLBACK_KEY_DOWN,
                                       m_keyDownEventHandler, this);

        m_keyUpEventHandler = [](void* data, Evas* evas, Evas_Object* obj,
                                 void* event_info) -> void {
            WebViewEFL* webView = (WebViewEFL*)data;
            Evas_Event_Key_Up* ev = (Evas_Event_Key_Up*)event_info;
            if (evas_object_focus_get(webView->m_mainBox) == EINA_TRUE) {
                return;
            }

#ifdef STARFISH_TIZEN_TV
            if ((strncmp(ev->key, "XF86Red", 7) == 0)) {
                ev->key = "Tab";
            } else if ((strncmp(ev->key, "XF86Back", 8) == 0)) {
                ev->key = "Escape";
            }
#endif
            auto keyValue = ecoreEventKeyToKeyValue(
                ev->key, (evas_key_modifier_is_set(ev->modifiers, "Shift_L") ==
                          EINA_TRUE) ||
                             (evas_key_modifier_is_set(
                                  ev->modifiers, "Shift_R") == EINA_TRUE));

            if (keyValue >= ArrowDownKey && keyValue <= ArrowRightKey) {
                g_arrowKeyDownTimestamp[keyValue - ArrowDownKey] = 0;
            }

            webView->FetchWebContainer()->DispatchKeyUpEvent(keyValue);
            webView->m_isKeyDown = false;
            return;
        };
        evas_object_event_callback_add(m_nonIMEKeyEventBox,
                                       EVAS_CALLBACK_KEY_UP,
                                       m_keyUpEventHandler, this);

        m_resizeHandler = [](void* data, Evas* e, Evas_Object* obj,
                             void* event_info) {
            WebViewEFL* wv = (WebViewEFL*)data;
            int w, h;
            evas_object_geometry_get(wv->m_mainBox, NULL, NULL, &w, &h);
            if (w == 0 || h == 0) {
                STARFISH_LOG_WARN("the main box has a zero size\n");
                return;
            }
            evas_object_resize(wv->m_graphicsAdapter, w, h);

#if defined(PORT_WINDOW_BACKEND_GL)
            g_evasGL = nullptr;
            g_evasGLAPI = nullptr;
            evas_object_image_native_surface_set(wv->m_graphicsAdapter, NULL);
            evas_gl_surface_destroy(wv->m_glEvasgl, wv->m_glSfc);
            evas_object_image_size_set(wv->m_graphicsAdapter, w, h);
            Evas_Native_Surface ns;
            wv->m_glSfc =
                evas_gl_surface_create(wv->m_glEvasgl, wv->m_glCfg, w, h);
            evas_gl_native_surface_get(wv->m_glEvasgl, wv->m_glSfc, &ns);
            evas_object_image_native_surface_set(wv->m_graphicsAdapter, &ns);

            STARFISH_LOG_INFO("WebViewEFL::resizeCallback::clearEvasGL %d %d\n",
                              w, h);

            evas_object_image_pixels_dirty_set(wv->m_graphicsAdapter,
                                               EINA_TRUE);
            evas_object_image_pixels_get_callback_set(
                wv->m_graphicsAdapter,
                [](void* data, Evas_Object* o) {
                    WebViewEFL* wv = (WebViewEFL*)data;
                    evas_gl_make_current(wv->m_glEvasgl, wv->m_glSfc,
                                         wv->m_glCtx);
                    wv->m_glGlapi->glClearColor(0, 0, 0, 0);
                    wv->m_glGlapi->glClear(GL_COLOR_BUFFER_BIT);
                    wv->m_glGlapi->glFlush();
                },
                wv);
#else
            evas_object_image_size_set(wv->m_graphicsAdapter, w, h);
#endif
            wv->FetchWebContainer()->ResizeTo(w, h);
        };
        evas_object_event_callback_add(m_mainBox, EVAS_CALLBACK_RESIZE,
                                       m_resizeHandler, this);

        evas_object_event_callback_add(
            m_mainBox, EVAS_CALLBACK_MOVE,
            [](void* data, Evas* e, Evas_Object* obj, void* event_info) {
                WebViewEFL* wv = (WebViewEFL*)data;
                int x, y;
                evas_object_geometry_get(wv->m_mainBox, &x, &y, NULL, NULL);
                evas_object_move(wv->m_graphicsAdapter, x, y);
            },
            this);

        ecore_imf_init();
        // Register IMF callbacks
        if (ecore_imf_context_default_id_get()) {
            m_imfContext =
                ecore_imf_context_add(ecore_imf_context_default_id_get());
        } else {
            STARFISH_LOG_ERROR(
                "ecore_imf_context_default_id_get returns null.. use fallback "
                "method\n");
            m_imfContext = ecore_imf_context_add(getImfMethod());
        }

        ecore_imf_context_client_window_set(
            m_imfContext,
            (void*)ecore_evas_window_get(ecore_evas_ecore_evas_get(
                evas_object_evas_get(m_graphicsAdapter))));
        ecore_imf_context_client_canvas_set(
            m_imfContext, evas_object_evas_get(m_graphicsAdapter));

        ecore_imf_context_retrieve_surrounding_callback_set(
            m_imfContext,
            [](void* data, Ecore_IMF_Context* ctx, char** text,
               int* cursor_pos) -> Eina_Bool {
                // This callback will be called when the Input Method Context
                // module
                // requests the surrounding context.
                if (text)
                    *text = strdup("");
                if (cursor_pos)
                    *cursor_pos = 0;
                return EINA_TRUE;
            },
            this);

        // register commit event callback
        ecore_imf_context_event_callback_add(
            m_imfContext, ECORE_IMF_CALLBACK_COMMIT,
            [](void* data, Ecore_IMF_Context* ctx, void* event_info) {
                WebViewEFL* self = (WebViewEFL*)data;
                char* commit_str = (char*)event_info;
                STARFISH_LOG_INFO("ECORE_IMF_CALLBACK_COMMIT %s\n", commit_str);
                self->FetchWebContainer()->DispatchCompositionEndEvent(
                    commit_str);
            },
            this);

        // register preedit changed event handler
        ecore_imf_context_event_callback_add(
            m_imfContext, ECORE_IMF_CALLBACK_PREEDIT_CHANGED,
            [](void* data, Ecore_IMF_Context* ctx, void* event_info) {
                WebViewEFL* self = (WebViewEFL*)data;
                char* str = NULL;
                int cursor_pos;
                ecore_imf_context_preedit_string_get(self->m_imfContext, &str,
                                                     &cursor_pos);
                STARFISH_LOG_INFO("ECORE_IMF_CALLBACK_PREEDIT_CHANGED %s %d\n",
                                  str, cursor_pos);
                if (str) {
                    self->FetchWebContainer()->DispatchCompositionUpdateEvent(
                        str);
                    free(str);
                }
            },
            this);

        // register key event handler
        evas_object_event_callback_add(
            m_mainBox, EVAS_CALLBACK_KEY_DOWN,
            [](void* data, Evas* e, Evas_Object* obj, void* event_info) {
                WebViewEFL* wv = (WebViewEFL*)data;
                Evas_Event_Key_Down* ev = (Evas_Event_Key_Down*)event_info;
                STARFISH_LOG_INFO(
                    "EVAS_CALLBACK_KEY_DOWN for ime object [%s]\n", ev->key);

#ifdef STARFISH_TIZEN_TV
                if ((strcmp(ev->key, "XF86Red") == 0)) {
                    ev->key = "Tab";
                }
#endif

                bool tryFilter = true;
                if ((strcmp(ev->key, "Tab") == 0)) {
                    tryFilter = false;
                }

                if ((strcmp(ev->key, "XF86Exit") == 0) ||
                    (strcmp(ev->key, "Select") == 0) ||
                    (strcmp(ev->key, "Cancel") == 0)) {
                    if (strcmp(ev->key, "Select") == 0) {
                        wv->FetchWebContainer()->AddIdleCallback(
                            [](void* data) {
                                WebViewEFL* self = (WebViewEFL*)data;
                                KeyValue kv = KeyValue::EnterKey;
                                self->FetchWebContainer()->DispatchKeyDownEvent(
                                    kv);
                                self->FetchWebContainer()
                                    ->DispatchKeyPressEvent(kv);
                                self->FetchWebContainer()->DispatchKeyUpEvent(
                                    kv);
                                self->HideSoftwareKeyboardIfPossible();
                            },
                            wv);
                    } else {
                        wv->FetchWebContainer()->AddIdleCallback(
                            [](void* data) {
                                WebViewEFL* self = (WebViewEFL*)data;
                                self->HideSoftwareKeyboardIfPossible();
                            },
                            wv);
                    }
                }

                // process non-char keys
                STARFISH_LOG_INFO("process non-char [%s]\n", ev->key);
                auto keyValue = ecoreEventKeyToKeyValue(
                    ev->key,
                    (evas_key_modifier_is_set(ev->modifiers, "Shift_L") ==
                     EINA_TRUE) ||
                        (evas_key_modifier_is_set(ev->modifiers, "Shift_R") ==
                         EINA_TRUE));
                if ((strcmp(ev->key, "Up") != 0) &&
                    (strcmp(ev->key, "Down") != 0)) {
                    wv->FetchWebContainer()->DispatchKeyDownEvent(keyValue);
                    wv->FetchWebContainer()->DispatchKeyPressEvent(keyValue);
                    wv->m_isKeyDown = true;
                }

                if (tryFilter && !isASCIIPrintableKey(keyValue)) {
                    Ecore_IMF_Event_Key_Down ecore_ev;
                    ecore_imf_evas_event_key_down_wrap(ev, &ecore_ev);
                    ecore_imf_context_filter_event(wv->m_imfContext,
                                                   ECORE_IMF_EVENT_KEY_DOWN,
                                                   (Ecore_IMF_Event*)&ecore_ev);
                }
            },
            this);
        evas_object_event_callback_add(
            m_mainBox, EVAS_CALLBACK_KEY_UP,
            [](void* data, Evas* e, Evas_Object* obj, void* event_info) {
                WebViewEFL* wv = (WebViewEFL*)data;
                Evas_Event_Key_Up* ev = (Evas_Event_Key_Up*)event_info;
                STARFISH_LOG_INFO("EVAS_CALLBACK_KEY_UP for ime object [%s]\n",
                                  ev->key);

#ifdef STARFISH_TIZEN_TV
                if ((strcmp(ev->key, "XF86Red") == 0)) {
                    ev->key = "Tab";
                }
#endif

                bool tryFilter = true;

                if ((strcmp(ev->key, "Tab") == 0)) {
                    tryFilter = false;
                }

                if (tryFilter) {
                    Ecore_IMF_Event_Key_Up ecore_ev;
                    ecore_imf_evas_event_key_up_wrap(ev, &ecore_ev);
                    if (ecore_imf_context_filter_event(
                            wv->m_imfContext, ECORE_IMF_EVENT_KEY_UP,
                            (Ecore_IMF_Event*)&ecore_ev)) {
                        return;
                    }
                }

                if ((strcmp(ev->key, "Up") != 0) &&
                    (strcmp(ev->key, "Down") != 0)) {
                    // process non-char keys
                    auto keyValue = ecoreEventKeyToKeyValue(
                        ev->key,
                        (evas_key_modifier_is_set(ev->modifiers, "Shift_L") ==
                         EINA_TRUE) ||
                            (evas_key_modifier_is_set(ev->modifiers,
                                                      "Shift_R") == EINA_TRUE));
                    wv->FetchWebContainer()->DispatchKeyUpEvent(keyValue);
                    wv->m_isKeyDown = false;
                }
            },
            this);

        evas_object_event_callback_add(
            m_mainBox, EVAS_CALLBACK_FOCUS_IN,
            [](void* data, Evas* e, Evas_Object* obj, void* event_info) {
                WebViewEFL* wv = (WebViewEFL*)data;
                Ecore_IMF_Context* ctx = wv->m_imfContext;
                Ecore_IMF_Event_Key_Down ev;
                ecore_imf_evas_event_key_down_wrap(
                    (Evas_Event_Key_Down*)event_info, &ev);
                ecore_imf_context_reset(ctx);
                ecore_imf_context_focus_in(ctx);
                ecore_imf_context_show(ctx);
            },
            this);

        evas_object_event_callback_add(
            m_mainBox, EVAS_CALLBACK_FOCUS_OUT,
            [](void* data, Evas* e, Evas_Object* obj, void* event_info) {
                WebViewEFL* wv = (WebViewEFL*)data;
                Ecore_IMF_Context* ctx = wv->m_imfContext;
                Ecore_IMF_Event_Key_Down ev;

                if (ecore_imf_context_input_panel_state_get(ctx) ==
                    ECORE_IMF_INPUT_PANEL_STATE_SHOW) {
                    ecore_imf_evas_event_key_down_wrap(
                        (Evas_Event_Key_Down*)event_info, &ev);
                    // ecore_imf_context_reset(ctx);
                    ecore_imf_context_focus_out(ctx);
                    ecore_imf_context_hide(ctx);
                }
            },
            this);

        ecore_imf_context_autocapital_type_set(m_imfContext,
                                               ECORE_IMF_AUTOCAPITAL_TYPE_NONE);
        ecore_imf_context_prediction_allow_set(m_imfContext, EINA_FALSE);

#endif

#if defined(PORT_WINDOW_BACKEND_GL)
        ::LWE::WebContainer* webContainer = ::LWE::WebContainer::CreateGL(
            width, height,
            [this](WebContainer* wc) {
                if (g_evasGL != m_glEvasgl) {
                    evas_gl_make_current(m_glEvasgl, m_glSfc, m_glCtx);
                    g_evasGL = m_glEvasgl;
                    g_evasGLAPI = m_glGlapi;
                    g_isEvasGLOnDirectMode = m_isEvasGLOnDirectMode;
                }
                if (m_glSync) {
                    Starfish::LongTaskFinder t("evasglWaitSync");
                    g_evasGLAPI->evasglClientWaitSync(
                        m_glEvasgl, m_glSync,
                        EVAS_GL_SYNC_PRIOR_COMMANDS_COMPLETE, EVAS_GL_FOREVER);
                    g_evasGLAPI->evasglDestroySync(m_glEvasgl, m_glSync);
                    m_glSync = nullptr;
                }
            },
            [this](WebContainer* wc, bool mayNeedsSync) {

                if (mayNeedsSync && g_evasGLAPI->evasglCreateSync &&
                    !m_glSync) {
                    int attr[] = { EVAS_GL_NONE };
                    m_glSync = g_evasGLAPI->evasglCreateSync(
                        m_glEvasgl, EVAS_GL_SYNC_FENCE, attr);
                }
                if (m_lastInputTime) {
                    ANNOTATE_SETUP;
                    ANNOTATE_CHANNEL_COLOR(3002, ANNOTATE_GREEN,
                                           "response time");
#ifdef STARFISH_ENABLE_PROFILE_TIMER
                    uint64_t end = Starfish::longTickCount();
                    float time = (float)((end - m_lastInputTime) / 1000.f);
                    STARFISH_LOG_INFO("response time is %f ms\n", time);
#endif
                    m_lastInputTime = 0;
                    ANNOTATE_CHANNEL_END(3002);
                }
            },
            devicePixelRatio, defaultFontName, locale, timezoneID);

        webContainer->RegisterSetNeedsRenderingCallback([this](
            ::LWE::WebContainer* wc,
            const std::function<void()>& doRenderingFunction) {
            evas_object_image_pixels_dirty_set(m_graphicsAdapter, EINA_TRUE);
            evas_object_image_pixels_get_callback_set(
                m_graphicsAdapter,
                [](void* data, Evas_Object* o) {
                    // We need to draw every time for preventing screen blinking
                    WebViewEFL* s = (WebViewEFL*)data;
                    if (s->m_lastDoRenderingFunction && !s->m_isDestroyed) {
                        s->m_lastDoRenderingFunction();
                    }
                },
                this);

            m_lastDoRenderingFunction = doRenderingFunction;
        });

        evas_object_image_pixels_get_callback_set(
            m_graphicsAdapter,
            [](void* data, Evas_Object* o) {
                // We need to draw every time for preventing screen blinking
                WebViewEFL* s = (WebViewEFL*)data;
                if (s->m_lastDoRenderingFunction && !s->m_isDestroyed) {
                    s->m_lastDoRenderingFunction();
                }
            },
            this);

#else
        ::LWE::WebContainer* webContainer =
            ::LWE::WebContainer::Create(width, height, devicePixelRatio,
                                        defaultFontName, locale, timezoneID);
        webContainer->RegisterPreRenderingHandler(
            [this]() -> ::LWE::WebContainer::RenderInfo {
                int width, height;
                evas_object_image_size_get(m_graphicsAdapter, &width, &height);
                auto buf =
                    evas_object_image_data_get(m_graphicsAdapter, EINA_TRUE);
                evas_object_image_data_set(m_graphicsAdapter, buf);

                ::LWE::WebContainer::RenderInfo result;
                result.updatedBufferAddress = buf;
                result.bufferStride =
                    evas_object_image_stride_get(m_graphicsAdapter);

                return result;

            });
        webContainer->RegisterOnRenderedHandler([this](
            ::LWE::WebContainer* c, ::LWE::WebContainer::RenderResult r) {
            evas_object_image_data_update_add(m_graphicsAdapter, r.updatedX,
                                              r.updatedY, r.updatedWidth,
                                              r.updatedHeight);
        });
#endif
        webContainer->RegisterOnShowSoftwareKeyboardIfPossibleHandler(
            [this](WebContainer*) { ShowSoftwareKeyboardIfPossible(); });

        webContainer->RegisterOnHideSoftwareKeyboardIfPossibleHandler(
            [this](WebContainer* t) { HideSoftwareKeyboardIfPossible(); });

        m_hideKeyboardTimeoutId = m_keyboardTimeoutId = SIZE_MAX;
        m_impl = webContainer;

        webContainer->SetUserData(
            "__internalLWEWebViewEFLNativeWindowEvasObject", win);

#if defined(STARFISH_TIZEN_MAJOR_VERSION) && STARFISH_TIZEN_MAJOR_VERSION >= 5
        webContainer->SetUserData(
            "__internalLWEWebViewEFLEcoreWaylandHandle",
            ecore_evas_wayland2_window_get(
                ecore_evas_ecore_evas_get(evas_object_evas_get(win))));
#elif defined(STARFISH_TIZEN_MAJOR_VERSION) && STARFISH_TIZEN_MAJOR_VERSION == 4
        webContainer->SetUserData(
            "__internalLWEWebViewEFLEcoreWaylandHandle",
            ecore_evas_wayland_window_get(
                ecore_evas_ecore_evas_get(evas_object_evas_get(win))));
#endif
    }

    virtual void Destroy() override
    {
        m_isDestroyed = true;

        Blur();

        FetchWebContainer()->Destroy();

        if (m_imfContext) {
            ecore_imf_context_del(m_imfContext);
        }

        ecore_imf_shutdown();

        evas_object_hide(m_graphicsAdapter);
#if defined(PORT_WINDOW_BACKEND_GL)
        if (m_glSync) {
            g_evasGLAPI->evasglDestroySync(m_glEvasgl, m_glSync);
        }
        evas_object_image_native_surface_set(m_graphicsAdapter, NULL);
        evas_gl_surface_destroy(m_glEvasgl, m_glSfc);
        evas_gl_context_destroy(m_glEvasgl, m_glCtx);
        evas_gl_config_free(m_glCfg);
        evas_gl_free(m_glEvasgl);
#endif

        if (m_resizeHandler) {
            evas_object_event_callback_del(m_mainBox, EVAS_CALLBACK_RESIZE,
                                           m_resizeHandler);
        }

        evas_object_event_callback_del(m_graphicsAdapter,
                                       EVAS_CALLBACK_MOUSE_DOWN,
                                       m_mouseDownEventHandler);
        evas_object_event_callback_del(
            m_graphicsAdapter, EVAS_CALLBACK_MOUSE_UP, m_mouseUpEventHandler);
        evas_object_event_callback_del(m_graphicsAdapter,
                                       EVAS_CALLBACK_MOUSE_WHEEL,
                                       m_mouseWheelEventHandler);
        evas_object_event_callback_del(m_graphicsAdapter,
                                       EVAS_CALLBACK_MOUSE_MOVE,
                                       m_mouseMoveEventHandler);
        evas_object_event_callback_del(m_windowObject, EVAS_CALLBACK_SHOW,
                                       m_windowShownHandler);
        evas_object_event_callback_del(
            m_nonIMEKeyEventBox, EVAS_CALLBACK_KEY_DOWN, m_keyDownEventHandler);
        evas_object_event_callback_del(
            m_nonIMEKeyEventBox, EVAS_CALLBACK_KEY_UP, m_keyUpEventHandler);

        evas_object_event_callback_del(m_windowObject, EVAS_CALLBACK_DEL,
                                       m_windowDelEventHandler);

        if (m_graphicsAdapter) {
            evas_object_del(m_graphicsAdapter);
            m_graphicsAdapter = nullptr;
        }

        if (m_nonIMEKeyEventBox) {
            evas_object_del(m_nonIMEKeyEventBox);
            m_nonIMEKeyEventBox = nullptr;
        }

        if (m_mainBox) {
            evas_object_del(m_mainBox);
            m_mainBox = nullptr;
        }

        delete this;
    }

    virtual void* Unwrap() override
    {
        return m_mainBox;
    }

    virtual void Focus() override
    {
        WebView::Focus();

        evas_object_focus_set(m_mainBox, EINA_FALSE);
        evas_object_focus_set(m_nonIMEKeyEventBox, EINA_TRUE);
    }

    virtual void Blur() override
    {
        WebView::Blur();

        evas_object_focus_set(m_mainBox, EINA_FALSE);
        evas_object_focus_set(m_nonIMEKeyEventBox, EINA_FALSE);
    }

    void ShowSoftwareKeyboardIfPossible()
    {
#if !defined(STARFISH_TIZEN_WEARABLE_WIDGET)
        if (ecore_imf_input_panel_hide() == EINA_FALSE) {
            FetchWebContainer()->AddIdleCallback(
                [](void* data) {
                    WebViewEFL* self = (WebViewEFL*)data;
                    evas_object_focus_set(self->m_nonIMEKeyEventBox,
                                          EINA_FALSE);
                    evas_object_focus_set(self->m_mainBox, EINA_TRUE);
                },
                this);
            FetchWebContainer()->ClearTimeout(m_keyboardTimeoutId);
            m_keyboardTimeoutId = SIZE_MAX;
        } else {
            m_keyboardTimeoutId = FetchWebContainer()->AddTimeout(
                [](void* data) {
                    WebViewEFL* self = (WebViewEFL*)data;
                    self->ShowSoftwareKeyboardIfPossible();
                    self->m_keyboardTimeoutId = SIZE_MAX;
                },
                this, 100);
        }
#endif
    }

    void HideSoftwareKeyboardIfPossible()
    {
#if !defined(STARFISH_TIZEN_WEARABLE_WIDGET)
        FetchWebContainer()->ClearTimeout(m_hideKeyboardTimeoutId);
        m_hideKeyboardTimeoutId = FetchWebContainer()->AddTimeout(
            [](void* data) {
                WebViewEFL* self = (WebViewEFL*)data;
                evas_object_focus_set(self->m_mainBox, EINA_FALSE);
                evas_object_focus_set(self->m_nonIMEKeyEventBox, EINA_TRUE);
                self->m_hideKeyboardTimeoutId = SIZE_MAX;
            },
            this, 100);
#endif
    }

protected:
    virtual ::LWE::WebContainer* FetchWebContainer() override
    {
        return (::LWE::WebContainer*)m_impl;
    }

    Evas_Object* m_nonIMEKeyEventBox;

    void (*m_resizeHandler)(void* data, Evas* evas, Evas_Object* obj,
                            void* event_info);
    void (*m_mouseDownEventHandler)(void* data, Evas* evas, Evas_Object* obj,
                                    void* event_info);
    void (*m_mouseMoveEventHandler)(void* data, Evas* evas, Evas_Object* obj,
                                    void* event_info);
    void (*m_mouseUpEventHandler)(void* data, Evas* evas, Evas_Object* obj,
                                  void* event_info);
    void (*m_mouseWheelEventHandler)(void* data, Evas* evas, Evas_Object* obj,
                                     void* event_info);
    void (*m_keyDownEventHandler)(void* data, Evas* evas, Evas_Object* obj,
                                  void* event_info);
    void (*m_keyUpEventHandler)(void* data, Evas* evas, Evas_Object* obj,
                                void* event_info);
    void (*m_windowDelEventHandler)(void* data, Evas* evas, Evas_Object* obj,
                                    void* event_info);
    void (*m_windowShownHandler)(void* data, Evas* evas, Evas_Object* obj,
                                 void* event_info);

    Evas_Object* m_windowObject;
    Evas_Object* m_mainBox;
    Evas_Object* m_graphicsAdapter;
#if defined(PORT_WINDOW_BACKEND_GL)
    Evas_GL_Context* m_glCtx;
    Evas_GL_Surface* m_glSfc;
    Evas_GL_Config* m_glCfg;
    Evas_GL* m_glEvasgl;
    Evas_GL_API* m_glGlapi;
    EvasGLSync m_glSync;
    bool m_isEvasGLOnDirectMode;
#endif

    Ecore_IMF_Context* m_imfContext;

    float m_lastMouseX, m_lastMouseY;
    bool m_isMouseLbuttonDown;
    bool m_isKeyDown;
    bool m_isDestroyed;
    uint32_t m_lastClickedTimestamp;
    uint32_t m_clickedCount;
    uint32_t m_lastKeyPressedTimestamp;
    uint64_t m_lastRenderingTime;
    uint64_t m_lastInputTime;
    int m_offsetYDueToSoftwareKeyboard;
    size_t m_keyboardTimeoutId;
    size_t m_hideKeyboardTimeoutId;

    std::function<void()> m_lastDoRenderingFunction;
};

WebView* WebView::Create(void* win, unsigned x, unsigned y, unsigned width,
                         unsigned height, float devicePixelRatio,
                         const char* defaultFontName, const char* locale,
                         const char* timezoneID)
{
    return new WebViewEFL(win, x, y, width, height, devicePixelRatio,
                          defaultFontName, locale, timezoneID);
}
}

#endif
