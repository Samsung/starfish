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
#include "platform/canvas/gl/IncludeGL.h"

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

    bool hasEverBound()
    {
        return m_hasEverBound;
    }

    void setHasEverBound()
    {
        m_hasEverBound = true;
    }

private:
    bool m_hasEverBound = false;
};

class WebGL2RenderingContext : public WebGLRenderingContext {
public:
    WebGL2RenderingContext(HTMLCanvasElement* canvasElement);
    ~WebGL2RenderingContext() override;

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(WebGL2RenderingContext);

    BEGIN_IMPLEMENT_NEW_WITH_GC_DESC(WebGL2RenderingContext,
                                     WebGLRenderingContext);
    FILL_GC_POINTER(WebGL2RenderingContext, m_uniformBufferBindings);
    END_IMPLEMENT_NEW_WITH_GC_DESC();

public:
    // Implement WebGLRenderingContextBase

    void bindBuffer(GLenum target, Optional<WebGLBuffer*> buffer);
    void bindFramebuffer(GLenum target, Optional<WebGLFramebuffer*> buffer);
    void bindTexture(GLenum target, Optional<WebGLTexture*> texture);

    ScriptValue getBufferParameter(GLenum target, GLenum pname);
    ScriptValue getParameter(GLenum pname);

    ScriptValue getProgramParameter(WebGLProgram* program, GLenum pname);

    ScriptValue getTexParameter(GLenum target, GLenum pname);

    ScriptValue getUniform(WebGLProgram* program,
                           WebGLUniformLocation* location);

    ScriptValue getVertexAttrib(GLuint index, GLenum pname);

    void texParameterf(GLenum target, GLenum pname, GLfloat param);
    void texParameteri(GLenum target, GLenum pname, GLint param);

