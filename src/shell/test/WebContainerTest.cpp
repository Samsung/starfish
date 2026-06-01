/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#include <iostream>

namespace StarfishShell {

LWE::WebContainer::WebContainerArguments getWebContainerArgs()
{
    LWE::WebContainer::WebContainerArguments args{
        .width = 800,
        .height = 600,
        .devicePixelRatio = 1,
        .defaultFontName = "serif",
        .locale = "ko-KR",
        .timezoneID = "Asia/Seoul",
    };
    return args;
}

LWE::WebContainer::RendererGLConfiguration getRendererConfig(Window* window)
{
    LWE::WebContainer::RendererGLConfiguration config;
    config.onMakeCurrent = [window](LWE::WebContainer* wc) {
        if (!window->renderer()) {
            return;
        }
        window->renderer()->makeCurrent();
    };
    config.onSwapBuffers = [window](LWE::WebContainer* wc, bool mayNeedsSync) {
        if (!window->renderer()) {
            return;
        }
        window->renderer()->swapBuffers();
    };
    config.onCreateSharedContext =
        [window](LWE::WebContainer* wc) -> uintptr_t {
        if (!window->renderer()) {
            return 0;
        }
        return window->renderer()->createSharedContext();
    };
    config.onDestroyContext = [window](LWE::WebContainer* wc,
                                       uintptr_t context) -> bool {
        if (!window->renderer()) {
            return false;
        }
        return window->renderer()->destroyContext(context);
    };
    config.onClearCurrentContext = [window](LWE::WebContainer* wc) -> bool {
        if (!window->renderer()) {
            return false;
        }
        return window->renderer()->clearCurrentContext();
    };
    config.onMakeCurrentWithContext = [window](LWE::WebContainer* wc,
                                               uintptr_t context) -> bool {
        if (!window->renderer()) {
            return false;
        }
        return window->renderer()->makeCurrentWithContext(context);
    };
    config.onGetProcAddress = [window](LWE::WebContainer* wc,
                                       const char* name) -> void* {
        if (!window->renderer()) {
            return nullptr;
        }
        return window->renderer()->getProcAddress(name);
    };
    config.onIsSupportedExtension = [window](LWE::WebContainer* wc,
                                             const char* extension) -> bool {
        if (!window->renderer()) {
            return false;
        }
        return window->renderer()->isSupportedExtension(extension);
    };

    return config;
}

class WebContainerCreationTest : public ::testing::Test {
public:
    WebContainerCreationTest() = default;

protected:
    void SetUp() override
    {
        LWE::LWE::Initialize("/tmp/starfish_storage/");
        m_window = Window::create();
        m_window->setInitHint(HINT_VISIBLE, 0);
        m_window->init("Starfish", 800, 600);
    }

    void TearDown()
    {
        if (m_lwe) {
            m_lwe->Destroy();
            m_lwe = nullptr;
        }

        if (m_window) {
            delete m_window;
            m_window = nullptr;
        }
        LWE::LWE::Finalize();
    }

    Window* m_window = nullptr;
    LWE::WebContainer* m_lwe = nullptr;
};

TEST_F(WebContainerCreationTest, CreateGL)
{
    LWE::WebContainer::WebContainerArguments args = getWebContainerArgs();
    LWE::WebContainer::RendererGLConfiguration config =
        getRendererConfig(m_window);

    m_lwe = LWE::WebContainer::CreateGL(args, config);
    EXPECT_TRUE(m_lwe != nullptr);
}

class WebContainerDestroyTest : public ::testing::Test {
public:
    WebContainerDestroyTest() = default;

protected:
    void SetUp() override
    {
        LWE::LWE::Initialize("/tmp/starfish_storage/");
        m_window = Window::create();
        m_window->setInitHint(HINT_VISIBLE, 0);
        m_window->init("Starfish", 800, 600);

        LWE::WebContainer::WebContainerArguments args = getWebContainerArgs();
        LWE::WebContainer::RendererGLConfiguration config =
            getRendererConfig(m_window);
        m_lwe = LWE::WebContainer::CreateGL(args, config);
    }

    void TearDown()
    {
        if (m_window) {
            delete m_window;
            m_window = nullptr;
        }
        LWE::LWE::Finalize();
    }

    Window* m_window = nullptr;
    LWE::WebContainer* m_lwe = nullptr;
};

TEST_F(WebContainerDestroyTest, Destroy)
{
    m_lwe->Destroy();
    // Expect no error.
    EXPECT_TRUE(true);
}

class WebContainerTest : public ::testing::Test {
public:
    WebContainerTest() = default;

protected:
    static void SetUpTestCase()
    {
        // give some delay for x11 server
        usleep(1000000);
        LWE::LWE::Initialize("/tmp/starfish_storage/");
        window = Window::create();
        window->setInitHint(HINT_VISIBLE, 0);
        window->init("Starfish", 800, 600);

        LWE::WebContainer::WebContainerArguments args = getWebContainerArgs();
        LWE::WebContainer::RendererGLConfiguration config =
            getRendererConfig(window);
        lwe = LWE::WebContainer::CreateGL(args, config);
    }

    static void TearDownTestCase()
    {
        if (window) {
            window->appLoop()->start(1); // ensure calling all pending jobs.
        }

        if (lwe) {
            lwe->Destroy();
            lwe = nullptr;
        }

        if (window) {
            delete window;
            window = nullptr;
        }
        LWE::LWE::Finalize();
    }

