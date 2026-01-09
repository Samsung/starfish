/*
 * Copyright (c) 2025-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#include "StarfishConfig.h"
#include "core/dom/canvas/webgl/WebGL2RenderingContext.h"
#include "binding/generated/ArrayBufferOrSharedArrayBufferOrArrayBufferViewUnion.h"
#include "binding/generated/ImageBitmapOrImageDataOrHTMLImageElementOrHTMLCanvasElementOrHTMLVideoElementUnion.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/canvas/webgl/WebGLProgram.h"
#include "core/util/debug/Trace.h"
#include "platform/canvas/gl/GL.h"
#include "platform/canvas/gl/IncludeGL.h"

/* WebGL-specific enums */
static constexpr GLenum kMAX_CLIENT_WAIT_TIMEOUT_WEBGL = 0x9247;

/* WebGL constants */
static constexpr GLint64 kMaxClientWaitTimeoutWebgl = 0;
static constexpr char kShadingLanguageVersion[] = "WebGL GLSL ES 3.00";
static constexpr char kVersion[] = "WebGL 2.0";

namespace Starfish {

WebGLQuery::WebGLQuery(ScriptBindingInstance* instance,
                       WebGLRenderingContext* context, GLuint object)
    : WebGLObject(instance, context, object)
{
}

WebGLSampler::WebGLSampler(ScriptBindingInstance* instance,
                           WebGLRenderingContext* context, GLuint object)
    : WebGLObject(instance, context, object)
{
}

WebGLSync::WebGLSync(ScriptBindingInstance* instance,
                     WebGLRenderingContext* context, GLsync object)
    : WebGLObject(instance, context, /* dummy */ 0)
    , m_glObject(object)
{
}

WebGLTransformFeedback::WebGLTransformFeedback(ScriptBindingInstance* instance,
                                               WebGLRenderingContext* context,
                                               GLuint object)
    : WebGLObject(instance, context, object)
{
}

WebGLVertexArrayObject::WebGLVertexArrayObject(ScriptBindingInstance* instance,
                                               WebGLRenderingContext* context,
                                               GLuint object)
    : WebGLObject(instance, context, object)
{
}

WebGL2RenderingContext::WebGL2RenderingContext(HTMLCanvasElement* canvasElement)
    : WebGLRenderingContext(canvasElement)
{
}

WebGL2RenderingContext::~WebGL2RenderingContext()
{
}

ScriptBindingInstance* WebGL2RenderingContext::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

// WebGLRenderingContextBase

#define ENTER_CONTEXT_SCOPE_IMPL(bailoutValue, ...) \
    GLContextScope contextScope_(m_context);        \
    if (contextScope_.hasError()) {                 \
        TRACE(WEBGL,                                \
              "\033[33m"                            \
              "GL Context error detected."          \
              "\033[0m");                           \
        return bailoutValue;                        \
    }

#if defined(NDEBUG) and !defined(ENABLE_TRACE)
#define ENTER_CONTEXT_SCOPE(bailoutValue, ...) \
    ENTER_CONTEXT_SCOPE_IMPL(bailoutValue);
#else
#define ENTER_CONTEXT_SCOPE(bailoutValue, ...)       \
    ENTER_CONTEXT_SCOPE_IMPL(bailoutValue);          \
    auto onScopeLeave = OnScopeLeave::create([&]() { \
        if (hasGLError()) {                          \
            TRACE(WEBGL,                             \
                  "\033[33m"                         \
                  "GL error detected."               \
                  "\033[0m");                        \
        }                                            \
    });
#endif

ScriptValue WebGL2RenderingContext::getParameter(GLenum pname)
{
    {
        ENTER_CONTEXT_SCOPE(scriptNull());

        switch (pname) {
        // DOMString
        case GL_SHADING_LANGUAGE_VERSION:
            return createScriptValue(
                createScriptASCIIString(kShadingLanguageVersion));
        case GL_VERSION:
            return createScriptValue(createScriptASCIIString(kVersion));
        // GLboolean
        case GL_RASTERIZER_DISCARD:
        case GL_TRANSFORM_FEEDBACK_ACTIVE:
        case GL_TRANSFORM_FEEDBACK_PAUSED: {
            std::vector<GLboolean> values(1);
            gl()->getBooleanv(pname, &values[0]);
            return createScriptValue(static_cast<bool>(values[0]));
        }
        // GLfloat
        case GL_MAX_TEXTURE_LOD_BIAS: {
            std::vector<GLfloat> values(1);
            gl()->getFloatv(pname, &values[0]);
            return createScriptValue(values[0]);
        }
        // GLint64
        case kMAX_CLIENT_WAIT_TIMEOUT_WEBGL:
            return createScriptValue(kMaxClientWaitTimeoutWebgl);
        case GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS:
        case GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS:
        case GL_MAX_ELEMENT_INDEX:
        case GL_MAX_SERVER_WAIT_TIMEOUT:
        case GL_MAX_UNIFORM_BLOCK_SIZE: {
            std::vector<GLint64> values(1);
            gl()->getInteger64v(pname, &values[0]);
            return createScriptValue(values[0]);
        }
        }
    }
    return WebGLRenderingContext::getParameter(pname);
}

// WebGL2RenderingContextBase

GLint WebGL2RenderingContext::getFragDataLocation(WebGLProgram* program,
                                                  String* name)
{
    ENTER_CONTEXT_SCOPE(-1);

    if (program->context() != this) {
        setGLError(GL_INVALID_OPERATION);
        return -1;
    }
    return gl()->getFragDataLocation(program->glObject(), CSTR(name));
}

Optional<WebGLSync*> WebGL2RenderingContext::fenceSync(GLenum condition,
                                                       GLbitfield flags)
{
    ENTER_CONTEXT_SCOPE(Optional<WebGLSync*>());

    GLsync sync = gl()->fenceSync(condition, flags);
    return new WebGLSync(scriptBindingInstance(), this, sync);
}

GLboolean WebGL2RenderingContext::isSync(Optional<WebGLSync*> sync)
{
    ENTER_CONTEXT_SCOPE(false);

    if (!sync.hasValue() || sync.value()->context() != this ||
        sync.value()->invalidated()) {
        return false;
    }
    return gl()->isSync(sync.value()->glObject());
}

void WebGL2RenderingContext::deleteSync(Optional<WebGLSync*> sync)
{
    ENTER_CONTEXT_SCOPE();

    if (!sync.hasValue()) {
        return;
    }
    WebGLSync* value = sync.value();
    if (value->context() != this) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }
    if (value->isDeleted()) {
        return;
    }
    value->markDeleted();
    gl()->deleteSync(value->glObject());
}

