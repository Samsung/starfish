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

#include "GL.h"
#include "core/page/WebView.h"
#include "core/modules/renderer/Renderer.h"

#if defined(PORT_WEBVIEW_BRIDGE_EFL) && defined(PORT_COMPOSITOR_BACKEND_GL)
#include <Evas_GL.h>

namespace Starfish {

class EvasGL : public GL {
public:
    virtual void activeTexture(GLenum texture) override
    {
        m_evasGLAPI->glActiveTexture(texture);
    }

    virtual void attachShader(GLuint program, GLuint shader) override
    {
        m_evasGLAPI->glAttachShader(program, shader);
    }

    virtual void bindAttribLocation(GLuint program, GLuint index,
                                    const GLchar *name) override
    {
        m_evasGLAPI->glBindAttribLocation(program, index, name);
    }

    virtual void bindBuffer(GLenum target, GLuint buffer) override
    {
        m_evasGLAPI->glBindBuffer(target, buffer);
    }

    virtual void bindFramebuffer(GLenum target, GLuint framebuffer) override
    {
        m_evasGLAPI->glBindFramebuffer(target, framebuffer);
    }

    virtual void bindRenderbuffer(GLenum target, GLuint renderbuffer) override
    {
        m_evasGLAPI->glBindRenderbuffer(target, renderbuffer);
    }

    virtual void bindTexture(GLenum target, GLuint texture) override
    {
        m_evasGLAPI->glBindTexture(target, texture);
    }

    virtual void blendColor(GLfloat red, GLfloat green, GLfloat blue,
                            GLfloat alpha) override
    {
        m_evasGLAPI->glBlendColor(red, green, blue, alpha);
    }

    virtual void blendEquation(GLenum mode) override
    {
        m_evasGLAPI->glBlendEquation(mode);
    }

    virtual void blendEquationSeparate(GLenum modeRGB,
                                       GLenum modeAlpha) override
    {
        m_evasGLAPI->glBlendEquationSeparate(modeRGB, modeAlpha);
    }

    virtual void blendFunc(GLenum sfactor, GLenum dfactor) override
    {
        m_evasGLAPI->glBlendFunc(sfactor, dfactor);
    }

    virtual void blendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB,
                                   GLenum sfactorAlpha,
                                   GLenum dfactorAlpha) override
    {
        m_evasGLAPI->glBlendFuncSeparate(sfactorRGB, dfactorRGB, sfactorAlpha,
                                         dfactorAlpha);
    }

    virtual void bufferData(GLenum target, GLsizeiptr size, const void *data,
                            GLenum usage) override
    {
        m_evasGLAPI->glBufferData(target, size, data, usage);
    }

    virtual void bufferSubData(GLenum target, GLintptr offset, GLsizeiptr size,
                               const void *data) override
    {
        m_evasGLAPI->glBufferSubData(target, offset, size, data);
    }

    virtual GLenum checkFramebufferStatus(GLenum target) override
    {
        return m_evasGLAPI->glCheckFramebufferStatus(target);
    }

    virtual void clear(GLbitfield mask) override
    {
        m_evasGLAPI->glClear(mask);
    }

    virtual void clearColor(GLclampf red, GLclampf green, GLclampf blue,
                            GLclampf alpha) override
    {
        m_evasGLAPI->glClearColor(red, green, blue, alpha);
    }

    virtual void clearDepthf(GLfloat d) override
    {
        m_evasGLAPI->glClearDepthf(d);
    }

    virtual void clearStencil(GLint s) override
    {
        m_evasGLAPI->glClearStencil(s);
    }

    virtual void colorMask(GLboolean red, GLboolean green, GLboolean blue,
                           GLboolean alpha) override
    {
        m_evasGLAPI->glColorMask(red, green, blue, alpha);
    }

    virtual void shaderSource(GLuint shader, GLsizei count,
                              const char *const *string,
                              const GLint *length) override
    {
        m_evasGLAPI->glShaderSource(shader, count, string, length);
    }

    virtual void getShaderInfoLog(GLuint shader, GLsizei bufsize,
                                  GLsizei *length, char *infolog) override
    {
        m_evasGLAPI->glGetShaderInfoLog(shader, bufsize, length, infolog);
    }

