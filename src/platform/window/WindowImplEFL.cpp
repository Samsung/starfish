/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#if defined(PORT_WINDOW_BACKEND_EFL)
#include "StarFish.h"

#include "core/dom/Element.h"
#include "core/dom/MouseEvent.h"
#include "core/dom/TouchEvent.h"
#include "platform/event/PlatformKeyEventData.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "platform/window/PlatformWindow.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/profiling/Profiling.h"
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

#ifdef STARFISH_TIZEN_WEARABLE_WIDGET
#include <tizen.h>
#endif

#if defined(PORT_GRAPHIC_BACKEND_EFL_CAIRO)
#include <cairo.h>
#endif

#if defined(PORT_GRAPHIC_BACKEND_EFL_SKIA)
#include "SkCanvas.h"
#include "SkSurface.h"
#endif

#if defined(STARFISH_TIZEN) && defined(PORT_GRAPHIC_BACKEND_EFL_CAIRO)
#include <Evas_GL.h>
#include <cairo-evas-gl.h>
#endif

#ifndef STARFISH_TIZEN_WEARABLE_WIDGET
extern "C" Ecore_Evas* ecore_evas_ecore_evas_get(const Evas* e);
extern "C" Ecore_Window ecore_evas_window_get(const Ecore_Evas* e);
#endif

#ifdef STARFISH_ENABLE_TEST
extern bool g_fireOnloadEvent;
extern StarFish::CanvasSurface* g_surfaceForScreehShot;
#endif

const uint32_t REPEAT_DURATION = 1000;
Evas* g_internalCanvas;

namespace StarFish {

struct IdlerData {
    void (*m_fn)(void*);
    void* m_data;
};

Evas* internalCanvas()
{
    STARFISH_RELEASE_ASSERT(g_internalCanvas);
    return g_internalCanvas;
}
static int g_totalCanvasSurfaceEFLSize;
static PlatformWindow* g_currentWnd = nullptr;
static void* g_focusedWin = nullptr;

class WindowImplEFL : public PlatformWindow {
public:
    WindowImplEFL(StarFish* sf)
        : PlatformWindow(sf)
    {
        m_mainBox = nullptr;
        m_nonIMEKeyEventBox = nullptr;
        m_dummyBox = nullptr;
        m_dummyBoxClipper = nullptr;
#if defined(PORT_GRAPHIC_BACKEND_EFL_CAIRO)
        m_canvasAdpater = nullptr;
        m_canvasAdpaterSurface = nullptr;
        m_canvasAdpaterCairo = nullptr;
#endif
#if defined(PORT_GRAPHIC_BACKEND_EFL_SKIA)
        m_canvasAdpater = nullptr;
        m_canvasAdpaterSurface = nullptr;
        m_canvasAdpaterSkia = nullptr;
#endif
        m_renderingAnimator = nullptr;
        m_isMouseLbuttonDown = false;
        m_isKeyDown = false;
        m_lastClickedTimestamp = 0;
        m_clickedCount = 0;
        m_canRendering = true;
        m_inRendering = false;
        m_imfContext = nullptr;
        m_lastKeyPressedTimestamp = 0;
        m_offsetYDueToSoftwareKeyboard = 0;
        m_softKeyboardOrigin = nullptr;
        m_lastRenderingTime = tickCount();

#if defined(STARFISH_TIZEN) && defined(PORT_GRAPHIC_BACKEND_EFL_CAIRO) && \
    defined(STARFISH_TIZEN_EVASGL_CAIRO)
        m_surface = nullptr;
        m_cairo = nullptr;
        m_cairoDevice = nullptr;
        m_evasGL = nullptr;
        m_evasGLConfig = nullptr;
        m_evasGLSurface = nullptr;
        m_evasGLContext = nullptr;
#endif
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
        evas_object_geometry_get(eflWindow->m_mainBox, NULL, NULL, &width,
                                 NULL);
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
        evas_object_geometry_get(eflWindow->m_mainBox, NULL, NULL, NULL,
                                 &height);
        return (height)-m_offsetYDueToSoftwareKeyboard;
    }

    virtual void resizeTo(int w, int h) override
    {
        evas_object_resize(m_mainBox, w, h);
    }

    virtual void* unwrap() override
    {
        return (void*)m_window;
    }

    virtual bool canRendering()
    {
        return m_canRendering;
    }

    virtual void clearResources() override;
    virtual Canvas* preparePainting() override;
    virtual Compositor* prepareCompositor() override;

    virtual void showSoftwareKeyboardIfPossible() override
    {
        if (ecore_imf_input_panel_hide() == EINA_FALSE) {
            starFish()->messageLoop()->addIdler(
                nullptr,
                [](size_t a, void* data) {
                    WindowImplEFL* self = ((WindowImplEFL*)data);
                    evas_object_focus_set(self->m_nonIMEKeyEventBox,
                                          EINA_FALSE);
                    evas_object_focus_set(self->m_mainBox, EINA_TRUE);
                    self->m_softKeyboardOrigin = self->webView()->focusedNode();
                },
                this);
            webView()->mainBrowsingContext()->window()->clearTimeout(
                m_keyboardTimeoutId);
        } else {
            m_keyboardTimeoutId =
                webView()->mainBrowsingContext()->window()->setTimeout(
                    [](Window* window, void* data) {
                        WindowImplEFL* self = ((WindowImplEFL*)data);
                        self->showSoftwareKeyboardIfPossible();
                    },
                    100, this);
        }
    }

