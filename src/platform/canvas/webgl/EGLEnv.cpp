/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_WEBGL)

#include "platform/canvas/webgl/EGLEnv.h"
#include "platform/canvas/webgl/EGL.h"
#include "platform/canvas/webgl/GLES.h"
#include "platform/canvas/webgl/EGLUtil.h"
#include "StarfishBase.h"
#include "StarfishPlatform.h"

namespace Starfish {

std::shared_ptr<EGLEnv> EGLEnv::instance()
{
    static std::shared_ptr<EGLEnv> instance;

    if (instance == nullptr) {
        instance = std::make_shared<EGLEnv>();
    }
    return instance;
}

/*
 * @brief Initialize the environment based on the current GL Context attached.
 */
bool EGLEnv::initialize()
{
#if !defined(PORT_WEBVIEW_BRIDGE_GLFW)
    STARFISH_ASSERT("Not supported yet");
#endif

    if (isInitialzed()) {
        STARFISH_LOG_WARN("Already initialzed.");
        return false;
    }

    EGLContext context = eglGetCurrentContext();

    if (!context) {
        STARFISH_LOG_ERROR("No attached context found.");
        return false;
    }

    EGLDisplay display = eglGetCurrentDisplay();
    EGLSurface read = eglGetCurrentSurface(EGL_READ);
    EGLSurface draw = eglGetCurrentSurface(EGL_DRAW);

    STARFISH_ASSERT(read == draw);

    EGLConfig config = nullptr;
    EGLint configId, numConfigs, currentConfigId;
    eglQueryContext(display, context, EGL_CONFIG_ID, &configId);
    eglGetConfigs(display, nullptr, 0, &numConfigs);

    std::vector<EGLConfig> configs(numConfigs);
    eglGetConfigs(display, configs.data(), numConfigs, &numConfigs);
    for (const auto& c : configs) {
        eglGetConfigAttrib(display, c, EGL_CONFIG_ID, &currentConfigId);
        if (currentConfigId == configId) {
            config = c;
            break;
        }
    }

    if (!display || !read || !draw || !config || !context) {
        STARFISH_LOG_ERROR("Not initialized properly.");
        return false;
    }

    m_platform.display = display;
    m_platform.surface = draw;
    m_platform.config = config;
    m_platform.context = context;

    EGLUtil::printEGLInfo(display, config, context);

    STARFISH_ASSERT(isInitialzed());
    return true;
}

bool EGLEnv::isInitialzed()
{
    if (!m_platform.display || !m_platform.surface || !m_platform.config ||
        !m_platform.context) {
        return false;
    }
    return true;
}

EGLDisplay EGLEnv::display()
{
    return m_platform.display;
}

EGLSurface EGLEnv::surface()
{
    return m_platform.surface;
}

EGLConfig EGLEnv::config()
{
    return m_platform.config;
}

EGLContext EGLEnv::context()
{
    return m_platform.context;
}

EGLPlatform EGLEnv::platform()
{
    return m_platform;
}

} // namespace Starfish

#endif