    virtual void getShaderSource(GLuint shader, GLsizei bufSize,
                                 GLsizei *length, GLchar *source)
    {
        m_evasGLAPI->glGetShaderSource(shader, bufSize, length, source);
    }

    virtual void compileShader(GLuint shader) override
    {
        m_evasGLAPI->glCompileShader(shader);
    }

    virtual void compressedTexImage2D(GLenum target, GLint level,
                                      GLenum internalformat, GLsizei width,
                                      GLsizei height, GLint border,
                                      GLsizei imageSize,
                                      const void *data) override
    {
        m_evasGLAPI->glCompressedTexImage2D(target, level, internalformat,
                                            width, height, border, imageSize,
                                            data);
    }

    virtual void compressedTexSubImage2D(GLenum target, GLint level,
                                         GLint xoffset, GLint yoffset,
                                         GLsizei width, GLsizei height,
                                         GLenum format, GLsizei imageSize,
                                         const void *data) override
    {
        m_evasGLAPI->glCompressedTexSubImage2D(target, level, xoffset, yoffset,
                                               width, height, format, imageSize,
                                               data);
    }

    virtual GLuint createProgram(void) override
    {
        return m_evasGLAPI->glCreateProgram();
    }

    virtual GLuint createShader(GLenum type) override
    {
        return m_evasGLAPI->glCreateShader(type);
    }

    virtual void cullFace(GLenum mode) override
    {
        m_evasGLAPI->glCullFace(mode);
    }

    virtual void deleteBuffers(GLsizei n, const GLuint *buffers) override
    {
        m_evasGLAPI->glDeleteBuffers(n, buffers);
    }

    virtual void deleteFramebuffers(GLsizei n,
                                    const GLuint *framebuffers) override
    {
        m_evasGLAPI->glDeleteFramebuffers(n, framebuffers);
    }

    virtual void deleteProgram(GLuint program) override
    {
        m_evasGLAPI->glDeleteProgram(program);
    }

    virtual void deleteRenderbuffers(GLsizei n,
                                     const GLuint *renderbuffers) override
    {
        m_evasGLAPI->glDeleteRenderbuffers(n, renderbuffers);
    }

    virtual void deleteShader(GLuint shader) override
    {
        m_evasGLAPI->glDeleteShader(shader);
    }

    virtual void deleteTextures(GLsizei n, const GLuint *textures) override
    {
        m_evasGLAPI->glDeleteTextures(n, textures);
    }

    virtual void depthFunc(GLenum func) override
    {
        m_evasGLAPI->glDepthFunc(func);
    }

    virtual void depthMask(GLboolean flag) override
    {
        m_evasGLAPI->glDepthMask(flag);
    }

    virtual void depthRangef(GLfloat n, GLfloat f) override
    {
        m_evasGLAPI->glDepthRangef(n, f);
    }

    virtual void detachShader(GLuint program, GLuint shader) override
    {
        m_evasGLAPI->glDetachShader(program, shader);
    }

    virtual void disable(GLenum cap) override
    {
        m_evasGLAPI->glDisable(cap);
    }

    virtual void disableVertexAttribArray(GLuint index) override
    {
        m_evasGLAPI->glDisableVertexAttribArray(index);
    }

    virtual void drawArrays(GLenum mode, GLint first, GLsizei count) override
    {
        m_evasGLAPI->glDrawArrays(mode, first, count);
    }

    virtual void drawElements(GLenum mode, GLsizei count, GLenum type,
                              const void *indices) override
    {
        m_evasGLAPI->glDrawElements(mode, count, type, indices);
    }

    virtual void enable(GLenum cap) override
    {
        m_evasGLAPI->glEnable(cap);
    }

    virtual void enableVertexAttribArray(GLuint index) override
    {
        m_evasGLAPI->glEnableVertexAttribArray(index);
    }

    virtual void finish(void) override
    {
        m_evasGLAPI->glFinish();
    }

    virtual void flush(void) override
    {
        m_evasGLAPI->glFlush();
    }

    virtual void framebufferRenderbuffer(GLenum target, GLenum attachment,
                                         GLenum renderbuffertarget,
                                         GLuint renderbuffer) override
    {
        m_evasGLAPI->glFramebufferRenderbuffer(
            target, attachment, renderbuffertarget, renderbuffer);
    }

