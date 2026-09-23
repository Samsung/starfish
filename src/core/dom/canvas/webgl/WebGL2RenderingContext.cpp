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
#include "binding/generated/Float32ArrayOrSequenceOfGLfloatUnion.h"
#include "binding/generated/ImageBitmapOrImageDataOrHTMLImageElementOrHTMLCanvasElementOrHTMLVideoElementUnion.h"
#include "binding/generated/Int32ArrayOrSequenceOfGLintUnion.h"
#include "binding/generated/Uint32ArrayOrSequenceOfGLuintUnion.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/canvas/webgl/WebGLBuffer.h"
#include "core/dom/canvas/webgl/WebGLFramebuffer.h"
#include "core/dom/canvas/webgl/WebGLProgram.h"
#include "core/dom/canvas/webgl/WebGLShader.h"
#include "core/dom/canvas/webgl/WebGLTexture.h"
#include "core/dom/canvas/webgl/WebGLRenderingContextState.h"
#include "core/dom/canvas/webgl/WebGLUniformLocation.h"
#include "core/dom/canvas/HTMLCanvasElement.h"
#include "core/util/debug/Trace.h"
#include "platform/canvas/gl/GL.h"
#include "platform/canvas/gl/IncludeGL.h"
#include <EscargotPublic.h>
#include <regex>

/* WebGL-specific enums */
static constexpr GLenum kMAX_CLIENT_WAIT_TIMEOUT_WEBGL = 0x9247;

/* WebGL constants */
static constexpr GLint64 kMaxClientWaitTimeoutWebgl = 0;
static constexpr GLsizei kMaximumSupportedStride = 255;
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
        if (hasNewGLError()) {                       \
            TRACE(WEBGL,                             \
                  "\033[33m"                         \
                  "GL error detected."               \
                  "\033[0m");                        \
        }                                            \
    });
#endif

void WebGL2RenderingContext::bindBuffer(GLenum target,
                                        Optional<WebGLBuffer*> buffer)
{
    ENTER_CONTEXT_SCOPE();

    if (target != GL_ARRAY_BUFFER && target != GL_COPY_READ_BUFFER &&
        target != GL_COPY_WRITE_BUFFER && target != GL_ELEMENT_ARRAY_BUFFER &&
        target != GL_PIXEL_PACK_BUFFER && target != GL_PIXEL_UNPACK_BUFFER &&
        target != GL_TRANSFORM_FEEDBACK_BUFFER && target != GL_UNIFORM_BUFFER) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    if (buffer.hasValue()) {
        WebGLBuffer* value = buffer.value();

        if (!isFromCurrentContext(value)) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        if (value->isDeleted()) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        if (value->target() != GL_NONE &&
            ((value->target() == GL_ELEMENT_ARRAY_BUFFER &&
              target != GL_ELEMENT_ARRAY_BUFFER &&
              target != GL_COPY_READ_BUFFER &&
              target != GL_COPY_WRITE_BUFFER) ||
             (value->target() != GL_ELEMENT_ARRAY_BUFFER &&
              target == GL_ELEMENT_ARRAY_BUFFER))) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        gl()->bindBuffer(target, value->glObject());
        if (hasNewGLError()) {
            return;
        }
        getState()->setBoundBuffer(target, value);

        value->setTargetOnce(target);
    } else {
        gl()->bindBuffer(target, 0);
        if (hasNewGLError()) {
            return;
        }
        getState()->setBoundBuffer(target, nullptr);
    }
}

void WebGL2RenderingContext::bindFramebuffer(
    GLenum target, Optional<WebGLFramebuffer*> maybeFramebuffer)
{
    ENTER_CONTEXT_SCOPE();

    if (target != GL_FRAMEBUFFER && target != GL_READ_FRAMEBUFFER &&
        target != GL_DRAW_FRAMEBUFFER) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    if (maybeFramebuffer.hasValue()) {
        WebGLFramebuffer* frameBuffer = maybeFramebuffer.value();

        if (!isFromCurrentContext(frameBuffer)) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        if (frameBuffer->isDeleted()) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        gl()->bindFramebuffer(target, frameBuffer->glObject());
        getState()->setWebGLFramebuffer(frameBuffer);
    } else {
        gl()->bindFramebuffer(target, m_framebufferTexture->fbo());
        getState()->setWebGLFramebuffer(nullptr);
    }
}

void WebGL2RenderingContext::bindTexture(GLenum target,
                                         Optional<WebGLTexture*> maybeTexture)
{
    if (target != GL_TEXTURE_2D && target != GL_TEXTURE_3D &&
        target != GL_TEXTURE_2D_ARRAY && target != GL_TEXTURE_CUBE_MAP) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    WebGLRenderingContext::bindTexture(target, maybeTexture);
}

ScriptValue WebGL2RenderingContext::getBufferParameter(GLenum target,
                                                       GLenum pname)
{
    ENTER_CONTEXT_SCOPE(scriptNull());

    if (target != GL_ARRAY_BUFFER && target != GL_COPY_READ_BUFFER &&
        target != GL_COPY_WRITE_BUFFER && target != GL_ELEMENT_ARRAY_BUFFER &&
        target != GL_PIXEL_PACK_BUFFER && target != GL_PIXEL_UNPACK_BUFFER &&
        target != GL_TRANSFORM_FEEDBACK_BUFFER && target != GL_UNIFORM_BUFFER) {
        setGLError(GL_INVALID_ENUM);
        return scriptNull();
    }

    switch (pname) {
    // GLsizeiptr
    case GL_BUFFER_SIZE: {
        GLint64 value;
        gl()->getBufferParameteri64v(target, pname, &value);
        if (hasNewGLError()) {
            return scriptNull();
        }
        return createScriptValue(static_cast<GLsizeiptr>(value));
    }
    // GLenum
    case GL_BUFFER_USAGE: {
        GLint value;
        gl()->getBufferParameteriv(target, pname, &value);
        if (hasNewGLError()) {
            return scriptNull();
        }
        return createScriptValue(static_cast<GLenum>(value));
    }
    default:
        break;
    }
    setGLError(GL_INVALID_ENUM);
    return scriptNull();
}

Optional<ScriptValue> WebGL2RenderingContext::getParameterImpl(GLenum pname)
{
    ENTER_CONTEXT_SCOPE(Optional<ScriptValue>());

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
    // GLint
    case GL_MAX_3D_TEXTURE_SIZE:
    case GL_MAX_ARRAY_TEXTURE_LAYERS:
    case GL_MAX_COLOR_ATTACHMENTS:
    case GL_MAX_COMBINED_UNIFORM_BLOCKS:
    case GL_MAX_DRAW_BUFFERS:
    case GL_MAX_ELEMENTS_INDICES:
    case GL_MAX_ELEMENTS_VERTICES:
    case GL_MAX_FRAGMENT_INPUT_COMPONENTS:
    case GL_MAX_FRAGMENT_UNIFORM_BLOCKS:
    case GL_MAX_FRAGMENT_UNIFORM_COMPONENTS:
    case GL_MAX_PROGRAM_TEXEL_OFFSET:
    case GL_MAX_SAMPLES:
    case GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS:
    case GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS:
    case GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS:
    case GL_MAX_UNIFORM_BUFFER_BINDINGS:
    case GL_MAX_VARYING_COMPONENTS:
    case GL_MAX_VERTEX_OUTPUT_COMPONENTS:
    case GL_MAX_VERTEX_UNIFORM_BLOCKS:
    case GL_MAX_VERTEX_UNIFORM_COMPONENTS:
    case GL_MIN_PROGRAM_TEXEL_OFFSET:
    case GL_PACK_ROW_LENGTH:
    case GL_PACK_SKIP_PIXELS:
    case GL_PACK_SKIP_ROWS:
    case GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT:
    case GL_UNPACK_IMAGE_HEIGHT:
    case GL_UNPACK_ROW_LENGTH:
    case GL_UNPACK_SKIP_IMAGES:
    case GL_UNPACK_SKIP_PIXELS:
    case GL_UNPACK_SKIP_ROWS: {
        std::vector<GLint> values(1);
        gl()->getIntegerv(pname, &values[0]);
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
    // WebGLVertexArrayObject
    case GL_VERTEX_ARRAY_BINDING: {
        GLint value = -1;
        gl()->getIntegerv(pname, &value);
        if (value == 0) {
            return scriptNull();
        }

        Optional<WebGLVertexArrayObject*> maybe =
            getState()->webGLVertexArrayObject();

        if (!maybe.hasValue() || maybe.value()->isDeleted()) {
            return scriptNull();
        }

        STARFISH_ASSERT(static_cast<GLint>(maybe.value()->glObject()) == value);
        return maybe.value()->scriptValue();
    }
    default:
        break;
    }
    return Optional<ScriptValue>();
}

ScriptValue WebGL2RenderingContext::getParameter(GLenum pname)
{
    Optional<ScriptValue> parameter = getParameterImpl(pname);
    if (parameter.hasValue()) {
        return parameter.value();
    }
    return WebGLRenderingContext::getParameter(pname);
}

ScriptValue WebGL2RenderingContext::getProgramParameter(WebGLProgram* program,
                                                        GLenum pname)
{
    ENTER_CONTEXT_SCOPE(scriptNull());

    if (!isFromCurrentContext(program)) {
        setGLError(GL_INVALID_OPERATION);
        return scriptNull();
    }

    GLint params = 0;
    gl()->getProgramiv(program->glObject(), pname, &params);

    if (hasNewGLError()) {
        return scriptNull();
    }

    switch (pname) {
    // GLboolean
    case GL_DELETE_STATUS:
        return createScriptValue(static_cast<bool>(params));
    case GL_LINK_STATUS:
        if (program->linkFailed()) {
            return createScriptValue(false);
        }
        return createScriptValue(static_cast<bool>(params));
    case GL_VALIDATE_STATUS:
        return createScriptValue(static_cast<bool>(params));
    // GLenum
    case GL_TRANSFORM_FEEDBACK_BUFFER_MODE:
        return createScriptValue(static_cast<GLenum>(params));
    // GLint
    case GL_ACTIVE_ATTRIBUTES:
    case GL_ACTIVE_UNIFORMS:
    case GL_ACTIVE_UNIFORM_BLOCKS:
    case GL_ATTACHED_SHADERS:
    case GL_TRANSFORM_FEEDBACK_VARYINGS:
        return createScriptValue(params);
    default:
        break;
    }
    setGLError(GL_INVALID_ENUM);
    return scriptNull();
}

