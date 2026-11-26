/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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

#include "ShellConfig.h"

#include "gtest/gtest.h"

#include "LWEWebView.h"
#include "Window.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

namespace StarfishShell {

// Defined in WebContainerTest.cpp.
LWE::WebContainer::WebContainerArguments getWebContainerArgs();
LWE::WebContainer::RendererGLConfiguration getRendererConfig(Window* window);

namespace {

    using PageHandler =
        std::function<void(LWE::WebContainer*, const std::string&)>;

    const char* kStorage = "/tmp/starfish_storage/";

} // namespace

// Exercises the embedding API surface of one GL WebContainer: navigation
// history, timers, user data, load-cycle and UI callbacks, input dispatch.
class WebContainerAPITest : public ::testing::Test {
protected:
    void SetUp() override
    {
        LWE::LWE::Initialize(kStorage);
        m_window = Window::create();
        m_window->setInitHint(HINT_VISIBLE, 0);
        m_window->init("Starfish", 800, 600);
        m_lwe = LWE::WebContainer::CreateGL(getWebContainerArgs(),
                                            getRendererConfig(m_window));
        ASSERT_TRUE(m_lwe);
    }

    void TearDown() override
    {
        if (m_lwe) {
            m_lwe->RegisterOnPageLoadedHandler(
                [](LWE::WebContainer*, const std::string&) {});
            spin(0.1);
            m_lwe->Destroy();
            m_lwe = nullptr;
        }
        delete m_window;
        m_window = nullptr;
        LWE::LWE::Finalize();
    }

    void spin(double seconds)
    {
        m_window->appLoop()->start(seconds);
    }

    // Runs the shell loop until done() holds or the deadline passes.
    bool runUntil(const std::function<bool()>& done, double seconds = 10)
    {
        auto end = std::chrono::steady_clock::now() +
                   std::chrono::duration<double>(seconds);
        while (!done()) {
            if (std::chrono::steady_clock::now() >= end) {
                return false;
            }
            spin(0.05);
        }
        return true;
    }

    // Loads through `start` and waits for the load event.
    void loadAndWait(const std::function<void()>& start)
    {
        bool loaded = false;
        m_lwe->RegisterOnPageLoadedHandler(
            [&loaded](LWE::WebContainer*, const std::string&) {
                loaded = true;
            });
        start();
        ASSERT_TRUE(runUntil([&] { return loaded; }));
        m_lwe->RegisterOnPageLoadedHandler(
            [](LWE::WebContainer*, const std::string&) {});
    }

    void loadData(const std::string& html)
    {
        loadAndWait([&] { m_lwe->LoadData(html); });
    }

    void loadURL(const std::string& url)
    {
        loadAndWait([&] { m_lwe->LoadURL(url); });
    }

    std::string js(const std::string& script)
    {
        return m_lwe->EvaluateJavaScript(script);
    }

