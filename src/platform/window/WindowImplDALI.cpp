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
#include <dali-toolkit/dali-toolkit.h>
// #include <dali/devel-api/adaptor-framework/window-devel.h>

#include "core/animation/Animation.h"
#include "core/dom/MouseEvent.h"
#include "core/dom/KeyboardEvent.h"
#include "core/modules/canvas/Canvas.h"
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

class EventController : public Dali::ConnectionTracker, public gc {
public:
    EventController(StarFish* sf)
    {
        m_sf = sf;
    }
    bool TouchEventHandler(Dali::Actor actor, const Dali::TouchData& data)
    {
        size_t pointCount = data.GetPointCount();
        if (pointCount == 1) {
            // Single touch event

            Dali::PointState::Type pointState = data.GetState(0);
            if (pointState == Dali::PointState::DOWN) {
                StarFishEnterer enter(m_sf);
                const Dali::Vector2& screen = data.GetScreenPosition(0);
                MouseData data(screen.x, screen.y);
                m_sf->platformWindow()->dispatchMouseEvent(
                    PlatformWindow::MouseEventDown, data);
            } else if (pointState == Dali::PointState::UP) {
                StarFishEnterer enter(m_sf);
                const Dali::Vector2& screen = data.GetScreenPosition(0);
                MouseData data(screen.x, screen.y);
                m_sf->platformWindow()->dispatchMouseEvent(
                    PlatformWindow::MouseEventUp, data);
            }
        }
        return true;
    }
    bool HoverEventHandler(Dali::Actor actor, const Dali::HoverEvent& event)
    {
        const Dali::Vector2& point = event.GetPoint(0).screen;
        StarFishEnterer enter(m_sf);
        MouseData data(point.x, point.y);
        m_sf->platformWindow()->dispatchMouseEvent(
            PlatformWindow::MouseEventMove, data);
        return true;
    }

protected:
    StarFish* m_sf;
};

class WindowImplDALI : public PlatformWindow {
public:
    WindowImplDALI(StarFish* sf)
        : PlatformWindow(sf)
    {
        m_renderingAnimator = 0;
        m_renderingIdlerData = nullptr;

        Dali::Vector2 size = Dali::Stage::GetCurrent().GetSize();
        m_daliBuffer = Dali::BufferImage::New(size.width, size.height,
                                              Dali::Pixel::BGRA8888);
        m_mainView = Dali::Toolkit::ImageView::New(m_daliBuffer);
        m_mainView.SetParentOrigin(Dali::ParentOrigin::TOP_LEFT);
        m_mainView.SetAnchorPoint(Dali::AnchorPoint::TOP_LEFT);
        m_mainView.SetPosition(0, 0);
        Dali::Stage::GetCurrent().Add(m_mainView);
        m_eventController = new EventController(sf);
        Dali::Stage::GetCurrent().GetRootLayer().TouchSignal().Connect(
            m_eventController, &EventController::TouchEventHandler);

        Dali::Stage::GetCurrent().GetRootLayer().HoveredSignal().Connect(
            m_eventController, &EventController::HoverEventHandler);

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
        return nullptr;
    }

    virtual void clearResources();
    virtual Canvas* preparePainting(bool forPainting);

    size_t m_renderingAnimator;
    IdlerData* m_renderingIdlerData;
    float m_lastMouseX, m_lastMouseY;
    Dali::BufferImage m_daliBuffer;
    Dali::Toolkit::ImageView m_mainView;
    EventController* m_eventController;
};

class CanvasSurfaceDALI : public CanvasSurface {
public:
    CanvasSurfaceDALI(PlatformWindow* wnd, size_t w, size_t h)
    {
        m_width = w;
        m_height = h;
        m_window = (WindowImplDALI*)wnd;

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
    WindowImplDALI* m_window;
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
//             WindowImplDALI* wnd = (WindowImplDALI*)user_data;
//             wnd->setNeedsLayout();
//             wnd->webView()->mainBrowsingContext()->setNeedsLayout();
//             return ECORE_CALLBACK_CANCEL;
//         },
//         user_data);
// }

PlatformWindow* PlatformWindow::create(StarFish* sf, void* win, int width,
                                       int height)
{
    auto wnd = new WindowImplDALI(sf);
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

    ((WindowImplDALI*)id->m_data)->m_renderingIdlerData = id;
    ((WindowImplDALI*)id->m_data)->m_renderingAnimator =
        starFish()->messageLoop()->addIdler(
            mainBrowsingContext(),
            [](size_t handle, void* data) {
                IdlerData* id = (IdlerData*)data;
                PlatformWindow* wnd = (PlatformWindow*)id->m_data;
                StarFishEnterer enter(wnd->starFish());
                id->m_fn(id->m_data);
                ((WindowImplDALI*)wnd)->m_renderingAnimator = 0;
                ((WindowImplDALI*)wnd)->m_renderingIdlerData = nullptr;
                ((WindowImplDALI*)wnd)->m_daliBuffer.Update();
                GC_FREE(id);
            },
            id);
}

Canvas* WindowImplDALI::preparePainting(bool forPainting)
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
    d->image = m_daliBuffer;
    m_mainView.SetSize(width, height);

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

    webView()->clearStackingContext(false);
}
}
#endif
