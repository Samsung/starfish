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

#if defined(SHELL_ENABLE_ELEMENTARY_GL)
namespace StarfishShell {

class WebViewCreationTest : public ::testing::Test {
public:
    WebViewCreationTest() = default;

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
    LWE::WebView* m_lwe = nullptr;
};

TEST_F(WebViewCreationTest, Create)
{
    m_lwe = LWE::WebView::Create(m_window->getNativeWindowHandle(), 0, 0, 800,
                                 600, 1, "serif", "ko-KR", "Asia/Seoul");

    EXPECT_TRUE(m_lwe != nullptr);
}

class WebViewDestroyTest : public ::testing::Test {
public:
    WebViewDestroyTest() = default;

protected:
    void SetUp() override
    {
        LWE::LWE::Initialize("/tmp/starfish_localStorage.txt",
                             "/tmp/starfish_cookieStore.txt",
                             "/tmp/starfish_cache/");
        m_window = Window::create();
        m_window->setInitHint(HINT_VISIBLE, 0);
        m_window->init("Starfish", 800, 600);

        m_lwe =
            LWE::WebView::Create(m_window->getNativeWindowHandle(), 0, 0, 800,
                                 600, 1, "serif", "ko-KR", "Asia/Seoul");
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
    LWE::WebView* m_lwe = nullptr;
};

TEST_F(WebViewDestroyTest, Destroy)
{
    m_lwe->Destroy();
    // Expect no error.
    EXPECT_TRUE(true);
}

class WebViewTest : public ::testing::Test {
public:
    WebViewTest() = default;

protected:
    static void SetUpTestCase()
    {
        LWE::LWE::Initialize("/tmp/starfish_localStorage.txt",
                             "/tmp/starfish_cookieStore.txt",
                             "/tmp/starfish_cache/");
        window = Window::create();
        window->setInitHint(HINT_VISIBLE, 0);
        window->init("Starfish", 800, 600);

        lwe = LWE::WebView::Create(window->getNativeWindowHandle(), 0, 0, 800,
                                   600, 1, "serif", "ko-KR", "Asia/Seoul");
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
    static LWE::WebView* lwe;
};

Window* WebViewTest::window = nullptr;
LWE::WebView* WebViewTest::lwe = nullptr;

TEST_F(WebViewTest, LoadURL)
{
    bool loaded = false;
    lwe->LoadURL("about:blank");
    lwe->RegisterOnPageLoadedHandler(
        [&loaded](LWE::WebView* wc, const std::string& string) {
            window->appLoop()->stop();
            loaded = true;
        });
    window->appLoop()->start(3); // Timout 3 sec.
    EXPECT_TRUE(loaded);
}

TEST_F(WebViewTest, LoadData)
{
    bool loaded = false;
    lwe->LoadData("<html><body>Hello World!</body></html>");
    lwe->RegisterOnPageLoadedHandler(
        [&loaded](LWE::WebView* wc, const std::string& string) {
            window->appLoop()->stop();
            loaded = true;
        });
    window->appLoop()->start(3); // Timout 3 sec.
    EXPECT_TRUE(loaded);
}

TEST_F(WebViewTest, GetURL)
{
    std::string onloadUrl;
    lwe->LoadURL("about:blank");
    lwe->RegisterOnPageLoadedHandler(
        [&onloadUrl](LWE::WebView* wc, const std::string& string) {
            window->appLoop()->stop();
            onloadUrl = string;
        });
    window->appLoop()->start(3); // Timout 3 sec.
    std::string getUrl = lwe->GetURL();
    EXPECT_TRUE(getUrl == onloadUrl);
}

} // namespace StarfishShell

#endif
