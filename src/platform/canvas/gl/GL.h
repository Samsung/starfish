/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishGL__
#define __StarfishGL__

#include "StarfishPlatform.h"

#if !defined(STARFISH_EFL_HEADLESS)

#include <SkMatrix.h>
#include "platform/canvas/gl/GLTypes.h"

namespace Starfish {

class Renderer;

class GL {
public:
    static GL *create(Renderer *renderer);
    static GL *createGeneric(Renderer *renderer);

    virtual ~GL()
    {
    }

    virtual void activeTexture(GLenum texture) = 0;
    virtual void attachShader(GLuint program, GLuint shader) = 0;
    virtual void bindAttribLocation(GLuint program, GLuint index,
                                    const GLchar *name) = 0;
    virtual void bindBuffer(GLenum target, GLuint buffer) = 0;
    virtual void bindFramebuffer(GLenum target, GLuint framebuffer) = 0;
    virtual void bindRenderbuffer(GLenum target, GLuint renderbuffer) = 0;
    virtual void bindTexture(GLenum target, GLuint texture) = 0;
    virtual void blendColor(GLfloat red, GLfloat green, GLfloat blue,
                            GLfloat alpha) = 0;
    virtual void blendEquation(GLenum mode) = 0;
    virtual void blendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) = 0;
    virtual void blendFunc(GLenum sfactor, GLenum dfactor) = 0;
    virtual void blendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB,
                                   GLenum sfactorAlpha,
                                   GLenum dfactorAlpha) = 0;
    virtual void bufferData(GLenum target, GLsizeiptr size, const void *data,
                            GLenum usage) = 0;
    virtual void bufferSubData(GLenum target, GLintptr offset, GLsizeiptr size,
                               const void *data) = 0;
    virtual GLenum checkFramebufferStatus(GLenum target) = 0;
    virtual void clear(GLbitfield mask) = 0;
    virtual void clearColor(GLclampf red, GLclampf green, GLclampf blue,
                            GLclampf alpha) = 0;
    virtual void clearDepthf(GLfloat d) = 0;
    virtual void clearStencil(GLint s) = 0;
    virtual void colorMask(GLboolean red, GLboolean green, GLboolean blue,
                           GLboolean alpha) = 0;
    virtual void shaderSource(GLuint shader, GLsizei count,
                              const char *const *string,
                              const GLint *length) = 0;
    virtual void getShaderInfoLog(GLuint shader, GLsizei bufsize,
                                  GLsizei *length, char *infolog) = 0;
    virtual void getShaderSource(GLuint shader, GLsizei bufSize,
                                 GLsizei *length, GLchar *source) = 0;
    virtual void compileShader(GLuint shader) = 0;
    virtual void compressedTexImage2D(GLenum target, GLint level,
                                      GLenum internalformat, GLsizei width,
                                      GLsizei height, GLint border,
                                      GLsizei imageSize, const void *data) = 0;
    virtual void compressedTexSubImage2D(GLenum target, GLint level,
                                         GLint xoffset, GLint yoffset,
                                         GLsizei width, GLsizei height,
                                         GLenum format, GLsizei imageSize,
                                         const void *data) = 0;
    virtual void copyTexImage2D(GLenum target, GLint level,
                                GLenum internalformat, GLint x, GLint y,
                                GLsizei width, GLsizei height,
                                GLint border) = 0;
    virtual void copyTexSubImage2D(GLenum target, GLint level, GLint xoffset,
                                   GLint yoffset, GLint x, GLint y,
                                   GLsizei width, GLsizei height) = 0;
    virtual GLuint createProgram(void) = 0;
    virtual GLuint createShader(GLenum type) = 0;
    virtual void cullFace(GLenum mode) = 0;
    virtual void deleteBuffers(GLsizei n, const GLuint *buffers) = 0;
    virtual void deleteFramebuffers(GLsizei n, const GLuint *framebuffers) = 0;
    virtual void deleteProgram(GLuint program) = 0;
    virtual void deleteRenderbuffers(GLsizei n,
                                     const GLuint *renderbuffers) = 0;
    virtual void deleteShader(GLuint shader) = 0;
    virtual void deleteTextures(GLsizei n, const GLuint *textures) = 0;
    virtual void depthFunc(GLenum func) = 0;
    virtual void depthMask(GLboolean flag) = 0;
    virtual void depthRangef(GLfloat n, GLfloat f) = 0;
    virtual void detachShader(GLuint program, GLuint shader) = 0;
    virtual void disable(GLenum cap) = 0;
    virtual void disableVertexAttribArray(GLuint index) = 0;
    virtual void drawArrays(GLenum mode, GLint first, GLsizei count) = 0;
    virtual void drawElements(GLenum mode, GLsizei count, GLenum type,
                              const void *indices) = 0;
    virtual void enable(GLenum cap) = 0;
    virtual void enableVertexAttribArray(GLuint index) = 0;
    virtual void finish(void) = 0;
    virtual void flush(void) = 0;
    virtual void framebufferRenderbuffer(GLenum target, GLenum attachment,
                                         GLenum renderbuffertarget,
                                         GLuint renderbuffer) = 0;
    virtual void framebufferTexture2D(GLenum target, GLenum attachment,
                                      GLenum textarget, GLuint texture,
                                      GLint level) = 0;
    virtual void frontFace(GLenum mode) = 0;
    virtual void genBuffers(GLsizei n, GLuint *buffers) = 0;
    virtual void generateMipmap(GLenum target) = 0;
    virtual void genFramebuffers(GLsizei n, GLuint *framebuffers) = 0;
    virtual void genRenderbuffers(GLsizei n, GLuint *renderbuffers) = 0;
    virtual void genTextures(GLsizei n, GLuint *textures) = 0;
    virtual void getActiveAttrib(GLuint program, GLuint index, GLsizei bufsize,
                                 GLsizei *length, GLint *size, GLenum *type,
                                 char *name) = 0;
    virtual void getActiveUniform(GLuint program, GLuint index, GLsizei bufsize,
                                  GLsizei *length, GLint *size, GLenum *type,
                                  char *name) = 0;
    virtual void getAttachedShaders(GLuint program, GLsizei maxcount,
                                    GLsizei *count, GLuint *shaders) = 0;
    virtual int getAttribLocation(GLuint program, const char *name) = 0;
    virtual void getBooleanv(GLenum pname, GLboolean *params) = 0;
    virtual void getBufferParameteriv(GLenum target, GLenum pname,
                                      GLint *params) = 0;
    virtual GLenum getError(void) = 0;
    virtual void getFloatv(GLenum pname, GLfloat *params) = 0;
    virtual void getFramebufferAttachmentParameteriv(GLenum target,
                                                     GLenum attachment,
                                                     GLenum pname,
                                                     GLint *params) = 0;
    virtual void getIntegerv(GLenum pname, GLint *params) = 0;
    virtual void getProgramiv(GLuint program, GLenum pname, GLint *params) = 0;
    virtual void getProgramInfoLog(GLuint program, GLsizei bufSize,
                                   GLsizei *length, GLchar *infoLog) = 0;
    virtual void getRenderbufferParameteriv(GLenum target, GLenum pname,
                                            GLint *params) = 0;
    virtual void getShaderiv(GLuint shader, GLenum pname, GLint *params) = 0;
    virtual const GLubyte *getString(GLenum name) = 0;
    virtual void getTexParameterfv(GLenum target, GLenum pname,
                                   GLfloat *params) = 0;
    virtual void getTexParameteriv(GLenum target, GLenum pname,
                                   GLint *params) = 0;
    virtual void getUniformfv(GLuint program, GLint location,
                              GLfloat *params) = 0;
    virtual void getUniformiv(GLuint program, GLint location,
                              GLint *params) = 0;
    virtual GLint getUniformLocation(GLuint program, const char *name) = 0;
    virtual void getVertexAttribfv(GLuint index, GLenum pname,
                                   GLfloat *params) = 0;
    virtual void getVertexAttribiv(GLuint index, GLenum pname,
                                   GLint *params) = 0;
    virtual void getVertexAttribPointerv(GLuint index, GLenum pname,
                                         void **pointer) = 0;
    virtual void getShaderPrecisionFormat(GLenum shaderType,
                                          GLenum precisionType, GLint *range,
                                          GLint *precision) = 0;
    virtual void hint(GLenum target, GLenum mode) = 0;
    virtual void lineWidth(GLfloat width) = 0;
    virtual void linkProgram(GLuint program) = 0;
    virtual void pixelStorei(GLenum pname, GLint param) = 0;
    virtual void polygonOffset(GLfloat factor, GLfloat units) = 0;
    virtual void readPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                            GLenum format, GLenum type, void *pixels) = 0;
    virtual void releaseShaderCompiler(void) = 0;
    virtual void renderbufferStorage(GLenum target, GLenum internalformat,
                                     GLsizei width, GLsizei height) = 0;
    virtual void sampleCoverage(GLfloat value, GLboolean invert) = 0;
    virtual void scissor(GLint x, GLint y, GLsizei width, GLsizei height) = 0;
    virtual void stencilFunc(GLenum func, GLint ref, GLuint mask) = 0;
    virtual void stencilFuncSeparate(GLenum face, GLenum func, GLint ref,
                                     GLuint mask) = 0;
    virtual void stencilMask(GLuint mask) = 0;
    virtual void stencilMaskSeparate(GLenum face, GLuint mask) = 0;
    virtual void stencilOp(GLenum fail, GLenum zfail, GLenum zpass) = 0;
    virtual void stencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail,
                                   GLenum dppass) = 0;
    virtual void texImage2D(GLenum target, GLint level, GLint internalformat,
                            GLsizei width, GLsizei height, GLint border,
                            GLenum format, GLenum type, const void *pixels) = 0;
    virtual void texParameterf(GLenum target, GLenum pname, GLfloat param) = 0;
    virtual void texParameterfv(GLenum target, GLenum pname,
                                const GLfloat *params) = 0;
    virtual void texParameteri(GLenum target, GLenum pname, GLint param) = 0;
    virtual void texParameteriv(GLenum target, GLenum pname,
                                const GLint *params) = 0;
    virtual void texSubImage2D(GLenum target, GLint level, GLint xoffset,
                               GLint yoffset, GLsizei width, GLsizei height,
                               GLenum format, GLenum type,
                               const void *pixels) = 0;
    virtual void uniform1f(GLint location, GLfloat x) = 0;
    virtual void uniform1fv(GLint location, GLsizei count,
                            const GLfloat *v) = 0;
    virtual void uniform1i(GLint location, GLint x) = 0;
    virtual void uniform1iv(GLint location, GLsizei count, const GLint *v) = 0;
    virtual void uniform2f(GLint location, GLfloat x, GLfloat y) = 0;
    virtual void uniform2fv(GLint location, GLsizei count,
                            const GLfloat *v) = 0;
    virtual void uniform2i(GLint location, GLint x, GLint y) = 0;
    virtual void uniform2iv(GLint location, GLsizei count, const GLint *v) = 0;
    virtual void uniform3f(GLint location, GLfloat x, GLfloat y, GLfloat z) = 0;
    virtual void uniform3fv(GLint location, GLsizei count,
                            const GLfloat *v) = 0;
    virtual void uniform3i(GLint location, GLint x, GLint y, GLint z) = 0;
    virtual void uniform3iv(GLint location, GLsizei count, const GLint *v) = 0;
    virtual void uniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z,
                           GLfloat w) = 0;
    virtual void uniform4fv(GLint location, GLsizei count,
                            const GLfloat *v) = 0;
    virtual void uniform4i(GLint location, GLint x, GLint y, GLint z,
                           GLint w) = 0;
    virtual void uniform4iv(GLint location, GLsizei count, const GLint *v) = 0;
    virtual void uniformMatrix2fv(GLint location, GLsizei count,
                                  GLboolean transpose,
                                  const GLfloat *value) = 0;
    virtual void uniformMatrix3fv(GLint location, GLsizei count,
                                  GLboolean transpose,
                                  const GLfloat *value) = 0;
    virtual void uniformMatrix4fv(GLint location, GLsizei count,
                                  GLboolean transpose,
                                  const GLfloat *value) = 0;
    virtual void useProgram(GLuint program) = 0;
    virtual void validateProgram(GLuint program) = 0;
    virtual void vertexAttrib1f(GLuint indx, GLfloat x) = 0;
    virtual void vertexAttrib1fv(GLuint indx, const GLfloat *values) = 0;
    virtual void vertexAttrib2f(GLuint indx, GLfloat x, GLfloat y) = 0;
    virtual void vertexAttrib2fv(GLuint indx, const GLfloat *values) = 0;
    virtual void vertexAttrib3f(GLuint indx, GLfloat x, GLfloat y,
                                GLfloat z) = 0;
    virtual void vertexAttrib3fv(GLuint indx, const GLfloat *values) = 0;
    virtual void vertexAttrib4f(GLuint indx, GLfloat x, GLfloat y, GLfloat z,
                                GLfloat w) = 0;
    virtual void vertexAttrib4fv(GLuint indx, const GLfloat *values) = 0;
    virtual void vertexAttribPointer(GLuint indx, GLint size, GLenum type,
                                     GLboolean normalized, GLsizei stride,
                                     const void *ptr) = 0;
    virtual void viewport(GLint x, GLint y, GLsizei width, GLsizei height) = 0;

    virtual void genVertexArrays(GLsizei n, GLuint *arrays) = 0;

    virtual GLboolean isEnabled(GLenum cap) = 0;

    virtual bool isGeneric() = 0;

    // Extensions
    virtual void *xglCreateImage(int target, void *buffer,
                                 const int *attriblist)
    {
        return nullptr;
    }
    virtual void xglDestroyImage(void *image)
    {
    }
    virtual void xglImageTargetTexture2DOES(GLenum target, void *image)
    {
    }
};

} // namespace Starfish

#endif

#endif
