/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#include "Starfish.h"

#if defined(PORT_WEBVIEW_BRIDGE_ECORE_WAYLAND2)
#define STARFISH_ENABLE_PROFILE_TIMER

#define EFL_BETA_API_SUPPORT
#include <Ecore_Wl2.h>
#undef EFL_BETA_API_SUPPORT
#include <Ecore_Input.h>
#include <wayland-egl-core.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <Ecore_Evas.h>
#include <Ecore_IMF.h>

#ifdef STREAMLINE_PROFILE
#include "streamline_annotate.h"
#else
#define ANNOTATE_SETUP
#define ANNOTATE_CHANNEL_COLOR(channel, color, str)
#define ANNOTATE_CHANNEL_END(channel)
#define ANNOTATE_GREEN 0x00ff001b
#endif

typedef EGLSyncKHR(EGLAPIENTRYP PFNEGLCREATESYNCKHRPROC)(
    EGLDisplay dpy, EGLenum type, const EGLint* attrib_list);
typedef EGLBoolean(EGLAPIENTRYP PFNEGLDESTROYSYNCKHRPROC)(EGLDisplay dpy,
                                                          EGLSyncKHR sync);
typedef EGLint(EGLAPIENTRYP PFNEGLCLIENTWAITSYNCKHRPROC)(EGLDisplay dpy,
                                                         EGLSyncKHR sync,
                                                         EGLint flags,
                                                         EGLTimeKHR timeout);

static PFNEGLCREATESYNCKHRPROC g_eglCreateSyncKHRProc;
static PFNEGLDESTROYSYNCKHRPROC g_eglDestroySyncKHRProc;
static PFNEGLCLIENTWAITSYNCKHRPROC g_eglClientWaitSyncKHRProc;

const int g_arrowKeyDownMinimumDelayInMS = 150;
static int g_arrowKeyDownTimestamp[4];

namespace LWE {

static const char* getIMFMethod()
{
    Eina_List* modules;

    modules = ecore_imf_context_available_ids_get();
    if (!modules) {
        return nullptr;
    }

    void* module;
    EINA_LIST_FREE(modules, module)
    {
        return (const char*)module;
    }

    return nullptr;
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
    } else if (strcmp("XF86PreviousChannel", ecoreKeyString) == 0) {
        return KeyValue::TVPreviousChannel;
    } else if (strcmp("XF86ChannelList", ecoreKeyString) == 0) {
        return KeyValue::TVChannelList;
    } else if (strcmp("XF86ChannelGuide", ecoreKeyString) == 0) {
        return KeyValue::TVChannelGuide;
    } else if (strcmp("XF86SimpleMenu", ecoreKeyString) == 0) {
        return KeyValue::TVSimpleMenu;
    } else if (strcmp("XF86EManual", ecoreKeyString) == 0) {
        return KeyValue::TVEManual;
    } else if (strcmp("XF86ExtraApp", ecoreKeyString) == 0) {
        return KeyValue::TVExtraApp;
    } else if (strcmp("XF86Search", ecoreKeyString) == 0) {
        return KeyValue::TVSearch;
    } else if (strcmp("XF86PictureSize", ecoreKeyString) == 0) {
        return KeyValue::TVPictureSize;
    } else if (strcmp("XF86Sleep", ecoreKeyString) == 0) {
        return KeyValue::TVSleep;
    } else if (strcmp("XF86Caption", ecoreKeyString) == 0) {
        return KeyValue::TVCaption;
    } else if (strcmp("XF86More", ecoreKeyString) == 0) {
        return KeyValue::TVMore;
    } else if (strcmp("XF86BTVoice", ecoreKeyString) == 0) {
        return KeyValue::TVBTVoice;
    } else if (strcmp("XF86Color", ecoreKeyString) == 0) {
        return KeyValue::TVColor;
    } else if (strcmp("XF86PlayBack", ecoreKeyString) == 0) {
        return KeyValue::TVPlayBack;
    }

    STARFISH_LOG_ERROR("WebViewEFL - unimplemented key %s", ecoreKeyString);
    return KeyValue::UnidentifiedKey;
}

