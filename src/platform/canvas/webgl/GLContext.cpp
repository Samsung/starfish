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

#include "StarfishConfig.h"

#if defined(STARFISH_ENABLE_WEBGL)

#include "platform/canvas/webgl/GLContext.h"
#include "core/modules/renderer/Renderer.h"

namespace Starfish {

// GLContext

GLContext::GLContext()
{
}

GLContext::GLContext(Renderer* renderer)
    : m_context(UINTPTR_MAX)
    , m_renderer(renderer)
{
}

bool GLContext::createSharedContext()
{
    STARFISH_ASSERT(m_context == UINTPTR_MAX);
    uintptr_t newContext = m_renderer->createSharedContext();
    if (newContext == UINTPTR_MAX) {
        return false;
    }
    m_context = newContext;
    return true;
}

bool GLContext::setCurrent()
{
    STARFISH_ASSERT(m_context != UINTPTR_MAX);

    return m_renderer->makeCurrentWithContext(m_context);
}

void GLContext::resetCurrent()
{
    m_renderer->clearCurrentContext();
}

bool GLContext::destory()
{
    if (m_context != UINTPTR_MAX) {
        if (!m_renderer->destroyContext(m_context)) {
            STARFISH_LOG_WARN("Context is not destoryed.");
            return false;
        }
        m_context = UINTPTR_MAX;
    }
    return true;
}

void GLContext::reset()
{
    m_context = UINTPTR_MAX;
}

bool GLContext::isValid()
{
    return m_context != UINTPTR_MAX;
}

// GLContextScope

thread_local GLContext GLContextScope::currentContext;

GLContextScope::GLContextScope(GLContext context)
{
    m_result = context.setCurrent();

    STARFISH_ASSERT(currentContext.isValid() == false);
    currentContext = context;
}

GLContextScope::~GLContextScope()
{
    if (m_result) {
        currentContext.resetCurrent();
    }
    currentContext.reset();
}

GLContext GLContextScope::getCurrentGLContext()
{
    return currentContext.isValid() ? currentContext : GLContext();
}

// GLRevertableContextScope

GLRevertableContextScope::GLRevertableContextScope(GLContext context,
                                                   Renderer* renderer)
    : m_renderer(renderer)
{
    context.setCurrent();
}

GLRevertableContextScope::~GLRevertableContextScope()
{
    // We assume that the previous context is always the main context. EvasGL
    // does not have an API to get the current GL context, so we cannot have a
    // unified API to know the current context unless we unify the API to change
    // context across the codebase.
    m_renderer->makeCurrent();
}

} // namespace Starfish

#endif
