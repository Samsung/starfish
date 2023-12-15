/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishWebGLRenderingContext__
#define __StarfishWebGLRenderingContext__

#if defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#include "core/dom/canvas/WebGLRenderingContextBaseMixIn.h"
#include "platform/canvas/webgl/GLESTypes.h"
#include "core/dom/canvas/WebGLUtils.h"
#include "core/dom/canvas/WebGLContextAttributes.h"
#include <unordered_set>
#include <unordered_map>

namespace Starfish {

class WebGLActiveInfo;
class WebGLBuffer;
class WebGLObject;
class WebGLProgram;
class WebGLShader;
class WebGLTexture;
class WebGLUniformLocation;
class Float32ArrayOrSequenceOfGLfloat;
class Int32ArrayOrSequenceOfGLint;
class ArrayBufferOrSharedArrayBufferOrArrayBufferView;
class
    ImageBitmapOrImageDataOrHTMLImageElementOrHTMLCanvasElementOrHTMLVideoElement;

using Float32List = Float32ArrayOrSequenceOfGLfloat;
using Int32List = Int32ArrayOrSequenceOfGLint;
using AllowSharedBufferSource = ArrayBufferOrSharedArrayBufferOrArrayBufferView;
using TexImageSource =
    ImageBitmapOrImageDataOrHTMLImageElementOrHTMLCanvasElementOrHTMLVideoElement;

class WebGLRenderingContext : public WebGLRenderingContextBaseMixIn {
public:
    WebGLRenderingContext(HTMLCanvasElement* canvasElement);
    virtual ~WebGLRenderingContext();

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(WebGLRenderingContext);

    void initialize() override;

    // Implement WebGLRenderingContextBase
    Nullable<WebGLContextAttributes> getContextAttributes();
    Nullable<GCVector<String*>> getSupportedExtensions();
    Nullable<ScriptObject> getExtension(String* name);
    void activeTexture(GLenum texture);
    void attachShader(WebGLProgram* program, WebGLShader* shader);
    void bindAttribLocation(WebGLProgram* program, GLuint index, String* name);
    void bindBuffer(GLenum target, Nullable<WebGLBuffer*> buffer);
    void bindTexture(GLenum target, Nullable<WebGLTexture*> texture);
    void clear(uint32_t mask);
    void clearColor(float red, float green, float blue, float alpha);
    void clearDepth(GLclampf depth);
    void clearStencil(GLint s);
    void colorMask(GLboolean red, GLboolean green, GLboolean blue,
                   GLboolean alpha);
    void compileShader(WebGLShader* shader);
    WebGLBuffer* createBuffer();
    WebGLProgram* createProgram();
    WebGLShader* createShader(unsigned long type);
    WebGLTexture* createTexture();
    void cullFace(GLenum mode);
    void deleteShader(WebGLShader* shader);
    void depthFunc(GLenum func);
    void disable(GLenum cap);
    void drawArrays(GLenum mode, GLint first, GLsizei count);
    void enable(GLenum cap);
    void enableVertexAttribArray(GLuint index);
    void frontFace(GLenum mode);
    WebGLActiveInfo* getActiveUniform(WebGLProgram* program, GLuint index);
    GLint getAttribLocation(WebGLProgram* program, String* name);
    ScriptValue getParameter(GLenum pname);
    GLenum getError();
    ScriptValue getProgramParameter(WebGLProgram* program, GLenum pname);
    String* getProgramInfoLog(WebGLProgram* program);
    ScriptValue getShaderParameter(WebGLShader* shader, GLenum pname);
    String* getShaderInfoLog(WebGLShader* shader);
    String* getShaderSource(WebGLShader* shader);
    WebGLUniformLocation* getUniformLocation(WebGLProgram* program,
                                             String* name);
    void linkProgram(WebGLProgram* program);
    void pixelStorei(GLenum pname, GLint param);
    void texParameteri(GLenum target, GLenum pname, GLint param);
    void uniform1f(WebGLUniformLocation* uniform, GLfloat x);
    void uniform2f(WebGLUniformLocation* uniform, GLfloat x, GLfloat y);
    void uniform3f(WebGLUniformLocation* uniform, GLfloat x, GLfloat y,
                   GLfloat z);
    void uniform4f(WebGLUniformLocation* uniform, GLfloat x, GLfloat y,
                   GLfloat z, GLfloat w);
    void uniform1i(WebGLUniformLocation* uniform, GLint x);
    void uniform2i(WebGLUniformLocation* uniform, GLint x, GLint y);
    void uniform3i(WebGLUniformLocation* uniform, GLint x, GLint y, GLint z);
    void uniform4i(WebGLUniformLocation* uniform, GLint x, GLint y, GLint z,
                   GLint w);
    void useProgram(WebGLProgram* program);
    void shaderSource(WebGLShader* shader, String* source);
    void stencilMask(GLuint mask);
    void vertexAttrib1f(GLuint index, GLfloat x);
    void vertexAttrib2f(GLuint index, GLfloat x, GLfloat y);
    void vertexAttrib3f(GLuint index, GLfloat x, GLfloat y, GLfloat z);
    void vertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z,
                        GLfloat w);

