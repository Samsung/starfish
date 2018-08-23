/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include "StarFish.h"

#if defined(PORT_COMPOSITOR_BACKEND_GL)

#include "core/modules/canvas/Compositor.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/image/NativeImageData.h"
#include "platform/window/PlatformWindow.h"

#if defined(STARFISH_ENABLE_TEST)
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
#elif defined(STARFISH_WINDOWS)
#include <GL/glew.h>
#include <GL/wglew.h>

#include <Windows.h>
#pragma comment(lib, "Opengl32.lib")
#else
#include <GLES3/gl3.h>
#endif

#ifndef GL_TEXTURE_EXTERNAL_OES
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#endif

#if defined(STARFISH_TIZEN)
#include <tbm_surface.h>
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

#if defined(PORT_WEBVIEW_BRIDGE_EFL)
Evas_GL_API* g_evasGLAPI;
#endif

namespace StarFish {

static size_t g_totalCanvasSurfaceGLSize;
static size_t g_textureTileSize = 512;
static bool g_needsCheckCompatibility = true;
static bool g_isSupportPixelStoreiUnpackingOfPixelDataFromMemory = false;
static bool g_isSupportExtensionEGLImageExternal = false;
static size_t g_maxTextureSize;

class CompositorContext {
public:
    GLuint m_texShaderProgram;
    GLuint m_texVertexShader;
    GLuint m_texFragmentShader;

    GLuint m_texWithAlphaShaderProgram;
    GLuint m_texWithAlphaFragmentShader;

    GLuint m_rectShaderProgram;
    GLuint m_rectVertexShader;
    GLuint m_rectFragmentShader;

    GLuint m_texShaderProgramEGLImageExternal;
    GLuint m_texFragmentShaderEGLImageExternal;

