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

#include "platform/canvas/webgl/XGLPlatform.h"
#include "platform/canvas/webgl/GLES.h"
#include "StarfishBase.h"

#if defined(GL_BRIDGE_EVASGL)
EVAS_GL_GLOBAL_GLES3_DEFINE() // __evas_gl_glapi
#endif

std::shared_ptr<XGLPlatform> XGLPlatform::instance()
{
    static std::shared_ptr<XGLPlatform> instance;

    if (instance == nullptr) {
        instance = std::make_shared<XGLPlatform>();
    }
    return instance;
}

const XGLPlatform& XGLPlatform::ref()
{
    std::shared_ptr<XGLPlatform> instance = XGLPlatform::instance();
    STARFISH_ASSERT(instance->isValid());
    return *instance;
}

void XGLPlatform::update(const XGLPlatform platform)
{
    STARFISH_LOG_WARN("XGLPlatform::update");

    type = platform.type;
    context = platform.context;

    if (type == Type::EGL) {
        egl.display = platform.egl.display;
        egl.surface = platform.egl.surface;
        egl.config = platform.egl.config;
    } else if (type == Type::EVAS) {
        evasgl.object = platform.evasgl.object;
        evasgl.api = platform.evasgl.api;
        evasgl.surface = platform.evasgl.surface;
    }

    STARFISH_ASSERT(isValid());

#if defined(GL_BRIDGE_EVASGL)
    if (__evas_gl_glapi == nullptr) {
        __evas_gl_glapi = evasgl.api;
    }
#endif
}

bool XGLPlatform::isValid()
{
    if (type == Type::EGL) {
        if (!egl.display || !egl.surface || !egl.config) {
            return false;
        }
    } else if (type == Type::EVAS) {
        if (!evasgl.object || !evasgl.api || !evasgl.surface) {
            return false;
        }
    } else {
        return false;
    }

    if (!context) {
        return false;
    }

    return true;
}