    void vertexAttrib1f(GLuint index, GLfloat x);
    void vertexAttrib2f(GLuint index, GLfloat x, GLfloat y);
    void vertexAttrib3f(GLuint index, GLfloat x, GLfloat y, GLfloat z);
    void vertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z,
                        GLfloat w);

    void vertexAttrib1fv(GLuint index, Float32List values);
    void vertexAttrib2fv(GLuint index, Float32List values);
    void vertexAttrib3fv(GLuint index, Float32List values);
    void vertexAttrib4fv(GLuint index, Float32List values);

    // Implement WebGL2RenderingContextBase

    /* Buffer objects */
    void copyBufferSubData(GLenum readTarget, GLenum writeTarget,
                           GLintptr readOffset, GLintptr writeOffset,
                           GLsizeiptr size);
    void getBufferSubData(GLenum target, GLintptr srcByteOffset,
                          ScriptArrayBufferView dstBuffer,
                          unsigned long long dstOffset = 0, GLuint length = 0);

    /* Programs and shaders */
    GLint getFragDataLocation(WebGLProgram* program, String* name);

    /* Uniforms */
    void uniform1ui(Optional<WebGLUniformLocation*> location, GLuint v0);
    void uniform2ui(Optional<WebGLUniformLocation*> location, GLuint v0,
                    GLuint v1);
    void uniform3ui(Optional<WebGLUniformLocation*> location, GLuint v0,
                    GLuint v1, GLuint v2);
    void uniform4ui(Optional<WebGLUniformLocation*> location, GLuint v0,
                    GLuint v1, GLuint v2, GLuint v3);

    void uniform1uiv(Optional<WebGLUniformLocation*> location, Uint32List data,
                     unsigned long long srcOffset = 0, GLuint srcLength = 0);
    void uniform2uiv(Optional<WebGLUniformLocation*> location, Uint32List data,
                     unsigned long long srcOffset = 0, GLuint srcLength = 0);
    void uniform3uiv(Optional<WebGLUniformLocation*> location, Uint32List data,
                     unsigned long long srcOffset = 0, GLuint srcLength = 0);
    void uniform4uiv(Optional<WebGLUniformLocation*> location, Uint32List data,
                     unsigned long long srcOffset = 0, GLuint srcLength = 0);

    void uniformMatrix3x2fv(Optional<WebGLUniformLocation*> location,
                            GLboolean transpose, Float32List data,
                            unsigned long long srcOffset = 0,
                            GLuint srcLength = 0);
    void uniformMatrix4x2fv(Optional<WebGLUniformLocation*> location,
                            GLboolean transpose, Float32List data,
                            unsigned long long srcOffset = 0,
                            GLuint srcLength = 0);

    void uniformMatrix2x3fv(Optional<WebGLUniformLocation*> location,
                            GLboolean transpose, Float32List data,
                            unsigned long long srcOffset = 0,
                            GLuint srcLength = 0);
    void uniformMatrix4x3fv(Optional<WebGLUniformLocation*> location,
                            GLboolean transpose, Float32List data,
                            unsigned long long srcOffset = 0,
                            GLuint srcLength = 0);

    void uniformMatrix2x4fv(Optional<WebGLUniformLocation*> location,
                            GLboolean transpose, Float32List data,
                            unsigned long long srcOffset = 0,
                            GLuint srcLength = 0);
    void uniformMatrix3x4fv(Optional<WebGLUniformLocation*> location,
                            GLboolean transpose, Float32List data,
                            unsigned long long srcOffset = 0,
                            GLuint srcLength = 0);

    /* Vertex attribs */
    void vertexAttribI4i(GLuint index, GLint x, GLint y, GLint z, GLint w);
    void vertexAttribI4iv(GLuint index, Int32List values);
    void vertexAttribI4ui(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w);
    void vertexAttribI4uiv(GLuint index, Uint32List values);
    void vertexAttribIPointer(GLuint index, GLint size, GLenum type,
                              GLsizei stride, GLintptr offset);

    /* Sync objects */
    Optional<WebGLSync*> fenceSync(GLenum condition, GLbitfield flags);
    GLboolean isSync(Optional<WebGLSync*> sync);
    void deleteSync(Optional<WebGLSync*> sync);
    GLenum clientWaitSync(WebGLSync* sync, GLbitfield flags, GLuint64 timeout);
    void waitSync(WebGLSync* sync, GLbitfield flags, GLint64 timeout);
    ScriptValue getSyncParameter(WebGLSync* sync, GLenum pname);

    /* Uniform Buffer Objects and Transform Feedback Buffers */
    void bindBufferBase(GLenum target, GLuint index,
                        Optional<WebGLBuffer*> buffer);
    void bindBufferRange(GLenum target, GLuint index,
                         Optional<WebGLBuffer*> buffer, GLintptr offset,
                         GLsizeiptr size);
    ScriptValue getActiveUniforms(WebGLProgram* program,
                                  GCAtomicVector<GLuint> uniformIndices,
                                  GLenum pname);
    Optional<GCAtomicVector<GLuint>> getUniformIndices(
        WebGLProgram* program, GCVector<String*> uniformNames);
    GLuint getUniformBlockIndex(WebGLProgram* program,
                                String* uniformBlockName);
    ScriptValue getActiveUniformBlockParameter(WebGLProgram* program,
                                               GLuint uniformBlockIndex,
                                               GLenum pname);
    String* getActiveUniformBlockName(WebGLProgram* program,
                                      GLuint uniformBlockIndex);
    void uniformBlockBinding(WebGLProgram* program, GLuint uniformBlockIndex,
                             GLuint uniformBlockBinding);

    /* Vertex Array Objects */
    WebGLVertexArrayObject* createVertexArray();
    void deleteVertexArray(Optional<WebGLVertexArrayObject*> vertexArray);
    GLboolean isVertexArray(Optional<WebGLVertexArrayObject*> vertexArray);
    void bindVertexArray(Optional<WebGLVertexArrayObject*> array);
    void vertexAttribDivisor(GLuint index, GLuint divisor);
    void drawArraysInstanced(GLenum mode, GLint first, GLsizei count,
                             GLsizei instanceCount);
    void drawElementsInstanced(GLenum mode, GLsizei count, GLenum type,
                               GLintptr offset, GLsizei instanceCount);
    void drawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count,
                           GLenum type, GLintptr offset);

    /* Framebuffer objects */
    void blitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                         GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                         GLbitfield mask, GLenum filter);
    void framebufferTextureLayer(GLenum target, GLenum attachment,
                                 Optional<WebGLTexture*> texture, GLint level,
                                 GLint layer);
    void invalidateFramebuffer(GLenum target,
                               GCAtomicVector<GLenum> attachments);
    void invalidateSubFramebuffer(GLenum target,
                                  GCAtomicVector<GLenum> attachments, GLint x,
                                  GLint y, GLsizei width, GLsizei height);
    void readBuffer(GLenum src);

    /* Renderbuffer objects */
    void renderbufferStorageMultisample(GLenum target, GLsizei samples,
                                        GLenum internalformat, GLsizei width,
                                        GLsizei height);

    /* Multiple Render Targets & Clear Buffers */
    void drawBuffers(GCAtomicVector<GLenum> buffers);
    void clearBufferfv(GLenum buffer, GLint drawbuffer, Float32List values,
                       unsigned long long srcOffset = 0);
    void clearBufferiv(GLenum buffer, GLint drawbuffer, Int32List values,
                       unsigned long long srcOffset = 0);
    void clearBufferuiv(GLenum buffer, GLint drawbuffer, Uint32List values,
                        unsigned long long srcOffset = 0);
    void clearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth,
                       GLint stencil);

    // Implement WebGL2RenderingContextOverloads

    void shaderSource(WebGLShader* shader, String* source) override;
    bool validateDrawCallUBO() override;

    // WebGL1:
    void bufferData(GLenum target, GLsizeiptr size, GLenum usage);
    void bufferData(GLenum target, Optional<AllowSharedBufferSource> srcData,
                    GLenum usage);
    void bufferSubData(GLenum target, GLintptr dstByteOffset,
                       AllowSharedBufferSource srcData);

    // WebGL2:
    void bufferData(GLenum target, ScriptArrayBufferView srcData, GLenum usage,
                    unsigned long long srcOffset, GLuint length = 0);
    void bufferSubData(GLenum target, GLintptr dstByteOffset,
                       ScriptArrayBufferView srcData,
                       unsigned long long srcOffset, GLuint length = 0);

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

    // WebGL2 entrypoints:
    void texStorage2D(GLenum target, GLsizei levels, GLenum internalformat,
                      GLsizei width, GLsizei height);
    void texStorage3D(GLenum target, GLsizei levels, GLenum internalformat,
                      GLsizei width, GLsizei height, GLsizei depth);

    void texImage2D(GLenum target, GLint level, GLint internalformat,
                    GLsizei width, GLsizei height, GLint border, GLenum format,
                    GLenum type, GLintptr pboOffset);
    void texImage2D(GLenum target, GLint level, GLint internalformat,
                    GLsizei width, GLsizei height, GLint border, GLenum format,
                    GLenum type, TexImageSource source);
    void texImage2D(GLenum target, GLint level, GLint internalformat,
                    GLsizei width, GLsizei height, GLint border, GLenum format,
                    GLenum type, ScriptArrayBufferView srcData,
                    unsigned long long srcOffset);

    void texImage3D(GLenum target, GLint level, GLint internalformat,
                    GLsizei width, GLsizei height, GLsizei depth, GLint border,
                    GLenum format, GLenum type, GLintptr pboOffset);
    void texImage3D(GLenum target, GLint level, GLint internalformat,
                    GLsizei width, GLsizei height, GLsizei depth, GLint border,
                    GLenum format, GLenum type, TexImageSource source);
    void texImage3D(GLenum target, GLint level, GLint internalformat,
                    GLsizei width, GLsizei height, GLsizei depth, GLint border,
                    GLenum format, GLenum type,
                    Optional<ScriptArrayBufferView> srcData);
    void texImage3D(GLenum target, GLint level, GLint internalformat,
                    GLsizei width, GLsizei height, GLsizei depth, GLint border,
                    GLenum format, GLenum type, ScriptArrayBufferView srcData,
                    unsigned long long srcOffset);

    void texSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                       GLint zoffset, GLsizei width, GLsizei height,
                       GLsizei depth, GLenum format, GLenum type,
                       GLintptr pboOffset);
    void texSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                       GLint zoffset, GLsizei width, GLsizei height,
                       GLsizei depth, GLenum format, GLenum type,
                       TexImageSource source);
    void texSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                       GLint zoffset, GLsizei width, GLsizei height,
                       GLsizei depth, GLenum format, GLenum type,
                       Optional<ScriptArrayBufferView> srcData,
                       unsigned long long srcOffset);

    void texSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                       GLsizei width, GLsizei height, GLenum format,
                       GLenum type, GLintptr pboOffset);
    void texSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                       GLsizei width, GLsizei height, GLenum format,
                       GLenum type, TexImageSource source);
    void texSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                       GLsizei width, GLsizei height, GLenum format,
                       GLenum type, ScriptArrayBufferView srcData,
                       unsigned long long srcOffset);

    void uniform1fv(Optional<WebGLUniformLocation*> location, Float32List data,
                    unsigned long long srcOffset = 0, GLuint srcLength = 0);
    void uniform2fv(Optional<WebGLUniformLocation*> location, Float32List data,
                    unsigned long long srcOffset = 0, GLuint srcLength = 0);
    void uniform3fv(Optional<WebGLUniformLocation*> location, Float32List data,
                    unsigned long long srcOffset = 0, GLuint srcLength = 0);
    void uniform4fv(Optional<WebGLUniformLocation*> location, Float32List data,
                    unsigned long long srcOffset = 0, GLuint srcLength = 0);

    void uniform1iv(Optional<WebGLUniformLocation*> location, Int32List data,
                    unsigned long long srcOffset = 0, GLuint srcLength = 0);
    void uniform2iv(Optional<WebGLUniformLocation*> location, Int32List data,
                    unsigned long long srcOffset = 0, GLuint srcLength = 0);
    void uniform3iv(Optional<WebGLUniformLocation*> location, Int32List data,
                    unsigned long long srcOffset = 0, GLuint srcLength = 0);
    void uniform4iv(Optional<WebGLUniformLocation*> location, Int32List data,
                    unsigned long long srcOffset = 0, GLuint srcLength = 0);

    void uniformMatrix2fv(Optional<WebGLUniformLocation*> location,
                          GLboolean transpose, Float32List data,
                          unsigned long long srcOffset = 0,
                          GLuint srcLength = 0);
    void uniformMatrix3fv(Optional<WebGLUniformLocation*> location,
                          GLboolean transpose, Float32List data,
                          unsigned long long srcOffset = 0,
                          GLuint srcLength = 0);
    void uniformMatrix4fv(Optional<WebGLUniformLocation*> location,
                          GLboolean transpose, Float32List data,
                          unsigned long long srcOffset = 0,
                          GLuint srcLength = 0);

    /* Reading back pixels */
    // WebGL1:
    void readPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                    GLenum format, GLenum type,
                    Optional<ScriptArrayBufferView> dstData);
    // WebGL2:
    void readPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                    GLenum format, GLenum type, GLintptr offset);
    void readPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                    GLenum format, GLenum type, ScriptArrayBufferView dstData,
                    unsigned long long dstOffset);