    virtual void hideSoftwareKeyboardIfPossible() override
    {
        starFish()->messageLoop()->addIdler(
            nullptr,
            [](size_t a, void* data) {
                WindowImplEFL* self = ((WindowImplEFL*)data);
                evas_object_focus_set(self->m_mainBox, EINA_FALSE);
                evas_object_focus_set(self->m_nonIMEKeyEventBox, EINA_TRUE);
            },
            this);
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
                if (nd) {
                    Node* e = nd->nearestParentElement();
                    if (e->isElement()) {
                        // FIXME (enable this)
                        // e->asElement()->scrollIntoView(true);
                    }
                }
            }
        }
    }

    virtual bool isIMEEnabledNow() override
    {
        return evas_object_focus_get(m_mainBox) == EINA_TRUE;
    }

    virtual void pause() override
    {
        STARFISH_LOG_INFO("WindowImpleEFL::pause()\n");
        PlatformWindow::pause();
        if (g_currentWnd == this) {
            g_currentWnd = nullptr;
        }
    }

    virtual void resume() override
    {
        STARFISH_LOG_INFO("WindowImpleEFL::resume()\n");
        if (g_currentWnd != this) {
            g_currentWnd = this;
        }
        PlatformWindow::resume();
        webView()->setNeedsRendering();
    }

    virtual void close() override
    {
        STARFISH_LOG_INFO("WindowImplEFL::close()\n");

        if (m_renderingAnimator) {
            ecore_animator_freeze(m_renderingAnimator);
            ecore_animator_del(m_renderingAnimator);
            m_renderingAnimator = nullptr;
        }

#if defined(PORT_GRAPHIC_BACKEND_EFL_CAIRO) || \
    defined(PORT_GRAPHIC_BACKEND_EFL_SKIA)
        if (m_canvasAdpater) {
            evas_object_del(m_canvasAdpater);
            m_canvasAdpater = nullptr;
        }
#endif
#if defined(PORT_GRAPHIC_BACKEND_EFL_CAIRO)
        if (m_canvasAdpaterSurface) {
            cairo_surface_destroy(m_canvasAdpaterSurface);
            cairo_destroy(m_canvasAdpaterCairo);
            m_canvasAdpaterSurface = nullptr;
            m_canvasAdpaterCairo = nullptr;
        }
#endif
#if defined(PORT_GRAPHIC_BACKEND_EFL_SKIA)
        if (m_canvasAdpaterSurface) {
            m_canvasAdpaterSurface = nullptr;
            m_canvasAdpaterSkia = nullptr;
        }
#endif

        if (m_dummyBoxClipper) {
            evas_object_del(m_dummyBoxClipper);
            m_dummyBoxClipper = nullptr;
        }

        if (m_dummyBox) {
            evas_object_del(m_dummyBox);
            m_dummyBox = nullptr;
        }

        if (m_mainBox) {
            // elm_win_resize_object_del(m_window, m_mainBox);
            evas_object_del(m_mainBox);
            m_mainBox = nullptr;
        }

        if (m_nonIMEKeyEventBox) {
            evas_object_del(m_nonIMEKeyEventBox);
            m_nonIMEKeyEventBox = nullptr;
        }

        if (m_imfContext) {
            ecore_imf_context_del(m_imfContext);
        }

        evas_event_callback_del(evas_object_evas_get(m_mainBox),
                                EVAS_CALLBACK_RENDER_POST, m_renderingHandler);

        if (m_resizeHandler) {
            evas_object_event_callback_del(m_window, EVAS_CALLBACK_RESIZE,
                                           m_resizeHandler);
        }

#ifndef STARFISH_TIZEN_WEARABLE_WIDGET
        evas_object_event_callback_del(m_mainBox, EVAS_CALLBACK_MOUSE_DOWN,
                                       m_mouseDownEventHandler);
        evas_object_event_callback_del(m_mainBox, EVAS_CALLBACK_MOUSE_UP,
                                       m_mouseUpEventHandler);
        evas_object_event_callback_del(m_mainBox, EVAS_CALLBACK_MOUSE_WHEEL,
                                       m_mouseWheelEventHandler);
        evas_object_event_callback_del(m_mainBox, EVAS_CALLBACK_MOUSE_MOVE,
                                       m_mouseMoveEventHandler);
        evas_object_event_callback_del(m_mainBox, EVAS_CALLBACK_KEY_DOWN,
                                       m_keyDownEventHandler);
        evas_object_event_callback_del(m_mainBox, EVAS_CALLBACK_KEY_UP,
                                       m_keyUpEventHandler);
#endif

#ifdef STARFISH_TIZEN_WEARABLE_WIDGET
        evas_object_event_callback_del(m_dummyBox, EVAS_CALLBACK_MOUSE_DOWN,
                                       m_mouseDownEventHandler);
        evas_object_event_callback_del(m_dummyBox, EVAS_CALLBACK_MOUSE_MOVE,
                                       m_mouseMoveEventHandler);
        evas_object_event_callback_del(m_dummyBox, EVAS_CALLBACK_MOUSE_UP,
                                       m_mouseUpEventHandler);
        evas_object_smart_callback_del(m_dummyBox, "clicked",
                                       m_clickEventHandler);

#endif
#ifdef STARFISH_TIZEN_WEARABLE_WIDGET
        if (!starFish()->updateFlag()) {
            evas_object_del((Evas_Object*)m_window);
        }
#endif
        PlatformWindow::close();
        if (g_currentWnd == this) {
            g_currentWnd = nullptr;
        }
    }

    virtual void onIdle() override
    {
        PlatformWindow::onIdle();
    }

    virtual bool rendering() override
    {
        m_inRendering = true;
        // ProfilerTimer renderingTimer("WindowImplEFL::rendering");
        bool ret = PlatformWindow::rendering();
        if (ret) {
            m_canRendering = false;
        }
#if defined(PORT_GRAPHIC_BACKEND_EFL_CAIRO)
        if (ret && m_canvasAdpaterCairo) {
            int w, h;
            evas_object_image_size_get(m_canvasAdpater, &w, &h);
            evas_object_image_data_update_add(m_canvasAdpater, 0, 0, w, h);
        }
#endif
#if defined(PORT_GRAPHIC_BACKEND_EFL_SKIA)
        if (ret && m_canvasAdpaterSkia) {
            int w, h;
            evas_object_image_size_get(m_canvasAdpater, &w, &h);
            evas_object_image_data_update_add(m_canvasAdpater, 0, 0, w, h);
        }
#endif
#if defined(STARFISH_TIZEN_WEARABLE_WIDGET)
        evas_object_raise(m_dummyBox);
#else
        if (!isIMEEnabledNow() && g_focusedWin == this) {
            evas_object_focus_set(m_nonIMEKeyEventBox, EINA_TRUE);
        }
#endif
        m_inRendering = false;
        return ret;
    }

    virtual void setNeedsRendering() override;

    uintptr_t m_handle;
    Evas_Object* m_window;
#if defined(PORT_GRAPHIC_BACKEND_EFL_CAIRO)
    Evas_Object* m_canvasAdpater;
    cairo_surface_t* m_canvasAdpaterSurface;
    cairo_t* m_canvasAdpaterCairo;
#endif
#if defined(PORT_GRAPHIC_BACKEND_EFL_SKIA)
    Evas_Object* m_canvasAdpater;
    sk_sp<SkSurface> m_canvasAdpaterSurface;
    SkCanvas* m_canvasAdpaterSkia;
#endif
    std::vector<Evas_Object*> m_objectList;
    Evas_Object* m_mainBox;
    Evas_Object* m_nonIMEKeyEventBox;
    Evas_Object* m_dummyBox;
    Evas_Object* m_dummyBoxClipper;

#if defined(STARFISH_TIZEN) && defined(PORT_GRAPHIC_BACKEND_EFL_CAIRO) && \
    defined(STARFISH_TIZEN_EVASGL_CAIRO)
    cairo_surface_t* m_surface;
    cairo_t* m_cairo;
    cairo_device_t* m_cairoDevice;
    Evas_GL* m_evasGL;
    Evas_GL_Config* m_evasGLConfig;
    Evas_GL_Surface* m_evasGLSurface;
    Evas_GL_Context* m_evasGLContext;
#endif

    void (*m_resizeHandler)(void* data, Evas* evas, Evas_Object* obj,
                            void* event_info);
    void (*m_renderingHandler)(void* data, Evas* evas, void* event_info);
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
    void (*m_clickEventHandler)(void* data, Evas_Object* obj, void* event_info);

    Ecore_Animator* m_renderingAnimator;

    Ecore_IMF_Context* m_imfContext;
    Node* m_softKeyboardOrigin;

    float m_lastMouseX, m_lastMouseY;
    bool m_isMouseLbuttonDown;
    bool m_isKeyDown;
    bool m_canRendering;
    bool m_inRendering;
    uint32_t m_lastClickedTimestamp;
    uint32_t m_clickedCount;
    uint32_t m_lastKeyPressedTimestamp;
    uint64_t m_lastRenderingTime;
    int m_offsetYDueToSoftwareKeyboard;
    size_t m_keyboardTimeoutId;
};

#if defined(PORT_COMPOSITOR_BACKEND_EFL)
class CanvasSurfaceEFL : public CanvasSurface {
public:
    CanvasSurfaceEFL(PlatformWindow* wnd, size_t w, size_t h)
    {
        m_window = (WindowImplEFL*)wnd;
        m_image =
            evas_object_image_add(evas_object_evas_get(m_window->m_window));
        evas_object_image_filled_set(m_image, EINA_TRUE);
#ifndef STARFISH_TIZEN_TV
        evas_object_image_colorspace_set(
            m_image, Evas_Colorspace::EVAS_COLORSPACE_ARGB8888);
#endif
        evas_object_image_alpha_set(m_image, EINA_TRUE);
        evas_object_anti_alias_set(m_image, EINA_TRUE);
        evas_object_image_content_hint_set(m_image,
                                           EVAS_IMAGE_CONTENT_HINT_DYNAMIC);
        STARFISH_RELEASE_ASSERT(evas_object_image_colorspace_get(m_image) ==
                                EVAS_COLORSPACE_ARGB8888);

        m_bufferStride = m_imageWidth = m_bufferWidth = m_width = SIZE_MAX;
        m_imageHeight = m_bufferHeight = m_height = SIZE_MAX;
        m_pixelRatio = 1;

        attachNativeBuffer(w, h);
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
        int w, h;
        evas_object_image_size_get(image, &w, &h);
        g_totalCanvasSurfaceEFLSize -= (w * h * 4);
        evas_object_image_size_set(image, 0, 0);
        evas_object_hide(image);
        STARFISH_RELEASE_ASSERT(evas_object_ref_get(image) == 0);
        evas_object_del(image);
    }