    Window* m_window = nullptr;
    LWE::WebContainer* m_lwe = nullptr;
};

TEST_F(WebContainerAPITest, TitleUserAgentFontSizeAndCacheMode)
{
    loadData("<title>API title</title>");
    EXPECT_EQ("API title", m_lwe->GetTitle());

    m_lwe->SetUserAgentString("LWE-API-Test/1.0");
    EXPECT_EQ("LWE-API-Test/1.0", m_lwe->GetUserAgentString());

    m_lwe->SetDefaultFontSize(20);
    EXPECT_EQ(20u, m_lwe->GetDefaultFontSize());

    int mode = m_lwe->GetCacheMode();
    m_lwe->SetCacheMode(mode);
    EXPECT_EQ(mode, m_lwe->GetCacheMode());
}

TEST_F(WebContainerAPITest, SettingsRoundTrip)
{
    LWE::Settings settings = m_lwe->GetSettings();
    settings.SetUserAgentString("LWE-Settings/2.0");
    settings.setDefaultFontSize(18);
    settings.SetScrollbarVisible(false);
    settings.SetUseExternalPopup(true);
    m_lwe->SetSettings(settings);
    spin(0.1); // applied on the engine loop

    LWE::Settings copy(m_lwe->GetSettings());
    EXPECT_EQ("LWE-Settings/2.0", copy.GetUserAgentString());
    EXPECT_FALSE(copy.ScrollbarVisible());
    EXPECT_TRUE(copy.UseExternalPopup());
}

TEST_F(WebContainerAPITest, HistoryNavigation)
{
    loadURL("data:text/html,<title>one</title>");
    loadURL("data:text/html,<title>two</title>");
    ASSERT_TRUE(m_lwe->CanGoBack());
    EXPECT_FALSE(m_lwe->CanGoForward());

    m_lwe->GoBack();
    EXPECT_TRUE(runUntil([&] { return m_lwe->GetTitle() == "one"; }));
    EXPECT_TRUE(m_lwe->CanGoForward());

    m_lwe->GoForward();
    EXPECT_TRUE(runUntil([&] { return m_lwe->GetTitle() == "two"; }));

    m_lwe->ClearHistory();
    EXPECT_TRUE(runUntil([&] { return !m_lwe->CanGoBack(); }));
    EXPECT_FALSE(m_lwe->CanGoForward());

    m_lwe->ClearCache();
}

TEST_F(WebContainerAPITest, TimeoutFiresUnlessCleared)
{
    struct Flags {
        bool fired = false;
        bool clearedFired = false;
    } flags;
    m_lwe->AddTimeout([](void* d) { static_cast<Flags*>(d)->fired = true; },
                      &flags, 10);
    size_t cleared = m_lwe->AddTimeout(
        [](void* d) { static_cast<Flags*>(d)->clearedFired = true; }, &flags,
        10);
    m_lwe->ClearTimeout(cleared);

    EXPECT_TRUE(runUntil([&] { return flags.fired; }));
    spin(0.2);
    EXPECT_FALSE(flags.clearedFired);
}

TEST_F(WebContainerAPITest, UserData)
{
    int value = 7;
    m_lwe->SetUserData("key", &value);
    spin(0.1); // stored on the engine loop
    EXPECT_EQ(&value, m_lwe->GetUserData("key"));
    EXPECT_EQ(nullptr, m_lwe->GetUserData("missing"));
}

TEST_F(WebContainerAPITest, ScrollBy)
{
    loadData(
        "<body style='margin:0'><div style='width:3000px;height:3000px'>"
        "</div></body>");
    m_lwe->ScrollTo(0, 0);
    m_lwe->ScrollBy(30, 40);
    EXPECT_TRUE(runUntil([&] {
        return m_lwe->GetScrollX() == 30 && m_lwe->GetScrollY() == 40;
    }));
    m_lwe->ScrollBy(-10, -10);
    EXPECT_TRUE(runUntil([&] {
        return m_lwe->GetScrollX() == 20 && m_lwe->GetScrollY() == 30;
    }));
}

TEST_F(WebContainerAPITest, LoadCycleCallbacks)
{
    std::vector<std::string> started, parsed, resources;
    std::vector<int> progress;
    m_lwe->RegisterOnPageStartedHandler(
        [&](LWE::WebContainer*, const std::string& url) {
            started.push_back(url);
        });
    m_lwe->RegisterOnPageParsedHandler(
        [&](LWE::WebContainer*, const std::string& url) {
            parsed.push_back(url);
        });
    m_lwe->RegisterOnLoadResourceHandler(
        [&](LWE::WebContainer*, const std::string& url) {
            resources.push_back(url);
        });
    m_lwe->RegisterOnProgressChangedHandler(
        [&](LWE::WebContainer*, int p) { progress.push_back(p); });
    spin(0.1);

    loadURL("data:text/html,<p>cycle</p>");
    EXPECT_FALSE(started.empty());
    EXPECT_FALSE(parsed.empty());
    EXPECT_FALSE(progress.empty());

    PageHandler none = [](LWE::WebContainer*, const std::string&) {};
    m_lwe->RegisterOnPageStartedHandler(none);
    m_lwe->RegisterOnPageParsedHandler(none);
    m_lwe->RegisterOnLoadResourceHandler(none);
    m_lwe->RegisterOnProgressChangedHandler([](LWE::WebContainer*, int) {});
}

TEST_F(WebContainerAPITest, ShouldOverrideUrlLoading)
{
    loadData("<title>stay</title>");
    std::vector<std::string> asked;
    m_lwe->RegisterShouldOverrideUrlLoadingHandler(
        [&](LWE::WebContainer*, const std::string& url) {
            asked.push_back(url);
            return true; // the embedder takes over: no navigation
        });
    spin(0.1);
    js("location.href = 'data:text/html,<title>moved</title>'");
    ASSERT_TRUE(runUntil([&] { return !asked.empty(); }));
    EXPECT_NE(std::string::npos, asked[0].find("moved"));
    spin(0.3);
    EXPECT_EQ("stay", m_lwe->GetTitle());

    // Returning false lets the navigation continue.
    m_lwe->RegisterShouldOverrideUrlLoadingHandler(
        [](LWE::WebContainer*, const std::string&) { return false; });
    spin(0.1);
    js("location.href = 'data:text/html,<title>moved</title>'");
    EXPECT_TRUE(runUntil([&] { return m_lwe->GetTitle() == "moved"; }));
}

TEST_F(WebContainerAPITest, ReceivedErrorCarriesResourceError)
{
    bool called = false;
    int code = 0;
    std::string url;
    m_lwe->RegisterOnReceivedErrorHandler(
        [&](LWE::WebContainer*, LWE::ResourceError error) {
            called = true;
            code = error.GetErrorCode();
            url = error.GetUrl();
            error.GetDescription();
        });
    spin(0.1);
    m_lwe->LoadURL("file:///nonexistent/lwe-api-test.html");
    ASSERT_TRUE(runUntil([&] { return called; }));
    EXPECT_NE(0, code);
    EXPECT_NE(std::string::npos, url.find("lwe-api-test.html"));
}

// Inside the fixture: with the dynamic loader, API objects need an
// initialized engine.
TEST_F(WebContainerAPITest, ResourceErrorAccessors)
{
    LWE::ResourceError error(-2, "host lookup failed", "http://invalid./");
    LWE::ResourceError copy(error);
    EXPECT_EQ(-2, copy.GetErrorCode());
    EXPECT_EQ("host lookup failed", copy.GetDescription());
    EXPECT_EQ("http://invalid./", copy.GetUrl());
}

TEST_F(WebContainerAPITest, ShowAlert)
{
    std::string message;
    m_lwe->RegisterShowAlertHandler(
        [&](LWE::WebContainer*, const std::string&, const std::string& msg) {
            message = msg;
        });
    loadData("<p>alert</p>");
    js("setTimeout(() => alert('hello from page'), 0)");
    EXPECT_TRUE(runUntil([&] { return !message.empty(); }));
    EXPECT_EQ("hello from page", message);
}

TEST_F(WebContainerAPITest, ExternalDropdownMenu)
{
    LWE::Settings settings = m_lwe->GetSettings();
    settings.SetUseExternalPopup(true);
    m_lwe->SetSettings(settings);

    std::vector<std::string> items;
    int checked = -1;
    m_lwe->RegisterShowDropdownMenuHandler(
        [&](LWE::WebContainer*, const std::vector<std::string>* list,
            int position) {
            items = *list;
            checked = position;
        });
    loadData(
        "<body style='margin:0'><select id=s style='width:200px;"
        "height:40px'><option>apple</option><option>banana</option>"
        "</select></body>");
    m_lwe->DispatchMouseDownEvent(LWE::LeftButton, LWE::LeftButtonDown, 20, 20);
    m_lwe->DispatchMouseUpEvent(LWE::LeftButton, LWE::NoButtonDown, 20, 20);
    ASSERT_TRUE(runUntil([&] { return !items.empty(); }));
    EXPECT_EQ(2u, items.size());
    EXPECT_EQ(0, checked);

    // The engine's handler takes ownership of this layout and deletes it.
    struct Selection {
        int position;
    };
    m_lwe->CallHandler("onDropdownMenuItemSelected", new Selection{ 1 });
    EXPECT_TRUE(runUntil([&] {
        return js("document.getElementById('s').selectedIndex") == "1";
    }));
}

TEST_F(WebContainerAPITest, CustomFileResourceHandlers)
{
    static const char kPage[] = "<title>custom io</title>";
    struct Handle {
        size_t offset = 0;
    };
    std::vector<std::string> opened;
    m_lwe->RegisterCustomFileResourceRequestHandlers(
        [](const char* path) -> const char* { return path; },
        [&](const char* path) -> void* {
            opened.push_back(path);
            return new Handle;
        },
        [](uint8_t* dest, size_t size, void* h) -> size_t {
            Handle* handle = static_cast<Handle*>(h);
            size_t left = sizeof(kPage) - 1 - handle->offset;
            size_t n = size < left ? size : left;
            memcpy(dest, kPage + handle->offset, n);
            handle->offset += n;
            return n;
        },
        [](void*) -> long int { return sizeof(kPage) - 1; },
        [](void* h) { delete static_cast<Handle*>(h); });
    spin(0.1);
    loadURL("file:///virtual/lwe-custom-io.html");
    EXPECT_EQ("custom io", m_lwe->GetTitle());
    ASSERT_FALSE(opened.empty());
    EXPECT_NE(std::string::npos, opened[0].find("lwe-custom-io.html"));
}

TEST_F(WebContainerAPITest, InputEventDispatch)
{
    bool keyboardShown = false;
    m_lwe->RegisterOnShowSoftwareKeyboardIfPossibleHandler(
        [&](LWE::WebContainer*) { keyboardShown = true; });
    m_lwe->RegisterOnHideSoftwareKeyboardIfPossibleHandler(
        [](LWE::WebContainer*) {});
    loadData(R"HTML(
        <body style='margin:0'>
        <input id=i style='position:absolute;left:0;top:0;width:200px;height:40px'>
        <div style='height:3000px'></div>
        <script>
        window.seen = new Set();
        for (const t of ['mousemove', 'mousedown', 'mouseup', 'wheel',
                         'touchstart', 'touchmove', 'touchend',
                         'pointerdown', 'pointerup', 'keydown', 'keypress',
                         'keyup', 'compositionstart', 'compositionupdate',
                         'compositionend'])
            document.addEventListener(t, e => seen.add(e.type), true);
        </script>
        </body>)HTML");

    m_lwe->DispatchMouseMoveEvent(LWE::NoButton, LWE::NoButtonDown, 50, 20);
    m_lwe->DispatchMouseDownEvent(LWE::LeftButton, LWE::LeftButtonDown, 50, 20);
    m_lwe->DispatchMouseUpEvent(LWE::LeftButton, LWE::NoButtonDown, 50, 20);
    js("window.cancelWheel = e => {"
       "window.wheelReceived = e instanceof WheelEvent && e.isTrusted && "
       "e.deltaY === 3 && e.deltaMode === WheelEvent.DOM_DELTA_LINE;"
       "e.preventDefault(); };"
       "document.querySelector('div').addEventListener('wheel', cancelWheel)");
    m_lwe->DispatchMouseWheelEvent(100, 300, 3);
    EXPECT_TRUE(
        runUntil([&] { return js("String(window.wheelReceived)") == "true"; }));
    EXPECT_EQ("0", js("String(scrollY)"));
    js("document.querySelector('div').removeEventListener('wheel', "
       "cancelWheel)");
    m_lwe->DispatchMouseWheelEvent(100, 300, 3);
    EXPECT_TRUE(runUntil([&] { return js("String(scrollY)") != "0"; }));

    float points[] = { 60.f, 20.f, 120.f, 200.f };
    int ids[] = { 1, 2 };
    m_lwe->DispatchTouchStartEvent(points, ids, 2);
    points[1] = 90.f; // beyond the touch slop, so touchmove reaches the page
    m_lwe->DispatchTouchMoveEvent(points, ids, 2);
    m_lwe->DispatchTouchEndEvent(points, ids, 2);

    js("document.getElementById('i').focus()");
    m_lwe->DispatchKeyDownEvent(LWE::AKey);
    m_lwe->DispatchKeyPressEvent(LWE::AKey);
    m_lwe->DispatchKeyUpEvent(LWE::AKey);
    m_lwe->DispatchCompositionStartEvent("");
    m_lwe->DispatchCompositionUpdateEvent("ㅎ");
    m_lwe->DispatchCompositionEndEvent("한");

    auto saw = [&](const char* type) {
        return js(std::string("seen.has('") + type + "')") == "true";
    };
    EXPECT_TRUE(runUntil([&] { return saw("compositionend"); }));
    for (const char* type :
         { "mousemove", "mousedown", "mouseup", "wheel", "touchstart",
           "touchmove", "touchend", "keydown", "keypress", "keyup",
           "compositionstart", "compositionupdate" }) {
        EXPECT_TRUE(saw(type)) << type;
    }
    EXPECT_TRUE(keyboardShown);

    m_lwe->Blur();
    m_lwe->Focus();
}

TEST_F(WebContainerAPITest, WheelScrollChainsOutOfFrame)
{
    loadData(R"HTML(
        <body style="margin:0;height:3000px">
        <iframe style="display:block;border:0;width:200px;height:100px"
                srcdoc="<body style='margin:0;height:100px'>inside"></iframe>
        <script>
        window.outerWheels = 0;
        window.innerWheels = 0;
        window.addEventListener("wheel", () => outerWheels++);
        </script></body>)HTML");
    js("document.querySelector('iframe').contentWindow.addEventListener("
       "'wheel', () => innerWheels++)");
    m_lwe->DispatchMouseWheelEvent(30, 30, 3);
    EXPECT_TRUE(runUntil([&] { return js("String(scrollY > 0)") == "true"; }));
    EXPECT_EQ("1", js("String(innerWheels)"));
    EXPECT_EQ("0", js("String(outerWheels)"));
}

TEST_F(WebContainerAPITest, IdleRenderingAndDebuggerHooks)
{
    // Static: the engine still calls these while the fixture tears down, and
    // from its own thread when it runs threaded (uv backends).
    static std::atomic<bool> idle, needsRendering;
    static std::function<void()> pendingRender;
    idle = needsRendering = false;
    m_lwe->RegisterOnIdleHandler([](LWE::WebContainer*) { idle = true; });
    m_lwe->RegisterCanRenderingHandler([](LWE::WebContainer*) { return true; });
    // The embedder renders later, not from inside the callback. It does so
    // on the engine's loop, since a threaded engine renders on its thread.
    m_lwe->RegisterSetNeedsRenderingCallback(
        [](LWE::WebContainer* wc, const std::function<void()>& render) {
            needsRendering = true;
            pendingRender = render;
            wc->AddIdleCallback(
                [](void*) {
                    auto run = std::move(pendingRender);
                    pendingRender = nullptr;
                    if (run) {
                        run();
                    }
                },
                nullptr);
        });
    // The screen matrix is only asked by hit testing in transformed views.
    m_lwe->RegisterGetScreenMatrixHandler([](LWE::WebContainer*) {
        return LWE::WebContainer::TransformationMatrix{ 1, 0, 0, 0, 1,
                                                        0, 0, 0, 1 };
    });
    m_lwe->RegisterDebuggerShouldInitHandler(
        [](const std::string&, int, bool& shouldInit) { shouldInit = false; });
    m_lwe->RegisterDebuggerShouldContinueWaitingHandler(
        [](const std::string&, int, bool& shouldWait) { shouldWait = false; });
    m_lwe->RegisterOnDownloadStartHandler(
        [](LWE::WebContainer*, const std::string&, const std::string&,
           const std::string&, const std::string&, long) {});
    spin(0.1);

    loadData("<div style='width:100px;height:100px;background:green'></div>");
    js("document.body.style.background = 'yellow'");
    EXPECT_TRUE(runUntil([] { return idle && needsRendering; }));
}

// Containers created through the GL render-target entry points. The
// software and buffer entry points need a software backend, not this one.
class WebContainerVariantTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        LWE::LWE::Initialize(kStorage);
        m_window = Window::create();
        m_window->setInitHint(HINT_VISIBLE, 0);
        m_window->init("Starfish", 320, 240);
    }

    void TearDown() override
    {
        if (m_lwe) {
            m_lwe->Destroy();
        }
        delete m_window;
        LWE::LWE::Finalize();
        // Static linkage never unloads the implementation; this must still
        // answer after Finalize.
        LWE::LWE::CanUnload();
    }

    LWE::WebContainer::WebContainerArguments args()
    {
        LWE::WebContainer::WebContainerArguments a = getWebContainerArgs();
        a.width = 320;
        a.height = 240;
        return a;
    }

    bool runUntil(const std::function<bool()>& done, double seconds = 10)
    {
        auto end = std::chrono::steady_clock::now() +
                   std::chrono::duration<double>(seconds);
        while (!done()) {
            if (std::chrono::steady_clock::now() >= end) {
                return false;
            }
            m_window->appLoop()->start(0.05);
        }
        return true;
    }

    void loadGreen()
    {
        bool loaded = false;
        m_lwe->RegisterOnPageLoadedHandler(
            [&loaded](LWE::WebContainer*, const std::string&) {
                loaded = true;
            });
        m_lwe->LoadData("<body style='margin:0;background:#00ff00'></body>");
        ASSERT_TRUE(runUntil([&] { return loaded; }));
        m_lwe->RegisterOnPageLoadedHandler(
            [](LWE::WebContainer*, const std::string&) {});
    }

    Window* m_window = nullptr;
    LWE::WebContainer* m_lwe = nullptr;
};

