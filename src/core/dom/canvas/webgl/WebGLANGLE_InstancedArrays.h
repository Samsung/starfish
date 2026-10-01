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

#ifndef __StarfishWebGLANGLE_InstancedArrays__
#define __StarfishWebGLANGLE_InstancedArrays__

#if defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#include "binding/ScriptWrappable.h"
#include "core/util/GCDescriptor.h"
#include "platform/canvas/gl/GLTypes.h"

namespace Starfish {

class WebGLRenderingContext;

// References:
// binding/generated/ANGLE_instanced_arraysBinding.cpp

class ANGLE_instanced_arrays : public ScriptWrappable {
public:
    ANGLE_instanced_arrays(ScriptBindingInstance* instance,
                           WebGLRenderingContext* context);
    void init(ScriptBindingInstance* instance, void* domObjectPointer) override;
    bool isANGLE_instanced_arrays() const override;
    ScriptBindingInstance* scriptBindingInstance() override
    {
        return m_scriptBindingInstance;
    }

    void drawArraysInstancedANGLE(GLenum mode, GLint first, GLsizei count,
                                  GLsizei primcount);
    void drawElementsInstancedANGLE(GLenum mode, GLsizei count, GLenum type,
                                    GLintptr offset, GLsizei primcount);
    void vertexAttribDivisorANGLE(GLuint index, GLuint divisor);

    BEGIN_IMPLEMENT_NEW_WITH_GC_DESC(ANGLE_instanced_arrays);
    FILL_GC_POINTER(ANGLE_instanced_arrays, m_context);
    END_IMPLEMENT_NEW_WITH_GC_DESC();

private:
    ScriptBindingInstance* m_scriptBindingInstance;
    WebGLRenderingContext* m_context;
};

} // namespace Starfish

#endif

#endif