GLenum WebGL2RenderingContext::clientWaitSync(WebGLSync* sync, GLbitfield flags,
                                              GLuint64 timeout)
{
    ENTER_CONTEXT_SCOPE(GL_WAIT_FAILED);

    if (sync->context() != this || flags & ~GL_SYNC_FLUSH_COMMANDS_BIT ||
        timeout > kMaxClientWaitTimeoutWebgl) {
        setGLError(GL_INVALID_OPERATION);
        return GL_WAIT_FAILED;
    }
    return gl()->clientWaitSync(sync->glObject(), flags, timeout);
}

void WebGL2RenderingContext::waitSync(WebGLSync* sync, GLbitfield flags,
                                      GLint64 timeout)
{
    ENTER_CONTEXT_SCOPE();

    if (sync->context() != this) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }
    gl()->waitSync(sync->glObject(), flags, timeout);
}

ScriptValue WebGL2RenderingContext::getSyncParameter(WebGLSync* sync,
                                                     GLenum pname)
{
    ENTER_CONTEXT_SCOPE(scriptNull());

    if (sync->context() != this) {
        setGLError(GL_INVALID_OPERATION);
        return scriptNull();
    }
    switch (pname) {
    // GLbitfield
    case GL_SYNC_FLAGS: {
        GLint value;
        gl()->getSynciv(sync->glObject(), pname, 1, nullptr, &value);
        if (hasGLError()) {
            return scriptNull();
        }
        return createScriptValue(static_cast<GLbitfield>(value));
    }
    // GLenum
    case GL_OBJECT_TYPE:
    case GL_SYNC_STATUS:
    case GL_SYNC_CONDITION: {
        GLint value;
        gl()->getSynciv(sync->glObject(), pname, 1, nullptr, &value);
        if (hasGLError()) {
            return scriptNull();
        }
        return createScriptValue(static_cast<GLenum>(value));
    }
    }
    setGLError(GL_INVALID_ENUM);
    return scriptNull();
}

// WebGL2RenderingContextOverloads

void WebGL2RenderingContext::bufferData(GLenum target, GLsizeiptr size,
                                        GLenum usage)
{
    WebGLRenderingContext::bufferData(target, size, usage);
}

void WebGL2RenderingContext::bufferData(
    GLenum target, Optional<AllowSharedBufferSource> srcData, GLenum usage)
{
    WebGLRenderingContext::bufferData(target, srcData, usage);
}

void WebGL2RenderingContext::bufferSubData(GLenum target,
                                           GLintptr dstByteOffset,
                                           AllowSharedBufferSource srcData)
{
    WebGLRenderingContext::bufferSubData(target, dstByteOffset, srcData);
}

void WebGL2RenderingContext::texImage2D(GLenum target, GLint level,
                                        GLint internalformat, GLsizei width,
                                        GLsizei height, GLint border,
                                        GLenum format, GLenum type,
                                        Optional<ScriptArrayBufferView> pixels)
{
    WebGLRenderingContext::texImage2D(target, level, internalformat, width,
                                      height, border, format, type, pixels);
}

void WebGL2RenderingContext::texImage2D(GLenum target, GLint level,
                                        GLint internalformat, GLenum format,
                                        GLenum type, TexImageSource source)
{
    WebGLRenderingContext::texImage2D(target, level, internalformat, format,
                                      type, source);
}

void WebGL2RenderingContext::texSubImage2D(
    GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width,
    GLsizei height, GLenum format, GLenum type,
    Optional<ScriptArrayBufferView> pixels)
{
    WebGLRenderingContext::texSubImage2D(target, level, xoffset, yoffset, width,
                                         height, format, type, pixels);
}

void WebGL2RenderingContext::texSubImage2D(GLenum target, GLint level,
                                           GLint xoffset, GLint yoffset,
                                           GLenum format, GLenum type,
                                           TexImageSource source)
{
    WebGLRenderingContext::texSubImage2D(target, level, xoffset, yoffset,
                                         format, type, source);
}

void WebGL2RenderingContext::readPixels(GLint x, GLint y, GLsizei width,
                                        GLsizei height, GLenum format,
                                        GLenum type,
                                        Optional<ScriptArrayBufferView> dstData)
{
    WebGLRenderingContext::readPixels(x, y, width, height, format, type,
                                      dstData);
}

} // namespace Starfish

#endif // defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)