// A headless shell has no GL to render into the target.
#if !defined(SHELL_HOST_HEADLESS_WEBCONTAINER)
TEST_F(WebContainerVariantTest, GLRenderTarget)
{
    int acquired = 0, released = 0;
    m_lwe = LWE::WebContainer::CreateGLWithRenderTarget(
        args(), getRendererConfig(m_window),
        [&](LWE::WebContainer*, LWE::WebContainer::GLRenderTarget& t) {
            t = LWE::WebContainer::GLRenderTarget{};
            t.width = 320;
            t.height = 240;
            t.clipWidth = 320;
            t.clipHeight = 240;
            t.matrix[0] = t.matrix[4] = t.matrix[8] = 1;
            t.opacity = 1;
            t.color[0] = t.color[1] = t.color[2] = 1;
            t.framebuffer = 0;
            acquired++;
            return true;
        },
        [&](LWE::WebContainer*, bool) { released++; });
    ASSERT_TRUE(m_lwe);
    loadGreen();
    EXPECT_TRUE(runUntil([&] { return released > 0; }));
    EXPECT_EQ(acquired, released);
}
#endif

TEST_F(WebContainerVariantTest, GLPlatformImage)
{
    // Without a platform image backend the prepared image maps to nothing,
    // so this only has to create, load and tear down cleanly.
    m_lwe = LWE::WebContainer::CreateGLWithPlatformImage(
        args(), getRendererConfig(m_window),
        []() { return LWE::WebContainer::ExternalImageInfo{ nullptr }; },
        [](LWE::WebContainer*, bool) {});
    ASSERT_TRUE(m_lwe);
    loadGreen();
}

} // namespace StarfishShell
