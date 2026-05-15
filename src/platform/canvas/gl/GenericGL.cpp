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

#include "StarfishConfig.h"
#include "Starfish.h"

#if !defined(STARFISH_HEADLESS)
#include "GL.h"
#include "IncludeGL.h"

#include "core/page/WebView.h"
#include "core/modules/renderer/Renderer.h"

#define EGL_NO_CONTEXT ((EGLContext)0)
typedef void *EGLDisplay;
typedef void *EGLContext;
typedef void *EGLImageKHR;
typedef void *EGLClientBuffer;
typedef unsigned int EGLBoolean;
typedef unsigned int EGLenum;
typedef GLint EGLint;
typedef EGLDisplay (*PFNGLEGLGETCURRENTDISPLAYPROC)();
typedef EGLImageKHR (*PFNEGLCREATEIMAGEKHRPROC)(EGLDisplay dpy, EGLContext ctx,
                                                EGLenum target,
                                                EGLClientBuffer buffer,
                                                const EGLint *attribList);
typedef EGLBoolean (*PFNEGLDESTROYIMAGEKHRPROC)(EGLDisplay dpy,
                                                EGLImageKHR image);

// gles3.0 only
#ifndef GLAPIENTRY
#define GLAPIENTRY
#endif
typedef void(GLAPIENTRY *PFNGLINVALIDATEFRAMEBUFFERPROC)(
    GLenum target, GLsizei numAttachments, const GLenum *attachments);

namespace Starfish {

class GenericGL : public GL {
public:
    virtual void activeTexture(GLenum texture) override
    {
        glActiveTexture(texture);
    }

    virtual void attachShader(GLuint program, GLuint shader) override
    {
        glAttachShader(program, shader);
    }

    virtual void bindAttribLocation(GLuint program, GLuint index,
                                    const GLchar *name) override
    {
        glBindAttribLocation(program, index, name);
    }

    virtual void bindBuffer(GLenum target, GLuint buffer) override
    {
        glBindBuffer(target, buffer);
    }

    virtual void bindFramebuffer(GLenum target, GLuint framebuffer) override
    {
        glBindFramebuffer(target, framebuffer);
    }

    virtual void bindRenderbuffer(GLenum target, GLuint renderbuffer) override
    {
        glBindRenderbuffer(target, renderbuffer);
    }

    virtual void bindTexture(GLenum target, GLuint texture) override
    {
        glBindTexture(target, texture);
    }

    virtual void blendColor(GLfloat red, GLfloat green, GLfloat blue,
                            GLfloat alpha) override
    {
        glBlendColor(red, green, blue, alpha);
    }

    virtual void blendEquation(GLenum mode) override
    {
        glBlendEquation(mode);
    }

    virtual void blendEquationSeparate(GLenum modeRGB,
                                       GLenum modeAlpha) override
    {
        glBlendEquationSeparate(modeRGB, modeAlpha);
    }

    virtual void blendFunc(GLenum sfactor, GLenum dfactor) override
    {
        glBlendFunc(sfactor, dfactor);
    }

