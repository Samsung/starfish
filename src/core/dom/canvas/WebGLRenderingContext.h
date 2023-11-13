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
#include <unordered_set>
#include <unordered_map>

namespace Starfish {

class WebGLBuffer;
class WebGLObject;
class WebGLProgram;
class WebGLShader;
class WebGLTexture;
class WebGLUniformLocation;
class ArrayBufferOrSharedArrayBufferOrArrayBufferView;
class
    ImageBitmapOrImageDataOrHTMLImageElementOrHTMLCanvasElementOrHTMLVideoElement;

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
    void activeTexture(GLenum texture);
    void attachShader(WebGLProgram* program, WebGLShader* shader);
    void bindAttribLocation(WebGLProgram* program, GLuint index, String* name);
    void bindBuffer(GLenum target, Nullable<WebGLBuffer*> buffer);
    void bindTexture(GLenum target, Nullable<WebGLTexture*> texture);
    void clear(uint32_t mask);
    void clearColor(float red, float green, float blue, float alpha);
    void compileShader(WebGLShader* shader);
    WebGLTexture* createTexture();
    WebGLBuffer* createBuffer();
    WebGLProgram* createProgram();
    WebGLShader* createShader(unsigned long type);
    void drawArrays(GLenum mode, GLint first, GLsizei count);
    void enableVertexAttribArray(GLuint index);
    GLint getAttribLocation(WebGLProgram* program, String* name);
    ScriptValue getParameter(GLenum pname);
    GLenum getError();
    ScriptValue getProgramParameter(WebGLProgram* program, GLenum pname);
    ScriptValue getShaderParameter(WebGLShader* shader, GLenum pname);
    String* getShaderSource(WebGLShader* shader);
    WebGLUniformLocation* getUniformLocation(WebGLProgram* program,
                                             String* name);
    void linkProgram(WebGLProgram* program);
    void pixelStorei(GLenum pname, GLint param);
    void texParameteri(GLenum target, GLenum pname, GLint param);
    void uniform2f(WebGLUniformLocation* location, GLfloat x, GLfloat y);
    void useProgram(WebGLProgram* program);
    void shaderSource(WebGLShader* shader, String* source);
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

private:
    bool checkWebGLObject(WebGLObject* object);
    bool checkAttribOrUniformName(String* name);
    bool hasGLError(const char* message = "");
    void setGLError(GLenum code);
    void updateGLError();
    std::unordered_set<GLenum> m_GLErrors;
    std::unordered_map<GLenum, GLuint> m_boundTextures;
    bool m_unpackFlipY;
    bool m_unpackPremultiplyAlpha;
    GLenum m_unpackColorspaceConversion;
};
} // namespace Starfish

#endif
#endif
