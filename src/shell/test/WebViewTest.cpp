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
        LWE::LWE::Initialize("/tmp/starfish_storage/");
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
        LWE::LWE::Initialize("/tmp/starfish_storage/");
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

#if defined(STARFISH_SHELL_X11) && defined(STARFISH_ENABLE_WEBGL)
namespace StarfishShell {

// Destroying one view must leave the GL of the others usable; the views of
// a process share its default EGL display.
TEST(WebViewX11Test, WebGLSurvivesAnotherViewDestroy)
{
    static const char* draw = R"JS(
        (function() {
            const canvas = document.createElement('canvas');
            const gl = canvas.getContext('webgl', { antialias: false });
            if (!gl)
                return false;
            gl.clearColor(0, 1, 0, 1);
            gl.clear(gl.COLOR_BUFFER_BIT);
            const pixel = new Uint8Array(4);
            gl.readPixels(0, 0, 1, 1, gl.RGBA, gl.UNSIGNED_BYTE, pixel);
            return pixel[0] === 0 && pixel[1] === 255 &&
                   pixel[2] === 0 && pixel[3] === 255;
        })()
    )JS";

    LWE::LWE::Initialize("/tmp/starfish_storage/");
    // Only for its app loop; each view below creates its own X window.
    Window* window = Window::create();
    window->setInitHint(HINT_VISIBLE, 0);
    window->init("Starfish", 320, 240);

    LWE::WebView* other = LWE::WebView::Create(nullptr, 0, 0, 320, 240, 1,
                                               "serif", "ko-KR", "Asia/Seoul");
    LWE::WebView* survivor = LWE::WebView::Create(
        nullptr, 0, 0, 320, 240, 1, "serif", "ko-KR", "Asia/Seoul");
    ASSERT_TRUE(other);
    ASSERT_TRUE(survivor);
    other->LoadData("<p>other</p>");
    survivor->LoadData("<p>survivor</p>");
    window->appLoop()->start(1);
    EXPECT_EQ("true", other->EvaluateJavaScript(draw));
    EXPECT_EQ("true", survivor->EvaluateJavaScript(draw));

    other->Destroy();
    window->appLoop()->start(1);
    EXPECT_EQ("true", survivor->EvaluateJavaScript(draw));

    survivor->Destroy();
    delete window;
    LWE::LWE::Finalize();
}

} // namespace StarfishShell
#endif
