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

    XGLPlatform platform = XGLPlatform::ref();

    XGLContext newContext;

    if (!XGLUtil::createXGLContext(newContext,
                                   shareContext ? platform.context : nullptr)) {
        return false;
    }
    m_context = newContext;
    return true;
}

bool GLContext::setCurrent()
{
    STARFISH_ASSERT(m_context != nullptr);

    bool result = XGLUtil::makeCurrentXGLContext(m_context);
    STARFISH_ASSERT(result);
    return result;
}

void GLContext::resetCurrent()
{
    XGLUtil::resetCurrentXGLContext();
}

bool GLContext::destory()
{
    if (m_context) {
        if (!XGLUtil::destroyXGLContext(m_context)) {
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
    XGLUtil::resetCurrentXGLContext();
    currentContext.reset();
}

GLContext GLContextScope::getCurrentXGLContext()
{
    return currentContext.isValid() ? currentContext : GLContext();
}

// GLRevertableContextScope

GLRevertableContextScope::GLRevertableContextScope(GLContext context)
{
    context.setCurrent();
}

GLRevertableContextScope::~GLRevertableContextScope()
{
    // We assume that the previous context is always the main context. EvasGL
    // does not have an API to get the current GL context, so we cannot have a
    // unified API to know the current context unless we unify the API to change
    // context across the codebase.
    XGLUtil::makeCurrentXGLContext(XGLPlatform::ref().context);
}

// WebGLContextScope

WebGLContextScope::WebGLContextScope(GLContext context, GLuint fbo)
{
    context.setCurrent();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

WebGLContextScope::~WebGLContextScope()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    XGLUtil::resetCurrentXGLContext();
}

} // namespace Starfish

#endif
