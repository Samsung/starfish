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

#ifndef __StarfishWebGLRenderingContextState__
#define __StarfishWebGLRenderingContextState__

#if defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#include "StarfishBase.h"
#include "platform/canvas/gl/GLTypes.h"
#include "platform/canvas/gl/IncludeGL.h"

namespace Starfish {

class WebGLBuffer;

#define STATEFUL_VALUES(V)                                             \
    V(WebGLVertexArrayObject, Optional<WebGLVertexArrayObject*>,       \
      webGLVertexArrayObject)                                          \
    V(WebGLVertexArrayObjectOES, Optional<WebGLVertexArrayObjectOES*>, \
      webGLVertexArrayObjectOES)                                       \
    V(WebGLFramebuffer, Optional<WebGLFramebuffer*>, webGLFramebuffer) \
    V(WebGLProgram, Optional<WebGLProgram*>, webGLProgram)

// Forward declarations
#define V(Constructor, _, __) class Constructor;
STATEFUL_VALUES(V);
#undef V

class WebGLRenderingContextState : public gc {
public:
    WebGLRenderingContextState();

// Define functions
#define V(Constructor, Type, MemberName)          \
    DEFINE_GETTER(Type, MemberName);              \
    DEFINE_SETTER(Type, MemberName, Constructor); \
    bool has##Constructor()                       \
    {                                             \
        return m_##MemberName.hasValue();         \
    }

    STATEFUL_VALUES(V);
#undef V

    std::unordered_set<GLuint> arraysEnabled();
    void disableVertexAttribArray(GLuint index);
    void enableVertexAttribArray(GLuint index);

    Optional<WebGLBuffer*> getBoundBuffer(GLuint target);
    void setBoundBuffer(GLenum target, Optional<WebGLBuffer*> maybe);

    Optional<WebGLBuffer*> getBufferBoundToVertexAttributes(GLuint index);

    void deleteVertexArray(GLuint vao);
    void deleteVertexArrayOES(GLuint vao);

    void unbindBuffer(WebGLBuffer* buffer);

    static constexpr size_t kMaxVertexAttribs = 32;
    struct AttribTypes {
        GLenum types[kMaxVertexAttribs];
        AttribTypes();
    };

    // What vertexAttribPointer and vertexAttribDivisor set for one array:
    // the buffer it reads and how, kept so a draw can be checked against
    // it. The buffer belongs in this record rather than in a std container
    // of its own because the table below is GC-allocated and scanned, which
    // is what keeps a buffer that script has dropped alive for as long as
    // an array still reads it.
    struct AttribPointer {
        Optional<WebGLBuffer*> buffer;
        GLintptr offset = 0;
        GLint size = 4;
        GLsizei stride = 0;
        GLenum type = GL_FLOAT;
        GLuint divisor = 0;
    };
    struct AttribPointers {
        AttribPointer pointers[kMaxVertexAttribs];
    };

    bool isVertexAttribArrayEnabled(GLuint index);
    GLenum getVertexAttribType(GLuint index);
    void setVertexAttribType(GLuint index, GLenum type);
    AttribPointer getVertexAttribPointer(GLuint index);
    void setVertexAttribPointer(GLuint index, Optional<WebGLBuffer*> buffer,
                                GLint size, GLenum type, GLsizei stride,
                                GLintptr offset);
    void setVertexAttribDivisor(GLuint index, GLuint divisor);
    GLenum getGenericVertexAttribType(GLuint index);
    void setGenericVertexAttribType(GLuint index, GLenum type);

private:
    GLuint vertexArray();

    // Define variables
#define V(Constructor, Type, MemberName) Type m_##MemberName;
    STATEFUL_VALUES(V);
#undef V

    GCUnorderedMap<GLuint, std::unordered_set<GLuint>> m_arraysEnabled;
    GCUnorderedMap<GLuint, WebGLBuffer*> m_elementArrayBuffers;
    GCUnorderedMap<GLenum, WebGLBuffer*> m_globalBuffersBound;
    GCUnorderedMap<GLuint, AttribTypes> m_vertexAttribType;
    GCUnorderedMap<GLuint, AttribPointers> m_vertexAttribPointer;
    GLenum m_genericVertexAttribType[kMaxVertexAttribs];
};
} // namespace Starfish

#endif
#endif
