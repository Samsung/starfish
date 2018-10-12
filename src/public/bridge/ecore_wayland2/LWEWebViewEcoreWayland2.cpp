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

#include "StarFishConfig.h"
#include "LWEWebView.h"

#include "StarFish.h"

#if defined(PORT_WEBVIEW_BRIDGE_ECORE_WAYLAND2)

#define EFL_BETA_API_SUPPORT
#include <Ecore_Wl2.h>
#undef EFL_BETA_API_SUPPORT
#include <Ecore_Input.h>
#include <wayland-egl-core.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

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

namespace LWE {

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

class WebViewWayland : public WebView {
public:
    WebViewWayland(void* winArg, unsigned x, unsigned y, unsigned width,
                   unsigned height, float devicePixelRatio,
                   const char* defaultFontName, const char* locale,
                   const char* timezoneID)
        : WebView(nullptr)
        , m_isMouseLbuttonDown(false)
        , m_isBufferSwapped(false)
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

        STARFISH_LOG_INFO("wl_display %p surface %p\n", display, wlSurface);

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
                                    8,
                                    EGL_RENDERABLE_TYPE,
                                    EGL_OPENGL_ES2_BIT,
                                    EGL_NONE };

        static const EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2,
                                                  EGL_NONE };

        mDisplay = eglGetDisplay((EGLNativeDisplayType)display);
        if (mDisplay == EGL_NO_DISPLAY) {
            STARFISH_LOG_INFO("Can't create egl display\n");
            exit(1);
        } else {
            STARFISH_LOG_INFO("Created egl display\n");
        }

        if (eglInitialize(mDisplay, &major, &minor) != EGL_TRUE) {
            STARFISH_LOG_INFO("Can't initialise egl display\n");
            exit(1);
        }
        STARFISH_LOG_INFO("EGL major: %d, minor %d\n", major, minor);

        eglGetConfigs(mDisplay, NULL, 0, &count);
        STARFISH_LOG_INFO("EGL has %d configs\n", count);

        configs = (void**)alloca(count * sizeof(*configs));

        eglChooseConfig(mDisplay, config_attribs, configs, count, &n);

        EGLConfig eglConf = configs[0];
        for (i = 0; i < n; i++) {
            eglGetConfigAttrib(mDisplay, configs[i], EGL_BUFFER_SIZE, &size);
            STARFISH_LOG_INFO("Buffer size for config %d is %d\n", i, size);
            eglGetConfigAttrib(mDisplay, configs[i], EGL_RED_SIZE, &size);
            STARFISH_LOG_INFO("Red size for config %d is %d\n", i, size);
            eglGetConfigAttrib(mDisplay, configs[i], EGL_GREEN_SIZE, &size);
            STARFISH_LOG_INFO("Green size for config %d is %d\n", i, size);
            eglGetConfigAttrib(mDisplay, configs[i], EGL_BLUE_SIZE, &size);
            STARFISH_LOG_INFO("Blue size for config %d is %d\n", i, size);
            eglGetConfigAttrib(mDisplay, configs[i], EGL_ALPHA_SIZE, &size);
            STARFISH_LOG_INFO("Alpha size for config %d is %d\n", i, size);
            eglGetConfigAttrib(mDisplay, configs[i], EGL_STENCIL_SIZE, &size);
            STARFISH_LOG_INFO("Stencil size for config %d is %d\n", i, size);
            eglGetConfigAttrib(mDisplay, configs[i], EGL_DEPTH_SIZE, &size);
            STARFISH_LOG_INFO("Depth size for config %d is %d\n", i, size);
            // just choose the first one
            eglConf = configs[i];
            break;
        }

        mContext = eglCreateContext(mDisplay, eglConf, EGL_NO_CONTEXT,
                                    context_attribs);

        mEglWindow = wl_egl_window_create(wlSurface, width, height);
        if (mEglWindow == EGL_NO_SURFACE) {
            STARFISH_LOG_INFO("Can't create egl window\n");
            exit(1);
        } else {
            STARFISH_LOG_INFO("Created egl window\n");
        }

        mSurface = eglCreateWindowSurface(mDisplay, eglConf, mEglWindow, NULL);

        if (eglMakeCurrent(mDisplay, mSurface, mSurface, mContext)) {
            STARFISH_LOG_INFO("Made current\n");
        } else {
            STARFISH_LOG_INFO("Made current failed\n");
        }

        mFence = g_eglCreateSyncKHRProc(mDisplay, EGL_SYNC_FENCE_KHR, NULL);
        STARFISH_LOG_INFO("SyncFence info %p\n", mFence);
        if (!mFence) {
            STARFISH_LOG_INFO("FENCE Error: %d\n", (int)eglGetError());
        }

        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT);
        glFlush();

        if (eglSwapBuffers(mDisplay, mSurface)) {
            STARFISH_LOG_INFO("Swapped buffers\n");
        } else {
            STARFISH_LOG_INFO("Swapped buffers failed\n");
        }

        size_t dispatchCount = 0;
        while (dispatchCount < 3) {
            if (wl_display_dispatch_pending(display) > 0) {
                wl_display_dispatch(display);
            }
            dispatchCount++;
        }

        eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE,
                       EGL_NO_CONTEXT);
        ::LWE::WebContainer* webContainer = ::LWE::WebContainer::CreateGL(
            width, height,
            [this](WebContainer* wc) {
                if (m_isBufferSwapped) {
                    EGLint result = g_eglClientWaitSyncKHRProc(
                        mDisplay, mFence, EGL_SYNC_FLUSH_COMMANDS_BIT_KHR,
                        EGL_FOREVER_KHR);
                    if (result == EGL_FALSE) {
                        STARFISH_LOG_INFO(
                            "EGL FENCE: error waiting for fence: %d\n",
                            (int)eglGetError());
                    }
                    m_isBufferSwapped = false;
                }
                if (!eglMakeCurrent(mDisplay, mSurface, mSurface, mContext)) {
                    auto eglError = eglGetError();
                    STARFISH_LOG_INFO("Made current failed error -> %d\n",
                                      (int)eglError);
                }
            },
            [this](WebContainer* wc) {
                if (!eglSwapBuffers(mDisplay, mSurface)) {
                    auto eglError = eglGetError();
                    STARFISH_LOG_INFO("Made current failed error -> %d\n",
                                      (int)eglError);
                }
                m_isBufferSwapped = true;
                eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE,
                               EGL_NO_CONTEXT);
            },
            devicePixelRatio, defaultFontName, locale, timezoneID);

        mEcoreEventHandlers.push_back(ecore_event_handler_add(
            ECORE_EVENT_MOUSE_BUTTON_DOWN,
            [](void* data, int type, void* event) -> Eina_Bool {
                Ecore_Event_Mouse_Button* mouseEvent =
                    (Ecore_Event_Mouse_Button*)(event);
                WebViewWayland* webView = (WebViewWayland*)data;
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
                WebViewWayland* webView = (WebViewWayland*)data;
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
                WebViewWayland* webView = (WebViewWayland*)data;
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
                WebViewWayland* webView = (WebViewWayland*)data;
                if (keyEvent->window ==
                    static_cast<unsigned int>(
                        ecore_wl2_window_id_get(webView->mEcoreWindow))) {
                    std::string keyName = keyEvent->keyname;

                    STARFISH_LOG_INFO(
                        "ECORE_EVENT_KEY_DOWN [%s, %d]\n", keyName.data(),
                        (keyEvent->modifiers & 1) || (keyEvent->modifiers & 2));

#ifdef STARFISH_TIZEN_TV
                    if ((strncmp(keyName.data(), "XF86Red", 7) == 0)) {
                        keyName = "Tab";
                    } else if ((strncmp(keyName.data(), "XF86Back", 8) == 0)) {
                        keyName = "Escape";
                    }
#endif
                    auto keyValue =
                        ecoreEventKeyToKeyValue(keyName.data(), false);

                    webView->FetchWebContainer()->DispatchKeyDownEvent(
                        keyValue);
                    webView->FetchWebContainer()->DispatchKeyPressEvent(
                        keyValue);
                }
                return ECORE_CALLBACK_PASS_ON;
            },
            this));

        mEcoreEventHandlers.push_back(ecore_event_handler_add(
            ECORE_EVENT_KEY_UP,
            [](void* data, int type, void* event) -> Eina_Bool {
                Ecore_Event_Key* keyEvent = (Ecore_Event_Key*)(event);
                WebViewWayland* webView = (WebViewWayland*)data;
                if (keyEvent->window ==
                    static_cast<unsigned int>(
                        ecore_wl2_window_id_get(webView->mEcoreWindow))) {
                    std::string keyName = keyEvent->keyname;

                    STARFISH_LOG_INFO(
                        "ECORE_EVENT_KEY_UP [%s, %d]\n", keyName.data(),
                        (keyEvent->modifiers & 1) || (keyEvent->modifiers & 2));

#ifdef STARFISH_TIZEN_TV
                    if ((strncmp(keyName.data(), "XF86Red", 7) == 0)) {
                        keyName = "Tab";
                    } else if ((strncmp(keyName.data(), "XF86Back", 8) == 0)) {
                        keyName = "Escape";
                    }
#endif
                    auto keyValue =
                        ecoreEventKeyToKeyValue(keyName.data(), false);

                    webView->FetchWebContainer()->DispatchKeyUpEvent(keyValue);
                }
                return ECORE_CALLBACK_PASS_ON;
            },
            this));

        webContainer->SetUserData("__internalLWEWebViewEFLEcoreWaylandHandle",
                                  mEcoreWindow);

        m_impl = webContainer;
    }
    virtual void Destroy() override
    {
        WebView::Destroy();

        g_eglDestroySyncKHRProc(mDisplay, mFence);
        eglDestroySurface(mDisplay, mSurface);
        wl_egl_window_destroy(mEglWindow);
        eglDestroyContext(mDisplay, mContext);

        for (size_t i = 0; i < mEcoreEventHandlers.size(); i++) {
            ecore_event_handler_del(mEcoreEventHandlers[i]);
        }
        mEcoreEventHandlers.clear();
    }

    bool m_isMouseLbuttonDown;
    bool m_isBufferSwapped;
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
};

WebView* WebView::Create(void* win, unsigned x, unsigned y, unsigned width,
                         unsigned height, float devicePixelRatio,
                         const char* defaultFontName, const char* locale,
                         const char* timezoneID)
{
    return new WebViewWayland(win, x, y, width, height, devicePixelRatio,
                              defaultFontName, locale, timezoneID);
}
}

#endif