class WebViewEcoreWayland2 : public WebView {
public:
    WebViewEcoreWayland2(void* winArg, unsigned x, unsigned y, unsigned width,
                         unsigned height, float devicePixelRatio,
                         const char* defaultFontName, const char* locale,
                         const char* timezoneID)
        : WebView(nullptr)
        , m_isMouseLbuttonDown(false)
        , m_isBufferSwapped(false)
        , m_hasFocus(true)
        , m_isShowing(false)
        , m_lastInputTime(0)
        , m_IMFContext(nullptr)
    {
        Ecore_Wl2_Window* win = (Ecore_Wl2_Window*)winArg;
        mEcoreWindow = win;

        g_eglCreateSyncKHRProc = reinterpret_cast<PFNEGLCREATESYNCKHRPROC>(
            eglGetProcAddress("eglCreateSyncKHR"));
        g_eglDestroySyncKHRProc = reinterpret_cast<PFNEGLDESTROYSYNCKHRPROC>(
            eglGetProcAddress("eglDestroySyncKHR"));
        g_eglClientWaitSyncKHRProc =
            reinterpret_cast<PFNEGLCLIENTWAITSYNCKHRPROC>(
                eglGetProcAddress("eglClientWaitSyncKHR"));

        Ecore_Wl2_Display* ecoreWlDisplay = ecore_wl2_window_display_get(win);
        auto display = ecore_wl2_display_get(ecoreWlDisplay);
        mWlDisplay = display;
        auto wlSurface = ecore_wl2_window_surface_get(win);

        STARFISH_LOG_INFO("wl_display %p surface %p", display, wlSurface);

        EGLint major, minor, count, n, size;
        EGLConfig* configs;
        int i;
        EGLint config_attribs[] = { EGL_SURFACE_TYPE,
                                    EGL_WINDOW_BIT,
                                    EGL_RED_SIZE,
                                    8,
                                    EGL_GREEN_SIZE,
                                    8,
                                    EGL_BLUE_SIZE,
                                    8,
                                    EGL_ALPHA_SIZE,
                                    8,
                                    EGL_DEPTH_SIZE,
                                    0,
                                    EGL_STENCIL_SIZE,
                                    0,
                                    EGL_RENDERABLE_TYPE,
                                    EGL_OPENGL_ES2_BIT,
                                    EGL_NONE };

        mDisplay = eglGetDisplay((EGLNativeDisplayType)display);
        if (mDisplay == EGL_NO_DISPLAY) {
            STARFISH_LOG_INFO("Can't create egl display");
            exit(1);
        } else {
            STARFISH_LOG_INFO("Created egl display");
        }

        if (eglInitialize(mDisplay, &major, &minor) != EGL_TRUE) {
            STARFISH_LOG_INFO("Can't initialise egl display");
            exit(1);
        }
        STARFISH_LOG_INFO("EGL major: %d, minor %d", major, minor);

        eglGetConfigs(mDisplay, NULL, 0, &count);
        STARFISH_LOG_INFO("EGL has %d configs", count);

        configs = ALLOCA(count * sizeof(*configs), void*);

        eglChooseConfig(mDisplay, config_attribs, configs, count, &n);

        EGLConfig eglConf = configs[0];
        for (i = 0; i < n; i++) {
            eglGetConfigAttrib(mDisplay, configs[i], EGL_BUFFER_SIZE, &size);
            STARFISH_LOG_INFO("Buffer size for config %d is %d", i, size);
            eglGetConfigAttrib(mDisplay, configs[i], EGL_RED_SIZE, &size);
            STARFISH_LOG_INFO("Red size for config %d is %d", i, size);
            eglGetConfigAttrib(mDisplay, configs[i], EGL_GREEN_SIZE, &size);
            STARFISH_LOG_INFO("Green size for config %d is %d", i, size);
            eglGetConfigAttrib(mDisplay, configs[i], EGL_BLUE_SIZE, &size);
            STARFISH_LOG_INFO("Blue size for config %d is %d", i, size);
            eglGetConfigAttrib(mDisplay, configs[i], EGL_ALPHA_SIZE, &size);
            STARFISH_LOG_INFO("Alpha size for config %d is %d", i, size);
            eglGetConfigAttrib(mDisplay, configs[i], EGL_STENCIL_SIZE, &size);
            STARFISH_LOG_INFO("Stencil size for config %d is %d", i, size);
            eglGetConfigAttrib(mDisplay, configs[i], EGL_DEPTH_SIZE, &size);
            STARFISH_LOG_INFO("Depth size for config %d is %d", i, size);
            // just choose the first one
            eglConf = configs[i];
            break;
        }

        // test version 3 first
        EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
        mContext =
            eglCreateContext(mDisplay, eglConf, EGL_NO_CONTEXT, contextAttribs);
        if (!mContext) {
            EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2,
                                        EGL_NONE };
            STARFISH_LOG_INFO(
                "failed to create opengl es 3+ context. use 2 instead");
            mContext = eglCreateContext(mDisplay, eglConf, EGL_NO_CONTEXT,
                                        contextAttribs);
        }

