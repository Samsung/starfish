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

#if defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#include "StarfishConfig.h"
#include "WebGLRenderingContextState.h"
#include "core/dom/canvas/webgl/WebGLOES_VertexArrayObject.h"

namespace Starfish {

WebGLRenderingContextState::WebGLRenderingContextState()
{
}

Optional<WebGLBuffer*>
WebGLRenderingContextState::getBufferBoundToVertexAttributes(GLuint index)
{
    GLuint vao = vertexArray();
    const auto& iter = m_buffersBoundToVertexAttributes[vao].find(index);
    if (iter == m_buffersBoundToVertexAttributes[vao].end()) {
        return nullptr;
    }

    STARFISH_ASSERT(iter->second != nullptr);
    return iter->second;
}

void WebGLRenderingContextState::setBufferBoundToVertexAttributes(
    GLuint index, Optional<WebGLBuffer*> maybe)
{
    GLuint vao = vertexArray();
    if (maybe.hasValue()) {
        m_buffersBoundToVertexAttributes[vao][index] = maybe.value();
    } else {
        m_buffersBoundToVertexAttributes[vao].erase(index);
    }
}

Optional<WebGLBuffer*> WebGLRenderingContextState::getBoundBuffer(GLuint target)
{
    GLuint vao = vertexArray();
    const auto& iter = m_buffersBound[vao].find(target);
    if (iter == m_buffersBound[vao].end()) {
        return nullptr;
    }

    STARFISH_ASSERT(iter->second != nullptr);
    return iter->second;
}

void WebGLRenderingContextState::setBoundBuffer(GLenum target,
                                                Optional<WebGLBuffer*> maybe)
{
    GLuint vao = vertexArray();
    if (maybe.hasValue()) {
        m_buffersBound[vao][target] = maybe.value();
    } else {
        m_buffersBound[vao].erase(target);
    }
}

void WebGLRenderingContextState::deleteVertexArrayOES(GLuint vao)
{
    STARFISH_ASSERT(vao != 0);
    if (m_webGLVertexArrayObjectOES.hasValue() &&
        m_webGLVertexArrayObjectOES.value()->glObject() == vao) {
        m_webGLVertexArrayObjectOES = nullptr;
    }
    m_buffersBound.erase(vao);
    m_buffersBoundToVertexAttributes.erase(vao);
}

GLuint WebGLRenderingContextState::vertexArray()
{
    if (m_webGLVertexArrayObjectOES.hasValue()) {
        return m_webGLVertexArrayObjectOES.value()->glObject();
    }
    return 0;
}

} // namespace Starfish

#endif