    virtual uint8_t* data()
    {
        void* address = evas_object_image_data_get(m_image, EINA_TRUE);
        STARFISH_ASSERT(address);
        evas_object_image_data_set(m_image, address);
        return (uint8_t*)address;
    }

    virtual void dump(const char* path)
    {
        evas_object_image_save(m_image, path, nullptr, nullptr);
    }

    virtual void detachNativeBuffer()
    {
        detachNative(m_image);
        m_image = nullptr;
    }

    void attachNativeBuffer(size_t w, size_t h)
    {
        if (m_width != w || m_height != h) {
            int ww, hh;
            evas_object_image_size_get(m_image, &ww, &hh);
            g_totalCanvasSurfaceEFLSize -= (ww * hh * 4);

            m_width = w;
            m_height = h;

            if ((int)w < m_window->starFish()->screenInfo().rect.width()) {
                w += STARFISH_CANVAS_SURFACE_MARGIN;
            }
            if ((int)h < m_window->starFish()->screenInfo().rect.height()) {
                h += STARFISH_CANVAS_SURFACE_MARGIN;
            }

            size_t v = 20000;
            void* address = nullptr;
            do {
                m_pixelRatio = 1;

                while ((m_width / m_pixelRatio > v) ||
                       (m_height / m_pixelRatio > v)) {
                    m_pixelRatio++;
                }

                m_imageWidth = std::max((size_t)1, m_width / m_pixelRatio);
                m_imageHeight = std::max((size_t)1, m_height / m_pixelRatio);

                m_bufferWidth = std::max((size_t)1, w / m_pixelRatio);
                m_bufferHeight = std::max((size_t)1, h / m_pixelRatio);

                evas_object_image_size_set(m_image, m_bufferWidth,
                                           m_bufferHeight);
                address = evas_object_image_data_get(m_image, EINA_FALSE);
                evas_object_image_data_set(m_image, address);
            } while (address == nullptr && (v -= 5000));
            STARFISH_ASSERT(address);

            int stride = evas_object_image_stride_get(m_image);
            if (stride < 0) {
                STARFISH_LOG_ERROR(
                    "evas_object_image_stride_get return minus value... "
                    "correct it!!");
                stride = m_bufferWidth * 4;
            }
            m_bufferStride = (size_t)stride;
            g_totalCanvasSurfaceEFLSize += (m_bufferWidth * m_bufferHeight * 4);
        }
    }

    virtual void resize(size_t w, size_t h)
    {
        STARFISH_RELEASE_ASSERT(w <= m_bufferWidth * m_pixelRatio);
        STARFISH_RELEASE_ASSERT(h <= m_bufferHeight * m_pixelRatio);

        m_width = w;
        m_height = h;

        m_imageWidth = std::max((size_t)1, m_width / m_pixelRatio);
        m_imageHeight = std::max((size_t)1, m_height / m_pixelRatio);

        STARFISH_RELEASE_ASSERT(m_imageWidth <= m_bufferWidth);
        STARFISH_RELEASE_ASSERT(m_imageHeight <= m_bufferHeight);
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

    virtual size_t bufferWidth()
    {
        return m_bufferWidth;
    }

    virtual size_t bufferHeight()
    {
        return m_bufferHeight;
    }

    virtual size_t imageWidth()
    {
        return m_imageWidth;
    }

    virtual size_t imageHeight()
    {
        return m_imageHeight;
    }

    virtual size_t pixelRatio()
    {
        return m_pixelRatio;
    }

    virtual size_t bufferStride()
    {
        return m_bufferStride;
    }

    virtual void clear()
    {
        void* address = evas_object_image_data_get(m_image, EINA_TRUE);
        size_t end = m_bufferStride * m_bufferHeight;
        memset(address, 0xff, end);
        evas_object_image_data_set(m_image, address);
    }

protected:
    WindowImplEFL* m_window;
    Evas_Object* m_image;
    size_t m_width;
    size_t m_height;
    size_t m_imageWidth;
    size_t m_imageHeight;
    size_t m_bufferWidth;
    size_t m_bufferHeight;
    size_t m_bufferStride;
    size_t m_pixelRatio;
};

CanvasSurface* CanvasSurface::create(PlatformWindow* wnd, size_t w, size_t h)
{
    return new CanvasSurfaceEFL(wnd, w, h);
}

#endif

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
    }
#ifdef STARFISH_TIZEN_TV
    else if (strcmp("XF86AudioRaiseVolume", ecoreKeyString) == 0) {
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
    }
#endif
    STARFISH_LOG_ERROR("WindowImplEFL - unimplemented key %s\n",
                       ecoreKeyString);
    return KeyValue::UnidentifiedKey;
}

static void setModifiersToPlatformKeyEventData(Ecore_Event_Key* d,
                                               PlatformKeyEventData& k)
{
    if (d->modifiers == 1 || d->keycode == 50 || d->keycode == 62) {
        k.setShiftKey(true);
    } else if (d->modifiers == 2 || d->keycode == 37) {
        k.setCtrlKey(true);
    } else if (d->modifiers == 4 || d->keycode == 64) {
        k.setAltKey(true);
    }
}

static void setModifiersToPlatformKeyEventData(Evas_Modifier* d,
                                               PlatformKeyEventData& k)
{
    if ((evas_key_modifier_is_set(d, "Shift_L") == EINA_TRUE) ||
        (evas_key_modifier_is_set(d, "Shift_R") == EINA_TRUE)) {
        k.setShiftKey(true);
    } else if ((evas_key_modifier_is_set(d, "Control_L") == EINA_TRUE) ||
               (evas_key_modifier_is_set(d, "Control_R") == EINA_TRUE)) {
        k.setCtrlKey(true);
    } else if ((evas_key_modifier_is_set(d, "Alt_L") == EINA_TRUE) ||
               (evas_key_modifier_is_set(d, "Alt_R") == EINA_TRUE)) {
        k.setAltKey(true);
    }
}

