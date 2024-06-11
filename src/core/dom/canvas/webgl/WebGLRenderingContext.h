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

#include "platform/canvas/gl/GLTypes.h"
#include "core/dom/canvas/webgl/WebGLRenderingContextBaseMixIn.h"
#include "core/dom/canvas/webgl/WebGLUtils.h"
#include "core/dom/canvas/webgl/WebGLContextAttributes.h"
#include "core/util/GCDescriptor.h"

#include <unordered_set>
#include <unordered_map>

namespace Starfish {

class WebGLActiveInfo;
class WebGLBuffer;
class WebGLObject;
class WebGLProgram;
class WebGLShader;
class WebGLTexture;
class WebGLFramebuffer;
class WebGLRenderbuffer;
class WebGLUniformLocation;
class WebGLRenderingContextState;
class String;
class Float32ArrayOrSequenceOfGLfloat;
class Int32ArrayOrSequenceOfGLint;
class ArrayBufferOrSharedArrayBufferOrArrayBufferView;
class
    ImageBitmapOrImageDataOrHTMLImageElementOrHTMLCanvasElementOrHTMLVideoElement;
class GL;

using Float32List = Float32ArrayOrSequenceOfGLfloat;
using Int32List = Int32ArrayOrSequenceOfGLint;
using AllowSharedBufferSource = ArrayBufferOrSharedArrayBufferOrArrayBufferView;
using TexImageSource =
    ImageBitmapOrImageDataOrHTMLImageElementOrHTMLCanvasElementOrHTMLVideoElement;

using GLErrorSet = std::unordered_set<GLenum>;
using GLTextureMap = std::unordered_map<GLenum, GLuint>;
using GLExtensionMap =
    GCUnorderedMap<std::string, ScriptObject, CaseInsensitiveHash,
                   CaseInsensitiveEqual>;

class WebGLRenderingContext : public WebGLRenderingContextBaseMixIn {
public:
    WebGLRenderingContext(HTMLCanvasElement* canvasElement);
    virtual ~WebGLRenderingContext();

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(WebGLRenderingContext);

    void preInitialize(ScriptValue contextAttributes);

    void initialize() override;
    void flush() override;
    void onResize() override;

    GLsizei drawingBufferWidth() const;
    GLsizei drawingBufferHeight() const;
    String* drawingBufferColorSpace();
    void setDrawingBufferColorSpace(String* value);
    String* unpackColorSpace();
    void setUnpackColorSpace(String* value);

    // Implement WebGLRenderingContextBase
    Nullable<WebGLContextAttributes> getContextAttributes();
    bool isContextLost();
    Nullable<GCVector<String*>> getSupportedExtensions();
    Nullable<ScriptObject> getExtension(String* name);
    void activeTexture(GLenum texture);
    void attachShader(WebGLProgram* program, WebGLShader* shader);
    void bindAttribLocation(WebGLProgram* program, GLuint index, String* name);
    void bindBuffer(GLenum target, Nullable<WebGLBuffer*> buffer);
    void bindFramebuffer(GLenum target, Nullable<WebGLFramebuffer*> buffer);
    void bindRenderbuffer(GLenum target, Nullable<WebGLRenderbuffer*> buffer);
    void bindTexture(GLenum target, Nullable<WebGLTexture*> texture);
    void blendColor(GLclampf red, GLclampf green, GLclampf blue,
                    GLclampf alpha);
    void blendEquation(GLenum mode);
    void blendEquationSeparate(GLenum modeRGB, GLenum modeAlpha);
    void blendFunc(GLenum sfactor, GLenum dfactor);
    void blendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha,
                           GLenum dstAlpha);