    virtual void blendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB,
                                   GLenum sfactorAlpha,
                                   GLenum dfactorAlpha) override
    {
        glBlendFuncSeparate(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
    }

    virtual void bufferData(GLenum target, GLsizeiptr size, const void *data,
                            GLenum usage) override
    {
        glBufferData(target, size, data, usage);
    }

    virtual void bufferSubData(GLenum target, GLintptr offset, GLsizeiptr size,
                               const void *data) override
    {
        glBufferSubData(target, offset, size, data);
    }

    virtual GLenum checkFramebufferStatus(GLenum target) override
    {
        return glCheckFramebufferStatus(target);
    }

    virtual void clear(GLbitfield mask) override
    {
        glClear(mask);
    }

    virtual void clearColor(GLclampf red, GLclampf green, GLclampf blue,
                            GLclampf alpha) override
    {
        glClearColor(red, green, blue, alpha);
    }

    virtual void clearDepthf(GLfloat d) override
    {
        glClearDepthf(d);
    }

    virtual void clearStencil(GLint s) override
    {
        glClearStencil(s);
    }

    virtual void colorMask(GLboolean red, GLboolean green, GLboolean blue,
                           GLboolean alpha) override
    {
        glColorMask(red, green, blue, alpha);
    }

    virtual void shaderSource(GLuint shader, GLsizei count,
                              const char *const *string,
                              const GLint *length) override
    {
        glShaderSource(shader, count, string, length);
    }

    virtual void getShaderInfoLog(GLuint shader, GLsizei bufsize,
                                  GLsizei *length, char *infolog) override
    {
        glGetShaderInfoLog(shader, bufsize, length, infolog);
    }

    virtual void getShaderSource(GLuint shader, GLsizei bufSize,
                                 GLsizei *length, GLchar *source)
    {
        glGetShaderSource(shader, bufSize, length, source);
    }

    virtual void compileShader(GLuint shader) override
    {
        glCompileShader(shader);
    }

    virtual void compressedTexImage2D(GLenum target, GLint level,
                                      GLenum internalformat, GLsizei width,
                                      GLsizei height, GLint border,
                                      GLsizei imageSize,
                                      const void *data) override
    {
        glCompressedTexImage2D(target, level, internalformat, width, height,
                               border, imageSize, data);
    }

    virtual void compressedTexSubImage2D(GLenum target, GLint level,
                                         GLint xoffset, GLint yoffset,
                                         GLsizei width, GLsizei height,
                                         GLenum format, GLsizei imageSize,
                                         const void *data) override
    {
        glCompressedTexSubImage2D(target, level, xoffset, yoffset, width,
                                  height, format, imageSize, data);
    }

    virtual void copyTexImage2D(GLenum target, GLint level,
                                GLenum internalformat, GLint x, GLint y,
                                GLsizei width, GLsizei height,
                                GLint border) override
    {
        glCopyTexImage2D(target, level, internalformat, x, y, width, height,
                         border);
    }

    virtual void copyTexSubImage2D(GLenum target, GLint level, GLint xoffset,
                                   GLint yoffset, GLint x, GLint y,
                                   GLsizei width, GLsizei height) override
    {
        glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width,
                            height);
    }

    virtual GLuint createProgram(void) override
    {
        return glCreateProgram();
    }

    virtual GLuint createShader(GLenum type) override
    {
        return glCreateShader(type);
    }

    virtual void cullFace(GLenum mode) override
    {
        glCullFace(mode);
    }

    virtual void deleteBuffers(GLsizei n, const GLuint *buffers) override
    {
        glDeleteBuffers(n, buffers);
    }

    virtual void deleteFramebuffers(GLsizei n,
                                    const GLuint *framebuffers) override
    {
        glDeleteFramebuffers(n, framebuffers);
    }

    virtual void deleteProgram(GLuint program) override
    {
        glDeleteProgram(program);
    }

    virtual void deleteRenderbuffers(GLsizei n,
                                     const GLuint *renderbuffers) override
    {
        glDeleteRenderbuffers(n, renderbuffers);
    }

    virtual void deleteShader(GLuint shader) override
    {
        glDeleteShader(shader);
    }

    virtual void deleteTextures(GLsizei n, const GLuint *textures) override
    {
        glDeleteTextures(n, textures);
    }

    virtual void depthFunc(GLenum func) override
    {
        glDepthFunc(func);
    }

    virtual void depthMask(GLboolean flag) override
    {
        glDepthMask(flag);
    }

    virtual void depthRangef(GLfloat n, GLfloat f) override
    {
        glDepthRangef(n, f);
    }

    virtual void detachShader(GLuint program, GLuint shader) override
    {
        glDetachShader(program, shader);
    }

    virtual void disable(GLenum cap) override
    {
        glDisable(cap);
    }

