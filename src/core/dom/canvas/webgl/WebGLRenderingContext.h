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
class WebGLShaderPrecisionFormat;
class String;
class Float32ArrayOrSequenceOfGLfloat;
class Int32ArrayOrSequenceOfGLint;
class ArrayBufferOrSharedArrayBufferOrArrayBufferView;
class
    ImageBitmapOrImageDataOrHTMLImageElementOrHTMLCanvasElementOrHTMLVideoElement;
class GL;
class TexImageHelper;

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
    void flushForReadback() override;
    void onResize() override;

    GLsizei drawingBufferWidth() const;
    GLsizei drawingBufferHeight() const;
    String* drawingBufferColorSpace();
    void setDrawingBufferColorSpace(String* value);
    String* unpackColorSpace();
    void setUnpackColorSpace(String* value);

    // Implement WebGLRenderingContextBase
    Optional<WebGLContextAttributes> getContextAttributes();
    bool isContextLost();
    Optional<GCVector<String*>> getSupportedExtensions();
    Optional<ScriptObject> getExtension(String* name);
    void activeTexture(GLenum texture);
    void attachShader(WebGLProgram* program, WebGLShader* shader);
    void bindAttribLocation(WebGLProgram* program, GLuint index, String* name);
    void bindBuffer(GLenum target, Optional<WebGLBuffer*> buffer);
    void bindFramebuffer(GLenum target, Optional<WebGLFramebuffer*> buffer);
    void bindRenderbuffer(GLenum target, Optional<WebGLRenderbuffer*> buffer);
    void bindTexture(GLenum target, Optional<WebGLTexture*> texture);
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

    void copyTexImage2D(GLenum target, GLint level, GLenum internalformat,
                        GLint x, GLint y, GLsizei width, GLsizei height,
                        GLint border);
    void copyTexSubImage2D(GLenum target, GLint level, GLint xoffset,
                           GLint yoffset, GLint x, GLint y, GLsizei width,
                           GLsizei height);

    WebGLBuffer* createBuffer();
    WebGLFramebuffer* createFramebuffer();
    WebGLProgram* createProgram();
    WebGLRenderbuffer* createRenderbuffer();
    WebGLShader* createShader(unsigned long type);
    WebGLTexture* createTexture();
    void cullFace(GLenum mode);
    void deleteBuffer(Optional<WebGLBuffer*> buffer);
    void deleteFramebuffer(Optional<WebGLFramebuffer*> framebuffer);
    void deleteProgram(Optional<WebGLProgram*> program);
    void deleteRenderbuffer(Optional<WebGLRenderbuffer*> renderbuffer);
    void deleteShader(Optional<WebGLShader*> shader);
    void deleteTexture(Optional<WebGLTexture*> texture);
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
    void flush();
    void framebufferRenderbuffer(GLenum target, GLenum attachment,
                                 GLenum renderbuffertarget,
                                 Optional<WebGLRenderbuffer*> renderbuffer);
    void framebufferTexture2D(GLenum target, GLenum attachment,
                              GLenum textarget, Optional<WebGLTexture*> texture,
                              GLint level);
    void frontFace(GLenum mode);
    void generateMipmap(GLenum target);
    WebGLActiveInfo* getActiveAttrib(WebGLProgram* program, GLuint index);
    WebGLActiveInfo* getActiveUniform(WebGLProgram* program, GLuint index);
    Optional<GCVector<WebGLShader*>> getAttachedShaders(WebGLProgram* program);
    GLint getAttribLocation(WebGLProgram* program, String* name);
    ScriptValue getBufferParameter(GLenum target, GLenum pname);
    ScriptValue getParameter(GLenum pname);
    GLenum getError();
    ScriptValue getFramebufferAttachmentParameter(GLenum target,
                                                  GLenum attachment,
                                                  GLenum pname);
    ScriptValue getProgramParameter(WebGLProgram* program, GLenum pname);
    String* getProgramInfoLog(WebGLProgram* program);
    ScriptValue getRenderbufferParameter(GLenum target, GLenum pname);
    ScriptValue getShaderParameter(WebGLShader* shader, GLenum pname);
    WebGLShaderPrecisionFormat* getShaderPrecisionFormat(GLenum shadertype,
                                                         GLenum precisiontype);
    String* getShaderInfoLog(WebGLShader* shader);
    String* getShaderSource(WebGLShader* shader);
    ScriptValue getTexParameter(GLenum target, GLenum pname);
    ScriptValue getUniform(WebGLProgram* program,
                           WebGLUniformLocation* location);
    WebGLUniformLocation* getUniformLocation(WebGLProgram* program,
                                             String* name);
    ScriptValue getVertexAttrib(GLuint index, GLenum pname);
    GLintptr getVertexAttribOffset(GLuint index, GLenum pname);

    void hint(GLenum target, GLenum mode);
    bool isBuffer(Optional<WebGLBuffer*> buffer);
    bool isEnabled(GLenum cap);
    bool isFramebuffer(Optional<WebGLFramebuffer*> framebuffer);
    bool isProgram(Optional<WebGLProgram*> program);
    bool isRenderbuffer(Optional<WebGLRenderbuffer*> renderbuffer);
    bool isShader(Optional<WebGLShader*> shader);
    bool isTexture(Optional<WebGLTexture*> texture);
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
    void uniform1f(Optional<WebGLUniformLocation*> uniform, GLfloat x);
    void uniform2f(Optional<WebGLUniformLocation*> uniform, GLfloat x,
                   GLfloat y);
    void uniform3f(Optional<WebGLUniformLocation*> uniform, GLfloat x,
                   GLfloat y, GLfloat z);
    void uniform4f(Optional<WebGLUniformLocation*> uniform, GLfloat x,
                   GLfloat y, GLfloat z, GLfloat w);
    void uniform1i(Optional<WebGLUniformLocation*> uniform, GLint x);
    void uniform2i(Optional<WebGLUniformLocation*> uniform, GLint x, GLint y);
    void uniform3i(Optional<WebGLUniformLocation*> uniform, GLint x, GLint y,
                   GLint z);
    void uniform4i(Optional<WebGLUniformLocation*> uniform, GLint x, GLint y,
                   GLint z, GLint w);
    void useProgram(Optional<WebGLProgram*> program);
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
    void bufferData(GLenum target, Optional<AllowSharedBufferSource> data,
                    GLenum usage);
    void bufferSubData(GLenum target, GLintptr offset,
                       AllowSharedBufferSource data);

    void compressedTexImage2D(GLenum target, GLint level, GLenum internalformat,
                              GLsizei width, GLsizei height, GLint border,
                              ScriptArrayBufferView data);
    void compressedTexSubImage2D(GLenum target, GLint level, GLint xoffset,
                                 GLint yoffset, GLsizei width, GLsizei height,
                                 GLenum format, ScriptArrayBufferView data);

    void readPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                    GLenum format, GLenum type,
                    Optional<ScriptArrayBufferView> pixels);

