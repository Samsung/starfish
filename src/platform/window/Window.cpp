#include "StarFishConfig.h"
#include "Window.h"

#include "dom/builder/XMLDocumentBuilder.h"
#include "dom/HTMLDocument.h"
#include "layout/FrameTreeBuilder.h"
#include "platform/canvas/font/Font.h"

#include "layout/Frame.h"
#include "layout/FrameBox.h"

#include <Elementary.h>
#include <Evas_Engine_Buffer.h>
#include <Ecore_X.h>
#include <Ecore_Input.h>
#include <Ecore_Input_Evas.h>

#ifdef STARFISH_TIZEN_WEARABLE
#include <efl_extension.h>
#include <tizen.h>
#endif

extern Evas* g_internalCanvas;

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

class WindowImplEFL : public Window {
public:
    WindowImplEFL(StarFish* sf)
        : Window(sf)
    {
    }

    virtual int width()
    {
        WindowImplEFL* eflWindow = (WindowImplEFL*)this;
        int width;
        evas_object_geometry_get(eflWindow->m_window, NULL, NULL, &width, NULL);
        return width;
    }

    virtual int height()
    {
        WindowImplEFL* eflWindow = (WindowImplEFL*)this;
        int height;
        evas_object_geometry_get(eflWindow->m_window, NULL, NULL, NULL, &height);
        return height;
    }

    uintptr_t m_handle;
    Evas_Object* m_window;
    Evas_Object* m_background;
    Evas_Object* m_canvasAdpater;
    std::vector<Evas_Object*> m_objectList;
    std::unordered_map<ImageData*, std::vector<std::pair<Evas_Object*, bool> > > m_drawnImageList;
    Evas_Object* m_dummyBox;
};

void mainRenderingFunction(Evas_Object* o, Evas_Object_Box_Data* priv, void* user_data)
{
    WindowImplEFL* wnd = (WindowImplEFL*)user_data;
    wnd->setNeedsLayout();
}

