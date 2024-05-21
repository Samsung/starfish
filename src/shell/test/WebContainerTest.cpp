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
        if (m_window) {
            delete m_window;
            m_window = nullptr;
        }
        if (m_lwe) {
            m_lwe->Destroy();
            m_lwe = nullptr;
        }
        LWE::LWE::Finalize();
    }

    Window* m_window = nullptr;
    LWE::WebContainer* m_lwe = nullptr;
};

TEST_F(WebContainerCreationTest, Basic)
{
    LWE::WebContainer::WebContainerArguments args{
        .width = 800,
        .height = 600,
        .devicePixelRatio = 1,
        .defaultFontName = "serif",
        .locale = "ko-KR",
        .timezoneID = "Asia/Seoul",
    };
    LWE::WebContainer::RendererGLConfiguration config;
    config.onMakeCurrent = [this](LWE::WebContainer* wc) {
        if (!m_window) {
            return;
        }
        m_window->renderer()->makeCurrent();
    };
    config.onSwapBuffers = [this](LWE::WebContainer* wc, bool mayNeedsSync) {
        if (!m_window) {
            return;
        }
        m_window->renderer()->swapBuffers();
    };
    config.onCreateSharedContext = [this](LWE::WebContainer* wc) -> uintptr_t {
        if (!m_window) {
            return 0;
        }
        return m_window->renderer()->createSharedContext();
    };
    config.onDestroyContext = [this](LWE::WebContainer* wc,
                                     uintptr_t context) -> bool {
        if (!m_window) {
            return false;
        }
        return m_window->renderer()->destroyContext(context);
    };
    config.onClearCurrentContext = [this](LWE::WebContainer* wc) -> bool {
        if (!m_window) {
            return false;
        }
        return m_window->renderer()->clearCurrentContext();
    };
    config.onMakeCurrentWithContext = [this](LWE::WebContainer* wc,
                                             uintptr_t context) -> bool {
        if (!m_window) {
            return false;
        }
        return m_window->renderer()->makeCurrentWithContext(context);
    };
    config.onGetProcAddress = [this](LWE::WebContainer* wc,
                                     const char* name) -> void* {
        if (!m_window) {
            return nullptr;
        }
        return m_window->renderer()->getProcAddress(name);
    };
    config.onIsSupportedExtension = [this](LWE::WebContainer* wc,
                                           const char* extension) -> bool {
        if (!m_window) {
            return false;
        }
        return m_window->renderer()->isSupportedExtension(extension);
    };

    m_lwe = LWE::WebContainer::CreateGL(args, config);
    EXPECT_TRUE(m_lwe != nullptr);

    bool done = false;
    m_lwe->RegisterOnPageLoadedHandler(
        [&done](LWE::WebContainer* wc, const std::string& url) {
            EXPECT_TRUE(true);
            done = true;
        });
    m_lwe->LoadURL("https://curl.se/libcurl/");

    while (!done) {
        usleep(1);
    }
    EXPECT_TRUE(true);
}

} // namespace StarfishShell