private:
    void handleTexImageWithArrayBufferView(
        GLenum target, GLint level, GLsizei width, GLsizei height,
        GLenum format, GLenum type, Optional<ScriptArrayBufferView> pixels,
        std::function<void(const TexImageHelper*)> updateImage,
        std::function<void(const std::vector<GLubyte>&)> updateBlackImage,
        std::function<void(const std::vector<GLushort>&)>
            updateTwoBytesBlackImage);
    void handleTexImageWithImageSource(
        const GLenum format, const GLenum type, const TexImageSource& source,
        std::function<void(const TexImageHelper*)> updateImage);

public:
    void texImage2D(GLenum target, GLint level, GLint internalFormat,
                    GLsizei width, GLsizei height, GLint border, GLenum format,
                    GLenum type, Optional<ScriptArrayBufferView> pixels);
    void texImage2D(GLenum target, GLint level, GLint internalFormat,
                    GLenum format, GLenum type, TexImageSource source);
    void texSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                       GLsizei width, GLsizei height, GLenum format,
                       GLenum type, Optional<ScriptArrayBufferView> pixels);
    void texSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                       GLenum format, GLenum type, TexImageSource source);

    void uniform1fv(Optional<WebGLUniformLocation*> location, Float32List v);
    void uniform2fv(Optional<WebGLUniformLocation*> location, Float32List v);
    void uniform3fv(Optional<WebGLUniformLocation*> location, Float32List v);
    void uniform4fv(Optional<WebGLUniformLocation*> location, Float32List v);

    void uniform1iv(Optional<WebGLUniformLocation*> location, Int32List v);
    void uniform2iv(Optional<WebGLUniformLocation*> location, Int32List v);
    void uniform3iv(Optional<WebGLUniformLocation*> location, Int32List v);
    void uniform4iv(Optional<WebGLUniformLocation*> location, Int32List v);

    void uniformMatrix2fv(Optional<WebGLUniformLocation*> uniform,
                          GLboolean transpose, Float32List value);
    void uniformMatrix3fv(Optional<WebGLUniformLocation*> uniform,
                          GLboolean transpose, Float32List value);
    void uniformMatrix4fv(Optional<WebGLUniformLocation*> uniform,
                          GLboolean transpose, Float32List value);

    GL* gl();

    BEGIN_IMPLEMENT_NEW_WITH_GC_DESC(WebGLRenderingContext,
                                     WebGLRenderingContextBaseMixIn);
    FILL_GC_POINTER(WebGLRenderingContext, m_state);
    FILL_GC_POINTER(WebGLRenderingContext, m_unpackColorSpace);
    FILL_GC_POINTER(WebGLRenderingContext, m_drawingBufferColorSpace);
    FILL_GC_COLLECTION(WebGLRenderingContext, m_enabledExtensions);
    END_IMPLEMENT_NEW_WITH_GC_DESC();