    virtual void disableVertexAttribArray(GLuint index) override
    {
        glDisableVertexAttribArray(index);
    }

    virtual void drawArrays(GLenum mode, GLint first, GLsizei count) override
    {
        glDrawArrays(mode, first, count);
    }

    virtual void drawElements(GLenum mode, GLsizei count, GLenum type,
                              const void *indices) override
    {
        glDrawElements(mode, count, type, indices);
    }

    virtual void enable(GLenum cap) override
    {
        glEnable(cap);
    }

    virtual void enableVertexAttribArray(GLuint index) override
    {
        glEnableVertexAttribArray(index);
    }

    virtual void finish(void) override
    {
        glFinish();
    }

    virtual void flush(void) override
    {
        glFlush();
    }

    virtual void framebufferRenderbuffer(GLenum target, GLenum attachment,
                                         GLenum renderbuffertarget,
                                         GLuint renderbuffer) override
    {
        glFramebufferRenderbuffer(target, attachment, renderbuffertarget,
                                  renderbuffer);
    }

    virtual void framebufferTexture2D(GLenum target, GLenum attachment,
                                      GLenum textarget, GLuint texture,
                                      GLint level) override
    {
        glFramebufferTexture2D(target, attachment, textarget, texture, level);
    }

    virtual void frontFace(GLenum mode) override
    {
        glFrontFace(mode);
    }

    virtual void genBuffers(GLsizei n, GLuint *buffers) override
    {
        glGenBuffers(n, buffers);
    }

    virtual void generateMipmap(GLenum target) override
    {
        glGenerateMipmap(target);
    }

    virtual void genFramebuffers(GLsizei n, GLuint *framebuffers) override
    {
        glGenFramebuffers(n, framebuffers);
    }

    virtual void genRenderbuffers(GLsizei n, GLuint *renderbuffers) override
    {
        glGenRenderbuffers(n, renderbuffers);
    }

    virtual void genTextures(GLsizei n, GLuint *textures) override
    {
        glGenTextures(n, textures);
    }

    virtual void getActiveAttrib(GLuint program, GLuint index, GLsizei bufsize,
                                 GLsizei *length, GLint *size, GLenum *type,
                                 char *name) override
    {
        glGetActiveAttrib(program, index, bufsize, length, size, type, name);
    }

    virtual void getActiveUniform(GLuint program, GLuint index, GLsizei bufsize,
                                  GLsizei *length, GLint *size, GLenum *type,
                                  char *name) override
    {
        glGetActiveUniform(program, index, bufsize, length, size, type, name);
    }

    virtual void getAttachedShaders(GLuint program, GLsizei maxcount,
                                    GLsizei *count, GLuint *shaders) override
    {
        glGetAttachedShaders(program, maxcount, count, shaders);
    }

    virtual int getAttribLocation(GLuint program, const char *name) override
    {
        return glGetAttribLocation(program, name);
    }

    virtual void getBooleanv(GLenum pname, GLboolean *params) override
    {
        glGetBooleanv(pname, params);
    }

    virtual void getBufferParameteriv(GLenum target, GLenum pname,
                                      GLint *params) override
    {
        glGetBufferParameteriv(target, pname, params);
    }

    virtual GLenum getError(void) override
    {
        return glGetError();
    }

    virtual void getFloatv(GLenum pname, GLfloat *params) override
    {
        glGetFloatv(pname, params);
    }

    virtual void getFramebufferAttachmentParameteriv(GLenum target,
                                                     GLenum attachment,
                                                     GLenum pname,
                                                     GLint *params) override
    {
        glGetFramebufferAttachmentParameteriv(target, attachment, pname,
                                              params);
    }

    virtual void getIntegerv(GLenum pname, GLint *params) override
    {
        glGetIntegerv(pname, params);
    }

    virtual void getProgramiv(GLuint program, GLenum pname,
                              GLint *params) override
    {
        glGetProgramiv(program, pname, params);
    }