    GLenum checkFramebufferStatus(GLenum target);
    void clear(uint32_t mask);
    void clearColor(float red, float green, float blue, float alpha);
    void clearDepth(GLclampf depth);
    void clearStencil(GLint s);
    void colorMask(GLboolean red, GLboolean green, GLboolean blue,
                   GLboolean alpha);
    void compileShader(WebGLShader* shader);
    WebGLBuffer* createBuffer();
    WebGLFramebuffer* createFramebuffer();
    WebGLProgram* createProgram();
    WebGLRenderbuffer* createRenderbuffer();
    WebGLShader* createShader(unsigned long type);
    WebGLTexture* createTexture();
    void cullFace(GLenum mode);
    void deleteBuffer(Nullable<WebGLBuffer*> buffer);
    void deleteFramebuffer(Nullable<WebGLFramebuffer*> framebuffer);
    void deleteProgram(Nullable<WebGLProgram*> program);
    void deleteRenderbuffer(Nullable<WebGLRenderbuffer*> renderbuffer);
    void deleteShader(Nullable<WebGLShader*> shader);
    void deleteTexture(Nullable<WebGLTexture*> texture);
    void depthFunc(GLenum func);
    void depthMask(GLboolean flag);
    void depthRange(GLclampf zNear, GLclampf zFar);
    void detachShader(WebGLProgram* program, WebGLShader* shader);
    void disable(GLenum cap);
    void disableVertexAttribArray(GLuint index);
    void drawArrays(GLenum mode, GLint first, GLsizei count);
    void drawElements(GLenum mode, GLsizei count, GLenum type, GLintptr offset);
    void enable(GLenum cap);
    void enableVertexAttribArray(GLuint index);
    void finish();
    void flushWebGL();
    void framebufferRenderbuffer(GLenum target, GLenum attachment,
                                 GLenum renderbuffertarget,
                                 Nullable<WebGLRenderbuffer*> renderbuffer);
    void framebufferTexture2D(GLenum target, GLenum attachment,
                              GLenum textarget, Nullable<WebGLTexture*> texture,
                              GLint level);
    void frontFace(GLenum mode);
    void generateMipmap(GLenum target);
    WebGLActiveInfo* getActiveAttrib(WebGLProgram* program, GLuint index);
    WebGLActiveInfo* getActiveUniform(WebGLProgram* program, GLuint index);
    Nullable<GCVector<WebGLShader*>> getAttachedShaders(WebGLProgram* program);
    GLint getAttribLocation(WebGLProgram* program, String* name);
    ScriptValue getBufferParameter(GLenum target, GLenum pname);
    ScriptValue getParameter(GLenum pname);
    GLenum getError();
    ScriptValue getProgramParameter(WebGLProgram* program, GLenum pname);
    String* getProgramInfoLog(WebGLProgram* program);
    ScriptValue getShaderParameter(WebGLShader* shader, GLenum pname);
    String* getShaderInfoLog(WebGLShader* shader);
    String* getShaderSource(WebGLShader* shader);
    ScriptValue getUniform(WebGLProgram* program,
                           WebGLUniformLocation* location);
    WebGLUniformLocation* getUniformLocation(WebGLProgram* program,
                                             String* name);
    ScriptValue getVertexAttrib(GLuint index, GLenum pname);
    GLintptr getVertexAttribOffset(GLuint index, GLenum pname);