static void setRepeatToPlatformKeyEventData(WindowImplEFL* window,
                                            uint32_t timestamp,
                                            PlatformKeyEventData& k)
{
    if (!window->m_isKeyDown) {
        window->m_lastKeyPressedTimestamp = timestamp;
        return;
    }

    if (timestamp - window->m_lastKeyPressedTimestamp < REPEAT_DURATION) {
        k.setRepeat(true);
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

const uint32_t CLICK_REFRESH_DELAY = 400;

void elm_box_layout_cb(Evas_Object* o, Evas_Object_Box_Data* priv,
                       void* user_data)
{
}
PlatformWindow* PlatformWindow::create(StarFish* sf, void* win, int width,
                                       int height)
{
    auto wnd = new WindowImplEFL(sf);
    g_currentWnd = wnd;

    wnd->m_starFish = sf;
    wnd->m_window = (Evas_Object*)win;
    wnd->m_mainBox = elm_box_add(wnd->m_window);
    evas_object_resize(wnd->m_mainBox, width, height);
    evas_object_move(wnd->m_mainBox, wnd->starFish()->posX(),
                     wnd->starFish()->posY());
    wnd->m_nonIMEKeyEventBox = elm_label_add(wnd->m_window);
    evas_object_show(wnd->m_nonIMEKeyEventBox);
    elm_box_layout_set(wnd->m_mainBox, elm_box_layout_cb, NULL, NULL);
    evas_object_show(wnd->m_mainBox);

#if defined(PORT_GRAPHIC_BACKEND_EFL_CAIRO) || \
    defined(PORT_GRAPHIC_BACKEND_EFL_SKIA)
    wnd->m_canvasAdpater =
        evas_object_image_add(evas_object_evas_get(wnd->m_mainBox));
    elm_box_pack_end(wnd->m_mainBox, wnd->m_canvasAdpater);
    evas_object_resize(wnd->m_canvasAdpater, width, height);
    evas_object_move(wnd->m_canvasAdpater, wnd->starFish()->posX(),
                     wnd->starFish()->posY());
    evas_object_image_content_hint_set(wnd->m_canvasAdpater,
                                       EVAS_IMAGE_CONTENT_HINT_DYNAMIC);
    evas_object_image_alpha_set(wnd->m_canvasAdpater, EINA_TRUE);
#endif
#if defined(STARFISH_TIZEN) && defined(PORT_GRAPHIC_BACKEND_EFL_CAIRO) && \
    defined(STARFISH_TIZEN_EVASGL_CAIRO)
    Evas_Native_Surface ns;
    wnd->m_evasGL = evas_gl_new(evas_object_evas_get(wnd->m_canvasAdpater));
    wnd->m_evasGLConfig = evas_gl_config_new();
    wnd->m_evasGLConfig->color_format = EVAS_GL_RGBA_8888;
    wnd->m_evasGLConfig->stencil_bits = EVAS_GL_STENCIL_BIT_8;
    wnd->m_evasGLConfig->multisample_bits = EVAS_GL_MULTISAMPLE_MED;
    wnd->m_evasGLSurface = evas_gl_surface_create(
        wnd->m_evasGL, wnd->m_evasGLConfig, wnd->width(), wnd->height());
    wnd->m_evasGLContext = evas_gl_context_create(wnd->m_evasGL, NULL);
    evas_gl_native_surface_get(wnd->m_evasGL, wnd->m_evasGLSurface, &ns);
    evas_object_image_native_surface_set(wnd->m_canvasAdpater, &ns);
    evas_object_image_pixels_get_callback_set(
        wnd->m_canvasAdpater,
        [](void* data, Evas_Object* o) {
            STARFISH_RELEASE_ASSERT(isMainThread());
            WindowImplEFL* wnd = (WindowImplEFL*)data;
            StarFishEnterer enter(wnd->starFish());
            wnd->rendering();
        },
        wnd);

    setenv("CAIRO_GL_COMPOSITOR", "msaa", 1);
    wnd->m_cairoDevice = (cairo_device_t*)cairo_evas_gl_device_create(
        wnd->m_evasGL, wnd->m_evasGLContext);
    cairo_gl_device_set_thread_aware(wnd->m_cairoDevice, 0);
    wnd->m_surface = (cairo_surface_t*)cairo_gl_surface_create_for_evas_gl(
        wnd->m_cairoDevice, wnd->m_evasGLSurface, wnd->m_evasGLConfig,
        wnd->width(), wnd->height());
    wnd->m_cairo = cairo_create(wnd->m_surface);
#endif
#ifndef STARFISH_TIZEN_WEARABLE_WIDGET
    Evas* e = evas_object_evas_get(wnd->m_mainBox);
    Ecore_Evas* ee = ecore_evas_ecore_evas_get(e);
    Ecore_Window ew = ecore_evas_window_get(ee);
    wnd->m_handle = (uintptr_t)ew;
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

    wnd->m_mouseDownEventHandler = [](void* data, Evas* evas, Evas_Object* obj,
                                      void* event_info) -> void {
        g_focusedWin = data;
        WindowImplEFL* sf = (WindowImplEFL*)data;
        Evas_Event_Mouse_Down* ev = (Evas_Event_Mouse_Down*)event_info;

        // We care just left button now
        int currentPosX = ev->output.x - sf->starFish()->posX();
        int currentPosY = ev->output.y - sf->starFish()->posY();
        if (ev->button == 1 && (currentPosX >= 0 && currentPosY >= 0)) {
            StarFishEnterer enter(sf->starFish());
            if (ev->timestamp - sf->m_lastClickedTimestamp >
                CLICK_REFRESH_DELAY) {
                sf->m_clickedCount = 1;
                sf->m_lastClickedTimestamp = ev->timestamp;
            } else {
                sf->m_clickedCount++;
            }
            MouseData mdata(
                MouseButtonValue::LeftButton, MouseButtonsValue::LeftButtonDown,
                currentPosX / sf->starFish()->screenInfo().deviceScaleFactor,
                currentPosY / sf->starFish()->screenInfo().deviceScaleFactor,
                sf->m_clickedCount);
            sf->dispatchMouseEvent(MouseEventKind::MouseEventDown, mdata);
            sf->m_isMouseLbuttonDown = true;
        }
        return;
    };
    evas_object_event_callback_add(wnd->m_mainBox, EVAS_CALLBACK_MOUSE_DOWN,
                                   wnd->m_mouseDownEventHandler, wnd);

    wnd->m_mouseUpEventHandler = [](void* data, Evas* evas, Evas_Object* obj,
                                    void* event_info) -> void {
        WindowImplEFL* sf = (WindowImplEFL*)data;
        Evas_Event_Mouse_Up* ev = (Evas_Event_Mouse_Up*)event_info;
        // We care just left button now
        int currentPosX = ev->output.x - sf->starFish()->posX();
        int currentPosY = ev->output.y - sf->starFish()->posY();
        if (ev->button == 1 && (currentPosX >= 0 && currentPosY >= 0)) {
            StarFishEnterer enter(sf->starFish());
            if (ev->timestamp - sf->m_lastClickedTimestamp >
                CLICK_REFRESH_DELAY) {
                sf->m_clickedCount = 1;
                sf->m_lastClickedTimestamp = ev->timestamp;
            } else {
                sf->m_clickedCount++;
            }
            MouseData mdata(
                MouseButtonValue::NoButton, MouseButtonsValue::NoButtonDown,
                currentPosX / sf->starFish()->screenInfo().deviceScaleFactor,
                currentPosY / sf->starFish()->screenInfo().deviceScaleFactor,
                sf->m_clickedCount);
            sf->dispatchMouseEvent(MouseEventKind::MouseEventUp, mdata);
            sf->m_isMouseLbuttonDown = false;
        }
        return;
    };
    evas_object_event_callback_add(wnd->m_mainBox, EVAS_CALLBACK_MOUSE_UP,
                                   wnd->m_mouseUpEventHandler, wnd);

    wnd->m_mouseWheelEventHandler = [](void* data, Evas* evas, Evas_Object* obj,
                                       void* event_info) -> void {
        WindowImplEFL* sf = (WindowImplEFL*)data;
        Evas_Event_Mouse_Wheel* ev = (Evas_Event_Mouse_Wheel*)event_info;
        StarFishEnterer enter(sf->m_starFish);
        // We care just left button now
        int currentPosX = ev->output.x - sf->starFish()->posX();
        int currentPosY = ev->output.y - sf->starFish()->posY();
        if (currentPosX >= 0 && currentPosY >= 0) {
            sf->dispatchMouseWheelEvent(
                currentPosX / sf->starFish()->screenInfo().deviceScaleFactor,
                currentPosY / sf->starFish()->screenInfo().deviceScaleFactor,
                ev->z, true);
        }
        return;
    };
    evas_object_event_callback_add(wnd->m_mainBox, EVAS_CALLBACK_MOUSE_WHEEL,
                                   wnd->m_mouseWheelEventHandler, wnd);

    wnd->m_mouseMoveEventHandler = [](void* data, Evas* evas, Evas_Object* obj,
                                      void* event_info) -> void {
        WindowImplEFL* sf = (WindowImplEFL*)data;
        Evas_Event_Mouse_Move* ev = (Evas_Event_Mouse_Move*)event_info;
        // We care just left button now
        int currentPosX = ev->cur.output.x - sf->starFish()->posX();
        int currentPosY = ev->cur.output.y - sf->starFish()->posY();
        if (currentPosX >= 0 && currentPosY >= 0) {
            StarFishEnterer enter(sf->m_starFish);
            unsigned char buttons = sf->m_isMouseLbuttonDown
                                        ? MouseButtonsValue::LeftButtonDown
                                        : 0;
            MouseData mdata(
                0, buttons,
                currentPosX / sf->starFish()->screenInfo().deviceScaleFactor,
                currentPosY / sf->starFish()->screenInfo().deviceScaleFactor,
                0);
            sf->dispatchMouseEvent(MouseEventKind::MouseEventMove, mdata);
        }
        return;
    };
    evas_object_event_callback_add(wnd->m_mainBox, EVAS_CALLBACK_MOUSE_MOVE,
                                   wnd->m_mouseMoveEventHandler, wnd);

    wnd->m_keyDownEventHandler = [](void* data, Evas* evas, Evas_Object* obj,
                                    void* event_info) -> void {
        WindowImplEFL* sf = (WindowImplEFL*)data;
        Evas_Event_Key_Down* ev = (Evas_Event_Key_Down*)event_info;

        if (evas_object_focus_get(sf->m_mainBox) == EINA_TRUE) {
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
            ev->key,
            (evas_key_modifier_is_set(ev->modifiers, "Shift_L") == EINA_TRUE) ||
                (evas_key_modifier_is_set(ev->modifiers, "Shift_R") ==
                 EINA_TRUE));
        PlatformKeyEventData pkdata(keyValue);
        setRepeatToPlatformKeyEventData(sf, ev->timestamp, pkdata);
        setModifiersToPlatformKeyEventData(ev->modifiers, pkdata);
        StarFishEnterer enter(sf->m_starFish);
        sf->dispatchKeyEvent(KeyEventKind::KeyEventDown, pkdata);
        sf->dispatchKeyEvent(KeyEventKind::KeyEventPress, pkdata);
        sf->m_isKeyDown = true;

#ifdef STARFISH_TIZEN_TV
        if ((strncmp(ev->key, "XF86Exit", 8) == 0)) {
            evas_object_del(sf->m_window);
        }
#endif
    };
    evas_object_event_callback_add(wnd->m_nonIMEKeyEventBox,
                                   EVAS_CALLBACK_KEY_DOWN,
                                   wnd->m_keyDownEventHandler, wnd);

    wnd->m_keyUpEventHandler = [](void* data, Evas* evas, Evas_Object* obj,
                                  void* event_info) -> void {
        WindowImplEFL* sf = (WindowImplEFL*)data;
        Evas_Event_Key_Up* ev = (Evas_Event_Key_Up*)event_info;

        if (evas_object_focus_get(sf->m_mainBox) == EINA_TRUE) {
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
            ev->key,
            (evas_key_modifier_is_set(ev->modifiers, "Shift_L") == EINA_TRUE) ||
                (evas_key_modifier_is_set(ev->modifiers, "Shift_R") ==
                 EINA_TRUE));

        PlatformKeyEventData kdata(keyValue);
        setModifiersToPlatformKeyEventData(ev->modifiers, kdata);
        StarFishEnterer enter(sf->m_starFish);
        sf->dispatchKeyEvent(KeyEventKind::KeyEventUp, kdata);
        sf->m_isKeyDown = false;
        return;
    };
    evas_object_event_callback_add(wnd->m_nonIMEKeyEventBox,
                                   EVAS_CALLBACK_KEY_UP,
                                   wnd->m_keyUpEventHandler, wnd);

#else
    Evas* e = evas_object_evas_get(wnd->m_window);

    wnd->m_dummyBox = elm_button_add(wnd->m_window);
    int w, h;
    evas_object_geometry_get(wnd->m_mainBox, &w, &h, NULL, NULL);
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

    wnd->m_mouseDownEventHandler = [](void* data, Evas* evas, Evas_Object* obj,
                                      void* event_info) -> void {
        WindowImplEFL* sf = (WindowImplEFL*)data;
        Evas_Event_Mouse_Down* ev = (Evas_Event_Mouse_Down*)event_info;
        sf->m_lastMouseX = ev->canvas.x;
        sf->m_lastMouseY = ev->canvas.y;

        StarFishEnterer enter(sf->starFish());
        sf->m_clickedCount = 1;

        MouseData mdata(
            MouseButtonValue::LeftButton, MouseButtonsValue::LeftButtonDown,
            ev->canvas.x / sf->starFish()->screenInfo().deviceScaleFactor,
            ev->canvas.y / sf->starFish()->screenInfo().deviceScaleFactor,
            sf->m_clickedCount);
        sf->dispatchMouseEvent(MouseEventKind::MouseEventDown, mdata);
        sf->m_isMouseLbuttonDown = true;
    };
    evas_object_event_callback_add(wnd->m_dummyBox, EVAS_CALLBACK_MOUSE_DOWN,
                                   wnd->m_mouseDownEventHandler, wnd);

    wnd->m_mouseMoveEventHandler = [](void* data, Evas* evas, Evas_Object* obj,
                                      void* event_info) -> void {
        WindowImplEFL* sf = (WindowImplEFL*)data;
        Evas_Event_Mouse_Move* ev = (Evas_Event_Mouse_Move*)event_info;
        ((WindowImplEFL*)sf)->m_lastMouseX = ev->cur.canvas.x;
        ((WindowImplEFL*)sf)->m_lastMouseY = ev->cur.canvas.y;

        StarFishEnterer enter(sf->m_starFish);

        unsigned char buttons =
            sf->m_isMouseLbuttonDown ? MouseButtonsValue::LeftButtonDown : 0;
        MouseData mdata(0, buttons,
                        ((WindowImplEFL*)sf)->m_lastMouseX /
                            sf->starFish()->screenInfo().deviceScaleFactor,
                        ((WindowImplEFL*)sf)->m_lastMouseY /
                            sf->starFish()->screenInfo().deviceScaleFactor,
                        0);
        sf->dispatchMouseEvent(MouseEventKind::MouseEventMove, mdata);
    };
    evas_object_event_callback_add(wnd->m_dummyBox, EVAS_CALLBACK_MOUSE_MOVE,
                                   wnd->m_mouseMoveEventHandler, wnd);

    wnd->m_mouseUpEventHandler = [](void* data, Evas* evas, Evas_Object* obj,
                                    void* event_info) -> void {
        WindowImplEFL* sf = (WindowImplEFL*)data;

        StarFishEnterer enter(sf->starFish());
        MouseData mdata(MouseButtonValue::NoButton,
                        MouseButtonsValue::NoButtonDown,
                        ((WindowImplEFL*)sf)->m_lastMouseX /
                            sf->starFish()->screenInfo().deviceScaleFactor,
                        ((WindowImplEFL*)sf)->m_lastMouseY /
                            sf->starFish()->screenInfo().deviceScaleFactor,
                        sf->m_clickedCount);
        sf->dispatchMouseEvent(MouseEventKind::MouseEventUp, mdata);
        sf->m_isMouseLbuttonDown = false;
    };
    evas_object_event_callback_add(wnd->m_dummyBox, EVAS_CALLBACK_MOUSE_UP,
                                   wnd->m_mouseUpEventHandler, wnd);

    wnd->m_clickEventHandler = [](void* data, Evas_Object* obj,
                                  void* event_info) -> void {
        WindowImplEFL* sf = (WindowImplEFL*)data;

        StarFishEnterer enter(sf->m_starFish);
        MouseData mdata(MouseButtonValue::NoButton,
                        MouseButtonsValue::NoButtonDown,
                        ((WindowImplEFL*)sf)->m_lastMouseX /
                            sf->starFish()->screenInfo().deviceScaleFactor,
                        ((WindowImplEFL*)sf)->m_lastMouseY /
                            sf->starFish()->screenInfo().deviceScaleFactor,
                        sf->m_clickedCount);
        sf->dispatchMouseEvent(MouseEventKind::MouseEventUp, mdata);
    };
    evas_object_smart_callback_add(wnd->m_dummyBox, "clicked",
                                   wnd->m_clickEventHandler, wnd);

#endif

    wnd->m_renderingHandler = [](void* data, Evas* evas,
                                 void* event_info) -> void {
        STARFISH_ASSERT(isMainThread());
        WindowImplEFL* wnd = (WindowImplEFL*)data;
        wnd->m_canRendering = true;
        wnd->m_lastRenderingTime = tickCount();
    };

    evas_event_callback_add(evas_object_evas_get(wnd->m_mainBox),
                            EVAS_CALLBACK_RENDER_POST, wnd->m_renderingHandler,
                            wnd);

    if (sf->shouldFitWindow()) {
        wnd->m_resizeHandler = [](void* data, Evas* e, Evas_Object* obj,
                                  void* event_info) {
            WindowImplEFL* wnd = (WindowImplEFL*)data;
            int w, h;
            evas_object_geometry_get(wnd->m_window, NULL, NULL, &w, &h);
            evas_object_resize(wnd->m_mainBox, w, h);
        };
        evas_object_event_callback_add(wnd->m_window, EVAS_CALLBACK_RESIZE,
                                       wnd->m_resizeHandler, wnd);
    } else {
        wnd->m_resizeHandler = nullptr;
    }

    evas_object_event_callback_add(
        wnd->m_mainBox, EVAS_CALLBACK_RESIZE,
        [](void* data, Evas* e, Evas_Object* obj, void* event_info) {
            WindowImplEFL* wnd = (WindowImplEFL*)data;
#if defined(PORT_GRAPHIC_BACKEND_EFL_CAIRO) || \
    defined(PORT_GRAPHIC_BACKEND_EFL_SKIA)
            ProfilerTimer t(wnd->starFish(), "WindowImplEFL resize");
            int w, h;
            evas_object_image_size_get(wnd->m_canvasAdpater, &w, &h);
            if (w != wnd->width() || h != wnd->height()) {
                evas_object_image_size_set(wnd->m_canvasAdpater, 1, 1);
                evas_object_image_fill_set(wnd->m_canvasAdpater, 0, 0, 1, 1);
                StarFishEnterer enter(wnd->starFish());
                wnd->onResize();
            }
#else
            StarFishEnterer enter(wnd->starFish());
            wnd->onResize();
#endif
        },
        wnd);
    evas_object_event_callback_add(
        wnd->m_mainBox, EVAS_CALLBACK_MOVE,
        [](void* data, Evas* e, Evas_Object* obj, void* event_info) {
#if defined(PORT_GRAPHIC_BACKEND_EFL_CAIRO) || \
    defined(PORT_GRAPHIC_BACKEND_EFL_SKIA)
            WindowImplEFL* wnd = (WindowImplEFL*)data;
            StarFishEnterer enter(wnd->starFish());
            int x, y;
            evas_object_geometry_get(wnd->m_mainBox, &x, &y, NULL, NULL);
            wnd->starFish()->setPos(x, y);
            evas_object_move(wnd->m_canvasAdpater, x, y);
#endif
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
            ecore_evas_ecore_evas_get(evas_object_evas_get(wnd->m_mainBox))));
    ecore_imf_context_client_canvas_set(wnd->m_imfContext,
                                        evas_object_evas_get(wnd->m_mainBox));

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
            self->dispatchCompositionEvent(
                CompositionEventKind::CompositionEventEnd,
                String::fromUTF8(commit_str), self->m_softKeyboardOrigin);
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
                CompositionEventKind::CompositionEventStart,
                String::emptyString, self->m_softKeyboardOrigin);
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
                    CompositionEventKind::CompositionEventUpdate,
                    String::fromUTF8(str), self->m_softKeyboardOrigin);
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
                if (self->m_softKeyboardOrigin ==
                    self->webView()->focusedNode()) {
                    self->webView()->blur();
                }
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
                    self->m_starFish->messageLoop()->addIdler(
                        nullptr,
                        [](size_t, void* data) {
                            WindowImplEFL* self = (WindowImplEFL*)data;
                            StarFishEnterer enter(self->m_starFish);
                            KeyValue kv = KeyValue::EnterKey;
                            PlatformKeyEventData pkdata(kv);
                            self->dispatchKeyEvent(KeyEventKind::KeyEventDown,
                                                   pkdata);
                            self->dispatchKeyEvent(KeyEventKind::KeyEventUp,
                                                   pkdata);
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

            // process non-char keys
            STARFISH_LOG_INFO("process non-char [%s]\n", ev->key);
            auto keyValue = ecoreEventKeyToKeyValue(
                ev->key, (evas_key_modifier_is_set(ev->modifiers, "Shift_L") ==
                          EINA_TRUE) ||
                             (evas_key_modifier_is_set(
                                  ev->modifiers, "Shift_R") == EINA_TRUE));
            if ((strcmp(ev->key, "Up") != 0) &&
                (strcmp(ev->key, "Down") != 0)) {
                PlatformKeyEventData kdata(keyValue);
                setRepeatToPlatformKeyEventData(self, ev->timestamp, kdata);
                setModifiersToPlatformKeyEventData(ev->modifiers, kdata);
                StarFishEnterer enter(self->m_starFish);
                self->dispatchKeyEvent(KeyEventKind::KeyEventDown, kdata);
                self->dispatchKeyEvent(KeyEventKind::KeyEventPress, kdata);
                self->m_isKeyDown = true;
            }

            if (tryFilter && !String::isASCIIPrintableKey(keyValue)) {
                Ecore_IMF_Event_Key_Down ecore_ev;
                ecore_imf_evas_event_key_down_wrap(ev, &ecore_ev);
                ecore_imf_context_filter_event(self->m_imfContext,
                                               ECORE_IMF_EVENT_KEY_DOWN,
                                               (Ecore_IMF_Event*)&ecore_ev);
            }
        },
        wnd);
    evas_object_event_callback_add(
        wnd->m_mainBox, EVAS_CALLBACK_KEY_UP,
        [](void* data, Evas* e, Evas_Object* obj, void* event_info) {
            WindowImplEFL* self = (WindowImplEFL*)data;
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
                        self->m_imfContext, ECORE_IMF_EVENT_KEY_UP,
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
                        (evas_key_modifier_is_set(ev->modifiers, "Shift_R") ==
                         EINA_TRUE));
                PlatformKeyEventData kdata(keyValue);
                setModifiersToPlatformKeyEventData(ev->modifiers, kdata);
                StarFishEnterer enter(self->m_starFish);
                self->dispatchKeyEvent(KeyEventKind::KeyEventUp, kdata);
                self->m_isKeyDown = false;
            }
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

            if (ecore_imf_context_input_panel_state_get(ctx) ==
                ECORE_IMF_INPUT_PANEL_STATE_SHOW) {
                ecore_imf_evas_event_key_down_wrap(
                    (Evas_Event_Key_Down*)event_info, &ev);
                // ecore_imf_context_reset(ctx);
                ecore_imf_context_focus_out(ctx);
                ecore_imf_context_hide(ctx);
            }
        },
        wnd);

    ecore_imf_context_autocapital_type_set(wnd->m_imfContext,
                                           ECORE_IMF_AUTOCAPITAL_TYPE_NONE);
    ecore_imf_context_prediction_allow_set(wnd->m_imfContext, EINA_FALSE);
    g_focusedWin = wnd;
    wnd->m_starFish->setWebViewDelegator(wnd->m_mainBox);
    return wnd;
}