    static Window* window;
    static LWE::WebContainer* lwe;
};

Window* WebContainerTest::window = nullptr;
LWE::WebContainer* WebContainerTest::lwe = nullptr;

TEST_F(WebContainerTest, LoadURL)
{
    bool loaded = false;
    lwe->RegisterOnPageLoadedHandler(
        [&loaded](LWE::WebContainer* wc, const std::string& string) {
            window->appLoop()->stop();
            loaded = true;
        });
    lwe->LoadURL("about:blank");
    window->appLoop()->start(1);
    EXPECT_TRUE(loaded);

    lwe->RegisterOnPageLoadedHandler(
        [](LWE::WebContainer*, const std::string&) {});
}

TEST_F(WebContainerTest, LoadData)
{
    bool loaded = false;
    lwe->RegisterOnPageLoadedHandler(
        [&loaded](LWE::WebContainer* wc, const std::string& string) {
            window->appLoop()->stop();
            loaded = true;
        });
    lwe->LoadData("<html><body>Hello World!</body></html>");
    window->appLoop()->start(1);
    EXPECT_TRUE(loaded);

    lwe->RegisterOnPageLoadedHandler(
        [](LWE::WebContainer*, const std::string&) {});
}

TEST_F(WebContainerTest, GetURL)
{
    std::string onloadUrl;
    lwe->RegisterOnPageLoadedHandler(
        [&onloadUrl](LWE::WebContainer* wc, const std::string& string) {
            window->appLoop()->stop();
            onloadUrl = string;
        });
    lwe->LoadURL("about:blank");
    window->appLoop()->start(1);
    std::string getUrl = lwe->GetURL();
    EXPECT_TRUE(getUrl == onloadUrl);

    lwe->RegisterOnPageLoadedHandler(
        [](LWE::WebContainer*, const std::string&) {});
}

TEST_F(WebContainerTest, ResizeAndWidthHeight)
{
    std::string onloadUrl;
    lwe->ResizeTo(640, 480);
    window->appLoop()->start(1);
    size_t width = lwe->Width();
    size_t height = lwe->Height();
    EXPECT_EQ(width, 640);
    EXPECT_EQ(height, 480);

    lwe->ResizeTo(800, 600);
    window->appLoop()->start(1);
    width = lwe->Width();
    height = lwe->Height();
    EXPECT_EQ(width, 800);
    EXPECT_EQ(height, 600);
}

TEST_F(WebContainerTest, ScrollTo)
{
    std::string onloadUrl;
    lwe->LoadData(
        "<html><head><style> body { overflow-x: scroll; overflow-y: scroll; "
        "width: 2000px; height: 2000px; } </style></head><body> "
        "</body></html>");
    window->appLoop()->start(1);
    lwe->ScrollTo(100, 100);
    int x = lwe->GetScrollX();
    int y = lwe->GetScrollY();
    EXPECT_EQ(x, 100);
    EXPECT_EQ(y, 100);
}

TEST_F(WebContainerTest, JavaScriptInterfaceAndEvaluateJavaScript)
{
    lwe->LoadURL("about:blank");
    lwe->AddJavaScriptInterface(
        "TEST", "echo", [](std::string param) -> std::string { return param; });
    window->appLoop()->start(1);
    std::string result = lwe->EvaluateJavaScript("TEST.echo('test')");
    EXPECT_TRUE(result == "test");

    lwe->RemoveJavascriptInterface("TEST", "echo");
    window->appLoop()->start(1);
    result = lwe->EvaluateJavaScript("TEST.echo('test')");
    EXPECT_TRUE(result != "test");
}

TEST_F(WebContainerTest, Reload)
{
    int cnt = 0;
    lwe->RegisterOnPageLoadedHandler(
        [&cnt](LWE::WebContainer* wc, const std::string& string) {
            window->appLoop()->stop();
            cnt++;
        });
    lwe->LoadURL("about:blank");
    window->appLoop()->start(1);
    lwe->Reload();
    window->appLoop()->start(1);
    EXPECT_EQ(cnt, 2);
    lwe->RegisterOnPageLoadedHandler(
        [](LWE::WebContainer*, const std::string&) {});
}

TEST_F(WebContainerTest, DevicePixelRatio)
{
    int cnt = 0;
    lwe->LoadURL("about:blank");
    EXPECT_EQ(lwe->GetDevicePixelRatio(), 1);
    lwe->SetDevicePixelRatio(2);
    EXPECT_EQ(lwe->GetDevicePixelRatio(), 2);
    lwe->SetDevicePixelRatio(1);
    EXPECT_EQ(lwe->GetDevicePixelRatio(), 1);
}

TEST_F(WebContainerTest, AddIdleCallback)
{
    int called = false;
    lwe->AddIdleCallback(
        [](void* data) {
            int* called = static_cast<int*>(data);
            *called = true;
            window->appLoop()->stop();
        },
        &called);
    lwe->LoadURL("about:blank");
    window->appLoop()->start(3);
    EXPECT_EQ(called, true);
    lwe->AddIdleCallback([](void* data) {}, nullptr);
}

TEST_F(WebContainerTest, PauseResume)
{
    std::string result = "";
    lwe->AddJavaScriptInterface("TEST", "setResult",
                                [&result](std::string param) -> std::string {
                                    result = param;
                                    return param;
                                });
    lwe->LoadData(
        "<html><head></head><body></"
        "body><script>document.addEventListener('visibilitychange',() => {  "
        "TEST.setResult(document.visibilityState);});</script></html>");
    window->appLoop()->start(1);
    lwe->Pause();
    window->appLoop()->start(1);
    EXPECT_EQ(result, "hidden");

    lwe->Resume();
    window->appLoop()->start(1);
    EXPECT_EQ(result, "visible");

    lwe->RemoveJavascriptInterface("TEST", "setResult");
}

} // namespace StarfishShell
