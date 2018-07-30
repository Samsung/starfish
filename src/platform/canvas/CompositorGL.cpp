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

#include <vector>
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

#if defined(PORT_WINDOW_BACKEND_EFL)
#include <Evas_GL.h>
#else
#include <GLES2/gl2.h>
#endif

#if defined(PORT_WINDOW_BACKEND_EFL)
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
#endif

namespace StarFish {

struct CompositorImplGLState {
    bool matrixStaysInRect;
    SkMatrix matrix;
    float opacity;
    Unit::Color color;
    ClipperLib::Paths clipPaths;
};

class CompositorImplGL : public Compositor {
#if defined(PORT_WINDOW_BACKEND_EFL)
    Evas_GL_API* g_evasGLAPI;
#endif
public:
    GLuint texShaderProgram;
    GLuint texVertexShader;
    GLuint texFragmentShader;

    GLuint texWithAlphaShaderProgram;
    GLuint texWithAlphaVertexShader;
    GLuint texWithAlphaFragmentShader;

    GLuint rectShaderProgram;
    GLuint rectVertexShader;
    GLuint rectFragmentShader;

    GLuint loadShader(GLenum type, const GLchar* shaderSrc)
    {
        GLuint shader;
        GLint compiled;

        // Create the shader object
        shader = glCreateShader(type);

        if (shader == 0) {
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

    CompositorImplGL(StarFish* starfish, void* data)
    {
        m_starfish = starfish;
#if defined(PORT_WINDOW_BACKEND_EFL)
        struct dummy {
            Evas_GL_API* evasGLAPI;
        };
        dummy* d = (dummy*)data;
        g_evasGLAPI = d->evasGLAPI;
#endif

        glEnable(GL_BLEND);
        glEnable(GL_TEXTURE_2D);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glViewport(0, 0, starfish->platformWindow()->width(),
                   starfish->platformWindow()->height());

        m_state.push_back(CompositorImplGLState());
        m_state.back().matrixStaysInRect = true;
        m_state.back().matrix = SkMatrix::I();
        m_state.back().opacity = 1;

        GLchar texVertexSource[] =
            "uniform mat4 uScreen;\n"
            "attribute vec2 aPosition;\n"
            "attribute vec2 aTexPos;\n"
            "varying vec2 vTexPos;\n"
            "void main() {\n"
            "  vTexPos = aTexPos;\n"
            "  gl_Position = uScreen * vec4(aPosition.xy, 0.0, 1.0);\n"
            "}";

        GLchar texFragmentSource[] =
            "precision mediump float;\n"
            "uniform sampler2D uTexture;\n"
            "varying vec2 vTexPos;\n"
            "void main(void)\n"
            "{\n"
            "  gl_FragColor = texture2D(uTexture, vTexPos);\n"
            "}";

        texVertexShader = loadShader(GL_VERTEX_SHADER, texVertexSource);
        checkError();
        texFragmentShader = loadShader(GL_FRAGMENT_SHADER, texFragmentSource);
        checkError();

        texShaderProgram = glCreateProgram();
        checkError();

        glAttachShader(texShaderProgram, texVertexShader);
        checkError();
        glAttachShader(texShaderProgram, texFragmentShader);
        checkError();

        glLinkProgram(texShaderProgram);
        checkError();

        glUseProgram(texShaderProgram);
        checkError();

        auto uScreenPos = glGetUniformLocation(texShaderProgram, "uScreen");
        auto uTexture = glGetUniformLocation(texShaderProgram, "uTexture");

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

        GLchar texWithAlphaVertexSource[] =
            "uniform mat4 uScreen;\n"
            "attribute vec2 aPosition;\n"
            "attribute vec2 aTexPos;\n"
            "varying vec2 vTexPos;\n"
            "uniform float uAlpha;\n"
            "varying float vAlpha;\n"
            "void main() {\n"
            "  vTexPos = aTexPos;\n"
            "  vAlpha = uAlpha;\n"
            "  gl_Position = uScreen * vec4(aPosition.xy, 0.0, 1.0);\n"
            "}";

        GLchar texWithAlphaFragmentSource[] =
            "precision mediump float;\n"
            "uniform sampler2D uTexture;\n"
            "varying vec2 vTexPos;\n"
            "varying float vAlpha;\n"
            "void main(void)\n"
            "{\n"
            "  gl_FragColor = texture2D(uTexture, vTexPos);\n"
            "  gl_FragColor.a *= vAlpha;\n"
            "  gl_FragColor.r *= vAlpha;\n"
            "  gl_FragColor.g *= vAlpha;\n"
            "  gl_FragColor.b *= vAlpha;\n"
            "}";

        texWithAlphaVertexShader =
            loadShader(GL_VERTEX_SHADER, texWithAlphaVertexSource);
        checkError();
        texWithAlphaFragmentShader =
            loadShader(GL_FRAGMENT_SHADER, texWithAlphaFragmentSource);
        checkError();

        texWithAlphaShaderProgram = glCreateProgram();
        checkError();

        glAttachShader(texWithAlphaShaderProgram, texWithAlphaVertexShader);
        checkError();
        glAttachShader(texWithAlphaShaderProgram, texWithAlphaFragmentShader);
        checkError();

        glLinkProgram(texWithAlphaShaderProgram);
        checkError();

        glUseProgram(texWithAlphaShaderProgram);
        checkError();

        uScreenPos = glGetUniformLocation(texWithAlphaShaderProgram, "uScreen");
        uTexture = glGetUniformLocation(texWithAlphaShaderProgram, "uTexture");

        glUniformMatrix4fv(uScreenPos, 1, false, uScreen);
        checkError();

        GLchar rectVertexSource[] =
            "uniform mat4 uScreen;\n"
            "attribute vec2 aPosition;\n"
            "uniform float uR;\n"
            "varying float vR;\n"
            "uniform float uG;\n"
            "varying float vG;\n"
            "uniform float uB;\n"
            "varying float vB;\n"
            "uniform float uA;\n"
            "varying float vA;\n"
            "void main() {\n"
            "  vR = uR;\n"
            "  vG = uG;\n"
            "  vB = uB;\n"
            "  vA = uA;\n"
            "  gl_Position = uScreen * vec4(aPosition.xy, 0.0, 1.0);\n"
            "}";

        GLchar rectFragmentSource[] =
            "precision mediump float;\n"
            "varying float vR;\n"
            "varying float vG;\n"
            "varying float vB;\n"
            "varying float vA;\n"
            "void main(void)\n"
            "{\n"
            "  gl_FragColor.a = vA;\n"
            "  gl_FragColor.r = vR;\n"
            "  gl_FragColor.g = vG;\n"
            "  gl_FragColor.b = vB;\n"
            "}";

        rectVertexShader = loadShader(GL_VERTEX_SHADER, rectVertexSource);
        checkError();
        rectFragmentShader = loadShader(GL_FRAGMENT_SHADER, rectFragmentSource);
        checkError();

        rectShaderProgram = glCreateProgram();
        checkError();

        glAttachShader(rectShaderProgram, rectVertexShader);
        checkError();
        glAttachShader(rectShaderProgram, rectFragmentShader);
        checkError();

        glLinkProgram(rectShaderProgram);
        checkError();

        glUseProgram(rectShaderProgram);
        checkError();

        uScreenPos = glGetUniformLocation(rectShaderProgram, "uScreen");
        glUniformMatrix4fv(uScreenPos, 1, false, uScreen);
        checkError();
    }

    ~CompositorImplGL()
    {
        glDeleteProgram(texShaderProgram);
        glDeleteShader(texVertexShader);
        checkError();
        glDeleteShader(texFragmentShader);
        checkError();

        glDeleteProgram(texWithAlphaShaderProgram);
        glDeleteShader(texWithAlphaVertexShader);
        checkError();
        glDeleteShader(texWithAlphaFragmentShader);
        checkError();

        glDeleteProgram(rectShaderProgram);
        glDeleteShader(rectVertexShader);
        checkError();
        glDeleteShader(rectFragmentShader);
        checkError();

        restore();
        STARFISH_ASSERT(m_state.size() == 0);
        glFlush();
    }

    virtual void clearColor(const Unit::Color& clr)
    {
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

        if (m_state.back().clipPaths.size()) {
            ClipperLib::Paths result = computeClippath(dest);
            if (result.size()) {
                if (m_state.back().matrixStaysInRect && result.size() == 1 &&
                    result[0].size() == 4) {
                    glUseProgram(rectShaderProgram);

                    auto aPosition =
                        glGetAttribLocation(rectShaderProgram, "aPosition");

                    float data[] = {
                        (float)result[0][0].X, (float)result[0][0].Y,
                        (float)result[0][1].X, (float)result[0][1].Y,
                        (float)result[0][2].X, (float)result[0][2].Y,
                        (float)result[0][3].X, (float)result[0][3].Y
                    };
                    glVertexAttribPointer(aPosition, 2, GL_FLOAT, false, 0,
                                          &data[0]);
                    glEnableVertexAttribArray(aPosition);

                    auto uA = glGetUniformLocation(rectShaderProgram, "uA");
                    auto uR = glGetUniformLocation(rectShaderProgram, "uR");
                    auto uG = glGetUniformLocation(rectShaderProgram, "uG");
                    auto uB = glGetUniformLocation(rectShaderProgram, "uB");
                    float a = m_state.back().opacity;

                    glUniform1f(uA, a * m_state.back().color.A());
                    glUniform1f(uR, a * m_state.back().color.R());
                    glUniform1f(uG, a * m_state.back().color.G());
                    glUniform1f(uB, a * m_state.back().color.B());

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
                        glUseProgram(rectShaderProgram);
                        auto aPosition =
                            glGetAttribLocation(rectShaderProgram, "aPosition");

                        glVertexAttribPointer(aPosition, 2, GL_FLOAT, false, 0,
                                              trianglePoints);
                        glEnableVertexAttribArray(aPosition);

                        auto uA = glGetUniformLocation(rectShaderProgram, "uA");
                        auto uR = glGetUniformLocation(rectShaderProgram, "uR");
                        auto uG = glGetUniformLocation(rectShaderProgram, "uG");
                        auto uB = glGetUniformLocation(rectShaderProgram, "uB");
                        float a = 1;

                        glUniform1f(uA,
                                    a * Unit::Color(255, 255, 255, 255).A());
                        glUniform1f(uR,
                                    a * Unit::Color(255, 255, 255, 255).R());
                        glUniform1f(uG,
                                    a * Unit::Color(255, 255, 255, 255).G());
                        glUniform1f(uB,
                                    a * Unit::Color(255, 255, 255, 255).B());

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

            glUseProgram(rectShaderProgram);

            auto aPosition =
                glGetAttribLocation(rectShaderProgram, "aPosition");

            glVertexAttribPointer(aPosition, 2, GL_FLOAT, false, 0, &data[0]);
            glEnableVertexAttribArray(aPosition);

            auto uA = glGetUniformLocation(rectShaderProgram, "uA");
            auto uR = glGetUniformLocation(rectShaderProgram, "uR");
            auto uG = glGetUniformLocation(rectShaderProgram, "uG");
            auto uB = glGetUniformLocation(rectShaderProgram, "uB");
            float a = m_state.back().opacity;

            glUniform1f(uA, a * m_state.back().color.A());
            glUniform1f(uR, a * m_state.back().color.R());
            glUniform1f(uG, a * m_state.back().color.G());
            glUniform1f(uB, a * m_state.back().color.B());

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            checkError();

            glUseProgram(0);
        }
    }

    virtual void drawRect(const LayoutRect& rt)
    {
        drawRect(Unit::Rect(rt.x(), rt.y(), rt.width(), rt.height()));
    }

    void checkError()
    {
#ifndef NDEBUG
        auto error = glGetError();
        if (error != 0) {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
#endif
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

    virtual void drawSurface(CanvasSurface* cs, const Unit::Rect& dst)
    {
        INSTALL_PROFILE_TIMER(m_starfish, "CompositorGL::drawSurface");
        float dest[4][2]; // 0(LT) 1(LB) 2(RT) 3(RB)

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
        bool shouldSkipTexturePainting = false;
        if (m_state.back().clipPaths.size()) {
            ClipperLib::Paths result = computeClippath(dest);
            if (result.size()) {
                stencilClippingEnabled = true;

                glEnable(GL_STENCIL_TEST);
                glClearStencil(0);
                glClear(GL_STENCIL_BUFFER_BIT);
                glColorMask(false, false, false, false);
                glDepthMask(false);
                glStencilFunc(GL_ALWAYS, 1, 1);
                glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

                std::vector<std::vector<Point>> polygon;
                std::vector<Point> pointPerIndex;
                for (size_t i = 0; i < result.size(); i++) {
                    polygon.push_back(std::vector<Point>());
                    for (size_t j = 0; j < result[i].size(); j++) {
                        polygon.back().push_back(
                            { (double)result[i][j].X, (double)result[i][j].Y });
                        pointPerIndex.push_back(
                            { (double)result[i][j].X, (double)result[i][j].Y });
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
                    /*
                    printf("trangle %f,%f-%f,%f-%f,%f ", trianglePoints[0],
                    trianglePoints[1],
                            trianglePoints[2], trianglePoints[3],
                            trianglePoints[4], trianglePoints[5]);
                     */
                    glUseProgram(rectShaderProgram);
                    auto aPosition =
                        glGetAttribLocation(rectShaderProgram, "aPosition");

                    glVertexAttribPointer(aPosition, 2, GL_FLOAT, false, 0,
                                          trianglePoints);
                    glEnableVertexAttribArray(aPosition);

                    auto uA = glGetUniformLocation(rectShaderProgram, "uA");
                    auto uR = glGetUniformLocation(rectShaderProgram, "uR");
                    auto uG = glGetUniformLocation(rectShaderProgram, "uG");
                    auto uB = glGetUniformLocation(rectShaderProgram, "uB");
                    float a = 1;

                    glUniform1f(uA, a * Unit::Color(255, 255, 255, 255).A());
                    glUniform1f(uR, a * Unit::Color(255, 255, 255, 255).R());
                    glUniform1f(uG, a * Unit::Color(255, 255, 255, 255).G());
                    glUniform1f(uB, a * Unit::Color(255, 255, 255, 255).B());

                    glDrawArrays(GL_TRIANGLES, 0, 3);
                    checkError();

                    glUseProgram(0);
                }

                glColorMask(true, true, true, true);
                glDepthMask(true);
                glStencilFunc(GL_EQUAL, 1, 1);
                glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            } else {
                shouldSkipTexturePainting = true;
            }
        }

        if (!shouldSkipTexturePainting) {
            GLuint tid = (GLuint)(size_t)cs->unwrap();

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
                glUseProgram(texShaderProgram);
                auto aPosition =
                    glGetAttribLocation(texShaderProgram, "aPosition");
                auto aTexPos = glGetAttribLocation(texShaderProgram, "aTexPos");

                glVertexAttribPointer(aPosition, 2, GL_FLOAT, false,
                                      (2 + 2) * 4, &data[0]);
                glEnableVertexAttribArray(aPosition);

                glVertexAttribPointer(aTexPos, 2, GL_FLOAT, false, (2 + 2) * 4,
                                      &data[2]);
                glEnableVertexAttribArray(aTexPos);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, tid);
                auto uTexture =
                    glGetUniformLocation(texShaderProgram, "uTexture");

                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                glBindTexture(GL_TEXTURE_2D, 0);
                checkError();
                glUseProgram(0);
            } else {
                glUseProgram(texWithAlphaShaderProgram);
                auto aPosition =
                    glGetAttribLocation(texWithAlphaShaderProgram, "aPosition");
                auto aTexPos =
                    glGetAttribLocation(texWithAlphaShaderProgram, "aTexPos");

                glVertexAttribPointer(aPosition, 2, GL_FLOAT, false,
                                      (2 + 2) * 4, &data[0]);
                glEnableVertexAttribArray(aPosition);

                glVertexAttribPointer(aTexPos, 2, GL_FLOAT, false, (2 + 2) * 4,
                                      &data[2]);
                glEnableVertexAttribArray(aTexPos);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, tid);
                auto uTexture =
                    glGetUniformLocation(texWithAlphaShaderProgram, "uTexture");
                auto uAlpha =
                    glGetUniformLocation(texWithAlphaShaderProgram, "uAlpha");
                glUniform1f(uAlpha, a);
                glUniform1i(uTexture, 0);

                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                glBindTexture(GL_TEXTURE_2D, 0);
                checkError();
                glUseProgram(0);
            }
        }

        if (stencilClippingEnabled) {
            glDisable(GL_STENCIL_TEST);
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
    }

    virtual void resetClip()
    {
        m_state.back().clipPaths.clear();
    }

protected:
    StarFish* m_starfish;
    std::vector<CompositorImplGLState> m_state;
};

Compositor* Compositor::create(StarFish* starfish, void* data)
{
    return new CompositorImplGL(starfish, data);
}

Compositor* Compositor::create(StarFish* starfish, CanvasSurface* surface)
{
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

} // namespace StarFish

#endif