    virtual void framebufferTexture2D(GLenum target, GLenum attachment,
                                      GLenum textarget, GLuint texture,
                                      GLint level) override
    {
        m_evasGLAPI->glFramebufferTexture2D(target, attachment, textarget,
                                            texture, level);
    }

    virtual void frontFace(GLenum mode) override
    {
        m_evasGLAPI->glFrontFace(mode);
    }

    virtual void genBuffers(GLsizei n, GLuint *buffers) override
    {
        m_evasGLAPI->glGenBuffers(n, buffers);
    }

    virtual void generateMipmap(GLenum target) override
    {
        m_evasGLAPI->glGenerateMipmap(target);
    }

    virtual void genFramebuffers(GLsizei n, GLuint *framebuffers) override
    {
        m_evasGLAPI->glGenFramebuffers(n, framebuffers);
    }

    virtual void genRenderbuffers(GLsizei n, GLuint *renderbuffers) override
    {
        m_evasGLAPI->glGenRenderbuffers(n, renderbuffers);
    }

    virtual void genTextures(GLsizei n, GLuint *textures) override
    {
        m_evasGLAPI->glGenTextures(n, textures);
    }

    virtual void getActiveAttrib(GLuint program, GLuint index, GLsizei bufsize,
                                 GLsizei *length, GLint *size, GLenum *type,
                                 char *name) override
    {
        m_evasGLAPI->glGetActiveAttrib(program, index, bufsize, length, size,
                                       type, name);
    }

    virtual void getActiveUniform(GLuint program, GLuint index, GLsizei bufsize,
                                  GLsizei *length, GLint *size, GLenum *type,
                                  char *name) override
    {
        m_evasGLAPI->glGetActiveUniform(program, index, bufsize, length, size,
                                        type, name);
    }

    virtual void getAttachedShaders(GLuint program, GLsizei maxcount,
                                    GLsizei *count, GLuint *shaders) override
    {
        m_evasGLAPI->glGetAttachedShaders(program, maxcount, count, shaders);
    }

    virtual int getAttribLocation(GLuint program, const char *name) override
    {
        return m_evasGLAPI->glGetAttribLocation(program, name);
    }

    virtual void getBooleanv(GLenum pname, GLboolean *params) override
    {
        m_evasGLAPI->glGetBooleanv(pname, params);
    }

    virtual void getBufferParameteriv(GLenum target, GLenum pname,
                                      GLint *params) override
    {
        m_evasGLAPI->glGetBufferParameteriv(target, pname, params);
    }

    virtual GLenum getError(void) override
    {
        return m_evasGLAPI->glGetError();
    }

    virtual void getFloatv(GLenum pname, GLfloat *params) override
    {
        m_evasGLAPI->glGetFloatv(pname, params);
    }

    virtual void getFramebufferAttachmentParameteriv(GLenum target,
                                                     GLenum attachment,
                                                     GLenum pname,
                                                     GLint *params) override
    {
        m_evasGLAPI->glGetFramebufferAttachmentParameteriv(target, attachment,
                                                           pname, params);
    }

    virtual void getIntegerv(GLenum pname, GLint *params) override
    {
        m_evasGLAPI->glGetIntegerv(pname, params);
    }

    virtual void getProgramiv(GLuint program, GLenum pname,
                              GLint *params) override
    {
        m_evasGLAPI->glGetProgramiv(program, pname, params);
    }

    virtual void getProgramInfoLog(GLuint program, GLsizei bufSize,
                                   GLsizei *length, GLchar *infoLog) override
    {
        m_evasGLAPI->glGetProgramInfoLog(program, bufSize, length, infoLog);
    }

    virtual void getRenderbufferParameteriv(GLenum target, GLenum pname,
                                            GLint *params) override
    {
        m_evasGLAPI->glGetRenderbufferParameteriv(target, pname, params);
    }

    virtual void getShaderiv(GLuint shader, GLenum pname,
                             GLint *params) override
    {
        m_evasGLAPI->glGetShaderiv(shader, pname, params);
    }

    virtual const GLubyte *getString(GLenum name) override
    {
        return m_evasGLAPI->glGetString(name);
    }

    virtual void getUniformfv(GLuint program, GLint location,
                              GLfloat *params) override
    {
        m_evasGLAPI->glGetUniformfv(program, location, params);
    }

