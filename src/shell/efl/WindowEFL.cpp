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

#if defined(STARFISH_SHELL_EFL)

#include "Window.h"

#include <Elementary.h>

namespace StarfishShell {

class WindowEFL final : public Window {
public:
    WindowEFL();
    ~WindowEFL();
    bool init(const char* appName, int width, int height) override;

    void terminate() override;
    void addAutoFitChild(void* child) override;
    void getCursorPos(double& xpos, double& ypos) override
    {
    }

    void* getNativeWindowHandle() override;
    RendererDelegate* renderer() override
    {
        return nullptr;
    }

    virtual void setRotate(int degree) override;

private:
    void initConfig();
    bool createSimpleWindow(const char* appName, int width, int height);

    Evas_Object* m_window = nullptr;
};

WindowEFL::WindowEFL()
{
    m_appLoop = AppLoop::create();
}

WindowEFL::~WindowEFL()
{
    m_appLoop->deinit();
}

bool WindowEFL::init(const char* appName, int width, int height)
{
    m_appLoop->init();
    initConfig();

    if (!createSimpleWindow(appName, width, height)) {
        exit(-1);
    }

    // example code about diligently getting focus in EFL
    evas_object_event_callback_add(
        m_window, EVAS_CALLBACK_FOCUS_IN,
        [](void* data, Evas* e, Evas_Object* obj, void* event_info) {
            WindowEFL* winEFL = reinterpret_cast<WindowEFL*>(data);
            if (winEFL->m_focusInHandler) {
                winEFL->m_focusInHandler();
            }
        },
        this);

    if (!m_isVisible) {
        evas_object_hide(m_window);
    } else {
        evas_object_show(m_window);
    }

    return true;
}

void WindowEFL::initConfig()
{
    elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

    const char* defaultEngine = "gl";
    const char* engine = getenv("STARFISH_ELM_ENGINE");
    if (!engine || strlen(engine) == 0) {
        engine = defaultEngine;
    }
    const char* defaultConfig = "opengl";
    const char* elmConfig = getenv("STARFISH_ELM_CONFIG");
    if (!elmConfig || strlen(elmConfig) == 0) {
        elmConfig = defaultConfig;
    }
    setenv("ELM_ENGINE", engine, 1);
#if defined(SHELL_ENABLE_ELEMENTARY_GL)
    elm_config_accel_preference_set(elmConfig);
#else
    elm_config_accel_preference_set(elmConfig);
#if !defined(SHELL_TIZEN)
    elm_config_preferred_engine_set("software_x11");
#endif
#endif
}

bool WindowEFL::createSimpleWindow(const char* appName, int width, int height)
{
    m_window = elm_win_add(NULL, appName, ELM_WIN_BASIC);
    elm_win_title_set(m_window, appName);
    elm_win_autodel_set(m_window, EINA_TRUE);
    evas_object_resize(m_window, width, height);
    evas_object_show(m_window);

    // Hack: Move the position slightly as shown below to avoid cases where the
    // window object is not displayed correctly, and then move back.
    Evas_Coord x, y;
    evas_object_geometry_get(m_window, &x, &y, nullptr, nullptr);
    evas_object_move(m_window, x + 1, y);
    evas_object_move(m_window, x, y);

#if defined(SHELL_TIZEN)
    int rots[4] = { 0, 90, 180, 270 };
    elm_win_wm_rotation_available_rotations_set(m_window, (const int*)(&rots),
                                                4);

#ifdef SHELL_ENABLE_TRANSPARENT_WINDOW
    // Set efl configuration for resizing window (Without this, Window'll be
    // full-screen only )
    elm_win_aux_hint_add(m_window, "wm.policy.win.user.geometry", "1");

    elm_win_alpha_set(m_window, EINA_TRUE);
    Evas_Object* bg = elm_bg_add(m_window);
    evas_object_color_set(bg, 0x00, 0x00, 0x00, 0x00);

    evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(m_window, bg);
    evas_object_show(bg);
#else
    Evas_Object* bg = elm_bg_add(m_window);
    // set background color to transparent for draw video correctly in tizen tv
    evas_object_color_set(bg, 0x00, 0x00, 0x00, 0x00);

    evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(m_window, bg);
    evas_object_show(bg);
#endif
#endif

    return true;
}

void* WindowEFL::getNativeWindowHandle()
{
    return m_window;
}

void WindowEFL::terminate()
{
}

void WindowEFL::addAutoFitChild(void* child)
{
    Evas_Object* c = static_cast<Evas_Object*>(child);
    evas_object_size_hint_weight_set(c, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(m_window, c);
}

void WindowEFL::setRotate(int degree)
{
    elm_win_rotation_with_resize_set(m_window, degree);
}

Window* Window::create()
{
    return new WindowEFL();
}

LWE::KeyValue Window::convertKeyCode(const unsigned long key, INPUT action,
                                     unsigned mods)
{
    return LWE::KeyValue::UnidentifiedKey;
}

} // namespace StarfishShell

#endif
