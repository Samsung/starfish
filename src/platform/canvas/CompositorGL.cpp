/*
 *  Copyright (C) 2011 Google Inc. All rights reserved.
 *  Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
 *  Copyright (C) 2012 Igalia S.L.
 *  Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

// #define STARFISH_ENABLE_PROFILE_TIMER

#include "StarfishConfig.h"
#include "Starfish.h"

#if defined(PORT_COMPOSITOR_BACKEND_GL)

#include "core/style/Style.h"
#include "core/modules/canvas/Compositor.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/image/NativeImageData.h"
#include "core/page/WebView.h"
#include "platform/window/PlatformWindow.h"

#if defined(STARFISH_ENABLE_TEST) && defined(PORT_CANVAS_BACKEND_CAIRO)
#include <cairo.h>
#endif

#include <array>
#include <SkMatrix.h>
#include <clipper.hpp>

#include <earcut.hpp>
// The number type to use for tessellation
using Coord = double;
// The index type. Defaults to uint32_t, but you can also pass uint16_t if you
// know that your
// data won't have more than 65536 vertices.
using N = uint32_t;
// Create array
using Point = std::array<Coord, 2>;

// We only Support OpenGL ES 2.0+ context

#if defined(PORT_WEBVIEW_BRIDGE_EFL)
#include <Evas_GL.h>
#if defined(STARFISH_TIZEN) // PORT_WEBVIEW_BRIDGE_EFL + STARFISH_TIZEN
#include <tbm_surface.h>
#endif
#elif defined(STARFISH_TIZEN) // STARFISH_TIZEN with out PORT_WEBVIEW_BRIDGE_EFL
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/eglext.h>
#include <tbm_surface.h>
static PFNEGLCREATEIMAGEKHRPROC g_eglCreateImageKHRProc;
static PFNEGLDESTROYIMAGEKHRPROC g_eglDestroyImageKHRProc;
static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC g_glEGLImageTargetTexture2DOESProc;
#define EGL_NATIVE_SURFACE_TIZEN 0x32A1
#elif defined(STARFISH_WINDOWS)
#include <GL/glew.h>
#include <GL/wglew.h>

#include <Windows.h>
#pragma comment(lib, "Opengl32.lib")
#elif defined(STARFISH_ANDROID)
#define EGL_EGLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#include <android/hardware_buffer.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <GLES3/gl3.h>

static void logEglError(const char* name) noexcept
{
    const char* err;
    switch (eglGetError()) {
    case EGL_NOT_INITIALIZED:
        err = "EGL_NOT_INITIALIZED";
        break;
    case EGL_BAD_ACCESS:
        err = "EGL_BAD_ACCESS";
        break;
    case EGL_BAD_ALLOC:
        err = "EGL_BAD_ALLOC";
        break;
    case EGL_BAD_ATTRIBUTE:
        err = "EGL_BAD_ATTRIBUTE";
        break;
    case EGL_BAD_CONTEXT:
        err = "EGL_BAD_CONTEXT";
        break;
    case EGL_BAD_CONFIG:
        err = "EGL_BAD_CONFIG";
        break;
    case EGL_BAD_CURRENT_SURFACE:
        err = "EGL_BAD_CURRENT_SURFACE";
        break;
    case EGL_BAD_DISPLAY:
        err = "EGL_BAD_DISPLAY";
        break;
    case EGL_BAD_SURFACE:
        err = "EGL_BAD_SURFACE";
        break;
    case EGL_BAD_MATCH:
        err = "EGL_BAD_MATCH";
        break;
    case EGL_BAD_PARAMETER:
        err = "EGL_BAD_PARAMETER";
        break;
    case EGL_BAD_NATIVE_PIXMAP:
        err = "EGL_BAD_NATIVE_PIXMAP";
        break;
    case EGL_BAD_NATIVE_WINDOW:
        err = "EGL_BAD_NATIVE_WINDOW";
        break;
    case EGL_CONTEXT_LOST:
        err = "EGL_CONTEXT_LOST";
        break;
    default:
        err = "unknown";
        break;
    }
    STARFISH_LOG_ERROR("%s failed with %s\n", name, err);
}
#else
#include <GLES3/gl3.h>
#endif

#if defined(PORT_WEBVIEW_BRIDGE_EFL)
#define glActiveTexture g_evasGLAPI->glActiveTexture
#define glAttachShader g_evasGLAPI->glAttachShader
#define glBindAttribLocation g_evasGLAPI->glBindAttribLocation
#define glBindBuffer g_evasGLAPI->glBindBuffer
#define glBindFramebuffer g_evasGLAPI->glBindFramebuffer
#define glBindRenderbuffer g_evasGLAPI->glBindRenderbuffer
#define glBindTexture g_evasGLAPI->glBindTexture
#define glBlendColor g_evasGLAPI->glBlendColor
#define glBlendEquation g_evasGLAPI->glBlendEquation
#define glBlendEquationSeparate g_evasGLAPI->glBlendEquationSeparate
#define glBlendFunc g_evasGLAPI->glBlendFunc
#define glBlendFuncSeparate g_evasGLAPI->glBlendFuncSeparate
#define glBufferData g_evasGLAPI->glBufferData
#define glBufferSubData g_evasGLAPI->glBufferSubData
#define glCheckFramebufferStatus g_evasGLAPI->glCheckFramebufferStatus
#define glClear g_evasGLAPI->glClear
#define glClearColor g_evasGLAPI->glClearColor
#define glClearDepthf g_evasGLAPI->glClearDepthf
#define glClearStencil g_evasGLAPI->glClearStencil
#define glColorMask g_evasGLAPI->glColorMask
#define glCompileShader g_evasGLAPI->glCompileShader
#define glCompressedTexImage2D g_evasGLAPI->glCompressedTexImage2D
#define glCompressedTexSubImage2D g_evasGLAPI->glCompressedTexSubImage2D
#define glCopyTexImage2D g_evasGLAPI->glCopyTexImage2D
#define glCopyTexSubImage2D g_evasGLAPI->glCopyTexSubImage2D
#define glCreateProgram g_evasGLAPI->glCreateProgram
#define glCreateShader g_evasGLAPI->glCreateShader
#define glCullFace g_evasGLAPI->glCullFace
#define glDeleteBuffers g_evasGLAPI->glDeleteBuffers
#define glDeleteFramebuffers g_evasGLAPI->glDeleteFramebuffers
#define glDeleteProgram g_evasGLAPI->glDeleteProgram
#define glDeleteRenderbuffers g_evasGLAPI->glDeleteRenderbuffers
#define glDeleteShader g_evasGLAPI->glDeleteShader
#define glDeleteTextures g_evasGLAPI->glDeleteTextures
#define glDepthFunc g_evasGLAPI->glDepthFunc
#define glDepthMask g_evasGLAPI->glDepthMask
#define glDepthRangef g_evasGLAPI->glDepthRangef
#define glDetachShader g_evasGLAPI->glDetachShader
#define glDisable g_evasGLAPI->glDisable
#define glDisableVertexAttribArray g_evasGLAPI->glDisableVertexAttribArray
#define glDrawArrays g_evasGLAPI->glDrawArrays
#define glDrawElements g_evasGLAPI->glDrawElements
#define glEnable g_evasGLAPI->glEnable
#define glEnableVertexAttribArray g_evasGLAPI->glEnableVertexAttribArray
#define glFinish g_evasGLAPI->glFinish
#define glFlush g_evasGLAPI->glFlush
#define glFramebufferRenderbuffer g_evasGLAPI->glFramebufferRenderbuffer
#define glFramebufferTexture2D g_evasGLAPI->glFramebufferTexture2D
#define glFrontFace g_evasGLAPI->glFrontFace
#define glGenBuffers g_evasGLAPI->glGenBuffers
#define glGenerateMipmap g_evasGLAPI->glGenerateMipmap
#define glGenFramebuffers g_evasGLAPI->glGenFramebuffers
#define glGenRenderbuffers g_evasGLAPI->glGenRenderbuffers
#define glGenTextures g_evasGLAPI->glGenTextures
#define glGetActiveAttrib g_evasGLAPI->glGetActiveAttrib
#define glGetActiveUniform g_evasGLAPI->glGetActiveUniform
#define glGetAttachedShaders g_evasGLAPI->glGetAttachedShaders
#define glGetAttribLocation g_evasGLAPI->glGetAttribLocation
#define glGetBooleanv g_evasGLAPI->glGetBooleanv
#define glGetBufferParameteriv g_evasGLAPI->glGetBufferParameteriv
#define glGetError g_evasGLAPI->glGetError
#define glGetFloatv g_evasGLAPI->glGetFloatv
#define glGetFramebufferAttachmentParameteriv \
    g_evasGLAPI->glGetFramebufferAttachmentParameteriv
#define glGetIntegerv g_evasGLAPI->glGetIntegerv
#define glGetProgramiv g_evasGLAPI->glGetProgramiv
#define glGetProgramInfoLog g_evasGLAPI->glGetProgramInfoLog
#define glGetRenderbufferParameteriv g_evasGLAPI->glGetRenderbufferParameteriv
#define glGetShaderiv g_evasGLAPI->glGetShaderiv
#define glGetShaderInfoLog g_evasGLAPI->glGetShaderInfoLog
#define glGetShaderPrecisionFormat g_evasGLAPI->glGetShaderPrecisionFormat
#define glGetShaderSource g_evasGLAPI->glGetShaderSource
#define glGetString g_evasGLAPI->glGetString
#define glGetTexParameterfv g_evasGLAPI->glGetTexParameterfv
#define glGetTexParameteriv g_evasGLAPI->glGetTexParameteriv
#define glGetUniformfv g_evasGLAPI->glGetUniformfv
#define glGetUniformiv g_evasGLAPI->glGetUniformiv
#define glGetUniformLocation g_evasGLAPI->glGetUniformLocation
#define glGetVertexAttribfv g_evasGLAPI->glGetVertexAttribfv
#define glGetVertexAttribiv g_evasGLAPI->glGetVertexAttribiv
#define glGetVertexAttribPointerv g_evasGLAPI->glGetVertexAttribPointerv
#define glHint g_evasGLAPI->glHint
#define glIsBuffer g_evasGLAPI->glIsBuffer
#define glIsEnabled g_evasGLAPI->glIsEnabled
#define glIsFramebuffer g_evasGLAPI->glIsFramebuffer
#define glIsProgram g_evasGLAPI->glIsProgram
#define glIsRenderbuffer g_evasGLAPI->glIsRenderbuffer
#define glIsShader g_evasGLAPI->glIsShader
#define glIsTexture g_evasGLAPI->glIsTexture
#define glLineWidth g_evasGLAPI->glLineWidth
#define glLinkProgram g_evasGLAPI->glLinkProgram
#define glPixelStorei g_evasGLAPI->glPixelStorei
#define glPolygonOffset g_evasGLAPI->glPolygonOffset
#define glReadPixels g_evasGLAPI->glReadPixels
#define glReleaseShaderCompiler g_evasGLAPI->glReleaseShaderCompiler
#define glRenderbufferStorage g_evasGLAPI->glRenderbufferStorage
#define glSampleCoverage g_evasGLAPI->glSampleCoverage
#define glScissor g_evasGLAPI->glScissor
#define glShaderBinary g_evasGLAPI->glShaderBinary
#define glShaderSource g_evasGLAPI->glShaderSource
#define glStencilFunc g_evasGLAPI->glStencilFunc
#define glStencilFuncSeparate g_evasGLAPI->glStencilFuncSeparate
#define glStencilMask g_evasGLAPI->glStencilMask
#define glStencilMaskSeparate g_evasGLAPI->glStencilMaskSeparate
#define glStencilOp g_evasGLAPI->glStencilOp
#define glStencilOpSeparate g_evasGLAPI->glStencilOpSeparate
#define glTexImage2D g_evasGLAPI->glTexImage2D
#define glTexParameterf g_evasGLAPI->glTexParameterf
#define glTexParameterfv g_evasGLAPI->glTexParameterfv
#define glTexParameteri g_evasGLAPI->glTexParameteri
#define glTexParameteriv g_evasGLAPI->glTexParameteriv
#define glTexSubImage2D g_evasGLAPI->glTexSubImage2D
#define glUniform1f g_evasGLAPI->glUniform1f
#define glUniform1fv g_evasGLAPI->glUniform1fv
#define glUniform1i g_evasGLAPI->glUniform1i
#define glUniform1iv g_evasGLAPI->glUniform1iv
#define glUniform2f g_evasGLAPI->glUniform2f
#define glUniform2fv g_evasGLAPI->glUniform2fv
#define glUniform2i g_evasGLAPI->glUniform2i
#define glUniform2iv g_evasGLAPI->glUniform2iv
#define glUniform3f g_evasGLAPI->glUniform3f
#define glUniform3fv g_evasGLAPI->glUniform3fv
#define glUniform3i g_evasGLAPI->glUniform3i
#define glUniform3iv g_evasGLAPI->glUniform3iv
#define glUniform4f g_evasGLAPI->glUniform4f
#define glUniform4fv g_evasGLAPI->glUniform4fv
#define glUniform4i g_evasGLAPI->glUniform4i
#define glUniform4iv g_evasGLAPI->glUniform4iv
#define glUniformMatrix2fv g_evasGLAPI->glUniformMatrix2fv
#define glUniformMatrix3fv g_evasGLAPI->glUniformMatrix3fv
#define glUniformMatrix4fv g_evasGLAPI->glUniformMatrix4fv
#define glUseProgram g_evasGLAPI->glUseProgram
#define glValidateProgram g_evasGLAPI->glValidateProgram
#define glVertexAttrib1f g_evasGLAPI->glVertexAttrib1f
#define glVertexAttrib1fv g_evasGLAPI->glVertexAttrib1fv
#define glVertexAttrib2f g_evasGLAPI->glVertexAttrib2f
#define glVertexAttrib2fv g_evasGLAPI->glVertexAttrib2fv
#define glVertexAttrib3f g_evasGLAPI->glVertexAttrib3f
#define glVertexAttrib3fv g_evasGLAPI->glVertexAttrib3fv
#define glVertexAttrib4f g_evasGLAPI->glVertexAttrib4f
#define glVertexAttrib4fv g_evasGLAPI->glVertexAttrib4fv
#define glVertexAttribPointer g_evasGLAPI->glVertexAttribPointer
#define glViewport g_evasGLAPI->glViewport
#define glBeginQuery g_evasGLAPI->glBeginQuery
#define glBeginTransformFeedback g_evasGLAPI->glBeginTransformFeedback
#define glBindBufferBase g_evasGLAPI->glBindBufferBase
#define glBindBufferRange g_evasGLAPI->glBindBufferRange
#define glBindSampler g_evasGLAPI->glBindSampler
#define glBindTransformFeedback g_evasGLAPI->glBindTransformFeedback
#define glBindVertexArray g_evasGLAPI->glBindVertexArray
#define glBlitFramebuffer g_evasGLAPI->glBlitFramebuffer
#define glClearBufferfi g_evasGLAPI->glClearBufferfi
#define glClearBufferfv g_evasGLAPI->glClearBufferfv
#define glClearBufferiv g_evasGLAPI->glClearBufferiv
#define glClearBufferuiv g_evasGLAPI->glClearBufferuiv
#define glClientWaitSync g_evasGLAPI->glClientWaitSync
#define glCompressedTexImage3D g_evasGLAPI->glCompressedTexImage3D
#define glCompressedTexSubImage3D g_evasGLAPI->glCompressedTexSubImage3D
#define glCopyBufferSubData g_evasGLAPI->glCopyBufferSubData
#define glCopyTexSubImage3D g_evasGLAPI->glCopyTexSubImage3D
#define glDeleteQueries g_evasGLAPI->glDeleteQueries
#define glDeleteSamplers g_evasGLAPI->glDeleteSamplers
#define glDeleteSync g_evasGLAPI->glDeleteSync
#define glDeleteTransformFeedbacks g_evasGLAPI->glDeleteTransformFeedbacks
#define glDeleteVertexArrays g_evasGLAPI->glDeleteVertexArrays
#define glDrawArraysInstanced g_evasGLAPI->glDrawArraysInstanced
#define glDrawBuffers g_evasGLAPI->glDrawBuffers
#define glDrawElementsInstanced g_evasGLAPI->glDrawElementsInstanced
#define glDrawRangeElements g_evasGLAPI->glDrawRangeElements
#define glEndQuery g_evasGLAPI->glEndQuery
#define glEndTransformFeedback g_evasGLAPI->glEndTransformFeedback
#define glFenceSync g_evasGLAPI->glFenceSync
#define glFlushMappedBufferRange g_evasGLAPI->glFlushMappedBufferRange
#define glFramebufferTextureLayer g_evasGLAPI->glFramebufferTextureLayer
#define glGenQueries g_evasGLAPI->glGenQueries
#define glGenSamplers g_evasGLAPI->glGenSamplers
#define glGenTransformFeedbacks g_evasGLAPI->glGenTransformFeedbacks
#define glGenVertexArrays g_evasGLAPI->glGenVertexArrays
#define glGetActiveUniformBlockiv g_evasGLAPI->glGetActiveUniformBlockiv
#define glGetActiveUniformBlockName g_evasGLAPI->glGetActiveUniformBlockName
#define glGetActiveUniformsiv g_evasGLAPI->glGetActiveUniformsiv
#define glGetBufferParameteri64v g_evasGLAPI->glGetBufferParameteri64v
#define glGetBufferPointerv g_evasGLAPI->glGetBufferPointerv
#define glGetFragDataLocation g_evasGLAPI->glGetFragDataLocation
#define glGetInteger64i_v g_evasGLAPI->glGetInteger64i_v
#define glGetInteger64v g_evasGLAPI->glGetInteger64v
#define glGetIntegeri_v g_evasGLAPI->glGetIntegeri_v
#define glGetInternalformativ g_evasGLAPI->glGetInternalformativ
#define glGetProgramBinary g_evasGLAPI->glGetProgramBinary
#define glGetQueryiv g_evasGLAPI->glGetQueryiv
#define glGetQueryObjectuiv g_evasGLAPI->glGetQueryObjectuiv
#define glGetSamplerParameterfv g_evasGLAPI->glGetSamplerParameterfv
#define glGetSamplerParameteriv g_evasGLAPI->glGetSamplerParameteriv
#define glGetStringi g_evasGLAPI->glGetStringi
#define glGetSynciv g_evasGLAPI->glGetSynciv
#define glGetTransformFeedbackVarying g_evasGLAPI->glGetTransformFeedbackVarying
#define glGetUniformBlockIndex g_evasGLAPI->glGetUniformBlockIndex
#define glGetUniformIndices g_evasGLAPI->glGetUniformIndices
#define glGetUniformuiv g_evasGLAPI->glGetUniformuiv
#define glGetVertexAttribIiv g_evasGLAPI->glGetVertexAttribIiv
#define glGetVertexAttribIuiv g_evasGLAPI->glGetVertexAttribIuiv
#define glInvalidateFramebuffer g_evasGLAPI->glInvalidateFramebuffer
#define glInvalidateSubFramebuffer g_evasGLAPI->glInvalidateSubFramebuffer
#define glIsQuery g_evasGLAPI->glIsQuery
#define glIsSampler g_evasGLAPI->glIsSampler
#define glIsSync g_evasGLAPI->glIsSync
#define glIsTransformFeedback g_evasGLAPI->glIsTransformFeedback
#define glIsVertexArray g_evasGLAPI->glIsVertexArray
#define glMapBufferRange g_evasGLAPI->glMapBufferRange
#define glPauseTransformFeedback g_evasGLAPI->glPauseTransformFeedback
#define glProgramBinary g_evasGLAPI->glProgramBinary
#define glProgramParameteri g_evasGLAPI->glProgramParameteri
#define glReadBuffer g_evasGLAPI->glReadBuffer
#define glRenderbufferStorageMultisample \
    g_evasGLAPI->glRenderbufferStorageMultisample
#define glResumeTransformFeedback g_evasGLAPI->glResumeTransformFeedback
#define glSamplerParameterf g_evasGLAPI->glSamplerParameterf
#define glSamplerParameterfv g_evasGLAPI->glSamplerParameterfv
#define glSamplerParameteri g_evasGLAPI->glSamplerParameteri
#define glSamplerParameteriv g_evasGLAPI->glSamplerParameteriv
#define glTexImage3D g_evasGLAPI->glTexImage3D
#define glTexStorage2D g_evasGLAPI->glTexStorage2D
#define glTexStorage3D g_evasGLAPI->glTexStorage3D
#define glTexSubImage3D g_evasGLAPI->glTexSubImage3D
#define glTransformFeedbackVaryings g_evasGLAPI->glTransformFeedbackVaryings
#define glUniform1ui g_evasGLAPI->glUniform1ui
#define glUniform1uiv g_evasGLAPI->glUniform1uiv
#define glUniform2ui g_evasGLAPI->glUniform2ui
#define glUniform2uiv g_evasGLAPI->glUniform2uiv
#define glUniform3ui g_evasGLAPI->glUniform3ui
#define glUniform3uiv g_evasGLAPI->glUniform3uiv
#define glUniform4ui g_evasGLAPI->glUniform4ui
#define glUniform4uiv g_evasGLAPI->glUniform4uiv
#define glUniformBlockBinding g_evasGLAPI->glUniformBlockBinding
#define glUniformMatrix2x3fv g_evasGLAPI->glUniformMatrix2x3fv
#define glUniformMatrix3x2fv g_evasGLAPI->glUniformMatrix3x2fv
#define glUniformMatrix2x4fv g_evasGLAPI->glUniformMatrix2x4fv
#define glUniformMatrix4x2fv g_evasGLAPI->glUniformMatrix4x2fv
#define glUniformMatrix3x4fv g_evasGLAPI->glUniformMatrix3x4fv
#define glUniformMatrix4x3fv g_evasGLAPI->glUniformMatrix4x3fv
#define glUnmapBuffer g_evasGLAPI->glUnmapBuffer
#define glVertexAttribDivisor g_evasGLAPI->glVertexAttribDivisor
#define glVertexAttribI4i g_evasGLAPI->glVertexAttribI4i
#define glVertexAttribI4iv g_evasGLAPI->glVertexAttribI4iv
#define glVertexAttribI4ui g_evasGLAPI->glVertexAttribI4ui
#define glVertexAttribI4uiv g_evasGLAPI->glVertexAttribI4uiv
#define glVertexAttribIPointer g_evasGLAPI->glVertexAttribIPointer
#define glWaitSync g_evasGLAPI->glWaitSync

#endif

#ifndef GL_TEXTURE_EXTERNAL_OES
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#endif

#ifndef GL_BGRA_EXT
#define GL_BGRA_EXT 0x80E1
#endif

#ifndef GL_MAJOR_VERSION
#define GL_MAJOR_VERSION 0x821B
#endif

#ifndef GL_MINOR_VERSION
#define GL_MINOR_VERSION 0x821C
#endif

#ifndef GL_UNPACK_ROW_LENGTH
#define GL_UNPACK_ROW_LENGTH 0x0CF2
#endif

#ifndef GL_UNPACK_SKIP_ROWS
#define GL_UNPACK_SKIP_ROWS 0x0CF3
#endif

#ifndef GL_UNPACK_SKIP_PIXELS
#define GL_UNPACK_SKIP_PIXELS 0x0CF4
#endif

#ifndef GL_DEPTH_STENCIL
#define GL_DEPTH_STENCIL 0x84F9
#endif

#ifndef GL_UNSIGNED_INT_24_8
#define GL_UNSIGNED_INT_24_8 0x84FA
#endif

#if defined(PORT_WEBVIEW_BRIDGE_EFL)
Evas_GL_API* g_evasGLAPI;
Evas_GL* g_evasGL;
bool g_isEvasGLOnDirectMode;
#endif

namespace Starfish {

static size_t g_textureTileSize = 512;
static bool g_needsCheckCompatibility = true;
static bool g_isSupportPixelStoreiUnpackingOfPixelDataFromMemory = false;
static bool g_isSupportExtensionEGLImageExternal = false;
static bool g_shouldUseEGLImageOnPlainSurface = false;
static bool g_useStencilBufferOnFBO = false;
static size_t g_maxTextureSize;

static void checkError()
{
#ifndef NDEBUG
    volatile auto error = glGetError();
    if (error != 0) {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
#endif
}

static GLuint loadShader(GLenum type, const GLchar* shaderSrc)
{
    GLuint shader;
    GLint compiled;

    checkError();
    LongTaskFinder t("loadShader");

    // Create the shader object
    shader = glCreateShader(type);

    // Load the shader source
    glShaderSource(shader, 1, &shaderSrc, NULL);

    // Compile the shader
    glCompileShader(shader);

    // Check the compile status
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    checkError();

    if (!compiled) {
        GLint maxLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

        // The maxLength includes the NULL character
        std::vector<GLchar> errorLog(maxLength);
        glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);

        STARFISH_LOG_ERROR("loadShader error.. shader source -> %s\n",
                           shaderSrc);
        STARFISH_LOG_ERROR("loadShader error.. error desc -> %s\n",
                           errorLog.data());
        // Provide the infolog in whatever manor you deem best.
        // Exit with failure.
        glDeleteShader(shader); // Don't leak the shader.
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
    return shader;
}

class CompositorContext {
public:
    GLuint m_rectVertexShader;
    GLuint m_rectFragmentShader;
    GLuint m_rectShaderProgram;
    GLint m_rectShaderProgramPosition;
    GLint m_rectShaderProgramColor;

    GLuint m_texVertexShader;
    GLuint m_texFragmentShader;
    GLuint m_texShaderProgram;
    GLint m_texShaderProgramTexPos;
    GLint m_texShaderProgramPosition;
    GLint m_texShaderProgramTexture;
    GLint m_texShaderProgramAlpha;
    GLuint m_texFragmentShaderWithOriginalColor;
    GLuint m_texShaderProgramWithOriginalColor;

    GLuint m_texFragmentShaderEGLImageExternal;
    GLuint m_texShaderProgramEGLImageExternal;
    GLint m_texShaderProgramEGLImageExternalTexPos;
    GLint m_texShaderProgramEGLImageExternalPosition;
    GLint m_texShaderProgramEGLImageExternalTexture;
    GLint m_texShaderProgramEGLImageExternalAlpha;

    GLuint m_texFragmentShaderEGLImageExternalWithOriginalColor;
    GLuint m_texShaderProgramEGLImageExternalWithOriginalColor;
    GLint m_texShaderProgramEGLImageExternalWithOriginalColorTexPos;
    GLint m_texShaderProgramEGLImageExternalWithOriginalColorPosition;
    GLint m_texShaderProgramEGLImageExternalWithOriginalColorTexture;
    GLint m_texShaderProgramEGLImageExternalWithOriginalColorAlpha;

    GLuint m_texFragmentBlurShaderW;
    GLuint m_texFragmentBlurShaderEGLImageExternalW;
    GLuint m_texFragmentBlurShaderH;

    GLuint m_texBlurShaderProgramW;
    GLint m_texBlurShaderProgramWTexPos;
    GLint m_texBlurShaderProgramWPosition;
    GLint m_texBlurShaderProgramWTexture;
    GLint m_texBlurShaderProgramWBlurRadius;
    GLint m_texBlurShaderProgramWTextureWidth;
    GLint m_texBlurShaderProgramWTextureHeight;
    GLuint m_texBlurShaderProgramEGLImageExternalW;
    GLint m_texBlurShaderProgramEGLImageExternalWTexPos;
    GLint m_texBlurShaderProgramEGLImageExternalWPosition;
    GLint m_texBlurShaderProgramEGLImageExternalWTexture;
    GLint m_texBlurShaderProgramEGLImageExternalWBlurRadius;
    GLint m_texBlurShaderProgramEGLImageExternalWTextureWidth;
    GLint m_texBlurShaderProgramEGLImageExternalWTextureHeight;

    GLuint m_texBlurShaderProgramH;
    GLint m_texBlurShaderProgramHTexPos;
    GLint m_texBlurShaderProgramHPosition;
    GLint m_texBlurShaderProgramHTexture;
    GLint m_texBlurShaderProgramHBlurRadius;
    GLint m_texBlurShaderProgramHTextureWidth;
    GLint m_texBlurShaderProgramHTextureHeight;
    GLint m_texBlurShaderProgramHAlpha;

    GLuint m_texTexPosBuffer;

    GLuint m_lastProgram;

    CompositorContext()
    {
        m_rectVertexShader = m_rectFragmentShader = m_rectShaderProgram =
            m_texShaderProgram = 0;
        m_rectShaderProgramPosition = 0;
        m_rectShaderProgramColor = 0;
        m_texShaderProgramPosition = 0;
        m_texShaderProgramTexture = 0;
        m_texShaderProgramAlpha = 0;
        m_texShaderProgramEGLImageExternalPosition = 0;
        m_texShaderProgramEGLImageExternalTexture = 0;
        m_texShaderProgramEGLImageExternalAlpha = 0;
        m_texShaderProgramEGLImageExternalWithOriginalColorPosition = 0;
        m_texShaderProgramEGLImageExternalWithOriginalColorTexture = 0;
        m_texShaderProgramEGLImageExternalWithOriginalColorAlpha = 0;
        m_texVertexShader = m_texFragmentShader = 0;
        m_texShaderProgramEGLImageExternal =
            m_texFragmentShaderEGLImageExternal = 0;
        m_texShaderProgramEGLImageExternalWithOriginalColor =
            m_texFragmentShaderEGLImageExternalWithOriginalColor = 0;
        m_texFragmentBlurShaderH = m_texFragmentBlurShaderW =
            m_texFragmentBlurShaderEGLImageExternalW = 0;
        m_texBlurShaderProgramW = m_texBlurShaderProgramEGLImageExternalW =
            m_texBlurShaderProgramH = 0;
        m_texFragmentShaderWithOriginalColor = 0;
        m_texShaderProgramWithOriginalColor = 0;

        m_texBlurShaderProgramWPosition = 0;
        m_texBlurShaderProgramWTexture = 0;
        m_texBlurShaderProgramWBlurRadius = 0;
        m_texBlurShaderProgramWTextureWidth = 0;
        m_texBlurShaderProgramWTextureHeight = 0;

        m_texBlurShaderProgramEGLImageExternalWPosition = 0;
        m_texBlurShaderProgramEGLImageExternalWTexture = 0;
        m_texBlurShaderProgramEGLImageExternalWBlurRadius = 0;
        m_texBlurShaderProgramEGLImageExternalWTextureWidth = 0;
        m_texBlurShaderProgramEGLImageExternalWTextureHeight = 0;

        m_texBlurShaderProgramHPosition = 0;
        m_texBlurShaderProgramHTexture = 0;
        m_texBlurShaderProgramHBlurRadius = 0;
        m_texBlurShaderProgramHTextureWidth = 0;
        m_texBlurShaderProgramHTextureHeight = 0;
        m_texBlurShaderProgramHAlpha = 0;

        m_texTexPosBuffer = 0;
        m_texShaderProgramTexPos = 0;
        m_texShaderProgramEGLImageExternalTexPos = 0;
        m_texShaderProgramEGLImageExternalWithOriginalColorTexPos = 0;
        m_texBlurShaderProgramWTexPos = 0;
        m_texBlurShaderProgramEGLImageExternalWTexPos = 0;
        m_texBlurShaderProgramHTexPos = 0;

        m_lastProgram = 0;
    }

    GLuint rectProgram()
    {
        if (!m_rectShaderProgram) {
            GLchar rectVertexSource[] =
                "attribute vec2 aPosition;\n"
                "void main() {\n"
                "  gl_Position = vec4(aPosition.xy, 0.0, 1.0);\n"
                "}";

            GLchar rectFragmentSource[] =
                "#ifdef GL_ES\n"
                "  precision mediump float;\n"
                "#endif\n"
                "uniform vec4 uColor;\n"
                "void main(void)\n"
                "{\n"
                "  gl_FragColor = uColor;\n"
                "}";

            m_rectVertexShader = loadShader(GL_VERTEX_SHADER, rectVertexSource);
            checkError();
            m_rectFragmentShader =
                loadShader(GL_FRAGMENT_SHADER, rectFragmentSource);
            checkError();

            m_rectShaderProgram = glCreateProgram();
            checkError();

            glAttachShader(m_rectShaderProgram, m_rectVertexShader);
            checkError();
            glAttachShader(m_rectShaderProgram, m_rectFragmentShader);
            checkError();

            glLinkProgram(m_rectShaderProgram);
            checkError();

            m_lastProgram = m_rectShaderProgram;
            glUseProgram(m_rectShaderProgram);

            m_rectShaderProgramPosition =
                glGetAttribLocation(m_rectShaderProgram, "aPosition");
            m_rectShaderProgramColor =
                glGetUniformLocation(m_rectShaderProgram, "uColor");

            glEnableVertexAttribArray(m_rectShaderProgramPosition);
        } else {
            if (m_lastProgram != m_rectShaderProgram) {
                m_lastProgram = m_rectShaderProgram;
                glUseProgram(m_rectShaderProgram);
            }
        }

        return m_rectShaderProgram;
    }

    GLuint texVertexShader()
    {
        if (!m_texVertexShader) {
            GLchar texVertexSource[] =
                "attribute vec2 aPosition;\n"
                "attribute vec2 aTexPos;\n"
                "varying vec2 vTexPos;\n"
                "void main() {\n"
                "  vTexPos = aTexPos;\n"
                "  gl_Position = vec4(aPosition.xy, 0.0, 1.0);\n"
                "}";
            m_texVertexShader = loadShader(GL_VERTEX_SHADER, texVertexSource);
        }
        return m_texVertexShader;
    }

    GLuint texShaderProgramEGLImageExternal()
    {
        if (!m_texShaderProgramEGLImageExternal) {
            GLchar texFragmentSourceEGLImageExternal[] =
                "#extension GL_OES_EGL_image_external : require\n"
                "#ifdef GL_ES\n"
                "  precision mediump float;\n"
                "#endif\n"
                "uniform samplerExternalOES uTexture;\n"
                "varying vec2 vTexPos;\n"
                "uniform float uAlpha;\n"
                "void main(void)\n"
                "{\n"
                "  vec4 texData = texture2D(uTexture, vTexPos) * uAlpha;\n"
#if defined(PORT_PIXEL_ORDER_BGRA)
                "  gl_FragColor.r = texData[2];\n"
                "  gl_FragColor.g = texData[1];\n"
                "  gl_FragColor.b = texData[0];\n"
                "  gl_FragColor.a = texData[3];\n"
#else
                "  gl_FragColor.r = texData[0];\n"
                "  gl_FragColor.g = texData[1];\n"
                "  gl_FragColor.b = texData[2];\n"
                "  gl_FragColor.a = texData[3];\n"
#endif
                "}";

            m_texFragmentShaderEGLImageExternal = loadShader(
                GL_FRAGMENT_SHADER, texFragmentSourceEGLImageExternal);
            checkError();

            m_texShaderProgramEGLImageExternal = glCreateProgram();
            checkError();

            glAttachShader(m_texShaderProgramEGLImageExternal,
                           texVertexShader());
            checkError();
            glAttachShader(m_texShaderProgramEGLImageExternal,
                           m_texFragmentShaderEGLImageExternal);
            checkError();

            glLinkProgram(m_texShaderProgramEGLImageExternal);
            checkError();

            m_lastProgram = m_texShaderProgramEGLImageExternal;
            glUseProgram(m_texShaderProgramEGLImageExternal);
            checkError();

            m_texShaderProgramEGLImageExternalPosition = glGetAttribLocation(
                m_texShaderProgramEGLImageExternal, "aPosition");
            m_texShaderProgramEGLImageExternalTexPos = glGetAttribLocation(
                m_texShaderProgramEGLImageExternal, "aTexPos");
            m_texShaderProgramEGLImageExternalTexture = glGetUniformLocation(
                m_texShaderProgramEGLImageExternal, "uTexture");
            m_texShaderProgramEGLImageExternalAlpha = glGetUniformLocation(
                m_texShaderProgramEGLImageExternal, "uAlpha");

            glUniform1i(m_texShaderProgramEGLImageExternalTexture, 0);
            glEnableVertexAttribArray(
                m_texShaderProgramEGLImageExternalPosition);

            glBindBuffer(GL_ARRAY_BUFFER, m_texTexPosBuffer);
            glEnableVertexAttribArray(m_texShaderProgramEGLImageExternalTexPos);
            glVertexAttribPointer(m_texShaderProgramEGLImageExternalTexPos, 2,
                                  GL_FLOAT, false, 0, 0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            glUniform1f(m_texShaderProgramEGLImageExternalAlpha, 1);
        } else {
            if (m_lastProgram != m_texShaderProgramEGLImageExternal) {
                m_lastProgram = m_texShaderProgramEGLImageExternal;
                glUseProgram(m_texShaderProgramEGLImageExternal);

                glBindBuffer(GL_ARRAY_BUFFER, m_texTexPosBuffer);
                glVertexAttribPointer(m_texShaderProgramEGLImageExternalTexPos,
                                      2, GL_FLOAT, false, 0, 0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }
        }

        return m_texShaderProgramEGLImageExternal;
    }

    GLuint texShaderProgramEGLImageExternalWithOriginalColor()
    {
        if (!m_texShaderProgramEGLImageExternalWithOriginalColor) {
            GLchar texFragmentSourceEGLImageExternal[] =
                "#extension GL_OES_EGL_image_external : require\n"
                "#ifdef GL_ES\n"
                "  precision mediump float;\n"
                "#endif\n"
                "uniform samplerExternalOES uTexture;\n"
                "varying vec2 vTexPos;\n"
                "uniform float uAlpha;\n"
                "void main(void)\n"
                "{\n"
                "  gl_FragColor = texture2D(uTexture, vTexPos) * uAlpha;\n"
                "}";

            m_texFragmentShaderEGLImageExternalWithOriginalColor = loadShader(
                GL_FRAGMENT_SHADER, texFragmentSourceEGLImageExternal);
            checkError();

            m_texShaderProgramEGLImageExternalWithOriginalColor =
                glCreateProgram();
            checkError();

            glAttachShader(m_texShaderProgramEGLImageExternalWithOriginalColor,
                           texVertexShader());
            checkError();
            glAttachShader(
                m_texShaderProgramEGLImageExternalWithOriginalColor,
                m_texFragmentShaderEGLImageExternalWithOriginalColor);
            checkError();

            glLinkProgram(m_texShaderProgramEGLImageExternalWithOriginalColor);
            checkError();

            m_lastProgram =
                m_texShaderProgramEGLImageExternalWithOriginalColorAlpha;
            glUseProgram(m_texShaderProgramEGLImageExternalWithOriginalColor);
            checkError();

            m_texShaderProgramEGLImageExternalWithOriginalColorPosition =
                glGetAttribLocation(
                    m_texShaderProgramEGLImageExternalWithOriginalColor,
                    "aPosition");
            m_texShaderProgramEGLImageExternalWithOriginalColorTexPos =
                glGetAttribLocation(
                    m_texShaderProgramEGLImageExternalWithOriginalColor,
                    "aTexPos");
            m_texShaderProgramEGLImageExternalWithOriginalColorTexture =
                glGetUniformLocation(
                    m_texShaderProgramEGLImageExternalWithOriginalColor,
                    "uTexture");
            m_texShaderProgramEGLImageExternalWithOriginalColorAlpha =
                glGetUniformLocation(
                    m_texShaderProgramEGLImageExternalWithOriginalColor,
                    "uAlpha");

            glUniform1i(
                m_texShaderProgramEGLImageExternalWithOriginalColorTexture, 0);
            glEnableVertexAttribArray(
                m_texShaderProgramEGLImageExternalWithOriginalColorPosition);

            glBindBuffer(GL_ARRAY_BUFFER, m_texTexPosBuffer);
            glEnableVertexAttribArray(
                m_texShaderProgramEGLImageExternalWithOriginalColorTexPos);
            glVertexAttribPointer(
                m_texShaderProgramEGLImageExternalWithOriginalColorTexPos, 2,
                GL_FLOAT, false, 0, 0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            glUniform1f(
                m_texShaderProgramEGLImageExternalWithOriginalColorAlpha, 1);
        } else {
            if (m_lastProgram !=
                m_texShaderProgramEGLImageExternalWithOriginalColor) {
                m_lastProgram =
                    m_texShaderProgramEGLImageExternalWithOriginalColor;
                glUseProgram(
                    m_texShaderProgramEGLImageExternalWithOriginalColor);

                glBindBuffer(GL_ARRAY_BUFFER, m_texTexPosBuffer);
                glVertexAttribPointer(
                    m_texShaderProgramEGLImageExternalWithOriginalColorTexPos,
                    2, GL_FLOAT, false, 0, 0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }
        }

        return m_texShaderProgramEGLImageExternalWithOriginalColor;
    }

    GLuint texShaderProgram()
    {
        if (!m_texShaderProgram) {
            // We only Support OpenGL ES 2.0+ context
            // but some develoment environment only support desktop context
            // so we add `#ifdef GL_ES` for debug purpose
            GLchar texFragmentSource[] =
                "#ifdef GL_ES\n"
                "  precision mediump float;\n"
                "#endif\n"
                "uniform sampler2D uTexture;\n"
                "varying vec2 vTexPos;\n"
                "uniform float uAlpha;\n"
                "void main(void)\n"
                "{\n"
                "  vec4 texData = texture2D(uTexture, vTexPos) * uAlpha;\n"
#if defined(PORT_PIXEL_ORDER_BGRA)
                "  gl_FragColor.r = texData[2];\n"
                "  gl_FragColor.g = texData[1];\n"
                "  gl_FragColor.b = texData[0];\n"
                "  gl_FragColor.a = texData[3];\n"
#else
                "  gl_FragColor.r = texData[0];\n"
                "  gl_FragColor.g = texData[1];\n"
                "  gl_FragColor.b = texData[2];\n"
                "  gl_FragColor.a = texData[3];\n"
#endif
                "}";

            m_texFragmentShader =
                loadShader(GL_FRAGMENT_SHADER, texFragmentSource);
            checkError();

            m_texShaderProgram = glCreateProgram();
            checkError();

            glAttachShader(m_texShaderProgram, texVertexShader());
            checkError();
            glAttachShader(m_texShaderProgram, m_texFragmentShader);
            checkError();

            glLinkProgram(m_texShaderProgram);
            checkError();

            glUseProgram(m_texShaderProgram);
            checkError();

            m_texShaderProgramPosition =
                glGetAttribLocation(m_texShaderProgram, "aPosition");
            m_texShaderProgramTexPos =
                glGetAttribLocation(m_texShaderProgram, "aTexPos");
            m_texShaderProgramTexture =
                glGetUniformLocation(m_texShaderProgram, "uTexture");
            m_texShaderProgramAlpha =
                glGetUniformLocation(m_texShaderProgram, "uAlpha");

            glUniform1i(m_texShaderProgramTexture, 0);
            glEnableVertexAttribArray(m_texShaderProgramPosition);

            glBindBuffer(GL_ARRAY_BUFFER, m_texTexPosBuffer);
            glEnableVertexAttribArray(m_texShaderProgramTexPos);
            glVertexAttribPointer(m_texShaderProgramTexPos, 2, GL_FLOAT, false,
                                  0, 0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            glUniform1f(m_texShaderProgramAlpha, 1);
        } else {
            if (m_lastProgram != m_texShaderProgram) {
                m_lastProgram = m_texShaderProgram;
                glUseProgram(m_texShaderProgram);

                glBindBuffer(GL_ARRAY_BUFFER, m_texTexPosBuffer);
                glVertexAttribPointer(m_texShaderProgramTexPos, 2, GL_FLOAT,
                                      false, 0, 0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }
        }
        return m_texShaderProgram;
    }

    GLuint texShaderProgramWithOriginalColor()
    {
        // this program MUST use same code except color conversion with
        // texShaderProgram
        if (!m_texShaderProgramWithOriginalColor) {
            GLchar texFragmentSource[] =
                "#ifdef GL_ES\n"
                "  precision mediump float;\n"
                "#endif\n"
                "uniform sampler2D uTexture;\n"
                "varying vec2 vTexPos;\n"
                "uniform float uAlpha;\n"
                "void main(void)\n"
                "{\n"
                "  gl_FragColor = texture2D(uTexture, vTexPos) * uAlpha;\n"
                "}";

            m_texFragmentShaderWithOriginalColor =
                loadShader(GL_FRAGMENT_SHADER, texFragmentSource);
            checkError();

            m_texShaderProgramWithOriginalColor = glCreateProgram();
            checkError();

            glAttachShader(m_texShaderProgramWithOriginalColor,
                           texVertexShader());
            checkError();
            glAttachShader(m_texShaderProgramWithOriginalColor,
                           m_texFragmentShaderWithOriginalColor);
            checkError();

            glLinkProgram(m_texShaderProgramWithOriginalColor);
            checkError();

            glUseProgram(m_texShaderProgramWithOriginalColor);
            checkError();

            glUniform1i(m_texShaderProgramTexture, 0);
            glEnableVertexAttribArray(m_texShaderProgramPosition);

            glBindBuffer(GL_ARRAY_BUFFER, m_texTexPosBuffer);
            glEnableVertexAttribArray(m_texShaderProgramTexPos);
            glVertexAttribPointer(m_texShaderProgramTexPos, 2, GL_FLOAT, false,
                                  0, 0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            glUniform1f(m_texShaderProgramAlpha, 1);
        } else {
            if (m_lastProgram != m_texShaderProgramWithOriginalColor) {
                m_lastProgram = m_texShaderProgramWithOriginalColor;
                glUseProgram(m_texShaderProgramWithOriginalColor);

                glBindBuffer(GL_ARRAY_BUFFER, m_texTexPosBuffer);
                glVertexAttribPointer(m_texShaderProgramTexPos, 2, GL_FLOAT,
                                      false, 0, 0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }
        }
        return m_texShaderProgramWithOriginalColor;
    }

// I take blur shader source from WebKit
// https://github.com/WebKit/webkit/blob/master/Source/WebCore/platform/graphics/texmap/TextureMapperShaderProgram.cpp(6f9b511a115311b13c06eb58038ddc2c78da5531)
#define GAUSSIAN_KERNEL_HALF_WIDTH 11
#define GAUSSIAN_KERNEL_STEP 0.2

    static inline float gauss(float x)
    {
        return exp(-(x * x) / 2.);
    }

    static std::vector<float> computeGaussianKernel()
    {
        std::vector<float> kernel;
        kernel.resize(GAUSSIAN_KERNEL_HALF_WIDTH);

        kernel[0] = gauss(0);
        float sum = kernel[0];
        for (unsigned i = 1; i < GAUSSIAN_KERNEL_HALF_WIDTH; ++i) {
            kernel[i] = gauss(i * GAUSSIAN_KERNEL_STEP);
            sum += 2 * kernel[i];
        }

        // Normalize the kernel.
        float scale = 1 / sum;
        for (unsigned i = 0; i < GAUSSIAN_KERNEL_HALF_WIDTH; ++i)
            kernel[i] *= scale;

        return kernel;
    }

    static std::string generateBlurEffectFragmentShader(
        bool isEGLImage, bool addColorAlign = false)
    {
        std::vector<float> gaussianKernel = computeGaussianKernel();
        std::stringstream ss;

        if (isEGLImage) {
            ss << "#extension GL_OES_EGL_image_external : require\n";
        }
        ss << "#ifdef GL_ES\n";
        ss << "  precision mediump float;\n";
        ss << "#endif\n";
        if (isEGLImage) {
            ss << "uniform samplerExternalOES uTexture;\n";
        } else {
            ss << "uniform sampler2D uTexture;\n";
        }

        ss << "uniform float uTextureWidth;\n";
        ss << "uniform float uTextureHeight;\n";
        if (addColorAlign) {
            ss << "uniform float uAlpha;\n";
        }
        ss << "uniform vec2 uBlurRadius;\n";
        ss << "varying vec2 vTexPos;\n";
        ss << "vec4 sampleColorAtRadius(float radius, vec2 texCoord, float sx, "
              "float sy) {\n";
        ss << "  vec2 coord = texCoord + vec2(radius * sx, radius * sy) * "
              "uBlurRadius;\n";
        ss << "  return texture2D(uTexture, coord);\n";
        ss << "}\n";
        ss << "void main(void) {\n";
        ss << "  float sy = 1.0;\n";
        ss << "  sy /= uTextureHeight;\n";
        ss << "  float sx = 1.0;\n";
        ss << "  sx /= uTextureWidth;\n";

        ss << "  vec4 total = sampleColorAtRadius(0., vTexPos, sx, sy) * "
           << gaussianKernel[0] << ";\n";
        for (int i = 1; i < GAUSSIAN_KERNEL_HALF_WIDTH; i++) {
            ss << "  total += sampleColorAtRadius(float("
               << i * GAUSSIAN_KERNEL_STEP << "), vTexPos, sx, sy) * "
               << gaussianKernel[i] << ";\n";
            ss << "  total += sampleColorAtRadius(float("
               << -i * GAUSSIAN_KERNEL_STEP << "), vTexPos, sx, sy) * "
               << gaussianKernel[i] << ";\n";
        }

        if (addColorAlign) {
            ss << "  total = total * uAlpha;\n";
#if defined(PORT_PIXEL_ORDER_BGRA)
            ss << "  gl_FragColor.r = total[2];\n";
            ss << "  gl_FragColor.g = total[1];\n";
            ss << "  gl_FragColor.b = total[0];\n";
            ss << "  gl_FragColor.a = total[3];\n";
#else
            ss << "  gl_FragColor.r = total[0];\n";
            ss << "  gl_FragColor.g = total[1];\n";
            ss << "  gl_FragColor.b = total[2];\n";
            ss << "  gl_FragColor.a = total[3];\n";
#endif
        } else {
            ss << "  gl_FragColor = total;\n";
        }

        ss << "}\n";
        return ss.str();
    }

    GLuint texFragmentBlurShaderW()
    {
        if (m_texFragmentBlurShaderW) {
            return m_texFragmentBlurShaderW;
        }
        m_texFragmentBlurShaderW = loadShader(
            GL_FRAGMENT_SHADER, generateBlurEffectFragmentShader(false).data());
        checkError();
        return m_texFragmentBlurShaderW;
    }

    GLuint texFragmentBlurShaderEGLImageExternalW()
    {
        if (m_texFragmentBlurShaderEGLImageExternalW) {
            return m_texFragmentBlurShaderEGLImageExternalW;
        }
        m_texFragmentBlurShaderEGLImageExternalW = loadShader(
            GL_FRAGMENT_SHADER, generateBlurEffectFragmentShader(true).data());
        checkError();
        return m_texFragmentBlurShaderEGLImageExternalW;
    }

    GLuint texFragmentBlurShaderH()
    {
        if (m_texFragmentBlurShaderH) {
            return m_texFragmentBlurShaderH;
        }
        m_texFragmentBlurShaderH =
            loadShader(GL_FRAGMENT_SHADER,
                       generateBlurEffectFragmentShader(false, true).data());
        checkError();
        return m_texFragmentBlurShaderH;
    }

    GLuint texBlurShaderProgramW()
    {
        if (m_texBlurShaderProgramW) {
            if (m_lastProgram != m_texBlurShaderProgramW) {
                m_lastProgram = m_texBlurShaderProgramW;
                glUseProgram(m_texBlurShaderProgramW);

                glBindBuffer(GL_ARRAY_BUFFER, m_texTexPosBuffer);
                glVertexAttribPointer(m_texBlurShaderProgramWTexPos, 2,
                                      GL_FLOAT, false, 0, 0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }
            return m_texBlurShaderProgramW;
        }
        m_texBlurShaderProgramW = glCreateProgram();

        glAttachShader(m_texBlurShaderProgramW, texVertexShader());
        glAttachShader(m_texBlurShaderProgramW, texFragmentBlurShaderW());
        glLinkProgram(m_texBlurShaderProgramW);
        checkError();

        m_lastProgram = m_texBlurShaderProgramW;
        glUseProgram(m_texBlurShaderProgramW);

        m_texBlurShaderProgramWPosition =
            glGetAttribLocation(m_texBlurShaderProgramW, "aPosition");
        m_texBlurShaderProgramWTexPos =
            glGetAttribLocation(m_texBlurShaderProgramW, "aTexPos");
        m_texBlurShaderProgramWTexture =
            glGetUniformLocation(m_texBlurShaderProgramW, "uTexture");
        m_texBlurShaderProgramWBlurRadius =
            glGetUniformLocation(m_texBlurShaderProgramW, "uBlurRadius");
        m_texBlurShaderProgramWTextureWidth =
            glGetUniformLocation(m_texBlurShaderProgramW, "uTextureWidth");
        m_texBlurShaderProgramWTextureHeight =
            glGetUniformLocation(m_texBlurShaderProgramW, "uTextureHeight");

        glUniform1i(m_texBlurShaderProgramWTexture, 0);
        glEnableVertexAttribArray(m_texBlurShaderProgramWPosition);

        glBindBuffer(GL_ARRAY_BUFFER, m_texTexPosBuffer);
        glEnableVertexAttribArray(m_texBlurShaderProgramWTexPos);
        glVertexAttribPointer(m_texBlurShaderProgramWTexPos, 2, GL_FLOAT, false,
                              0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        return m_texBlurShaderProgramW;
    }

    GLuint texBlurShaderProgramEGLImageExternalW()
    {
        if (m_texBlurShaderProgramEGLImageExternalW) {
            if (m_lastProgram != m_texBlurShaderProgramEGLImageExternalW) {
                m_lastProgram = m_texBlurShaderProgramEGLImageExternalW;
                glUseProgram(m_texBlurShaderProgramEGLImageExternalW);

                glBindBuffer(GL_ARRAY_BUFFER, m_texTexPosBuffer);
                glVertexAttribPointer(
                    m_texBlurShaderProgramEGLImageExternalWTexPos, 2, GL_FLOAT,
                    false, 0, 0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }
            return m_texBlurShaderProgramEGLImageExternalW;
        }
        m_texBlurShaderProgramEGLImageExternalW = glCreateProgram();

        glAttachShader(m_texBlurShaderProgramEGLImageExternalW,
                       texVertexShader());
        glAttachShader(m_texBlurShaderProgramEGLImageExternalW,
                       texFragmentBlurShaderEGLImageExternalW());
        glLinkProgram(m_texBlurShaderProgramEGLImageExternalW);
        checkError();

        m_lastProgram = m_texBlurShaderProgramEGLImageExternalW;
        glUseProgram(m_texBlurShaderProgramEGLImageExternalW);

        m_texBlurShaderProgramEGLImageExternalWPosition = glGetAttribLocation(
            m_texBlurShaderProgramEGLImageExternalW, "aPosition");
        m_texBlurShaderProgramEGLImageExternalWTexPos = glGetAttribLocation(
            m_texBlurShaderProgramEGLImageExternalW, "aTexPos");
        m_texBlurShaderProgramEGLImageExternalWTexture = glGetUniformLocation(
            m_texBlurShaderProgramEGLImageExternalW, "uTexture");
        m_texBlurShaderProgramEGLImageExternalWBlurRadius =
            glGetUniformLocation(m_texBlurShaderProgramEGLImageExternalW,
                                 "uBlurRadius");
        m_texBlurShaderProgramEGLImageExternalWTextureWidth =
            glGetUniformLocation(m_texBlurShaderProgramEGLImageExternalW,
                                 "uTextureWidth");
        m_texBlurShaderProgramEGLImageExternalWTextureHeight =
            glGetUniformLocation(m_texBlurShaderProgramEGLImageExternalW,
                                 "uTextureHeight");

        glUniform1i(m_texBlurShaderProgramEGLImageExternalWTexture, 0);
        glEnableVertexAttribArray(
            m_texBlurShaderProgramEGLImageExternalWPosition);

        glBindBuffer(GL_ARRAY_BUFFER, m_texTexPosBuffer);
        glEnableVertexAttribArray(
            m_texBlurShaderProgramEGLImageExternalWTexPos);
        glVertexAttribPointer(m_texBlurShaderProgramEGLImageExternalWTexPos, 2,
                              GL_FLOAT, false, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        return m_texBlurShaderProgramEGLImageExternalW;
    }

    GLuint texBlurShaderProgramH()
    {
        if (m_texBlurShaderProgramH) {
            if (m_lastProgram != m_texBlurShaderProgramH) {
                m_lastProgram = m_texBlurShaderProgramH;
                glUseProgram(m_texBlurShaderProgramH);

                glBindBuffer(GL_ARRAY_BUFFER, m_texTexPosBuffer);
                glVertexAttribPointer(m_texBlurShaderProgramHTexPos, 2,
                                      GL_FLOAT, false, 0, 0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }
            return m_texBlurShaderProgramH;
        }
        m_texBlurShaderProgramH = glCreateProgram();

        glAttachShader(m_texBlurShaderProgramH, texVertexShader());
        glAttachShader(m_texBlurShaderProgramH, texFragmentBlurShaderH());
        glLinkProgram(m_texBlurShaderProgramH);
        checkError();

        m_lastProgram = m_texBlurShaderProgramH;
        glUseProgram(m_texBlurShaderProgramH);

        m_texBlurShaderProgramHPosition =
            glGetAttribLocation(m_texBlurShaderProgramH, "aPosition");
        m_texBlurShaderProgramHTexPos =
            glGetAttribLocation(m_texBlurShaderProgramH, "aTexPos");
        m_texBlurShaderProgramHTexture =
            glGetUniformLocation(m_texBlurShaderProgramH, "uTexture");
        m_texBlurShaderProgramHBlurRadius =
            glGetUniformLocation(m_texBlurShaderProgramH, "uBlurRadius");
        m_texBlurShaderProgramHTextureWidth =
            glGetUniformLocation(m_texBlurShaderProgramH, "uTextureWidth");
        m_texBlurShaderProgramHTextureHeight =
            glGetUniformLocation(m_texBlurShaderProgramH, "uTextureHeight");
        m_texBlurShaderProgramHAlpha =
            glGetUniformLocation(m_texBlurShaderProgramH, "uAlpha");

        glUniform1i(m_texBlurShaderProgramHTexture, 0);
        glEnableVertexAttribArray(m_texBlurShaderProgramHPosition);

        glBindBuffer(GL_ARRAY_BUFFER, m_texTexPosBuffer);
        glEnableVertexAttribArray(m_texBlurShaderProgramHTexPos);
        glVertexAttribPointer(m_texBlurShaderProgramHTexPos, 2, GL_FLOAT, false,
                              0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glUniform1f(m_texBlurShaderProgramHAlpha, 1);

        return m_texBlurShaderProgramH;
    }
};

void Compositor::destroyCompositorContext(PlatformWindow* wnd,
                                          CompositorContext* ctx)
{
    if (ctx) {
        glUseProgram(0);

        if (ctx->m_texBlurShaderProgramW) {
            glDetachShader(ctx->m_texBlurShaderProgramW,
                           ctx->m_texVertexShader);
            glDetachShader(ctx->m_texBlurShaderProgramW,
                           ctx->m_texFragmentBlurShaderW);
            glDeleteProgram(ctx->m_texBlurShaderProgramW);
        }

        if (ctx->m_texBlurShaderProgramEGLImageExternalW) {
            glDetachShader(ctx->m_texBlurShaderProgramEGLImageExternalW,
                           ctx->m_texVertexShader);
            glDetachShader(ctx->m_texBlurShaderProgramEGLImageExternalW,
                           ctx->m_texFragmentBlurShaderEGLImageExternalW);
            glDeleteProgram(ctx->m_texBlurShaderProgramEGLImageExternalW);
        }

        if (ctx->m_texBlurShaderProgramH) {
            glDetachShader(ctx->m_texBlurShaderProgramH,
                           ctx->m_texVertexShader);
            glDetachShader(ctx->m_texBlurShaderProgramH,
                           ctx->m_texFragmentBlurShaderH);
            glDeleteProgram(ctx->m_texBlurShaderProgramH);
        }

        if (ctx->m_texFragmentBlurShaderW) {
            glDeleteShader(ctx->m_texFragmentBlurShaderW);
        }

        if (ctx->m_texFragmentBlurShaderH) {
            glDeleteShader(ctx->m_texFragmentBlurShaderH);
        }

        if (ctx->m_texFragmentBlurShaderEGLImageExternalW) {
            glDeleteShader(ctx->m_texFragmentBlurShaderEGLImageExternalW);
        }

        if (ctx->m_rectShaderProgram) {
            glDetachShader(ctx->m_rectShaderProgram, ctx->m_rectVertexShader);
            glDetachShader(ctx->m_rectShaderProgram, ctx->m_rectFragmentShader);
            glDeleteProgram(ctx->m_rectShaderProgram);
            glDeleteShader(ctx->m_rectVertexShader);
            glDeleteShader(ctx->m_rectFragmentShader);
        }

        if (ctx->m_texShaderProgramEGLImageExternal) {
            glDetachShader(ctx->m_texShaderProgramEGLImageExternal,
                           ctx->m_texVertexShader);
            glDetachShader(ctx->m_texShaderProgramEGLImageExternal,
                           ctx->m_texFragmentShaderEGLImageExternal);
            glDeleteProgram(ctx->m_texShaderProgramEGLImageExternal);
            glDeleteShader(ctx->m_texFragmentShaderEGLImageExternal);
        }

        if (ctx->m_texShaderProgramWithOriginalColor) {
            glDetachShader(ctx->m_texShaderProgramWithOriginalColor,
                           ctx->m_texVertexShader);
            glDetachShader(ctx->m_texShaderProgramWithOriginalColor,
                           ctx->m_texFragmentShaderWithOriginalColor);
            glDeleteProgram(ctx->m_texShaderProgramWithOriginalColor);
        }

        if (ctx->m_texShaderProgram) {
            glDetachShader(ctx->m_texShaderProgram, ctx->m_texVertexShader);
            glDetachShader(ctx->m_texShaderProgram, ctx->m_texFragmentShader);
            glDeleteProgram(ctx->m_texShaderProgram);
        }

        if (ctx->m_texVertexShader) {
            glDeleteShader(ctx->m_texVertexShader);
        }

        if (ctx->m_texFragmentShader) {
            glDeleteShader(ctx->m_texFragmentShader);
        }

        glDeleteBuffers(1, &ctx->m_texTexPosBuffer);

        delete ctx;
    }
}

CompositorContext* Compositor::initCompositorContext(PlatformWindow* wnd)
{
    wnd->glMakeCurrent();

    if (g_needsCheckCompatibility) {
        GLint siz;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &siz);
        checkError();
        g_maxTextureSize = siz;

        g_textureTileSize = CanvasSurface::g_canvasSurfaceTileSize;

        if (g_textureTileSize > g_maxTextureSize) {
            g_textureTileSize = g_maxTextureSize;
        }

        bool isOpenGLES3 = true;
        int major;
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        if (glGetError()) {
            isOpenGLES3 = false;
            major = 2;
        }

        if (isOpenGLES3) {
            STARFISH_LOG_INFO("GL_MAJOR_VERSION %d\n", (int)major);
        } else {
            STARFISH_LOG_INFO("GL_MAJOR_VERSION 2\n");
        }

        if (major >= 3) {
            g_isSupportPixelStoreiUnpackingOfPixelDataFromMemory = true;
        }

        const char* ex = (const char*)glGetString(GL_EXTENSIONS);

        if (ex) {
            STARFISH_LOG_INFO("GL_EXTENSIONS -> %s\n", ex);
            g_isSupportExtensionEGLImageExternal =
                strstr(ex, "GL_OES_EGL_image_external") != nullptr;
        } else {
            STARFISH_LOG_INFO("GL_EXTENSIONS -> returns null...\n");
        }

#if (!defined(STARFISH_TIZEN) && !defined(STARFISH_ANDROID)) || \
    (defined(STARFISH_ANDROID) && !defined(USE_EGLIMAGE_EXT_ANDROID))
        g_isSupportExtensionEGLImageExternal = false;
#endif

#if defined(STARFISH_TIZEN)
        if (g_isSupportPixelStoreiUnpackingOfPixelDataFromMemory) {
            g_shouldUseEGLImageOnPlainSurface = true;
        }
#endif
        if (g_isSupportExtensionEGLImageExternal) {
            STARFISH_LOG_INFO("use EGLImageExternal!\n");
        }

#if defined(STARFISH_TIZEN) && \
    !defined(PORT_WEBVIEW_BRIDGE_EFL) // STARFISH_TIZEN without
                                      // PORT_WEBVIEW_BRIDGE_EFL
        g_eglCreateImageKHRProc = reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(
            eglGetProcAddress("eglCreateImageKHR"));
        g_eglDestroyImageKHRProc = reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(
            eglGetProcAddress("eglDestroyImageKHR"));
        g_glEGLImageTargetTexture2DOESProc =
            reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(
                eglGetProcAddress("glEGLImageTargetTexture2DOES"));
#endif

#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_TIZEN_VERSION_5_0) && \
    defined(PORT_WEBVIEW_BRIDGE_EFL)
        g_useStencilBufferOnFBO = true;
#endif
        g_needsCheckCompatibility = false;
        checkError();
    }

    CompositorContext* compositorContext = new CompositorContext;

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE0);

    glGenBuffers(1, &compositorContext->m_texTexPosBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, compositorContext->m_texTexPosBuffer);
    float texPos[] = { 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f };
    glBufferData(GL_ARRAY_BUFFER, sizeof(texPos), texPos, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return compositorContext;
}

class CanvasSurfaceGL : public CanvasSurface {
public:
    CanvasSurfaceGL(PlatformWindow* wnd, size_t w, size_t h,
                    CanvasSurfaceFlag flag)
    {
        m_window = (PlatformWindow*)wnd;
        m_width = w;
        m_height = h;
        m_bufferWidth = m_width = -1;
        m_bufferHeight = m_height = -1;
        m_buffer = nullptr;
        m_isEGLImageExternal = false;
        m_isEGLBufferOwner = false;
        m_isEGLImageNeedsFlipRGB = false;
        m_flag = flag;
        m_wTextureCount = 0;
        m_hTextureCount = 0;
        m_textureTileSize = 0;
#if defined(STARFISH_TIZEN)
        m_tbmSurface = nullptr;
        m_eglImage = nullptr;
#elif defined(STARFISH_ANDROID) && defined(USE_EGLIMAGE_EXT_ANDROID)
        m_aHardwareBuffer = nullptr;
        m_eglImage = nullptr;
#endif

        attachNativeBuffer(w, h, flag);
        checkError();
        GC_REGISTER_FINALIZER_NO_ORDER(this,
                                       [](void* obj, void* cd) {
                                           CanvasSurfaceGL* s =
                                               (CanvasSurfaceGL*)obj;
                                           s->detachNativeBuffer();
                                       },
                                       NULL, NULL, NULL);
    }

#if defined(STARFISH_ENABLE_TEST) && defined(PORT_CANVAS_BACKEND_CAIRO)
    virtual void dump(const char* path)
    {
        STARFISH_ASSERT(m_buffer);
        auto surface = cairo_image_surface_create_for_data(
            m_buffer, CAIRO_FORMAT_ARGB32, bufferWidth(), bufferHeight(),
            bufferStride());
        cairo_surface_write_to_png(surface, path);
        cairo_surface_destroy(surface);
    }
#endif

    virtual void detachNativeBuffer() override
    {
        if (m_textureFragments.size()) {
            if (m_isEGLImageExternal && !m_isEGLBufferOwner) {
            } else {
                g_totalAllocatedCanvasSurfaceSize -=
                    m_bufferWidth * m_bufferHeight * sizeof(uint32_t);
            }

            bool ret = m_window->glMakeCurrent();
            if (m_isEGLImageExternal) {
#if defined(STARFISH_TIZEN) && !defined(PORT_WEBVIEW_BRIDGE_EFL)
                EGLDisplay display = eglGetCurrentDisplay();
                g_eglDestroyImageKHRProc(display, m_eglImage);
                m_eglImage = nullptr;
                if (m_isEGLBufferOwner) {
                    LongTaskFinder t("tbm_surface_destroy", 1);
                    tbm_surface_destroy(m_tbmSurface);
                }
                m_tbmSurface = nullptr;
#elif defined(STARFISH_TIZEN) && defined(PORT_WEBVIEW_BRIDGE_EFL)
                g_evasGLAPI->evasglDestroyImage(m_eglImage);
                m_eglImage = nullptr;
                if (m_isEGLBufferOwner) {
                    LongTaskFinder t("tbm_surface_destroy", 1);
                    tbm_surface_destroy(m_tbmSurface);
                }
                m_tbmSurface = nullptr;
#elif defined(STARFISH_ANDROID) && defined(USE_EGLIMAGE_EXT_ANDROID)
                EGLDisplay display = eglGetCurrentDisplay();
                eglDestroyImageKHR(display, m_eglImage);

                m_eglImage = nullptr;
                if (m_isEGLBufferOwner) {
                    AHardwareBuffer_release(m_aHardwareBuffer);
                }
                m_aHardwareBuffer = nullptr;
#endif
            }

            if (m_isEGLImageExternal) {
            } else {
                free(m_buffer);
            }

            if (ret) {
                for (size_t i = 0; i < m_textureFragments.size(); i++) {
                    GLuint id = m_textureFragments[i].textureID;
                    glDeleteTextures(1, &id);
                }
            }

            m_textureFragments.clear();

            m_buffer = nullptr;
            m_width = 0;
            m_height = 0;
            m_bufferStride = m_bufferWidth = m_width = 0;
            m_bufferHeight = m_height = 0;

            m_isEGLImageNeedsFlipRGB = m_isEGLBufferOwner =
                m_isEGLImageExternal = false;
        }
    }

    bool attachNativeBuffer(size_t w, size_t h, CanvasSurfaceFlag flag) override
    {
        if (m_width != w || m_height != h) {
            detachNativeBuffer();
            m_width = w;
            m_height = h;
            m_flag = flag;

            float windowDevicePixelRatio =
                m_window->webView()->screenInfo().devicePixelRatio;

            if (!(m_flag & CanvasSurfaceFlag::CanvasElement)) {
                m_bufferWidth =
                    std::max((size_t)1, (size_t)(w * windowDevicePixelRatio));
                m_bufferHeight =
                    std::max((size_t)1, (size_t)(h * windowDevicePixelRatio));
            } else {
                m_bufferWidth = w;
                m_bufferHeight = h;
            }

            if (!g_shouldUseEGLImageOnPlainSurface &&
                g_isSupportExtensionEGLImageExternal &&
                m_bufferWidth <= g_maxTextureSize &&
                m_bufferHeight <= g_maxTextureSize) {
                m_isEGLBufferOwner = m_isEGLImageExternal = true;
                m_isEGLImageNeedsFlipRGB = false;
#if defined(STARFISH_TIZEN)
                tbm_surface_info_s surfaceInfo;
                {
                    LongTaskFinder t("tbm_surface_create", 1);
                    m_tbmSurface = tbm_surface_create(
                        m_bufferWidth, m_bufferHeight, TBM_FORMAT_ABGR8888);
                    tbm_surface_get_info(m_tbmSurface, &surfaceInfo);
                }
                STARFISH_RELEASE_ASSERT(surfaceInfo.num_planes == 1);
                m_bufferStride = surfaceInfo.planes[0].stride;
                m_buffer = nullptr;
#elif defined(STARFISH_ANDROID) && defined(USE_EGLIMAGE_EXT_ANDROID)
                AHardwareBuffer_Desc desc{
                    m_bufferWidth, m_bufferHeight, 1,
                    AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
                    AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN |
                        AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN |
                        AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT |
                        AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE,
                    0, 0, 0
                };

                AHardwareBuffer_allocate(&desc, &m_aHardwareBuffer);
                STARFISH_RELEASE_ASSERT(m_aHardwareBuffer);
                AHardwareBuffer_Desc outDesc;
                AHardwareBuffer_describe(m_aHardwareBuffer, &outDesc);
                m_bufferStride = outDesc.stride * 4;
                m_buffer = nullptr;
#endif
            } else {
                m_isEGLImageExternal = false;
                m_isEGLBufferOwner = false;
                m_bufferStride = m_bufferWidth * sizeof(uint32_t);
                m_buffer = nullptr;
            }

            g_totalAllocatedCanvasSurfaceSize +=
                m_bufferWidth * m_bufferHeight * sizeof(uint32_t);

            ensureGenerateTexture();
            return true;
        }
        return false;
    }

    void ensureGenerateTexture()
    {
        m_window->glMakeCurrent();

        STARFISH_RELEASE_ASSERT(m_textureFragments.size() == 0);
        if (m_isEGLImageExternal) {
#if defined(STARFISH_TIZEN) || defined(STARFISH_ANDROID)
            CanvasSurfaceTextureInfo::CanvasSurfaceTextureInfoFragment fragment;
#if defined(STARFISH_TIZEN) && !defined(PORT_WEBVIEW_BRIDGE_EFL)
            {
                STARFISH_RELEASE_ASSERT(m_tbmSurface);
                STARFISH_RELEASE_ASSERT(m_eglImage == nullptr);
                EGLint attribs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
                                     EGL_NONE };
                EGLDisplay display = eglGetCurrentDisplay();
                m_eglImage = g_eglCreateImageKHRProc(
                    display, EGL_NO_CONTEXT, EGL_NATIVE_SURFACE_TIZEN,
                    (void*)(intptr_t)m_tbmSurface, attribs);
                checkError();
            }
#elif defined(STARFISH_TIZEN) && defined(PORT_WEBVIEW_BRIDGE_EFL)
            {
                STARFISH_RELEASE_ASSERT(m_tbmSurface);
                STARFISH_RELEASE_ASSERT(m_eglImage == nullptr);
                int eglImgAttr[] = { EVAS_GL_IMAGE_PRESERVED, GL_TRUE, 0 };
                m_eglImage = g_evasGLAPI->evasglCreateImage(
                    EVAS_GL_NATIVE_SURFACE_TIZEN, (void*)(intptr_t)m_tbmSurface,
                    eglImgAttr);
                checkError();
            }
#elif defined(STARFISH_ANDROID) && defined(USE_EGLIMAGE_EXT_ANDROID)
            {
                STARFISH_RELEASE_ASSERT(m_aHardwareBuffer);
                STARFISH_RELEASE_ASSERT(m_eglImage == nullptr);

                EGLClientBuffer clientBuffer =
                    eglGetNativeClientBufferANDROID(m_aHardwareBuffer);
                if (UNLIKELY(!clientBuffer)) {
                    logEglError("eglGetNativeClientBufferANDROID");
                    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
                }
                EGLint attribs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
                                     EGL_NONE };
                EGLDisplay display = eglGetCurrentDisplay();
                // eglCreateImageKHR will add a ref to the AHardwareBuffer
                m_eglImage = eglCreateImageKHR(display, EGL_NO_CONTEXT,
                                               EGL_NATIVE_BUFFER_ANDROID,
                                               clientBuffer, attribs);
                if (UNLIKELY(!m_eglImage)) {
                    logEglError("eglCreateImageKHR");
                    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
                }
            }
#endif

#if defined(USE_EGLIMAGE_EXT_ANDROID) || !defined(STARFISH_ANDROID)
            if (nullptr == m_eglImage) {
                STARFISH_LOG_INFO("result of eglCreateImageKHR is fail\n");
            }
#endif
            {
                GLuint textureID;
                glGenTextures(1, &textureID);

                glBindTexture(GL_TEXTURE_EXTERNAL_OES, textureID);
                checkError();

                glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER,
                                GL_LINEAR);
                glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER,
                                GL_LINEAR);

                glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S,
                                GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T,
                                GL_CLAMP_TO_EDGE);

                checkError();
#if defined(STARFISH_TIZEN) && !defined(PORT_WEBVIEW_BRIDGE_EFL)
                g_glEGLImageTargetTexture2DOESProc(GL_TEXTURE_EXTERNAL_OES,
                                                   m_eglImage);
#elif defined(STARFISH_TIZEN) && defined(PORT_WEBVIEW_BRIDGE_EFL)
                g_evasGLAPI->glEvasGLImageTargetTexture2DOES(
                    GL_TEXTURE_EXTERNAL_OES, m_eglImage);
#elif defined(STARFISH_ANDROID) && defined(USE_EGLIMAGE_EXT_ANDROID)
                glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES,
                                             m_eglImage);
#endif
                checkError();

                glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
                checkError();

                fragment.textureID = textureID;
                fragment.srcX = 0;
                fragment.srcY = 0;
                fragment.srcWidth = 1;
                fragment.srcHeight = 1;

                m_textureFragments.push_back(fragment);
            }
#endif
            return;
        }

        m_textureTileSize = g_textureTileSize;
        m_wTextureCount = ceil((float)m_bufferWidth / m_textureTileSize);
        m_hTextureCount = ceil((float)m_bufferHeight / m_textureTileSize);

        if (m_flag & CanvasSurfaceFlag::ElementHasFilterEffect) {
            m_wTextureCount = m_hTextureCount = 1;
        }

        size_t coveredRowsCount = 0;
        for (size_t y = 0; y < m_hTextureCount; y++) {
            size_t coveredColsCount = 0;
            for (size_t x = 0; x < m_wTextureCount; x++) {
                GLuint textureID;

                size_t texureDataX = coveredColsCount;
                size_t texureDataY = coveredRowsCount;
                size_t texureDataWidth =
                    std::min((size_t)m_textureTileSize,
                             m_bufferWidth - coveredColsCount);
                size_t texureDataHeight =
                    std::min((size_t)m_textureTileSize,
                             m_bufferHeight - coveredRowsCount);

                if (m_flag & CanvasSurfaceFlag::ElementHasFilterEffect) {
                    texureDataWidth = m_bufferWidth;
                    texureDataHeight = m_bufferHeight;
                }

                glGenTextures(1, &textureID);
                glBindTexture(GL_TEXTURE_2D, textureID);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                checkError();

                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texureDataWidth,
                             texureDataHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                             nullptr);
                checkError();

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                GL_LINEAR);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                                GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                                GL_CLAMP_TO_EDGE);
                checkError();

                glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
                checkError();

                CanvasSurfaceTextureInfo::CanvasSurfaceTextureInfoFragment
                    fragment;
                fragment.textureID = textureID;
                fragment.srcX = texureDataX / (float)m_bufferWidth;
                fragment.srcY = texureDataY / (float)m_bufferHeight;
                fragment.srcWidth = texureDataWidth / (float)m_bufferWidth;
                fragment.srcHeight = texureDataHeight / (float)m_bufferHeight;

                m_textureFragments.push_back(fragment);
                coveredColsCount += m_textureTileSize;
            }

            coveredRowsCount += m_textureTileSize;
        }
    }

    virtual MappedNativeBuffer mapBuffer(size_t bufferX, size_t bufferY,
                                         size_t bufferWidth,
                                         size_t bufferHeight) override
    {
        if (m_isEGLImageExternal) {
            if (!m_buffer) {
#if defined(STARFISH_TIZEN)
                tbm_surface_info_s surfaceInfo;
                {
                    LongTaskFinder t("tbm_surface_map", 1);
                    tbm_surface_map(m_tbmSurface, TBM_SURF_OPTION_WRITE,
                                    &surfaceInfo);
                }
                STARFISH_RELEASE_ASSERT(surfaceInfo.num_planes == 1);
                STARFISH_RELEASE_ASSERT(surfaceInfo.planes[0].stride ==
                                        m_bufferStride);
                m_buffer = surfaceInfo.planes[0].ptr;
#elif defined(STARFISH_ANDROID) && defined(USE_EGLIMAGE_EXT_ANDROID)
                AHardwareBuffer_lock(m_aHardwareBuffer,
                                     AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN |
                                         AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN,
                                     -1, NULL, (void**)&m_buffer);
#endif
            }
        } else {
            if (!m_buffer) {
                m_buffer =
                    (unsigned char*)malloc(m_bufferStride * m_bufferHeight);
            }
        }

        CanvasSurface::MappedNativeBuffer b;
        b.m_bufferAddress = m_buffer;
        b.m_mappedBufferX = 0;
        b.m_mappedBufferY = 0;
        b.m_mappedBufferWidth = m_bufferWidth;
        b.m_mappedBufferHeight = m_bufferHeight;
        b.m_mappedBufferStride = m_bufferStride;
        return b;
    }

    virtual size_t width() override
    {
        return m_width;
    }

    virtual size_t height() override
    {
        return m_height;
    }

    virtual size_t bufferWidth() override
    {
        return m_bufferWidth;
    }

    virtual size_t bufferHeight() override
    {
        return m_bufferHeight;
    }

    virtual size_t bufferStride() override
    {
        return m_bufferStride;
    }

    size_t wTextureCount()
    {
        return m_wTextureCount;
    }

    size_t hTextureCount()
    {
        return m_hTextureCount;
    }

    size_t textureTileSize()
    {
        return m_textureTileSize;
    }
    virtual void unmapBufferAndNotifyUpdatedRegion(size_t dirtyX, size_t dirtyY,
                                                   size_t dirtyWidth,
                                                   size_t dirtyHeight) override
    {
        STARFISH_ASSERT(m_wTextureCount != 0);
        STARFISH_ASSERT(m_hTextureCount != 0);
        STARFISH_ASSERT(m_textureTileSize != 0);
        if (m_textureFragments.size() == 0) {
            return;
        }

        if (m_isEGLImageExternal) {
#if defined(STARFISH_TIZEN)
            {
                LongTaskFinder t("tbm_surface_unmap", 1);
                tbm_surface_unmap(m_tbmSurface);
            }
#elif defined(STARFISH_ANDROID) && defined(USE_EGLIMAGE_EXT_ANDROID)
            int32_t fence = -1;
            AHardwareBuffer_unlock(m_aHardwareBuffer, &fence);
#endif
            m_buffer = nullptr;
            return;
        }

        STARFISH_RELEASE_ASSERT(m_buffer);

        if (dirtyWidth && dirtyHeight) {
            m_window->glMakeCurrent();
            size_t fragmentIndex = 0;

            Unit::Rect dRect(dirtyX, dirtyY, dirtyWidth, dirtyHeight);

            size_t coveredRowsCount = 0;
            for (size_t y = 0; y < m_hTextureCount; y++) {
                size_t coveredColsCount = 0;
                for (size_t x = 0; x < m_wTextureCount; x++) {
                    GLuint textureID;

                    size_t textureDataX = coveredColsCount;
                    size_t textureDataY = coveredRowsCount;
                    size_t textureDataWidth =
                        std::min((size_t)m_textureTileSize,
                                 m_bufferWidth - coveredColsCount);
                    size_t textureDataHeight =
                        std::min((size_t)m_textureTileSize,
                                 m_bufferHeight - coveredRowsCount);

                    if (m_flag & CanvasSurfaceFlag::ElementHasFilterEffect) {
                        textureDataWidth = m_bufferWidth;
                        textureDataHeight = m_bufferHeight;
                    }

                    CanvasSurfaceTextureInfo::CanvasSurfaceTextureInfoFragment&
                        fragment = m_textureFragments[fragmentIndex];

                    Unit::Rect tRect(textureDataX, textureDataY,
                                     textureDataWidth, textureDataHeight);

                    if (tRect.intersects(dRect)) {
                        auto left = std::max(tRect.x(), dRect.x());
                        auto right = std::min(tRect.maxX(), dRect.maxX());
                        auto bottom = std::min(tRect.maxY(), dRect.maxY());
                        auto top = std::max(tRect.y(), dRect.y());

                        left -= textureDataX;
                        right -= textureDataX;
                        bottom -= textureDataY;
                        top -= textureDataY;

                        size_t xx = left;
                        size_t xxEnd = right;
                        size_t yy = top;
                        size_t yyEnd = bottom;

                        if (((xxEnd - xx) > 0) && ((yyEnd - yy) > 0)) {
                            GLuint tid =
                                (GLuint)m_textureFragments[fragmentIndex]
                                    .textureID;
                            LongTaskFinder t("update texture tile..", 1);
                            auto bData = m_buffer;
                            auto bStride = bufferStride();

                            glBindTexture(GL_TEXTURE_2D, tid);
                            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                            checkError();

                            if (g_isSupportPixelStoreiUnpackingOfPixelDataFromMemory) {
                                glPixelStorei(GL_UNPACK_ROW_LENGTH,
                                              bufferWidth());
                                glPixelStorei(GL_UNPACK_SKIP_PIXELS, xx);
                                glPixelStorei(GL_UNPACK_SKIP_ROWS, yy);

                                auto data = bData;
                                data += textureDataY * bStride;
                                data += textureDataX * 4;
                                glTexSubImage2D(GL_TEXTURE_2D, 0, xx, yy,
                                                xxEnd - xx, yyEnd - yy, GL_RGBA,
                                                GL_UNSIGNED_BYTE, data);

                                glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
                                glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
                                glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
                            } else {
                                for (; yy < yyEnd; yy++) {
                                    auto data = bData;
                                    data += ((yy + textureDataY) * bStride);
                                    data += ((textureDataX + xx) * 4);
                                    glTexSubImage2D(GL_TEXTURE_2D, 0, xx, yy,
                                                    xxEnd - xx, 1, GL_RGBA,
                                                    GL_UNSIGNED_BYTE, data);
                                    checkError();
                                }
                            }

                            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
                            checkError();
                        }
                    }

                    fragmentIndex++;
                    coveredColsCount += m_textureTileSize;
                }

                coveredRowsCount += m_textureTileSize;
            }
        }

        if (!(m_flag & CanvasSurface::CanvasElement)) {
            free(m_buffer);
            m_buffer = nullptr;
        }
    }

    virtual CanvasSurfaceTextureInfo textureInfo() override
    {
        CanvasSurfaceTextureInfo info;
        info.fragments = std::vector<
            CanvasSurfaceTextureInfo::CanvasSurfaceTextureInfoFragment>(
            m_textureFragments.data(),
            m_textureFragments.data() + m_textureFragments.size());
        return info;
    }

    virtual void attachPlatformExternalBuffer(void* buffer) override
    {
        detachNativeBuffer();

        m_isEGLBufferOwner = false;
        m_isEGLImageExternal = true;
        m_isEGLImageNeedsFlipRGB = false;

        size_t w = 0, h = 0;
#if defined(STARFISH_TIZEN)
        m_tbmSurface = (tbm_surface_h)buffer;
        m_buffer = nullptr;
        tbm_surface_info_s surfaceInfo;
        tbm_surface_get_info(m_tbmSurface, &surfaceInfo);
        w = surfaceInfo.width;
        h = surfaceInfo.height;
        m_bufferStride = surfaceInfo.planes[0].stride;
        switch (surfaceInfo.format) {
        case TBM_FORMAT_ABGR8888:
        case TBM_FORMAT_BGR565:
        case TBM_FORMAT_BGR888:
            m_isEGLImageNeedsFlipRGB = false;
            break;
        default:
            m_isEGLImageNeedsFlipRGB = true;
            break;
        }
#elif defined(STARFISH_ANDROID) && defined(USE_EGLIMAGE_EXT_ANDROID)
        m_aHardwareBuffer = (AHardwareBuffer*)buffer;
        AHardwareBuffer_Desc outDesc;
        AHardwareBuffer_describe(m_aHardwareBuffer, &outDesc);
        m_bufferStride = outDesc.stride * 4;
        w = outDesc.width;
        h = outDesc.height;
        m_buffer = nullptr;
#endif

        m_width = w;
        m_height = h;

        float windowDevicePixelRatio =
            m_window->webView()->screenInfo().devicePixelRatio;

        m_bufferWidth =
            std::max((size_t)1, (size_t)(w * windowDevicePixelRatio));
        m_bufferHeight =
            std::max((size_t)1, (size_t)(h * windowDevicePixelRatio));

        ensureGenerateTexture();
    }

protected:
    friend class CompositorImplGL;
    PlatformWindow* m_window;
    unsigned char* m_buffer;
    size_t m_width;
    size_t m_height;
    size_t m_bufferWidth;
    size_t m_bufferHeight;
    size_t m_bufferStride;
    size_t m_wTextureCount;
    size_t m_hTextureCount;
    size_t m_textureTileSize;
    GCAtomicVector<CanvasSurfaceTextureInfo::CanvasSurfaceTextureInfoFragment>
        m_textureFragments;

    bool m_isEGLImageExternal;
    bool m_isEGLBufferOwner;
    bool m_isEGLImageNeedsFlipRGB;
    CanvasSurfaceFlag m_flag;
#if defined(STARFISH_TIZEN) && defined(PORT_WEBVIEW_BRIDGE_EFL)
    tbm_surface_h m_tbmSurface;
    EvasGLImage m_eglImage;
#elif defined(STARFISH_TIZEN)
    tbm_surface_h m_tbmSurface;
    EGLImageKHR m_eglImage;
#elif defined(STARFISH_ANDROID) && defined(USE_EGLIMAGE_EXT_ANDROID)
    AHardwareBuffer* m_aHardwareBuffer;
    EGLImageKHR m_eglImage;
#endif
};

CanvasSurface* CanvasSurface::create(PlatformWindow* wnd, size_t w, size_t h,
                                     CanvasSurfaceFlag flag)
{
    return new CanvasSurfaceGL(wnd, w, h, flag);
}

struct CompositorImplGLState {
    bool matrixStaysInRect;
    bool clipPathsWasChanged;
    bool clipPathsAreSimple; // there are only rect clip
    SkMatrix matrix;
    float opacity;
    float blurRadius;
    Unit::Color color;
    std::shared_ptr<ClipperLib::Paths> clipPaths;
};

class CompositorImplGL : public Compositor {
public:
    void applyDevicePixelRatio()
    {
        m_state.back().matrix.preScale(
            m_webView->screenInfo().devicePixelRatio,
            m_webView->screenInfo().devicePixelRatio);
    }

    void setViewport()
    {
        size_t w = m_webView->platformWindow()->width();
        size_t h = m_webView->platformWindow()->height();
        glViewport(0, 0, w, h);
    }

    void scissor(float x, float y, float width, float height)
    {
        glScissor(x,
                  (float)m_webView->platformWindow()->height() - (y + height),
                  width, height);
    }

    void mapPointsToScreen(float& x, float& y)
    {
        SkPoint pt;
        pt = SkPoint::Make(x, y);
        auto& lastState = m_state.back();
        lastState.matrix.mapPoints(&pt, 1);
        m_screenMatrix.mapPoints(&pt, 1);
        x = pt.x();
        y = pt.y();
    }

    void mapPointsToLogicalScreen(float& x, float& y)
    {
        SkPoint pt;
        pt = SkPoint::Make(x, y);
        auto& lastState = m_state.back();
        lastState.matrix.mapPoints(&pt, 1);
        x = pt.x();
        y = pt.y();
    }

    void mapLogicalScreenPointsToScreen(float& x, float& y)
    {
        SkPoint pt;
        pt = SkPoint::Make(x, y);
        auto& lastState = m_state.back();
        m_screenMatrix.mapPoints(&pt, 1);
        x = pt.x();
        y = pt.y();
    }

    SkMatrix computeScreenMatrix()
    {
        SkMatrix m = SkMatrix::I();
#if defined(PORT_WEBVIEW_BRIDGE_EFL)
        size_t w = m_webView->platformWindow()->width();
        size_t h = m_webView->platformWindow()->height();
        int deg = evas_gl_rotation_get(g_evasGL);
        if (deg % 180 == 90) {
            float tx = w / 2.f;
            float ty = h / 2.f;

            m.preTranslate(tx, ty);

            SkMatrix t = SkMatrix::I();
            t.preRotate(360 - deg);
            t.preScale(h / (float)w, w / (float)h);
            m.preConcat(t);

            m.preTranslate(-tx, -ty);
        }
#endif
        return m;
    }

    CompositorImplGL(WebView* webView, CompositorContext* compositorContext)
    {
        LongTaskFinder t("CompositorImplGL::CompositorImplGL", 1);
        webView->platformWindow()->glMakeCurrent();

        m_seenFBOUsage = false;
        m_webView = webView;
        m_compositorContext = compositorContext;
        m_screenMatrix = computeScreenMatrix();

        setViewport();

        m_state.reserve(32);
        m_state.push_back(CompositorImplGLState());
        auto& lastState = m_state.back();
        lastState.clipPathsWasChanged = false;
        lastState.matrixStaysInRect = true;
        lastState.clipPathsAreSimple = false;
        lastState.matrix = SkMatrix::I();
        lastState.opacity = 1;
        lastState.blurRadius = 0;
        lastState.clipPaths.reset(new ClipperLib::Paths());

        clip(Unit::Rect(0, 0, m_webView->platformWindow()->width(),
                        m_webView->platformWindow()->height()));

        applyDevicePixelRatio();
    }

    ~CompositorImplGL()
    {
        restore();
        STARFISH_ASSERT(m_state.size() == 0);
        STARFISH_ASSERT(m_fboState.size() == 0);

        glBindTexture(GL_TEXTURE_2D, 0);
        if (g_isSupportExtensionEGLImageExternal) {
            glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
        }

        glUseProgram(0);
        m_compositorContext->m_lastProgram = 0;

#if defined(STARFISH_TIZEN) && defined(PORT_WEBVIEW_BRIDGE_EFL)
        // there is blinking on EvasGL with FBO
        // explicit sync fixes blinking
        if (m_seenFBOUsage && !g_isEvasGLOnDirectMode) {
            m_webView->platformWindow()->glMayNeedsSync();
        }
#endif
    }

    virtual void clearColor(const Unit::Color& clr)
    {
        LongTaskFinder p("CompositorImplGL::clearColor", 1);
        glClearColor(clr.R(), clr.G(), clr.B(), clr.A());
        glClear(GL_COLOR_BUFFER_BIT);
    }

    // state
    virtual void save()
    {
        auto s = m_state.back();
        CompositorImplGLState newState;
        newState.matrixStaysInRect = s.matrixStaysInRect;
        newState.color = s.color;
        newState.matrix = s.matrix;
        newState.opacity = s.opacity;
        newState.clipPaths = s.clipPaths;
        newState.clipPathsWasChanged = false;
        newState.clipPathsAreSimple = s.clipPathsAreSimple;
        m_state.push_back(s);
    }

    // pop state stack and restore state
    virtual void restore()
    {
        m_state.pop_back();
    }

    // transformations (default transform is the identity matrix)
    virtual void scale(double x, double y)
    {
        m_state.back().matrix.preScale(x, y);
    }

    virtual void rotate(double angle)
    {
        m_state.back().matrix.preRotate(angle);

        if (!m_state.back().matrix.rectStaysRect()) {
            m_state.back().matrixStaysInRect = false;
        }
    }

    virtual void translate(double x, double y)
    {
        m_state.back().matrix.preTranslate(x, y);
    }

    virtual void translate(LayoutUnit x, LayoutUnit y)
    {
        m_state.back().matrix.preTranslate((double)x, (double)y);
    }

    virtual void beginOpacityLayer(float c)
    {
        save();
        m_state.back().opacity *= c;
    }

    virtual void endOpacityLayer()
    {
        restore();
    }

    virtual void clip(const Unit::Rect& rt)
    {
        ClipperLib::Path path;
        SkPoint pt;
        pt = SkPoint::Make(rt.x(), rt.y());
        auto& lastState = m_state.back();
        lastState.matrix.mapPoints(&pt, 1);
        path.emplace_back(floor(pt.x()), floor(pt.y()));

        pt = SkPoint::Make(rt.x() + rt.width(), rt.y());
        lastState.matrix.mapPoints(&pt, 1);
        path.emplace_back(ceil(pt.x()), floor(pt.y()));

        pt = SkPoint::Make(rt.x() + rt.width(), rt.y() + rt.height());
        lastState.matrix.mapPoints(&pt, 1);
        path.emplace_back(ceil(pt.x()), ceil(pt.y()));

        pt = SkPoint::Make(rt.x(), rt.y() + rt.height());
        lastState.matrix.mapPoints(&pt, 1);
        path.emplace_back(floor(pt.x()), ceil(pt.y()));

        if (!lastState.clipPathsWasChanged) {
            lastState.clipPaths.reset(
                new ClipperLib::Paths(*lastState.clipPaths.get()));
            lastState.clipPathsWasChanged = true;
        }

        if (!lastState.matrixStaysInRect) {
            lastState.clipPathsAreSimple = false;
        }

        lastState.clipPaths.get()->push_back(path);
    }

    virtual void setColor(const Unit::Color& clr_)
    {
        m_state.back().color = clr_;
    }

    virtual void punchHole(const Unit::Rect& rt)
    {
        save();
        setColor(Unit::Color(0, 0, 0, 0));
        glBlendFunc(GL_ONE, GL_ZERO);
        drawRect(rt);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        restore();
    }

    virtual void drawRect(const Unit::Rect& rt)
    {
        float dest[4][2]; // 0(LT) 1(LB) 2(RT) 3(RB)

        auto& lastState = m_state.back();
        dest[0][0] = rt.x();
        dest[0][1] = rt.y();
        mapPointsToLogicalScreen(dest[0][0], dest[0][1]);

        dest[1][0] = rt.x();
        dest[1][1] = rt.maxY();
        mapPointsToLogicalScreen(dest[1][0], dest[1][1]);

        dest[2][0] = rt.maxX();
        dest[2][1] = rt.y();
        mapPointsToLogicalScreen(dest[2][0], dest[2][1]);

        dest[3][0] = rt.maxX();
        dest[3][1] = rt.maxY();
        mapPointsToLogicalScreen(dest[3][0], dest[3][1]);

        auto currentColor = lastState.color;

        if (lastState.clipPaths.get()->size()) {
            ClipperLib::Paths result = computeClippath(dest);
            if (result.size()) {
                if (lastState.matrixStaysInRect && result.size() == 1 &&
                    result[0].size() == 4) {
                    m_compositorContext->rectProgram();

                    float minX = (float)result[0][0].X,
                          minY = (float)result[0][0].Y,
                          maxX = (float)result[0][0].X,
                          maxY = (float)result[0][0].Y;

                    for (size_t i = 1; i < 4; i++) {
                        minX = std::min((float)result[0][i].X, minX);
                        minY = std::min((float)result[0][i].Y, minY);
                        maxX = std::max((float)result[0][i].X, maxX);
                        maxY = std::max((float)result[0][i].Y, maxY);
                    }

                    mapLogicalScreenPointsToScreen(minX, minY);
                    mapLogicalScreenPointsToScreen(maxX, maxY);

                    float hw = 2.f / m_webView->platformWindow()->width();
                    float hh = -2.f / m_webView->platformWindow()->height();
                    float position[] = {
                        minX * hw - 1, minY * hh + 1, // V1
                        minX * hw - 1, maxY * hh + 1, // V2
                        maxX * hw - 1, minY * hh + 1, // V3
                        maxX * hw - 1, maxY * hh + 1, // V4
                    };

                    glVertexAttribPointer(
                        m_compositorContext->m_rectShaderProgramPosition, 2,
                        GL_FLOAT, false, 0, position);

                    float a = lastState.opacity;

                    glUniform4f(m_compositorContext->m_rectShaderProgramColor,
                                a * currentColor.R(), a * currentColor.G(),
                                a * currentColor.B(), a * currentColor.A());

                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                    checkError();
                } else {
                    // polygon painting
                    std::vector<std::vector<Point>> polygon;
                    std::vector<Point> pointPerIndex;
                    for (size_t i = 0; i < result.size(); i++) {
                        polygon.push_back(std::vector<Point>());
                        for (size_t j = 0; j < result[i].size(); j++) {
                            polygon.back().push_back(
                                { (double)result[i][j].X,
                                  (double)result[i][j].Y });
                            pointPerIndex.push_back({ (double)result[i][j].X,
                                                      (double)result[i][j].Y });
                        }
                    }

                    m_compositorContext->rectProgram();
                    std::vector<N> indices = mapbox::earcut<N>(polygon);
                    for (size_t i = 0; i < indices.size(); i += 3) {
                        float trianglePoints[6] = {
                            (float)pointPerIndex[indices[i]][0],
                            (float)pointPerIndex[indices[i]][1],
                            (float)pointPerIndex[indices[i + 1]][0],
                            (float)pointPerIndex[indices[i + 1]][1],
                            (float)pointPerIndex[indices[i + 2]][0],
                            (float)pointPerIndex[indices[i + 2]][1]
                        };

                        mapLogicalScreenPointsToScreen(trianglePoints[0],
                                                       trianglePoints[1]);
                        mapLogicalScreenPointsToScreen(trianglePoints[2],
                                                       trianglePoints[3]);
                        mapLogicalScreenPointsToScreen(trianglePoints[4],
                                                       trianglePoints[5]);

                        float hw = 2.f / m_webView->platformWindow()->width();
                        float hh = -2.f / m_webView->platformWindow()->height();
                        float position[] = {
                            trianglePoints[0] * hw - 1,
                            trianglePoints[1] * hh + 1, // V1
                            trianglePoints[2] * hw - 1,
                            trianglePoints[3] * hh + 1, // V2
                            trianglePoints[4] * hw - 1,
                            trianglePoints[5] * hh + 1, // V3
                        };

                        glVertexAttribPointer(
                            m_compositorContext->m_rectShaderProgramPosition, 2,
                            GL_FLOAT, false, 0, position);
                        float a = lastState.opacity;
                        glUniform4f(
                            m_compositorContext->m_rectShaderProgramColor,
                            a * currentColor.R(), a * currentColor.G(),
                            a * currentColor.B(), a * currentColor.A());

                        glDrawArrays(GL_TRIANGLES, 0, 3);
                        checkError();
                    }
                }
            }
        } else {
            float hw = 2.f / m_webView->platformWindow()->width();
            float hh = -2.f / m_webView->platformWindow()->height();

            mapLogicalScreenPointsToScreen(dest[0][0], dest[0][1]);
            mapLogicalScreenPointsToScreen(dest[1][0], dest[1][1]);
            mapLogicalScreenPointsToScreen(dest[2][0], dest[2][1]);
            mapLogicalScreenPointsToScreen(dest[3][0], dest[3][1]);

            float data[] = {
                dest[0][0] * hw - 1, dest[0][1] * hh + 1, // V1
                dest[1][0] * hw - 1, dest[1][1] * hh + 1, // V2
                dest[2][0] * hw - 1, dest[2][1] * hh + 1, // V3
                dest[3][0] * hw - 1, dest[3][1] * hh + 1  // V4
            };

            m_compositorContext->rectProgram();

            glVertexAttribPointer(
                m_compositorContext->m_rectShaderProgramPosition, 2, GL_FLOAT,
                false, 0, &data[0]);

            float a = lastState.opacity;
            glUniform4f(m_compositorContext->m_rectShaderProgramColor,
                        a * currentColor.R(), a * currentColor.G(),
                        a * currentColor.B(), a * currentColor.A());

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            checkError();
        }
    }

    virtual void drawRect(const LayoutRect& rt)
    {
        drawRect(Unit::Rect(rt.x(), rt.y(), rt.width(), rt.height()));
    }

    // returns paths & paths stays in rect
    ClipperLib::Paths computeClippath(float (&dest)[4][2])
    {
        ClipperLib::Clipper clipper;

        ClipperLib::Path texture;
        texture.emplace_back(floor(dest[0][0]), floor(dest[0][1]));
        texture.emplace_back(ceil(dest[2][0]), floor(dest[2][1]));
        texture.emplace_back(ceil(dest[3][0]), ceil(dest[3][1]));
        texture.emplace_back(floor(dest[1][0]), ceil(dest[1][1]));

        // clipping debug code
        /*
        puts("dest");
        printf("%f,%f ", dest[0][0], dest[0][1]);
        printf("%f,%f ", dest[2][0], dest[2][1]);
        printf("%f,%f ", dest[3][0], dest[3][1]);
        printf("%f,%f ", dest[1][0], dest[1][1]);
        puts("texture");
        printf("%d,%d ", (int)texture[0].X, (int)texture[0].Y);
        printf("%d,%d ", (int)texture[1].X, (int)texture[1].Y);
        printf("%d,%d ", (int)texture[2].X, (int)texture[2].Y);
        printf("%d,%d ", (int)texture[3].X, (int)texture[3].Y);
        puts("");

        puts("clipPathlog");
        for (size_t i = 0; i < m_state.back().clipPaths.size(); i ++) {
            printf("i=%d ", (int)i);

            std::vector<float> pts;
            for (size_t j = 0; j < m_state.back().clipPaths[i].size(); j ++) {
                printf("%d,%d ", (int)m_state.back().clipPaths[i][j].X,
        (int)m_state.back().clipPaths[i][j].Y);
            }
            puts("");
        }
        */

        auto& lastState = m_state.back();
        clipper.AddPath(texture, ClipperLib::PolyType::ptSubject, true);
        clipper.AddPath(lastState.clipPaths.get()->at(0),
                        ClipperLib::PolyType::ptClip, true);

        ClipperLib::Paths result;
        clipper.Execute(ClipperLib::ClipType::ctIntersection, result);

        for (size_t i = 1; i < lastState.clipPaths.get()->size(); i++) {
            clipper.Clear();
            clipper.AddPaths(result, ClipperLib::PolyType::ptSubject, true);
            clipper.AddPath(lastState.clipPaths.get()->at(i),
                            ClipperLib::PolyType::ptClip, true);

            ClipperLib::Paths newResult;
            clipper.Execute(ClipperLib::ClipType::ctIntersection, newResult);
            result = newResult;
        }

        return result;
    }

    void drawFilteredTexture(CanvasSurfaceGL* cs, float dest[4][2],
                             GLuint textureID, GLenum textureKind,
                             GLenum textureBindNumber, size_t textureWidth,
                             size_t textureHeight)
    {
        auto& lastState = m_state.back();

        // Use FBO in order to 2-pass blur
        pushFBOContext(textureWidth, textureHeight, false,
                       LayoutRect(0, 0, textureWidth, textureHeight));

        bool isStencilEnabled = glIsEnabled(GL_STENCIL_TEST);
        bool isScissorEnabled = glIsEnabled(GL_SCISSOR_TEST);

        if (isStencilEnabled) {
            glDisable(GL_STENCIL_TEST);
        }
        if (isScissorEnabled) {
            glDisable(GL_SCISSOR_TEST);
        }

        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);

        bool isEGLImage = textureKind != GL_TEXTURE_2D;
        float blurMainRadius = lastState.blurRadius;
        // original code don't set sub radius but we set magic number
        // because we don't have antialias yet
        // setting sub radius reduce glitch
        float blurSubRadius = lastState.blurRadius / 5;
        if (blurSubRadius == (int)lastState.blurRadius) {
            blurSubRadius *= 0.85;
        }
        // blur W
        {
            float position[] = { -1, -1, -1, 1, 1, -1, 1, 1 };

            if (isEGLImage) {
                m_compositorContext->texBlurShaderProgramEGLImageExternalW();
            } else {
                m_compositorContext->texBlurShaderProgramW();
            }

            if (isEGLImage) {
                glVertexAttribPointer(
                    m_compositorContext
                        ->m_texBlurShaderProgramEGLImageExternalWPosition,
                    2, GL_FLOAT, false, 2 * 4, position);
                glUniform1f(
                    m_compositorContext
                        ->m_texBlurShaderProgramEGLImageExternalWTextureWidth,
                    textureWidth);
                glUniform1f(
                    m_compositorContext
                        ->m_texBlurShaderProgramEGLImageExternalWTextureHeight,
                    textureHeight);
                glUniform2f(
                    m_compositorContext
                        ->m_texBlurShaderProgramEGLImageExternalWBlurRadius,
                    blurMainRadius, blurSubRadius);
            } else {
                glVertexAttribPointer(
                    m_compositorContext->m_texBlurShaderProgramWPosition, 2,
                    GL_FLOAT, false, 2 * 4, position);
                glUniform1f(
                    m_compositorContext->m_texBlurShaderProgramWTextureWidth,
                    textureWidth);
                glUniform1f(
                    m_compositorContext->m_texBlurShaderProgramWTextureHeight,
                    textureHeight);
                glUniform2f(
                    m_compositorContext->m_texBlurShaderProgramWBlurRadius,
                    blurMainRadius, blurSubRadius);
            }
            glBindTexture(textureKind, textureID);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

        GLuint fboTex = popFBOContext();
        checkError();

        if (isStencilEnabled) {
            glEnable(GL_STENCIL_TEST);
        }
        if (isScissorEnabled) {
            glEnable(GL_SCISSOR_TEST);
        }

        // blur H
        {
            m_compositorContext->texBlurShaderProgramH();

            float hw = 2.f / m_webView->platformWindow()->width();
            float hh = -2.f / m_webView->platformWindow()->height();
            float position[] = {
                dest[0][0] * hw - 1, dest[0][1] * hh + 1, // V1
                dest[1][0] * hw - 1, dest[1][1] * hh + 1, // V2
                dest[2][0] * hw - 1, dest[2][1] * hh + 1, // V3
                dest[3][0] * hw - 1, dest[3][1] * hh + 1, // V4
            };

            glVertexAttribPointer(
                m_compositorContext->m_texBlurShaderProgramHPosition, 2,
                GL_FLOAT, false, 2 * 4, position);

            glBindTexture(GL_TEXTURE_2D, fboTex);

            glUniform1f(
                m_compositorContext->m_texBlurShaderProgramHTextureWidth,
                textureWidth);
            glUniform1f(
                m_compositorContext->m_texBlurShaderProgramHTextureHeight,
                textureHeight);

            glUniform2f(m_compositorContext->m_texBlurShaderProgramHBlurRadius,
                        -blurSubRadius, blurMainRadius);
            float a = lastState.opacity;
            if (a != 1) {
                glUniform1f(m_compositorContext->m_texBlurShaderProgramHAlpha,
                            a);
            }

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            if (glGetError() == 1286) {
                STARFISH_LOG_ERROR("drawFilteredTexture got error 1286\n");
            }
            if (a != 1) {
                glUniform1f(m_compositorContext->m_texBlurShaderProgramHAlpha,
                            1);
            }
            checkError();
        }

        glDeleteTextures(1, &fboTex);
        checkError();
    }

    void drawTexture(CanvasSurfaceGL* cs, float dest[4][2], GLuint textureID,
                     GLenum textureKind, GLenum textureBindNumber,
                     size_t textureWidth, size_t textureHeight)
    {
        mapLogicalScreenPointsToScreen(dest[0][0], dest[0][1]);
        mapLogicalScreenPointsToScreen(dest[1][0], dest[1][1]);
        mapLogicalScreenPointsToScreen(dest[2][0], dest[2][1]);
        mapLogicalScreenPointsToScreen(dest[3][0], dest[3][1]);

        auto& lastState = m_state.back();
        if (lastState.blurRadius) {
            drawFilteredTexture(cs, dest, textureID, textureKind,
                                textureBindNumber, textureWidth, textureHeight);
            return;
        }
        bool isEGLImage = textureKind != GL_TEXTURE_2D;
        if (isEGLImage) {
            if (cs->m_isEGLImageNeedsFlipRGB) {
                m_compositorContext
                    ->texShaderProgramEGLImageExternalWithOriginalColor();
            } else {
                m_compositorContext->texShaderProgramEGLImageExternal();
            }
        } else {
            m_compositorContext->texShaderProgram();
        }

        float hw = 2.f / m_webView->platformWindow()->width();
        float hh = -2.f / m_webView->platformWindow()->height();
        float position[] = {
            dest[0][0] * hw - 1, dest[0][1] * hh + 1, // V1
            dest[1][0] * hw - 1, dest[1][1] * hh + 1, // V2
            dest[2][0] * hw - 1, dest[2][1] * hh + 1, // V3
            dest[3][0] * hw - 1, dest[3][1] * hh + 1, // V4
        };

        glBindTexture(textureKind, textureID);

        GLint* positionPos;
        GLint* alphaPos;

        float a = lastState.opacity;
        if (isEGLImage) {
            if (cs->m_isEGLImageNeedsFlipRGB) {
                positionPos =
                    &m_compositorContext
                         ->m_texShaderProgramEGLImageExternalWithOriginalColorPosition;
                alphaPos =
                    &m_compositorContext
                         ->m_texShaderProgramEGLImageExternalWithOriginalColorAlpha;
            } else {
                positionPos = &m_compositorContext
                                   ->m_texShaderProgramEGLImageExternalPosition;
                alphaPos = &m_compositorContext
                                ->m_texShaderProgramEGLImageExternalAlpha;
            }
        } else {
            positionPos = &m_compositorContext->m_texShaderProgramPosition;
            alphaPos = &m_compositorContext->m_texShaderProgramAlpha;
        }

        glVertexAttribPointer(*positionPos, 2, GL_FLOAT, false, 2 * 4,
                              position);
        if (a != 1) {
            glUniform1f(*alphaPos, a);
        }

        checkError();

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        checkError();

        if (a != 1) {
            glUniform1f(*alphaPos, 1);
        }
    }

    Unit::Rect boundingRect(const ClipperLib::Path& path)
    {
        int minX = 0, minY = 0, maxX = 0, maxY = 0;

        if (path.size()) {
            minX = path[0].X;
            minY = path[0].Y;
            maxX = path[0].X;
            maxY = path[0].Y;
        }

        for (size_t i = 1; i < path.size(); i++) {
            minX = std::min((int)path[i].X, minX);
            minY = std::min((int)path[i].Y, minY);
            maxX = std::max((int)path[i].X, maxX);
            maxY = std::max((int)path[i].Y, maxY);
        }

        return Unit::Rect(minX, minY, std::abs(maxX - minX),
                          std::abs(maxY - minY));
    }

    virtual void drawSurface(CanvasSurface* cs, const Unit::Rect& dst)
    {
        INSTALL_PROFILE_TIMER("CompositorGL::drawSurface");
        auto textureInfo = cs->textureInfo();
        if (textureInfo.fragments.size() == 0) {
            return;
        }

        float dest[4][2]; // 0(LT) 1(LB) 2(RT) 3(RB)

        CanvasSurfaceGL* csGL = (CanvasSurfaceGL*)cs;

        auto& lastState = m_state.back();
        dest[0][0] = dst.x();
        dest[0][1] = dst.y();
        mapPointsToLogicalScreen(dest[0][0], dest[0][1]);

        dest[1][0] = dst.x();
        dest[1][1] = dst.maxY();
        mapPointsToLogicalScreen(dest[1][0], dest[1][1]);

        dest[2][0] = dst.maxX();
        dest[2][1] = dst.y();
        mapPointsToLogicalScreen(dest[2][0], dest[2][1]);

        dest[3][0] = dst.maxX();
        dest[3][1] = dst.maxY();
        mapPointsToLogicalScreen(dest[3][0], dest[3][1]);

        bool fboStencilClippingEnabled = false;
        bool stencilClippingEnabled = false;
        bool scissorClippingEnabled = false;
        bool shouldSkipTexturePainting = false;
        Unit::Rect visibleArea =
            Unit::Rect(0, 0, m_webView->platformWindow()->width(),
                       m_webView->platformWindow()->height());

        if (lastState.clipPaths.get()->size()) {
            if (lastState.clipPathsAreSimple) {
                const ClipperLib::Paths& clipPaths = *lastState.clipPaths.get();
                for (size_t i = 0; i < clipPaths.size(); i++) {
                    const Unit::Rect& r1 = visibleArea;
                    STARFISH_ASSERT(clipPaths[i].size() == 4);

                    float minX = (float)clipPaths[i][0].X,
                          minY = (float)clipPaths[i][0].Y,
                          maxX = (float)clipPaths[i][0].X,
                          maxY = (float)clipPaths[i][0].Y;

                    for (size_t j = 1; j < 4; j++) {
                        minX = std::min((float)clipPaths[i][j].X, minX);
                        minY = std::min((float)clipPaths[i][j].Y, minY);
                        maxX = std::max((float)clipPaths[i][j].X, maxX);
                        maxY = std::max((float)clipPaths[i][j].Y, maxY);
                    }

                    Unit::Rect r2(minX, minY, maxX - minX, maxY - minY);
                    float leftX = std::max(r1.x(), r2.x());
                    float rightX = std::min(r1.maxX(), r2.maxX());
                    float topY = std::max(r1.y(), r2.y());
                    float bottomY = std::min(r1.maxY(), r2.maxY());

                    if (leftX < rightX && topY < bottomY) {
                        visibleArea = Unit::Rect(leftX, topY, rightX - leftX,
                                                 bottomY - topY);
                    } else {
                        // Rectangles do not overlap, or overlap has an area of
                        // zero (edge/corner overlap)
                        visibleArea = Unit::Rect(0, 0, 0, 0);
                        shouldSkipTexturePainting = true;
                        break;
                    }
                }

                glEnable(GL_SCISSOR_TEST);
                scissor(visibleArea.x(), visibleArea.y(), visibleArea.width(),
                        visibleArea.height());
                scissorClippingEnabled = true;
            } else {
                visibleArea = Unit::Rect(0, 0, 0, 0);
                ClipperLib::Paths result = computeClippath(dest);
                if (result.size()) {
                    if (lastState.matrixStaysInRect && result.size() == 1 &&
                        result[0].size() == 4) {
                        scissorClippingEnabled = true;

                        float minX = (float)result[0][0].X,
                              minY = (float)result[0][0].Y,
                              maxX = (float)result[0][0].X,
                              maxY = (float)result[0][0].Y;

                        for (size_t i = 1; i < 4; i++) {
                            minX = std::min((float)result[0][i].X, minX);
                            minY = std::min((float)result[0][i].Y, minY);
                            maxX = std::max((float)result[0][i].X, maxX);
                            maxY = std::max((float)result[0][i].Y, maxY);
                        }

                        visibleArea =
                            Unit::Rect(minX, minY, maxX - minX, maxY - minY);
                        glEnable(GL_SCISSOR_TEST);
                        scissor(minX, minY, maxX - minX, maxY - minY);
                    } else {
                        std::vector<std::vector<Point>> polygon;
                        std::vector<Point> pointPerIndex;
                        for (size_t i = 0; i < result.size(); i++) {
                            polygon.push_back(std::vector<Point>());
                            for (size_t j = 0; j < result[i].size(); j++) {
                                polygon.back().push_back(
                                    { (double)result[i][j].X,
                                      (double)result[i][j].Y });
                                pointPerIndex.push_back(
                                    { (double)result[i][j].X,
                                      (double)result[i][j].Y });
                            }

                            visibleArea.unite(boundingRect(result[i]));
                        }

                        if (g_useStencilBufferOnFBO) {
                            fboStencilClippingEnabled = true;
                            size_t w = m_webView->platformWindow()->width();
                            size_t h = m_webView->platformWindow()->height();
                            pushFBOContext(w, h, true, LayoutRect(0, 0, w, h));
                        }

                        // do render on fbo
                        glClearColor(0, 0, 0, 0);
                        glClear(GL_COLOR_BUFFER_BIT);
                        checkError();

                        stencilClippingEnabled = true;

                        glEnable(GL_STENCIL_TEST);
                        glClearStencil(0);
                        glClear(GL_STENCIL_BUFFER_BIT);
                        glColorMask(false, false, false, false);
                        glStencilFunc(GL_ALWAYS, 1, 1);
                        glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

                        m_compositorContext->rectProgram();
                        std::vector<N> indices = mapbox::earcut<N>(polygon);
                        for (size_t i = 0; i < indices.size(); i += 3) {
                            float trianglePoints[6] = {
                                (float)pointPerIndex[indices[i]][0],
                                (float)pointPerIndex[indices[i]][1],
                                (float)pointPerIndex[indices[i + 1]][0],
                                (float)pointPerIndex[indices[i + 1]][1],
                                (float)pointPerIndex[indices[i + 2]][0],
                                (float)pointPerIndex[indices[i + 2]][1]
                            };

                            mapLogicalScreenPointsToScreen(trianglePoints[0],
                                                           trianglePoints[1]);
                            mapLogicalScreenPointsToScreen(trianglePoints[2],
                                                           trianglePoints[3]);
                            mapLogicalScreenPointsToScreen(trianglePoints[4],
                                                           trianglePoints[5]);

                            float hw =
                                2.f / m_webView->platformWindow()->width();
                            float hh =
                                -2.f / m_webView->platformWindow()->height();
                            float position[] = {
                                trianglePoints[0] * hw - 1,
                                trianglePoints[1] * hh + 1, // V1
                                trianglePoints[2] * hw - 1,
                                trianglePoints[3] * hh + 1, // V2
                                trianglePoints[4] * hw - 1,
                                trianglePoints[5] * hh + 1, // V3
                            };

                            glVertexAttribPointer(
                                m_compositorContext
                                    ->m_rectShaderProgramPosition,
                                2, GL_FLOAT, false, 0, position);
                            glUniform4f(
                                m_compositorContext->m_rectShaderProgramColor,
                                Unit::Color(255, 255, 255, 255).R(),
                                Unit::Color(255, 255, 255, 255).G(),
                                Unit::Color(255, 255, 255, 255).B(),
                                Unit::Color(255, 255, 255, 255).A());
                            glDrawArrays(GL_TRIANGLES, 0, 3);
                            checkError();
                        }

                        glColorMask(true, true, true, true);
                        glStencilFunc(GL_EQUAL, 1, 1);
                        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
                    }
                } else {
                    shouldSkipTexturePainting = true;
                }
            }
        }

        if (!shouldSkipTexturePainting) {
            if (csGL->m_isEGLImageExternal) {
                drawTexture(csGL, dest, csGL->m_textureFragments[0].textureID,
                            GL_TEXTURE_EXTERNAL_OES, -1, csGL->m_bufferWidth,
                            csGL->m_bufferHeight);
            } else {
                size_t coveredRowsCount = 0;
                size_t i = 0;
                for (size_t y = 0; y < csGL->hTextureCount(); y++) {
                    size_t coveredColsCount = 0;
                    for (size_t x = 0; x < csGL->wTextureCount(); x++) {
                        size_t texureDataX = coveredColsCount;
                        size_t texureDataY = coveredRowsCount;
                        size_t texureDataWidth =
                            std::min((size_t)csGL->textureTileSize(),
                                     cs->bufferWidth() - coveredColsCount);
                        size_t texureDataHeight =
                            std::min((size_t)csGL->textureTileSize(),
                                     cs->bufferHeight() - coveredRowsCount);

                        if (csGL->m_flag &
                            CanvasSurface::ElementHasFilterEffect) {
                            texureDataWidth = cs->bufferWidth();
                            texureDataHeight = cs->bufferHeight();
                        }

                        float newDest[4][2]; // 0(LT) 1(LB) 2(RT) 3(RB)

                        auto& fragment = textureInfo.fragments[i];
                        float oldW = dst.width();
                        float oldH = dst.height();
                        Unit::Rect newDst(oldW * fragment.srcX + dst.x(),
                                          oldH * fragment.srcY + dst.y(),
                                          oldW * fragment.srcWidth,
                                          oldH * fragment.srcHeight);

                        SkPoint pt;
                        pt = SkPoint::Make(newDst.x(), newDst.y());

                        lastState.matrix.mapPoints(&pt, 1);
                        newDest[0][0] = pt.x();
                        newDest[0][1] = pt.y();

                        pt = SkPoint::Make(newDst.x(), newDst.maxY());
                        lastState.matrix.mapPoints(&pt, 1);
                        newDest[1][0] = pt.x();
                        newDest[1][1] = pt.y();

                        pt = SkPoint::Make(newDst.maxX(), newDst.y());
                        lastState.matrix.mapPoints(&pt, 1);
                        newDest[2][0] = pt.x();
                        newDest[2][1] = pt.y();

                        pt = SkPoint::Make(newDst.maxX(), newDst.maxY());
                        lastState.matrix.mapPoints(&pt, 1);
                        newDest[3][0] = pt.x();
                        newDest[3][1] = pt.y();

                        float minX = newDest[0][0], minY = newDest[0][1],
                              maxX = newDest[0][0], maxY = newDest[0][1];

                        for (size_t i = 1; i < 4; i++) {
                            minX = std::min(newDest[i][0], minX);
                            minY = std::min(newDest[i][1], minY);
                            maxX = std::max(newDest[i][0], maxX);
                            maxY = std::max(newDest[i][1], maxY);
                        }

                        Unit::Rect screenBoundingRect(minX, minY,
                                                      std::abs(maxX - minX),
                                                      std::abs(maxY - minY));

                        if (screenBoundingRect.intersects(visibleArea)) {
                            GLuint tid = (GLuint)fragment.textureID;
                            drawTexture(csGL, newDest, tid, GL_TEXTURE_2D,
                                        GL_TEXTURE0, texureDataWidth,
                                        texureDataHeight);
                        }
                        i++;
                        coveredColsCount += csGL->textureTileSize();
                    }

                    coveredRowsCount += csGL->textureTileSize();
                }
            }
        }

        if (stencilClippingEnabled) {
            glDisable(GL_STENCIL_TEST);
            if (fboStencilClippingEnabled) {
                GLuint fboTex = popFBOContext();
                checkError();

                m_compositorContext->texShaderProgramWithOriginalColor();

                float position[] = { -1, -1, -1, 1, 1, -1, 1, 1 };

                glBindTexture(GL_TEXTURE_2D, fboTex);
                checkError();

                glVertexAttribPointer(
                    m_compositorContext->m_texShaderProgramPosition, 2,
                    GL_FLOAT, false, 2 * 4, position);
                checkError();

                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

                auto errChk = glGetError();
                if (errChk == 1286) {
                    STARFISH_LOG_ERROR("fbo stencil clipping got error 1286\n");
                }
                glDeleteTextures(1, &fboTex);
            }
        }
        if (scissorClippingEnabled) {
            glDisable(GL_SCISSOR_TEST);
        }
    }

    virtual void postMatrix(const SkMatrix& matrix)
    {
        auto& lastState = m_state.back();
        lastState.matrix.preConcat(matrix);

        if (!lastState.matrix.rectStaysRect()) {
            lastState.matrixStaysInRect = false;
        }
    }

    SkMatrix currentTransformMatrix()
    {
        return m_state.back().matrix;
    }

    virtual void applyMatrixTo(LayoutLocation& lp)
    {
        SkPoint point = SkPoint::Make((float)lp.x(), (float)lp.y());
        m_state.back().matrix.mapPoints(&point, 1);
        lp.setX(point.x());
        lp.setY(point.y());
    }

    virtual void applyMatrixTo(LayoutRect& lp)
    {
        SkRect sss = SkRect::MakeXYWH(SkFloatToScalar((float)lp.x()),
                                      SkFloatToScalar((float)lp.y()),
                                      SkFloatToScalar((float)lp.width()),
                                      SkFloatToScalar((float)lp.height()));
        m_state.back().matrix.mapRect(&sss);
        sss.sort();
        lp.setX(sss.x());
        lp.setY(sss.y());
        lp.setWidth(sss.width());
        lp.setHeight(sss.height());
    }

    virtual void resetMatrixAndClip()
    {
        auto& lastState = m_state.back();
        lastState.matrix = SkMatrix::I();
        if (!lastState.clipPathsWasChanged) {
            lastState.clipPaths.reset(new ClipperLib::Paths());
            lastState.clipPathsWasChanged = true;
        } else {
            lastState.clipPaths.get()->clear();
        }
        lastState.matrixStaysInRect = true;
        lastState.clipPathsAreSimple = true;

        clip(Unit::Rect(0, 0, m_webView->platformWindow()->width(),
                        m_webView->platformWindow()->height()));
        applyDevicePixelRatio();
    }

    virtual void resetClip()
    {
        auto& lastState = m_state.back();
        if (!lastState.clipPathsWasChanged) {
            lastState.clipPaths.reset(new ClipperLib::Paths());
            lastState.clipPathsWasChanged = true;
        } else {
            lastState.clipPaths.get()->clear();
        }
        lastState.clipPathsAreSimple = true;
        clip(Unit::Rect(0, 0, m_webView->platformWindow()->width(),
                        m_webView->platformWindow()->height()));
    }

    void addToPath(float x, float y)
    {
        SkPoint pt = SkPoint::Make(x, y);
        m_state.back().matrix.mapPoints(&pt, 1);
        m_path.push_back(
            ClipperLib::IntPoint(floor(pt.x() + 0.5f), floor(pt.y() + 0.5f)));
    }

    virtual void moveTo(float x, float y)
    {
        addToPath(x, y);
    }

    virtual void lineTo(float x, float y)
    {
        addToPath(x, y);
    }

    virtual void arcNegative(double cx, double cy, double radius, double angle1,
                             double angle2)
    {
        float angleDiff = angle2 - angle1;
        if (std::abs(angleDiff) >= M_PI * 2) {
            angleDiff = -M_PI * 2;
        } else {
            while (angleDiff > 0.0f) {
                angleDiff -= M_PI * 2;
            }
        }

        size_t divCount = std::abs(radius * angleDiff) *
                          m_state.back().matrix.getScaleX() *
                          m_state.back().matrix.getScaleY();
        if (divCount == 0) {
            divCount = 1;
        }

        for (size_t i = 0; i <= divCount; i++) {
            float a = angle1 + angleDiff * (i / (float)divCount);
            float dx = cos(a);
            float dy = sin(a);
            float x = cx + dx * radius;
            float y = cy + dy * radius;
            addToPath(x, y);
        }
    }
    virtual void clipPath()
    {
        auto& lastState = m_state.back();
        if (!lastState.clipPathsWasChanged) {
            lastState.clipPaths.reset(
                new ClipperLib::Paths(*lastState.clipPaths.get()));
            lastState.clipPathsWasChanged = true;
        }
        lastState.clipPaths.get()->push_back(m_path);
        lastState.clipPathsAreSimple = false;
        m_path.clear();
        m_path.shrink_to_fit();
    }

    virtual bool supportsBlurEffect()
    {
        return true;
    }

    virtual void enableBlurEffect(float blurRadius)
    {
        m_state.back().blurRadius = blurRadius;
    }

    struct FBOState {
        GLuint fboId;
        GLuint fboTex;
        GLuint fboSupportTex;
        GLuint renderBufferId;
        LayoutRect viewport;
    };

    void pushFBOContext(size_t width, size_t height, bool needsStencilDepth,
                        LayoutRect viewport)
    {
        m_seenFBOUsage = true;

        FBOState newFBOState;

        // generate FBO
        glGenFramebuffers(1, &newFBOState.fboId);
        checkError();

        // generate texture
        glGenTextures(1, &newFBOState.fboTex);
        checkError();

        // generate render buffer
        glGenRenderbuffers(1, &newFBOState.renderBufferId);
        checkError();

        // Bind Frame buffer
        glBindFramebuffer(GL_FRAMEBUFFER, newFBOState.fboId);
        checkError();

        // Bind texture
        glBindTexture(GL_TEXTURE_2D, newFBOState.fboTex);
        checkError();

        // Define texture parameters
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        checkError();

        // Bind render buffer and define buffer dimension
        glBindRenderbuffer(GL_RENDERBUFFER, newFBOState.renderBufferId);

        // Attach texture FBO color attachment
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, newFBOState.fboTex, 0);
        checkError();

        if (needsStencilDepth) {
            glGenTextures(1, &newFBOState.fboSupportTex);
            glBindTexture(GL_TEXTURE_2D, newFBOState.fboSupportTex);
            glTexImage2D(GL_TEXTURE_2D, 0, // target and mipmap level
                         GL_DEPTH_STENCIL, width, height, // size of texture
                         0,                               // border size
                         GL_DEPTH_STENCIL, // format of of data we are uploading
                                           // to to the texture (ignored)
                         GL_UNSIGNED_INT_24_8, // type of of data we are
                                               // uploading to to the texture
                                               // (ignored)
                         NULL /* no data uploaded */);
            checkError();
            // attatch the depth/stencil texture to both the stencil and depth
            // render objects.
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                   GL_TEXTURE_2D, newFBOState.fboSupportTex, 0);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                   GL_TEXTURE_2D, newFBOState.fboSupportTex, 0);
            checkError();
        } else {
            newFBOState.fboSupportTex = 0;
        }

        newFBOState.viewport = viewport;
        glViewport(viewport.x(), viewport.y(), viewport.width(),
                   viewport.height());

        m_fboState.push_back(newFBOState);
    }

    GLuint popFBOContext() // returns texture
    {
        FBOState lastState = m_fboState.back();
        m_fboState.pop_back();

        if (m_fboState.size()) {
            auto& s = m_fboState.back();
            glBindRenderbuffer(GL_RENDERBUFFER, s.renderBufferId);
            glBindFramebuffer(GL_FRAMEBUFFER, s.fboId);

            glViewport(s.viewport.x(), s.viewport.y(), s.viewport.width(),
                       s.viewport.height());
        } else {
            glBindRenderbuffer(GL_RENDERBUFFER, 0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            setViewport();
        }

        glDeleteRenderbuffers(1, &lastState.renderBufferId);
        glDeleteFramebuffers(1, &lastState.fboId);
        if (lastState.fboSupportTex) {
            glDeleteTextures(1, &lastState.fboSupportTex);
        }
        return lastState.fboTex;
    }

protected:
    bool m_seenFBOUsage;
    WebView* m_webView;
    CompositorContext* m_compositorContext;
    std::vector<CompositorImplGLState> m_state;
    std::vector<FBOState> m_fboState;

    ClipperLib::Path m_path;
    SkMatrix m_screenMatrix;
};