    virtual void getUniformiv(GLuint program, GLint location,
                              GLint *params) override
    {
        m_evasGLAPI->glGetUniformiv(program, location, params);
    }

    virtual GLint getUniformLocation(GLuint program, const char *name) override
    {
        return m_evasGLAPI->glGetUniformLocation(program, name);
    }

    virtual void getVertexAttribfv(GLuint index, GLenum pname,
                                   GLfloat *params) override
    {
        m_evasGLAPI->glGetVertexAttribfv(index, pname, params);
    }

    virtual void getVertexAttribiv(GLuint index, GLenum pname,
                                   GLint *params) override
    {
        m_evasGLAPI->glGetVertexAttribiv(index, pname, params);
    }

    virtual void getVertexAttribPointerv(GLuint index, GLenum pname,
                                         void **pointer) override
    {
        m_evasGLAPI->glGetVertexAttribPointerv(index, pname, pointer);
    }

    virtual void hint(GLenum target, GLenum mode) override
    {
        m_evasGLAPI->glHint(target, mode);
    }

    virtual void lineWidth(GLfloat width) override
    {
        m_evasGLAPI->glLineWidth(width);
    }

    virtual void linkProgram(GLuint program) override
    {
        m_evasGLAPI->glLinkProgram(program);
    }

    virtual void pixelStorei(GLenum pname, GLint param) override
    {
        m_evasGLAPI->glPixelStorei(pname, param);
    }

    virtual void polygonOffset(GLfloat factor, GLfloat units) override
    {
        m_evasGLAPI->glPolygonOffset(factor, units);
    }