void WindowImplEFL::setNeedsRendering()
{
    WindowImplEFL* wnd = this;

#if defined(STARFISH_TIZEN) && defined(PORT_GRAPHIC_BACKEND_EFL_CAIRO) && \
    defined(STARFISH_TIZEN_EVASGL_CAIRO)
    evas_object_image_pixels_dirty_set(wnd->m_canvasAdpater, EINA_TRUE);
#else
    // refresh rendering animator
    if (wnd->m_renderingAnimator) {
        ecore_animator_freeze(wnd->m_renderingAnimator);
        ecore_animator_del(wnd->m_renderingAnimator);
        wnd->m_renderingAnimator = nullptr;
    }

    wnd->m_renderingAnimator = ecore_animator_add(
        [](void* data) -> Eina_Bool {
            WindowImplEFL* wnd = (WindowImplEFL*)data;
            if (g_currentWnd != wnd) {
                STARFISH_LOG_INFO("An invalid animator callback was called.\n")
                return ECORE_CALLBACK_CANCEL;
            }

#ifndef STARFISH_ENABLE_TEST
            if (evas_object_visible_get(wnd->m_window) != EINA_TRUE) {
                return ECORE_CALLBACK_RENEW;
            }
#endif

            if (!wnd->m_canRendering) {
                return ECORE_CALLBACK_RENEW;
            }

            StarFishEnterer enter(wnd->starFish());
            wnd->rendering();
            wnd->m_renderingAnimator = nullptr;
            return ECORE_CALLBACK_CANCEL;
        },
        wnd);
#endif
}