#ifndef STARFISH_TIZEN_WEARABLE
Window* Window::create(StarFish* sf, size_t w, size_t h)
{
    Evas_Object* wndObj = elm_win_add(NULL, "StarFish", ELM_WIN_BASIC);
    g_internalCanvas = evas_object_evas_get(wndObj);
    auto wnd = new WindowImplEFL(sf);
    wnd->m_starFish = sf;
    wnd->m_window = wndObj;
    elm_win_title_set(wnd->m_window, "StarFish");

    Evas* e = evas_object_evas_get(wnd->m_window);
    Ecore_Evas* ee = ecore_evas_ecore_evas_get(e);
    Ecore_Window ew = ecore_evas_window_get(ee);
    wnd->m_handle = (uintptr_t)ew;

    elm_win_autodel_set(wnd->m_window, EINA_TRUE);

    wnd->m_dummyBox = elm_box_add(wnd->m_window);
    evas_object_size_hint_weight_set(wnd->m_dummyBox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(wnd->m_window, wnd->m_dummyBox);
    elm_box_layout_set(wnd->m_dummyBox, mainRenderingFunction, wnd, NULL);
    evas_object_show(wnd->m_dummyBox);

    if (w != SIZE_MAX && h != SIZE_MAX)
        evas_object_resize(wnd->m_window, (int)w, (int)h);
    evas_object_show(wnd->m_window);
    /*
    evas_event_callback_add(e, EVAS_CALLBACK_RENDER_FLUSH_POST, [](void *data, Evas *e, void *event_info) {
    }, wnd);
*/
    ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN, [](void* data, int type, void* event) -> Eina_Bool {
        Window* sf = (Window*)data;
        Ecore_Event_Mouse_Button* d = (Ecore_Event_Mouse_Button*)event;
        sf->dispatchTouchEvent(d->x, d->y, Window::TouchEventDown);
        return EINA_TRUE;
    }, wnd);

    ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP, [](void* data, int type, void* event) -> Eina_Bool {
        Window* sf = (Window*)data;
        Ecore_Event_Mouse_Button* d = (Ecore_Event_Mouse_Button*)event;
        sf->dispatchTouchEvent(d->x, d->y, Window::TouchEventUp);
        return EINA_TRUE;
    }, wnd);

    ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE, [](void* data, int type, void* event) -> Eina_Bool {
        Window* sf = (Window*)data;
        Ecore_Event_Mouse_Move* d = (Ecore_Event_Mouse_Move*)event;
        sf->dispatchTouchEvent(d->x, d->y, Window::TouchEventMove);
        return EINA_TRUE;
    }, wnd);

    ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, [](void* data, int type, void* event) -> Eina_Bool {
        Window* sf = (Window*)data;
        Ecore_Event_Key* d = (Ecore_Event_Key*)event;
        sf->dispatchKeyEvent(String::createASCIIString(d->keyname), Window::KeyEventDown);
        return EINA_TRUE;
    }, wnd);

    ecore_event_handler_add(ECORE_EVENT_KEY_UP, [](void* data, int type, void* event) -> Eina_Bool {
        Window* sf = (Window*)data;
        Ecore_Event_Key* d = (Ecore_Event_Key*)event;
        sf->dispatchKeyEvent(String::createASCIIString(d->keyname), Window::KeyEventUp);
        return EINA_TRUE;
    }, wnd);

    return wnd;
}
#else
Window* Window::create(StarFish* sf, size_t w, size_t h, void* win)
{
    g_internalCanvas = evas_object_evas_get((Evas_Object*)win);
    auto wnd = new WindowImplEFL(sf);
    wnd->m_starFish = sf;
    wnd->m_window = (Evas_Object*)win;

    Evas* e = evas_object_evas_get(wnd->m_window);
    Evas_Object* mainBox = elm_box_add(wnd->m_window);
    evas_object_size_hint_weight_set(mainBox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(wnd->m_window, mainBox);
    elm_box_layout_set(mainBox, mainRenderingFunction, wnd, NULL);
    evas_object_show(mainBox);

    wnd->m_dummyBox = evas_object_rectangle_add(e);
    evas_object_color_set(wnd->m_dummyBox, 0, 0, 0, 0); // opaque background
    evas_object_resize(wnd->m_dummyBox, w, h);
    evas_object_show(wnd->m_dummyBox);
    evas_object_show(wnd->m_window);

    evas_object_event_callback_add(wnd->m_dummyBox, EVAS_CALLBACK_MOUSE_DOWN, [](void* data, Evas* evas, Evas_Object* obj, void* event_info) -> void {
        Window* sf = (Window*)data;
        Evas_Event_Mouse_Down* ev = (Evas_Event_Mouse_Down*) event_info;
        sf->dispatchTouchEvent(ev->canvas.x, ev->canvas.y, Window::TouchEventDown);
        return;
    }, wnd);

    evas_object_event_callback_add(wnd->m_dummyBox, EVAS_CALLBACK_MOUSE_MOVE, [](void* data, Evas* evas, Evas_Object* obj, void* event_info) -> void {
        Window* sf = (Window*)data;
        Evas_Event_Mouse_Move* ev = (Evas_Event_Mouse_Move*) event_info;
        sf->dispatchTouchEvent(ev->cur.canvas.x, ev->cur.canvas.y, Window::TouchEventMove);
        return;
    }, wnd);

    evas_object_event_callback_add(wnd->m_dummyBox, EVAS_CALLBACK_MOUSE_UP, [](void* data, Evas* evas, Evas_Object* obj, void* event_info) -> void {
        Window* sf = (Window*)data;
        Evas_Event_Mouse_Up* ev = (Evas_Event_Mouse_Up*) event_info;
        sf->dispatchTouchEvent(ev->canvas.x, ev->canvas.y, Window::TouchEventUp);
        return;
    }, wnd);

    evas_event_callback_add(e, EVAS_CALLBACK_RENDER_FLUSH_POST, [](void* data, Evas* e, void* event_info) {
    }, wnd);

    /*
    eext_rotary_event_handler_add([](void *data, Eext_Rotary_Event_Info *info) -> Eina_Bool {
        Window* sf = (Window*)data;
        if (info->direction == EEXT_ROTARY_DIRECTION_CLOCKWISE) {
            sf->dispatchKeyEvent(String::createASCIIString("rotaryClockWise"), Window::KeyEventDown);
            sf->dispatchKeyEvent(String::createASCIIString("rotaryClockWise"), Window::KeyEventUp);
        } else {
            sf->dispatchKeyEvent(String::createASCIIString("rotaryCounterClockWise"), Window::KeyEventDown);
            sf->dispatchKeyEvent(String::createASCIIString("rotaryCounterClockWise"), Window::KeyEventUp);
        }
        return EINA_TRUE;
    }, this);
    */

    return wnd;
}
#endif

Window::Window(StarFish* starFish)
    : m_touchDownPoint(0, 0)
    , m_starFish(starFish)
{
    STARFISH_ASSERT(m_starFish->scriptBindingInstance());

    m_document = new HTMLDocument(this, m_starFish->scriptBindingInstance(), m_styleResolver.resolveDocumentStyle(m_starFish));
    initScriptWrappable(this);
    m_document->initScriptWrappable(m_document);
    m_timeoutCounter = 0;

    m_needsRendering = false;
    m_needsStyleRecalc = true;
    m_needsFrameTreeBuild = true;
    m_needsLayout = true;
    m_needsPainting = true;

    m_hasRootElementBackground = false;
    m_hasBodyElementBackground = false;
    m_isRunning = true;
    m_activeNodeWithTouchDown = nullptr;

    CSSStyleSheet* userAgentStyleSheet = new CSSStyleSheet;

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSStyleRule::Kind::TypeSelector, String::createASCIIString("html"), CSSStyleRule::PseudoClass::None, document());
        bool result;
        rule->styleDeclaration()->addValuePair(CSSStyleValuePair::fromString("display", "block", result));
        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSStyleRule::Kind::TypeSelector, String::createASCIIString("head"), CSSStyleRule::PseudoClass::None, document());
        bool result;
        rule->styleDeclaration()->addValuePair(CSSStyleValuePair::fromString("display", "none", result));
        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSStyleRule::Kind::TypeSelector, String::createASCIIString("style"), CSSStyleRule::PseudoClass::None, document());
        bool result;
        rule->styleDeclaration()->addValuePair(CSSStyleValuePair::fromString("display", "none", result));
        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSStyleRule::Kind::TypeSelector, String::createASCIIString("script"), CSSStyleRule::PseudoClass::None, document());
        bool result;
        rule->styleDeclaration()->addValuePair(CSSStyleValuePair::fromString("display", "none", result));
        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSStyleRule::Kind::TypeSelector, String::createASCIIString("meta"), CSSStyleRule::PseudoClass::None, document());
        bool result;
        rule->styleDeclaration()->addValuePair(CSSStyleValuePair::fromString("display", "none", result));
        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSStyleRule::Kind::TypeSelector, String::createASCIIString("body"), CSSStyleRule::PseudoClass::None, document());
        bool result;
        rule->styleDeclaration()->addValuePair(CSSStyleValuePair::fromString("display", "block", result));
        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSStyleRule::Kind::TypeSelector, String::createASCIIString("body"), CSSStyleRule::PseudoClass::None, document());
        bool result;
        rule->styleDeclaration()->addValuePair(CSSStyleValuePair::fromString("margin-top", "8px", result));
        rule->styleDeclaration()->addValuePair(CSSStyleValuePair::fromString("margin-bottom", "8px", result));
        rule->styleDeclaration()->addValuePair(CSSStyleValuePair::fromString("margin-left", "8px", result));
        rule->styleDeclaration()->addValuePair(CSSStyleValuePair::fromString("margin-right", "8px", result));
        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSStyleRule::Kind::TypeSelector, String::createASCIIString("div"), CSSStyleRule::PseudoClass::None, document());
        bool result;
        rule->styleDeclaration()->addValuePair(CSSStyleValuePair::fromString("display", "block", result));
        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSStyleRule::Kind::TypeSelector, String::createASCIIString("p"), CSSStyleRule::PseudoClass::None, document());
        bool result;
        rule->styleDeclaration()->addValuePair(CSSStyleValuePair::fromString("display", "block", result));
        rule->styleDeclaration()->addValuePair(CSSStyleValuePair::fromString("margin-top", "1em", result));
        rule->styleDeclaration()->addValuePair(CSSStyleValuePair::fromString("margin-bottom", "1em", result));
        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSStyleRule::Kind::TypeSelector, String::createASCIIString("span"), CSSStyleRule::PseudoClass::None, document());
        bool result;
        rule->styleDeclaration()->addValuePair(CSSStyleValuePair::fromString("display", "inline", result));
        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSStyleRule::Kind::TypeSelector, String::createASCIIString("img"), CSSStyleRule::PseudoClass::None, document());
        bool result;
        rule->styleDeclaration()->addValuePair(CSSStyleValuePair::fromString("display", "inline", result));
        userAgentStyleSheet->addRule(rule);
    }

    m_styleResolver.addSheet(userAgentStyleSheet);

    if (m_starFish->startUpFlag() & StarFishStartUpFlag::enableBlackTheme) {
        m_document->style()->setColor(Color(255, 255, 255, 255));
    }

    setNeedsRendering();
}