    GLuint m_texWithAlphaShaderProgramEGLImageExternal;
    GLuint m_texWithAlphaFragmentShaderEGLImageExternal;
};

static void checkError()
{
#ifndef NDEBUG
    volatile auto error = glGetError();
    if (error != 0) {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
#endif
}

static GLuint loadShader(GLenum type, const GLchar* shaderSrc)
{
    GLuint shader;
    GLint compiled;

    // Create the shader object
    shader = glCreateShader(type);

    if (glGetError()) {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    // Load the shader source
    glShaderSource(shader, 1, &shaderSrc, NULL);

    // Compile the shader
    glCompileShader(shader);

    // Check the compile status
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled) {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    return shader;
}

void Compositor::destroyCompositorContext(CompositorContext* ctx)
{
    delete ctx;
}

CompositorContext* Compositor::initCompositorContext(PlatformWindow* wnd)
{
    wnd->glMakeCurrent();

    if (g_needsCheckCompatibility) {
        GLint siz;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &siz);
        checkError();
        g_maxTextureSize = siz;

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

        if (major >= 3) {
            g_isSupportPixelStoreiUnpackingOfPixelDataFromMemory = true;
        }

        g_isSupportExtensionEGLImageExternal =
            strstr((const char*)glGetString(GL_EXTENSIONS),
                   "GL_OES_EGL_image_external") != nullptr;

#ifndef STARFISH_TIZEN
        g_isSupportExtensionEGLImageExternal = false;
#endif

        g_needsCheckCompatibility = false;
        checkError();
    }

    CompositorContext* compositorContext = new CompositorContext;

    GLchar texVertexSource[] =
        "uniform mat4 uScreen;\n"
        "attribute vec2 aPosition;\n"
        "attribute vec2 aTexPos;\n"
        "varying vec2 vTexPos;\n"
        "void main() {\n"
        "  vTexPos = aTexPos;\n"
        "  gl_Position = uScreen * vec4(aPosition.xy, 0.0, 1.0);\n"
        "}";

    // We only Support OpenGL ES 2.0+ context
    // but some develoment environment only support desktop context
    // so we add `#ifdef GL_ES` for debug purpose
    GLchar texFragmentSource[] =
        "#ifdef GL_ES\n"
        "  precision mediump float;\n"
        "#endif\n"
        "uniform sampler2D uTexture;\n"
        "varying vec2 vTexPos;\n"
        "void main(void)\n"
        "{\n"
        "  gl_FragColor = texture2D(uTexture, vTexPos);\n"
        "}";

    compositorContext->m_texVertexShader =
        loadShader(GL_VERTEX_SHADER, texVertexSource);
    checkError();
    compositorContext->m_texFragmentShader =
        loadShader(GL_FRAGMENT_SHADER, texFragmentSource);
    checkError();

    compositorContext->m_texShaderProgram = glCreateProgram();
    checkError();

    glAttachShader(compositorContext->m_texShaderProgram,
                   compositorContext->m_texVertexShader);
    checkError();
    glAttachShader(compositorContext->m_texShaderProgram,
                   compositorContext->m_texFragmentShader);
    checkError();

    glLinkProgram(compositorContext->m_texShaderProgram);
    checkError();

    glUseProgram(compositorContext->m_texShaderProgram);
    checkError();

    GLchar texWithAlphaFragmentSource[] =
        "#ifdef GL_ES\n"
        "  precision mediump float;\n"
        "#endif\n"
        "uniform sampler2D uTexture;\n"
        "varying vec2 vTexPos;\n"
        "uniform vec4 uAlpha;\n"
        "void main(void)\n"
        "{\n"
        "  gl_FragColor = texture2D(uTexture, vTexPos) * uAlpha;\n"
        "}";

    compositorContext->m_texWithAlphaFragmentShader =
        loadShader(GL_FRAGMENT_SHADER, texWithAlphaFragmentSource);
    checkError();

    compositorContext->m_texWithAlphaShaderProgram = glCreateProgram();
    checkError();

    glAttachShader(compositorContext->m_texWithAlphaShaderProgram,
                   compositorContext->m_texVertexShader);
    checkError();
    glAttachShader(compositorContext->m_texWithAlphaShaderProgram,
                   compositorContext->m_texWithAlphaFragmentShader);
    checkError();

    glLinkProgram(compositorContext->m_texWithAlphaShaderProgram);
    checkError();

    glUseProgram(compositorContext->m_texWithAlphaShaderProgram);
    checkError();

    if (g_isSupportExtensionEGLImageExternal) {
        GLchar texFragmentSourceEGLImageExternal[] =
            "#extension GL_OES_EGL_image_external : require\n"
            "#ifdef GL_ES\n"
            "  precision mediump float;\n"
            "#endif\n"
            "uniform samplerExternalOES uTexture;\n"
            "varying vec2 vTexPos;\n"
            "void main(void)\n"
            "{\n"
            "  gl_FragColor = texture2D(uTexture, vTexPos);\n"
            "}";

        compositorContext->m_texFragmentShaderEGLImageExternal =
            loadShader(GL_FRAGMENT_SHADER, texFragmentSourceEGLImageExternal);
        checkError();

        compositorContext->m_texShaderProgramEGLImageExternal =
            glCreateProgram();
        checkError();

        glAttachShader(compositorContext->m_texShaderProgramEGLImageExternal,
                       compositorContext->m_texVertexShader);
        checkError();
        glAttachShader(compositorContext->m_texShaderProgramEGLImageExternal,
                       compositorContext->m_texFragmentShaderEGLImageExternal);
        checkError();

        glLinkProgram(compositorContext->m_texShaderProgramEGLImageExternal);
        checkError();

        glUseProgram(compositorContext->m_texShaderProgramEGLImageExternal);
        checkError();

        GLchar texWithAlphaFragmentSourceEGLImageExternal[] =
            "#extension GL_OES_EGL_image_external : require\n"
            "#ifdef GL_ES\n"
            "  precision mediump float;\n"
            "#endif\n"
            "uniform samplerExternalOES uTexture;\n"
            "varying vec2 vTexPos;\n"
            "uniform vec4 uAlpha;\n"
            "void main(void)\n"
            "{\n"
            "  gl_FragColor = texture2D(uTexture, vTexPos) * uAlpha;\n"
            "}";

        compositorContext->m_texWithAlphaFragmentShaderEGLImageExternal =
            loadShader(GL_FRAGMENT_SHADER,
                       texWithAlphaFragmentSourceEGLImageExternal);
        checkError();

        compositorContext->m_texWithAlphaShaderProgramEGLImageExternal =
            glCreateProgram();
        checkError();

        glAttachShader(
            compositorContext->m_texWithAlphaShaderProgramEGLImageExternal,
            compositorContext->m_texVertexShader);
        checkError();
        glAttachShader(
            compositorContext->m_texWithAlphaShaderProgramEGLImageExternal,
            compositorContext->m_texWithAlphaFragmentShaderEGLImageExternal);
        checkError();

        glLinkProgram(
            compositorContext->m_texWithAlphaShaderProgramEGLImageExternal);
        checkError();

        glUseProgram(
            compositorContext->m_texWithAlphaShaderProgramEGLImageExternal);
        checkError();

    } else {
        compositorContext->m_texShaderProgramEGLImageExternal = 0;
        compositorContext->m_texFragmentShaderEGLImageExternal = 0;
        compositorContext->m_texWithAlphaShaderProgramEGLImageExternal = 0;
        compositorContext->m_texWithAlphaFragmentShaderEGLImageExternal = 0;
    }

    GLchar rectVertexSource[] =
        "uniform mat4 uScreen;\n"
        "attribute vec2 aPosition;\n"
        "void main() {\n"
        "  gl_Position = uScreen * vec4(aPosition.xy, 0.0, 1.0);\n"
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

    compositorContext->m_rectVertexShader =
        loadShader(GL_VERTEX_SHADER, rectVertexSource);
    checkError();
    compositorContext->m_rectFragmentShader =
        loadShader(GL_FRAGMENT_SHADER, rectFragmentSource);
    checkError();

    compositorContext->m_rectShaderProgram = glCreateProgram();
    checkError();

    glAttachShader(compositorContext->m_rectShaderProgram,
                   compositorContext->m_rectVertexShader);
    checkError();
    glAttachShader(compositorContext->m_rectShaderProgram,
                   compositorContext->m_rectFragmentShader);
    checkError();

    glLinkProgram(compositorContext->m_rectShaderProgram);
    checkError();

    return compositorContext;
}

class CanvasSurfaceGL : public CanvasSurface {
public:
    CanvasSurfaceGL(PlatformWindow* wnd, size_t w, size_t h)
    {
        m_window = (PlatformWindow*)wnd;
        m_width = w;
        m_height = h;
        m_imageWidth = m_bufferWidth = m_width = -1;
        m_imageHeight = m_bufferHeight = m_height = -1;
        m_buffer = nullptr;
        m_isEGLImageExternal = false;

#if defined(STARFISH_TIZEN)
        m_tbmSurface = nullptr;
#endif

        attachNativeBuffer(w, h);
        checkError();
        GC_REGISTER_FINALIZER_NO_ORDER(this,
                                       [](void* obj, void* cd) {
                                           CanvasSurfaceGL* s =
                                               (CanvasSurfaceGL*)obj;
                                           s->detachNativeBuffer();
                                       },
                                       NULL, NULL, NULL);
    }

    virtual bool isCanvasSurfaceGL()
    {
        return true;
    }

    virtual void detachNativeBuffer()
    {
        if (!m_window->isClosed())
            m_window->glMakeCurrent();
        if (m_isEGLImageExternal && m_textureFragments.size()) {
#if defined(STARFISH_TIZEN)
            if (!m_window->isClosed())
                g_evasGLAPI->evasglDestroyImage(m_eglImage);

            m_eglImage = nullptr;
            tbm_surface_destroy(m_tbmSurface);
            m_tbmSurface = nullptr;
#endif
        }
        if (m_textureFragments.size()) {
            if (m_isEGLImageExternal) {
            } else {
                free(m_buffer);
            }

            if (!m_window->isClosed()) {
                for (size_t i = 0; i < m_textureFragments.size(); i++) {
                    GLuint id = m_textureFragments[i].textureID;
                    glDeleteTextures(1, &id);
                }
            }
            m_textureFragments.clear();
            m_textureFragmentsFlags.clear();
            m_dirtyAreaTextureFragments.clear();
            g_totalCanvasSurfaceGLSize -=
                m_bufferWidth * m_bufferHeight * sizeof(uint32_t);

            m_buffer = nullptr;

            STARFISH_LOG_INFO("total CanvasSurface size %fMB\n",
                              g_totalCanvasSurfaceGLSize / 1024.f / 1024.f);
        }

        m_isEGLImageExternal = false;
    }

    void attachNativeBuffer(size_t w, size_t h)
    {
        if (m_width != w || m_height != h) {
            detachNativeBuffer();
            m_width = w;
            m_height = h;

            float windowDevicePixelRatio =
                m_window->starFish()->screenInfo().devicePixelRatio;

            m_imageWidth =
                std::max((size_t)1, (size_t)(m_width * windowDevicePixelRatio));
            m_imageHeight = std::max(
                (size_t)1, (size_t)(m_height * windowDevicePixelRatio));

            m_bufferWidth =
                std::max((size_t)1, (size_t)(w * windowDevicePixelRatio));
            m_bufferHeight =
                std::max((size_t)1, (size_t)(h * windowDevicePixelRatio));

            if (g_isSupportExtensionEGLImageExternal &&
                m_bufferWidth <= g_maxTextureSize &&
                m_bufferHeight <= g_maxTextureSize) {
                m_isEGLImageExternal = true;
#if defined(STARFISH_TIZEN)
                m_tbmSurface = tbm_surface_create(m_bufferWidth, m_bufferHeight,
                                                  TBM_FORMAT_ABGR8888);
                tbm_surface_info_s surfaceInfo;
                tbm_surface_map(m_tbmSurface, TBM_SURF_OPTION_WRITE,
                                &surfaceInfo);

                STARFISH_RELEASE_ASSERT(surfaceInfo.num_planes == 1);
                m_bufferStride = surfaceInfo.planes[0].stride;
                m_buffer = nullptr;
                tbm_surface_unmap(m_tbmSurface);
#endif
            } else {
                m_isEGLImageExternal = false;
                m_bufferStride = m_bufferWidth * 4;
                m_buffer = (unsigned char*)malloc(
                    m_bufferWidth * m_bufferHeight * sizeof(uint32_t));
            }

            g_totalCanvasSurfaceGLSize +=
                m_bufferWidth * m_bufferHeight * sizeof(uint32_t);
            STARFISH_LOG_INFO("total CanvasSurface size %fMB\n",
                              g_totalCanvasSurfaceGLSize / 1024.f / 1024.f);

            ensureGenerateTexture();
        }
    }

    virtual void resize(size_t w, size_t h)
    {
        STARFISH_RELEASE_ASSERT(w <= m_bufferWidth);
        STARFISH_RELEASE_ASSERT(h <= m_bufferHeight);

        m_width = w;
        m_height = h;

        m_imageWidth = std::max((size_t)1, m_width);
        m_imageHeight = std::max((size_t)1, m_height);

        STARFISH_RELEASE_ASSERT(m_imageWidth <= m_bufferWidth);
        STARFISH_RELEASE_ASSERT(m_imageHeight <= m_bufferHeight);
    }

    void ensureGenerateTexture()
    {
        m_window->glMakeCurrent();

        STARFISH_RELEASE_ASSERT(m_textureFragments.size() == 0);
        if (m_isEGLImageExternal) {
#if defined(STARFISH_TIZEN)
            CanvasSurfaceTextureInfo::CanvasSurfaceTextureInfoFragment fragment;
            {
                STARFISH_RELEASE_ASSERT(m_tbmSurface);
                STARFISH_RELEASE_ASSERT(m_eglImage == nullptr);
                int eglImgAttr[] = { EVAS_GL_IMAGE_PRESERVED, GL_TRUE, 0 };
                m_eglImage = g_evasGLAPI->evasglCreateImage(
                    EVAS_GL_NATIVE_SURFACE_TIZEN, (void*)(intptr_t)m_tbmSurface,
                    eglImgAttr);
                checkError();
            }
            {
                GLuint textureID;
                glGenTextures(1, &textureID);
                glActiveTexture(GL_TEXTURE0);

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

                g_evasGLAPI->glEvasGLImageTargetTexture2DOES(
                    GL_TEXTURE_EXTERNAL_OES, m_eglImage);
                checkError();

                glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
                checkError();

                fragment.textureID = textureID;
                fragment.srcX = 0;
                fragment.srcY = 0;
                fragment.srcWidth = 1;
                fragment.srcHeight = 1;

                m_textureFragments.push_back(fragment);
                FragmentFlags flags;
                flags.m_isDirty = true;
                m_textureFragmentsFlags.push_back(flags);
                m_dirtyAreaTextureFragments.push_back(
                    Unit::Rect(0, 0, m_bufferWidth, m_bufferHeight));
            }
#endif
            return;
        }

        size_t wTextureCount = ceil((float)m_bufferWidth / g_textureTileSize);
        size_t hTextureCount = ceil((float)m_bufferHeight / g_textureTileSize);

        size_t coveredRowsCount = 0;
        for (size_t y = 0; y < hTextureCount; y++) {
            size_t coveredColsCount = 0;
            for (size_t x = 0; x < wTextureCount; x++) {
                GLuint textureID;

                size_t texureDataX = coveredColsCount;
                size_t texureDataY = coveredRowsCount;
                size_t texureDataWidth =
                    std::min((size_t)g_textureTileSize,
                             m_bufferWidth - coveredColsCount);
                size_t texureDataHeight =
                    std::min((size_t)g_textureTileSize,
                             m_bufferHeight - coveredRowsCount);

                glGenTextures(1, &textureID);
                glActiveTexture(GL_TEXTURE0);
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
                glBindTexture(GL_TEXTURE_2D, 0);
                checkError();

                CanvasSurfaceTextureInfo::CanvasSurfaceTextureInfoFragment
                    fragment;
                fragment.textureID = textureID;
                fragment.srcX = texureDataX / (float)m_bufferWidth;
                fragment.srcY = texureDataY / (float)m_bufferHeight;
                fragment.srcWidth = texureDataWidth / (float)m_bufferWidth;
                fragment.srcHeight = texureDataHeight / (float)m_bufferHeight;

                m_textureFragments.push_back(fragment);
                FragmentFlags flags;
                flags.m_isDirty = true;
                m_textureFragmentsFlags.push_back(flags);
                m_dirtyAreaTextureFragments.push_back(
                    Unit::Rect(0, 0, texureDataWidth, texureDataHeight));

                coveredColsCount += g_textureTileSize;
            }

            coveredRowsCount += g_textureTileSize;
        }
    }

    virtual uint8_t* mapBuffer()
    {
        if (m_isEGLImageExternal) {
            if (m_buffer) {
                return m_buffer;
            }
#if defined(STARFISH_TIZEN)
            tbm_surface_info_s surfaceInfo;
            tbm_surface_map(m_tbmSurface, TBM_SURF_OPTION_WRITE, &surfaceInfo);
            STARFISH_RELEASE_ASSERT(surfaceInfo.num_planes == 1);
            STARFISH_RELEASE_ASSERT(surfaceInfo.planes[0].stride ==
                                    m_bufferStride);
            m_buffer = surfaceInfo.planes[0].ptr;
#endif
            return m_buffer;
        }
        return m_buffer;
    }

    virtual size_t width()
    {
        return m_width;
    }

    virtual size_t height()
    {
        return m_height;
    }

    virtual size_t bufferWidth()
    {
        return m_bufferWidth;
    }

    virtual size_t bufferHeight()
    {
        return m_bufferHeight;
    }

    virtual size_t imageWidth()
    {
        return m_imageWidth;
    }

    virtual size_t imageHeight()
    {
        return m_imageHeight;
    }

    virtual size_t pixelRatio()
    {
        return 1;
    }

    virtual size_t bufferStride()
    {
        return m_bufferStride;
    }

    virtual void clear()
    {
        size_t end = m_bufferStride * m_bufferHeight;
        memset(m_buffer, 0x00, end);
    }

    virtual void unMapBufferAndNotifyUpdateRegion(size_t dirtyX, size_t dirtyY,
                                                  size_t dirtyWidth,
                                                  size_t dirtyHeight) override
    {
        if (m_textureFragments.size() == 0) {
            return;
        }

        if (m_isEGLImageExternal) {
#if defined(STARFISH_TIZEN)
            tbm_surface_unmap(m_tbmSurface);
#endif
            m_buffer = nullptr;
            return;
        }

        if (dirtyWidth && dirtyHeight) {
            m_window->glMakeCurrent();
            size_t wTextureCount =
                ceil((float)m_bufferWidth / g_textureTileSize);
            size_t hTextureCount =
                ceil((float)m_bufferHeight / g_textureTileSize);
            size_t fragmentIndex = 0;

            Unit::Rect dRect(dirtyX, dirtyY, dirtyWidth, dirtyHeight);

            size_t coveredRowsCount = 0;
            for (size_t y = 0; y < hTextureCount; y++) {
                size_t coveredColsCount = 0;
                for (size_t x = 0; x < wTextureCount; x++) {
                    GLuint textureID;

                    size_t texureDataX = coveredColsCount;
                    size_t texureDataY = coveredRowsCount;
                    size_t texureDataWidth =
                        std::min((size_t)g_textureTileSize,
                                 m_bufferWidth - coveredColsCount);
                    size_t texureDataHeight =
                        std::min((size_t)g_textureTileSize,
                                 m_bufferHeight - coveredRowsCount);
                    CanvasSurfaceTextureInfo::CanvasSurfaceTextureInfoFragment&
                        fragment = m_textureFragments[fragmentIndex];

                    Unit::Rect tRect(texureDataX, texureDataY, texureDataWidth,
                                     texureDataHeight);

                    if (tRect.intersects(dRect)) {
                        m_textureFragmentsFlags[fragmentIndex].m_isDirty = true;

                        auto left = std::max(tRect.x(), dRect.x());
                        auto right = std::min(tRect.maxX(), dRect.maxX());
                        auto bottom = std::min(tRect.maxY(), dRect.maxY());
                        auto top = std::max(tRect.y(), dRect.y());

                        left -= texureDataX;
                        right -= texureDataX;
                        bottom -= texureDataY;
                        top -= texureDataY;

                        m_dirtyAreaTextureFragments[fragmentIndex].unite(
                            Unit::Rect(left, top, right - left, bottom - top));
                    }

                    fragmentIndex++;
                    coveredColsCount += g_textureTileSize;
                }

                coveredRowsCount += g_textureTileSize;
            }
        }
    }

    virtual CanvasSurfaceTextureInfo textureInfo()
    {
        CanvasSurfaceTextureInfo info;
        info.fragments = std::vector<
            CanvasSurfaceTextureInfo::CanvasSurfaceTextureInfoFragment>(
            m_textureFragments.data(),
            m_textureFragments.data() + m_textureFragments.size());
        return info;
    }

protected:
    friend class CompositorImplGL;
    PlatformWindow* m_window;
    unsigned char* m_buffer;
    size_t m_width;
    size_t m_height;
    size_t m_imageWidth;
    size_t m_imageHeight;
    size_t m_bufferWidth;
    size_t m_bufferHeight;
    size_t m_bufferStride;
    GCAtomicVector<CanvasSurfaceTextureInfo::CanvasSurfaceTextureInfoFragment>
        m_textureFragments;
    struct FragmentFlags {
        bool m_isDirty;
    };
    GCAtomicVector<FragmentFlags> m_textureFragmentsFlags;
    GCAtomicVector<Unit::Rect> m_dirtyAreaTextureFragments;

    bool m_isEGLImageExternal;
#if defined(STARFISH_TIZEN)
    tbm_surface_h m_tbmSurface;
    EvasGLImage m_eglImage;
#endif
};

CanvasSurface* CanvasSurface::create(PlatformWindow* wnd, size_t w, size_t h)
{
    return new CanvasSurfaceGL(wnd, w, h);
}

struct CompositorImplGLState {
    bool matrixStaysInRect;
    SkMatrix matrix;
    float opacity;
    Unit::Color color;
    ClipperLib::Paths clipPaths;
};

class CompositorImplGL : public Compositor {
public:
    void applyDevicePixelRatio()
    {
        m_state.back().matrix.preScale(
            m_starfish->screenInfo().devicePixelRatio,
            m_starfish->screenInfo().devicePixelRatio);
    }

    CompositorImplGL(StarFish* starfish, CompositorContext* compositorContext,
                     void* data)
    {
        INSTALL_PROFILE_TIMER(m_starfish, __PRETTY_FUNCTION__);
        m_starfish = starfish;
        m_compositorContext = compositorContext;
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glViewport(0, 0, starfish->platformWindow()->width(),
                   starfish->platformWindow()->height());

        m_state.push_back(CompositorImplGLState());
        m_state.back().matrixStaysInRect = true;
        m_state.back().matrix = SkMatrix::I();
        m_state.back().opacity = 1;

        glUseProgram(m_compositorContext->m_texShaderProgram);
        auto uScreenPos = glGetUniformLocation(
            m_compositorContext->m_texShaderProgram, "uScreen");

        float uScreen[] = { 2.f / m_starfish->platformWindow()->width(),
                            0.f,
                            0.f,
                            0.f,
                            0.f,
                            -2.f / m_starfish->platformWindow()->height(),
                            0.f,
                            0.f,
                            0.f,
                            0.f,
                            0.f,
                            0.f,
                            -1.f,
                            1.f,
                            0.f,
                            1.f };

        glUniformMatrix4fv(uScreenPos, 1, false, uScreen);
        checkError();

        glUseProgram(m_compositorContext->m_texWithAlphaShaderProgram);
        uScreenPos = glGetUniformLocation(
            m_compositorContext->m_texWithAlphaShaderProgram, "uScreen");

        glUniformMatrix4fv(uScreenPos, 1, false, uScreen);
        checkError();

        if (g_isSupportExtensionEGLImageExternal) {
            glUseProgram(
                m_compositorContext->m_texShaderProgramEGLImageExternal);
            uScreenPos = glGetUniformLocation(
                m_compositorContext->m_texShaderProgramEGLImageExternal,
                "uScreen");

            glUniformMatrix4fv(uScreenPos, 1, false, uScreen);
            checkError();

            glUseProgram(m_compositorContext
                             ->m_texWithAlphaShaderProgramEGLImageExternal);
            uScreenPos = glGetUniformLocation(
                m_compositorContext
                    ->m_texWithAlphaShaderProgramEGLImageExternal,
                "uScreen");

            glUniformMatrix4fv(uScreenPos, 1, false, uScreen);
            checkError();
        }

        glUseProgram(m_compositorContext->m_rectShaderProgram);
        checkError();

        uScreenPos = glGetUniformLocation(
            m_compositorContext->m_rectShaderProgram, "uScreen");
        glUniformMatrix4fv(uScreenPos, 1, false, uScreen);
        checkError();

        glUseProgram(0);

        clip(Unit::Rect(0, 0, m_starfish->platformWindow()->width(),
                        m_starfish->platformWindow()->height()));

        applyDevicePixelRatio();
    }

    ~CompositorImplGL()
    {
        INSTALL_PROFILE_TIMER(m_starfish, __PRETTY_FUNCTION__);
        restore();
        STARFISH_ASSERT(m_state.size() == 0);
    }

    virtual void clearColor(const Unit::Color& clr)
    {
        INSTALL_PROFILE_TIMER(m_starfish, __PRETTY_FUNCTION__);
        glClearColor(clr.R(), clr.G(), clr.B(), clr.A());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                GL_STENCIL_BUFFER_BIT);
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
        m_state.back().matrix.mapPoints(&pt, 1);
        path.emplace_back(pt.x(), pt.y());

        pt = SkPoint::Make(rt.x() + rt.width(), rt.y());
        m_state.back().matrix.mapPoints(&pt, 1);
        path.emplace_back(pt.x(), pt.y());

        pt = SkPoint::Make(rt.x() + rt.width(), rt.y() + rt.height());
        m_state.back().matrix.mapPoints(&pt, 1);
        path.emplace_back(pt.x(), pt.y());

        pt = SkPoint::Make(rt.x(), rt.y() + rt.height());
        m_state.back().matrix.mapPoints(&pt, 1);
        path.emplace_back(pt.x(), pt.y());

        m_state.back().clipPaths.push_back(path);
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
        INSTALL_PROFILE_TIMER(m_starfish, __PRETTY_FUNCTION__);
        float dest[4][2]; // 0(LT) 1(LB) 2(RT) 3(RB)

        SkPoint pt;
        pt = SkPoint::Make(rt.x(), rt.y());

        m_state.back().matrix.mapPoints(&pt, 1);
        dest[0][0] = pt.x();
        dest[0][1] = pt.y();

        pt = SkPoint::Make(rt.x(), rt.maxY());
        m_state.back().matrix.mapPoints(&pt, 1);
        dest[1][0] = pt.x();
        dest[1][1] = pt.y();

        pt = SkPoint::Make(rt.maxX(), rt.y());
        m_state.back().matrix.mapPoints(&pt, 1);
        dest[2][0] = pt.x();
        dest[2][1] = pt.y();

        pt = SkPoint::Make(rt.maxX(), rt.maxY());
        m_state.back().matrix.mapPoints(&pt, 1);
        dest[3][0] = pt.x();
        dest[3][1] = pt.y();

        auto currentColor = m_state.back().color;

        if (m_state.back().clipPaths.size()) {
            ClipperLib::Paths result = computeClippath(dest);
            if (result.size()) {
                if (m_state.back().matrixStaysInRect && result.size() == 1 &&
                    result[0].size() == 4) {
                    glUseProgram(m_compositorContext->m_rectShaderProgram);

                    auto aPosition = glGetAttribLocation(
                        m_compositorContext->m_rectShaderProgram, "aPosition");

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

                    float data[] = { minX, minY, minX, maxY,
                                     maxX, minY, maxX, maxY };

                    glVertexAttribPointer(aPosition, 2, GL_FLOAT, false, 0,
                                          &data[0]);
                    glEnableVertexAttribArray(aPosition);

                    auto uColor = glGetUniformLocation(
                        m_compositorContext->m_rectShaderProgram, "uColor");
                    float a = m_state.back().opacity;

                    glUniform4f(uColor, a * currentColor.R(),
                                a * currentColor.G(), a * currentColor.B(),
                                a * currentColor.A());

                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                    checkError();

                    glUseProgram(0);
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
                        glUseProgram(m_compositorContext->m_rectShaderProgram);
                        auto aPosition = glGetAttribLocation(
                            m_compositorContext->m_rectShaderProgram,
                            "aPosition");

                        glVertexAttribPointer(aPosition, 2, GL_FLOAT, false, 0,
                                              trianglePoints);
                        glEnableVertexAttribArray(aPosition);

                        auto uColor = glGetUniformLocation(
                            m_compositorContext->m_rectShaderProgram, "uColor");
                        float a = m_state.back().opacity;

                        glUniform4f(uColor, a * currentColor.R(),

                                    a * currentColor.G(), a * currentColor.B(),
                                    a * currentColor.A());

                        glDrawArrays(GL_TRIANGLES, 0, 3);
                        checkError();

                        glUseProgram(0);
                    }
                }
            }
        } else {
            float data[] = {
                dest[0][0], dest[0][1], // V1
                dest[1][0], dest[1][1], // V2
                dest[2][0], dest[2][1], // V3
                dest[3][0], dest[3][1]  // V4
            };

            glUseProgram(m_compositorContext->m_rectShaderProgram);

            auto aPosition = glGetAttribLocation(
                m_compositorContext->m_rectShaderProgram, "aPosition");

            glVertexAttribPointer(aPosition, 2, GL_FLOAT, false, 0, &data[0]);
            glEnableVertexAttribArray(aPosition);

            auto uColor = glGetUniformLocation(
                m_compositorContext->m_rectShaderProgram, "uColor");
            float a = m_state.back().opacity;

            glUniform4f(uColor, a * currentColor.R(), a * currentColor.G(),
                        a * currentColor.B(), a * currentColor.A());

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            checkError();

            glUseProgram(0);
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
        texture.emplace_back(dest[0][0], dest[0][1]);
        texture.emplace_back(dest[2][0], dest[2][1]);
        texture.emplace_back(dest[3][0], dest[3][1]);
        texture.emplace_back(dest[1][0], dest[1][1]);

        // clipping debug code
        /*
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

        clipper.AddPath(texture, ClipperLib::PolyType::ptSubject, true);
        clipper.AddPath(m_state.back().clipPaths[0],
                        ClipperLib::PolyType::ptClip, true);

        ClipperLib::Paths result;
        clipper.Execute(ClipperLib::ClipType::ctIntersection, result);

        for (size_t i = 1; i < m_state.back().clipPaths.size(); i++) {
            clipper.Clear();
            clipper.AddPaths(result, ClipperLib::PolyType::ptSubject, true);
            clipper.AddPath(m_state.back().clipPaths[i],
                            ClipperLib::PolyType::ptClip, true);

            ClipperLib::Paths newResult;
            clipper.Execute(ClipperLib::ClipType::ctIntersection, newResult);
            result = newResult;
        }

        return result;
    }

    void drawTexture(CanvasSurface* cs, float dest[4][2], GLuint textureID)
    {
        float data[] = { dest[0][0], dest[0][1], // V1
                         0.f,        0.f,        // Texture coordinate .for V1

                         dest[1][0], dest[1][1], // V2
                         0.f,        1.f,

                         dest[2][0], dest[2][1], // V3
                         1.f,        0.f,

                         dest[3][0], dest[3][1], // V4
                         1.f,        1.f };
        float a = m_state.back().opacity;
        if (a == 1) {
            glUseProgram(m_compositorContext->m_texShaderProgram);
            auto aPosition = glGetAttribLocation(
                m_compositorContext->m_texShaderProgram, "aPosition");
            auto aTexPos = glGetAttribLocation(
                m_compositorContext->m_texShaderProgram, "aTexPos");

            glVertexAttribPointer(aPosition, 2, GL_FLOAT, false, (2 + 2) * 4,
                                  &data[0]);
            glEnableVertexAttribArray(aPosition);

            glVertexAttribPointer(aTexPos, 2, GL_FLOAT, false, (2 + 2) * 4,
                                  &data[2]);
            glEnableVertexAttribArray(aTexPos);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureID);
            auto uTexture = glGetUniformLocation(
                m_compositorContext->m_texShaderProgram, "uTexture");
            glUniform1i(uTexture, 0);

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glBindTexture(GL_TEXTURE_2D, 0);
            checkError();
            glUseProgram(0);
        } else {
            glUseProgram(m_compositorContext->m_texWithAlphaShaderProgram);
            auto aPosition = glGetAttribLocation(
                m_compositorContext->m_texWithAlphaShaderProgram, "aPosition");
            auto aTexPos = glGetAttribLocation(
                m_compositorContext->m_texWithAlphaShaderProgram, "aTexPos");

            glVertexAttribPointer(aPosition, 2, GL_FLOAT, false, (2 + 2) * 4,
                                  &data[0]);
            glEnableVertexAttribArray(aPosition);

            glVertexAttribPointer(aTexPos, 2, GL_FLOAT, false, (2 + 2) * 4,
                                  &data[2]);
            glEnableVertexAttribArray(aTexPos);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureID);
            auto uTexture = glGetUniformLocation(
                m_compositorContext->m_texWithAlphaShaderProgram, "uTexture");
            auto uAlpha = glGetUniformLocation(
                m_compositorContext->m_texWithAlphaShaderProgram, "uAlpha");
            glUniform4f(uAlpha, a, a, a, a);
            glUniform1i(uTexture, 0);

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glBindTexture(GL_TEXTURE_2D, 0);
            checkError();
            glUseProgram(0);
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
        if (!cs->isCanvasSurfaceGL()) {
            return;
        }

        INSTALL_PROFILE_TIMER(m_starfish, __PRETTY_FUNCTION__);

        auto textureInfo = cs->textureInfo();
        if (textureInfo.fragments.size() == 0) {
            return;
        }

        float dest[4][2]; // 0(LT) 1(LB) 2(RT) 3(RB)

        CanvasSurfaceGL* csGL = (CanvasSurfaceGL*)cs;

        SkPoint pt;
        pt = SkPoint::Make(dst.x(), dst.y());

        m_state.back().matrix.mapPoints(&pt, 1);
        dest[0][0] = pt.x();
        dest[0][1] = pt.y();

        pt = SkPoint::Make(dst.x(), dst.maxY());
        m_state.back().matrix.mapPoints(&pt, 1);
        dest[1][0] = pt.x();
        dest[1][1] = pt.y();

        pt = SkPoint::Make(dst.maxX(), dst.y());
        m_state.back().matrix.mapPoints(&pt, 1);
        dest[2][0] = pt.x();
        dest[2][1] = pt.y();

        pt = SkPoint::Make(dst.maxX(), dst.maxY());
        m_state.back().matrix.mapPoints(&pt, 1);
        dest[3][0] = pt.x();
        dest[3][1] = pt.y();

        bool stencilClippingEnabled = false;
        bool scissorClippingEnabled = false;
        bool shouldSkipTexturePainting = false;
        Unit::Rect visibleArea =
            Unit::Rect(0, 0, m_starfish->platformWindow()->width(),
                       m_starfish->platformWindow()->height());

        if (m_state.back().clipPaths.size()) {
            visibleArea = Unit::Rect(0, 0, 0, 0);
            ClipperLib::Paths result = computeClippath(dest);
            if (result.size()) {
                if (m_state.back().matrixStaysInRect && result.size() == 1 &&
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
                    glScissor(minX,
                              m_starfish->platformWindow()->height() - maxY,
                              maxX - minX, maxY - minY);
                } else {
                    stencilClippingEnabled = true;

                    glEnable(GL_STENCIL_TEST);
                    glClearStencil(0);
                    glClear(GL_STENCIL_BUFFER_BIT);
                    glColorMask(false, false, false, false);
                    glStencilFunc(GL_ALWAYS, 1, 1);
                    glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

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

                        visibleArea.unite(boundingRect(result[i]));
                    }

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
                        glUseProgram(m_compositorContext->m_rectShaderProgram);
                        auto aPosition = glGetAttribLocation(
                            m_compositorContext->m_rectShaderProgram,
                            "aPosition");

                        glVertexAttribPointer(aPosition, 2, GL_FLOAT, false, 0,
                                              trianglePoints);
                        glEnableVertexAttribArray(aPosition);

                        auto uColor = glGetUniformLocation(
                            m_compositorContext->m_rectShaderProgram, "uColor");
                        float a = 1;
                        glUniform4f(uColor,
                                    a * Unit::Color(255, 255, 255, 255).R(),
                                    a * Unit::Color(255, 255, 255, 255).G(),
                                    a * Unit::Color(255, 255, 255, 255).B(),
                                    a * Unit::Color(255, 255, 255, 255).A());
                        glDrawArrays(GL_TRIANGLES, 0, 3);
                        checkError();

                        glUseProgram(0);
                    }

                    glColorMask(true, true, true, true);
                    glStencilFunc(GL_EQUAL, 1, 1);
                    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
                }
            } else {
                shouldSkipTexturePainting = true;
            }
        }

        if (!shouldSkipTexturePainting) {
            if (csGL->m_isEGLImageExternal) {
                float data[] = { dest[0][0], dest[0][1], // V1
                                 0.f,        0.f, // Texture coordinate .for V1

                                 dest[1][0], dest[1][1], // V2
                                 0.f,        1.f,

                                 dest[2][0], dest[2][1], // V3
                                 1.f,        0.f,

                                 dest[3][0], dest[3][1], // V4
                                 1.f,        1.f };
                float a = m_state.back().opacity;
                if (a == 1) {
                    glUseProgram(m_compositorContext
                                     ->m_texShaderProgramEGLImageExternal);
                    glBindTexture(GL_TEXTURE_EXTERNAL_OES,
                                  csGL->m_textureFragments[0].textureID);
                    auto aPosition = glGetAttribLocation(
                        m_compositorContext->m_texShaderProgramEGLImageExternal,
                        "aPosition");
                    auto aTexPos = glGetAttribLocation(
                        m_compositorContext->m_texShaderProgramEGLImageExternal,
                        "aTexPos");

                    glVertexAttribPointer(aPosition, 2, GL_FLOAT, false,
                                          (2 + 2) * 4, &data[0]);
                    glEnableVertexAttribArray(aPosition);

                    glVertexAttribPointer(aTexPos, 2, GL_FLOAT, false,
                                          (2 + 2) * 4, &data[2]);
                    glEnableVertexAttribArray(aTexPos);

                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
                    checkError();
                    glUseProgram(0);
                } else {
                    glUseProgram(
                        m_compositorContext
                            ->m_texWithAlphaShaderProgramEGLImageExternal);
                    glBindTexture(GL_TEXTURE_EXTERNAL_OES,
                                  csGL->m_textureFragments[0].textureID);
                    auto aPosition = glGetAttribLocation(
                        m_compositorContext
                            ->m_texWithAlphaShaderProgramEGLImageExternal,
                        "aPosition");
                    auto aTexPos = glGetAttribLocation(
                        m_compositorContext
                            ->m_texWithAlphaShaderProgramEGLImageExternal,
                        "aTexPos");

                    glVertexAttribPointer(aPosition, 2, GL_FLOAT, false,
                                          (2 + 2) * 4, &data[0]);
                    glEnableVertexAttribArray(aPosition);

                    glVertexAttribPointer(aTexPos, 2, GL_FLOAT, false,
                                          (2 + 2) * 4, &data[2]);
                    glEnableVertexAttribArray(aTexPos);

                    auto uAlpha = glGetUniformLocation(
                        m_compositorContext
                            ->m_texWithAlphaShaderProgramEGLImageExternal,
                        "uAlpha");
                    glUniform4f(uAlpha, a, a, a, a);

                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
                    checkError();
                    glUseProgram(0);
                }
            } else {
                size_t wTextureCount =
                    ceil((float)cs->bufferWidth() / g_textureTileSize);
                size_t hTextureCount =
                    ceil((float)cs->bufferHeight() / g_textureTileSize);

                size_t coveredRowsCount = 0;
                size_t i = 0;
                for (size_t y = 0; y < hTextureCount; y++) {
                    size_t coveredColsCount = 0;
                    for (size_t x = 0; x < wTextureCount; x++) {
                        size_t texureDataX = coveredColsCount;
                        size_t texureDataY = coveredRowsCount;
                        size_t texureDataWidth =
                            std::min((size_t)g_textureTileSize,
                                     cs->bufferWidth() - coveredColsCount);
                        size_t texureDataHeight =
                            std::min((size_t)g_textureTileSize,
                                     cs->bufferHeight() - coveredRowsCount);

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

                        m_state.back().matrix.mapPoints(&pt, 1);
                        newDest[0][0] = pt.x();
                        newDest[0][1] = pt.y();

                        pt = SkPoint::Make(newDst.x(), newDst.maxY());
                        m_state.back().matrix.mapPoints(&pt, 1);
                        newDest[1][0] = pt.x();
                        newDest[1][1] = pt.y();

                        pt = SkPoint::Make(newDst.maxX(), newDst.y());
                        m_state.back().matrix.mapPoints(&pt, 1);
                        newDest[2][0] = pt.x();
                        newDest[2][1] = pt.y();

                        pt = SkPoint::Make(newDst.maxX(), newDst.maxY());
                        m_state.back().matrix.mapPoints(&pt, 1);
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

                            if (csGL->m_textureFragmentsFlags[i].m_isDirty) {
                                INSTALL_PROFILE_TIMER(m_starfish,
                                                      "update texture tile..");

                                size_t xx =
                                    csGL->m_dirtyAreaTextureFragments[i].x();
                                size_t xxEnd =
                                    csGL->m_dirtyAreaTextureFragments[i].maxX();
                                size_t yy =
                                    csGL->m_dirtyAreaTextureFragments[i].y();
                                size_t yyEnd =
                                    csGL->m_dirtyAreaTextureFragments[i].maxY();

                                auto bData = csGL->mapBuffer();
                                auto bStride = csGL->bufferStride();

                                glActiveTexture(GL_TEXTURE0);
                                glBindTexture(GL_TEXTURE_2D, tid);
                                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                                checkError();

                                if (g_isSupportPixelStoreiUnpackingOfPixelDataFromMemory) {
                                    glPixelStorei(GL_UNPACK_ROW_LENGTH,
                                                  csGL->bufferWidth());
                                    glPixelStorei(GL_UNPACK_SKIP_PIXELS, xx);
                                    glPixelStorei(GL_UNPACK_SKIP_ROWS, yy);

                                    auto data = bData;
                                    data += (texureDataY * bStride);
                                    data += (texureDataX * 4);
                                    glTexSubImage2D(GL_TEXTURE_2D, 0, xx, yy,
                                                    xxEnd - xx, yyEnd - yy,
                                                    GL_RGBA, GL_UNSIGNED_BYTE,
                                                    data);

                                    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
                                    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
                                    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
                                } else {
                                    for (; yy < yyEnd; yy++) {
                                        auto data = bData;
                                        data += ((yy + texureDataY) * bStride);
                                        data += ((texureDataX + xx) * 4);
                                        glTexSubImage2D(GL_TEXTURE_2D, 0, xx,
                                                        yy, xxEnd - xx, 1,
                                                        GL_RGBA,
                                                        GL_UNSIGNED_BYTE, data);
                                        checkError();
                                    }
                                }

                                glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
                                glBindTexture(GL_TEXTURE_2D, 0);
                                checkError();

                                csGL->m_textureFragmentsFlags[i].m_isDirty =
                                    false;
                                csGL->m_dirtyAreaTextureFragments[i] =
                                    Unit::Rect(0, 0, 0, 0);
                                csGL->unMapBufferAndNotifyUpdateRegion(0, 0, 0,
                                                                       0);
                            }
                            drawTexture(cs, newDest, tid);
                        }

                        i++;
                        coveredColsCount += g_textureTileSize;
                    }

                    coveredRowsCount += g_textureTileSize;
                }
            }
        }

        if (stencilClippingEnabled) {
            glDisable(GL_STENCIL_TEST);
        }
        if (scissorClippingEnabled) {
            glScissor(0, 0, m_starfish->platformWindow()->width(),
                      m_starfish->platformWindow()->height());
            glDisable(GL_SCISSOR_TEST);
        }
    }

    virtual void postMatrix(const SkMatrix& matrix)
    {
        m_state.back().matrix.preConcat(matrix);

        if (!m_state.back().matrix.rectStaysRect()) {
            m_state.back().matrixStaysInRect = false;
        }
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
        m_state.back().matrix = SkMatrix::I();
        m_state.back().clipPaths.clear();
        m_state.back().matrixStaysInRect = true;

        clip(Unit::Rect(0, 0, m_starfish->platformWindow()->width(),
                        m_starfish->platformWindow()->height()));
        applyDevicePixelRatio();
    }

    virtual void resetClip()
    {
        m_state.back().clipPaths.clear();
        clip(Unit::Rect(0, 0, m_starfish->platformWindow()->width(),
                        m_starfish->platformWindow()->height()));
    }

protected:
    StarFish* m_starfish;
    CompositorContext* m_compositorContext;
    std::vector<CompositorImplGLState> m_state;
};

Compositor* Compositor::create(StarFish* starfish, CompositorContext* ctx,
                               void* data)
{
    return new CompositorImplGL(starfish, ctx, data);
}

Compositor* Compositor::create(StarFish* starfish, CompositorContext* ctx,
                               CanvasSurface* surface)
{
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

#if defined(STARFISH_ENABLE_TEST)
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
    for (int y = 0; y < deviceHeight; y++) {
        for (int x = 0; x < deviceWidth; x++) {
            uint8_t* head = &buffer[rowLength * y + x * 4];
            std::swap(head[0], head[2]);
        }
    }

    // flip W
    /*
        for (int y = 0; y < deviceHeight; y++) {
            uint32_t* head = (uint32_t*)&buffer[rowLength * y];
            for (int x = 0; x < deviceWidth / 2; x++) {
                std::swap(head[x], head[deviceWidth - x - 1]);
            }
        }
    */
    // flip H
    for (int y = 0; y < deviceHeight / 2; y++) {
        uint32_t* head = (uint32_t*)&buffer[rowLength * y];
        uint32_t* head2 =
            (uint32_t*)&buffer[rowLength * (deviceHeight - y - 1)];
        for (int x = 0; x < deviceWidth; x++) {
            std::swap(head[x], head2[x]);
        }
    }

    cairo_surface_t* png_buffer;
    png_buffer = cairo_image_surface_create_for_data(
        (unsigned char*)buffer, CAIRO_FORMAT_ARGB32, deviceWidth, deviceHeight,
        rowLength);

    cairo_surface_write_to_png(png_buffer, path);
    cairo_surface_destroy(png_buffer);

    delete buffer;
    callback();
}
#endif

} // namespace StarFish

#endif
