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
#include "platform/canvas/webgl/GLEnv.h"
#include "platform/canvas/webgl/XGLUtil.h"
#include "StarfishBase.h"

namespace Starfish {

// GLContext

GLContext::GLContext()
{
}

GLContext::GLContext(XGLContext context)
    : m_context(context)
{
}

bool GLContext::create(bool shareContext)
{
    STARFISH_ASSERT(m_context == nullptr);

    std::shared_ptr<GLEnv> env = GLEnv::instance();

    if (!env->isInitialzed()) {
        STARFISH_LOG_ERROR("Platform is not initialzed.");
        return false;
    }

    XGLContext newContext;

    if (!XGLUtil::createXGLContext(newContext, env->platform(),
                                   shareContext ? env->context() : nullptr)) {
        return false;
    }
    m_context = newContext;
    return true;
}

bool GLContext::setCurrent()
{
    STARFISH_ASSERT(m_context != nullptr);

    bool result = XGLUtil::makeCurrentXGLContext(GLEnv::instance()->platform(),
                                                 m_context);
    STARFISH_ASSERT(result);
    return result;
}

void GLContext::resetCurrent()
{
    XGLUtil::resetCurrentXGLContext(GLEnv::instance()->platform());
}

bool GLContext::destory()
{
    if (m_context) {
        if (!XGLUtil::destroyXGLContext(GLEnv::instance()->platform(),
                                        m_context)) {
            STARFISH_LOG_WARN("Context is not destoryed.");
            return false;
        }
        m_context = nullptr;
    }
    return true;
}

void GLContext::reset()
{
    m_context = nullptr;
}

bool GLContext::isValid()
{
    return m_context != nullptr;
}

// GLContextScope

thread_local GLContext GLContextScope::currentContext;

GLContextScope::GLContextScope(GLContext context)
{
    context.setCurrent();

    STARFISH_ASSERT(currentContext.isValid() == false);
    currentContext = context;
}

GLContextScope::~GLContextScope()
{
    XGLUtil::resetCurrentXGLContext(GLEnv::instance()->platform());
    currentContext.reset();
}

GLContext GLContextScope::getCurrentXGLContext()
{
    return currentContext.isValid() ? currentContext : GLContext();
}

// GLRevertableContextScope

GLRevertableContextScope::GLRevertableContextScope(GLContext context)
{
    m_previousContext = GLContext(XGLUtil::getCurrentXGLContext());
    context.setCurrent();
}

GLRevertableContextScope::~GLRevertableContextScope()
{
    m_previousContext.setCurrent();
}

// WebGLContextScope

WebGLContextScope::WebGLContextScope(GLContext context, GLuint fbo)
{
    context.setCurrent();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

WebGLContextScope::~WebGLContextScope()
{
    XGLUtil::resetCurrentXGLContext(GLEnv::instance()->platform());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

} // namespace Starfish

#endif