Canvas* WindowImplEFL::preparePainting()
{
#if defined(PORT_GRAPHIC_BACKEND_EFL)
#ifdef STARFISH_ENABLE_TEST
    {
        const char* path = getenv("SCREEN_SHOT");
        if (path && strlen(path) && g_fireOnloadEvent) {
            g_surfaceForScreehShot =
                CanvasSurface::create(this, width(), height());
            starFish()->addPointerInRootSet(g_surfaceForScreehShot);
            Canvas* c = Canvas::create(starFish(), g_surfaceForScreehShot);
            return c;
        }
    }
#endif
    int width, height;
    evas_object_geometry_get(m_mainBox, NULL, NULL, &width, &height);
    Evas* evas = evas_object_evas_get(m_mainBox);
    struct dummy {
        void* a;
        void* b;
        int w;
        int h;
        std::vector<Evas_Object*>* objList;
        std::vector<Evas_Object*>* surfaceList;
        bool f;
    };
    dummy* d = new dummy;
    d->a = evas;
    d->b = nullptr;
    d->w = width + starFish()->posX();
    d->h = height + starFish()->posY();
    d->objList = &m_objectList;
    d->f = false;
    auto iter = m_objectList.begin();
    while (iter != m_objectList.end()) {
        evas_object_del(*iter);
        iter++;
    }
    m_objectList.clear();
    m_objectList.shrink_to_fit();
    Canvas* canvas = Canvas::createDirect(starFish(), d);
    delete d;

    return canvas;
#endif

#if defined(PORT_GRAPHIC_BACKEND_EFL_CAIRO)
#if defined(STARFISH_TIZEN) && defined(STARFISH_TIZEN_EVASGL_CAIRO)
    struct dummy {
        cairo_t* cairo;
        cairo_surface_t* surface;
        int w;
        int h;
    } d;
    d.cairo = m_cairo;
    d.surface = m_surface;
    d.w = width();
    d.h = height();
    return Canvas::createDirect(starFish(), &d);
#endif
#ifdef STARFISH_ENABLE_TEST
    {
        const char* path = getenv("SCREEN_SHOT");
        if (path && strlen(path) && g_fireOnloadEvent) {
            g_surfaceForScreehShot =
                CanvasSurface::create(this, width(), height());
            starFish()->addPointerInRootSet(g_surfaceForScreehShot);
            STARFISH_LOG_INFO(
                "WindowImplEFL::preparePainting buffer info(screen shot) %p -> "
                "%p\n",
                g_surfaceForScreehShot->data(),
                g_surfaceForScreehShot->data() +
                    (g_surfaceForScreehShot->bufferStride() *
                     g_surfaceForScreehShot->bufferHeight()));
            Canvas* c = Canvas::create(starFish(), g_surfaceForScreehShot);
            return c;
        }
    }
#endif

    auto iter = m_objectList.begin();
    while (iter != m_objectList.end()) {
        evas_object_del(*iter);
        iter++;
    }
    m_objectList.clear();
    m_objectList.shrink_to_fit();

    if (m_canvasAdpaterCairo) {
        cairo_destroy(m_canvasAdpaterCairo);
        cairo_surface_destroy(m_canvasAdpaterSurface);
        m_canvasAdpaterCairo = nullptr;
        m_canvasAdpaterSurface = nullptr;
    }

    {
        int w, h;
        evas_object_image_size_get(m_canvasAdpater, &w, &h);
        if (w != width() || h != height()) {
            evas_object_resize(m_canvasAdpater, width(), height());
            evas_object_image_size_set(m_canvasAdpater, width(), height());
            evas_object_image_fill_set(m_canvasAdpater, 0, 0, width(),
                                       height());
        }
    }

    int w, h;
    evas_object_image_size_get(m_canvasAdpater, &w, &h);
    evas_object_show(m_canvasAdpater);
    void* addr = evas_object_image_data_get(m_canvasAdpater, EINA_TRUE);
    evas_object_image_data_set(m_canvasAdpater, addr);
    m_canvasAdpaterSurface = cairo_image_surface_create_for_data(
        (unsigned char*)addr, CAIRO_FORMAT_ARGB32, w, h,
        evas_object_image_stride_get(m_canvasAdpater));
    STARFISH_LOG_INFO("WindowImplEFL::preparePainting buffer info %p -> %p\n",
                      addr,
                      ((unsigned char*)addr) +
                          (evas_object_image_stride_get(m_canvasAdpater) * h));
    cairo_surface_set_device_scale(m_canvasAdpaterSurface,
                                   m_starFish->screenInfo().deviceScaleFactor,
                                   m_starFish->screenInfo().deviceScaleFactor);
    m_canvasAdpaterCairo = cairo_create(m_canvasAdpaterSurface);

    struct dummy {
        cairo_t* cairo;
        cairo_surface_t* surface;
        int w;
        int h;
    } d;
    d.cairo = m_canvasAdpaterCairo;
    d.surface = m_canvasAdpaterSurface;
    d.w = width() + starFish()->posX();
    d.h = height() + starFish()->posY();
    return Canvas::createDirect(starFish(), &d);
#endif
#if defined(PORT_GRAPHIC_BACKEND_EFL_SKIA)
#ifdef STARFISH_ENABLE_TEST
    {
        const char* path = getenv("SCREEN_SHOT");
        if (path && strlen(path) && g_fireOnloadEvent) {
            g_surfaceForScreehShot =
                CanvasSurface::create(this, width(), height());
            starFish()->addPointerInRootSet(g_surfaceForScreehShot);
            STARFISH_LOG_INFO(
                "WindowImplEFL::preparePainting buffer info(screen shot) %p -> "
                "%p\n",
                g_surfaceForScreehShot->data(),
                g_surfaceForScreehShot->data() +
                    (g_surfaceForScreehShot->bufferStride() *
                     g_surfaceForScreehShot->bufferHeight()));
            Canvas* c = Canvas::create(starFish(), g_surfaceForScreehShot);
            return c;
        }
    }
#endif

    auto iter = m_objectList.begin();
    while (iter != m_objectList.end()) {
        evas_object_del(*iter);
        iter++;
    }
    m_objectList.clear();
    m_objectList.shrink_to_fit();

    if (m_canvasAdpaterSkia) {
        m_canvasAdpaterSkia = nullptr;
        m_canvasAdpaterSurface = nullptr;
    }

    {
        int w, h;
        evas_object_image_size_get(m_canvasAdpater, &w, &h);
        if (w != width() || h != height()) {
            evas_object_resize(m_canvasAdpater, width(), height());
            evas_object_image_size_set(m_canvasAdpater, width(), height());
            evas_object_image_fill_set(m_canvasAdpater, 0, 0, width(),
                                       height());
        }
    }

    int w, h;
    evas_object_image_size_get(m_canvasAdpater, &w, &h);
    evas_object_show(m_canvasAdpater);
    void* addr = evas_object_image_data_get(m_canvasAdpater, EINA_TRUE);
    evas_object_image_data_set(m_canvasAdpater, addr);

    SkImageInfo info = SkImageInfo::MakeN32Premul(w, h);
    size_t rowBytes = evas_object_image_stride_get(m_canvasAdpater);
    m_canvasAdpaterSurface = SkSurface::MakeRasterDirect(info, addr, rowBytes);
    // FIXME : Apply device scale factor
    STARFISH_LOG_INFO("WindowImplEFL::preparePainting buffer info %p -> %p\n",
                      addr,
                      ((unsigned char*)addr) +
                          (evas_object_image_stride_get(m_canvasAdpater) * h));

    m_canvasAdpaterSkia = m_canvasAdpaterSurface->getCanvas();
    struct dummy {
        SkCanvas* canvas;
        sk_sp<SkSurface> surface;
        int w;
        int h;
    } d;
    d.canvas = m_canvasAdpaterSkia;
    d.surface = m_canvasAdpaterSurface;
    d.w = width() + starFish()->posX();
    d.h = height() + starFish()->posY();
    return Canvas::createDirect(starFish(), &d);

#endif
}