public:
    // Use setGLError only on WebGLRenderingContext and GLExtensions.
    void setGLError(GLenum code, const char* message = nullptr);
    bool hasGLError();
    bool hasNewGLError();
    bool executeInContextScope(std::function<void()> callback);
    WebGLRenderingContextState* getState();

private:
    bool checkAttribOrUniformName(String* name);

protected:
    bool isFromCurrentContext(WebGLObject* object);

private:
    bool isBoundCubeMapTexture(GLenum target);

protected:
    bool isFromCurrentProgram(WebGLUniformLocation* uniform);

private:
    bool isExtensionEnabled(const char* requestedName);
    bool isDefaultFramebufferBound();
    GLuint getCurrentFBO();
    GLint getCurrentProgram();
    void completePendingJobs();
    void setPendingClearMask(uint32_t mask);

protected:
    GLenum getUniformType(WebGLProgram* program,
                          WebGLUniformLocation* location);
    Optional<ScriptValue> getUniformImpl(WebGLProgram* program,
                                         WebGLUniformLocation* location,
                                         GLenum type);

    void implementUniformNfv(
        size_t n, void (GL::*uniformNfv)(GLint, GLsizei, const GLfloat*),
        Optional<WebGLUniformLocation*> location, Float32List data,
        unsigned long long srcOffset, GLuint srcLength);
    void implementUniformNiv(
        size_t n, void (GL::*uniformNiv)(GLint, GLsizei, const GLint*),
        Optional<WebGLUniformLocation*> location, Int32List data,
        unsigned long long srcOffset, GLuint srcLength);
    void implementUniformMatrixMxNfv(
        size_t m, size_t n,
        void (GL::*uniformMatrixMxNfv)(GLint, GLsizei, GLboolean,
                                       const GLfloat*),
        Optional<WebGLUniformLocation*> location, GLboolean transpose,
        Float32List data, unsigned long long srcOffset, GLuint srcLength);

private:
    virtual bool checkInternalFormat(GLint internalFormat, GLenum format,
                                     GLenum type);
    virtual bool isSrcDataValid(ScriptArrayBufferView srcData, GLenum type);
    virtual size_t getBytesPerPixel(GLenum format, GLenum type);

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