protected:
    Optional<ScriptValue> getParameterImpl(GLenum pname);

    Optional<ScriptValue> getUniformImpl(WebGLProgram* program,
                                         WebGLUniformLocation* location,
                                         GLenum type);

    void implementUniformNuiv(
        size_t n, void (GL::*uniformNuiv)(GLint, GLsizei, const GLuint*),
        Optional<WebGLUniformLocation*> location, Uint32List data,
        unsigned long long srcOffset, GLuint srcLength);

private:
    bool checkInternalFormat(GLint internalFormat, GLenum format,
                             GLenum type) override;
    bool isSrcDataValid(ScriptArrayBufferView srcData, GLenum type) override;
    size_t getBytesPerPixel(GLenum format, GLenum type) override;
    int webGLVersion() const override
    {
        return 2;
    }

    GLenum m_currentVertexAttribType = GL_FLOAT;

    class IndexedBufferBinding : public gc {
    public:
        IndexedBufferBinding(WebGLBuffer* b = nullptr, GLintptr o = 0,
                             GLsizeiptr s = 0, bool r = false)
            : buffer(b)
            , offset(o)
            , size(s)
            , isRange(r)
        {
        }
        WebGLBuffer* buffer;
        GLintptr offset;
        GLsizeiptr size;
        bool isRange;
    };
    GCVector<IndexedBufferBinding*> m_uniformBufferBindings;
};

} // namespace Starfish

#endif // defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#endif // __StarfishWebGL2RenderingContext__
