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
#include "WebGLANGLE_InstancedArrays.h"
#include "core/dom/canvas/webgl/WebGLRenderingContext.h"

#include "platform/canvas/gl/IncludeGL.h"
#include "platform/canvas/gl/GL.h"

namespace Starfish {

ANGLE_instanced_arrays::ANGLE_instanced_arrays(ScriptBindingInstance* instance,
                                               WebGLRenderingContext* context)
    : ScriptWrappable(this)
    , m_scriptBindingInstance(instance)
    , m_context(context)
{
}

// The ANGLE entry points are the WebGL 2 ones under another name: the
// validation and the GL call are shared with the context implementation.
void ANGLE_instanced_arrays::drawArraysInstancedANGLE(GLenum mode, GLint first,
                                                      GLsizei count,
                                                      GLsizei primcount)
{
    m_context->drawArraysInstanced(mode, first, count, primcount);
}

void ANGLE_instanced_arrays::drawElementsInstancedANGLE(
    GLenum mode, GLsizei count, GLenum type, GLintptr offset, GLsizei primcount)
{
    m_context->drawElementsInstanced(mode, count, type, offset, primcount);
}

void ANGLE_instanced_arrays::vertexAttribDivisorANGLE(GLuint index,
                                                      GLuint divisor)
{
    m_context->vertexAttribDivisor(index, divisor);
}

} // namespace Starfish

#endif