        mEglWindow = wl_egl_window_create(wlSurface, width, height);
        if (mEglWindow == EGL_NO_SURFACE) {
            STARFISH_LOG_INFO("Can't create egl window");
            exit(1);
        } else {
            STARFISH_LOG_INFO("Created egl window");
        }

        mSurface = eglCreateWindowSurface(mDisplay, eglConf, mEglWindow, NULL);

        if (eglMakeCurrent(mDisplay, mSurface, mSurface, mContext)) {
            STARFISH_LOG_INFO("Made current");
        } else {
            STARFISH_LOG_INFO("Made current failed");
        }

        mFence = nullptr;

#if !defined(PORT_WEBVIEW_BRIDGE_ECORE_WAYLAND2_HANDLE_FROM_ELM_WIN)
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT);
        glFlush();

        if (eglSwapBuffers(mDisplay, mSurface)) {
            STARFISH_LOG_INFO("Swapped buffers");
        } else {
            STARFISH_LOG_INFO("Swapped buffers failed");
        }

        STARFISH_LOG_INFO("wl_display_dispatch few times");
        size_t dispatchCount = 0;
        while (dispatchCount < 3) {
            if (wl_display_dispatch_pending(display) > 0) {
                wl_display_dispatch(display);
            }
            dispatchCount++;
        }