Compositor* Compositor::create3D(WebView* webView, CompositorContext* ctx)
{
    return new CompositorImplGL(webView, ctx);
}

Compositor* Compositor::create2D(WebView* webView, CompositorContext* ctx,
                                 CanvasSurface* surface)
{
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
}

bool Compositor::supportsFilterEffect(size_t textureWidth, size_t textureHeight)
{
    if (textureWidth > g_maxTextureSize || textureHeight > g_maxTextureSize) {
        return false;
    }
    return true;
}

#if defined(STARFISH_ENABLE_TEST)
#if defined(PORT_CANVAS_BACKEND_CAIRO)
void screenShotImpl(PlatformWindow* wnd, const char* path,
                    std::function<void()> callback)
{
    glFinish();

    auto deviceWidth = wnd->width();
    auto deviceHeight = wnd->height();
    auto rowLength = deviceWidth * 4;

    auto dataLength = rowLength * deviceHeight;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    uint8_t* buffer = new uint8_t[dataLength];
    glReadPixels(0, 0, deviceWidth, deviceHeight, GL_RGBA, GL_UNSIGNED_BYTE,
                 buffer);

    // convert to rgba to bgra for cairo
    for (uint32_t y = 0; y < deviceHeight; y++) {
        for (uint32_t x = 0; x < deviceWidth; x++) {
            uint8_t* head = &buffer[rowLength * y + x * 4];
            std::swap(head[0], head[2]);
        }
    }

    // flip W
    /*
        for (uint32_t y = 0; y < deviceHeight; y++) {
            uint32_t* head = (uint32_t*)&buffer[rowLength * y];
            for (uint32_t x = 0; x < deviceWidth / 2; x++) {
                std::swap(head[x], head[deviceWidth - x - 1]);
            }
        }
    */
    // flip H
    for (uint32_t y = 0; y < deviceHeight / 2; y++) {
        uint32_t* head = (uint32_t*)&buffer[rowLength * y];
        uint32_t* head2 =
            (uint32_t*)&buffer[rowLength * (deviceHeight - y - 1)];
        for (uint32_t x = 0; x < deviceWidth; x++) {
            std::swap(head[x], head2[x]);
        }
    }

    cairo_surface_t* png_buffer;
    png_buffer = cairo_image_surface_create_for_data(
        (unsigned char*)buffer, CAIRO_FORMAT_ARGB32, deviceWidth, deviceHeight,
        rowLength);

    cairo_surface_write_to_png(png_buffer, path);
    cairo_surface_destroy(png_buffer);

    delete[] buffer;
    callback();
}
#elif defined(PORT_CANVAS_BACKEND_SKIA)
void screenShotImpl(PlatformWindow* wnd, const char* path,
                    std::function<void()> callback)
{
}
#endif
#endif

} // namespace Starfish

#endif
