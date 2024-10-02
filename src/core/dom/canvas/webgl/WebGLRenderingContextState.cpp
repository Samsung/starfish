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

namespace Starfish {

WebGLRenderingContextState::WebGLRenderingContextState()
{
}

Optional<WebGLBuffer*>
WebGLRenderingContextState::getBufferBoundToVertexAttributes(GLuint index)
{
    const auto& iter = m_buffersBoundToVertexAttributes.find(index);
    if (iter == m_buffersBoundToVertexAttributes.end()) {
        return nullptr;
    }

    STARFISH_ASSERT(iter->second != nullptr);
    return iter->second;
}

void WebGLRenderingContextState::setBufferBoundToVertexAttributes(
    GLuint index, Optional<WebGLBuffer*> maybe)
{
    if (maybe.hasValue()) {
        m_buffersBoundToVertexAttributes.insert_or_assign(index, maybe.value());
    } else {
        m_buffersBoundToVertexAttributes.erase(index);
    }
}

Optional<WebGLBuffer*> WebGLRenderingContextState::getBoundBuffer(GLuint target)
{
    const auto& iter = m_buffersBound.find(target);
    if (iter == m_buffersBound.end()) {
        return nullptr;
    }

    STARFISH_ASSERT(iter->second != nullptr);
    return iter->second;
}

void WebGLRenderingContextState::setBoundBuffer(GLenum target,
                                                Optional<WebGLBuffer*> maybe)
{
    if (maybe.hasValue()) {
        m_buffersBound.insert_or_assign(target, maybe.value());
    } else {
        m_buffersBound.erase(target);
    }
}

} // namespace Starfish

#endif