#define STARFISH_ENABLE_TIMER

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
#endif
    }

protected:
#ifdef STARFISH_ENABLE_TIMER
    unsigned long m_start;
    const char* m_msg;
#endif
};

void Window::rendering()
{
    if (!m_needsRendering)
        return;

    Timer renderingTimer("Window::rendering");

    if (m_needsStyleRecalc) {
        // resolve style
        Timer t("resolve style");
        m_styleResolver.resolveDOMStyle(m_document);
        m_needsStyleRecalc = false;

        if (m_starFish->startUpFlag() & StarFishStartUpFlag::enableComputedStyleDump) {
            // dump style
            m_styleResolver.dumpDOMStyle(m_document);
        }
    }

    if (m_needsFrameTreeBuild) {
        // create frame tree
        Timer t("create frame tree");
        FrameTreeBuilder::buildFrameTree(m_document);
        m_needsFrameTreeBuild = false;
    }

    if (m_needsLayout) {
        // lay out frame tree
        Timer t("lay out frame tree");
        LayoutContext ctx;
        m_document->frame()->layout(ctx);
        m_needsLayout = false;
    }

    if (m_starFish->startUpFlag() & StarFishStartUpFlag::enableFrameTreeDump) {
        FrameTreeBuilder::dumpFrameTree(m_document);
    }

    if (m_needsPainting) {
        Timer t("painting");
        m_document->frame()->firstChild()->asFrameBox()->stackingContext()->computeStackingContextProperties();

        if (m_starFish->startUpFlag() & StarFishStartUpFlag::enableStackingContextDump) {
            STARFISH_ASSERT(m_document->frame()->firstChild()->asFrameBox()->isRootElement());
            StackingContext* ctx = m_document->frame()->firstChild()->asFrameBox()->stackingContext();

            std::function<void(StackingContext*, int)> dumpSC = [&dumpSC](StackingContext* ctx, int depth)
            {
                for (int i = 0; i < depth; i ++) {
                    printf("  ");
                }

                printf("StackingContext[%p, frame %p, buf %d]\n", ctx, ctx->owner(), (int)ctx->needsOwnBuffer());

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

        // painting
        WindowImplEFL* eflWindow = (WindowImplEFL*)this;
        int width, height;
        evas_object_geometry_get(eflWindow->m_window, NULL, NULL, &width, &height);
        Evas* evas = evas_object_evas_get(eflWindow->m_window);
        struct dummy {
            void* a;
            void* b;
            int w;
            int h;
            std::vector<Evas_Object*>* objList;
        };
        dummy* d = new dummy;
        d->a = evas;
        d->b = &eflWindow->m_drawnImageList;
        d->w = width;
        d->h = height;
        d->objList = &eflWindow->m_objectList;
        auto iter = eflWindow->m_objectList.begin();
        while (iter != eflWindow->m_objectList.end()) {
            evas_object_del(*iter);
            iter++;
        }
        eflWindow->m_objectList.clear();
        eflWindow->m_objectList.shrink_to_fit();

        Canvas* canvas = Canvas::createDirect(d);
        delete d;

        if (m_hasRootElementBackground || m_hasBodyElementBackground) {
            LayoutRect rt(0, 0, width, height);
            if (m_hasRootElementBackground) {
                FrameBox::paintBackground(canvas, document()->rootElement()->style(), rt, rt);
            } else {
                FrameBox::paintBackground(canvas, document()->rootElement()->body()->style(), rt, rt);
            }

        } else {
            if (m_starFish->startUpFlag() & StarFishStartUpFlag::enableBlackTheme)
                canvas->clearColor(Color(0, 0, 0, 255));
            else
                canvas->clearColor(Color(255, 255, 255, 255));
        }

        m_document->frame()->paint(canvas, PaintingStageEnd);
        m_needsPainting = false;

        delete canvas;
#ifdef STARFISH_TIZEN_WEARABLE
        evas_object_raise(eflWindow->m_dummyBox);
#endif
    }

#ifdef STARFISH_TIZEN_WEARABLE
/*
    canvas->save();
    canvas->setColor(Color(255, 0, 0, 255));
    canvas->drawRect(Rect(180-20, 320, 40, 20));
    canvas->setColor(Color(255, 255, 255, 255));
    String* txt = String::createASCIIString("SWC");
    Font* fnt = FontSelector::loadFont(String::createASCIIString(""), 12);
    Size siz = fnt->measureText(txt);
    canvas->setFont(fnt);
    canvas->drawText(180 - siz.width()/2, 324, txt);
    canvas->restore();
    */
#endif

    m_needsRendering = false;

#ifdef NDEBUG
    STARFISH_LOG_INFO("rendering end. GC heapSize...%f MB\n", GC_get_heap_size() / 1024.f / 1024.f);
#else
    STARFISH_LOG_INFO("rendering end. GC heapSize...%f MB / %f MB\n", GC_get_memory_use() / 1024.f / 1024.f, GC_get_heap_size() / 1024.f / 1024.f);
#endif

}

void Window::setNeedsRenderingSlowCase()
{
    STARFISH_ASSERT(!m_needsRendering);
    m_needsRendering = true;

    ecore_animator_add([](void* data) -> Eina_Bool {
        // FIXME
        // we pass window data as idler.
        // but, bdwgc is cannot see ecore's memory
        // we should add window as root set
        // or pointer live check
        Window* wnd = (Window*) data;
        wnd->rendering();

        return ECORE_CALLBACK_CANCEL;
    }, this);
}

void Window::loadXMLDocument(String* filePath)
{
    XMLDocumentBuilder* builder = new XMLDocumentBuilder;
    builder->build(m_document, filePath);
}

struct TimeoutData {
    Window* m_window;
    uint32_t m_id;
};

uint32_t Window::setTimeout(WindowSetTimeoutHandler handler, uint32_t delay, void* data)
{
    TimeoutData* td = new TimeoutData;
    td->m_window = this;
    uint32_t id = m_timeoutCounter++;
    td->m_id = id;
    m_timeoutHandler.insert(std::make_pair(id, std::make_pair(handler, data)));

    // FIXME
    // instance of window is not rooted.
    // because timeoutdata is stored in memory area in ecore.
    // this implemention is very unsafe
    ecore_timer_add(delay / 1000.0, [](void* data) -> Eina_Bool {
        TimeoutData* td = (TimeoutData*)data;
        auto a = td->m_window->m_timeoutHandler.find(td->m_id);

        if (a->second.second != nullptr)
            a->second.first(td->m_window, a->second.second);

        td->m_window->m_timeoutHandler.erase(td->m_window->m_timeoutHandler.find(td->m_id));
        delete td;
        return ECORE_CALLBACK_DONE;
    }, td);

    return id;
}

void Window::clearTimeout(uint32_t id)
{
    // TODO : Use ecore_timer_del(Ecore_Timer *timer)
    auto handlerData = m_timeoutHandler.find(id);
    if (handlerData != m_timeoutHandler.end())
        handlerData->second.second = nullptr;
}

uint32_t Window::requestAnimationFrame(WindowSetTimeoutHandler handler, void* data)
{
    TimeoutData* td = new TimeoutData;
    td->m_window = this;
    uint32_t id = m_requestAnimationFrameCounter++;
    td->m_id = id;
    m_requestAnimationFrameHandler.insert(std::make_pair(id, std::make_pair(handler, data)));

    ecore_animator_add([](void* data) -> Eina_Bool {
        TimeoutData* td = (TimeoutData*)data;
        auto a = td->m_window->m_requestAnimationFrameHandler.find(td->m_id);

        if (a->second.second != nullptr)
            a->second.first(td->m_window, a->second.second);

        td->m_window->m_requestAnimationFrameHandler.erase(td->m_window->m_requestAnimationFrameHandler.find(td->m_id));
        delete td;
        return ECORE_CALLBACK_DONE;
    }, td);

    return id;
}

void Window::cancelAnimationFrame(uint32_t reqID)
{
    auto handlerData = m_requestAnimationFrameHandler.find(reqID);

    if (handlerData != m_requestAnimationFrameHandler.end())
        handlerData->second.second = nullptr;
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

        if (m_starFish->startUpFlag() & StarFishStartUpFlag::enableHitTestDump) {
            printf("hitTest Result-> ");
            frame->node()->dump();
            puts("");
        }

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

    Node* node = hitTest(x, y);
    if (kind == TouchEventDown) {
        m_touchDownPoint = Location(x, y);
        setActiveNode(node);
    } else {
        if (kind == TouchEventMove) {
            if ((starFish()->deviceKind() & deviceKindUseTouchScreen) && m_activeNodeWithTouchDown && ((abs(m_touchDownPoint.x() - x) > 15) || (abs(m_touchDownPoint.y() - y) > 15))) {
                releaseActiveNode();
                m_activeNodeWithTouchDown = nullptr;
            }
        } else if (kind == TouchEventUp) {
            bool shouldCallOnClick = false;
            if (m_activeNodeWithTouchDown == node) {
                shouldCallOnClick = true;
            }

            Node* t = m_activeNodeWithTouchDown;

            bool shouldDispatchEvent = shouldCallOnClick;
            while (t) {
                if (shouldDispatchEvent && (t->isElement() && t->asElement()->isHTMLElement())) {
                    QualifiedName eventType = QualifiedName::fromString(document()->window()->starFish(), "click");
                    Event* e = new Event(eventType, EventInit(true, false));
                    EventTarget::dispatchEvent(t->asNode(), e);
                    shouldDispatchEvent = false;
                }
                t = t->parentNode();
            }

            releaseActiveNode();

            m_activeNodeWithTouchDown = nullptr;
        }
    }
}

void Window::dispatchKeyEvent(String* key, KeyEventKind kind)
{
}

void Window::pause()
{
    STARFISH_LOG_INFO("onPause");
    m_isRunning = false;

    document()->setVisibleState(PageVisibilityState::PageVisibilityStateHidden);
    document()->visibilityStateChanged();
}

void Window::resume()
{
    STARFISH_LOG_INFO("onResume");
    m_isRunning = true;
    m_needsRendering = true;
    m_needsPainting = true;
    WindowImplEFL* eflWindow = (WindowImplEFL*)this;

    auto a = eflWindow->m_drawnImageList.begin();
    while (a != eflWindow->m_drawnImageList.end()) {
        evas_object_unref((Evas_Object*)a->first->unwrap());
        a++;
    }
    eflWindow->m_drawnImageList.clear();
    rendering();

    document()->setVisibleState(PageVisibilityState::PageVisibilityStateVisible);
    document()->visibilityStateChanged();
}

void Window::close()
{
    STARFISH_LOG_INFO("onClose");
}


}