    virtual void getProgramInfoLog(GLuint program, GLsizei bufSize,
                                   GLsizei *length, GLchar *infoLog) override
    {
        glGetProgramInfoLog(program, bufSize, length, infoLog);
    }

    virtual void getRenderbufferParameteriv(GLenum target, GLenum pname,
                                            GLint *params) override
    {
        glGetRenderbufferParameteriv(target, pname, params);
    }

    virtual void getShaderiv(GLuint shader, GLenum pname,
                             GLint *params) override
    {
        glGetShaderiv(shader, pname, params);
    }

    virtual const GLubyte *getString(GLenum name) override
    {
        return glGetString(name);
    }

    virtual void getTexParameterfv(GLenum target, GLenum pname,
                                   GLfloat *params) override
    {
        glGetTexParameterfv(target, pname, params);
    }

    virtual void getTexParameteriv(GLenum target, GLenum pname,
                                   GLint *params) override
    {
        return glGetTexParameteriv(target, pname, params);
    }

    virtual void getUniformfv(GLuint program, GLint location,
                              GLfloat *params) override
    {
        glGetUniformfv(program, location, params);
    }

    virtual void getUniformiv(GLuint program, GLint location,
                              GLint *params) override
    {
        glGetUniformiv(program, location, params);
    }

    virtual GLint getUniformLocation(GLuint program, const char *name) override
    {
        return glGetUniformLocation(program, name);
    }

    virtual void getVertexAttribfv(GLuint index, GLenum pname,
                                   GLfloat *params) override
    {
        glGetVertexAttribfv(index, pname, params);
    }

    virtual void getVertexAttribiv(GLuint index, GLenum pname,
                                   GLint *params) override
    {
        glGetVertexAttribiv(index, pname, params);
    }

    virtual void getVertexAttribPointerv(GLuint index, GLenum pname,
                                         void **pointer) override
    {
        glGetVertexAttribPointerv(index, pname, pointer);
    }

    virtual void getShaderPrecisionFormat(GLenum shaderType,
                                          GLenum precisionType, GLint *range,
                                          GLint *precision) override
    {
        glGetShaderPrecisionFormat(shaderType, precisionType, range, precision);
    }

    virtual void hint(GLenum target, GLenum mode) override
    {
        glHint(target, mode);
    }

    virtual GLboolean isBuffer(GLuint buffer) override
    {
        return glIsBuffer(buffer);
    }

    virtual void lineWidth(GLfloat width) override
    {
        glLineWidth(width);
    }

    virtual void linkProgram(GLuint program) override
    {
        glLinkProgram(program);
    }

    virtual void pixelStorei(GLenum pname, GLint param) override
    {
        glPixelStorei(pname, param);
    }

    virtual void polygonOffset(GLfloat factor, GLfloat units) override
    {
        glPolygonOffset(factor, units);
    }

