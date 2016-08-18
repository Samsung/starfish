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
#endif


namespace StarFish {

namespace {
    class __GET_TICK_COUNT {
    public:
        __GET_TICK_COUNT()
        {
            if (gettimeofday(&tv_, NULL) != 0)
                throw 0;
        }
        timeval tv_;
    };
    __GET_TICK_COUNT timeStart;
}

struct IdlerData {
    void (*m_fn)(void*);
    void* m_data;
};

class WindowImplEFL : public Window {
public:
    WindowImplEFL(StarFish* sf, URL* url)
        : Window(sf, url)
    {
        m_mainBox = nullptr;
        m_dummyBox = nullptr;
        m_isActive = true;
        m_renderingAnimator = nullptr;
        m_renderingIdlerData = nullptr;

        GC_REGISTER_FINALIZER_NO_ORDER(this, [] (void* obj, void* cd) {
            STARFISH_LOG_INFO("WindowImplEFL::~WindowImplEFL\n");
        }, NULL, NULL, NULL);
    }

    virtual int width()
    {
#ifdef STARFISH_ENABLE_TEST
        if (getenv("SCREEN_SHOT_WIDTH") && strlen(getenv("SCREEN_SHOT_WIDTH"))) {
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
        if (getenv("SCREEN_SHOT_HEIGHT") && strlen(getenv("SCREEN_SHOT_HEIGHT"))) {
            return atoi(getenv("SCREEN_SHOT_HEIGHT"));
        }
#endif
        WindowImplEFL* eflWindow = (WindowImplEFL*)this;
        int height;
        evas_object_geometry_get(eflWindow->m_window, NULL, NULL, NULL, &height);
        return height;
    }

    virtual void resizeTo(int w, int h)
    {
        evas_object_resize(m_window, w, h);
    }

    void clearEFLResources()
    {
        if (m_document && m_document->frame() && m_document->frame()->firstChild() && m_document->frame()->firstChild()->asFrameBox()->stackingContext()) {
            StackingContext* ctx = m_document->frame()->firstChild()->asFrameBox()->stackingContext();
            std::function<void(StackingContext*)> clearSC = [&clearSC](StackingContext* ctx)
            {
                ctx->clearOwnBuffer();
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
        }

        WindowImplEFL* eflWindow = (WindowImplEFL*)this;
        auto a = eflWindow->m_drawnImageList.begin();
        while (a != eflWindow->m_drawnImageList.end()) {
            std::vector<std::pair<Evas_Object*, bool> > & vec = a->second;

            for (size_t i = 0; i < vec.size(); i ++) {
                Evas_Object* obj = vec[i].first;
                evas_object_del(obj);
            }
            a++;
        }
        eflWindow->m_drawnImageList.clear();
    }

    bool m_isActive;
    uintptr_t m_handle;
    Evas_Object* m_window;
    Evas_Object* m_background;
    Evas_Object* m_canvasAdpater;
    std::vector<Evas_Object*> m_objectList;
    std::vector<Evas_Object*> m_surfaceList;
    std::unordered_map<ImageData*, std::vector<std::pair<Evas_Object*, bool> > > m_drawnImageList;
    Evas_Object* m_mainBox;
    Evas_Object* m_dummyBox;

    Ecore_Event_Handler* m_desktopMouseDownEventHandler;
    Ecore_Event_Handler* m_desktopMouseMoveEventHandler;
    Ecore_Event_Handler* m_desktopMouseUpEventHandler;
    Ecore_Event_Handler* m_desktopKeyDownEventHandler;
    Ecore_Event_Handler* m_desktopKeyUpEventHandler;

    void (*m_mobileMouseDownEventHandler)(void* data, Evas* evas, Evas_Object* obj, void* event_info);
    void (*m_mobileMouseMoveEventHandler)(void* data, Evas* evas, Evas_Object* obj, void* event_info);
    void (*m_mobileMouseUpEventHandler)(void* data, Evas* evas, Evas_Object* obj, void* event_info);

    Ecore_Animator* m_renderingAnimator;
    IdlerData* m_renderingIdlerData;
};

class CanvasSurfaceEFL : public CanvasSurface {
public:
    CanvasSurfaceEFL(Window* wnd, size_t w, size_t h)
    {
        m_window = (WindowImplEFL*)wnd;
        m_image = evas_object_image_add(evas_object_evas_get(m_window->m_window));
        evas_object_image_size_set(m_image, w, h);
        evas_object_image_filled_set(m_image, EINA_TRUE);
        evas_object_image_colorspace_set(m_image, Evas_Colorspace::EVAS_COLORSPACE_ARGB8888);
        evas_object_image_alpha_set(m_image, EINA_TRUE);
        evas_object_anti_alias_set(m_image, EINA_TRUE);
        STARFISH_RELEASE_ASSERT(evas_object_image_colorspace_get(m_image) == EVAS_COLORSPACE_ARGB8888);
        m_width = w;
        m_height = h;
        STARFISH_LOG_INFO("create CanvasSurfaceEFL %p %p\n", this, m_image);

        STARFISH_ASSERT(evas_object_visible_get(m_image) == EINA_FALSE);
        /*
        GC_REGISTER_FINALIZER_NO_ORDER(this, [] (void* obj, void* cd) {
            CanvasSurfaceEFL* s = (CanvasSurfaceEFL*)obj;
            STARFISH_LOG_INFO("release CanvasSurfaceEFL %p\n", s);
            s->detachNative((Evas_Object*)cd);
        }, m_image, NULL, NULL);
        */
    }

    void detachNative(Evas_Object* image)
    {
        if (!image)
            return;
        CanvasSurfaceEFL* s = (CanvasSurfaceEFL*)this;
        STARFISH_LOG_INFO("detach CanvasSurfaceEFL NativeBuffer %p\n", image);
        // evas_object_image_size_set(image, 0, 0);
        evas_object_hide(image);
        STARFISH_RELEASE_ASSERT(evas_object_ref_get(image) == 0);
        evas_object_del(image);
/*
        auto iter = std::find(s->m_window->m_surfaceList.begin(), s->m_window->m_surfaceList.end(), s->m_image);
        if (s->m_window->m_surfaceList.end() != iter)
            s->m_window->m_surfaceList.erase(iter);
*/
    }

    virtual void detachNativeBuffer()
    {
        detachNative(m_image);
        m_image = nullptr;
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
        memset(address, 0, end);
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

void mainRenderingFunction(Evas_Object* o, Evas_Object_Box_Data* priv, void* user_data)
{
    WindowImplEFL* wnd = (WindowImplEFL*)user_data;
    if (!wnd->inRendering())
        wnd->setNeedsLayout();
}


Window* Window::create(StarFish* sf, void* win, int width, int height, URL* url)
{
    auto wnd = new WindowImplEFL(sf, url);
    wnd->m_starFish = sf;
    wnd->m_window = (Evas_Object*)win;

#ifndef STARFISH_TIZEN_WEARABLE_LIB
    Evas* e = evas_object_evas_get(wnd->m_window);
    Ecore_Evas* ee = ecore_evas_ecore_evas_get(e);
    Ecore_Window ew = ecore_evas_window_get(ee);
    wnd->m_handle = (uintptr_t)ew;

    wnd->m_mainBox = elm_box_add(wnd->m_window);
    evas_object_size_hint_weight_set(wnd->m_mainBox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(wnd->m_window, wnd->m_mainBox);
    elm_box_layout_set(wnd->m_mainBox, mainRenderingFunction, wnd, NULL);
    evas_object_show(wnd->m_mainBox);
#ifdef STARFISH_ENABLE_TEST
    {
        const char* path = getenv("SCREEN_SHOT");
        const char* hide = getenv("HIDE_WINDOW");
        if ((path && strlen(path)) || (hide && strlen(hide)))
            evas_object_hide(wnd->m_window);
        else
            evas_object_show(wnd->m_window);
    }
#else
    evas_object_show(wnd->m_window);
#endif
    /*
    evas_event_callback_add(e, EVAS_CALLBACK_RENDER_FLUSH_POST, [](void *data, Evas *e, void *event_info) {
    }, wnd);
*/

    wnd->m_desktopMouseDownEventHandler = ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN, [](void* data, int type, void* event) -> Eina_Bool {
        Window* sf = (Window*)data;
        Ecore_Event_Mouse_Button* d = (Ecore_Event_Mouse_Button*)event;
        ScriptBindingInstanceEnterer enter(sf->starFish()->scriptBindingInstance());
        sf->dispatchTouchEvent(d->x, d->y, Window::TouchEventDown);
        return EINA_TRUE;
    }, wnd);

    wnd->m_desktopMouseUpEventHandler = ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP, [](void* data, int type, void* event) -> Eina_Bool {
        Window* sf = (Window*)data;
        Ecore_Event_Mouse_Button* d = (Ecore_Event_Mouse_Button*)event;
        ScriptBindingInstanceEnterer enter(sf->starFish()->scriptBindingInstance());
        sf->dispatchTouchEvent(d->x, d->y, Window::TouchEventUp);
        return EINA_TRUE;
    }, wnd);

    wnd->m_desktopMouseMoveEventHandler = ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE, [](void* data, int type, void* event) -> Eina_Bool {
        Window* sf = (Window*)data;
        Ecore_Event_Mouse_Move* d = (Ecore_Event_Mouse_Move*)event;
        ScriptBindingInstanceEnterer enter(sf->starFish()->scriptBindingInstance());
        sf->dispatchTouchEvent(d->x, d->y, Window::TouchEventMove);
        return EINA_TRUE;
    }, wnd);

    wnd->m_desktopKeyDownEventHandler = ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, [](void* data, int type, void* event) -> Eina_Bool {
        Window* sf = (Window*)data;
        Ecore_Event_Key* d = (Ecore_Event_Key*)event;
        ScriptBindingInstanceEnterer enter(sf->starFish()->scriptBindingInstance());
        sf->dispatchKeyEvent(String::createASCIIString(d->keyname), Window::KeyEventDown);
        return EINA_TRUE;
    }, wnd);

    wnd->m_desktopKeyUpEventHandler = ecore_event_handler_add(ECORE_EVENT_KEY_UP, [](void* data, int type, void* event) -> Eina_Bool {
        Window* sf = (Window*)data;
        Ecore_Event_Key* d = (Ecore_Event_Key*)event;
        ScriptBindingInstanceEnterer enter(sf->starFish()->scriptBindingInstance());
        sf->dispatchKeyEvent(String::createASCIIString(d->keyname), Window::KeyEventUp);
        return EINA_TRUE;
    }, wnd);

#else
    Evas* e = evas_object_evas_get(wnd->m_window);
    wnd->m_mainBox = elm_box_add(wnd->m_window);
    evas_object_size_hint_weight_set(wnd->m_mainBox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(wnd->m_window, wnd->m_mainBox);
    elm_box_layout_set(wnd->m_mainBox, mainRenderingFunction, wnd, NULL);
    evas_object_show(wnd->m_mainBox);

    wnd->m_dummyBox = evas_object_rectangle_add(e);
    evas_object_color_set(wnd->m_dummyBox, 0, 0, 0, 0); // opaque background
    int w, h;
    evas_object_geometry_get(wnd->m_window, &w, &h, NULL, NULL);
    evas_object_resize(wnd->m_dummyBox, width, height);
    evas_object_move(wnd->m_dummyBox, 0, 0);
    evas_object_show(wnd->m_dummyBox);
    evas_object_show(wnd->m_window);

    wnd->m_mobileMouseDownEventHandler = [](void* data, Evas* evas, Evas_Object* obj, void* event_info) -> void {
        Window* sf = (Window*)data;
        Evas_Event_Mouse_Down* ev = (Evas_Event_Mouse_Down*) event_info;
        ScriptBindingInstanceEnterer enter(sf->starFish()->scriptBindingInstance());
        sf->dispatchTouchEvent(ev->canvas.x, ev->canvas.y, Window::TouchEventDown);
        return;
    };
    evas_object_event_callback_add(wnd->m_dummyBox, EVAS_CALLBACK_MOUSE_DOWN, wnd->m_mobileMouseDownEventHandler, wnd);

    wnd->m_mobileMouseMoveEventHandler = [](void* data, Evas* evas, Evas_Object* obj, void* event_info) -> void {
        Window* sf = (Window*)data;
        Evas_Event_Mouse_Move* ev = (Evas_Event_Mouse_Move*) event_info;
        ScriptBindingInstanceEnterer enter(sf->starFish()->scriptBindingInstance());
        sf->dispatchTouchEvent(ev->cur.canvas.x, ev->cur.canvas.y, Window::TouchEventMove);
        return;
    };
    evas_object_event_callback_add(wnd->m_dummyBox, EVAS_CALLBACK_MOUSE_MOVE, wnd->m_mobileMouseMoveEventHandler, wnd);

    wnd->m_mobileMouseMoveEventHandler = [](void* data, Evas* evas, Evas_Object* obj, void* event_info) -> void {
        Window* sf = (Window*)data;
        Evas_Event_Mouse_Up* ev = (Evas_Event_Mouse_Up*) event_info;
        ScriptBindingInstanceEnterer enter(sf->starFish()->scriptBindingInstance());
        sf->dispatchTouchEvent(ev->canvas.x, ev->canvas.y, Window::TouchEventUp);
        return;
    };
    evas_object_event_callback_add(wnd->m_dummyBox, EVAS_CALLBACK_MOUSE_UP, wnd->m_mobileMouseMoveEventHandler, wnd);
#endif
    return wnd;
}

Window::Window(StarFish* starFish, URL* url)
    : m_starFish(starFish)
    , m_document(nullptr)
    , m_touchDownPoint(0, 0)
{
    initScriptWrappable(this);
    STARFISH_ASSERT(m_starFish->scriptBindingInstance());
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
    m_activeNodeWithTouchDown = nullptr;

    m_document = new HTMLDocument(this, starFish->scriptBindingInstance(), url, String::createASCIIString("UTF-8"));
}

// #define STARFISH_ENABLE_TIMER

#ifdef STARFISH_ENABLE_TIMER
static unsigned long getLongTickCount()
{
    static time_t secStart = timeStart.tv_.tv_sec;
    static time_t usecStart = timeStart.tv_.tv_usec;
    timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec - secStart) * 1000 + (tv.tv_usec - usecStart);
}
#endif

class Timer {
public:
    Timer(const char* msg)
    {
#ifdef STARFISH_ENABLE_TIMER
        m_start = getLongTickCount();
        m_msg = msg;
#endif
    }
    ~Timer()
    {
#ifdef STARFISH_ENABLE_TIMER
        unsigned long end = getLongTickCount();
        STARFISH_LOG_INFO("did %s in %f ms\n", m_msg, (end - m_start) / 1000.f);
        fflush(stdout);
#endif
    }

protected:
#ifdef STARFISH_ENABLE_TIMER
    unsigned long m_start;
    const char* m_msg;
#endif
};

#ifdef STARFISH_ENABLE_TEST
static Evas_Object* g_imgBufferForScreehShot;
static CanvasSurface* g_surfaceForScreehShot;
#endif

Canvas* preparePainting(WindowImplEFL* eflWindow)
{
#ifdef STARFISH_ENABLE_TEST
    {
        const char* path = getenv("SCREEN_SHOT");
        if (path && strlen(path) && g_fireOnloadEvent) {
            g_surfaceForScreehShot = CanvasSurface::create(eflWindow, eflWindow->width(), eflWindow->height());
            g_imgBufferForScreehShot = (Evas_Object*)g_surfaceForScreehShot->unwrap();
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
    d->b = &eflWindow->m_drawnImageList;
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
    canvas->clearColor(Color(255, 255, 255, 255));

    if (m_hasRootElementBackground || m_hasBodyElementBackground) {
        WindowImplEFL* eflWindow = (WindowImplEFL*)this;
        int width, height;
        evas_object_geometry_get(eflWindow->m_window, NULL, NULL, &width, &height);
        LayoutRect colorRect(0, 0, width, height);
        if (m_hasRootElementBackground) {
            FrameBox* rootRect = document()->rootElement()->frame()->asFrameBox();
            LayoutLocation rootRectPos = rootRect->absolutePoint(document()->frame()->asFrameBox());
            LayoutRect imgRect(rootRectPos.x() + rootRect->borderLeft(), rootRectPos.y() + rootRect->borderTop(), rootRect->width() - rootRect->borderWidth(), rootRect->height() - rootRect->borderHeight());

            FrameBox::paintBackground(canvas, document()->rootElement()->style(), imgRect, colorRect, true);
        } else {
            LayoutRect imgRect(0, 0, width, height);
            if (document()->rootElement()->body()->frame()) {
                FrameBox* bodyRect = document()->rootElement()->body()->frame()->asFrameBox();
                imgRect.setHeight(bodyRect->height() + bodyRect->marginHeight());
            }

            FrameBox::paintBackground(canvas, document()->rootElement()->body()->style(), imgRect, colorRect, true);
        }

    }
}



void Window::rendering()
{
    if (!m_needsRendering)
        return;
    m_inRendering = true;

    WindowImplEFL* eflWindow = (WindowImplEFL*)this;
    STARFISH_RELEASE_ASSERT(eflWindow->m_isActive);

    Timer renderingTimer("Window::rendering");

    if (m_needsStyleRecalc || m_needsStyleRecalcForWholeDocument) {
        if (m_needsStyleRecalcForWholeDocument) {
            for (size_t i = 0; i < document()->styleResolver()->sheets().size(); i ++) {
                document()->styleResolver()->sheets()[i]->parseSheetIfneeds();
            }
        }

        // resolve style
        Timer t("resolve style");
        document()->styleResolver()->resolveDOMStyle(m_document, m_needsStyleRecalcForWholeDocument);
        m_needsStyleRecalc = false;
        m_needsStyleRecalcForWholeDocument = false;

#ifdef STARFISH_ENABLE_TEST
        if (m_starFish->startUpFlag() & StarFishStartUpFlag::enableComputedStyleDump) {
            // dump style
            document()->styleResolver()->dumpDOMStyle(m_document);
        }
#endif
    }

    if (m_needsFrameTreeBuild) {

        if (m_document->frame()) {
            clearStackingContext();

            // create frame tree
            Timer t("create frame tree");
            FrameTreeBuilder::buildFrameTree(m_document);
            m_needsFrameTreeBuild = false;
        }
    }

    if (m_needsLayout) {
        // lay out frame tree
        Timer t("lay out frame tree");

        clearStackingContext();

        LayoutContext ctx(starFish(), m_document->frame()->asFrameBox()->asFrameBlockBox()->asFrameDocument());
        m_document->frame()->layout(ctx, Frame::LayoutWantToResolve::ResolveAll);

#ifndef NDEBUG
        {
            LayoutContext ctx(starFish(), m_document->frame()->asFrameBox()->asFrameBlockBox()->asFrameDocument());
            m_document->frame()->layout(ctx, Frame::LayoutWantToResolve::ResolveAll);
        }
#endif
        {
            Timer t("computeStackingContextProperties");
            m_document->frame()->asFrameBox()->iterateChildBoxes([](FrameBox* box) -> bool
            {
                box->establishesStackingContextIfNeeds();
                return true;
            }, nullptr, nullptr);
            if (m_document->frame()->firstChild())
                m_document->frame()->firstChild()->asFrameBox()->stackingContext()->computeStackingContextProperties();
        }
        m_needsLayout = false;
#ifdef STARFISH_ENABLE_TEST
        if (m_starFish->startUpFlag() & StarFishStartUpFlag::enableFrameTreeDump) {
            FrameTreeBuilder::dumpFrameTree(m_document);
        }
#endif
    }

    if (m_needsPainting) {
        Timer t("painting");

        // painting
        Canvas* canvas = preparePainting(eflWindow);

        paintWindowBackground(canvas);
        {
            PaintingContext ctx(canvas);
            ctx.m_paintingStage = PaintingStageEnd;
            m_document->frame()->paint(ctx);
        }
        m_needsPainting = false;
        if (m_document->frame()->firstChild())
            m_needsComposite = m_document->frame()->firstChild()->asFrameBox()->stackingContext()->needsOwnBuffer();
        else
            m_needsComposite = false;

        delete canvas;
#ifdef STARFISH_TIZEN_WEARABLE
        evas_object_raise(eflWindow->m_dummyBox);
#endif

#ifdef STARFISH_ENABLE_TEST
        if (m_starFish->startUpFlag() & StarFishStartUpFlag::enableStackingContextDump) {
            if (m_document->frame()->firstChild()) {
                STARFISH_ASSERT(m_document->frame()->firstChild()->asFrameBox()->isRootElement());
                StackingContext* ctx = m_document->frame()->firstChild()->asFrameBox()->stackingContext();

                std::function<void(StackingContext*, int)> dumpSC = [&dumpSC](StackingContext* ctx, int depth)
                {
                    for (int i = 0; i < depth; i ++) {
                        printf("  ");
                    }

                    auto fr = ctx->visibleRect();


                    std::string className;
                    for (unsigned i = 0; i < ctx->owner()->node()->asElement()->asHTMLElement()->classNames().size(); i++) {
                        className += ctx->owner()->node()->asElement()->asHTMLElement()->classNames()[i]->utf8Data();
                        className += " ";
                    }

                    printf("StackingContext[%p, node %p %s id:%s className:%s , frame %p, buf %p %d %d %d %d]\n",
                        ctx, ctx->owner()->node(), ctx->owner()->node()->localName()->utf8Data(), ctx->owner()->node()->asElement()->asHTMLElement()->id()->utf8Data(), className.data(), ctx->owner(), ctx->buffer()
                        , (int)fr.x(), (int)fr.y(), (int)fr.width(), (int)fr.height());

                    auto iter = ctx->childContexts().begin();
                    while (iter != ctx->childContexts().end()) {

                        int32_t num = iter->first;

                        for (int i = 0; i < depth + 1; i ++) {
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
        Timer t("composite");
        if (m_document->frame()->firstChild() && m_document->frame()->firstChild()->asFrameBox()->stackingContext()->needsOwnBuffer()) {
            Canvas* canvas = preparePainting(eflWindow);
            paintWindowBackground(canvas);
            m_document->frame()->firstChild()->asFrameBox()->stackingContext()->compositeStackingContext(canvas);

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
            // int writeImage(char* filename, int width, int height, void *buffer)
            // writeImage(path, width(), height(), evas_object_image_data_get(g_imgBufferForScreehShot, EINA_FALSE));
            if (getenv("EXIT_AFTER_SCREEN_SHOT") && strlen(getenv("EXIT_AFTER_SCREEN_SHOT")))
                exit(0);

            delete g_surfaceForScreehShot;
            g_surfaceForScreehShot = nullptr;
        }
    }

#endif
}

void Window::clearStackingContext()
{
    if (m_document->frame()) {
        StackingContext* root = nullptr;
        HTMLHtmlElement* rootElement = document()->rootElement();
        m_document->frame()->asFrameBox()->iterateChildBoxes([&](FrameBox* box) -> bool {
            root = box->stackingContext();
            if (box->node() == rootElement) {
                return false;
            }
            return root == nullptr;
        });

        if (root) {
            std::function<void(StackingContext*, int)> clearSC = [&clearSC](StackingContext* ctx, int depth)
            {
                auto iter = ctx->childContexts().begin();
                while (iter != ctx->childContexts().end()) {
                    int32_t num = iter->first;
                    auto iter2 = iter->second->begin();
                    while (iter2 != iter->second->end()) {
                        clearSC(*iter2, depth + 2);
                        iter2++;
                    }
                    iter++;
                }
                ctx->owner()->clearStackingContextIfNeeds();
            };

            clearSC(root, 0);
        }
    }
}

#ifdef STARFISH_ENABLE_TEST
void Window::setNetworkState(bool state)
{
    int sockfd;
    struct ifreq ifr;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0)
        return;

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
    setNeedsPainting();
    setenv("SCREEN_SHOT", filePath.data(), 1);
    rendering();
    setenv("SCREEN_SHOT", "", 1);

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

    IdlerData* id = new(NoGC) IdlerData;
    id->m_fn = [](void* data) -> void {
        Window* wnd = (Window*) data;
        wnd->rendering();
    };
    id->m_data = this;
    ((WindowImplEFL*)this)->m_renderingIdlerData = id;
    ((WindowImplEFL*)this)->m_renderingAnimator = ecore_animator_add([](void* data) -> Eina_Bool {
        IdlerData* id = (IdlerData*)data;
        Window* wnd = (Window*)id->m_data;
        ScriptBindingInstanceEnterer enter(wnd->starFish()->scriptBindingInstance());
        id->m_fn(id->m_data);
        ((WindowImplEFL*)wnd)->m_renderingAnimator = nullptr;
        ((WindowImplEFL*)wnd)->m_renderingIdlerData = nullptr;
        GC_FREE(id);
        return ECORE_CALLBACK_CANCEL;
    }, id);
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

uint32_t Window::setTimeout(WindowSetTimeoutHandler handler, uint32_t delay, void* data)
{
    WindowImplEFL* eflWindow = (WindowImplEFL*)this;
    STARFISH_RELEASE_ASSERT(eflWindow->m_isActive);

    TimeoutData* td = new(NoGC) TimeoutData;
    td->m_window = this;
    uint32_t id = ++m_timeoutCounter;
    td->m_id = id;
    td->m_data = data;
    td->m_handler = handler;
    td->m_timerID = ecore_timer_add(delay / 1000.0, [](void* data) -> Eina_Bool {
        TimeoutData* td = (TimeoutData*)data;
        ScriptBindingInstanceEnterer enter(td->m_window->starFish()->scriptBindingInstance());
        Window* wnd = td->m_window;
        uint32_t id = td->m_id;
        td->m_handler(td->m_window, td->m_data);
        auto iter = wnd->m_timeoutHandler.find(id);
        if (iter != wnd->m_timeoutHandler.end()) {
            wnd->m_timeoutHandler.erase(iter);
            GC_FREE(td);
        }
        return ECORE_CALLBACK_DONE;
    }, td);

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

uint32_t Window::setInterval(WindowSetTimeoutHandler handler, uint32_t delay, void* data)
{
    WindowImplEFL* eflWindow = (WindowImplEFL*)this;
    STARFISH_RELEASE_ASSERT(eflWindow->m_isActive);

    TimeoutData* td = new(NoGC) TimeoutData;
    td->m_window = this;
    uint32_t id = ++m_timeoutCounter;
    td->m_id = id;
    td->m_data = data;
    td->m_handler = handler;
    td->m_timerID = ecore_timer_add(delay / 1000.0, [](void* data) -> Eina_Bool {
        TimeoutData* td = (TimeoutData*)data;
        ScriptBindingInstanceEnterer enter(td->m_window->starFish()->scriptBindingInstance());
        auto a = td->m_window->m_timeoutHandler.find(td->m_id);
        td->m_handler(td->m_window, td->m_data);
        return ECORE_CALLBACK_RENEW;
    }, td);

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

uint32_t Window::requestAnimationFrame(WindowSetTimeoutHandler handler, void* data)
{
    WindowImplEFL* eflWindow = (WindowImplEFL*)this;
    STARFISH_RELEASE_ASSERT(eflWindow->m_isActive);

    TimeoutData* td = new(NoGC) TimeoutData;
    td->m_window = this;
    uint32_t id = ++m_requestAnimationFrameCounter;
    td->m_id = id;
    td->m_data = data;
    td->m_handler = handler;
    td->m_timerID = (Ecore_Timer*)ecore_animator_add([](void* data) -> Eina_Bool {
        TimeoutData* td = (TimeoutData*)data;
        ScriptBindingInstanceEnterer enter(td->m_window->starFish()->scriptBindingInstance());
        auto a = td->m_window->m_requestAnimationFrameHandler.find(td->m_id);
        td->m_handler(td->m_window, td->m_data);
        a = td->m_window->m_requestAnimationFrameHandler.find(td->m_id);
        if (td->m_window->m_requestAnimationFrameHandler.end() != a) {
            td->m_window->m_requestAnimationFrameHandler.erase(a);
        }
        GC_FREE(td);
        return ECORE_CALLBACK_DONE;
    }, td);

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
    renderingIfNeeds();

    if (document() && document()->frame()) {
        Frame* frame = document()->frame()->hitTest(x, y, HitTestStageEnd);
        if (!frame)
            return nullptr;

        while (!frame->node()) {
            frame = frame->parent();
        }
#ifdef STARFISH_ENABLE_TEST
        if (m_starFish->startUpFlag() & StarFishStartUpFlag::enableHitTestDump) {
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
        t->setState(Node::NodeStateActive);
        m_activeNodes.push_back(t);
        t = t->parentNode();
    }
    m_activeNodeWithTouchDown = n;
}

void Window::releaseActiveNode()
{
    for (size_t i = 0; i < m_activeNodes.size() ; i ++) {
        m_activeNodes[i]->setState(Node::NodeStateNormal);
    }
    m_activeNodes.clear();
    m_activeNodes.shrink_to_fit();
    m_activeNodeWithTouchDown = nullptr;
}

void Window::dispatchTouchEvent(float x, float y, TouchEventKind kind)
{
    // STARFISH_LOG_INFO("Window::dispatchTouchEvent %f %f kind %d\n", x, y, (int)kind);
    if (!m_isRunning)
        return;

    if (kind == TouchEventDown) {
        Node* node = hitTest(x, y);
        m_touchDownPoint = Location(x, y);
        setActiveNode(node);
    } else if (kind == TouchEventMove) {
        if ((starFish()->deviceKind() & deviceKindUseTouchScreen) && m_activeNodeWithTouchDown && ((abs(m_touchDownPoint.x() - x) > 30) || (abs(m_touchDownPoint.y() - y) > 30))) {
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
            if (shouldDispatchEvent && (t->isElement() && t->asElement()->isHTMLElement())) {
                String* eventType = starFish()->staticStrings()->m_click.localName();
                Event* e = new Event(eventType, EventInit(true, true));
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
            String* eventType = starFish()->staticStrings()->m_click.localName();
            Event* e = new Event(eventType, EventInit(true, true));
            EventTarget::dispatchEvent(t->asDocument(), e);
        }

        releaseActiveNode();

        m_activeNodeWithTouchDown = nullptr;
    }
}

void Window::dispatchKeyEvent(String* key, KeyEventKind kind)
{
}

// https://html.spec.whatwg.org/multipage/browsers.html#named-access-on-the-window-object
HTMLCollection* Window::namedAccess(String* name)
{
    // TODO
    // when child browser context(ex- iframe) implemented, we should re-implement this block
    if (document()) {
        return document()->namedAccess(name);
    } else {
        return nullptr;
    }
}


void Window::pause()
{
    STARFISH_LOG_INFO("onPause\n");
    m_isRunning = false;

    document()->setVisibleState(PageVisibilityState::PageVisibilityStateHidden);
    document()->visibilityStateChanged();
}

void Window::resume()
{
    WindowImplEFL* eflWindow = (WindowImplEFL*)this;
    eflWindow->clearEFLResources();
    STARFISH_LOG_INFO("onResume\n");
    m_isRunning = true;
    m_needsRendering = true;
    m_needsPainting = true;
    rendering();

    evas_render(evas_object_evas_get(eflWindow->m_window));

    document()->setVisibleState(PageVisibilityState::PageVisibilityStateVisible);
    document()->visibilityStateChanged();
}

void Window::close()
{
    STARFISH_LOG_INFO("window::onClose\n");

    m_document->close();

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

    if (eflWindow->m_dummyBox) {
        evas_object_del(eflWindow->m_dummyBox);
        eflWindow->m_dummyBox = nullptr;
    }

    if (eflWindow->m_mainBox) {
        elm_win_resize_object_del(eflWindow->m_window, eflWindow->m_mainBox);
        evas_object_del(eflWindow->m_mainBox);
        eflWindow->m_mainBox = nullptr;
    }

    eflWindow->m_objectList.clear();
    eflWindow->m_objectList.shrink_to_fit();
    eflWindow->m_surfaceList.clear();
    eflWindow->m_surfaceList.shrink_to_fit();
    eflWindow->m_drawnImageList.clear();

#ifndef STARFISH_TIZEN_WEARABLE
    ecore_event_handler_del(eflWindow->m_desktopMouseDownEventHandler);
    ecore_event_handler_del(eflWindow->m_desktopMouseUpEventHandler);
    ecore_event_handler_del(eflWindow->m_desktopMouseMoveEventHandler);
    ecore_event_handler_del(eflWindow->m_desktopKeyDownEventHandler);
    ecore_event_handler_del(eflWindow->m_desktopKeyUpEventHandler);
#endif

#ifdef STARFISH_TIZEN_WEARABLE
    evas_object_event_callback_del(eflWindow->m_dummyBox, EVAS_CALLBACK_MOUSE_DOWN, eflWindow->m_mobileMouseDownEventHandler);
    evas_object_event_callback_del(eflWindow->m_dummyBox, EVAS_CALLBACK_MOUSE_MOVE, eflWindow->m_mobileMouseMoveEventHandler);
    evas_object_event_callback_del(eflWindow->m_dummyBox, EVAS_CALLBACK_MOUSE_UP, eflWindow->m_mobileMouseMoveEventHandler);
#endif
}

}