    virtual void readPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                            GLenum format, GLenum type, void *pixels) override
    {
        m_evasGLAPI->glReadPixels(x, y, width, height, format, type, pixels);
    }

    virtual void releaseShaderCompiler(void) override
    {
        m_evasGLAPI->glReleaseShaderCompiler();
    }

    virtual void renderbufferStorage(GLenum target, GLenum internalformat,
                                     GLsizei width, GLsizei height) override
    {
        m_evasGLAPI->glRenderbufferStorage(target, internalformat, width,
                                           height);
    }

    virtual void sampleCoverage(GLfloat value, GLboolean invert) override
    {
        m_evasGLAPI->glSampleCoverage(value, invert);
    }

    virtual void scissor(GLint x, GLint y, GLsizei width,
                         GLsizei height) override
    {
        m_evasGLAPI->glScissor(x, y, width, height);
    }

    virtual void stencilFunc(GLenum func, GLint ref, GLuint mask) override
    {
        m_evasGLAPI->glStencilFunc(func, ref, mask);
    }

    virtual void stencilFuncSeparate(GLenum face, GLenum func, GLint ref,
                                     GLuint mask) override
    {
        m_evasGLAPI->glStencilFuncSeparate(face, func, ref, mask);
    }

    virtual void stencilMask(GLuint mask) override
    {
        m_evasGLAPI->glStencilMask(mask);
    }

    virtual void stencilMaskSeparate(GLenum face, GLuint mask) override
    {
        m_evasGLAPI->glStencilMaskSeparate(face, mask);
    }

    virtual void stencilOp(GLenum fail, GLenum zfail, GLenum zpass) override
    {
        m_evasGLAPI->glStencilOp(fail, zfail, zpass);
    }

    virtual void stencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail,
                                   GLenum dppass) override
    {
        m_evasGLAPI->glStencilOpSeparate(face, sfail, dpfail, dppass);
    }

    virtual void texImage2D(GLenum target, GLint level, GLint internalformat,
                            GLsizei width, GLsizei height, GLint border,
                            GLenum format, GLenum type,
                            const void *pixels) override
    {
        m_evasGLAPI->glTexImage2D(target, level, internalformat, width, height,
                                  border, format, type, pixels);
    }

    virtual void texParameterf(GLenum target, GLenum pname,
                               GLfloat param) override
    {
        m_evasGLAPI->glTexParameterf(target, pname, param);
    }

    virtual void texParameterfv(GLenum target, GLenum pname,
                                const GLfloat *params) override
    {
        m_evasGLAPI->glTexParameterfv(target, pname, params);
    }

    virtual void texParameteri(GLenum target, GLenum pname,
                               GLint param) override
    {
        m_evasGLAPI->glTexParameteri(target, pname, param);
    }

    virtual void texParameteriv(GLenum target, GLenum pname,
                                const GLint *params) override
    {
        m_evasGLAPI->glTexParameteriv(target, pname, params);
    }

    virtual void texSubImage2D(GLenum target, GLint level, GLint xoffset,
                               GLint yoffset, GLsizei width, GLsizei height,
                               GLenum format, GLenum type,
                               const void *pixels) override
    {
        m_evasGLAPI->glTexSubImage2D(target, level, xoffset, yoffset, width,
                                     height, format, type, pixels);
    }

    virtual void uniform1f(GLint location, GLfloat x) override
    {
        m_evasGLAPI->glUniform1f(location, x);
    }

    virtual void uniform1fv(GLint location, GLsizei count,
                            const GLfloat *v) override
    {
        m_evasGLAPI->glUniform1fv(location, count, v);
    }

    virtual void uniform1i(GLint location, GLint x) override
    {
        m_evasGLAPI->glUniform1i(location, x);
    }

    virtual void uniform1iv(GLint location, GLsizei count,
                            const GLint *v) override
    {
        m_evasGLAPI->glUniform1iv(location, count, v);
    }

    virtual void uniform2f(GLint location, GLfloat x, GLfloat y) override
    {
        m_evasGLAPI->glUniform2f(location, x, y);
    }

    virtual void uniform2fv(GLint location, GLsizei count,
                            const GLfloat *v) override
    {
        m_evasGLAPI->glUniform2fv(location, count, v);
    }

    virtual void uniform2i(GLint location, GLint x, GLint y) override
    {
        m_evasGLAPI->glUniform2i(location, x, y);
    }

    virtual void uniform2iv(GLint location, GLsizei count,
                            const GLint *v) override
    {
        m_evasGLAPI->glUniform2iv(location, count, v);
    }

    virtual void uniform3f(GLint location, GLfloat x, GLfloat y,
                           GLfloat z) override
    {
        m_evasGLAPI->glUniform3f(location, x, y, z);
    }

    virtual void uniform3fv(GLint location, GLsizei count,
                            const GLfloat *v) override
    {
        m_evasGLAPI->glUniform3fv(location, count, v);
    }

    virtual void uniform3i(GLint location, GLint x, GLint y, GLint z) override
    {
        m_evasGLAPI->glUniform3i(location, x, y, z);
    }

    virtual void uniform3iv(GLint location, GLsizei count,
                            const GLint *v) override
    {
        m_evasGLAPI->glUniform3iv(location, count, v);
    }

    virtual void uniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z,
                           GLfloat w) override
    {
        m_evasGLAPI->glUniform4f(location, x, y, z, w);
    }

    virtual void uniform4fv(GLint location, GLsizei count,
                            const GLfloat *v) override
    {
        m_evasGLAPI->glUniform4fv(location, count, v);
    }

    virtual void uniform4i(GLint location, GLint x, GLint y, GLint z,
                           GLint w) override
    {
        m_evasGLAPI->glUniform4i(location, x, y, z, w);
    }

    virtual void uniform4iv(GLint location, GLsizei count,
                            const GLint *v) override
    {
        m_evasGLAPI->glUniform4iv(location, count, v);
    }

    virtual void uniformMatrix2fv(GLint location, GLsizei count,
                                  GLboolean transpose,
                                  const GLfloat *value) override
    {
        m_evasGLAPI->glUniformMatrix2fv(location, count, transpose, value);
    }

    virtual void uniformMatrix3fv(GLint location, GLsizei count,
                                  GLboolean transpose,
                                  const GLfloat *value) override
    {
        m_evasGLAPI->glUniformMatrix3fv(location, count, transpose, value);
    }

    virtual void uniformMatrix4fv(GLint location, GLsizei count,
                                  GLboolean transpose,
                                  const GLfloat *value) override
    {
        m_evasGLAPI->glUniformMatrix4fv(location, count, transpose, value);
    }

    virtual void useProgram(GLuint program) override
    {
        m_evasGLAPI->glUseProgram(program);
    }

    virtual void validateProgram(GLuint program) override
    {
        m_evasGLAPI->glValidateProgram(program);
    }

    virtual void vertexAttrib1f(GLuint indx, GLfloat x) override
    {
        m_evasGLAPI->glVertexAttrib1f(indx, x);
    }

    virtual void vertexAttrib1fv(GLuint indx, const GLfloat *values) override
    {
        m_evasGLAPI->glVertexAttrib1fv(indx, values);
    }

    virtual void vertexAttrib2f(GLuint indx, GLfloat x, GLfloat y) override
    {
        m_evasGLAPI->glVertexAttrib2f(indx, x, y);
    }

    virtual void vertexAttrib2fv(GLuint indx, const GLfloat *values) override
    {
        m_evasGLAPI->glVertexAttrib2fv(indx, values);
    }

    virtual void vertexAttrib3f(GLuint indx, GLfloat x, GLfloat y,
                                GLfloat z) override
    {
        m_evasGLAPI->glVertexAttrib3f(indx, x, y, z);
    }

    virtual void vertexAttrib3fv(GLuint indx, const GLfloat *values) override
    {
        m_evasGLAPI->glVertexAttrib3fv(indx, values);
    }

    virtual void vertexAttrib4f(GLuint indx, GLfloat x, GLfloat y, GLfloat z,
                                GLfloat w) override
    {
        m_evasGLAPI->glVertexAttrib4f(indx, x, y, z, w);
    }

    virtual void vertexAttrib4fv(GLuint indx, const GLfloat *values) override
    {
        m_evasGLAPI->glVertexAttrib4fv(indx, values);
    }

    virtual void vertexAttribPointer(GLuint indx, GLint size, GLenum type,
                                     GLboolean normalized, GLsizei stride,
                                     const void *ptr) override
    {
        m_evasGLAPI->glVertexAttribPointer(indx, size, type, normalized, stride,
                                           ptr);
    }

    virtual void viewport(GLint x, GLint y, GLsizei width,
                          GLsizei height) override
    {
        m_evasGLAPI->glViewport(x, y, width, height);
    }

    virtual GLboolean isEnabled(GLenum cap) override
    {
        return m_evasGLAPI->glIsEnabled(cap);
    }

    virtual bool isGeneric() override
    {
        return false;
    }

    virtual SkMatrix computeScreenMatrix(Renderer *renderer, size_t screenWidth,
                                         size_t screenHeight) override
    {
        SkMatrix m = SkMatrix::I();
        void *evasGL =
            renderer->webView()
                ->publicLayerUserDataMap()["__internalLWEWebViewEvasGL"];
        if (evasGL) {
            const size_t w = screenWidth;
            const size_t h = screenHeight;
            int deg = evas_gl_rotation_get(static_cast<Evas_GL *>(evasGL));
            if (deg % 180 == 90) {
                float tx = w / 2.f;
                float ty = h / 2.f;

                m.preTranslate(tx, ty);

                SkMatrix t = SkMatrix::I();
                t.preRotate(360 - deg);
                t.preScale(h / (float)w, w / (float)h);
                m.preConcat(t);

                m.preTranslate(-tx, -ty);
            } else if (deg == 180) {
                float tx = w / 2.f;
                float ty = h / 2.f;
                m.preTranslate(tx, ty);

                SkMatrix t = SkMatrix::I();
                t.preRotate(360 - deg);
                m.preConcat(t);

                m.preTranslate(-tx, -ty);
            }
        }
        return SkMatrix::I();
    }

    virtual void *evasglCreateImage(int target, void *buffer,
                                    const int *attriblist) override
    {
        return m_evasGLAPI->evasglCreateImage(target, buffer, attriblist);
    }

    virtual void evasglDestroyImage(void *image) override
    {
        m_evasGLAPI->evasglDestroyImage(image);
    }

    virtual void evasGLImageTargetTexture2DOES(GLenum target,
                                               void *image) override
    {
        m_evasGLAPI->glEvasGLImageTargetTexture2DOES(target, image);
    }

    EvasGL(void *p)
        : m_evasGLAPI(static_cast<Evas_GL_API *>(p))
    {
    }

private:
    Evas_GL_API *m_evasGLAPI;
};

GL *GL::create(Renderer *r)
{
    void *glAPI =
        r->webView()->publicLayerUserDataMap()["__internalLWEWebViewEvasGLAPI"];
    if (glAPI) {
        return new EvasGL(glAPI);
    }
    return GL::createGeneric();
}

} // namespace Starfish
#endif
