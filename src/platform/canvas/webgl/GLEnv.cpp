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

#include "platform/canvas/webgl/GLEnv.h"
#include "platform/canvas/webgl/XGL.h"
#include "platform/canvas/webgl/GLES.h"
#include "platform/canvas/webgl/XGLUtil.h"
#include "StarfishBase.h"
#include "StarfishPlatform.h"

namespace Starfish {

std::shared_ptr<GLEnv> GLEnv::instance()
{
    static std::shared_ptr<GLEnv> instance;

    if (instance == nullptr) {
        instance = std::make_shared<GLEnv>();
    }
    return instance;
}

/*
 * @brief Initialize the environment based on the current GL Context attached.
 */
bool GLEnv::initialize()
{
#if defined(GL_BRIDGE_EVASGL)
    // TODO
    STARFISH_ASSERT_NOT_REACHED();
    return false;
#else
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

    m_platform.egl.display = display;
    m_platform.egl.surface = draw;
    m_platform.egl.config = config;
    m_platform.context = context;

    XGLUtil::printXGLInfo(m_platform);

    STARFISH_ASSERT(isInitialzed());

#endif
    return true;
}

bool GLEnv::isInitialzed()
{
    if (!m_platform.egl.display || !m_platform.egl.surface ||
        !m_platform.egl.config || !m_platform.context) {
        return false;
    }
    return true;
}

XGLContext GLEnv::context()
{
    return m_platform.context;
}

XGLPlatform GLEnv::platform()
{
    return m_platform;
}

} // namespace Starfish

#endif