    void vertexAttrib1fv(GLuint index, Float32List values);
    void vertexAttrib2fv(GLuint index, Float32List values);
    void vertexAttrib3fv(GLuint index, Float32List values);
    void vertexAttrib4fv(GLuint index, Float32List values);
    void vertexAttribPointer(GLuint index, GLint size, GLenum type,
                             GLboolean normalized, GLsizei stride,
                             GLintptr offset);
    void viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

    // Implement WebGLRenderingContextOverloads
    void bufferData(GLenum target, GLsizeiptr size, GLenum usage);
    void bufferData(GLenum target, Nullable<AllowSharedBufferSource> data,
                    GLenum usage);
    void texImage2D(GLenum target, GLint level, GLint internalFormat,
                    GLsizei width, GLsizei height, GLint border, GLenum format,
                    GLenum type, Nullable<ScriptArrayBufferView> pixels);
    void texImage2D(GLenum target, GLint level, GLint internalFormat,
                    GLenum format, GLenum type, TexImageSource source);

    void uniform1fv(WebGLUniformLocation* location, Float32List v);
    void uniform2fv(WebGLUniformLocation* location, Float32List v);
    void uniform3fv(WebGLUniformLocation* location, Float32List v);
    void uniform4fv(WebGLUniformLocation* location, Float32List v);

    void uniform1iv(WebGLUniformLocation* location, Int32List v);
    void uniform2iv(WebGLUniformLocation* location, Int32List v);
    void uniform3iv(WebGLUniformLocation* location, Int32List v);
    void uniform4iv(WebGLUniformLocation* location, Int32List v);

    void uniformMatrix2fv(WebGLUniformLocation* uniform, GLboolean transpose,
                          Float32List value);
    void uniformMatrix3fv(WebGLUniformLocation* uniform, GLboolean transpose,
                          Float32List value);
    void uniformMatrix4fv(WebGLUniformLocation* uniform, GLboolean transpose,
                          Float32List value);

private:
    bool checkWebGLObject(WebGLObject* object);
    bool checkAttribOrUniformName(String* name);
    bool hasGLError();
    void setGLError(GLenum code, const char* message = nullptr);
    void updateGLError();
    bool isBoundCubeMapTexture(GLenum target);
    bool isFromCurrentProgram(WebGLUniformLocation* uniform);
    std::unordered_set<GLenum> m_GLErrors;
    std::unordered_map<GLenum, GLuint> m_boundTextures;
    GCUnorderedMap<std::string, ScriptObject, CaseInsensitiveHash,
                   CaseInsensitiveEqual>
        m_enabledExtensions;
    bool m_unpackFlipY;
    bool m_unpackPremultiplyAlpha;
    GLenum m_unpackColorspaceConversion;
    WebGLContextAttributes m_attributes;
};
} // namespace Starfish

#endif
#endif
