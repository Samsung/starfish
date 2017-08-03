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
#ifdef PORT_GRAPHIC_BACKEND_GENERAL_BUFFER

#include "StarFish.h"
#include <cairo.h>

#include "core/animation/Animation.h"
#include "core/dom/MouseEvent.h"
#include "core/dom/KeyboardEvent.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/threading/Locker.h"
#include "core/modules/message_loop/MessageLoop.h"

#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "platform/window/PlatformWindow.h"

#ifdef STARFISH_ENABLE_TEST
extern bool g_fireOnloadEvent;
extern unsigned char* g_imgBufferForScreehShot;
extern StarFish::CanvasSurface* g_surfaceForScreehShot;
#endif

// #define STARFISH_ENABLE_TIMER
namespace StarFish {

struct IdlerData {
    void (*m_fn)(void*);
    void* m_data;
};

class WindowImplGB : public PlatformWindow {
public:
    WindowImplGB(StarFish* sf, int32_t width, int32_t height)
        : PlatformWindow(sf)
        , m_width(width)
        , m_height(height)
        , m_internalBuffer(nullptr)
        , m_rendingLockMutex(new Mutex())
    {
        m_renderingAnimator = 0;
        m_renderingIdlerData = nullptr;
        initBuffer();

        GC_REGISTER_FINALIZER_NO_ORDER(this,
                                       [](void* obj, void* cd) {
                                           STARFISH_LOG_INFO(
                                               "WindowImplGB::~WindowImplGB\n");
                                           WindowImplGB* s = (WindowImplGB*)obj;
#if !defined(STARFISH_TIZEN)
                                           free(s->m_internalBuffer);
#endif
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
        return m_width;
    }

    virtual int32_t height() override
    {
#ifdef STARFISH_ENABLE_TEST
        if (getenv("SCREEN_SHOT_HEIGHT") &&
            strlen(getenv("SCREEN_SHOT_HEIGHT"))) {
            return atoi(getenv("SCREEN_SHOT_HEIGHT"));
        }
#endif
        return m_height;
    }

    virtual void resizeTo(int w, int h)
    {
        // TODO
    }

    virtual void* unwrap()
    {
        // return getCompletedBuffer();
        return nullptr;
    }

    void initBuffer()
    {
#if defined(STARFISH_TIZEN)
        m_internalBuffer = m_starFish->frameBuffer();
#else
        if (m_internalBuffer == nullptr)
            m_internalBuffer =
                (void*)malloc(m_width * m_height * sizeof(uint32_t));
#endif
        m_stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, m_width);
    }

    void flushBuffer()
    {
#if !defined(STARFISH_TIZEN)
        memcpy(m_starFish->frameBuffer(), m_internalBuffer,
               m_width * m_height * sizeof(uint32_t));

#endif
        m_starFish->setNeedsUpdate();
    }

    virtual void clearResources();
    virtual Canvas* preparePainting(bool forPainting);

    int32_t m_width;
    int32_t m_height;
    size_t m_renderingAnimator;
    IdlerData* m_renderingIdlerData;
    float m_lastMouseX, m_lastMouseY;

    void* m_internalBuffer;
    size_t m_stride;
    Mutex* m_rendingLockMutex;
};

class CanvasSurfaceDALI : public CanvasSurface {
public:
    CanvasSurfaceDALI(PlatformWindow* wnd, size_t w, size_t h)
    {
        m_width = w;
        m_height = h;
        m_window = (WindowImplGB*)wnd;

        buffer = (unsigned char*)malloc(w * h * sizeof(uint32_t));
        size_t end = m_width * m_height * sizeof(uint32_t);
        memset(buffer, 0x00, end);
        GC_REGISTER_FINALIZER_NO_ORDER(this,
                                       [](void* obj, void* cd) {
                                           CanvasSurfaceDALI* s =
                                               (CanvasSurfaceDALI*)obj;
                                           // STARFISH_LOG_INFO("release
                                           // CanvasSurfaceDALI %p\n", s);
                                           s->detachNativeBuffer();
                                       },
                                       NULL, NULL, NULL);
    }

    virtual void detachNativeBuffer()
    {
        free(buffer);
        buffer = nullptr;
    }

    virtual void resize(size_t w, size_t h)
    {
    }

