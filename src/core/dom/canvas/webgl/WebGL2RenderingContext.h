/*
 * Copyright (c) 2025-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishWebGL2RenderingContext__
#define __StarfishWebGL2RenderingContext__

#if defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#include "core/dom/canvas/webgl/WebGLRenderingContext.h"

typedef struct __GLsync* GLsync;

namespace Starfish {

class Uint32ArrayOrSequenceOfGLuint;

using Uint32List = Uint32ArrayOrSequenceOfGLuint;

class WebGLQuery : public WebGLObject {
public:
    WebGLQuery(ScriptBindingInstance* instance, WebGLRenderingContext* context,
               GLuint object);
    void init(ScriptBindingInstance* instance, void* domObjectPointer) override;
    bool isWebGLQuery() const override;
};

class WebGLSampler : public WebGLObject {
public:
    WebGLSampler(ScriptBindingInstance* instance,
                 WebGLRenderingContext* context, GLuint object);
    void init(ScriptBindingInstance* instance, void* domObjectPointer) override;
    bool isWebGLSampler() const override;
};

class WebGLSync : public WebGLObject {
public:
    WebGLSync(ScriptBindingInstance* instance, WebGLRenderingContext* context,
              GLsync object);
    void init(ScriptBindingInstance* instance, void* domObjectPointer) override;
    bool isWebGLSync() const override;

    GLsync glObject() const
    {
        return m_glObject;
    }

private:
    GLsync m_glObject;
};

class WebGLTransformFeedback : public WebGLObject {
public:
    WebGLTransformFeedback(ScriptBindingInstance* instance,
                           WebGLRenderingContext* context, GLuint object);
    void init(ScriptBindingInstance* instance, void* domObjectPointer) override;
    bool isWebGLTransformFeedback() const override;
};

class WebGLVertexArrayObject : public WebGLObject {
public:
    WebGLVertexArrayObject(ScriptBindingInstance* instance,
                           WebGLRenderingContext* context, GLuint object);
    void init(ScriptBindingInstance* instance, void* domObjectPointer) override;
    bool isWebGLVertexArrayObject() const override;
};

class WebGL2RenderingContext : public WebGLRenderingContext {
public:
    WebGL2RenderingContext(HTMLCanvasElement* canvasElement);
    ~WebGL2RenderingContext() override;

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(WebGL2RenderingContext);

    // Implement WebGLRenderingContextBase

    ScriptValue getParameter(GLenum pname);

    // Implement WebGL2RenderingContextOverloads

    // WebGL1:
    void bufferData(GLenum target, GLsizeiptr size, GLenum usage);
    void bufferData(GLenum target, Optional<AllowSharedBufferSource> srcData,
                    GLenum usage);
    void bufferSubData(GLenum target, GLintptr dstByteOffset,
                       AllowSharedBufferSource srcData);

    // WebGL1 legacy entrypoints:
    void texImage2D(GLenum target, GLint level, GLint internalformat,
                    GLsizei width, GLsizei height, GLint border, GLenum format,
                    GLenum type, Optional<ScriptArrayBufferView> pixels);
    void texImage2D(GLenum target, GLint level, GLint internalformat,
                    GLenum format, GLenum type, TexImageSource source);

    void texSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                       GLsizei width, GLsizei height, GLenum format,
                       GLenum type, Optional<ScriptArrayBufferView> pixels);
    void texSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                       GLenum format, GLenum type, TexImageSource source);

    /* Reading back pixels */
    // WebGL1:
    void readPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                    GLenum format, GLenum type,
                    Optional<ScriptArrayBufferView> dstData);
};

} // namespace Starfish

#endif // defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#endif // __StarfishWebGL2RenderingContext__
