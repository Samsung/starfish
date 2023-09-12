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

#include "platform/canvas/webgl/GLContext.h"
#include "platform/canvas/webgl/EGLEnv.h"
#include "platform/canvas/webgl/EGLUtil.h"
#include "StarfishBase.h"

namespace Starfish {

// GLContext

GLContext::GLContext()
{
}

GLContext::GLContext(EGLContext context)
    : m_context(context)
{
}

bool GLContext::create(bool shareContext)
{
    STARFISH_ASSERT(m_context == nullptr);

    std::shared_ptr<EGLEnv> platform = EGLEnv::instance();

    if (!platform->isInitialzed()) {
        STARFISH_LOG_ERROR("Platform is not initialzed.");
        return false;
    }

    EGLDisplay display = platform->display();
    EGLSurface config = platform->config();
    EGLContext context = shareContext ? platform->context() : nullptr;
    EGLContext newContext;

    if (!EGLUtil::createEGLContext(newContext, display, config, context)) {
        return false;
    }
    m_context = newContext;
    return true;
}

bool GLContext::setCurrent()
{
    STARFISH_ASSERT(m_context != nullptr);

    EGLDisplay display = EGLEnv::instance()->display();
    EGLSurface surface = EGLEnv::instance()->surface();
    STARFISH_ASSERT(display != nullptr);
    STARFISH_ASSERT(surface != nullptr);

    bool result =
        EGLUtil::makeCurrentEGLContext(display, surface, surface, m_context);
    STARFISH_ASSERT(result);
    return result;
}

void GLContext::resetCurrent()
{
    EGLUtil::resetCurrentEGLContext(EGLEnv::instance()->display());
}

bool GLContext::destory()
{
    if (m_context) {
        if (!EGLUtil::destroyEGLContext(EGLEnv::instance()->display(),
                                        m_context)) {
            STARFISH_LOG_WARN("Context is not destoryed.");
            return false;
        }
        m_context = nullptr;
    }
    return true;
}

// GLContextScope

GLContextScope::GLContextScope(GLContext context)
{
    context.setCurrent();
}

GLContextScope::~GLContextScope()
{
    EGLUtil::resetCurrentEGLContext(EGLEnv::instance()->display());
}

// WebGLContextScope

WebGLContextScope::WebGLContextScope(GLContext context, GLuint fbo)
{
    context.setCurrent();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

WebGLContextScope::~WebGLContextScope()
{
    EGLUtil::resetCurrentEGLContext(EGLEnv::instance()->display());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

} // namespace Starfish

#endif
