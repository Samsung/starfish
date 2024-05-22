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
        LWE::LWE::Initialize("/tmp/starfish_localStorage.txt",
                             "/tmp/starfish_cookieStore.txt",
                             "/tmp/starfish_cache/");
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
        LWE::LWE::Initialize("/tmp/starfish_localStorage.txt",
                             "/tmp/starfish_cookieStore.txt",
                             "/tmp/starfish_cache/");
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
        LWE::LWE::Initialize("/tmp/starfish_localStorage.txt",
                             "/tmp/starfish_cookieStore.txt",
                             "/tmp/starfish_cache/");
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
    lwe->LoadURL("about:blank");
    lwe->RegisterOnPageLoadedHandler(
        [&loaded](LWE::WebContainer* wc, const std::string& string) {
            window->appLoop()->stop();
            loaded = true;
        });
    window->appLoop()->start(1); // Timout 1 sec.
    EXPECT_TRUE(loaded);
}

TEST_F(WebContainerTest, LoadData)
{
    bool loaded = false;
    lwe->LoadData("<html><body>Hello World!</body></html>");
    lwe->RegisterOnPageLoadedHandler(
        [&loaded](LWE::WebContainer* wc, const std::string& string) {
            window->appLoop()->stop();
            loaded = true;
        });
    window->appLoop()->start(1); // Timout 1 sec.
    EXPECT_TRUE(loaded);
}

TEST_F(WebContainerTest, GetURL)
{
    std::string onloadUrl;
    lwe->LoadURL("about:blank");
    lwe->RegisterOnPageLoadedHandler(
        [&onloadUrl](LWE::WebContainer* wc, const std::string& string) {
            window->appLoop()->stop();
            onloadUrl = string;
        });
    window->appLoop()->start(1); // Timout 1 sec.
    std::string getUrl = lwe->GetURL();
    EXPECT_TRUE(getUrl == onloadUrl);
}

} // namespace StarfishShell
