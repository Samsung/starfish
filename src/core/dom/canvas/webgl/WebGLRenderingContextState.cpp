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
#include "core/dom/canvas/webgl/WebGL2RenderingContext.h"
#include "core/dom/canvas/webgl/WebGLOES_VertexArrayObject.h"

namespace Starfish {

WebGLRenderingContextState::AttribTypes::AttribTypes()
{
    for (size_t i = 0; i < kMaxVertexAttribs; ++i) {
        types[i] = GL_FLOAT;
    }
}

WebGLRenderingContextState::WebGLRenderingContextState()
{
    for (size_t i = 0; i < kMaxVertexAttribs; ++i) {
        m_genericVertexAttribType[i] = GL_FLOAT;
    }
}

std::unordered_set<GLuint> WebGLRenderingContextState::arraysEnabled()
{
    GLuint vao = vertexArray();
    return m_arraysEnabled[vao];
}

void WebGLRenderingContextState::disableVertexAttribArray(GLuint index)
{
    GLuint vao = vertexArray();
    m_arraysEnabled[vao].erase(index);
}

void WebGLRenderingContextState::enableVertexAttribArray(GLuint index)
{
    GLuint vao = vertexArray();
    m_arraysEnabled[vao].insert(index);
}

Optional<WebGLBuffer*>
WebGLRenderingContextState::getBufferBoundToVertexAttributes(GLuint index)
{
    return getVertexAttribPointer(index).buffer;
}

Optional<WebGLBuffer*> WebGLRenderingContextState::getBoundBuffer(GLuint target)
{
    if (target == GL_ELEMENT_ARRAY_BUFFER) {
        GLuint vao = vertexArray();
        const auto& iter = m_elementArrayBuffers.find(vao);
        if (iter == m_elementArrayBuffers.end()) {
            return nullptr;
        }
        STARFISH_ASSERT(iter->second != nullptr);
        return iter->second;
    }

    const auto& iter = m_globalBuffersBound.find(target);
    if (iter == m_globalBuffersBound.end()) {
        return nullptr;
    }

    STARFISH_ASSERT(iter->second != nullptr);
    return iter->second;
}

void WebGLRenderingContextState::setBoundBuffer(GLenum target,
                                                Optional<WebGLBuffer*> maybe)
{
    if (target == GL_ELEMENT_ARRAY_BUFFER) {
        GLuint vao = vertexArray();
        if (maybe.hasValue()) {
            m_elementArrayBuffers[vao] = maybe.value();
        } else {
            m_elementArrayBuffers.erase(vao);
        }
        return;
    }

    if (maybe.hasValue()) {
        m_globalBuffersBound[target] = maybe.value();
    } else {
        m_globalBuffersBound.erase(target);
    }
}

void WebGLRenderingContextState::deleteVertexArray(GLuint vao)
{
    STARFISH_ASSERT(vao != 0);
    if (m_webGLVertexArrayObject.hasValue() &&
        m_webGLVertexArrayObject.value()->glObject() == vao) {
        m_webGLVertexArrayObject = nullptr;
    }
    m_arraysEnabled.erase(vao);
    m_elementArrayBuffers.erase(vao);
    m_vertexAttribType.erase(vao);
    m_vertexAttribPointer.erase(vao);
}

void WebGLRenderingContextState::deleteVertexArrayOES(GLuint vao)
{
    STARFISH_ASSERT(vao != 0);
    if (m_webGLVertexArrayObjectOES.hasValue() &&
        m_webGLVertexArrayObjectOES.value()->glObject() == vao) {
        m_webGLVertexArrayObjectOES = nullptr;
    }
    m_arraysEnabled.erase(vao);
    m_elementArrayBuffers.erase(vao);
    m_vertexAttribType.erase(vao);
    m_vertexAttribPointer.erase(vao);
}

void WebGLRenderingContextState::unbindBuffer(WebGLBuffer* buffer)
{
    if (!buffer) {
        return;
    }
    for (auto it = m_globalBuffersBound.begin();
         it != m_globalBuffersBound.end();) {
        if (it->second == buffer) {
            it = m_globalBuffersBound.erase(it);
        } else {
            ++it;
        }
    }
    GLuint vao = vertexArray();
    auto iter = m_elementArrayBuffers.find(vao);
    if (iter != m_elementArrayBuffers.end() && iter->second == buffer) {
        m_elementArrayBuffers.erase(iter);
    }
    if (m_vertexAttribPointer.count(vao)) {
        for (AttribPointer& pointer : m_vertexAttribPointer[vao].pointers) {
            if (pointer.buffer.hasValue() && pointer.buffer.value() == buffer) {
                pointer.buffer = nullptr;
            }
        }
    }
}

GLuint WebGLRenderingContextState::vertexArray()
{
    if (m_webGLVertexArrayObject.hasValue()) {
        return m_webGLVertexArrayObject.value()->glObject();
    }
    if (m_webGLVertexArrayObjectOES.hasValue()) {
        return m_webGLVertexArrayObjectOES.value()->glObject();
    }
    return 0;
}

bool WebGLRenderingContextState::isVertexAttribArrayEnabled(GLuint index)
{
    GLuint vao = vertexArray();
    const auto& set = m_arraysEnabled[vao];
    return set.find(index) != set.end();
}

GLenum WebGLRenderingContextState::getVertexAttribType(GLuint index)
{
    if (index >= kMaxVertexAttribs) {
        return GL_FLOAT;
    }
    GLuint vao = vertexArray();
    const auto& iter = m_vertexAttribType.find(vao);
    if (iter == m_vertexAttribType.end()) {
        return GL_FLOAT;
    }
    return iter->second.types[index];
}

void WebGLRenderingContextState::setVertexAttribType(GLuint index, GLenum type)
{
    if (index >= kMaxVertexAttribs) {
        return;
    }
    GLuint vao = vertexArray();
    m_vertexAttribType[vao].types[index] = type;
}

WebGLRenderingContextState::AttribPointer
WebGLRenderingContextState::getVertexAttribPointer(GLuint index)
{
    if (index >= kMaxVertexAttribs) {
        return AttribPointer();
    }
    GLuint vao = vertexArray();
    const auto& iter = m_vertexAttribPointer.find(vao);
    if (iter == m_vertexAttribPointer.end()) {
        return AttribPointer();
    }
    return iter->second.pointers[index];
}

void WebGLRenderingContextState::setVertexAttribPointer(
    GLuint index, Optional<WebGLBuffer*> buffer, GLint size, GLenum type,
    GLsizei stride, GLintptr offset)
{
    if (index >= kMaxVertexAttribs) {
        return;
    }
    GLuint vao = vertexArray();
    AttribPointer& pointer = m_vertexAttribPointer[vao].pointers[index];
    pointer.buffer = buffer;
    pointer.size = size;
    pointer.type = type;
    pointer.stride = stride;
    pointer.offset = offset;
}

void WebGLRenderingContextState::setVertexAttribDivisor(GLuint index,
                                                        GLuint divisor)
{
    if (index >= kMaxVertexAttribs) {
        return;
    }
    GLuint vao = vertexArray();
    m_vertexAttribPointer[vao].pointers[index].divisor = divisor;
}

GLenum WebGLRenderingContextState::getGenericVertexAttribType(GLuint index)
{
    if (index >= kMaxVertexAttribs) {
        return GL_FLOAT;
    }
    return m_genericVertexAttribType[index];
}

void WebGLRenderingContextState::setGenericVertexAttribType(GLuint index,
                                                            GLenum type)
{
    if (index < kMaxVertexAttribs) {
        m_genericVertexAttribType[index] = type;
    }
}

} // namespace Starfish

#endif