    virtual void readPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                            GLenum format, GLenum type, void *pixels) override
    {
        glReadPixels(x, y, width, height, format, type, pixels);
    }

    virtual void releaseShaderCompiler(void) override
    {
        glReleaseShaderCompiler();
    }

    virtual void renderbufferStorage(GLenum target, GLenum internalformat,
                                     GLsizei width, GLsizei height) override
    {
        glRenderbufferStorage(target, internalformat, width, height);
    }

    virtual void sampleCoverage(GLfloat value, GLboolean invert) override
    {
        glSampleCoverage(value, invert);
    }

    virtual void scissor(GLint x, GLint y, GLsizei width,
                         GLsizei height) override
    {
        glScissor(x, y, width, height);
    }

    virtual void stencilFunc(GLenum func, GLint ref, GLuint mask) override
    {
        glStencilFunc(func, ref, mask);
    }

    virtual void stencilFuncSeparate(GLenum face, GLenum func, GLint ref,
                                     GLuint mask) override
    {
        glStencilFuncSeparate(face, func, ref, mask);
    }

    virtual void stencilMask(GLuint mask) override
    {
        glStencilMask(mask);
    }

    virtual void stencilMaskSeparate(GLenum face, GLuint mask) override
    {
        glStencilMaskSeparate(face, mask);
    }

    virtual void stencilOp(GLenum fail, GLenum zfail, GLenum zpass) override
    {
        glStencilOp(fail, zfail, zpass);
    }

    virtual void stencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail,
                                   GLenum dppass) override
    {
        glStencilOpSeparate(face, sfail, dpfail, dppass);
    }

    virtual void texImage2D(GLenum target, GLint level, GLint internalformat,
                            GLsizei width, GLsizei height, GLint border,
                            GLenum format, GLenum type,
                            const void *pixels) override
    {
        glTexImage2D(target, level, internalformat, width, height, border,
                     format, type, pixels);
    }

    virtual void texParameterf(GLenum target, GLenum pname,
                               GLfloat param) override
    {
        glTexParameterf(target, pname, param);
    }

    virtual void texParameterfv(GLenum target, GLenum pname,
                                const GLfloat *params) override
    {
        glTexParameterfv(target, pname, params);
    }

    virtual void texParameteri(GLenum target, GLenum pname,
                               GLint param) override
    {
        glTexParameteri(target, pname, param);
    }

    virtual void texParameteriv(GLenum target, GLenum pname,
                                const GLint *params) override
    {
        glTexParameteriv(target, pname, params);
    }

    virtual void texSubImage2D(GLenum target, GLint level, GLint xoffset,
                               GLint yoffset, GLsizei width, GLsizei height,
                               GLenum format, GLenum type,
                               const void *pixels) override
    {
        glTexSubImage2D(target, level, xoffset, yoffset, width, height, format,
                        type, pixels);
    }

    virtual void uniform1f(GLint location, GLfloat x) override
    {
        glUniform1f(location, x);
    }

    virtual void uniform1fv(GLint location, GLsizei count,
                            const GLfloat *v) override
    {
        glUniform1fv(location, count, v);
    }

    virtual void uniform1i(GLint location, GLint x) override
    {
        glUniform1i(location, x);
    }

    virtual void uniform1iv(GLint location, GLsizei count,
                            const GLint *v) override
    {
        glUniform1iv(location, count, v);
    }

    virtual void uniform2f(GLint location, GLfloat x, GLfloat y) override
    {
        glUniform2f(location, x, y);
    }

    virtual void uniform2fv(GLint location, GLsizei count,
                            const GLfloat *v) override
    {
        glUniform2fv(location, count, v);
    }

    virtual void uniform2i(GLint location, GLint x, GLint y) override
    {
        glUniform2i(location, x, y);
    }

    virtual void uniform2iv(GLint location, GLsizei count,
                            const GLint *v) override
    {
        glUniform2iv(location, count, v);
    }

    virtual void uniform3f(GLint location, GLfloat x, GLfloat y,
                           GLfloat z) override
    {
        glUniform3f(location, x, y, z);
    }

    virtual void uniform3fv(GLint location, GLsizei count,
                            const GLfloat *v) override
    {
        glUniform3fv(location, count, v);
    }

    virtual void uniform3i(GLint location, GLint x, GLint y, GLint z) override
    {
        glUniform3i(location, x, y, z);
    }

    virtual void uniform3iv(GLint location, GLsizei count,
                            const GLint *v) override
    {
        glUniform3iv(location, count, v);
    }

    virtual void uniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z,
                           GLfloat w) override
    {
        glUniform4f(location, x, y, z, w);
    }

    virtual void uniform4fv(GLint location, GLsizei count,
                            const GLfloat *v) override
    {
        glUniform4fv(location, count, v);
    }

    virtual void uniform4i(GLint location, GLint x, GLint y, GLint z,
                           GLint w) override
    {
        glUniform4i(location, x, y, z, w);
    }

    virtual void uniform4iv(GLint location, GLsizei count,
                            const GLint *v) override
    {
        glUniform4iv(location, count, v);
    }

    virtual void uniformMatrix2fv(GLint location, GLsizei count,
                                  GLboolean transpose,
                                  const GLfloat *value) override
    {
        glUniformMatrix2fv(location, count, transpose, value);
    }

    virtual void uniformMatrix3fv(GLint location, GLsizei count,
                                  GLboolean transpose,
                                  const GLfloat *value) override
    {
        glUniformMatrix3fv(location, count, transpose, value);
    }

    virtual void uniformMatrix4fv(GLint location, GLsizei count,
                                  GLboolean transpose,
                                  const GLfloat *value) override
    {
        glUniformMatrix4fv(location, count, transpose, value);
    }

    virtual void useProgram(GLuint program) override
    {
        glUseProgram(program);
    }

    virtual void validateProgram(GLuint program) override
    {
        glValidateProgram(program);
    }

    virtual void vertexAttrib1f(GLuint indx, GLfloat x) override
    {
        glVertexAttrib1f(indx, x);
    }

    virtual void vertexAttrib1fv(GLuint indx, const GLfloat *values) override
    {
        glVertexAttrib1fv(indx, values);
    }

    virtual void vertexAttrib2f(GLuint indx, GLfloat x, GLfloat y) override
    {
        glVertexAttrib2f(indx, x, y);
    }

    virtual void vertexAttrib2fv(GLuint indx, const GLfloat *values) override
    {
        glVertexAttrib2fv(indx, values);
    }

    virtual void vertexAttrib3f(GLuint indx, GLfloat x, GLfloat y,
                                GLfloat z) override
    {
        glVertexAttrib3f(indx, x, y, z);
    }

    virtual void vertexAttrib3fv(GLuint indx, const GLfloat *values) override
    {
        glVertexAttrib3fv(indx, values);
    }

    virtual void vertexAttrib4f(GLuint indx, GLfloat x, GLfloat y, GLfloat z,
                                GLfloat w) override
    {
        glVertexAttrib4f(indx, x, y, z, w);
    }

    virtual void vertexAttrib4fv(GLuint indx, const GLfloat *values) override
    {
        glVertexAttrib4fv(indx, values);
    }

    virtual void vertexAttribPointer(GLuint indx, GLint size, GLenum type,
                                     GLboolean normalized, GLsizei stride,
                                     const void *ptr) override
    {
        glVertexAttribPointer(indx, size, type, normalized, stride, ptr);
    }

    virtual void viewport(GLint x, GLint y, GLsizei width,
                          GLsizei height) override
    {
        glViewport(x, y, width, height);
    }

    virtual void genVertexArrays(GLsizei n, GLuint *arrays)
    {
        glGenVertexArrays(n, arrays);
    }

    virtual GLboolean isEnabled(GLenum cap) override
    {
        return glIsEnabled(cap);
    }

    virtual bool isGeneric() override
    {
        return true;
    }

    virtual void *xglCreateImage(int target, void *buffer,
                                 const int *attriblist) override
    {
        STARFISH_ASSERT(m_eglGetCurrentDisplayProc != nullptr);
        STARFISH_ASSERT(m_eglCreateImageKHRProc != nullptr);
        EGLDisplay display = m_eglGetCurrentDisplayProc();
        return m_eglCreateImageKHRProc(display, EGL_NO_CONTEXT, target, buffer,
                                       attriblist);
    }

    virtual void xglDestroyImage(void *image) override
    {
        STARFISH_ASSERT(m_eglGetCurrentDisplayProc != nullptr);
        STARFISH_ASSERT(m_eglDestroyImageKHRProc != nullptr);
        EGLDisplay display = m_eglGetCurrentDisplayProc();
        m_eglDestroyImageKHRProc(display, image);
    }

    virtual void xglImageTargetTexture2DOES(GLenum target, void *image) override
    {
#if !defined(STARFISH_WINDOWS)
        STARFISH_ASSERT(m_glEGLImageTargetTexture2DOESProc != nullptr);
        m_glEGLImageTargetTexture2DOESProc(target, image);
#endif
    }

    void uniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose,
                            const GLfloat *value) override
    {
        glUniformMatrix2x3fv(location, count, transpose, value);
    }

    void uniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose,
                            const GLfloat *value) override
    {
        glUniformMatrix3x2fv(location, count, transpose, value);
    }

    void uniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose,
                            const GLfloat *value) override
    {
        glUniformMatrix2x4fv(location, count, transpose, value);
    }

    void uniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose,
                            const GLfloat *value) override
    {
        glUniformMatrix4x2fv(location, count, transpose, value);
    }

    void uniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose,
                            const GLfloat *value) override
    {
        glUniformMatrix3x4fv(location, count, transpose, value);
    }

    void uniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose,
                            const GLfloat *value) override
    {
        glUniformMatrix4x3fv(location, count, transpose, value);
    }

    void bindVertexArray(GLuint array) override
    {
        glBindVertexArray(array);
    }

    void deleteVertexArrays(GLsizei n, const GLuint *arrays) override
    {
        glDeleteVertexArrays(n, arrays);
    }

    GLboolean isVertexArray(GLuint array) override
    {
        return glIsVertexArray(array);
    }

    void bindBufferRange(GLenum target, GLuint index, GLuint buffer,
                         GLintptr offset, GLsizeiptr size) override
    {
        glBindBufferRange(target, index, buffer, offset, size);
    }

    void bindBufferBase(GLenum target, GLuint index, GLuint buffer) override
    {
        glBindBufferBase(target, index, buffer);
    }

    void vertexAttribIPointer(GLuint index, GLint size, GLenum type,
                              GLsizei stride, const void *pointer) override
    {
        glVertexAttribIPointer(index, size, type, stride, pointer);
    }

    void getVertexAttribIiv(GLuint index, GLenum pname, GLint *params) override
    {
        glGetVertexAttribIiv(index, pname, params);
    }

    void getVertexAttribIuiv(GLuint index, GLenum pname,
                             GLuint *params) override
    {
        glGetVertexAttribIuiv(index, pname, params);
    }

    void vertexAttribI4i(GLuint index, GLint x, GLint y, GLint z,
                         GLint w) override
    {
        glVertexAttribI4i(index, x, y, z, w);
    }

    void vertexAttribI4ui(GLuint index, GLuint x, GLuint y, GLuint z,
                          GLuint w) override
    {
        glVertexAttribI4ui(index, x, y, z, w);
    }

    void vertexAttribI4iv(GLuint index, const GLint *v) override
    {
        glVertexAttribI4iv(index, v);
    }

    void vertexAttribI4uiv(GLuint index, const GLuint *v) override
    {
        glVertexAttribI4uiv(index, v);
    }

    void getUniformuiv(GLuint program, GLint location, GLuint *params) override
    {
        glGetUniformuiv(program, location, params);
    }

    GLint getFragDataLocation(GLuint program, const GLchar *name) override
    {
        return glGetFragDataLocation(program, name);
    }

    void uniform1ui(GLint location, GLuint v0) override
    {
        glUniform1ui(location, v0);
    }

    void uniform2ui(GLint location, GLuint v0, GLuint v1) override
    {
        glUniform2ui(location, v0, v1);
    }

    void uniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2) override
    {
        glUniform3ui(location, v0, v1, v2);
    }

    void uniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2,
                    GLuint v3) override
    {
        glUniform4ui(location, v0, v1, v2, v3);
    }

    void uniform1uiv(GLint location, GLsizei count,
                     const GLuint *value) override
    {
        glUniform1uiv(location, count, value);
    }

    void uniform2uiv(GLint location, GLsizei count,
                     const GLuint *value) override
    {
        glUniform2uiv(location, count, value);
    }

    void uniform3uiv(GLint location, GLsizei count,
                     const GLuint *value) override
    {
        glUniform3uiv(location, count, value);
    }

    void uniform4uiv(GLint location, GLsizei count,
                     const GLuint *value) override
    {
        glUniform4uiv(location, count, value);
    }

    void getActiveUniformsiv(GLuint program, GLsizei uniformCount,
                             const GLuint *uniformIndices, GLenum pname,
                             GLint *params) override
    {
        glGetActiveUniformsiv(program, uniformCount, uniformIndices, pname,
                              params);
    }

    GLsync fenceSync(GLenum condition, GLbitfield flags) override
    {
        return glFenceSync(condition, flags);
    }

    GLboolean isSync(GLsync sync) override
    {
        return glIsSync(sync);
    }

    void deleteSync(GLsync sync) override
    {
        glDeleteSync(sync);
    }

    GLenum clientWaitSync(GLsync sync, GLbitfield flags,
                          GLuint64 timeout) override
    {
        return glClientWaitSync(sync, flags, timeout);
    }

    void waitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) override
    {
        glWaitSync(sync, flags, timeout);
    }

    void getInteger64v(GLenum pname, GLint64 *data) override
    {
        glGetInteger64v(pname, data);
    }

    void getSynciv(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei *length,
                   GLint *values) override
    {
        glGetSynciv(sync, pname, bufSize, length, values);
    }

    void getBufferParameteri64v(GLenum target, GLenum pname,
                                GLint64 *params) override
    {
        glGetBufferParameteri64v(target, pname, params);
    }

    void invalidateFramebuffer(GLenum target, GLsizei numAttachments,
                               const GLenum *attachments) override
    {
        glInvalidateFramebuffer(target, numAttachments, attachments);
    }

    GenericGL(Renderer *renderer)
    {
        m_eglGetCurrentDisplayProc =
            reinterpret_cast<PFNGLEGLGETCURRENTDISPLAYPROC>(
                renderer->getProcAddress("eglGetCurrentDisplay"));

        m_eglCreateImageKHRProc = reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(
            renderer->getProcAddress("eglCreateImageKHR"));
        m_eglDestroyImageKHRProc = reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(
            renderer->getProcAddress("eglDestroyImageKHR"));
#if !defined(STARFISH_WINDOWS)
        m_glEGLImageTargetTexture2DOESProc =
            reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(
                renderer->getProcAddress("glEGLImageTargetTexture2DOES"));
#endif
        m_glInvalidateFramebuffer =
            reinterpret_cast<PFNGLINVALIDATEFRAMEBUFFERPROC>(
                renderer->getProcAddress("glInvalidateFramebuffer"));
    }

    ~GenericGL()
    {
        m_eglGetCurrentDisplayProc = nullptr;
        m_eglCreateImageKHRProc = nullptr;
        m_eglDestroyImageKHRProc = nullptr;
#if !defined(STARFISH_WINDOWS)
        m_glEGLImageTargetTexture2DOESProc = nullptr;
#endif
        m_glInvalidateFramebuffer = nullptr;
    }

private:
    PFNGLEGLGETCURRENTDISPLAYPROC m_eglGetCurrentDisplayProc = nullptr;
    PFNEGLCREATEIMAGEKHRPROC m_eglCreateImageKHRProc = nullptr;
    PFNEGLDESTROYIMAGEKHRPROC m_eglDestroyImageKHRProc = nullptr;
#if !defined(STARFISH_WINDOWS)
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC m_glEGLImageTargetTexture2DOESProc =
        nullptr;
#endif
    PFNGLINVALIDATEFRAMEBUFFERPROC m_glInvalidateFramebuffer = nullptr;
};

#if !defined(PORT_WEBVIEW_BRIDGE_EFL)
GL *GL::create(Renderer *renderer)
{
    return new GenericGL(renderer);
}
#endif
GL *GL::createGeneric(Renderer *renderer)
{
    return new GenericGL(renderer);
}

} // namespace Starfish

#endif