Compositor* WindowImplEFL::prepareCompositor()
{
#if defined(PORT_COMPOSITOR_BACKEND_CAIRO)
#if defined(STARFISH_TIZEN) && defined(STARFISH_TIZEN_EVASGL_CAIRO)
    struct dummy {
        cairo_t* cairo;
        cairo_surface_t* surface;
        int w;
        int h;
    } d;
    d.cairo = m_cairo;
    d.surface = m_surface;
    d.w = width();
    d.h = height();
    return Compositor::create(starFish(), &d);
#endif
#ifdef STARFISH_ENABLE_TEST
    {
        const char* path = getenv("SCREEN_SHOT");
        if (path && strlen(path) && g_fireOnloadEvent) {
            g_surfaceForScreehShot =
                CanvasSurface::create(this, width(), height());
            starFish()->addPointerInRootSet(g_surfaceForScreehShot);
            Compositor* c =
                Compositor::create(starFish(), g_surfaceForScreehShot);
            return c;
        }
    }
#endif

    auto iter = m_objectList.begin();
    while (iter != m_objectList.end()) {
        evas_object_del(*iter);
        iter++;
    }
    m_objectList.clear();
    m_objectList.shrink_to_fit();

    if (m_canvasAdpaterCairo) {
        cairo_destroy(m_canvasAdpaterCairo);
        cairo_surface_destroy(m_canvasAdpaterSurface);
        m_canvasAdpaterCairo = nullptr;
        m_canvasAdpaterSurface = nullptr;
    }

    {
        int w, h;
        evas_object_image_size_get(m_canvasAdpater, &w, &h);
        if (w != width() || h != height()) {
            evas_object_image_size_set(m_canvasAdpater, width(), height());
            evas_object_image_fill_set(m_canvasAdpater, 0, 0, width(),
                                       height());
        }
    }

    evas_object_show(m_canvasAdpater);
    void* addr = evas_object_image_data_get(m_canvasAdpater, EINA_TRUE);
    evas_object_image_data_set(m_canvasAdpater, addr);
    m_canvasAdpaterSurface = cairo_image_surface_create_for_data(
        (unsigned char*)addr, CAIRO_FORMAT_ARGB32, width(), height(),
        evas_object_image_stride_get(m_canvasAdpater));
    cairo_surface_set_device_scale(m_canvasAdpaterSurface,
                                   m_starFish->screenInfo().deviceScaleFactor,
                                   m_starFish->screenInfo().deviceScaleFactor);
    m_canvasAdpaterCairo = cairo_create(m_canvasAdpaterSurface);

    struct dummy {
        cairo_t* cairo;
        cairo_surface_t* surface;
        int w;
        int h;
    } d;
    d.cairo = m_canvasAdpaterCairo;
    d.surface = m_canvasAdpaterSurface;
    d.w = width();
    d.h = height();
    return Compositor::create(starFish(), &d);
#endif
#if defined(PORT_COMPOSITOR_BACKEND_EFL)
#ifdef STARFISH_ENABLE_TEST
    {
        const char* path = getenv("SCREEN_SHOT");
        if (path && strlen(path) && g_fireOnloadEvent) {
            g_surfaceForScreehShot =
                CanvasSurface::create(this, width(), height());

            STARFISH_LOG_INFO(
                "WindowImplEFL::preparePainting buffer info(screen shot) %p -> "
                "%p\n",
                g_surfaceForScreehShot->data(),
                g_surfaceForScreehShot->data() +
                    (g_surfaceForScreehShot->bufferStride() *
                     g_surfaceForScreehShot->bufferHeight()));

            Compositor* c =
                Compositor::create(starFish(), g_surfaceForScreehShot);
            return c;
        }
    }
#endif
    int width, height;
    evas_object_geometry_get(m_mainBox, NULL, NULL, &width, &height);
    Evas* evas = evas_object_evas_get(m_window);
    struct dummy {
        void* a;
        void* b;
        int w;
        int h;
        std::vector<Evas_Object*>* objList;
        std::vector<Evas_Object*>* surfaceList;
        bool f;
    };
    elm_box_unpack_all(m_mainBox);
#if defined(PORT_GRAPHIC_BACKEND_EFL_CAIRO) || \
    defined(PORT_GRAPHIC_BACKEND_EFL_SKIA)
    elm_box_pack_end(m_mainBox, m_canvasAdpater);
#endif
    dummy* d = new dummy;
    d->a = evas;
    d->b = m_mainBox;
    d->w = width + starFish()->posX();
    d->h = height + starFish()->posY();
    d->objList = &m_objectList;
    d->f = true;
    auto iter = m_objectList.begin();
    while (iter != m_objectList.end()) {
        evas_object_del(*iter);
        iter++;
    }
    m_objectList.clear();
    m_objectList.shrink_to_fit();

    Compositor* c = Compositor::create(starFish(), d);
    delete d;

#if defined(PORT_GRAPHIC_BACKEND_EFL_CAIRO) || \
    defined(PORT_GRAPHIC_BACKEND_EFL_SKIA)
    {
        int w, h;
        evas_object_image_size_get(m_canvasAdpater, &w, &h);
        if (w != 1 || h != 1) {
            evas_object_image_size_set(m_canvasAdpater, 1, 1);
            evas_object_image_fill_set(m_canvasAdpater, 0, 0, 1, 1);
            void* addr = evas_object_image_data_get(m_canvasAdpater, EINA_TRUE);
            memset(addr, 0, 4);
            evas_object_image_data_set(m_canvasAdpater, addr);
            evas_object_image_data_update_add(m_canvasAdpater, 0, 0, 1, 1);
            evas_object_hide(m_canvasAdpater);
        }
    }
#endif
    return c;
#endif
}

void WindowImplEFL::clearResources()
{
    STARFISH_LOG_INFO("WindowImplEFL::clearResources()\n");
    if (m_renderingAnimator) {
        STARFISH_LOG_INFO("Remove animator\n");
        ecore_animator_freeze(m_renderingAnimator);
        ecore_animator_del(m_renderingAnimator);
        m_renderingAnimator = nullptr;
    }

    auto iter = m_objectList.begin();
    while (iter != m_objectList.end()) {
        evas_object_del(*iter);
        iter++;
    }
    m_objectList.clear();
    m_objectList.shrink_to_fit();

    m_objectList.clear();
    m_objectList.shrink_to_fit();
}

} // namespace StarFish
#endif