    void hint(GLenum target, GLenum mode);
    bool isBuffer(Nullable<WebGLBuffer*> buffer);
    bool isEnabled(GLenum cap);
    bool isFramebuffer(Nullable<WebGLFramebuffer*> framebuffer);
    bool isProgram(Nullable<WebGLProgram*> program);
    bool isRenderbuffer(Nullable<WebGLRenderbuffer*> renderbuffer);
    bool isShader(Nullable<WebGLShader*> shader);
    bool isTexture(Nullable<WebGLTexture*> texture);
    void lineWidth(GLfloat width);
    void linkProgram(WebGLProgram* program);
    void pixelStorei(GLenum pname, GLint param);
    void polygonOffset(GLfloat factor, GLfloat units);
    void renderbufferStorage(GLenum target, GLenum internalformat,
                             GLsizei width, GLsizei height);
    void sampleCoverage(GLclampf value, GLboolean invert);
    void scissor(GLint x, GLint y, GLsizei width, GLsizei height);
    void texParameterf(GLenum target, GLenum pname, GLfloat param);
    void texParameteri(GLenum target, GLenum pname, GLint param);
    void uniform1f(Nullable<WebGLUniformLocation*> uniform, GLfloat x);
    void uniform2f(Nullable<WebGLUniformLocation*> uniform, GLfloat x,
                   GLfloat y);
    void uniform3f(Nullable<WebGLUniformLocation*> uniform, GLfloat x,
                   GLfloat y, GLfloat z);
    void uniform4f(Nullable<WebGLUniformLocation*> uniform, GLfloat x,
                   GLfloat y, GLfloat z, GLfloat w);
    void uniform1i(Nullable<WebGLUniformLocation*> uniform, GLint x);
    void uniform2i(Nullable<WebGLUniformLocation*> uniform, GLint x, GLint y);
    void uniform3i(Nullable<WebGLUniformLocation*> uniform, GLint x, GLint y,
                   GLint z);
    void uniform4i(Nullable<WebGLUniformLocation*> uniform, GLint x, GLint y,
                   GLint z, GLint w);
    void useProgram(Nullable<WebGLProgram*> program);
    void validateProgram(WebGLProgram* program);
    void shaderSource(WebGLShader* shader, String* source);

    void stencilFunc(GLenum func, GLint ref, GLuint mask);
    void stencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask);
    void stencilMask(GLuint mask);
    void stencilMaskSeparate(GLenum face, GLuint mask);
    void stencilOp(GLenum fail, GLenum zfail, GLenum zpass);
    void stencilOpSeparate(GLenum face, GLenum fail, GLenum zfail,
                           GLenum zpass);

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
    void bufferSubData(GLenum target, GLintptr offset,
                       AllowSharedBufferSource data);
    void readPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                    GLenum format, GLenum type,
                    Nullable<ScriptArrayBufferView> pixels);
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

    // NOTE: GLError can only be set on WebGLRenderingContext and GLExtensions.
    void setGLError(GLenum code, const char* message = nullptr);
    bool executeInContextScope(std::function<void()> callback);
    WebGLRenderingContextState* getState();
    GL* gl();

    BEGIN_IMPLEMENT_NEW_WITH_GC_DESC(WebGLRenderingContext,
                                     WebGLRenderingContextBaseMixIn);
    FILL_GC_POINTER(WebGLRenderingContext, m_state);
    FILL_GC_POINTER(WebGLRenderingContext, m_unpackColorSpace);
    FILL_GC_POINTER(WebGLRenderingContext, m_drawingBufferColorSpace);
    FILL_GC_COLLECTION(WebGLRenderingContext, m_enabledExtensions);
    END_IMPLEMENT_NEW_WITH_GC_DESC();

private:
    bool checkWebGLObject(WebGLObject* object);
    bool isFromCurrentContext(WebGLObject* object);
    bool checkAttribOrUniformName(String* name);
    bool hasGLError();
    void updateGLError();
    bool isBoundCubeMapTexture(GLenum target);
    bool isFromCurrentProgram(WebGLUniformLocation* uniform);
    bool isExtensionEnabled(const char* requestedName);
    bool isDefaultFramebufferBound();
    GLuint getCurrentFBO();
    GLint getCurrentProgram();
    void completePendingJobs();
    void setPendingClearMask(uint32_t mask);

    bool m_hasPendingJobsBetweenFrames;
    uint32_t m_pendingClearMask;

    GLErrorSet m_GLErrors;
    GLTextureMap m_boundTextures;
    bool m_unpackFlipY;
    bool m_unpackPremultiplyAlpha;
    GLenum m_unpackColorspaceConversion;
    WebGLContextAttributes m_attributes;
    bool m_isContextLost;
    GL* m_gl;

    // The followings are gc managed.
    WebGLRenderingContextState* m_state;
    String* m_unpackColorSpace;
    String* m_drawingBufferColorSpace;
    GLExtensionMap m_enabledExtensions;
};
} // namespace Starfish

#endif
#endif