#endif

        eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE,
                       EGL_NO_CONTEXT);
        ::LWE::WebContainer* webContainer = ::LWE::WebContainer::CreateGL(
            width, height,
            [this](WebContainer* wc) {
                if (m_isBufferSwapped) {
                    if (mFence) {
                        Starfish::LongTaskFinder p(
                            "WebViewEcoreWayland2 - eglClientWaitSyncKHRProc",
                            1);
                        EGLint result = g_eglClientWaitSyncKHRProc(
                            mDisplay, mFence, EGL_SYNC_FLUSH_COMMANDS_BIT_KHR,
                            EGL_FOREVER_KHR);
                        if (result == EGL_FALSE) {
                            STARFISH_LOG_INFO(
                                "EGL FENCE: error waiting for fence: %d",
                                (int)eglGetError());
                        }
                        g_eglDestroySyncKHRProc(mDisplay, mFence);
                        mFence = nullptr;
                    }
                    m_isBufferSwapped = false;
                }
                {
                    Starfish::LongTaskFinder p(
                        "WebViewEcoreWayland2 - eglMakeCurrent", 1);
                    if (!eglMakeCurrent(mDisplay, mSurface, mSurface,
                                        mContext)) {
                        auto eglError = eglGetError();
                        STARFISH_LOG_INFO("Made current failed error -> %d",
                                          (int)eglError);
                    }
                }
            },
            [this](WebContainer* wc, bool mayNeedsSync) {
                {
                    Starfish::LongTaskFinder p(
                        "WebViewEcoreWayland2 - eglSwapBuffers", 2);
                    if (!eglSwapBuffers(mDisplay, mSurface)) {
                        auto eglError = eglGetError();
                        STARFISH_LOG_INFO("Made current failed error -> %d",
                                          (int)eglError);
                    }
                }
                if (m_lastInputTime) {
                    ANNOTATE_SETUP;
                    ANNOTATE_CHANNEL_COLOR(3002, ANNOTATE_GREEN,
                                           "response time");
#ifdef STARFISH_ENABLE_PROFILE_TIMER
                    uint64_t end = Starfish::longTickCount();
                    float time = (float)((end - m_lastInputTime) / 1000.f);
                    STARFISH_LOG_INFO("response time is %f ms", time);
#endif
                    m_lastInputTime = 0;
                    ANNOTATE_CHANNEL_END(3002);
                }
                if (mayNeedsSync) {
                    mFence = g_eglCreateSyncKHRProc(mDisplay,
                                                    EGL_SYNC_FENCE_KHR, NULL);
                    if (!mFence) {
                        STARFISH_LOG_INFO("eglCreateSyncKHR Error: %d",
                                          (int)eglGetError());
                    }
                }
                m_isBufferSwapped = true;
            },
            devicePixelRatio, defaultFontName, locale, timezoneID);

        mEcoreEventHandlers.push_back(ecore_event_handler_add(
            ECORE_EVENT_MOUSE_BUTTON_DOWN,
            [](void* data, int type, void* event) -> Eina_Bool {
                Ecore_Event_Mouse_Button* mouseEvent =
                    (Ecore_Event_Mouse_Button*)(event);
                WebViewEcoreWayland2* webView = (WebViewEcoreWayland2*)data;
                if (mouseEvent->window ==
                    static_cast<unsigned int>(
                        ecore_wl2_window_id_get(webView->mEcoreWindow))) {
                    int currentPosX = mouseEvent->x;
                    int currentPosY = mouseEvent->y;

                    if (mouseEvent->buttons == 1) {
                        webView->FetchWebContainer()->DispatchMouseDownEvent(
                            MouseButtonValue::LeftButton,
                            MouseButtonsValue::LeftButtonDown, currentPosX,
                            currentPosY);
                        webView->m_isMouseLbuttonDown = true;
                    }

                    // TODO
                    // webView->HideSoftwareKeyboardIfPossible();
                }
                return ECORE_CALLBACK_PASS_ON;
            },
            this));

        mEcoreEventHandlers.push_back(ecore_event_handler_add(
            ECORE_EVENT_MOUSE_BUTTON_UP,
            [](void* data, int type, void* event) -> Eina_Bool {
                Ecore_Event_Mouse_Button* mouseEvent =
                    (Ecore_Event_Mouse_Button*)(event);
                WebViewEcoreWayland2* webView = (WebViewEcoreWayland2*)data;
                if (mouseEvent->window ==
                    static_cast<unsigned int>(
                        ecore_wl2_window_id_get(webView->mEcoreWindow))) {
                    int currentPosX = mouseEvent->x;
                    int currentPosY = mouseEvent->y;

                    if (mouseEvent->buttons == 1) {
                        webView->FetchWebContainer()->DispatchMouseUpEvent(
                            MouseButtonValue::NoButton,
                            MouseButtonsValue::NoButtonDown, currentPosX,
                            currentPosY);
                        webView->m_isMouseLbuttonDown = false;
                    }
                }
                return ECORE_CALLBACK_PASS_ON;
            },
            this));

        mEcoreEventHandlers.push_back(ecore_event_handler_add(
            ECORE_EVENT_MOUSE_MOVE,
            [](void* data, int type, void* event) -> Eina_Bool {
                Ecore_Event_Mouse_Move* mouseEvent =
                    (Ecore_Event_Mouse_Move*)(event);
                WebViewEcoreWayland2* webView = (WebViewEcoreWayland2*)data;
                if (mouseEvent->window ==
                    static_cast<unsigned int>(
                        ecore_wl2_window_id_get(webView->mEcoreWindow))) {
                    int currentPosX = mouseEvent->x;
                    int currentPosY = mouseEvent->y;

                    unsigned char buttons =
                        webView->m_isMouseLbuttonDown
                            ? MouseButtonsValue::LeftButtonDown
                            : 0;
                    webView->FetchWebContainer()->DispatchMouseMoveEvent(
                        MouseButtonValue::NoButton, (MouseButtonsValue)buttons,
                        currentPosX, currentPosY);
                }
                return ECORE_CALLBACK_PASS_ON;
            },
            this));

        mEcoreEventHandlers.push_back(ecore_event_handler_add(
            ECORE_EVENT_KEY_DOWN,
            [](void* data, int type, void* event) -> Eina_Bool {
                Ecore_Event_Key* keyEvent = (Ecore_Event_Key*)(event);
                WebViewEcoreWayland2* webView = (WebViewEcoreWayland2*)data;
                if (keyEvent->window ==
                    static_cast<unsigned int>(
                        ecore_wl2_window_id_get(webView->mEcoreWindow))) {
                    std::string keyName = keyEvent->keyname;

                    STARFISH_LOG_INFO(
                        "ECORE_EVENT_KEY_DOWN [%s, %d]", keyName.data(),
                        (keyEvent->modifiers & 1) || (keyEvent->modifiers & 2));

                    bool lastInputTimeWasZeroBefore = false;
                    if (webView->m_lastInputTime == 0) {
                        ANNOTATE_SETUP;
                        ANNOTATE_CHANNEL_COLOR(3000, ANNOTATE_GREEN,
                                               "ECORE_EVENT_KEY_DOWN");
                        lastInputTimeWasZeroBefore = true;
                        webView->m_lastInputTime = Starfish::longTickCount();
                        ANNOTATE_CHANNEL_END(3000);
                    }

                    if (!webView->m_hasFocus) {
                        STARFISH_LOG_INFO(
                            "ignore keydown because we dont have focus");
                        return ECORE_CALLBACK_PASS_ON;
                    }

#ifdef STARFISH_TIZEN_TV
                    if ((strncmp(keyName.data(), "XF86Red", 7) == 0)) {
                        keyName = "Tab";
                    } else if ((strncmp(keyName.data(), "XF86Back", 8) == 0)) {
                        keyName = "Escape";
                    }
#endif

                    if ((strcmp(keyName.data(), "XF86Exit") == 0) ||
                        (strcmp(keyName.data(), "Select") == 0) ||
                        (strcmp(keyName.data(), "Cancel") == 0)) {
                        if (strcmp(keyName.data(), "Select") == 0) {
                            webView->FetchWebContainer()->AddIdleCallback(
                                [](void* data) {
                                    WebViewEcoreWayland2* self =
                                        (WebViewEcoreWayland2*)data;
                                    KeyValue kv = KeyValue::EnterKey;
                                    self->FetchWebContainer()
                                        ->DispatchKeyDownEvent(kv);
                                    self->FetchWebContainer()
                                        ->DispatchKeyPressEvent(kv);
                                    self->FetchWebContainer()
                                        ->DispatchKeyUpEvent(kv);
                                    self->HideSoftwareKeyboardIfPossible();
                                },
                                webView);
                        } else {
                            webView->FetchWebContainer()->AddIdleCallback(
                                [](void* data) {
                                    WebViewEcoreWayland2* self =
                                        (WebViewEcoreWayland2*)data;
                                    self->HideSoftwareKeyboardIfPossible();
                                },
                                webView);
                        }
                    }

                    auto keyValue =
                        ecoreEventKeyToKeyValue(keyName.data(), false);

                    if (keyValue >= ArrowDownKey && keyValue <= ArrowRightKey) {
                        int currentTimestamp = keyEvent->timestamp;
                        if (currentTimestamp -
                                g_arrowKeyDownTimestamp[keyValue -
                                                        ArrowDownKey] <
                            g_arrowKeyDownMinimumDelayInMS) {
                            return ECORE_CALLBACK_PASS_ON;
                        }
                        g_arrowKeyDownTimestamp[keyValue - ArrowDownKey] =
                            currentTimestamp;
                    }

                    if (lastInputTimeWasZeroBefore) {
                        webView->FetchWebContainer()->DispatchKeyDownEvent(
                            keyValue);
                        webView->FetchWebContainer()->DispatchKeyPressEvent(
                            keyValue);
                    } else {
                        struct Param {
                            WebViewEcoreWayland2* webView;
                            KeyValue keyValue;
                        };
                        Param* p = new Param();
                        p->webView = webView;
                        p->keyValue = keyValue;

                        webView->FetchWebContainer()->AddIdleCallback(
                            [](void* data) {
                                Param* p = (Param*)data;
                                p->webView->FetchWebContainer()
                                    ->DispatchKeyDownEvent(p->keyValue);
                                p->webView->FetchWebContainer()
                                    ->DispatchKeyPressEvent(p->keyValue);
                                delete p;
                            },
                            p);
                    }
                }
                return ECORE_CALLBACK_PASS_ON;
            },
            this));

        mEcoreEventHandlers.push_back(ecore_event_handler_add(
            ECORE_EVENT_KEY_UP,
            [](void* data, int type, void* event) -> Eina_Bool {
                Ecore_Event_Key* keyEvent = (Ecore_Event_Key*)(event);
                WebViewEcoreWayland2* webView = (WebViewEcoreWayland2*)data;
                if (keyEvent->window ==
                    static_cast<unsigned int>(
                        ecore_wl2_window_id_get(webView->mEcoreWindow))) {
                    std::string keyName = keyEvent->keyname;

                    STARFISH_LOG_INFO(
                        "ECORE_EVENT_KEY_UP [%s, %d]", keyName.data(),
                        (keyEvent->modifiers & 1) || (keyEvent->modifiers & 2));

                    if (!webView->m_hasFocus) {
                        STARFISH_LOG_INFO(
                            "ignore keyup because we dont have focus");
                        return ECORE_CALLBACK_PASS_ON;
                    }

#ifdef STARFISH_TIZEN_TV
                    if ((strncmp(keyName.data(), "XF86Red", 7) == 0)) {
                        keyName = "Tab";
                    } else if ((strncmp(keyName.data(), "XF86Back", 8) == 0)) {
                        keyName = "Escape";
                    }
#endif
                    auto keyValue =
                        ecoreEventKeyToKeyValue(keyName.data(), false);

                    if (keyValue >= ArrowDownKey && keyValue <= ArrowRightKey) {
                        g_arrowKeyDownTimestamp[keyValue - ArrowDownKey] = 0;
                    }

                    struct Param {
                        WebViewEcoreWayland2* webView;
                        KeyValue keyValue;
                    };
                    Param* p = new Param();
                    p->webView = webView;
                    p->keyValue = keyValue;

                    webView->FetchWebContainer()->AddIdleCallback(
                        [](void* data) {
                            Param* p = (Param*)data;
                            p->webView->FetchWebContainer()->DispatchKeyUpEvent(
                                p->keyValue);
                            delete p;
                        },
                        p);
                }
                return ECORE_CALLBACK_PASS_ON;
            },
            this));

        ecore_imf_init();

        webContainer->RegisterOnShowSoftwareKeyboardIfPossibleHandler(
            [this](WebContainer*) { ShowSoftwareKeyboardIfPossible(); });

        webContainer->RegisterOnHideSoftwareKeyboardIfPossibleHandler(
            [this](WebContainer* t) { HideSoftwareKeyboardIfPossible(); });

        webContainer->SetUserData("__internalLWEWebViewEFLEcoreWaylandHandle",
                                  mEcoreWindow);

        m_impl = webContainer;
    }

    void createInputMethod()
    {
        createIMFContext();
        registerIMFCallback();
    }

    void createIMFContext()
    {
        const char* contextId = ecore_imf_context_default_id_get();
        if (contextId) {
            m_IMFContext = ecore_imf_context_add(contextId);
        } else {
            STARFISH_LOG_ERROR("Default context is null. Use fallback");
            m_IMFContext = ecore_imf_context_add(getIMFMethod());
        }
        ecore_imf_context_client_window_set(
            m_IMFContext, (void*)ecore_wl2_window_id_get(mEcoreWindow));
    }

    static void CommitCallback(void* data, Ecore_IMF_Context* ctx,
                               void* event_info)
    {
        WebViewEcoreWayland2* self = (WebViewEcoreWayland2*)data;
        char* commit_str = (char*)event_info;
        STARFISH_LOG_INFO("ECORE_IMF_CALLBACK_COMMIT %s", commit_str);
        self->FetchWebContainer()->DispatchCompositionEndEvent(commit_str);
    }

    static void PreeditCallback(void* data, Ecore_IMF_Context* ctx,
                                void* event_info)
    {
        WebViewEcoreWayland2* self = (WebViewEcoreWayland2*)data;
        char* str = NULL;
        int cursor_pos;
        ecore_imf_context_preedit_string_get(self->m_IMFContext, &str,
                                             &cursor_pos);
        STARFISH_LOG_INFO("ECORE_IMF_CALLBACK_PREEDIT_CHANGED %s %d", str,
                          cursor_pos);
        if (str) {
            self->FetchWebContainer()->DispatchCompositionUpdateEvent(str);
            free(str);
        }
    }

    static void PrivateCommandCallback(void* data, Ecore_IMF_Context* ctx,
                                       void* event_info)
    {
        // TODO
    }

    static void DeleteSurroundingCallback(void* data, Ecore_IMF_Context* ctx,
                                          void* event_info)
    {
        // TODO
    }

    static void InputPanelStatChangedCallback(void* data,
                                              Ecore_IMF_Context* context,
                                              int value)
    {
        if (!data) {
            STARFISH_LOG_INFO("[No Data]");
            return;
        }
        WebViewEcoreWayland2* wv = (WebViewEcoreWayland2*)data;
        switch (value) {
        case ECORE_IMF_INPUT_PANEL_STATE_SHOW:
            wv->ShowPanel();
            STARFISH_LOG_INFO("[PANEL_STATE_SHOW]");
            break;
        case ECORE_IMF_INPUT_PANEL_STATE_HIDE:
            wv->HidePanel();
            STARFISH_LOG_INFO("[PANEL_STATE_HIDE]");
            break;
        case ECORE_IMF_INPUT_PANEL_STATE_WILL_SHOW:
            STARFISH_LOG_INFO("[PANEL_STATE_WILL_SHOW]");
            break;
        default:
            STARFISH_LOG_INFO("[PANEL_STATE_EVENT (default: %d)]", value);
            break;
        }
    }

    void registerIMFCallback()
    {
        STARFISH_ASSERT(m_IMFContext);

        ecore_imf_context_input_panel_enabled_set(m_IMFContext, false);
        ecore_imf_context_use_preedit_set(m_IMFContext, true);

        ecore_imf_context_event_callback_add(
            m_IMFContext, ECORE_IMF_CALLBACK_COMMIT, &CommitCallback, this);
        ecore_imf_context_event_callback_add(m_IMFContext,
                                             ECORE_IMF_CALLBACK_PREEDIT_CHANGED,
                                             &PreeditCallback, this);
        ecore_imf_context_event_callback_add(
            m_IMFContext, ECORE_IMF_CALLBACK_DELETE_SURROUNDING,
            &DeleteSurroundingCallback, this);
        ecore_imf_context_event_callback_add(
            m_IMFContext, ECORE_IMF_CALLBACK_PRIVATE_COMMAND_SEND,
            &PrivateCommandCallback, this);

        ecore_imf_context_input_panel_event_callback_add(
            m_IMFContext, ECORE_IMF_INPUT_PANEL_STATE_EVENT,
            &InputPanelStatChangedCallback, this);

        // These APIs have to be set when IMF's setting status is changed.
        ecore_imf_context_autocapital_type_set(m_IMFContext,
                                               ECORE_IMF_AUTOCAPITAL_TYPE_NONE);
        ecore_imf_context_prediction_allow_set(m_IMFContext, EINA_FALSE);
        ecore_imf_context_input_panel_layout_set(
            m_IMFContext, ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL);
        ecore_imf_context_input_panel_return_key_type_set(
            m_IMFContext, ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DEFAULT);
        ecore_imf_context_input_panel_layout_variation_set(m_IMFContext, 0);
    }

    void unregisterIMFCallback()
    {
        ecore_imf_context_event_callback_del(
            m_IMFContext, ECORE_IMF_CALLBACK_COMMIT, &CommitCallback);
        ecore_imf_context_event_callback_del(
            m_IMFContext, ECORE_IMF_CALLBACK_PREEDIT_CHANGED, &PreeditCallback);
        ecore_imf_context_event_callback_del(
            m_IMFContext, ECORE_IMF_CALLBACK_DELETE_SURROUNDING,
            &DeleteSurroundingCallback);
        ecore_imf_context_event_callback_del(
            m_IMFContext, ECORE_IMF_CALLBACK_PRIVATE_COMMAND_SEND,
            &PrivateCommandCallback);
        ecore_imf_context_input_panel_event_callback_del(
            m_IMFContext, ECORE_IMF_INPUT_PANEL_STATE_EVENT,
            &InputPanelStatChangedCallback);
        ecore_imf_context_del(m_IMFContext);
    }

    virtual void Destroy() override
    {
        Blur();

        FetchWebContainer()->Destroy();

        if (m_IMFContext) {
            unregisterIMFCallback();
            m_IMFContext = nullptr;
        }

        if (mFence) {
            g_eglDestroySyncKHRProc(mDisplay, mFence);
        }
        eglDestroySurface(mDisplay, mSurface);
        wl_egl_window_destroy(mEglWindow);
        eglDestroyContext(mDisplay, mContext);

        for (size_t i = 0; i < mEcoreEventHandlers.size(); i++) {
            ecore_event_handler_del(mEcoreEventHandlers[i]);
        }
        mEcoreEventHandlers.clear();

        delete this;
    }

    virtual void Focus() override
    {
        WebView::Focus();

        m_hasFocus = true;
    }

    virtual void Blur() override
    {
        WebView::Blur();

        m_hasFocus = false;
    }

    bool m_isMouseLbuttonDown;
    bool m_isBufferSwapped;
    bool m_hasFocus;
    bool m_isShowing;
    uint64_t m_lastInputTime;
    Ecore_IMF_Context* m_IMFContext;
    Ecore_Wl2_Window* mEcoreWindow;
    wl_display* mWlDisplay;
    EGLDisplay mDisplay;
    EGLSurface mSurface;
    EGLContext mContext;
    EGLSyncKHR mFence;
    wl_egl_window* mEglWindow;

    std::vector<Ecore_Event_Handler*> mEcoreEventHandlers;

    virtual ::LWE::WebContainer* FetchWebContainer() override
    {
        return (::LWE::WebContainer*)m_impl;
    }

    void ShowPanel()
    {
        Ecore_IMF_Context* ctx = m_IMFContext;

        STARFISH_LOG_INFO("ShowPanel() [wv->m_isShowing:%d] ", m_isShowing);
        if (!m_isShowing) {
            m_isShowing = true;
            ecore_imf_context_input_panel_show(ctx);
            ecore_imf_context_focus_in(ctx);
        }
    }

    void ShowSoftwareKeyboardIfPossible()
    {
        STARFISH_LOG_INFO("1.Show IMF()");

#if !defined(STARFISH_TIZEN_WEARABLE_WIDGET)
        FetchWebContainer()->AddIdleCallback(
            [](void* data) {
                WebViewEcoreWayland2* wv = (WebViewEcoreWayland2*)data;
                if (!wv->m_IMFContext) {
                    wv->createInputMethod();
                }
                wv->ShowPanel();
            },
            this);
#endif
    }

    void HidePanel()
    {
        Ecore_IMF_Context* ctx = m_IMFContext;

        STARFISH_LOG_INFO("HidePanel() [wv->m_isShowing:%d] ", m_isShowing);
        if (ctx && m_isShowing) {
            m_isShowing = false;
            ecore_imf_context_reset(ctx);
            ecore_imf_context_focus_out(ctx);
            ecore_imf_context_input_panel_hide(ctx);
        }
    }

    void HideSoftwareKeyboardIfPossible()
    {
        STARFISH_LOG_INFO("1.Hide IMF()");

#if !defined(STARFISH_TIZEN_WEARABLE_WIDGET)
        FetchWebContainer()->AddIdleCallback(
            [](void* data) {
                WebViewEcoreWayland2* wv = (WebViewEcoreWayland2*)data;
                wv->HidePanel();
            },
            this);
#endif
    }
};

WebView* WebView::Create(void* win, unsigned x, unsigned y, unsigned width,
                         unsigned height, float devicePixelRatio,
                         const char* defaultFontName, const char* locale,
                         const char* timezoneID)
{
#if defined(PORT_WEBVIEW_BRIDGE_ECORE_WAYLAND2_HANDLE_FROM_ELM_WIN)
    auto wndObj = ecore_evas_wayland2_window_get(
        ecore_evas_ecore_evas_get(evas_object_evas_get((const Eo*)win)));
    return new WebViewEcoreWayland2(wndObj, x, y, width, height,
                                    devicePixelRatio, defaultFontName, locale,
                                    timezoneID);
#endif
    return new WebViewEcoreWayland2(win, x, y, width, height, devicePixelRatio,
                                    defaultFontName, locale, timezoneID);
}
} // namespace LWE

#endif