    virtual void* unwrap()
    {
        return (void*)buffer;
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
        size_t end = m_width * m_height * sizeof(uint32_t);
        memset(buffer, 0x00, end);
    }

protected:
    WindowImplGB* m_window;
    unsigned char* buffer;
    size_t m_width;
    size_t m_height;
};

CanvasSurface* CanvasSurface::create(PlatformWindow* wnd, size_t w, size_t h)
{
    return new CanvasSurfaceDALI(wnd, w, h);
}

// static void mainRenderingFunction(Evas_Object* o, Evas_Object_Box_Data* priv,
//                                   void* user_data)
// {
//     ecore_animator_add(
//         [](void* user_data) -> Eina_Bool {
//             WindowImplGB* wnd = (WindowImplGB*)user_data;
//             wnd->setNeedsLayout();
//             wnd->webView()->mainBrowsingContext()->setNeedsLayout();
//             return ECORE_CALLBACK_CANCEL;
//         },
//         user_data);
// }

PlatformWindow* PlatformWindow::create(StarFish* sf, void* win, int width,
                                       int height)
{
    auto wnd = new WindowImplGB(sf, width, height);
    wnd->m_starFish = sf;

#ifdef STARFISH_ENABLE_TEST
    {
        const char* path = getenv("SCREEN_SHOT");
        const char* hide = getenv("HIDE_WINDOW");

        // AFAIK ,There is no way to hide the window in DALi@linux.(mh.byun)
        /*
        {
            Dali::Application* app = (Dali::Application*)sf->nativeHandle();
            if(app){
                Dali::Window dali_win  = app->GetWindow();
                if ((path && strlen(path)) || (hide && strlen(hide))) {
                    Dali::DevelWindow::Hide(dali_win);
                    // wnd->m_mainView.SetVisible(false);
                } else {
                    Dali::DevelWindow::Show(dali_win);
                    // wnd->m_mainView.SetVisible(true);
                }
            }
        }
        */
    }
#endif

    return wnd;
}

PlatformWindow::~PlatformWindow()
{
    STARFISH_LOG_INFO("PlatformWindow::~PlatformWindow\n");
}

void WebView::setNeedsRendering()
{
    WindowImplGB* wnd = (WindowImplGB*)starFish()->platformWindow();

    // TODO: refresh rendering animator here.

    m_needsRendering = true;

    IdlerData* id = new (NoGC) IdlerData;
    id->m_fn = [](void* data) -> void {
        PlatformWindow* wnd = (PlatformWindow*)data;
        wnd->rendering();
    };
    id->m_data = starFish()->platformWindow();

    wnd->m_renderingIdlerData = id;
    wnd->m_renderingAnimator = starFish()->messageLoop()->addIdler(
        mainBrowsingContext(),
        [](size_t handle, void* data) {
            IdlerData* id = (IdlerData*)data;
            PlatformWindow* wnd = (PlatformWindow*)id->m_data;
            StarFishEnterer enter(wnd->starFish());
            {
                Locker<Mutex> l(*((WindowImplGB*)wnd)->m_rendingLockMutex);
                ((WindowImplGB*)wnd)->initBuffer();
                id->m_fn(id->m_data);
                ((WindowImplGB*)wnd)->m_renderingAnimator = 0;
                ((WindowImplGB*)wnd)->m_renderingIdlerData = nullptr;
                ((WindowImplGB*)wnd)->flushBuffer();
            }
            GC_FREE(id);
        },
        id);
}

Canvas* WindowImplGB::preparePainting(bool forPainting)
{
#ifdef STARFISH_ENABLE_TEST
    {
        const char* path = getenv("SCREEN_SHOT");
        if (path && strlen(path) && g_fireOnloadEvent) {
            g_surfaceForScreehShot =
                CanvasSurface::create(this, width(), height());
            g_imgBufferForScreehShot =
                (unsigned char*)g_surfaceForScreehShot->unwrap();
            return Canvas::create(g_surfaceForScreehShot);
        }
    }
#endif

    struct dummy {
        void* image;
        int w;
        int h;
        int stride;
    };

    dummy* d = new dummy;
    d->w = m_width;
    d->h = m_height;
    d->image = m_internalBuffer;
    d->stride = m_stride;
    Canvas* canvas = Canvas::createDirect(d);
    delete d;

    return canvas;
}

void WindowImplGB::clearResources()
{
    if (m_renderingAnimator) {
        starFish()->messageLoop()->removeIdler(m_renderingAnimator);
        m_renderingAnimator = 0;
        GC_FREE(m_renderingIdlerData);
    }

    webView()->clearStackingContext(false);
}
}
#endif
