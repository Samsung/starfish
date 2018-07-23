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

#include "StarFishConfig.h"
#include "StarFish.h"

#if defined(PORT_COMPOSITOR_BACKEND_GL)

#include "core/modules/canvas/Compositor.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/image/NativeImageData.h"
#include "platform/window/PlatformWindow.h"

#include <vector>
#include <SkMatrix.h>

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
    SkMatrix matrix;
    float opacity;
    Unit::Color color;
};

class CompositorImplGL : public Compositor {
#if defined(PORT_WINDOW_BACKEND_EFL)
    Evas_GL_API* g_evasGLAPI;
#endif
public:
    GLuint texShaderProgram;
    GLuint texVertexShader;
    GLuint texFragmentShader;

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
        m_state.back().matrix = SkMatrix::I();
        m_state.back().opacity = 1;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        GLchar texVertexSource[] =
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

        GLchar texFragmentSource[] =
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

        GLchar rectVertexSource[] =
            "uniform mat4 uScreen;\n"
            "attribute vec2 aTexPos;\n"
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
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    // state
    virtual void save()
    {
        auto s = m_state.back();
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
        glUseProgram(rectShaderProgram);

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

        auto aPosition = glGetAttribLocation(rectShaderProgram, "aPosition");

        float data[] = {
            dest[0][0], dest[0][1], // V1
            dest[1][0], dest[1][1], // V2
            dest[2][0], dest[2][1], // V3
            dest[3][0], dest[3][1]  // V4
        };

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
            STARFISH_ASSERT_NOT_REACHED();
        }
#endif
    }

    virtual void drawSurface(CanvasSurface* cs, const Unit::Rect& dst)
    {
        glUseProgram(texShaderProgram);

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

        GLuint tid = (GLuint)(size_t)cs->unwrap();

        auto aPosition = glGetAttribLocation(texShaderProgram, "aPosition");
        auto aTexPos = glGetAttribLocation(texShaderProgram, "aTexPos");

        float data[] = { dest[0][0], dest[0][1], // V1
                         0.f,        0.f,        // Texture coordinate .for V1

                         dest[1][0], dest[1][1], // V2
                         0.f,        1.f,

                         dest[2][0], dest[2][1], // V3
                         1.f,        0.f,

                         dest[3][0], dest[3][1], // V4
                         1.f,        1.f };

        glVertexAttribPointer(aPosition, 2, GL_FLOAT, false, (2 + 2) * 4,
                              &data[0]);
        glEnableVertexAttribArray(aPosition);

        glVertexAttribPointer(aTexPos, 2, GL_FLOAT, false, (2 + 2) * 4,
                              &data[2]);
        glEnableVertexAttribArray(aTexPos);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tid);
        auto uTexture = glGetUniformLocation(texShaderProgram, "uTexture");
        auto uAlpha = glGetUniformLocation(texShaderProgram, "uAlpha");
        float a = m_state.back().opacity;
        glUniform1f(uAlpha, a);
        glUniform1i(uTexture, 0);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindTexture(GL_TEXTURE_2D, 0);
        checkError();
    }

    virtual void postMatrix(const SkMatrix& matrix)
    {
        m_state.back().matrix.preConcat(matrix);
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
    }

    virtual void resetClip()
    {
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