ScriptValue WebGL2RenderingContext::getTexParameter(GLenum target, GLenum pname)
{
    ENTER_CONTEXT_SCOPE(scriptNull());

    if (target != GL_TEXTURE_2D && target != GL_TEXTURE_3D &&
        target != GL_TEXTURE_2D_ARRAY && target != GL_TEXTURE_CUBE_MAP) {
        setGLError(GL_INVALID_ENUM);
        return scriptNull();
    }

    if (!hasBoundTexture(target)) {
        setGLError(GL_INVALID_OPERATION);
        return scriptNull();
    }

    if (pname == GL_TEXTURE_MAX_ANISOTROPY_EXT &&
        isExtensionEnabled("EXT_texture_filter_anisotropic")) {
        GLfloat params = 0;
        gl()->getTexParameterfv(target, pname, &params);
        if (hasNewGLError()) {
            return scriptNull();
        }
        return createScriptValue(params);
    }

    switch (pname) {
    // GLboolean
    case GL_TEXTURE_IMMUTABLE_FORMAT: {
        WebGLTexture* texture = getBoundTextureObject(target);
        if (texture) {
            return createScriptValue(texture->isImmutable());
        }
        GLint params = 0;
        gl()->getTexParameteriv(target, pname, &params);
        if (hasNewGLError()) {
            return scriptNull();
        }
        return createScriptValue(static_cast<bool>(params));
    }
    // GLenum
    case GL_TEXTURE_COMPARE_FUNC:
    case GL_TEXTURE_COMPARE_MODE:
    case GL_TEXTURE_MAG_FILTER:
    case GL_TEXTURE_MIN_FILTER:
    case GL_TEXTURE_WRAP_R:
    case GL_TEXTURE_WRAP_S:
    case GL_TEXTURE_WRAP_T: {
        GLint params = 0;
        gl()->getTexParameteriv(target, pname, &params);
        if (hasNewGLError()) {
            return scriptNull();
        }
        return createScriptValue(static_cast<GLenum>(params));
    }
    // GLfloat
    case GL_TEXTURE_MAX_LOD:
    case GL_TEXTURE_MIN_LOD: {
        GLfloat params = 0;
        gl()->getTexParameterfv(target, pname, &params);
        if (hasNewGLError()) {
            return scriptNull();
        }
        return createScriptValue(params);
    }
    // GLint
    case GL_TEXTURE_BASE_LEVEL:
    case GL_TEXTURE_MAX_LEVEL: {
        GLint params = 0;
        gl()->getTexParameteriv(target, pname, &params);
        if (hasNewGLError()) {
            return scriptNull();
        }
        return createScriptValue(params);
    }
    // GLuint
    case GL_TEXTURE_IMMUTABLE_LEVELS: {
        GLint params = 0;
        gl()->getTexParameteriv(target, pname, &params);
        if (hasNewGLError()) {
            return scriptNull();
        }
        return createScriptValue(static_cast<GLuint>(params));
    }
    default:
        break;
    }
    setGLError(GL_INVALID_ENUM);
    return scriptNull();
}

Optional<ScriptValue> WebGL2RenderingContext::getUniformImpl(
    WebGLProgram* program, WebGLUniformLocation* location, GLenum type)
{
    ENTER_CONTEXT_SCOPE(Optional<ScriptValue>());

    switch (type) {
    case GL_UNSIGNED_INT: {
        GLuint value;
        gl()->getUniformuiv(program->glObject(), location->location(), &value);
        return createScriptValue(value);
    }
    case GL_UNSIGNED_INT_VEC2: {
        std::vector<GLuint> values(2);
        gl()->getUniformuiv(program->glObject(), location->location(),
                            &values[0]);
        return createScriptValue(
            createTypedArray<Escargot::Uint32ArrayObjectRef>(
                scriptBindingInstance(), values));
    }
    case GL_UNSIGNED_INT_VEC3: {
        std::vector<GLuint> values(3);
        gl()->getUniformuiv(program->glObject(), location->location(),
                            &values[0]);
        return createScriptValue(
            createTypedArray<Escargot::Uint32ArrayObjectRef>(
                scriptBindingInstance(), values));
    }
    case GL_UNSIGNED_INT_VEC4: {
        std::vector<GLuint> values(4);
        gl()->getUniformuiv(program->glObject(), location->location(),
                            &values[0]);
        return createScriptValue(
            createTypedArray<Escargot::Uint32ArrayObjectRef>(
                scriptBindingInstance(), values));
    }
    case GL_FLOAT_MAT2x3: {
        std::vector<GLfloat> values(6);
        gl()->getUniformfv(program->glObject(), location->location(),
                           &values[0]);
        return createScriptValue(
            createTypedArray<Escargot::Float32ArrayObjectRef>(
                scriptBindingInstance(), values));
    }
    case GL_FLOAT_MAT2x4: {
        std::vector<GLfloat> values(8);
        gl()->getUniformfv(program->glObject(), location->location(),
                           &values[0]);
        return createScriptValue(
            createTypedArray<Escargot::Float32ArrayObjectRef>(
                scriptBindingInstance(), values));
    }
    case GL_FLOAT_MAT3x2: {
        std::vector<GLfloat> values(6);
        gl()->getUniformfv(program->glObject(), location->location(),
                           &values[0]);
        return createScriptValue(
            createTypedArray<Escargot::Float32ArrayObjectRef>(
                scriptBindingInstance(), values));
    }
    case GL_FLOAT_MAT3x4: {
        std::vector<GLfloat> values(12);
        gl()->getUniformfv(program->glObject(), location->location(),
                           &values[0]);
        return createScriptValue(
            createTypedArray<Escargot::Float32ArrayObjectRef>(
                scriptBindingInstance(), values));
    }
    case GL_FLOAT_MAT4x2: {
        std::vector<GLfloat> values(8);
        gl()->getUniformfv(program->glObject(), location->location(),
                           &values[0]);
        return createScriptValue(
            createTypedArray<Escargot::Float32ArrayObjectRef>(
                scriptBindingInstance(), values));
    }
    case GL_FLOAT_MAT4x3: {
        std::vector<GLfloat> values(12);
        gl()->getUniformfv(program->glObject(), location->location(),
                           &values[0]);
        return createScriptValue(
            createTypedArray<Escargot::Float32ArrayObjectRef>(
                scriptBindingInstance(), values));
    }
    case GL_SAMPLER_3D:
    case GL_SAMPLER_2D_SHADOW:
    case GL_SAMPLER_2D_ARRAY:
    case GL_SAMPLER_2D_ARRAY_SHADOW:
    case GL_SAMPLER_CUBE_SHADOW:
    case GL_INT_SAMPLER_2D:
    case GL_INT_SAMPLER_3D:
    case GL_INT_SAMPLER_CUBE:
    case GL_INT_SAMPLER_2D_ARRAY:
    case GL_UNSIGNED_INT_SAMPLER_2D:
    case GL_UNSIGNED_INT_SAMPLER_3D:
    case GL_UNSIGNED_INT_SAMPLER_CUBE:
    case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY: {
        GLint value;
        gl()->getUniformiv(program->glObject(), location->location(), &value);
        return createScriptValue(value);
    }
    default:
        break;
    }
    return Optional<ScriptValue>();
}

ScriptValue WebGL2RenderingContext::getUniform(WebGLProgram* program,
                                               WebGLUniformLocation* location)
{
    if (!isFromCurrentContext(program)) {
        setGLError(GL_INVALID_OPERATION);
        return scriptNull();
    }

    if (location->program()->context() != this) {
        setGLError(GL_INVALID_OPERATION);
        return scriptNull();
    }

    GLenum type = getUniformType(program, location);
    Optional<ScriptValue> uniform = getUniformImpl(program, location, type);
    if (uniform.hasValue()) {
        return uniform.value();
    }
    uniform = WebGLRenderingContext::getUniformImpl(program, location, type);
    return uniform.valueOr(scriptNull());
}

ScriptValue WebGL2RenderingContext::getVertexAttrib(GLuint index, GLenum pname)
{
    ENTER_CONTEXT_SCOPE(scriptNull());

    switch (pname) {
    // GLboolean
    case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
    case GL_VERTEX_ATTRIB_ARRAY_INTEGER:
    case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED: {
        GLint value = 0;
        gl()->getVertexAttribiv(index, pname, &value);
        return createScriptValue(static_cast<GLboolean>(value));
    }
    // GLenum
    case GL_VERTEX_ATTRIB_ARRAY_TYPE: {
        GLint value = GL_FLOAT;
        gl()->getVertexAttribiv(index, pname, &value);
        return createScriptValue(static_cast<GLenum>(value));
    }
    // GLint
    case GL_VERTEX_ATTRIB_ARRAY_DIVISOR:
    case GL_VERTEX_ATTRIB_ARRAY_SIZE:
    case GL_VERTEX_ATTRIB_ARRAY_STRIDE: {
        GLint value;
        gl()->getVertexAttribiv(index, pname, &value);
        return createScriptValue(value);
    }
    // One of Float32Array, Int32Array or Uint32Array (each with 4 elements)
    case GL_CURRENT_VERTEX_ATTRIB: {
        switch (m_currentVertexAttribType) {
        case GL_INT: {
            std::vector<int32_t> values(4);
            gl()->getVertexAttribIiv(index, pname, &values[0]);
            return createScriptValue(
                createTypedArray<Escargot::Int32ArrayObjectRef>(
                    scriptBindingInstance(), values));
        }
        case GL_UNSIGNED_INT: {
            std::vector<uint32_t> values(4);
            gl()->getVertexAttribIuiv(index, pname, &values[0]);
            return createScriptValue(
                createTypedArray<Escargot::Uint32ArrayObjectRef>(
                    scriptBindingInstance(), values));
        }
        case GL_FLOAT: {
            std::vector<float> values(4);
            gl()->getVertexAttribfv(index, pname, &values[0]);
            return createScriptValue(
                createTypedArray<Escargot::Float32ArrayObjectRef>(
                    scriptBindingInstance(), values));
        }
        default:
            STARFISH_ASSERT_NOT_REACHED();
            return scriptNull();
        }
    }
    // WebGLBuffer
    case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING: {
        GLint value = 0;
        gl()->getVertexAttribiv(index, pname, &value);

        TRACE(WEBGL, KV(index), KV(value));

        Optional<WebGLVertexArrayObject*> maybe =
            getState()->webGLVertexArrayObject();

        if (!maybe.hasValue()) {
            return scriptNull(); // No mention found for this in the spec.
        }

        Optional<WebGLBuffer*> maybeBuffer =
            getState()->getBufferBoundToVertexAttributes(index);

        if (!maybeBuffer.hasValue()) {
            return scriptNull(); // No mention found for this in the spec.
        }

        TRACE(WEBGL, KV(index), KV(maybeBuffer.value()->glObject()));

        STARFISH_ASSERT(static_cast<GLuint>(value) ==
                        maybeBuffer.value()->glObject());

        return maybeBuffer.value()->scriptValue();
    }
    default:
        break;
    }
    setGLError(GL_INVALID_ENUM);
    return scriptNull();
}

