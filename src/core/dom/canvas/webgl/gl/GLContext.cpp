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

#include "core/dom/canvas/webgl/gl/GLContext.h"
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
    STARFISH_ASSERT(m_renderer != nullptr);

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
    STARFISH_ASSERT(m_renderer != nullptr);

    if (m_context == m_renderer->getCurrentContext()) {
        // NOTE: As a performance safeguard, the makeCurrent call is skipped in
        // advance in this condition. It's worth considering moving this into
        // the Renderer logic.
        return true;
    }
    return m_renderer->makeCurrentWithContext(m_context);
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
    // FIXME: Here assumes that the previous context is always the main context.
    // We may use eglGetCurrentContext, evas_gl_current_context_get, or
    // something for this through the Renderer.
    m_renderer->makeCurrent();
}

} // namespace Starfish

#endif
