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
#ifdef PORT_GRAPHIC_BACKEND_DALI

#include "StarFish.h"

#include "core/animation/Animation.h"
#include "core/dom/MouseEvent.h"
#include "core/dom/KeyboardEvent.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/window/Window.h"

#include <dali-toolkit/dali-toolkit.h>
#if defined(STARFISH_TIZEN_3_0) || defined(STARFISH_TIZEN_OBS)
#include <Ecore.h>
#else
#include <Ecore_X.h>
#endif
#include <Ecore_Input.h>
#include <Ecore_Input_Evas.h>

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

class WindowImplDALI : public Window {
public:
    WindowImplDALI(StarFish* sf)
        : Window(sf)
    {
        m_renderingAnimator = 0;
        m_renderingIdlerData = nullptr;

        GC_REGISTER_FINALIZER_NO_ORDER(
            this,
            [](void* obj, void* cd) {
                STARFISH_LOG_INFO("WindowImplDALI::~WindowImplDALI\n");
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
        Dali::Vector2 size = Dali::Stage::GetCurrent().GetSize();
        return (int32_t)size.width;
    }

    virtual int32_t height() override
    {
#ifdef STARFISH_ENABLE_TEST
        if (getenv("SCREEN_SHOT_HEIGHT") &&
            strlen(getenv("SCREEN_SHOT_HEIGHT"))) {
            return atoi(getenv("SCREEN_SHOT_HEIGHT"));
        }
#endif
        Dali::Vector2 size = Dali::Stage::GetCurrent().GetSize();
        return (int32_t)size.height;
    }

    virtual void resizeTo(int w, int h)
    {
        m_mainView.SetSize(w, h);
    }

    virtual void* unwrap()
    {
        return (void*)m_window_efl;
        // return nullptr;
    }

    virtual void clearResources();
    virtual Canvas* preparePainting(bool forPainting);

    size_t m_renderingAnimator;
    IdlerData* m_renderingIdlerData;
    float m_lastMouseX, m_lastMouseY;
    Dali::BufferImage m_image;
    Dali::Toolkit::ImageView m_mainView;
    // Temp Soluation(should be removed!)
    Evas_Object* m_window_efl;
};

class CanvasSurfaceDALI : public CanvasSurface {
public:
    CanvasSurfaceDALI(Window* wnd, size_t w, size_t h)
    {
        m_width = w;
        m_height = h;
        m_window = (WindowImplDALI*)wnd;

        m_image = (char*)malloc(w * h * sizeof(uint32_t));
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
        // free(m_image);
    }

    virtual void resize(size_t w, size_t h)
    {
        // m_image_view.SetSize(w, h);
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
        size_t end = m_width * m_height * sizeof(uint32_t);
        memset(m_image, 0xff, end);
    }

protected:
    WindowImplDALI* m_window;
    char* m_image;
    size_t m_width;
    size_t m_height;
};

CanvasSurface* CanvasSurface::create(Window* wnd, size_t w, size_t h)
{
    return new CanvasSurfaceDALI(wnd, w, h);
}

// static void mainRenderingFunction(Evas_Object* o, Evas_Object_Box_Data* priv,
//                                   void* user_data)
// {
//     ecore_animator_add(
//         [](void* user_data) -> Eina_Bool {
//             WindowImplDALI* wnd = (WindowImplDALI*)user_data;
//             wnd->setNeedsLayout();
//             return ECORE_CALLBACK_CANCEL;
//         },
//         user_data);
// }

Window* Window::create(StarFish* sf, void* win, int width, int height)
{
    auto wnd = new WindowImplDALI(sf);
    wnd->m_starFish = sf;

    // Temp Code!!
    wnd->m_window_efl = elm_win_add(NULL, "StarFish", ELM_WIN_BASIC);

#ifdef STARFISH_ENABLE_TEST
    {
        const char* path = getenv("SCREEN_SHOT");
        const char* hide = getenv("HIDE_WINDOW");
        if ((path && strlen(path)) || (hide && strlen(hide))) {
            // evas_object_hide(wnd->m_window);
        } else {
            // evas_object_show(wnd->m_window);
        }
    }
#endif

    wnd->m_animationExecutor = new AnimationExecutor(wnd);
    return wnd;
}

Window::~Window()
{
    STARFISH_LOG_INFO("Window::~Window\n");
    if (m_animationExecutor->isAlive()) {
        m_animationExecutor->stopIfNeeds();
    }
}

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

    ((WindowImplDALI*)this)->m_renderingIdlerData = id;
    ((WindowImplDALI*)this)->m_renderingAnimator =
        starFish()->messageLoop()->addIdler(
            [](size_t handle, void* data) {
                IdlerData* id = (IdlerData*)data;
                Window* wnd = (Window*)id->m_data;
                StarFishEnterer enter(wnd->m_starFish);
                id->m_fn(id->m_data);
                ((WindowImplDALI*)wnd)->m_renderingAnimator = 0;
                ((WindowImplDALI*)wnd)->m_renderingIdlerData = nullptr;
                ((WindowImplDALI*)wnd)->m_image.Update();
                GC_FREE(id);
            },
            id);
}

Canvas* WindowImplDALI::preparePainting(bool forPainting)
{
#ifdef STARFISH_ENABLE_TEST
    {
        // const char* path = getenv("SCREEN_SHOT");
        // if (path && strlen(path) && g_fireOnloadEvent) {
        //     g_surfaceForScreehShot =
        //         CanvasSurface::create(this, width(), height());
        //     g_imgBufferForScreehShot =
        //         (Evas_Object*)g_surfaceForScreehShot->unwrap();
        //     return Canvas::create(g_surfaceForScreehShot);
        // }
    }
#endif

    Dali::Actor rootLayer = Dali::Stage::GetCurrent().GetRootLayer();

    for (unsigned int i = 1; i < rootLayer.GetChildCount(); ++i) {
        Dali::Actor child = rootLayer.GetChildAt(i);
        Dali::Stage::GetCurrent().Remove(child);
    }

    int width, height;
    Dali::Vector2 size = Dali::Stage::GetCurrent().GetSize();
    width = size.width;
    height = size.height;

    struct dummy {
        Dali::BufferImage image;
        int w;
        int h;
    };

    dummy* d = new dummy;
    d->w = width;
    d->h = height;

    m_image = Dali::BufferImage::New(width, height, Dali::Pixel::BGRA8888);
    d->image = m_image;
    m_mainView = Dali::Toolkit::ImageView::New(d->image);
    m_mainView.SetParentOrigin(Dali::ParentOrigin::TOP_LEFT);
    m_mainView.SetAnchorPoint(Dali::AnchorPoint::TOP_LEFT);
    m_mainView.SetSize(width, height);
    m_mainView.SetPosition(0, 0);
    Dali::Stage::GetCurrent().Add(m_mainView);

    Canvas* canvas = Canvas::createDirect(d);
    delete d;

    return canvas;
}

void WindowImplDALI::clearResources()
{
    if (m_renderingAnimator) {
        starFish()->messageLoop()->removeIdler(m_renderingAnimator);
        GC_FREE(m_renderingIdlerData);
    }

    clearStackingContext(false);
}
}
#endif