void WebGL2RenderingContext::texParameterf(GLenum target, GLenum pname,
                                           GLfloat param)
{
    ENTER_CONTEXT_SCOPE();

    if (pname == GL_TEXTURE_MAX_ANISOTROPY_EXT) {
        if (!isExtensionEnabled("EXT_texture_filter_anisotropic")) {
            setGLError(GL_INVALID_ENUM);
            return;
        }
    } else if (pname != GL_TEXTURE_BASE_LEVEL &&
               pname != GL_TEXTURE_COMPARE_FUNC &&
               pname != GL_TEXTURE_COMPARE_MODE &&
               pname != GL_TEXTURE_MAG_FILTER &&
               pname != GL_TEXTURE_MAX_LEVEL && pname != GL_TEXTURE_MAX_LOD &&
               pname != GL_TEXTURE_MIN_FILTER && pname != GL_TEXTURE_MIN_LOD &&
               pname != GL_TEXTURE_WRAP_R && pname != GL_TEXTURE_WRAP_S &&
               pname != GL_TEXTURE_WRAP_T) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    if (target != GL_TEXTURE_2D && target != GL_TEXTURE_3D &&
        target != GL_TEXTURE_2D_ARRAY && target != GL_TEXTURE_CUBE_MAP) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    if (!hasBoundTexture(target)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    gl()->texParameterf(target, pname, param);
}

void WebGL2RenderingContext::texParameteri(GLenum target, GLenum pname,
                                           GLint param)
{
    ENTER_CONTEXT_SCOPE();

    if (pname == GL_TEXTURE_MAX_ANISOTROPY_EXT) {
        if (!isExtensionEnabled("EXT_texture_filter_anisotropic")) {
            setGLError(GL_INVALID_ENUM);
            return;
        }
    } else if (pname != GL_TEXTURE_BASE_LEVEL &&
               pname != GL_TEXTURE_COMPARE_FUNC &&
               pname != GL_TEXTURE_COMPARE_MODE &&
               pname != GL_TEXTURE_MAG_FILTER &&
               pname != GL_TEXTURE_MAX_LEVEL && pname != GL_TEXTURE_MAX_LOD &&
               pname != GL_TEXTURE_MIN_FILTER && pname != GL_TEXTURE_MIN_LOD &&
               pname != GL_TEXTURE_WRAP_R && pname != GL_TEXTURE_WRAP_S &&
               pname != GL_TEXTURE_WRAP_T) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    if (target != GL_TEXTURE_2D && target != GL_TEXTURE_3D &&
        target != GL_TEXTURE_2D_ARRAY && target != GL_TEXTURE_CUBE_MAP) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    if (!hasBoundTexture(target)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    gl()->texParameteri(target, pname, param);
}

// WebGL2RenderingContextBase

void WebGL2RenderingContext::copyBufferSubData(GLenum readTarget,
                                               GLenum writeTarget,
                                               GLintptr readOffset,
                                               GLintptr writeOffset,
                                               GLsizeiptr size)
{
    ENTER_CONTEXT_SCOPE();

    GLenum readTargetType = readTarget;
    if (readTargetType == GL_COPY_READ_BUFFER ||
        readTargetType == GL_COPY_WRITE_BUFFER) {
        WebGLBuffer* readBuffer =
            getState()->getBoundBuffer(readTarget).valueOr(nullptr);
        if (readBuffer != nullptr) {
            readTargetType = readBuffer->target();
        }
    }

    GLenum writeTargetType = writeTarget;
    if (writeTargetType == GL_COPY_READ_BUFFER ||
        writeTargetType == GL_COPY_WRITE_BUFFER) {
        WebGLBuffer* writeBuffer =
            getState()->getBoundBuffer(writeTarget).valueOr(nullptr);
        if (writeBuffer != nullptr) {
            writeTargetType = writeBuffer->target();
        }
    }

    if ((readTargetType == GL_ELEMENT_ARRAY_BUFFER &&
         writeTargetType != GL_ELEMENT_ARRAY_BUFFER) ||
        (writeTargetType == GL_ELEMENT_ARRAY_BUFFER &&
         readTargetType != GL_ELEMENT_ARRAY_BUFFER)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    glCopyBufferSubData(readTarget, writeTarget, readOffset, writeOffset, size);
}

void WebGL2RenderingContext::getBufferSubData(GLenum target,
                                              GLintptr srcByteOffset,
                                              ScriptArrayBufferView dstBuffer,
                                              unsigned long long dstOffset,
                                              GLuint length)
{
    ENTER_CONTEXT_SCOPE();

    size_t dstLength = dstBuffer->isDataViewObject() ? dstBuffer->byteLength()
                                                     : dstBuffer->arrayLength();

    unsigned long long copyLength =
        (length == 0) ? dstLength - dstOffset : length;

    if (copyLength == 0) {
        return;
    }

    size_t elementSize =
        dstBuffer->isDataViewObject()
            ? 1
            : dstBuffer->byteLength() / dstBuffer->arrayLength();

    unsigned long long copyByteLength = copyLength * elementSize;

    if (!getState()->getBoundBuffer(target).hasValue()) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    // TODO: If target is TRANSFORM_FEEDBACK_BUFFER, and any transform feedback
    // object is currently active, generates an INVALID_OPERATION error.

    GLint64 bufSize;
    gl()->getBufferParameteri64v(target, GL_BUFFER_SIZE, &bufSize);
    if (hasNewGLError() || bufSize < 0) {
        return;
    }
    if (dstOffset > dstLength || copyLength > dstLength - dstOffset ||
        srcByteOffset < 0 || srcByteOffset > bufSize ||
        copyByteLength >
            static_cast<unsigned long long>(bufSize - srcByteOffset)) {
        setGLError(GL_INVALID_VALUE);
        return;
    }

    void* src = glMapBufferRange(target, srcByteOffset, copyByteLength,
                                 GL_MAP_READ_BIT);
    if (hasNewGLError() || src == nullptr) {
        return;
    }
    memcpy(dstBuffer->rawBuffer() + dstOffset * elementSize, src,
           copyByteLength);
    glUnmapBuffer(target);
}

GLint WebGL2RenderingContext::getFragDataLocation(WebGLProgram* program,
                                                  String* name)
{
    ENTER_CONTEXT_SCOPE(-1);

    if (!isFromCurrentContext(program)) {
        setGLError(GL_INVALID_OPERATION);
        return -1;
    }
    return gl()->getFragDataLocation(program->glObject(), CSTR(name));
}

void WebGL2RenderingContext::vertexAttrib1f(GLuint index, GLfloat x)
{
    WebGLRenderingContext::vertexAttrib1f(index, x);
    m_currentVertexAttribType = GL_FLOAT;
}

void WebGL2RenderingContext::vertexAttrib2f(GLuint index, GLfloat x, GLfloat y)
{
    WebGLRenderingContext::vertexAttrib2f(index, x, y);
    m_currentVertexAttribType = GL_FLOAT;
}

void WebGL2RenderingContext::vertexAttrib3f(GLuint index, GLfloat x, GLfloat y,
                                            GLfloat z)
{
    WebGLRenderingContext::vertexAttrib3f(index, x, y, z);
    m_currentVertexAttribType = GL_FLOAT;
}

void WebGL2RenderingContext::vertexAttrib4f(GLuint index, GLfloat x, GLfloat y,
                                            GLfloat z, GLfloat w)
{
    WebGLRenderingContext::vertexAttrib4f(index, x, y, z, w);
    m_currentVertexAttribType = GL_FLOAT;
}

void WebGL2RenderingContext::vertexAttrib1fv(GLuint index, Float32List values)
{
    WebGLRenderingContext::vertexAttrib1fv(index, values);
    m_currentVertexAttribType = GL_FLOAT;
}

void WebGL2RenderingContext::vertexAttrib2fv(GLuint index, Float32List values)
{
    WebGLRenderingContext::vertexAttrib2fv(index, values);
    m_currentVertexAttribType = GL_FLOAT;
}

void WebGL2RenderingContext::vertexAttrib3fv(GLuint index, Float32List values)
{
    WebGLRenderingContext::vertexAttrib3fv(index, values);
    m_currentVertexAttribType = GL_FLOAT;
}

void WebGL2RenderingContext::vertexAttrib4fv(GLuint index, Float32List values)
{
    WebGLRenderingContext::vertexAttrib4fv(index, values);
    m_currentVertexAttribType = GL_FLOAT;
}

void WebGL2RenderingContext::uniform1ui(
    Optional<WebGLUniformLocation*> location, GLuint v0)
{
    ENTER_CONTEXT_SCOPE();

    if (!location.hasValue()) {
        return;
    }

    WebGLUniformLocation* uniform = location.value();

    if (!isFromCurrentProgram(uniform)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    gl()->uniform1ui(uniform->location(), v0);
}

void WebGL2RenderingContext::uniform2ui(
    Optional<WebGLUniformLocation*> location, GLuint v0, GLuint v1)
{
    ENTER_CONTEXT_SCOPE();

    if (!location.hasValue()) {
        return;
    }

    WebGLUniformLocation* uniform = location.value();

    if (!isFromCurrentProgram(uniform)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    gl()->uniform2ui(uniform->location(), v0, v1);
}

void WebGL2RenderingContext::uniform3ui(
    Optional<WebGLUniformLocation*> location, GLuint v0, GLuint v1, GLuint v2)
{
    ENTER_CONTEXT_SCOPE();

    if (!location.hasValue()) {
        return;
    }

    WebGLUniformLocation* uniform = location.value();

    if (!isFromCurrentProgram(uniform)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    gl()->uniform3ui(uniform->location(), v0, v1, v2);
}

void WebGL2RenderingContext::uniform4ui(
    Optional<WebGLUniformLocation*> location, GLuint v0, GLuint v1, GLuint v2,
    GLuint v3)
{
    ENTER_CONTEXT_SCOPE();

    if (!location.hasValue()) {
        return;
    }

    WebGLUniformLocation* uniform = location.value();

    if (!isFromCurrentProgram(uniform)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    gl()->uniform4ui(uniform->location(), v0, v1, v2, v3);
}

void WebGL2RenderingContext::implementUniformNuiv(
    size_t n, void (GL::*uniformNuiv)(GLint, GLsizei, const GLuint*),
    Optional<WebGLUniformLocation*> location, Uint32List data,
    unsigned long long srcOffset, GLuint srcLength)
{
    ENTER_CONTEXT_SCOPE();
    /* location is nullable. */
    if (!location.hasValue()) {
        return;
    }
    if (!isFromCurrentProgram(location.value())) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }
    if (data.isUint32ArrayValue()) {
        const ScriptUint32Array values = data.getUint32ArrayValue();
        const size_t dataLength = values->arrayLength();
        if (srcOffset >= dataLength) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
        if (srcLength == 0) {
            srcLength = dataLength - srcOffset;
        }
        if (srcLength > dataLength - srcOffset || srcLength < n ||
            srcLength % n != 0) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
        /* count specifies the number of sets. */
        (gl()->*uniformNuiv)(
            location.value()->location(), srcLength / n,
            reinterpret_cast<const GLuint*>(values->rawBuffer()) + srcOffset);
    } else if (data.isSequenceOfGLuintValue()) {
        const GCAtomicVector<uint32_t> values = data.getSequenceOfGLuintValue();
        const size_t dataLength = values.size();
        if (srcOffset >= dataLength) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
        if (srcLength == 0) {
            srcLength = dataLength - srcOffset;
        }
        if (srcLength > dataLength - srcOffset || srcLength < n ||
            srcLength % n != 0) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
        /* count specifies the number of sets. */
        (gl()->*uniformNuiv)(location.value()->location(), srcLength / n,
                             values.data() + srcOffset);
    } else {
        STARFISH_ASSERT_NOT_REACHED();
    }
}

void WebGL2RenderingContext::uniform1uiv(
    Optional<WebGLUniformLocation*> location, Uint32List data,
    unsigned long long srcOffset, GLuint srcLength)
{
    implementUniformNuiv(1, &GL::uniform1uiv, location, data, srcOffset,
                         srcLength);
}

void WebGL2RenderingContext::uniform2uiv(
    Optional<WebGLUniformLocation*> location, Uint32List data,
    unsigned long long srcOffset, GLuint srcLength)
{
    implementUniformNuiv(2, &GL::uniform2uiv, location, data, srcOffset,
                         srcLength);
}

void WebGL2RenderingContext::uniform3uiv(
    Optional<WebGLUniformLocation*> location, Uint32List data,
    unsigned long long srcOffset, GLuint srcLength)
{
    implementUniformNuiv(3, &GL::uniform3uiv, location, data, srcOffset,
                         srcLength);
}

void WebGL2RenderingContext::uniform4uiv(
    Optional<WebGLUniformLocation*> location, Uint32List data,
    unsigned long long srcOffset, GLuint srcLength)
{
    implementUniformNuiv(4, &GL::uniform4uiv, location, data, srcOffset,
                         srcLength);
}

void WebGL2RenderingContext::uniformMatrix3x2fv(
    Optional<WebGLUniformLocation*> location, GLboolean transpose,
    Float32List data, unsigned long long srcOffset, GLuint srcLength)
{
    implementUniformMatrixMxNfv(3, 2, &GL::uniformMatrix3x2fv, location,
                                transpose, data, srcOffset, srcLength);
}

void WebGL2RenderingContext::uniformMatrix4x2fv(
    Optional<WebGLUniformLocation*> location, GLboolean transpose,
    Float32List data, unsigned long long srcOffset, GLuint srcLength)
{
    implementUniformMatrixMxNfv(4, 2, &GL::uniformMatrix4x2fv, location,
                                transpose, data, srcOffset, srcLength);
}

void WebGL2RenderingContext::uniformMatrix2x3fv(
    Optional<WebGLUniformLocation*> location, GLboolean transpose,
    Float32List data, unsigned long long srcOffset, GLuint srcLength)
{
    implementUniformMatrixMxNfv(2, 3, &GL::uniformMatrix2x3fv, location,
                                transpose, data, srcOffset, srcLength);
}

void WebGL2RenderingContext::uniformMatrix4x3fv(
    Optional<WebGLUniformLocation*> location, GLboolean transpose,
    Float32List data, unsigned long long srcOffset, GLuint srcLength)
{
    implementUniformMatrixMxNfv(4, 3, &GL::uniformMatrix4x3fv, location,
                                transpose, data, srcOffset, srcLength);
}

void WebGL2RenderingContext::uniformMatrix2x4fv(
    Optional<WebGLUniformLocation*> location, GLboolean transpose,
    Float32List data, unsigned long long srcOffset, GLuint srcLength)
{
    implementUniformMatrixMxNfv(2, 4, &GL::uniformMatrix2x4fv, location,
                                transpose, data, srcOffset, srcLength);
}

void WebGL2RenderingContext::uniformMatrix3x4fv(
    Optional<WebGLUniformLocation*> location, GLboolean transpose,
    Float32List data, unsigned long long srcOffset, GLuint srcLength)
{
    implementUniformMatrixMxNfv(3, 4, &GL::uniformMatrix3x4fv, location,
                                transpose, data, srcOffset, srcLength);
}

void WebGL2RenderingContext::vertexAttribI4i(GLuint index, GLint x, GLint y,
                                             GLint z, GLint w)
{
    ENTER_CONTEXT_SCOPE();

    gl()->vertexAttribI4i(index, x, y, z, w);
    m_currentVertexAttribType = GL_INT;
}

void WebGL2RenderingContext::vertexAttribI4iv(GLuint index, Int32List values)
{
    ENTER_CONTEXT_SCOPE();

    if (values.isInt32ArrayValue()) {
        const ScriptInt32Array v = values.getInt32ArrayValue();
        const size_t dataLength = v->arrayLength();
        if (dataLength != 4) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
        gl()->vertexAttribI4iv(index,
                               reinterpret_cast<const GLint*>(v->rawBuffer()));
    } else if (values.isSequenceOfGLintValue()) {
        GCAtomicVector<int32_t> v = values.getSequenceOfGLintValue();
        const size_t dataLength = v.size();
        if (dataLength != 4) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
        gl()->vertexAttribI4iv(index, v.data());
    } else {
        STARFISH_ASSERT_NOT_REACHED();
    }
    m_currentVertexAttribType = GL_INT;
}

void WebGL2RenderingContext::vertexAttribI4ui(GLuint index, GLuint x, GLuint y,
                                              GLuint z, GLuint w)
{
    ENTER_CONTEXT_SCOPE();

    gl()->vertexAttribI4ui(index, x, y, z, w);
    m_currentVertexAttribType = GL_UNSIGNED_INT;
}

void WebGL2RenderingContext::vertexAttribI4uiv(GLuint index, Uint32List values)
{
    ENTER_CONTEXT_SCOPE();

    if (values.isUint32ArrayValue()) {
        const ScriptUint32Array v = values.getUint32ArrayValue();
        const size_t dataLength = v->arrayLength();
        if (dataLength != 4) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
        gl()->vertexAttribI4uiv(
            index, reinterpret_cast<const GLuint*>(v->rawBuffer()));
    } else if (values.isSequenceOfGLuintValue()) {
        GCAtomicVector<uint32_t> v = values.getSequenceOfGLuintValue();
        const size_t dataLength = v.size();
        if (dataLength != 4) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
        gl()->vertexAttribI4uiv(index, v.data());
    } else {
        STARFISH_ASSERT_NOT_REACHED();
    }
    m_currentVertexAttribType = GL_UNSIGNED_INT;
}

void WebGL2RenderingContext::vertexAttribIPointer(GLuint index, GLint size,
                                                  GLenum type, GLsizei stride,
                                                  GLintptr offset)
{
    ENTER_CONTEXT_SCOPE();

    switch (type) {
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
        if (offset % 2 != 0 || stride % 2 != 0) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }
        break;
    case GL_INT:
    case GL_UNSIGNED_INT:
        if (offset % 4 != 0 || stride % 4 != 0) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }
        break;
    default:
        break;
    }

    if (offset < 0) {
        setGLError(GL_INVALID_VALUE);
        return;
    }

    if (!getState()->getBoundBuffer(GL_ARRAY_BUFFER).hasValue() &&
        offset != 0) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    if (stride > kMaximumSupportedStride) {
        setGLError(GL_INVALID_VALUE);
        return;
    }

    gl()->vertexAttribIPointer(index, size, type, stride,
                               reinterpret_cast<const void*>(offset));
    if (hasNewGLError()) {
        return;
    }
    getState()->setBufferBoundToVertexAttributes(
        index, getState()->getBoundBuffer(GL_ARRAY_BUFFER));
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

    if (!sync.hasValue() || !isFromCurrentContext(sync.value()) ||
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
    if (!isFromCurrentContext(value)) {
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

    if (!isFromCurrentContext(sync) || flags & ~GL_SYNC_FLUSH_COMMANDS_BIT ||
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

    if (!isFromCurrentContext(sync)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }
    gl()->waitSync(sync->glObject(), flags, timeout);
}

ScriptValue WebGL2RenderingContext::getSyncParameter(WebGLSync* sync,
                                                     GLenum pname)
{
    ENTER_CONTEXT_SCOPE(scriptNull());

    if (!isFromCurrentContext(sync)) {
        setGLError(GL_INVALID_OPERATION);
        return scriptNull();
    }
    switch (pname) {
    // GLbitfield
    case GL_SYNC_FLAGS: {
        GLint value;
        gl()->getSynciv(sync->glObject(), pname, 1, nullptr, &value);
        if (hasNewGLError()) {
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
        if (hasNewGLError()) {
            return scriptNull();
        }
        return createScriptValue(static_cast<GLenum>(value));
    }
    default:
        break;
    }
    setGLError(GL_INVALID_ENUM);
    return scriptNull();
}

void WebGL2RenderingContext::bindBufferBase(GLenum target, GLuint index,
                                            Optional<WebGLBuffer*> buffer)
{
    ENTER_CONTEXT_SCOPE();

    if (target != GL_TRANSFORM_FEEDBACK_BUFFER && target != GL_UNIFORM_BUFFER) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    if (buffer.hasValue()) {
        WebGLBuffer* value = buffer.value();

        if (!isFromCurrentContext(value)) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        if (value->isDeleted()) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        if (value->target() != GL_NONE &&
            ((value->target() == GL_ELEMENT_ARRAY_BUFFER &&
              target != GL_ELEMENT_ARRAY_BUFFER &&
              target != GL_COPY_READ_BUFFER &&
              target != GL_COPY_WRITE_BUFFER) ||
             (value->target() != GL_ELEMENT_ARRAY_BUFFER &&
              target == GL_ELEMENT_ARRAY_BUFFER))) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        gl()->bindBufferBase(target, index, value->glObject());
        if (hasNewGLError()) {
            return;
        }
        getState()->setBoundBuffer(target, value);

        value->setTargetOnce(target);
        if (target == GL_UNIFORM_BUFFER) {
            if (index >= m_uniformBufferBindings.size()) {
                m_uniformBufferBindings.resize(index + 1);
            }
            m_uniformBufferBindings[index] =
                new IndexedBufferBinding(value, 0, 0, false);
        }
    } else {
        gl()->bindBufferBase(target, index, 0);
        if (hasNewGLError()) {
            return;
        }
        getState()->setBoundBuffer(target, nullptr);
        if (target == GL_UNIFORM_BUFFER &&
            index < m_uniformBufferBindings.size()) {
            m_uniformBufferBindings[index] = nullptr;
        }
    }
}

void WebGL2RenderingContext::bindBufferRange(GLenum target, GLuint index,
                                             Optional<WebGLBuffer*> buffer,
                                             GLintptr offset, GLsizeiptr size)
{
    ENTER_CONTEXT_SCOPE();

    if (target != GL_TRANSFORM_FEEDBACK_BUFFER && target != GL_UNIFORM_BUFFER) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    if (buffer.hasValue()) {
        WebGLBuffer* value = buffer.value();

        if (!isFromCurrentContext(value)) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        if (value->isDeleted()) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        if (value->target() != GL_NONE &&
            ((value->target() == GL_ELEMENT_ARRAY_BUFFER &&
              target != GL_ELEMENT_ARRAY_BUFFER &&
              target != GL_COPY_READ_BUFFER &&
              target != GL_COPY_WRITE_BUFFER) ||
             (value->target() != GL_ELEMENT_ARRAY_BUFFER &&
              target == GL_ELEMENT_ARRAY_BUFFER))) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        gl()->bindBufferRange(target, index, value->glObject(), offset, size);
        if (hasNewGLError()) {
            return;
        }
        getState()->setBoundBuffer(target, value);

        value->setTargetOnce(target);
        if (target == GL_UNIFORM_BUFFER) {
            if (index >= m_uniformBufferBindings.size()) {
                m_uniformBufferBindings.resize(index + 1);
            }
            m_uniformBufferBindings[index] =
                new IndexedBufferBinding(value, offset, size, true);
        }
    } else {
        gl()->bindBufferRange(target, index, 0, offset, size);
        if (hasNewGLError()) {
            return;
        }
        getState()->setBoundBuffer(target, nullptr);
        if (target == GL_UNIFORM_BUFFER &&
            index < m_uniformBufferBindings.size()) {
            m_uniformBufferBindings[index] = nullptr;
        }
    }
}

GLuint WebGL2RenderingContext::getUniformBlockIndex(WebGLProgram* program,
                                                    String* uniformBlockName)
{
    ENTER_CONTEXT_SCOPE(GL_INVALID_INDEX);
    if (!isFromCurrentContext(program)) {
        setGLError(GL_INVALID_OPERATION);
        return GL_INVALID_INDEX;
    }
    if (!uniformBlockName || uniformBlockName->length() == 0) {
        return GL_INVALID_INDEX;
    }
    return gl()->getUniformBlockIndex(
        program->glObject(), uniformBlockName->toUTF8NonGCString().c_str());
}

ScriptValue WebGL2RenderingContext::getActiveUniformBlockParameter(
    WebGLProgram* program, GLuint uniformBlockIndex, GLenum pname)
{
    ENTER_CONTEXT_SCOPE(scriptNull());
    if (!isFromCurrentContext(program)) {
        setGLError(GL_INVALID_OPERATION);
        return scriptNull();
    }
    switch (pname) {
    case GL_UNIFORM_BLOCK_BINDING:
    case GL_UNIFORM_BLOCK_DATA_SIZE:
    case GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS: {
        GLint params = 0;
        gl()->getActiveUniformBlockiv(program->glObject(), uniformBlockIndex,
                                      pname, &params);
        if (hasNewGLError()) {
            return scriptNull();
        }
        return createScriptValue(params);
    }
    case GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES: {
        GLint uniformCount = 0;
        gl()->getActiveUniformBlockiv(program->glObject(), uniformBlockIndex,
                                      GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS,
                                      &uniformCount);
        if (hasNewGLError() || uniformCount == 0) {
            return createScriptValue(
                createTypedArray<Escargot::Uint32ArrayObjectRef>(
                    scriptBindingInstance(), std::vector<GLuint>()));
        }
        std::vector<GLint> indices(uniformCount);
        gl()->getActiveUniformBlockiv(program->glObject(), uniformBlockIndex,
                                      pname, indices.data());
        if (hasNewGLError()) {
            return scriptNull();
        }
        return createScriptValue(
            createTypedArray<Escargot::Uint32ArrayObjectRef>(
                scriptBindingInstance(),
                std::vector<GLuint>(indices.begin(), indices.end())));
    }
    case GL_UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER:
    case GL_UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER: {
        GLint params = 0;
        gl()->getActiveUniformBlockiv(program->glObject(), uniformBlockIndex,
                                      pname, &params);
        if (hasNewGLError()) {
            return scriptNull();
        }
        return createScriptValue(static_cast<bool>(params));
    }
    default:
        setGLError(GL_INVALID_ENUM);
        return scriptNull();
    }
}

String* WebGL2RenderingContext::getActiveUniformBlockName(
    WebGLProgram* program, GLuint uniformBlockIndex)
{
    ENTER_CONTEXT_SCOPE(nullptr);
    if (!isFromCurrentContext(program)) {
        setGLError(GL_INVALID_OPERATION);
        return nullptr;
    }
    GLint maxNameLength = 0;
    gl()->getActiveUniformBlockiv(program->glObject(), uniformBlockIndex,
                                  GL_UNIFORM_BLOCK_NAME_LENGTH, &maxNameLength);
    if (hasNewGLError() || maxNameLength == 0) {
        return nullptr;
    }
    std::vector<GLchar> name(maxNameLength);
    GLsizei length = 0;
    gl()->getActiveUniformBlockName(program->glObject(), uniformBlockIndex,
                                    maxNameLength, &length, name.data());
    if (hasNewGLError()) {
        return nullptr;
    }
    return String::fromUTF8(name.data(), length);
}

void WebGL2RenderingContext::uniformBlockBinding(WebGLProgram* program,
                                                 GLuint uniformBlockIndex,
                                                 GLuint uniformBlockBinding)
{
    ENTER_CONTEXT_SCOPE();
    if (!isFromCurrentContext(program)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    GLint maxBindings = 0;
    gl()->getIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxBindings);
    if (uniformBlockBinding >= static_cast<GLuint>(maxBindings)) {
        setGLError(GL_INVALID_VALUE,
                   "uniformBlockBinding is greater than or equal to "
                   "MAX_UNIFORM_BUFFER_BINDINGS.");
        return;
    }

    gl()->uniformBlockBinding(program->glObject(), uniformBlockIndex,
                              uniformBlockBinding);
}

Optional<GCAtomicVector<GLuint>> WebGL2RenderingContext::getUniformIndices(
    WebGLProgram* program, GCVector<String*> uniformNames)
{
    ENTER_CONTEXT_SCOPE(Optional<GCAtomicVector<GLuint>>());

    if (!isFromCurrentContext(program)) {
        setGLError(GL_INVALID_OPERATION);
        return Optional<GCAtomicVector<GLuint>>();
    }

    size_t count = uniformNames.size();
    if (count == 0) {
        return GCAtomicVector<GLuint>();
    }

    std::vector<std::string> nameStrings;
    nameStrings.reserve(count);
    std::vector<const GLchar*> names;
    names.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        if (!uniformNames[i]) {
            return Optional<GCAtomicVector<GLuint>>();
        }
        nameStrings.push_back(uniformNames[i]->toUTF8NonGCString());
        names.push_back(nameStrings.back().c_str());
    }

    GCAtomicVector<GLuint> indices;
    indices.resize(count);
    gl()->getUniformIndices(program->glObject(), count, names.data(),
                            indices.data());

    if (hasNewGLError()) {
        return Optional<GCAtomicVector<GLuint>>();
    }

    return indices;
}

ScriptValue WebGL2RenderingContext::getActiveUniforms(
    WebGLProgram* program, GCAtomicVector<GLuint> uniformIndices, GLenum pname)
{
    ENTER_CONTEXT_SCOPE(scriptNull());

    if (!isFromCurrentContext(program)) {
        setGLError(GL_INVALID_OPERATION);
        return scriptNull();
    }
    switch (pname) {
    // sequence<GLboolean>
    case GL_UNIFORM_IS_ROW_MAJOR: {
        GLsizei count = uniformIndices.size();
        std::vector<GLint> values(count);
        gl()->getActiveUniformsiv(program->glObject(), count,
                                  uniformIndices.data(), pname, values.data());
        return createScriptValue(
            createArray(scriptBindingInstance(),
                        std::vector<bool>(values.begin(), values.end())));
    }
    // sequence<GLenum>
    case GL_UNIFORM_TYPE: {
        GLsizei count = uniformIndices.size();
        std::vector<GLint> values(count);
        gl()->getActiveUniformsiv(program->glObject(), count,
                                  uniformIndices.data(), pname, values.data());
        return createScriptValue(
            createTypedArray<Escargot::Uint32ArrayObjectRef>(
                scriptBindingInstance(),
                std::vector<GLenum>(values.begin(), values.end())));
    }
    // sequence<GLint>
    case GL_UNIFORM_ARRAY_STRIDE:
    case GL_UNIFORM_BLOCK_INDEX:
    case GL_UNIFORM_MATRIX_STRIDE:
    case GL_UNIFORM_OFFSET: {
        GLsizei count = uniformIndices.size();
        std::vector<GLint> values(count);
        gl()->getActiveUniformsiv(program->glObject(), count,
                                  uniformIndices.data(), pname, values.data());
        return createScriptValue(
            createTypedArray<Escargot::Int32ArrayObjectRef>(
                scriptBindingInstance(), values));
    }
    // sequence<GLuint>
    case GL_UNIFORM_SIZE: {
        GLsizei count = uniformIndices.size();
        std::vector<GLint> values(count);
        gl()->getActiveUniformsiv(program->glObject(), count,
                                  uniformIndices.data(), pname, values.data());
        return createScriptValue(
            createTypedArray<Escargot::Uint32ArrayObjectRef>(
                scriptBindingInstance(),
                std::vector<GLenum>(values.begin(), values.end())));
    }
    }
    setGLError(GL_INVALID_ENUM);
    return scriptNull();
}

WebGLVertexArrayObject* WebGL2RenderingContext::createVertexArray()
{
    ENTER_CONTEXT_SCOPE(nullptr);

    GLuint vao = 0;
    gl()->genVertexArrays(1, &vao);
    return new WebGLVertexArrayObject(scriptBindingInstance(), this, vao);
}

void WebGL2RenderingContext::deleteVertexArray(
    Optional<WebGLVertexArrayObject*> vertexArray)
{
    ENTER_CONTEXT_SCOPE();

    if (!vertexArray.hasValue()) {
        return;
    }

    WebGLVertexArrayObject* value = vertexArray.value();

    if (!isFromCurrentContext(value)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    if (value->isDeleted()) {
        return;
    }

    GLuint vao = value->glObject();
    TRACE(WEBGL, KV(vao));
    gl()->deleteVertexArrays(1, &vao);
    value->markDeleted();
    getState()->deleteVertexArray(vao);
}

GLboolean WebGL2RenderingContext::isVertexArray(
    Optional<WebGLVertexArrayObject*> vertexArray)
{
    ENTER_CONTEXT_SCOPE(false);

    if (!vertexArray.hasValue()) {
        return false;
    }

    WebGLVertexArrayObject* value = vertexArray.value();

    if (!isFromCurrentContext(value) || value->invalidated()) {
        return false;
    }

    if (!value->hasEverBound()) {
        return false;
    }

    return gl()->isVertexArray(value->glObject());
}

void WebGL2RenderingContext::bindVertexArray(
    Optional<WebGLVertexArrayObject*> array)
{
    ENTER_CONTEXT_SCOPE();

    if (!array.hasValue()) {
        gl()->bindVertexArray(0);
        getState()->setWebGLVertexArrayObject(nullptr);
        return;
    }

    WebGLVertexArrayObject* value = array.value();

    if (!isFromCurrentContext(value)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    if (value->isDeleted()) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    TRACE(WEBGL, KV(value->glObject()));
    gl()->bindVertexArray(value->glObject());
    if (hasNewGLError()) {
        return;
    }
    value->setHasEverBound();
    getState()->setWebGLVertexArrayObject(value);
}

// WebGL2RenderingContextOverloads

bool WebGL2RenderingContext::validateDrawCallUBO()
{
    GLint programId = getCurrentProgram();
    if (!programId)
        return true;

    GLint numBlocks = 0;
    gl()->getProgramiv(programId, GL_ACTIVE_UNIFORM_BLOCKS, &numBlocks);

    for (GLint i = 0; i < numBlocks; i++) {
        GLint dataSize = 0;
        gl()->getActiveUniformBlockiv(programId, i, GL_UNIFORM_BLOCK_DATA_SIZE,
                                      &dataSize);
        if (dataSize > 0) {
            GLint binding = 0;
            gl()->getActiveUniformBlockiv(programId, i,
                                          GL_UNIFORM_BLOCK_BINDING, &binding);

            if (binding < 0 ||
                static_cast<size_t>(binding) >=
                    m_uniformBufferBindings.size() ||
                m_uniformBufferBindings[binding] == nullptr ||
                m_uniformBufferBindings[binding]->buffer == nullptr ||
                m_uniformBufferBindings[binding]->buffer->isDeleted()) {
                setGLError(GL_INVALID_OPERATION,
                           "draw: UniformBlock is not backed by a buffer.");
                return false;
            }

            IndexedBufferBinding* b = m_uniformBufferBindings[binding];

            GLint totalBufferSize = 0;
            gl()->bindBuffer(GL_UNIFORM_BUFFER, b->buffer->glObject());
            gl()->getBufferParameteriv(GL_UNIFORM_BUFFER, GL_BUFFER_SIZE,
                                       &totalBufferSize);

            // Restore generic bound buffer
            Optional<WebGLBuffer*> prevBound =
                getState()->getBoundBuffer(GL_UNIFORM_BUFFER);
            gl()->bindBuffer(GL_UNIFORM_BUFFER,
                             (prevBound.hasValue() && prevBound.value())
                                 ? prevBound.value()->glObject()
                                 : 0);

            if (totalBufferSize == 0) {
                setGLError(GL_INVALID_OPERATION,
                           "draw: UniformBlock is backed by a buffer with no "
                           "data store.");
                return false;
            }

            GLsizeiptr effectiveSize = totalBufferSize;
            if (b->isRange) {
                effectiveSize = b->size;
            }

            if (effectiveSize < dataSize) {
                setGLError(GL_INVALID_OPERATION,
                           "draw: bound UNIFORM_BUFFER size is smaller than "
                           "active uniform block data size.");
                return false;
            }
        }
    }
    return true;
}

void WebGL2RenderingContext::shaderSource(WebGLShader* shader, String* source)
{
    ENTER_CONTEXT_SCOPE();

    std::string originalStr = source->toUTF8NonGCString();
    shader->setSource(originalStr);

    std::string str = originalStr;

    if (str.find("layout(packed)") != std::string::npos ||
        str.find("layout (packed)") != std::string::npos ||
        str.find("layout(shared)") != std::string::npos ||
        str.find("layout (shared)") != std::string::npos) {
        str = "#error WebGL2 does not allow packed or shared layouts\n" + str;
    }

    // WebGL2 uniform blocks default to std140 layout if unspecified.
    // If layout(...) is missing before "uniform BlockName {", inject
    // "layout(std140) ".
    std::regex blockRegex(R"((^|[^\w\)])uniform\s+([A-Za-z0-9_]+)\s*\{)");
    std::string result;
    size_t lastPos = 0;
    auto words_begin = std::sregex_iterator(str.begin(), str.end(), blockRegex);
    auto words_end = std::sregex_iterator();

    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
        std::smatch match = *i;
        size_t matchPos = match.position();

        bool hasLayout = false;
        if (matchPos > 0) {
            size_t p = matchPos;
            while (p > 0 && (str[p - 1] == ' ' || str[p - 1] == '\t' ||
                             str[p - 1] == '\r' || str[p - 1] == '\n')) {
                p--;
            }
            if (p > 0 && str[p - 1] == ')') {
                int depth = 1;
                size_t q = p - 1;
                while (q > 0 && depth > 0) {
                    q--;
                    if (str[q] == ')')
                        depth++;
                    else if (str[q] == '(')
                        depth--;
                }
                if (depth == 0) {
                    size_t w = q;
                    while (w > 0 && (str[w - 1] == ' ' || str[w - 1] == '\t'))
                        w--;
                    if (w >= 6 && str.substr(w - 6, 6) == "layout") {
                        hasLayout = true;
                    }
                }
            }
        }

        result.append(str, lastPos, matchPos - lastPos);
        if (!hasLayout) {
            std::string prefix = match[1].str();
            std::string blockName = match[2].str();
            result += prefix + "layout(std140) uniform " + blockName + " {";
        } else {
            result += match.str();
        }
        lastPos = matchPos + match.length();
    }
    result.append(str, lastPos, str.length() - lastPos);
    str = result;

    const char* sourceArray[1] = { str.c_str() };
    gl()->shaderSource(shader->glObject(), 1, sourceArray, nullptr);
}

void WebGL2RenderingContext::vertexAttribDivisor(GLuint index, GLuint divisor)
{
    ENTER_CONTEXT_SCOPE();
    gl()->vertexAttribDivisor(index, divisor);
}

void WebGL2RenderingContext::drawArraysInstanced(GLenum mode, GLint first,
                                                 GLsizei count,
                                                 GLsizei instanceCount)
{
    ENTER_CONTEXT_SCOPE();
    completePendingJobs();
    if (!validateDrawCallUBO())
        return;
    gl()->drawArraysInstanced(mode, first, count, instanceCount);
    m_ownerHTMLCanvasElement->setNeedsComposite();
}

void WebGL2RenderingContext::drawElementsInstanced(GLenum mode, GLsizei count,
                                                   GLenum type, GLintptr offset,
                                                   GLsizei instanceCount)
{
    ENTER_CONTEXT_SCOPE();
    completePendingJobs();
    if (!validateDrawCallUBO())
        return;
    gl()->drawElementsInstanced(mode, count, type,
                                reinterpret_cast<const void*>(offset),
                                instanceCount);
    m_ownerHTMLCanvasElement->setNeedsComposite();
}

void WebGL2RenderingContext::drawRangeElements(GLenum mode, GLuint start,
                                               GLuint end, GLsizei count,
                                               GLenum type, GLintptr offset)
{
    ENTER_CONTEXT_SCOPE();
    completePendingJobs();
    if (!validateDrawCallUBO())
        return;
    gl()->drawRangeElements(mode, start, end, count, type,
                            reinterpret_cast<const void*>(offset));
    m_ownerHTMLCanvasElement->setNeedsComposite();
}

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

void WebGL2RenderingContext::bufferData(GLenum target,
                                        ScriptArrayBufferView srcData,
                                        GLenum usage,
                                        unsigned long long srcOffset,
                                        GLuint length)
{
    ENTER_CONTEXT_SCOPE();

    size_t srcLength = srcData->isDataViewObject() ? srcData->byteLength()
                                                   : srcData->arrayLength();

    unsigned long long copyLength =
        (length == 0) ? srcLength - srcOffset : length;

    if (copyLength == 0) {
        return;
    }

    size_t elementSize = srcData->isDataViewObject()
                             ? 1
                             : srcData->byteLength() / srcData->arrayLength();

    unsigned long long copyByteLength = copyLength * elementSize;

    if (!getState()->getBoundBuffer(target).hasValue()) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    if (srcOffset > srcLength || copyLength > srcLength - srcOffset) {
        setGLError(GL_INVALID_VALUE);
        return;
    }

    gl()->bufferData(target, copyByteLength,
                     srcData->rawBuffer() + srcOffset * elementSize, usage);
}

void WebGL2RenderingContext::bufferSubData(GLenum target,
                                           GLintptr dstByteOffset,
                                           ScriptArrayBufferView srcData,
                                           unsigned long long srcOffset,
                                           GLuint length)
{
    ENTER_CONTEXT_SCOPE();

    size_t srcLength = srcData->isDataViewObject() ? srcData->byteLength()
                                                   : srcData->arrayLength();

    unsigned long long copyLength =
        (length == 0) ? srcLength - srcOffset : length;

    if (copyLength == 0) {
        return;
    }

    size_t elementSize = srcData->isDataViewObject()
                             ? 1
                             : srcData->byteLength() / srcData->arrayLength();

    unsigned long long copyByteLength = copyLength * elementSize;

    if (!getState()->getBoundBuffer(target).hasValue()) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    GLint64 bufSize;
    gl()->getBufferParameteri64v(target, GL_BUFFER_SIZE, &bufSize);
    if (hasNewGLError() || bufSize < 0) {
        return;
    }
    if (dstByteOffset < 0 || dstByteOffset > bufSize ||
        copyByteLength >
            static_cast<unsigned long long>(bufSize - dstByteOffset) ||
        srcOffset > srcLength || copyLength > srcLength - srcOffset) {
        setGLError(GL_INVALID_VALUE);
        return;
    }

    gl()->bufferSubData(target, dstByteOffset, copyByteLength,
                        srcData->rawBuffer() + srcOffset * elementSize);
}

#ifndef GL_INTERNALFORMAT_SUPPORTED
#define GL_INTERNALFORMAT_SUPPORTED 0x826F
#endif

bool WebGL2RenderingContext::checkInternalFormat(GLint internalFormat,
                                                 GLenum format, GLenum type)
{
    if (!Pixel::isInternalFormatValid(internalFormat, format, type, 2)) {
        setGLError(GL_INVALID_OPERATION,
                   StringUtils::formatString(
                       "The given parameters, internal format (0x%04X), format "
                       "(0x%04X), and type (0x%04X), are not valid.",
                       internalFormat, format, type)
                       .c_str());
        return false;
    }

    return true;
}

bool WebGL2RenderingContext::isSrcDataValid(ScriptArrayBufferView srcData,
                                            GLenum type)
{
    return (srcData->isInt8ArrayObject() && type == GL_BYTE) ||
           (srcData->isUint8ArrayObject() && type == GL_UNSIGNED_BYTE) ||
           (srcData->isUint8ClampedArrayObject() && type == GL_UNSIGNED_BYTE) ||
           (srcData->isInt16ArrayObject() && type == GL_SHORT) ||
           (srcData->isUint16ArrayObject() &&
            (type == GL_UNSIGNED_SHORT || type == GL_UNSIGNED_SHORT_5_6_5 ||
             type == GL_UNSIGNED_SHORT_5_5_5_1 ||
             type == GL_UNSIGNED_SHORT_4_4_4_4 || type == GL_HALF_FLOAT)) ||
           (srcData->isInt32ArrayObject() && type == GL_INT) ||
           (srcData->isUint32ArrayObject() &&
            (type == GL_UNSIGNED_INT || type == GL_UNSIGNED_INT_5_9_9_9_REV ||
             type == GL_UNSIGNED_INT_2_10_10_10_REV ||
             type == GL_UNSIGNED_INT_10F_11F_11F_REV ||
             type == GL_UNSIGNED_INT_24_8)) ||
           (srcData->isFloat32ArrayObject() && type == GL_FLOAT);
}

size_t WebGL2RenderingContext::getBytesPerPixel(GLenum format, GLenum type)
{
    return Pixel::getBytesPerPixel(format, type, 2);
}

void WebGL2RenderingContext::texStorage2D(GLenum target, GLsizei levels,
                                          GLenum internalformat, GLsizei width,
                                          GLsizei height)
{
    ENTER_CONTEXT_SCOPE();

    if (target != GL_TEXTURE_2D && target != GL_TEXTURE_CUBE_MAP) {
        setGLError(GL_INVALID_ENUM, "target is invalid.");
        return;
    }

    if (!Pixel::isSizedInternalFormat(internalformat)) {
        setGLError(GL_INVALID_ENUM,
                   "internalformat is not a sized internal format.");
        return;
    }

    if (levels <= 0 || width <= 0 || height <= 0) {
        setGLError(GL_INVALID_VALUE, "dimensions must be greater than 0.");
        return;
    }

    GLsizei maxDim2D = std::max(width, height);
    GLsizei maxLevels2D = 0;
    while (maxDim2D > 0) {
        maxLevels2D++;
        maxDim2D >>= 1;
    }
    if (levels > maxLevels2D) {
        setGLError(GL_INVALID_OPERATION,
                   "levels is too large for the given dimensions.");
        return;
    }

    if (!hasBoundTexture(target)) {
        setGLError(
            GL_INVALID_OPERATION,
            StringUtils::formatString("target (0x%04X) is not bound.", target)
                .c_str());
        return;
    }

    WebGLTexture* texture = getBoundTextureObject(target);
    if (texture && texture->isImmutable()) {
        setGLError(GL_INVALID_OPERATION, "Texture is already immutable.");
        return;
    }

    gl()->texStorage2D(target, levels, internalformat, width, height);

    if (texture && !hasNewGLError()) {
        texture->setImmutable(true);
    }
}

void WebGL2RenderingContext::texStorage3D(GLenum target, GLsizei levels,
                                          GLenum internalformat, GLsizei width,
                                          GLsizei height, GLsizei depth)
{
    ENTER_CONTEXT_SCOPE();

    if (target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY) {
        setGLError(GL_INVALID_ENUM, "target is invalid.");
        return;
    }

    if (!Pixel::isSizedInternalFormat(internalformat)) {
        setGLError(GL_INVALID_ENUM,
                   "internalformat is not a sized internal format.");
        return;
    }

    if (levels <= 0 || width <= 0 || height <= 0 || depth <= 0) {
        setGLError(GL_INVALID_VALUE, "dimensions must be greater than 0.");
        return;
    }

    GLsizei maxDim3D = std::max(width, std::max(height, depth));
    GLsizei maxLevels3D = 0;
    while (maxDim3D > 0) {
        maxLevels3D++;
        maxDim3D >>= 1;
    }
    if (levels > maxLevels3D) {
        setGLError(GL_INVALID_OPERATION,
                   "levels is too large for the given dimensions.");
        return;
    }

    if (!hasBoundTexture(target)) {
        setGLError(
            GL_INVALID_OPERATION,
            StringUtils::formatString("target (0x%04X) is not bound.", target)
                .c_str());
        return;
    }

    WebGLTexture* texture = getBoundTextureObject(target);
    if (texture && texture->isImmutable()) {
        setGLError(GL_INVALID_OPERATION, "Texture is already immutable.");
        return;
    }

    gl()->texStorage3D(target, levels, internalformat, width, height, depth);

    if (texture && !hasNewGLError()) {
        texture->setImmutable(true);
    }
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

void WebGL2RenderingContext::texImage2D(GLenum target, GLint level,
                                        GLint internalformat, GLsizei width,
                                        GLsizei height, GLint border,
                                        GLenum format, GLenum type,
                                        GLintptr pboOffset)
{
    ENTER_CONTEXT_SCOPE();

    WebGLTexture* texture = getBoundTextureObject(target);
    if (texture && texture->isImmutable()) {
        setGLError(GL_INVALID_OPERATION, "Texture is immutable.");
        return;
    }

    if (!hasBoundTexture(target)) {
        setGLError(
            GL_INVALID_OPERATION,
            StringUtils::formatString("target (0x%04X) is not bound.", target)
                .c_str());
        return;
    }

    if (!getState()->getBoundBuffer(GL_PIXEL_UNPACK_BUFFER).hasValue() ||
        getState()->getBoundBuffer(GL_PIXEL_UNPACK_BUFFER).value() == nullptr) {
        setGLError(GL_INVALID_OPERATION, "PIXEL_UNPACK_BUFFER is not bound.");
        return;
    }

    gl()->texImage2D(target, level, internalformat, width, height, border,
                     format, type, reinterpret_cast<const void*>(pboOffset));
}

void WebGL2RenderingContext::texImage2D(GLenum target, GLint level,
                                        GLint internalformat, GLsizei width,
                                        GLsizei height, GLint border,
                                        GLenum format, GLenum type,
                                        TexImageSource source)
{
    ENTER_CONTEXT_SCOPE();

    if (!hasBoundTexture(target)) {
        setGLError(
            GL_INVALID_OPERATION,
            StringUtils::formatString("target (0x%04X) is not bound.", target)
                .c_str());
        return;
    }

    if (border != 0) {
        setGLError(GL_INVALID_VALUE, "border must be 0.");
        return;
    }

    if (!checkInternalFormat(internalformat, format, type)) {
        return;
    }

    GLint skipPixels = 0;
    GLint skipRows = 0;
    gl()->getIntegerv(GL_UNPACK_SKIP_PIXELS, &skipPixels);
    gl()->getIntegerv(GL_UNPACK_SKIP_ROWS, &skipRows);

    handleTexImageWithImageSource(
        format, type, source,
        [&](const TexImageHelper* helper) {
            STARFISH_ASSERT(helper != nullptr);
            gl()->texImage2D(
                target, level, helper->dataFormat().valueOr(internalformat),
                width, height, 0, helper->dataFormat().valueOr(format), type,
                helper->data());
        },
        0, skipPixels, skipRows, width, height, 1, 0);
}

void WebGL2RenderingContext::texImage2D(GLenum target, GLint level,
                                        GLint internalformat, GLsizei width,
                                        GLsizei height, GLint border,
                                        GLenum format, GLenum type,
                                        ScriptArrayBufferView srcData,
                                        unsigned long long srcOffset)
{
    ENTER_CONTEXT_SCOPE();

    WebGLTexture* texture = getBoundTextureObject(target);
    if (texture && texture->isImmutable()) {
        setGLError(GL_INVALID_OPERATION, "Texture is immutable.");
        return;
    }

    if (!hasBoundTexture(target)) {
        setGLError(
            GL_INVALID_OPERATION,
            StringUtils::formatString("target (0x%04X) is not bound.", target)
                .c_str());
        return;
    }

    if (border != 0) {
        setGLError(GL_INVALID_VALUE, "border must be 0.");
        return;
    }

    if (!checkInternalFormat(internalformat, format, type)) {
        return;
    }

    handleTexImageWithArrayBufferView(
        target, level, width, height, 1, format, type, srcData, srcOffset,
        [&](const TexImageHelper* helper) {
            STARFISH_ASSERT(helper != nullptr);
            gl()->texImage2D(target, level, internalformat, width, height, 0,
                             format, type, helper->data());
        },
        [&](const std::vector<GLubyte>& blackData) {
            gl()->texImage2D(target, level, internalformat, width, height, 0,
                             format, type, blackData.data());
        },
        [&](const std::vector<GLushort>& blackData) {
            gl()->texImage2D(target, level, internalformat, width, height, 0,
                             format, type, blackData.data());
        });
}

void WebGL2RenderingContext::texImage3D(GLenum target, GLint level,
                                        GLint internalformat, GLsizei width,
                                        GLsizei height, GLsizei depth,
                                        GLint border, GLenum format,
                                        GLenum type, GLintptr pboOffset)
{
    ENTER_CONTEXT_SCOPE();

    WebGLTexture* texture = getBoundTextureObject(target);
    if (texture && texture->isImmutable()) {
        setGLError(GL_INVALID_OPERATION, "Texture is immutable.");
        return;
    }

    if (!hasBoundTexture(target)) {
        setGLError(
            GL_INVALID_OPERATION,
            StringUtils::formatString("target (0x%04X) is not bound.", target)
                .c_str());
        return;
    }

    if (!getState()->getBoundBuffer(GL_PIXEL_UNPACK_BUFFER).hasValue() ||
        getState()->getBoundBuffer(GL_PIXEL_UNPACK_BUFFER).value() == nullptr) {
        setGLError(GL_INVALID_OPERATION, "PIXEL_UNPACK_BUFFER is not bound.");
        return;
    }

    gl()->texImage3D(target, level, internalformat, width, height, depth,
                     border, format, type,
                     reinterpret_cast<const void*>(pboOffset));
}

void WebGL2RenderingContext::texImage3D(GLenum target, GLint level,
                                        GLint internalformat, GLsizei width,
                                        GLsizei height, GLsizei depth,
                                        GLint border, GLenum format,
                                        GLenum type, TexImageSource source)
{
    ENTER_CONTEXT_SCOPE();

    if (!hasBoundTexture(target)) {
        setGLError(
            GL_INVALID_OPERATION,
            StringUtils::formatString("target (0x%04X) is not bound.", target)
                .c_str());
        return;
    }

    if (border != 0) {
        setGLError(GL_INVALID_VALUE, "border must be 0.");
        return;
    }

    if (!checkInternalFormat(internalformat, format, type)) {
        return;
    }

    GLint skipPixels = 0;
    GLint skipRows = 0;
    GLint imageHeight = 0;
    gl()->getIntegerv(GL_UNPACK_SKIP_PIXELS, &skipPixels);
    gl()->getIntegerv(GL_UNPACK_SKIP_ROWS, &skipRows);
    gl()->getIntegerv(GL_UNPACK_IMAGE_HEIGHT, &imageHeight);

    handleTexImageWithImageSource(
        format, type, source,
        [&](const TexImageHelper* helper) {
            STARFISH_ASSERT(helper != nullptr);
            gl()->texImage3D(
                target, level, helper->dataFormat().valueOr(internalformat),
                width, height, depth, 0, helper->dataFormat().valueOr(format),
                type, helper->data());
        },
        height, skipPixels, skipRows, width, height, depth, imageHeight);
}

void WebGL2RenderingContext::texImage3D(GLenum target, GLint level,
                                        GLint internalformat, GLsizei width,
                                        GLsizei height, GLsizei depth,
                                        GLint border, GLenum format,
                                        GLenum type,
                                        Optional<ScriptArrayBufferView> srcData)
{
    ENTER_CONTEXT_SCOPE();

    if (target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY) {
        setGLError(GL_INVALID_ENUM, "target is invalid.");
        return;
    }

    if (!hasBoundTexture(target)) {
        setGLError(
            GL_INVALID_OPERATION,
            StringUtils::formatString("target (0x%04X) is not bound.", target)
                .c_str());
        return;
    }

    if (!checkInternalFormat(internalformat, format, type)) {
        return;
    }

    handleTexImageWithArrayBufferView(
        target, level, width, height, depth, format, type, srcData, 0,
        [&](const TexImageHelper* helper) {
            STARFISH_ASSERT(helper != nullptr);
            gl()->texImage3D(target, level, internalformat, width, height,
                             depth, 0, format, type, helper->data());
        },
        [&](const std::vector<GLubyte>& blackData) {
            gl()->texImage3D(target, level, internalformat, width, height,
                             depth, 0, format, type, blackData.data());
        },
        [&](const std::vector<GLushort>& blackData) {
            gl()->texImage3D(target, level, internalformat, width, height,
                             depth, 0, format, type, blackData.data());
        });
}

void WebGL2RenderingContext::texImage3D(
    GLenum target, GLint level, GLint internalformat, GLsizei width,
    GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type,
    ScriptArrayBufferView srcData, unsigned long long srcOffset)
{
    ENTER_CONTEXT_SCOPE();

    if (target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY) {
        setGLError(GL_INVALID_ENUM, "target is invalid.");
        return;
    }

    if (!hasBoundTexture(target)) {
        setGLError(
            GL_INVALID_OPERATION,
            StringUtils::formatString("target (0x%04X) is not bound.", target)
                .c_str());
        return;
    }

    if (border != 0) {
        setGLError(GL_INVALID_VALUE, "border must be 0.");
        return;
    }

    if (!checkInternalFormat(internalformat, format, type)) {
        return;
    }

    handleTexImageWithArrayBufferView(
        target, level, width, height, depth, format, type, srcData, srcOffset,
        [&](const TexImageHelper* helper) {
            STARFISH_ASSERT(helper != nullptr);
            gl()->texImage3D(target, level, internalformat, width, height,
                             depth, 0, format, type, helper->data());
        },
        [&](const std::vector<GLubyte>& blackData) {
            gl()->texImage3D(target, level, internalformat, width, height,
                             depth, 0, format, type, blackData.data());
        },
        [&](const std::vector<GLushort>& blackData) {
            gl()->texImage3D(target, level, internalformat, width, height,
                             depth, 0, format, type, blackData.data());
        });
}

void WebGL2RenderingContext::texSubImage3D(GLenum target, GLint level,
                                           GLint xoffset, GLint yoffset,
                                           GLint zoffset, GLsizei width,
                                           GLsizei height, GLsizei depth,
                                           GLenum format, GLenum type,
                                           GLintptr pboOffset)
{
    ENTER_CONTEXT_SCOPE();

    if (target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY) {
        setGLError(GL_INVALID_ENUM, "target is invalid.");
        return;
    }

    if (!hasBoundTexture(target)) {
        setGLError(
            GL_INVALID_OPERATION,
            StringUtils::formatString("target (0x%04X) is not bound.", target)
                .c_str());
        return;
    }

    if (!getState()->getBoundBuffer(GL_PIXEL_UNPACK_BUFFER).hasValue() ||
        getState()->getBoundBuffer(GL_PIXEL_UNPACK_BUFFER).value() == nullptr) {
        setGLError(GL_INVALID_OPERATION, "PIXEL_UNPACK_BUFFER is not bound.");
        return;
    }

    gl()->texSubImage3D(target, level, xoffset, yoffset, zoffset, width, height,
                        depth, format, type,
                        reinterpret_cast<const void*>(pboOffset));
}

void WebGL2RenderingContext::texSubImage3D(GLenum target, GLint level,
                                           GLint xoffset, GLint yoffset,
                                           GLint zoffset, GLsizei width,
                                           GLsizei height, GLsizei depth,
                                           GLenum format, GLenum type,
                                           TexImageSource source)
{
    ENTER_CONTEXT_SCOPE();

    if (target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY) {
        setGLError(GL_INVALID_ENUM, "target is invalid.");
        return;
    }

    if (!hasBoundTexture(target)) {
        setGLError(
            GL_INVALID_OPERATION,
            StringUtils::formatString("target (0x%04X) is not bound.", target)
                .c_str());
        return;
    }

    GLint skipPixels = 0;
    GLint skipRows = 0;
    GLint imageHeight = 0;
    gl()->getIntegerv(GL_UNPACK_SKIP_PIXELS, &skipPixels);
    gl()->getIntegerv(GL_UNPACK_SKIP_ROWS, &skipRows);
    gl()->getIntegerv(GL_UNPACK_IMAGE_HEIGHT, &imageHeight);

    handleTexImageWithImageSource(
        format, type, source,
        [&](const TexImageHelper* helper) {
            STARFISH_ASSERT(helper != nullptr);
            gl()->texSubImage3D(
                target, level, xoffset, yoffset, zoffset, width, height, depth,
                helper->dataFormat().valueOr(format), type, helper->data());
        },
        height, skipPixels, skipRows, width, height, depth, imageHeight);
}

void WebGL2RenderingContext::texSubImage3D(
    GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
    GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type,
    Optional<ScriptArrayBufferView> srcData, unsigned long long srcOffset)
{
    ENTER_CONTEXT_SCOPE();

    if (target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY) {
        setGLError(GL_INVALID_ENUM, "target is invalid.");
        return;
    }

    if (!hasBoundTexture(target)) {
        setGLError(
            GL_INVALID_OPERATION,
            StringUtils::formatString("target (0x%04X) is not bound.", target)
                .c_str());
        return;
    }

    if (!srcData.hasValue()) {
        setGLError(GL_INVALID_VALUE, "srcData is null.");
        return;
    }

    handleTexImageWithArrayBufferView(
        target, level, width, height, depth, format, type, srcData, srcOffset,
        [&](const TexImageHelper* helper) {
            STARFISH_ASSERT(helper != nullptr);
            gl()->texSubImage3D(target, level, xoffset, yoffset, zoffset, width,
                                height, depth, format, type, helper->data());
        },
        [&](const std::vector<GLubyte>& blackData) {
            gl()->texSubImage3D(target, level, xoffset, yoffset, zoffset, width,
                                height, depth, format, type, blackData.data());
        },
        [&](const std::vector<GLushort>& blackData) {
            gl()->texSubImage3D(target, level, xoffset, yoffset, zoffset, width,
                                height, depth, format, type, blackData.data());
        });
}

void WebGL2RenderingContext::texSubImage2D(GLenum target, GLint level,
                                           GLint xoffset, GLint yoffset,
                                           GLsizei width, GLsizei height,
                                           GLenum format, GLenum type,
                                           GLintptr pboOffset)
{
    ENTER_CONTEXT_SCOPE();

    if (!hasBoundTexture(target)) {
        setGLError(
            GL_INVALID_OPERATION,
            StringUtils::formatString("target (0x%04X) is not bound.", target)
                .c_str());
        return;
    }

    if (!getState()->getBoundBuffer(GL_PIXEL_UNPACK_BUFFER).hasValue() ||
        getState()->getBoundBuffer(GL_PIXEL_UNPACK_BUFFER).value() == nullptr) {
        setGLError(GL_INVALID_OPERATION, "PIXEL_UNPACK_BUFFER is not bound.");
        return;
    }

    gl()->texSubImage2D(target, level, xoffset, yoffset, width, height, format,
                        type, reinterpret_cast<const void*>(pboOffset));
}

void WebGL2RenderingContext::texSubImage2D(GLenum target, GLint level,
                                           GLint xoffset, GLint yoffset,
                                           GLsizei width, GLsizei height,
                                           GLenum format, GLenum type,
                                           TexImageSource source)
{
    ENTER_CONTEXT_SCOPE();

    if (!hasBoundTexture(target)) {
        setGLError(
            GL_INVALID_OPERATION,
            StringUtils::formatString("target (0x%04X) is not bound.", target)
                .c_str());
        return;
    }

    GLint skipPixels = 0;
    GLint skipRows = 0;
    gl()->getIntegerv(GL_UNPACK_SKIP_PIXELS, &skipPixels);
    gl()->getIntegerv(GL_UNPACK_SKIP_ROWS, &skipRows);

    handleTexImageWithImageSource(
        format, type, source,
        [&](const TexImageHelper* helper) {
            STARFISH_ASSERT(helper != nullptr);
            gl()->texSubImage2D(target, level, xoffset, yoffset, width, height,
                                helper->dataFormat().valueOr(format), type,
                                helper->data());
        },
        0, skipPixels, skipRows, width, height, 1, 0);
}

void WebGL2RenderingContext::texSubImage2D(GLenum target, GLint level,
                                           GLint xoffset, GLint yoffset,
                                           GLsizei width, GLsizei height,
                                           GLenum format, GLenum type,
                                           ScriptArrayBufferView srcData,
                                           unsigned long long srcOffset)
{
    ENTER_CONTEXT_SCOPE();

    if (!hasBoundTexture(target)) {
        setGLError(
            GL_INVALID_OPERATION,
            StringUtils::formatString("target (0x%04X) is not bound.", target)
                .c_str());
        return;
    }

    handleTexImageWithArrayBufferView(
        target, level, width, height, 1, format, type, srcData, srcOffset,
        [&](const TexImageHelper* helper) {
            STARFISH_ASSERT(helper != nullptr);
            gl()->texSubImage2D(target, level, xoffset, yoffset, width, height,
                                format, type, helper->data());
        },
        [&](const std::vector<GLubyte>& blackData) {
            gl()->texSubImage2D(target, level, xoffset, yoffset, width, height,
                                format, type, blackData.data());
        },
        [&](const std::vector<GLushort>& blackData) {
            gl()->texSubImage2D(target, level, xoffset, yoffset, width, height,
                                format, type, blackData.data());
        });
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

void WebGL2RenderingContext::uniform1fv(
    Optional<WebGLUniformLocation*> location, Float32List data,
    unsigned long long srcOffset, GLuint srcLength)
{
    implementUniformNfv(1, &GL::uniform1fv, location, data, srcOffset,
                        srcLength);
}

void WebGL2RenderingContext::uniform2fv(
    Optional<WebGLUniformLocation*> location, Float32List data,
    unsigned long long srcOffset, GLuint srcLength)
{
    implementUniformNfv(2, &GL::uniform2fv, location, data, srcOffset,
                        srcLength);
}

void WebGL2RenderingContext::uniform3fv(
    Optional<WebGLUniformLocation*> location, Float32List data,
    unsigned long long srcOffset, GLuint srcLength)
{
    implementUniformNfv(3, &GL::uniform3fv, location, data, srcOffset,
                        srcLength);
}

void WebGL2RenderingContext::uniform4fv(
    Optional<WebGLUniformLocation*> location, Float32List data,
    unsigned long long srcOffset, GLuint srcLength)
{
    implementUniformNfv(4, &GL::uniform4fv, location, data, srcOffset,
                        srcLength);
}

void WebGL2RenderingContext::uniform1iv(
    Optional<WebGLUniformLocation*> location, Int32List data,
    unsigned long long srcOffset, GLuint srcLength)
{
    implementUniformNiv(1, &GL::uniform1iv, location, data, srcOffset,
                        srcLength);
}

void WebGL2RenderingContext::uniform2iv(
    Optional<WebGLUniformLocation*> location, Int32List data,
    unsigned long long srcOffset, GLuint srcLength)
{
    implementUniformNiv(2, &GL::uniform2iv, location, data, srcOffset,
                        srcLength);
}

void WebGL2RenderingContext::uniform3iv(
    Optional<WebGLUniformLocation*> location, Int32List data,
    unsigned long long srcOffset, GLuint srcLength)
{
    implementUniformNiv(3, &GL::uniform3iv, location, data, srcOffset,
                        srcLength);
}

void WebGL2RenderingContext::uniform4iv(
    Optional<WebGLUniformLocation*> location, Int32List data,
    unsigned long long srcOffset, GLuint srcLength)
{
    implementUniformNiv(4, &GL::uniform4iv, location, data, srcOffset,
                        srcLength);
}

void WebGL2RenderingContext::uniformMatrix2fv(
    Optional<WebGLUniformLocation*> location, GLboolean transpose,
    Float32List data, unsigned long long srcOffset, GLuint srcLength)
{
    implementUniformMatrixMxNfv(2, 2, &GL::uniformMatrix2fv, location,
                                transpose, data, srcOffset, srcLength);
}

void WebGL2RenderingContext::uniformMatrix3fv(
    Optional<WebGLUniformLocation*> location, GLboolean transpose,
    Float32List data, unsigned long long srcOffset, GLuint srcLength)
{
    implementUniformMatrixMxNfv(3, 3, &GL::uniformMatrix3fv, location,
                                transpose, data, srcOffset, srcLength);
}

void WebGL2RenderingContext::uniformMatrix4fv(
    Optional<WebGLUniformLocation*> location, GLboolean transpose,
    Float32List data, unsigned long long srcOffset, GLuint srcLength)
{
    implementUniformMatrixMxNfv(4, 4, &GL::uniformMatrix4fv, location,
                                transpose, data, srcOffset, srcLength);
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
