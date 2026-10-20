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
#include "core/dom/canvas/webgl/WebGLActiveInfo.h"
#include "core/dom/canvas/webgl/WebGLBuffer.h"
#include "core/dom/canvas/webgl/WebGLFramebuffer.h"
#include "core/dom/canvas/webgl/WebGLProgram.h"
#include "core/dom/canvas/webgl/WebGLShader.h"
#include "core/dom/canvas/webgl/WebGLTexture.h"
#include "core/dom/canvas/webgl/WebGLRenderingContextState.h"
#include "core/dom/canvas/webgl/WebGLUniformLocation.h"
#include "core/dom/canvas/HTMLCanvasElement.h"
#include "core/page/Window.h"
#include "core/util/debug/Trace.h"
#include "platform/canvas/gl/GL.h"
#include "platform/canvas/gl/IncludeGL.h"
#include <EscargotPublic.h>

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

void WebGL2RenderingContext::clear(uint32_t mask)
{
    if (mask & GL_COLOR_BUFFER_BIT) {
        Optional<WebGLFramebuffer*> maybeFb =
            getBoundFramebuffer(GL_DRAW_FRAMEBUFFER);
        if (maybeFb.hasValue() && maybeFb.value()) {
            WebGLFramebuffer* fb = maybeFb.value();
            const GLenum* drawBuffers = fb->drawBuffers();
            size_t count = fb->drawBufferCount();
            if (count == 0) {
                count = 1;
            }
            for (size_t i = 0; i < count; ++i) {
                GLenum b = drawBuffers[i];
                if (b >= GL_COLOR_ATTACHMENT0 &&
                    b < GL_COLOR_ATTACHMENT0 + 16) {
                    size_t idx = b - GL_COLOR_ATTACHMENT0;
                    GLenum fmt = fb->getColorAttachmentFormat(idx);
                    if (WebGLFramebuffer::isIntegerInternalFormat(fmt)) {
                        setGLError(GL_INVALID_OPERATION,
                                   "clear(COLOR_BUFFER_BIT) not allowed on "
                                   "integer color buffers.");
                        return;
                    }
                }
            }
        }
    }

    WebGLRenderingContext::clear(mask);
}

bool WebGL2RenderingContext::isDefaultFramebufferBound()
{
    return !m_drawFramebuffer.hasValue() ||
           m_drawFramebuffer.value() == nullptr;
}

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

void WebGL2RenderingContext::deleteBuffer(Optional<WebGLBuffer*> maybe)
{
    if (maybe.hasValue()) {
        WebGLBuffer* buffer = maybe.value();
        for (size_t i = 0; i < m_uniformBufferBindings.size(); ++i) {
            if (m_uniformBufferBindings[i] &&
                m_uniformBufferBindings[i]->buffer == buffer) {
                m_uniformBufferBindings[i] = nullptr;
            }
        }
        for (size_t i = 0; i < m_transformFeedbackBufferBindings.size(); ++i) {
            if (m_transformFeedbackBufferBindings[i] &&
                m_transformFeedbackBufferBindings[i]->buffer == buffer) {
                m_transformFeedbackBufferBindings[i] = nullptr;
            }
        }
    }
    WebGLRenderingContext::deleteBuffer(maybe);
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

        if (target == GL_FRAMEBUFFER) {
            m_readFramebuffer = maybeFramebuffer;
            m_drawFramebuffer = maybeFramebuffer;
        } else if (target == GL_READ_FRAMEBUFFER) {
            m_readFramebuffer = maybeFramebuffer;
        } else if (target == GL_DRAW_FRAMEBUFFER) {
            m_drawFramebuffer = maybeFramebuffer;
        }

        if (target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER) {
            gl()->drawBuffers(frameBuffer->drawBufferCount(),
                              frameBuffer->drawBuffers());
        }
        if (target == GL_FRAMEBUFFER || target == GL_READ_FRAMEBUFFER) {
            gl()->readBuffer(frameBuffer->readBuffer());
        }
    } else {
        gl()->bindFramebuffer(target, m_framebufferTexture->fbo());
        getState()->setWebGLFramebuffer(nullptr);

        if (target == GL_FRAMEBUFFER) {
            m_readFramebuffer = maybeFramebuffer;
            m_drawFramebuffer = maybeFramebuffer;
        } else if (target == GL_READ_FRAMEBUFFER) {
            m_readFramebuffer = maybeFramebuffer;
        } else if (target == GL_DRAW_FRAMEBUFFER) {
            m_drawFramebuffer = maybeFramebuffer;
        }

        if (target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER) {
            GLenum att0 = GL_COLOR_ATTACHMENT0;
            gl()->drawBuffers(1, &att0);
        }
        if (target == GL_FRAMEBUFFER || target == GL_READ_FRAMEBUFFER) {
            gl()->readBuffer(m_defaultReadBuffer == GL_BACK
                                 ? GL_COLOR_ATTACHMENT0
                                 : GL_NONE);
        }
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
    case GL_MIN_PROGRAM_TEXEL_OFFSET:
        return createScriptValue(-8);
    case GL_MAX_PROGRAM_TEXEL_OFFSET:
        return createScriptValue(7);
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
    case GL_MAX_SAMPLES:
    case GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS:
    case GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS:
    case GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS:
    case GL_MAX_UNIFORM_BUFFER_BINDINGS:
    case GL_MAX_VARYING_COMPONENTS:
    case GL_MAX_VERTEX_OUTPUT_COMPONENTS:
    case GL_MAX_VERTEX_UNIFORM_BLOCKS:
    case GL_MAX_VERTEX_UNIFORM_COMPONENTS:
    case GL_PACK_ROW_LENGTH:
    case GL_PACK_SKIP_PIXELS:
    case GL_PACK_SKIP_ROWS:
    case GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT:
    case GL_UNPACK_IMAGE_HEIGHT:
    case GL_FRAGMENT_SHADER_DERIVATIVE_HINT:
    case GL_UNPACK_ROW_LENGTH:
    case GL_UNPACK_SKIP_IMAGES:
    case GL_UNPACK_SKIP_PIXELS:
    case GL_UNPACK_SKIP_ROWS: {
        std::vector<GLint> values(1);
        gl()->getIntegerv(pname, &values[0]);
        return createScriptValue(values[0]);
    }
    case GL_DRAW_BUFFER0:
    case GL_DRAW_BUFFER1:
    case GL_DRAW_BUFFER2:
    case GL_DRAW_BUFFER3:
    case GL_DRAW_BUFFER4:
    case GL_DRAW_BUFFER5:
    case GL_DRAW_BUFFER6:
    case GL_DRAW_BUFFER7:
    case GL_DRAW_BUFFER8:
    case GL_DRAW_BUFFER9:
    case GL_DRAW_BUFFER10:
    case GL_DRAW_BUFFER11:
    case GL_DRAW_BUFFER12:
    case GL_DRAW_BUFFER13:
    case GL_DRAW_BUFFER14:
    case GL_DRAW_BUFFER15: {
        size_t index = pname - GL_DRAW_BUFFER0;
        if (isDefaultFramebufferBound()) {
            if (index == 0) {
                return createScriptValue(static_cast<uint32_t>(GL_BACK));
            }
            return createScriptValue(static_cast<uint32_t>(GL_NONE));
        }
        Optional<WebGLFramebuffer*> maybeFb = getState()->webGLFramebuffer();
        if (maybeFb.hasValue() && maybeFb.value()) {
            WebGLFramebuffer* fb = maybeFb.value();
            if (index < fb->drawBufferCount()) {
                return createScriptValue(
                    static_cast<uint32_t>(fb->drawBuffers()[index]));
            }
        }
        return createScriptValue(static_cast<uint32_t>(GL_NONE));
    }
    case GL_READ_BUFFER: {
        if (isDefaultFramebufferBound()) {
            return createScriptValue(
                static_cast<uint32_t>(m_defaultReadBuffer));
        }
        Optional<WebGLFramebuffer*> maybeFb = getState()->webGLFramebuffer();
        if (maybeFb.hasValue() && maybeFb.value()) {
            return createScriptValue(
                static_cast<uint32_t>(maybeFb.value()->readBuffer()));
        }
        return createScriptValue(static_cast<uint32_t>(GL_COLOR_ATTACHMENT0));
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
    // WebGLTexture
    case GL_TEXTURE_BINDING_3D:
    case GL_TEXTURE_BINDING_2D_ARRAY: {
        GLint value = 0;
        gl()->getIntegerv(pname, &value);
        if (value == 0) {
            return scriptNull();
        }
        WebGLTexture* texture = getBoundTextureObject(
            pname == GL_TEXTURE_BINDING_3D ? GL_TEXTURE_3D
                                           : GL_TEXTURE_2D_ARRAY);
        if (texture == nullptr || texture->isDeleted() ||
            static_cast<GLint>(texture->glObject()) != value) {
            return scriptNull();
        }
        return texture->scriptValue();
    }
    // WebGLBuffer
    case GL_COPY_READ_BUFFER_BINDING:
    case GL_COPY_WRITE_BUFFER_BINDING:
    case GL_PIXEL_PACK_BUFFER_BINDING:
    case GL_PIXEL_UNPACK_BUFFER_BINDING:
    case GL_TRANSFORM_FEEDBACK_BUFFER_BINDING:
    case GL_UNIFORM_BUFFER_BINDING: {
        GLenum target;
        switch (pname) {
        case GL_COPY_READ_BUFFER_BINDING:
            target = GL_COPY_READ_BUFFER;
            break;
        case GL_COPY_WRITE_BUFFER_BINDING:
            target = GL_COPY_WRITE_BUFFER;
            break;
        case GL_PIXEL_PACK_BUFFER_BINDING:
            target = GL_PIXEL_PACK_BUFFER;
            break;
        case GL_PIXEL_UNPACK_BUFFER_BINDING:
            target = GL_PIXEL_UNPACK_BUFFER;
            break;
        case GL_TRANSFORM_FEEDBACK_BUFFER_BINDING:
            target = GL_TRANSFORM_FEEDBACK_BUFFER;
            break;
        case GL_UNIFORM_BUFFER_BINDING:
            target = GL_UNIFORM_BUFFER;
            break;
        default:
            target = GL_UNIFORM_BUFFER;
            break;
        }
        WebGLBuffer* buffer =
            getState()->getBoundBuffer(target).valueOr(nullptr);
        if (buffer && !buffer->isDeleted()) {
            return buffer->scriptValue();
        }
        return scriptNull();
    }
    case GL_READ_FRAMEBUFFER_BINDING: {
        if (!m_readFramebuffer.hasValue() ||
            m_readFramebuffer.value()->isDeleted()) {
            return scriptNull();
        }
        return m_readFramebuffer.value()->scriptValue();
    }
    case GL_DRAW_FRAMEBUFFER_BINDING: {
        if (!m_drawFramebuffer.hasValue() ||
            m_drawFramebuffer.value()->isDeleted()) {
            return scriptNull();
        }
        return m_drawFramebuffer.value()->scriptValue();
    }
    case GL_SAMPLER_BINDING: {
        GLenum activeUnit = activeTextureUnit();
        if (activeUnit >= GL_TEXTURE0) {
            size_t unit = activeUnit - GL_TEXTURE0;
            if (unit < m_samplerBindings.size() && m_samplerBindings[unit] &&
                !m_samplerBindings[unit]->isDeleted()) {
                return m_samplerBindings[unit]->scriptValue();
            }
        }
        return scriptNull();
    }
    case GL_TRANSFORM_FEEDBACK_BINDING: {
        if (m_boundTransformFeedback &&
            !m_boundTransformFeedback->isDeleted()) {
            return m_boundTransformFeedback->scriptValue();
        }
        return scriptNull();
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

    switch (pname) {
    case GL_DELETE_STATUS:
    case GL_LINK_STATUS:
    case GL_VALIDATE_STATUS:
    case GL_ATTACHED_SHADERS:
    case GL_ACTIVE_ATTRIBUTES:
    case GL_ACTIVE_UNIFORMS:
    case GL_ACTIVE_UNIFORM_BLOCKS:
    case GL_TRANSFORM_FEEDBACK_BUFFER_MODE:
    case GL_TRANSFORM_FEEDBACK_VARYINGS:
        break;
    default:
        setGLError(GL_INVALID_ENUM);
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
    // GLint
    case GL_ACTIVE_ATTRIBUTES:
    case GL_ACTIVE_UNIFORMS:
    case GL_ATTACHED_SHADERS:
    case GL_ACTIVE_UNIFORM_BLOCKS:
    case GL_TRANSFORM_FEEDBACK_VARYINGS:
        return createScriptValue(params);
    case GL_TRANSFORM_FEEDBACK_BUFFER_MODE:
        return createScriptValue(static_cast<GLenum>(params));
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
        return createScriptValue(value != 0);
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

        Optional<WebGLBuffer*> maybeBuffer =
            getState()->getBufferBoundToVertexAttributes(index);

        if (!maybeBuffer.hasValue() || !maybeBuffer.value()) {
            return scriptNull();
        }

        TRACE(WEBGL, KV(index), KV(maybeBuffer.value()->glObject()));

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
    if (!hasNewGLError()) {
        WebGLTexture* texture = getBoundTextureObject(target);
        if (texture) {
            if (pname == GL_TEXTURE_BASE_LEVEL) {
                texture->setBaseLevel(static_cast<GLint>(param));
            } else if (pname == GL_TEXTURE_MAX_LEVEL) {
                texture->setMaxLevel(static_cast<GLint>(param));
            }
        }
    }
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

    if (pname == GL_TEXTURE_WRAP_S || pname == GL_TEXTURE_WRAP_T ||
        pname == GL_TEXTURE_WRAP_R) {
        if (param != GL_REPEAT && param != GL_CLAMP_TO_EDGE &&
            param != GL_MIRRORED_REPEAT) {
            setGLError(GL_INVALID_ENUM);
            return;
        }
    } else if (pname == GL_TEXTURE_MAG_FILTER) {
        if (param != GL_NEAREST && param != GL_LINEAR) {
            setGLError(GL_INVALID_ENUM);
            return;
        }
    } else if (pname == GL_TEXTURE_MIN_FILTER) {
        if (param != GL_NEAREST && param != GL_LINEAR &&
            param != GL_NEAREST_MIPMAP_NEAREST &&
            param != GL_LINEAR_MIPMAP_NEAREST &&
            param != GL_NEAREST_MIPMAP_LINEAR &&
            param != GL_LINEAR_MIPMAP_LINEAR) {
            setGLError(GL_INVALID_ENUM);
            return;
        }
    } else if (pname == GL_TEXTURE_COMPARE_MODE) {
        if (param != GL_NONE && param != GL_COMPARE_REF_TO_TEXTURE) {
            setGLError(GL_INVALID_ENUM);
            return;
        }
    } else if (pname == GL_TEXTURE_COMPARE_FUNC) {
        if (param != GL_LEQUAL && param != GL_GEQUAL && param != GL_LESS &&
            param != GL_GREATER && param != GL_EQUAL && param != GL_NOTEQUAL &&
            param != GL_ALWAYS && param != GL_NEVER) {
            setGLError(GL_INVALID_ENUM);
            return;
        }
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
    if (!hasNewGLError()) {
        WebGLTexture* texture = getBoundTextureObject(target);
        if (texture) {
            if (pname == GL_TEXTURE_BASE_LEVEL) {
                texture->setBaseLevel(param);
            } else if (pname == GL_TEXTURE_MAX_LEVEL) {
                texture->setMaxLevel(param);
            }
        }
    }
}

// WebGL2RenderingContextBase

void WebGL2RenderingContext::copyBufferSubData(GLenum readTarget,
                                               GLenum writeTarget,
                                               GLintptr readOffset,
                                               GLintptr writeOffset,
                                               GLsizeiptr size)
{
    ENTER_CONTEXT_SCOPE();

    auto isValidBufferTarget = [](GLenum target) -> bool {
        switch (target) {
        case GL_ARRAY_BUFFER:
        case GL_COPY_READ_BUFFER:
        case GL_COPY_WRITE_BUFFER:
        case GL_ELEMENT_ARRAY_BUFFER:
        case GL_PIXEL_PACK_BUFFER:
        case GL_PIXEL_UNPACK_BUFFER:
        case GL_TRANSFORM_FEEDBACK_BUFFER:
        case GL_UNIFORM_BUFFER:
            return true;
        default:
            return false;
        }
    };

    if (!isValidBufferTarget(readTarget) || !isValidBufferTarget(writeTarget)) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    if (readOffset < 0 || writeOffset < 0 || size < 0) {
        setGLError(GL_INVALID_VALUE);
        return;
    }

    WebGLBuffer* readBuffer =
        getState()->getBoundBuffer(readTarget).valueOr(nullptr);
    WebGLBuffer* writeBuffer =
        getState()->getBoundBuffer(writeTarget).valueOr(nullptr);

    if (!readBuffer || readBuffer->isDeleted() || !writeBuffer ||
        writeBuffer->isDeleted()) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    GLenum readTargetType = (readTarget == GL_COPY_READ_BUFFER ||
                             readTarget == GL_COPY_WRITE_BUFFER)
                                ? readBuffer->target()
                                : readTarget;
    GLenum writeTargetType = (writeTarget == GL_COPY_READ_BUFFER ||
                              writeTarget == GL_COPY_WRITE_BUFFER)
                                 ? writeBuffer->target()
                                 : writeTarget;

    if (readBuffer != writeBuffer &&
        ((readTargetType == GL_ELEMENT_ARRAY_BUFFER &&
          writeTargetType != GL_ELEMENT_ARRAY_BUFFER) ||
         (writeTargetType == GL_ELEMENT_ARRAY_BUFFER &&
          readTargetType != GL_ELEMENT_ARRAY_BUFFER))) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    unsigned long long uReadOffset =
        static_cast<unsigned long long>(readOffset);
    unsigned long long uWriteOffset =
        static_cast<unsigned long long>(writeOffset);
    unsigned long long uSize = static_cast<unsigned long long>(size);

    if (uReadOffset + uSize >
            static_cast<unsigned long long>(readBuffer->byteLength()) ||
        uWriteOffset + uSize >
            static_cast<unsigned long long>(writeBuffer->byteLength())) {
        setGLError(GL_INVALID_VALUE);
        return;
    }

    if (readBuffer == writeBuffer) {
        GLintptr readEnd = readOffset + size;
        GLintptr writeEnd = writeOffset + size;
        if (readEnd > writeOffset && writeEnd > readOffset) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
    }

    if (size == 0) {
        return;
    }

    gl()->copyBufferSubData(readTarget, writeTarget, readOffset, writeOffset,
                            size);
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

    if (m_boundTransformFeedback && m_boundTransformFeedback->isActive() &&
        !m_boundTransformFeedback->isPaused()) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

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
    getState()->setGenericVertexAttribType(index, GL_FLOAT);
}

void WebGL2RenderingContext::vertexAttrib2f(GLuint index, GLfloat x, GLfloat y)
{
    WebGLRenderingContext::vertexAttrib2f(index, x, y);
    m_currentVertexAttribType = GL_FLOAT;
    getState()->setGenericVertexAttribType(index, GL_FLOAT);
}

void WebGL2RenderingContext::vertexAttrib3f(GLuint index, GLfloat x, GLfloat y,
                                            GLfloat z)
{
    WebGLRenderingContext::vertexAttrib3f(index, x, y, z);
    m_currentVertexAttribType = GL_FLOAT;
    getState()->setGenericVertexAttribType(index, GL_FLOAT);
}

void WebGL2RenderingContext::vertexAttrib4f(GLuint index, GLfloat x, GLfloat y,
                                            GLfloat z, GLfloat w)
{
    WebGLRenderingContext::vertexAttrib4f(index, x, y, z, w);
    m_currentVertexAttribType = GL_FLOAT;
    getState()->setGenericVertexAttribType(index, GL_FLOAT);
}

void WebGL2RenderingContext::vertexAttrib1fv(GLuint index, Float32List values)
{
    WebGLRenderingContext::vertexAttrib1fv(index, values);
    m_currentVertexAttribType = GL_FLOAT;
    getState()->setGenericVertexAttribType(index, GL_FLOAT);
}

void WebGL2RenderingContext::vertexAttrib2fv(GLuint index, Float32List values)
{
    WebGLRenderingContext::vertexAttrib2fv(index, values);
    m_currentVertexAttribType = GL_FLOAT;
    getState()->setGenericVertexAttribType(index, GL_FLOAT);
}

void WebGL2RenderingContext::vertexAttrib3fv(GLuint index, Float32List values)
{
    WebGLRenderingContext::vertexAttrib3fv(index, values);
    m_currentVertexAttribType = GL_FLOAT;
    getState()->setGenericVertexAttribType(index, GL_FLOAT);
}

void WebGL2RenderingContext::vertexAttrib4fv(GLuint index, Float32List values)
{
    WebGLRenderingContext::vertexAttrib4fv(index, values);
    m_currentVertexAttribType = GL_FLOAT;
    getState()->setGenericVertexAttribType(index, GL_FLOAT);
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
    getState()->setGenericVertexAttribType(index, GL_INT);
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
    getState()->setGenericVertexAttribType(index, GL_INT);
}

void WebGL2RenderingContext::vertexAttribI4ui(GLuint index, GLuint x, GLuint y,
                                              GLuint z, GLuint w)
{
    ENTER_CONTEXT_SCOPE();

    gl()->vertexAttribI4ui(index, x, y, z, w);
    m_currentVertexAttribType = GL_UNSIGNED_INT;
    getState()->setGenericVertexAttribType(index, GL_UNSIGNED_INT);
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
    getState()->setGenericVertexAttribType(index, GL_UNSIGNED_INT);
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
    GLenum mappedType = (type == GL_UNSIGNED_BYTE ||
                         type == GL_UNSIGNED_SHORT || type == GL_UNSIGNED_INT)
                            ? GL_UNSIGNED_INT
                            : GL_INT;
    getState()->setVertexAttribType(index, mappedType);
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

void WebGL2RenderingContext::transformFeedbackVaryings(
    WebGLProgram* program, GCVector<String*> varyings, GLenum bufferMode)
{
    ENTER_CONTEXT_SCOPE();

    if (!isFromCurrentContext(program)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    if (bufferMode != GL_INTERLEAVED_ATTRIBS &&
        bufferMode != GL_SEPARATE_ATTRIBS) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    if (bufferMode == GL_SEPARATE_ATTRIBS) {
        GLint maxSeparateAttribs = 0;
        gl()->getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS,
                          &maxSeparateAttribs);
        if (static_cast<GLint>(varyings.size()) > maxSeparateAttribs) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
    }

    std::vector<std::string> strings;
    std::vector<const char*> cstrings;
    strings.reserve(varyings.size());
    cstrings.reserve(varyings.size());

    for (size_t i = 0; i < varyings.size(); ++i) {
        strings.push_back(varyings[i]->toUTF8NonGCString());
        cstrings.push_back(strings.back().c_str());
    }

    gl()->transformFeedbackVaryings(program->glObject(),
                                    static_cast<GLsizei>(cstrings.size()),
                                    cstrings.data(), bufferMode);

    program->setTransformFeedbackBufferMode(bufferMode);
    program->setTransformFeedbackVaryingsCount(
        static_cast<GLint>(varyings.size()));
}

Optional<WebGLFramebuffer*> WebGL2RenderingContext::getBoundFramebuffer(
    GLenum target)
{
    if (target == GL_READ_FRAMEBUFFER) {
        return m_readFramebuffer;
    }
    return m_drawFramebuffer;
}

void WebGL2RenderingContext::bindBufferBase(GLenum target, GLuint index,
                                            Optional<WebGLBuffer*> buffer)
{
    ENTER_CONTEXT_SCOPE();

    if (target != GL_TRANSFORM_FEEDBACK_BUFFER && target != GL_UNIFORM_BUFFER) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    if (target == GL_UNIFORM_BUFFER) {
        GLint maxBindings = 0;
        gl()->getIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxBindings);
        if (index >= static_cast<GLuint>(maxBindings)) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
    } else if (target == GL_TRANSFORM_FEEDBACK_BUFFER) {
        GLint maxBindings = 0;
        gl()->getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS,
                          &maxBindings);
        if (index >= static_cast<GLuint>(maxBindings)) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
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
        } else if (target == GL_TRANSFORM_FEEDBACK_BUFFER) {
            if (index >= m_transformFeedbackBufferBindings.size()) {
                m_transformFeedbackBufferBindings.resize(index + 1);
            }
            m_transformFeedbackBufferBindings[index] =
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
        } else if (target == GL_TRANSFORM_FEEDBACK_BUFFER &&
                   index < m_transformFeedbackBufferBindings.size()) {
            m_transformFeedbackBufferBindings[index] = nullptr;
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

    if (offset < 0 || size <= 0) {
        setGLError(GL_INVALID_VALUE);
        return;
    }

    if (target == GL_UNIFORM_BUFFER) {
        GLint maxBindings = 0;
        gl()->getIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxBindings);
        if (index >= static_cast<GLuint>(maxBindings)) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
        GLint alignment = 1;
        gl()->getIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &alignment);
        if (alignment > 0 && (offset % alignment) != 0) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
    } else if (target == GL_TRANSFORM_FEEDBACK_BUFFER) {
        GLint maxBindings = 0;
        gl()->getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS,
                          &maxBindings);
        if (index >= static_cast<GLuint>(maxBindings)) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
        if ((offset % 4) != 0 || (size % 4) != 0) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
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
        } else if (target == GL_TRANSFORM_FEEDBACK_BUFFER) {
            if (index >= m_transformFeedbackBufferBindings.size()) {
                m_transformFeedbackBufferBindings.resize(index + 1);
            }
            m_transformFeedbackBufferBindings[index] =
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
        } else if (target == GL_TRANSFORM_FEEDBACK_BUFFER &&
                   index < m_transformFeedbackBufferBindings.size()) {
            m_transformFeedbackBufferBindings[index] = nullptr;
        }
    }
}

ScriptValue WebGL2RenderingContext::getIndexedParameter(GLenum target,
                                                        GLuint index)
{
    ENTER_CONTEXT_SCOPE(scriptNull());

    switch (target) {
    case GL_TRANSFORM_FEEDBACK_BUFFER_BINDING: {
        GLint maxBindings = 0;
        gl()->getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS,
                          &maxBindings);
        if (index >= static_cast<GLuint>(maxBindings)) {
            setGLError(GL_INVALID_VALUE);
            return scriptNull();
        }
        if (index < m_transformFeedbackBufferBindings.size() &&
            m_transformFeedbackBufferBindings[index] != nullptr &&
            m_transformFeedbackBufferBindings[index]->buffer != nullptr &&
            !m_transformFeedbackBufferBindings[index]->buffer->isDeleted()) {
            return m_transformFeedbackBufferBindings[index]
                ->buffer->scriptValue();
        }
        return scriptNull();
    }
    case GL_TRANSFORM_FEEDBACK_BUFFER_SIZE: {
        GLint maxBindings = 0;
        gl()->getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS,
                          &maxBindings);
        if (index >= static_cast<GLuint>(maxBindings)) {
            setGLError(GL_INVALID_VALUE);
            return scriptNull();
        }
        if (index < m_transformFeedbackBufferBindings.size() &&
            m_transformFeedbackBufferBindings[index] != nullptr &&
            m_transformFeedbackBufferBindings[index]->isRange) {
            return createScriptValue(static_cast<double>(
                m_transformFeedbackBufferBindings[index]->size));
        }
        return createScriptValue(0.0);
    }
    case GL_TRANSFORM_FEEDBACK_BUFFER_START: {
        GLint maxBindings = 0;
        gl()->getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS,
                          &maxBindings);
        if (index >= static_cast<GLuint>(maxBindings)) {
            setGLError(GL_INVALID_VALUE);
            return scriptNull();
        }
        if (index < m_transformFeedbackBufferBindings.size() &&
            m_transformFeedbackBufferBindings[index] != nullptr &&
            m_transformFeedbackBufferBindings[index]->isRange) {
            return createScriptValue(static_cast<double>(
                m_transformFeedbackBufferBindings[index]->offset));
        }
        return createScriptValue(0.0);
    }
    case GL_UNIFORM_BUFFER_BINDING: {
        GLint maxBindings = 0;
        gl()->getIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxBindings);
        if (index >= static_cast<GLuint>(maxBindings)) {
            setGLError(GL_INVALID_VALUE);
            return scriptNull();
        }
        if (index < m_uniformBufferBindings.size() &&
            m_uniformBufferBindings[index] != nullptr &&
            m_uniformBufferBindings[index]->buffer != nullptr &&
            !m_uniformBufferBindings[index]->buffer->isDeleted()) {
            return m_uniformBufferBindings[index]->buffer->scriptValue();
        }
        return scriptNull();
    }
    case GL_UNIFORM_BUFFER_SIZE: {
        GLint maxBindings = 0;
        gl()->getIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxBindings);
        if (index >= static_cast<GLuint>(maxBindings)) {
            setGLError(GL_INVALID_VALUE);
            return scriptNull();
        }
        if (index < m_uniformBufferBindings.size() &&
            m_uniformBufferBindings[index] != nullptr &&
            m_uniformBufferBindings[index]->isRange) {
            return createScriptValue(
                static_cast<double>(m_uniformBufferBindings[index]->size));
        }
        return createScriptValue(0.0);
    }
    case GL_UNIFORM_BUFFER_START: {
        GLint maxBindings = 0;
        gl()->getIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxBindings);
        if (index >= static_cast<GLuint>(maxBindings)) {
            setGLError(GL_INVALID_VALUE);
            return scriptNull();
        }
        if (index < m_uniformBufferBindings.size() &&
            m_uniformBufferBindings[index] != nullptr &&
            m_uniformBufferBindings[index]->isRange) {
            return createScriptValue(
                static_cast<double>(m_uniformBufferBindings[index]->offset));
        }
        return createScriptValue(0.0);
    }
    default:
        setGLError(GL_INVALID_ENUM);
        return scriptNull();
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
    GLint linkStatus = 0;
    gl()->getProgramiv(program->glObject(), GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE || program->linkFailed()) {
        setGLError(GL_INVALID_OPERATION);
        return scriptNull();
    }
    GLint activeBlocks = 0;
    gl()->getProgramiv(program->glObject(), GL_ACTIVE_UNIFORM_BLOCKS,
                       &activeBlocks);
    if (activeBlocks < 0 ||
        uniformBlockIndex >= static_cast<GLuint>(activeBlocks)) {
        setGLError(GL_INVALID_VALUE);
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
    GLint linkStatus = 0;
    gl()->getProgramiv(program->glObject(), GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE || program->linkFailed()) {
        setGLError(GL_INVALID_OPERATION);
        return nullptr;
    }
    GLint activeBlocks = 0;
    gl()->getProgramiv(program->glObject(), GL_ACTIVE_UNIFORM_BLOCKS,
                       &activeBlocks);
    if (activeBlocks < 0 ||
        uniformBlockIndex >= static_cast<GLuint>(activeBlocks)) {
        setGLError(GL_INVALID_VALUE);
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
    program->setUniformBlockBinding(uniformBlockIndex, uniformBlockBinding);
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

    if (!array.hasValue() || array.value() == nullptr) {
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

/* Query Objects */

WebGLQuery* WebGL2RenderingContext::createQuery()
{
    ENTER_CONTEXT_SCOPE(nullptr);
    GLuint query = 0;
    gl()->genQueries(1, &query);
    return new WebGLQuery(scriptBindingInstance(), this, query);
}

void WebGL2RenderingContext::deleteQuery(Optional<WebGLQuery*> query)
{
    ENTER_CONTEXT_SCOPE();
    if (!query.hasValue()) {
        return;
    }
    WebGLQuery* value = query.value();
    if (!isFromCurrentContext(value)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }
    if (value->isDeleted()) {
        return;
    }
    GLuint id = value->glObject();
    gl()->deleteQueries(1, &id);
    value->markDeleted();

    if (m_activeQueryAnySamplesPassed == value) {
        m_activeQueryAnySamplesPassed = nullptr;
    }
    if (m_activeQueryAnySamplesPassedConservative == value) {
        m_activeQueryAnySamplesPassedConservative = nullptr;
    }
    if (m_activeQueryTransformFeedbackPrimitivesWritten == value) {
        m_activeQueryTransformFeedbackPrimitivesWritten = nullptr;
    }
}

GLboolean WebGL2RenderingContext::isQuery(Optional<WebGLQuery*> query)
{
    ENTER_CONTEXT_SCOPE(false);
    if (!query.hasValue()) {
        return false;
    }
    WebGLQuery* value = query.value();
    if (!isFromCurrentContext(value) || value->invalidated()) {
        return false;
    }
    if (!value->hasEverBeenBound()) {
        return false;
    }
    return gl()->isQuery(value->glObject());
}

void WebGL2RenderingContext::beginQuery(GLenum target, WebGLQuery* query)
{
    ENTER_CONTEXT_SCOPE();
    if (!query) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }
    if (!isFromCurrentContext(query)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }
    if (query->isDeleted()) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }
    if (query->isActive()) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }
    if (query->hasEverBeenBound() && query->target() != target) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    WebGLQuery** activeSlot = nullptr;
    switch (target) {
    case GL_ANY_SAMPLES_PASSED:
        activeSlot = &m_activeQueryAnySamplesPassed;
        break;
    case GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
        activeSlot = &m_activeQueryAnySamplesPassedConservative;
        break;
    case GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN:
        activeSlot = &m_activeQueryTransformFeedbackPrimitivesWritten;
        break;
    default:
        setGLError(GL_INVALID_ENUM);
        return;
    }

    if (*activeSlot != nullptr) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    gl()->beginQuery(target, query->glObject());
    if (hasNewGLError()) {
        return;
    }

    query->setTarget(target);
    query->setHasEverBeenBound();
    query->setActive(true);
    query->setCanBeAvailable(false);
    *activeSlot = query;
}

void WebGL2RenderingContext::endQuery(GLenum target)
{
    ENTER_CONTEXT_SCOPE();
    WebGLQuery** activeSlot = nullptr;
    switch (target) {
    case GL_ANY_SAMPLES_PASSED:
        activeSlot = &m_activeQueryAnySamplesPassed;
        break;
    case GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
        activeSlot = &m_activeQueryAnySamplesPassedConservative;
        break;
    case GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN:
        activeSlot = &m_activeQueryTransformFeedbackPrimitivesWritten;
        break;
    default:
        setGLError(GL_INVALID_ENUM);
        return;
    }

    if (*activeSlot == nullptr) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    gl()->endQuery(target);
    if (hasNewGLError()) {
        return;
    }

    WebGLQuery* q = *activeSlot;
    q->setActive(false);
    *activeSlot = nullptr;

    HTMLCanvasElement* canvas = m_ownerHTMLCanvasElement;
    if (canvas && canvas->window()) {
        canvas->window()->requestAnimationFrame(
            [](void* data) {
                WebGLQuery* query = static_cast<WebGLQuery*>(data);
                query->setCanBeAvailable(true);
            },
            q);
    } else {
        q->setCanBeAvailable(true);
    }
}

WebGLQuery* WebGL2RenderingContext::getQuery(GLenum target, GLenum pname)
{
    ENTER_CONTEXT_SCOPE(nullptr);
    if (pname != GL_CURRENT_QUERY) {
        setGLError(GL_INVALID_ENUM);
        return nullptr;
    }
    switch (target) {
    case GL_ANY_SAMPLES_PASSED:
        return m_activeQueryAnySamplesPassed;
    case GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
        return m_activeQueryAnySamplesPassedConservative;
    case GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN:
        return m_activeQueryTransformFeedbackPrimitivesWritten;
    default:
        setGLError(GL_INVALID_ENUM);
        return nullptr;
    }
}

ScriptValue WebGL2RenderingContext::getQueryParameter(WebGLQuery* query,
                                                      GLenum pname)
{
    ENTER_CONTEXT_SCOPE(scriptNull());
    if (!query || !isFromCurrentContext(query) || query->isDeleted()) {
        setGLError(GL_INVALID_OPERATION);
        return scriptNull();
    }
    if (!query->hasEverBeenBound() || query->isActive()) {
        setGLError(GL_INVALID_OPERATION);
        return scriptNull();
    }

    switch (pname) {
    case GL_QUERY_RESULT_AVAILABLE: {
        if (!query->canBeAvailable()) {
            return createScriptValue(false);
        }
        GLuint available = 0;
        gl()->getQueryObjectuiv(query->glObject(), GL_QUERY_RESULT_AVAILABLE,
                                &available);
        if (hasNewGLError()) {
            return scriptNull();
        }
        return createScriptValue(available == GL_TRUE);
    }
    case GL_QUERY_RESULT: {
        GLuint result = 0;
        gl()->getQueryObjectuiv(query->glObject(), GL_QUERY_RESULT, &result);
        if (hasNewGLError()) {
            return scriptNull();
        }
        return createScriptValue(static_cast<double>(result));
    }
    default:
        setGLError(GL_INVALID_ENUM);
        return scriptNull();
    }
}

/* Sampler Objects */

WebGLSampler* WebGL2RenderingContext::createSampler()
{
    ENTER_CONTEXT_SCOPE(nullptr);
    GLuint sampler = 0;
    gl()->genSamplers(1, &sampler);
    return new WebGLSampler(scriptBindingInstance(), this, sampler);
}

void WebGL2RenderingContext::deleteSampler(Optional<WebGLSampler*> sampler)
{
    ENTER_CONTEXT_SCOPE();
    if (!sampler.hasValue()) {
        return;
    }
    WebGLSampler* value = sampler.value();
    if (!isFromCurrentContext(value)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }
    if (value->isDeleted()) {
        return;
    }
    GLuint id = value->glObject();
    gl()->deleteSamplers(1, &id);
    value->markDeleted();

    for (size_t i = 0; i < m_samplerBindings.size(); ++i) {
        if (m_samplerBindings[i] == value) {
            gl()->bindSampler(static_cast<GLuint>(i), 0);
            m_samplerBindings[i] = nullptr;
        }
    }
}

GLboolean WebGL2RenderingContext::isSampler(Optional<WebGLSampler*> sampler)
{
    ENTER_CONTEXT_SCOPE(false);
    if (!sampler.hasValue()) {
        return false;
    }
    WebGLSampler* value = sampler.value();
    if (!isFromCurrentContext(value) || value->invalidated()) {
        return false;
    }
    return !value->isDeleted();
}

void WebGL2RenderingContext::bindSampler(GLuint unit,
                                         Optional<WebGLSampler*> sampler)
{
    ENTER_CONTEXT_SCOPE();
    GLint maxUnits = 0;
    gl()->getIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxUnits);
    if (maxUnits <= 0) {
        maxUnits = 32;
    }
    if (unit >= static_cast<GLuint>(maxUnits)) {
        setGLError(GL_INVALID_VALUE);
        return;
    }

    if (sampler.hasValue()) {
        WebGLSampler* value = sampler.value();
        if (!isFromCurrentContext(value)) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }
        if (value->isDeleted()) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        gl()->bindSampler(unit, value->glObject());
        if (hasNewGLError()) {
            return;
        }
        if (unit >= m_samplerBindings.size()) {
            m_samplerBindings.resize(unit + 1);
        }
        m_samplerBindings[unit] = value;
    } else {
        gl()->bindSampler(unit, 0);
        if (hasNewGLError()) {
            return;
        }
        if (unit < m_samplerBindings.size()) {
            m_samplerBindings[unit] = nullptr;
        }
    }
}

void WebGL2RenderingContext::samplerParameteri(WebGLSampler* sampler,
                                               GLenum pname, GLint param)
{
    ENTER_CONTEXT_SCOPE();
    samplerParameteriImpl(sampler, pname, param);
}

void WebGL2RenderingContext::samplerParameteriImpl(WebGLSampler* sampler,
                                                   GLenum pname, GLint param)
{
    if (!sampler || !isFromCurrentContext(sampler) || sampler->isDeleted()) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    switch (pname) {
    case GL_TEXTURE_WRAP_S:
    case GL_TEXTURE_WRAP_T:
    case GL_TEXTURE_WRAP_R:
        if (param != GL_REPEAT && param != GL_CLAMP_TO_EDGE &&
            param != GL_MIRRORED_REPEAT) {
            setGLError(GL_INVALID_ENUM);
            return;
        }
        break;
    case GL_TEXTURE_MAG_FILTER:
        if (param != GL_NEAREST && param != GL_LINEAR) {
            setGLError(GL_INVALID_ENUM);
            return;
        }
        break;
    case GL_TEXTURE_MIN_FILTER:
        if (param != GL_NEAREST && param != GL_LINEAR &&
            param != GL_NEAREST_MIPMAP_NEAREST &&
            param != GL_LINEAR_MIPMAP_NEAREST &&
            param != GL_NEAREST_MIPMAP_LINEAR &&
            param != GL_LINEAR_MIPMAP_LINEAR) {
            setGLError(GL_INVALID_ENUM);
            return;
        }
        break;
    case GL_TEXTURE_COMPARE_MODE:
        if (param != GL_NONE && param != GL_COMPARE_REF_TO_TEXTURE) {
            setGLError(GL_INVALID_ENUM);
            return;
        }
        break;
    case GL_TEXTURE_COMPARE_FUNC:
        if (param != GL_LEQUAL && param != GL_GEQUAL && param != GL_LESS &&
            param != GL_GREATER && param != GL_EQUAL && param != GL_NOTEQUAL &&
            param != GL_ALWAYS && param != GL_NEVER) {
            setGLError(GL_INVALID_ENUM);
            return;
        }
        break;
    case GL_TEXTURE_MIN_LOD:
    case GL_TEXTURE_MAX_LOD:
        break;
    default:
        setGLError(GL_INVALID_ENUM);
        return;
    }

    gl()->samplerParameteri(sampler->glObject(), pname, param);
}

void WebGL2RenderingContext::samplerParameterf(WebGLSampler* sampler,
                                               GLenum pname, GLfloat param)
{
    ENTER_CONTEXT_SCOPE();
    if (!sampler || !isFromCurrentContext(sampler) || sampler->isDeleted()) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    switch (pname) {
    case GL_TEXTURE_MIN_LOD:
    case GL_TEXTURE_MAX_LOD:
        break;
    case GL_TEXTURE_WRAP_S:
    case GL_TEXTURE_WRAP_T:
    case GL_TEXTURE_WRAP_R:
    case GL_TEXTURE_MAG_FILTER:
    case GL_TEXTURE_MIN_FILTER:
    case GL_TEXTURE_COMPARE_MODE:
    case GL_TEXTURE_COMPARE_FUNC:
        samplerParameteriImpl(sampler, pname, static_cast<GLint>(param));
        return;
    default:
        setGLError(GL_INVALID_ENUM);
        return;
    }

    gl()->samplerParameterf(sampler->glObject(), pname, param);
}

ScriptValue WebGL2RenderingContext::getSamplerParameter(WebGLSampler* sampler,
                                                        GLenum pname)
{
    ENTER_CONTEXT_SCOPE(scriptNull());
    if (!sampler || !isFromCurrentContext(sampler) || sampler->isDeleted()) {
        setGLError(GL_INVALID_OPERATION);
        return scriptNull();
    }

    switch (pname) {
    case GL_TEXTURE_MIN_LOD:
    case GL_TEXTURE_MAX_LOD: {
        GLfloat param = 0.0f;
        gl()->getSamplerParameterfv(sampler->glObject(), pname, &param);
        if (hasNewGLError()) {
            return scriptNull();
        }
        return createScriptValue(static_cast<double>(param));
    }
    case GL_TEXTURE_WRAP_S:
    case GL_TEXTURE_WRAP_T:
    case GL_TEXTURE_WRAP_R:
    case GL_TEXTURE_MAG_FILTER:
    case GL_TEXTURE_MIN_FILTER:
    case GL_TEXTURE_COMPARE_MODE:
    case GL_TEXTURE_COMPARE_FUNC: {
        GLint param = 0;
        gl()->getSamplerParameteriv(sampler->glObject(), pname, &param);
        if (hasNewGLError()) {
            return scriptNull();
        }
        return createScriptValue(static_cast<GLenum>(param));
    }
    default:
        setGLError(GL_INVALID_ENUM);
        return scriptNull();
    }
}

/* Transform Feedback */

WebGLTransformFeedback* WebGL2RenderingContext::createTransformFeedback()
{
    ENTER_CONTEXT_SCOPE(nullptr);
    GLuint tf = 0;
    gl()->genTransformFeedbacks(1, &tf);
    return new WebGLTransformFeedback(scriptBindingInstance(), this, tf);
}

void WebGL2RenderingContext::deleteTransformFeedback(
    Optional<WebGLTransformFeedback*> tf)
{
    ENTER_CONTEXT_SCOPE();
    if (!tf.hasValue()) {
        return;
    }
    WebGLTransformFeedback* value = tf.value();
    if (!isFromCurrentContext(value)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }
    if (value->isDeleted()) {
        return;
    }

    if (value->isActive()) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    GLuint id = value->glObject();
    gl()->deleteTransformFeedbacks(1, &id);
    value->markDeleted();

    if (m_boundTransformFeedback == value) {
        gl()->bindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
        m_boundTransformFeedback = nullptr;
    }
}

GLboolean WebGL2RenderingContext::isTransformFeedback(
    Optional<WebGLTransformFeedback*> tf)
{
    ENTER_CONTEXT_SCOPE(false);
    if (!tf.hasValue()) {
        return false;
    }
    WebGLTransformFeedback* value = tf.value();
    if (!isFromCurrentContext(value) || value->invalidated()) {
        return false;
    }
    if (!value->hasEverBeenBound()) {
        return false;
    }
    return gl()->isTransformFeedback(value->glObject());
}

void WebGL2RenderingContext::bindTransformFeedback(
    GLenum target, Optional<WebGLTransformFeedback*> tf)
{
    ENTER_CONTEXT_SCOPE();
    if (target != GL_TRANSFORM_FEEDBACK) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    if (m_boundTransformFeedback && m_boundTransformFeedback->isActive() &&
        !m_boundTransformFeedback->isPaused()) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    if (tf.hasValue()) {
        WebGLTransformFeedback* value = tf.value();
        if (!isFromCurrentContext(value)) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }
        if (value->isDeleted()) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        gl()->bindTransformFeedback(target, value->glObject());
        if (hasNewGLError()) {
            return;
        }
        value->setHasEverBeenBound();
        m_boundTransformFeedback = value;
    } else {
        gl()->bindTransformFeedback(target, 0);
        if (hasNewGLError()) {
            return;
        }
        m_boundTransformFeedback = nullptr;
    }
}

void WebGL2RenderingContext::beginTransformFeedback(GLenum primitiveMode)
{
    ENTER_CONTEXT_SCOPE();
    if (primitiveMode != GL_POINTS && primitiveMode != GL_LINES &&
        primitiveMode != GL_TRIANGLES) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    Optional<WebGLProgram*> prog = getState()->webGLProgram();
    if (!prog.hasValue() || prog.value() == nullptr ||
        prog.value()->isDeleted()) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    if (m_boundTransformFeedback) {
        if (m_boundTransformFeedback->isActive()) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }
        gl()->beginTransformFeedback(primitiveMode);
        if (hasNewGLError()) {
            return;
        }
        m_boundTransformFeedback->setActive(true);
        m_boundTransformFeedback->setPaused(false);
        m_boundTransformFeedback->setPrimitiveMode(primitiveMode);
        m_boundTransformFeedback->setProgram(prog.value());
    } else {
        gl()->beginTransformFeedback(primitiveMode);
    }
}

void WebGL2RenderingContext::endTransformFeedback()
{
    ENTER_CONTEXT_SCOPE();
    if (m_boundTransformFeedback) {
        if (!m_boundTransformFeedback->isActive()) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }
        gl()->endTransformFeedback();
        if (hasNewGLError()) {
            return;
        }
        m_boundTransformFeedback->setActive(false);
        m_boundTransformFeedback->setPaused(false);
        m_boundTransformFeedback->setProgram(nullptr);
    } else {
        gl()->endTransformFeedback();
    }
}

void WebGL2RenderingContext::pauseTransformFeedback()
{
    ENTER_CONTEXT_SCOPE();
    if (m_boundTransformFeedback) {
        if (!m_boundTransformFeedback->isActive() ||
            m_boundTransformFeedback->isPaused()) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }
        gl()->pauseTransformFeedback();
        if (hasNewGLError()) {
            return;
        }
        m_boundTransformFeedback->setPaused(true);
    } else {
        gl()->pauseTransformFeedback();
    }
}

void WebGL2RenderingContext::resumeTransformFeedback()
{
    ENTER_CONTEXT_SCOPE();
    if (m_boundTransformFeedback) {
        if (!m_boundTransformFeedback->isActive() ||
            !m_boundTransformFeedback->isPaused()) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }
        Optional<WebGLProgram*> prog = getState()->webGLProgram();
        if (!prog.hasValue() ||
            prog.value() != m_boundTransformFeedback->program()) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }
        gl()->resumeTransformFeedback();
        if (hasNewGLError()) {
            return;
        }
        m_boundTransformFeedback->setPaused(false);
    } else {
        gl()->resumeTransformFeedback();
    }
}

WebGLActiveInfo* WebGL2RenderingContext::getTransformFeedbackVarying(
    WebGLProgram* program, GLuint index)
{
    ENTER_CONTEXT_SCOPE(nullptr);
    if (!program || !isFromCurrentContext(program) || program->isDeleted()) {
        setGLError(GL_INVALID_OPERATION);
        return nullptr;
    }

    GLint maxVaryings = 0;
    gl()->getProgramiv(program->glObject(), GL_TRANSFORM_FEEDBACK_VARYINGS,
                       &maxVaryings);
    if (index >= static_cast<GLuint>(maxVaryings)) {
        setGLError(GL_INVALID_VALUE);
        return nullptr;
    }

    GLint maxLen = 0;
    gl()->getProgramiv(program->glObject(),
                       GL_TRANSFORM_FEEDBACK_VARYING_MAX_LENGTH, &maxLen);
    if (maxLen <= 0) {
        maxLen = 256;
    }

    std::vector<char> nameBuffer(maxLen + 1, 0);
    GLsizei length = 0;
    GLsizei size = 0;
    GLenum type = 0;

    gl()->getTransformFeedbackVarying(program->glObject(), index, maxLen,
                                      &length, &size, &type, nameBuffer.data());
    if (hasNewGLError()) {
        return nullptr;
    }

    return new WebGLActiveInfo(
        scriptBindingInstance(), size, type,
        String::createASCIIString(nameBuffer.data(), length));
}

// WebGL2RenderingContextOverloads

bool WebGL2RenderingContext::validateDrawCallUBO()
{
    Optional<WebGLProgram*> maybeProg = getState()->webGLProgram();
    if (!maybeProg.hasValue() || !maybeProg.value()) {
        return true;
    }

    WebGLProgram* program = maybeProg.value();
    if (!program->hasActiveUniformBlocks()) {
        return true;
    }

    const auto& blocks = program->uniformBlocks();
    for (size_t i = 0; i < blocks.size(); i++) {
        GLint dataSize = blocks[i].dataSize;
        if (dataSize <= 0) {
            continue;
        }

        GLint binding = blocks[i].binding;
        if (binding < 0 ||
            static_cast<size_t>(binding) >= m_uniformBufferBindings.size() ||
            m_uniformBufferBindings[binding] == nullptr ||
            m_uniformBufferBindings[binding]->buffer == nullptr ||
            m_uniformBufferBindings[binding]->buffer->isDeleted()) {
            setGLError(GL_INVALID_OPERATION,
                       "draw: UniformBlock is not backed by a buffer.");
            return false;
        }

        IndexedBufferBinding* b = m_uniformBufferBindings[binding];
        GLsizeiptr totalBufferSize = b->buffer->byteLength();

        if (totalBufferSize == 0) {
            setGLError(GL_INVALID_OPERATION,
                       "draw: UniformBlock is backed by a buffer with no "
                       "data store.");
            return false;
        }

        GLsizeiptr effectiveSize = b->isRange ? b->size : totalBufferSize;
        if (effectiveSize < dataSize) {
            setGLError(GL_INVALID_OPERATION,
                       "draw: bound UNIFORM_BUFFER size is smaller than "
                       "active uniform block data size.");
            return false;
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
    std::string result;
    size_t lastPos = 0;
    size_t pos = 0;

    while ((pos = str.find("uniform", pos)) != std::string::npos) {
        // Check word boundary before "uniform"
        if (pos > 0 && (isalnum(str[pos - 1]) || str[pos - 1] == '_')) {
            pos += 7;
            continue;
        }

        // Must be followed by whitespace
        size_t afterUniform = pos + 7;
        if (afterUniform >= str.size() ||
            !(str[afterUniform] == ' ' || str[afterUniform] == '\t' ||
              str[afterUniform] == '\r' || str[afterUniform] == '\n')) {
            pos += 7;
            continue;
        }

        // Skip whitespace to find blockName
        size_t nameStart = afterUniform;
        while (nameStart < str.size() &&
               (str[nameStart] == ' ' || str[nameStart] == '\t' ||
                str[nameStart] == '\r' || str[nameStart] == '\n')) {
            nameStart++;
        }

        // Parse identifier
        size_t nameEnd = nameStart;
        while (nameEnd < str.size() &&
               (isalnum(str[nameEnd]) || str[nameEnd] == '_')) {
            nameEnd++;
        }

        if (nameEnd == nameStart) {
            pos += 7;
            continue;
        }

        // Skip whitespace after blockName
        size_t bracePos = nameEnd;
        while (bracePos < str.size() &&
               (str[bracePos] == ' ' || str[bracePos] == '\t' ||
                str[bracePos] == '\r' || str[bracePos] == '\n')) {
            bracePos++;
        }

        // Must be '{' for uniform block
        if (bracePos >= str.size() || str[bracePos] != '{') {
            pos = nameEnd;
            continue;
        }

        // Check if layout(...) exists before "uniform"
        bool hasLayout = false;
        size_t p = pos;
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

        result.append(str, lastPos, pos - lastPos);
        if (!hasLayout) {
            result += "layout(std140) ";
        }
        result.append(str, pos, (bracePos + 1) - pos);
        lastPos = bracePos + 1;
        pos = lastPos;
    }
    result.append(str, lastPos, str.length() - lastPos);
    str = result;

    const char* sourceArray[1] = { str.c_str() };
    gl()->shaderSource(shader->glObject(), 1, sourceArray, nullptr);
}

void WebGL2RenderingContext::drawRangeElements(GLenum mode, GLuint start,
                                               GLuint end, GLsizei count,
                                               GLenum type, GLintptr offset)
{
    ENTER_CONTEXT_SCOPE();

    if (!isValidDrawMode(mode)) {
        setGLError(GL_INVALID_ENUM, "invalid draw mode");
        return;
    }

    if (type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT &&
        type != GL_UNSIGNED_INT) {
        setGLError(GL_INVALID_ENUM, "invalid type");
        return;
    }

    if (end < start || count < 0 || offset < 0) {
        setGLError(GL_INVALID_VALUE, "invalid range, count or offset");
        return;
    }

    if ((type == GL_UNSIGNED_SHORT && (offset % 2) != 0) ||
        (type == GL_UNSIGNED_INT && (offset % 4) != 0)) {
        setGLError(GL_INVALID_OPERATION, "offset not properly aligned");
        return;
    }

    completePendingJobs();

    if (count > 0) {
        Optional<WebGLBuffer*> elementBuffer =
            getState()->getBoundBuffer(GL_ELEMENT_ARRAY_BUFFER);
        if (!elementBuffer.hasValue() || !elementBuffer.value()) {
            setGLError(
                GL_INVALID_OPERATION,
                "drawRangeElements: no buffer bound to ELEMENT_ARRAY_BUFFER");
            return;
        }
        size_t bytesPerIndex = 1;
        if (type == GL_UNSIGNED_SHORT) {
            bytesPerIndex = 2;
        } else if (type == GL_UNSIGNED_INT) {
            bytesPerIndex = 4;
        }
        if (static_cast<size_t>(elementBuffer.value()->byteLength()) <
            static_cast<size_t>(offset) +
                static_cast<size_t>(count) * bytesPerIndex) {
            setGLError(GL_INVALID_OPERATION,
                       "drawRangeElements: element buffer out of bounds");
            return;
        }
    }

    for (GLuint array : getState()->arraysEnabled()) {
        if (!getState()->getBufferBoundToVertexAttributes(array)) {
            setGLError(
                GL_INVALID_OPERATION,
                "drawRangeElements: vertex attribute enabled without bound "
                "buffer");
            return;
        }
    }

    if (!validateDrawCallUBO() || !validateDrawCallVertexAttribs())
        return;
    gl()->drawRangeElements(mode, start, end, count, type,
                            reinterpret_cast<const void*>(offset));
    m_ownerHTMLCanvasElement->setNeedsComposite();
}

void WebGL2RenderingContext::blitFramebuffer(GLint srcX0, GLint srcY0,
                                             GLint srcX1, GLint srcY1,
                                             GLint dstX0, GLint dstY0,
                                             GLint dstX1, GLint dstY1,
                                             GLbitfield mask, GLenum filter)
{
    ENTER_CONTEXT_SCOPE();

    if (filter != GL_NEAREST && filter != GL_LINEAR) {
        setGLError(GL_INVALID_ENUM, "filter must be GL_NEAREST or GL_LINEAR.");
        return;
    }

    if ((mask & ~(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                  GL_STENCIL_BUFFER_BIT)) != 0) {
        setGLError(GL_INVALID_VALUE, "invalid mask bitfield.");
        return;
    }

    if (filter == GL_LINEAR &&
        (mask & (GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)) != 0) {
        setGLError(GL_INVALID_OPERATION,
                   "linear filter not allowed for depth/stencil blit.");
        return;
    }

    if (mask & GL_COLOR_BUFFER_BIT) {
        Optional<WebGLFramebuffer*> maybeReadFb =
            getBoundFramebuffer(GL_READ_FRAMEBUFFER);
        Optional<WebGLFramebuffer*> maybeDrawFb =
            getBoundFramebuffer(GL_DRAW_FRAMEBUFFER);
        WebGLFramebuffer* readFb =
            maybeReadFb.hasValue() ? maybeReadFb.value() : nullptr;
        WebGLFramebuffer* drawFb =
            maybeDrawFb.hasValue() ? maybeDrawFb.value() : nullptr;
        GLenum readBuf = readFb ? readFb->readBuffer() : GL_BACK;

        if (readFb == drawFb && readFb != nullptr) {
            const GLenum* drawBuffers = drawFb->drawBuffers();
            size_t count = drawFb->drawBufferCount();
            if (count == 0) {
                count = 1;
            }
            for (size_t i = 0; i < count; ++i) {
                GLenum db = drawBuffers[i];
                if (db != GL_NONE && db == readBuf) {
                    setGLError(GL_INVALID_OPERATION,
                               "blitFramebuffer cannot have identical read and "
                               "draw buffer.");
                    return;
                }
            }
        }
        if (readFb && drawFb) {
            if (readBuf >= GL_COLOR_ATTACHMENT0 &&
                readBuf < GL_COLOR_ATTACHMENT0 + 16) {
                size_t readIdx = readBuf - GL_COLOR_ATTACHMENT0;
                void* readObj = readFb->getColorAttachmentObject(readIdx);
                const GLenum* drawBuffers = drawFb->drawBuffers();
                size_t count = drawFb->drawBufferCount();
                if (count == 0) {
                    count = 1;
                }
                for (size_t i = 0; i < count; ++i) {
                    GLenum db = drawBuffers[i];
                    if (db >= GL_COLOR_ATTACHMENT0 &&
                        db < GL_COLOR_ATTACHMENT0 + 16) {
                        size_t drawIdx = db - GL_COLOR_ATTACHMENT0;
                        void* drawObj =
                            drawFb->getColorAttachmentObject(drawIdx);
                        if (readObj != nullptr && readObj == drawObj) {
                            if (readFb->isSameImageAs(readIdx, drawFb,
                                                      drawIdx)) {
                                setGLError(
                                    GL_INVALID_OPERATION,
                                    "blitFramebuffer cannot have identical "
                                    "read and draw attachment.");
                                return;
                            }
                        }
                        GLsizei readSamples =
                            readFb->getColorAttachmentSamples(readIdx);
                        GLsizei drawSamples =
                            drawFb->getColorAttachmentSamples(drawIdx);
                        GLenum readFmt =
                            readFb->getColorAttachmentFormat(readIdx);
                        GLenum drawFmt =
                            drawFb->getColorAttachmentFormat(drawIdx);
                        if ((readSamples > 0 || drawSamples > 0) &&
                            readFmt != drawFmt) {
                            setGLError(GL_INVALID_OPERATION,
                                       "blitFramebuffer with multisampling "
                                       "requires identical formats.");
                            return;
                        }
                    }
                }
            }
        }
    }

    gl()->blitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1,
                          dstY1, mask, filter);
    m_ownerHTMLCanvasElement->setNeedsComposite();
}

void WebGL2RenderingContext::framebufferTextureLayer(
    GLenum target, GLenum attachment, Optional<WebGLTexture*> maybeTexture,
    GLint level, GLint layer)
{
    ENTER_CONTEXT_SCOPE();

    if (isDefaultFramebufferBound()) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    GLuint texName = 0;
    if (maybeTexture.hasValue()) {
        WebGLTexture* texture = maybeTexture.value();
        if (texture) {
            if (!isFromCurrentContext(texture) || texture->isDeleted()) {
                setGLError(GL_INVALID_OPERATION);
                return;
            }
            texName = texture->glObject();
        }
    }

    if (attachment == GL_DEPTH_STENCIL_ATTACHMENT) {
        gl()->framebufferTextureLayer(target, GL_DEPTH_ATTACHMENT, texName,
                                      level, layer);
        gl()->framebufferTextureLayer(target, GL_STENCIL_ATTACHMENT, texName,
                                      level, layer);
    } else {
        gl()->framebufferTextureLayer(target, attachment, texName, level,
                                      layer);
    }
    Optional<WebGLFramebuffer*> maybeFb = getBoundFramebuffer(target);
    if (maybeFb.hasValue() && maybeFb.value()) {
        maybeFb.value()->setAttachmentTextureLayer(
            attachment,
            maybeTexture.hasValue() ? maybeTexture.value() : nullptr, level,
            layer);
    }
    m_ownerHTMLCanvasElement->setNeedsComposite();
}

static bool isValidRenderbufferInternalFormat(GLenum internalformat)
{
    switch (internalformat) {
    case GL_R8:
    case GL_R8_SNORM:
    case GL_RG8:
    case GL_RG8_SNORM:
    case GL_RGB8:
    case GL_RGB8_SNORM:
    case GL_RGB565:
    case GL_RGBA4:
    case GL_RGB5_A1:
    case GL_RGBA8:
    case GL_RGBA8_SNORM:
    case GL_RGB10_A2:
    case GL_RGB10_A2UI:
    case GL_SRGB8:
    case GL_SRGB8_ALPHA8:
    case GL_R16F:
    case GL_RG16F:
    case GL_RGB16F:
    case GL_RGBA16F:
    case GL_R32F:
    case GL_RG32F:
    case GL_RGB32F:
    case GL_RGBA32F:
    case GL_R11F_G11F_B10F:
    case GL_RGB9_E5:
    case GL_R8I:
    case GL_R8UI:
    case GL_R16I:
    case GL_R16UI:
    case GL_R32I:
    case GL_R32UI:
    case GL_RG8I:
    case GL_RG8UI:
    case GL_RG16I:
    case GL_RG16UI:
    case GL_RG32I:
    case GL_RG32UI:
    case GL_RGB8I:
    case GL_RGB8UI:
    case GL_RGB16I:
    case GL_RGB16UI:
    case GL_RGB32I:
    case GL_RGB32UI:
    case GL_RGBA8I:
    case GL_RGBA8UI:
    case GL_RGBA16I:
    case GL_RGBA16UI:
    case GL_RGBA32I:
    case GL_RGBA32UI:
    case GL_RGB:
    case GL_RGBA:
    case GL_DEPTH_STENCIL:
    case GL_DEPTH_COMPONENT16:
    case GL_DEPTH_COMPONENT24:
    case GL_DEPTH_COMPONENT32F:
    case GL_DEPTH24_STENCIL8:
    case GL_DEPTH32F_STENCIL8:
    case GL_STENCIL_INDEX8:
        return true;
    default:
        return false;
    }
}

ScriptValue WebGL2RenderingContext::getInternalformatParameter(
    GLenum target, GLenum internalformat, GLenum pname)
{
    ENTER_CONTEXT_SCOPE(scriptNull());

    if (target != GL_RENDERBUFFER) {
        setGLError(GL_INVALID_ENUM, "target must be RENDERBUFFER");
        return scriptNull();
    }

    if (pname != GL_SAMPLES) {
        setGLError(GL_INVALID_ENUM, "pname must be SAMPLES");
        return scriptNull();
    }

    if (!isValidRenderbufferInternalFormat(internalformat)) {
        setGLError(GL_INVALID_ENUM, "internalformat not supported");
        return scriptNull();
    }

    if (WebGLFramebuffer::isIntegerInternalFormat(internalformat)) {
        return createScriptValue(
            createTypedArray<Escargot::Int32ArrayObjectRef>(
                scriptBindingInstance(), std::vector<int32_t>()));
    }

    GLint numSampleCounts = 0;
    gl()->getInternalformativ(target, internalformat, GL_NUM_SAMPLE_COUNTS, 1,
                              &numSampleCounts);
    if (hasNewGLError()) {
        return scriptNull();
    }
    if (numSampleCounts <= 0) {
        return createScriptValue(
            createTypedArray<Escargot::Int32ArrayObjectRef>(
                scriptBindingInstance(), std::vector<int32_t>()));
    }

    std::vector<int32_t> samples(numSampleCounts);
    gl()->getInternalformativ(target, internalformat, GL_SAMPLES,
                              numSampleCounts, samples.data());
    if (hasNewGLError()) {
        return scriptNull();
    }

    return createScriptValue(createTypedArray<Escargot::Int32ArrayObjectRef>(
        scriptBindingInstance(), samples));
}

void WebGL2RenderingContext::renderbufferStorageMultisample(
    GLenum target, GLsizei samples, GLenum internalformat, GLsizei width,
    GLsizei height)
{
    ENTER_CONTEXT_SCOPE();

    if (target != GL_RENDERBUFFER) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    if (width < 0 || height < 0 || samples < 0) {
        setGLError(GL_INVALID_VALUE);
        return;
    }

    if (internalformat == 0x881B /* GL_RGB16F */ ||
        internalformat == 0x8815 /* GL_RGB32F */) {
        setGLError(GL_INVALID_ENUM, "RGB float formats are not renderable");
        return;
    }
    if (WebGLFramebuffer::isFloatColorFormat(internalformat)) {
        if (!isExtensionEnabled("EXT_color_buffer_float")) {
            setGLError(GL_INVALID_ENUM, "EXT_color_buffer_float not enabled");
            return;
        }
    }

    GLint maxSize = 0;
    gl()->getIntegerv(GL_MAX_RENDERBUFFER_SIZE, &maxSize);
    if (maxSize > 0 && (width > maxSize || height > maxSize)) {
        setGLError(GL_INVALID_VALUE);
        return;
    }

    GLint maxSamples = 0;
    gl()->getIntegerv(GL_MAX_SAMPLES, &maxSamples);
    if (maxSamples > 0 && samples > maxSamples) {
        setGLError(GL_INVALID_VALUE);
        return;
    }

    Optional<WebGLRenderbuffer*> rb = boundRenderbuffer();
    if (!rb.hasValue() || rb.value() == nullptr || rb.value()->isDeleted()) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    GLenum glInternalFormat = internalformat;
    if (glInternalFormat == GL_DEPTH_STENCIL) {
        glInternalFormat = GL_DEPTH24_STENCIL8;
    }

    gl()->renderbufferStorageMultisample(target, samples, glInternalFormat,
                                         width, height);
    if (!hasNewGLError()) {
        rb.value()->setStorage(internalformat, width, height, samples);
    }
}

static bool filterInvalidateAttachments(
    GLenum target, const GCAtomicVector<GLenum>& attachments, bool isDefaultFb,
    WebGLFramebuffer* userFb, bool hasDepth, bool hasStencil,
    GLint maxColorAttachments, std::vector<GLenum>& validAttachments)
{
    for (size_t i = 0; i < attachments.size(); ++i) {
        GLenum a = attachments[i];
        if (isDefaultFb) {
            if (a == GL_COLOR) {
                validAttachments.push_back(GL_COLOR_ATTACHMENT0);
            } else if (a == GL_DEPTH) {
                if (hasDepth) {
                    validAttachments.push_back(GL_DEPTH_ATTACHMENT);
                }
            } else if (a == GL_STENCIL) {
                if (hasStencil) {
                    validAttachments.push_back(GL_STENCIL_ATTACHMENT);
                }
            } else {
                return false;
            }
        } else {
            if (a >= GL_COLOR_ATTACHMENT0 &&
                a < static_cast<GLenum>(GL_COLOR_ATTACHMENT0 +
                                        maxColorAttachments)) {
                if (userFb && userFb->attachedTexture(a)) {
                    validAttachments.push_back(a);
                }
            } else if (a == GL_DEPTH_ATTACHMENT || a == GL_STENCIL_ATTACHMENT ||
                       a == GL_DEPTH_STENCIL_ATTACHMENT) {
                if (userFb && userFb->attachedRenderBuffer()) {
                    validAttachments.push_back(a);
                }
            } else {
                return false;
            }
        }
    }
    return true;
}

void WebGL2RenderingContext::invalidateFramebuffer(
    GLenum target, GCAtomicVector<GLenum> attachments)
{
    ENTER_CONTEXT_SCOPE();

    if (target != GL_FRAMEBUFFER && target != GL_READ_FRAMEBUFFER &&
        target != GL_DRAW_FRAMEBUFFER) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    GLint maxColorAttachments = 0;
    gl()->getIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);

    WebGLFramebuffer* userFb = nullptr;
    if (!isDefaultFramebufferBound()) {
        Optional<WebGLFramebuffer*> maybeFb = getState()->webGLFramebuffer();
        if (maybeFb.hasValue()) {
            userFb = maybeFb.value();
        }
    }

    std::vector<GLenum> validAttachments;
    if (!filterInvalidateAttachments(target, attachments,
                                     isDefaultFramebufferBound(), userFb,
                                     hasDepthBuffer(), hasStencilBuffer(),
                                     maxColorAttachments, validAttachments)) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    if (!validAttachments.empty()) {
        gl()->invalidateFramebuffer(target, validAttachments.size(),
                                    validAttachments.data());
    }
}

void WebGL2RenderingContext::invalidateSubFramebuffer(
    GLenum target, GCAtomicVector<GLenum> attachments, GLint x, GLint y,
    GLsizei width, GLsizei height)
{
    ENTER_CONTEXT_SCOPE();

    if (target != GL_FRAMEBUFFER && target != GL_READ_FRAMEBUFFER &&
        target != GL_DRAW_FRAMEBUFFER) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    if (width < 0 || height < 0) {
        setGLError(GL_INVALID_VALUE);
        return;
    }

    GLint maxColorAttachments = 0;
    gl()->getIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);

    WebGLFramebuffer* userFb = nullptr;
    if (!isDefaultFramebufferBound()) {
        Optional<WebGLFramebuffer*> maybeFb = getState()->webGLFramebuffer();
        if (maybeFb.hasValue()) {
            userFb = maybeFb.value();
        }
    }

    std::vector<GLenum> validAttachments;
    if (!filterInvalidateAttachments(target, attachments,
                                     isDefaultFramebufferBound(), userFb,
                                     hasDepthBuffer(), hasStencilBuffer(),
                                     maxColorAttachments, validAttachments)) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    if (!validAttachments.empty()) {
        gl()->invalidateSubFramebuffer(target, validAttachments.size(),
                                       validAttachments.data(), x, y, width,
                                       height);
    }
}

void WebGL2RenderingContext::readBuffer(GLenum src)
{
    ENTER_CONTEXT_SCOPE();

    GLint maxColorAttachments = 0;
    gl()->getIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);

    if (isDefaultFramebufferBound()) {
        if (src == GL_BACK) {
            m_defaultReadBuffer = GL_BACK;
            gl()->readBuffer(GL_COLOR_ATTACHMENT0);
            return;
        }
        if (src == GL_NONE) {
            m_defaultReadBuffer = GL_NONE;
            gl()->readBuffer(GL_NONE);
            return;
        }
        if (src >= GL_COLOR_ATTACHMENT0 &&
            src < static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + 16)) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }
        setGLError(GL_INVALID_ENUM);
        return;
    }

    if (src == GL_NONE) {
        Optional<WebGLFramebuffer*> maybeFb = getState()->webGLFramebuffer();
        if (maybeFb.hasValue() && maybeFb.value()) {
            maybeFb.value()->setReadBuffer(GL_NONE);
        }
        gl()->readBuffer(GL_NONE);
        return;
    }

    if (src >= GL_COLOR_ATTACHMENT0 &&
        src < static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + maxColorAttachments)) {
        Optional<WebGLFramebuffer*> maybeFb = getState()->webGLFramebuffer();
        if (maybeFb.hasValue() && maybeFb.value()) {
            maybeFb.value()->setReadBuffer(src);
        }
        gl()->readBuffer(src);
        return;
    }

    if (src >=
            static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + maxColorAttachments) &&
        src < static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + 16)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    if (src == GL_BACK) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    setGLError(GL_INVALID_ENUM);
}

void WebGL2RenderingContext::drawBuffers(GCAtomicVector<GLenum> buffers)
{
    ENTER_CONTEXT_SCOPE();
    GLint maxDrawBuffers = 0;
    gl()->getIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
    if (buffers.size() > static_cast<size_t>(maxDrawBuffers)) {
        setGLError(GL_INVALID_VALUE,
                   "buffers length exceeds MAX_DRAW_BUFFERS.");
        return;
    }

    if (isDefaultFramebufferBound()) {
        for (size_t i = 0; i < buffers.size(); ++i) {
            if (i == 0) {
                if (buffers[i] != GL_NONE && buffers[i] != GL_BACK) {
                    setGLError(GL_INVALID_OPERATION);
                    return;
                }
            } else {
                if (buffers[i] != GL_NONE) {
                    setGLError(GL_INVALID_OPERATION);
                    return;
                }
            }
        }
    } else {
        for (size_t i = 0; i < buffers.size(); ++i) {
            if (buffers[i] != GL_NONE &&
                buffers[i] != static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + i)) {
                setGLError(GL_INVALID_OPERATION,
                           "buffers[i] must be NONE or COLOR_ATTACHMENTi");
                return;
            }
        }
    }

    if (isDefaultFramebufferBound()) {
        // The WebGL default framebuffer is emulated with m_framebufferTexture's
        // FBO, so GL_BACK is not a name GL accepts here -- the driver wants
        // COLOR_ATTACHMENTi or NONE and rejects anything else. Translate, the
        // same way bindFramebuffer() and readBuffer() already do.
        std::vector<GLenum> translated(buffers.size(), GL_NONE);
        for (size_t i = 0; i < buffers.size(); ++i) {
            translated[i] =
                buffers[i] == GL_BACK ? GL_COLOR_ATTACHMENT0 : GL_NONE;
        }
        gl()->drawBuffers(translated.size(), translated.data());
        return;
    }

    gl()->drawBuffers(buffers.size(), buffers.data());

    Optional<WebGLFramebuffer*> maybeFb =
        getBoundFramebuffer(GL_DRAW_FRAMEBUFFER);
    if (maybeFb.hasValue() && maybeFb.value()) {
        maybeFb.value()->setDrawBuffers(buffers);
    }
}

void WebGL2RenderingContext::clearBufferfv(GLenum buffer, GLint drawbuffer,
                                           Float32List values,
                                           unsigned long long srcOffset)
{
    ENTER_CONTEXT_SCOPE();
    if (buffer != GL_COLOR && buffer != GL_DEPTH) {
        setGLError(GL_INVALID_ENUM, "buffer must be GL_COLOR or GL_DEPTH.");
        return;
    }
    if (buffer == GL_DEPTH && drawbuffer != 0) {
        setGLError(GL_INVALID_VALUE, "drawbuffer must be 0 for DEPTH.");
        return;
    }

    if (buffer == GL_COLOR) {
        Optional<WebGLFramebuffer*> maybeFb =
            getBoundFramebuffer(GL_DRAW_FRAMEBUFFER);
        if (maybeFb.hasValue() && maybeFb.value()) {
            WebGLFramebuffer* fb = maybeFb.value();
            const GLenum* drawBuffers = fb->drawBuffers();
            size_t count = fb->drawBufferCount();
            GLenum b = drawbuffer < static_cast<GLint>(count)
                           ? drawBuffers[drawbuffer]
                           : (drawbuffer == 0 ? GL_COLOR_ATTACHMENT0 : GL_NONE);
            if (b >= GL_COLOR_ATTACHMENT0 && b < GL_COLOR_ATTACHMENT0 + 16) {
                size_t idx = b - GL_COLOR_ATTACHMENT0;
                GLenum fmt = fb->getColorAttachmentFormat(idx);
                if (WebGLFramebuffer::isIntegerInternalFormat(fmt)) {
                    setGLError(
                        GL_INVALID_OPERATION,
                        "clearBufferfv not allowed on integer color buffers.");
                    return;
                }
            }
        }
    }

    if (values.isFloat32ArrayValue()) {
        const ScriptFloat32Array arr = values.getFloat32ArrayValue();
        if (srcOffset >= arr->arrayLength()) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
        const GLfloat* ptr =
            reinterpret_cast<const GLfloat*>(arr->rawBuffer()) + srcOffset;
        gl()->clearBufferfv(buffer, drawbuffer, ptr);
    } else if (values.isSequenceOfGLfloatValue()) {
        const GCAtomicVector<double> seq = values.getSequenceOfGLfloatValue();
        if (srcOffset >= seq.size()) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
        std::vector<GLfloat> floats(seq.begin(), seq.end());
        gl()->clearBufferfv(buffer, drawbuffer, floats.data() + srcOffset);
    }
    m_ownerHTMLCanvasElement->setNeedsComposite();
}

void WebGL2RenderingContext::clearBufferiv(GLenum buffer, GLint drawbuffer,
                                           Int32List values,
                                           unsigned long long srcOffset)
{
    ENTER_CONTEXT_SCOPE();
    if (buffer != GL_COLOR && buffer != GL_STENCIL) {
        setGLError(GL_INVALID_ENUM, "buffer must be GL_COLOR or GL_STENCIL.");
        return;
    }
    if (buffer == GL_STENCIL && drawbuffer != 0) {
        setGLError(GL_INVALID_VALUE, "drawbuffer must be 0 for STENCIL.");
        return;
    }

    if (buffer == GL_COLOR) {
        Optional<WebGLFramebuffer*> maybeFb =
            getBoundFramebuffer(GL_DRAW_FRAMEBUFFER);
        if (!maybeFb.hasValue() || !maybeFb.value()) {
            setGLError(GL_INVALID_OPERATION,
                       "clearBufferiv not allowed on default framebuffer.");
            return;
        }
        WebGLFramebuffer* fb = maybeFb.value();
        const GLenum* drawBuffers = fb->drawBuffers();
        size_t count = fb->drawBufferCount();
        GLenum b = drawbuffer < static_cast<GLint>(count)
                       ? drawBuffers[drawbuffer]
                       : (drawbuffer == 0 ? GL_COLOR_ATTACHMENT0 : GL_NONE);
        if (b >= GL_COLOR_ATTACHMENT0 && b < GL_COLOR_ATTACHMENT0 + 16) {
            size_t idx = b - GL_COLOR_ATTACHMENT0;
            GLenum fmt = fb->getColorAttachmentFormat(idx);
            if (!WebGLFramebuffer::isSignedIntegerInternalFormat(fmt)) {
                setGLError(GL_INVALID_OPERATION,
                           "clearBufferiv only allowed on signed integer "
                           "color buffers.");
                return;
            }
        }
    }

    if (values.isInt32ArrayValue()) {
        const ScriptInt32Array arr = values.getInt32ArrayValue();
        if (srcOffset >= arr->arrayLength()) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
        const GLint* ptr =
            reinterpret_cast<const GLint*>(arr->rawBuffer()) + srcOffset;
        gl()->clearBufferiv(buffer, drawbuffer, ptr);
    } else if (values.isSequenceOfGLintValue()) {
        const GCAtomicVector<int32_t> seq = values.getSequenceOfGLintValue();
        if (srcOffset >= seq.size()) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
        gl()->clearBufferiv(buffer, drawbuffer, seq.data() + srcOffset);
    }
    m_ownerHTMLCanvasElement->setNeedsComposite();
}

void WebGL2RenderingContext::clearBufferuiv(GLenum buffer, GLint drawbuffer,
                                            Uint32List values,
                                            unsigned long long srcOffset)
{
    ENTER_CONTEXT_SCOPE();
    if (buffer != GL_COLOR) {
        setGLError(GL_INVALID_ENUM, "buffer must be GL_COLOR.");
        return;
    }

    {
        Optional<WebGLFramebuffer*> maybeFb =
            getBoundFramebuffer(GL_DRAW_FRAMEBUFFER);
        if (!maybeFb.hasValue() || !maybeFb.value()) {
            setGLError(GL_INVALID_OPERATION,
                       "clearBufferuiv not allowed on default framebuffer.");
            return;
        }
        WebGLFramebuffer* fb = maybeFb.value();
        const GLenum* drawBuffers = fb->drawBuffers();
        size_t count = fb->drawBufferCount();
        GLenum b = drawbuffer < static_cast<GLint>(count)
                       ? drawBuffers[drawbuffer]
                       : (drawbuffer == 0 ? GL_COLOR_ATTACHMENT0 : GL_NONE);
        if (b >= GL_COLOR_ATTACHMENT0 && b < GL_COLOR_ATTACHMENT0 + 16) {
            size_t idx = b - GL_COLOR_ATTACHMENT0;
            GLenum fmt = fb->getColorAttachmentFormat(idx);
            if (!WebGLFramebuffer::isUnsignedIntegerInternalFormat(fmt)) {
                setGLError(GL_INVALID_OPERATION,
                           "clearBufferuiv only allowed on unsigned integer "
                           "color buffers.");
                return;
            }
        }
    }

    if (values.isUint32ArrayValue()) {
        const ScriptUint32Array arr = values.getUint32ArrayValue();
        if (srcOffset >= arr->arrayLength()) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
        const GLuint* ptr =
            reinterpret_cast<const GLuint*>(arr->rawBuffer()) + srcOffset;
        gl()->clearBufferuiv(buffer, drawbuffer, ptr);
    } else if (values.isSequenceOfGLuintValue()) {
        const GCAtomicVector<uint32_t> seq = values.getSequenceOfGLuintValue();
        if (srcOffset >= seq.size()) {
            setGLError(GL_INVALID_VALUE);
            return;
        }
        gl()->clearBufferuiv(buffer, drawbuffer, seq.data() + srcOffset);
    }
    m_ownerHTMLCanvasElement->setNeedsComposite();
}

void WebGL2RenderingContext::clearBufferfi(GLenum buffer, GLint drawbuffer,
                                           GLfloat depth, GLint stencil)
{
    ENTER_CONTEXT_SCOPE();
    if (buffer != GL_DEPTH_STENCIL) {
        setGLError(GL_INVALID_ENUM, "buffer must be GL_DEPTH_STENCIL.");
        return;
    }
    if (drawbuffer != 0) {
        setGLError(GL_INVALID_VALUE, "drawbuffer must be 0 for DEPTH_STENCIL.");
        return;
    }
    gl()->clearBufferfi(buffer, drawbuffer, depth, stencil);
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

    Optional<WebGLBuffer*> maybeBound = getState()->getBoundBuffer(target);
    if (maybeBound.hasValue() && maybeBound.value()) {
        maybeBound.value()->setByteLength(copyByteLength);
    }
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
    if (!Pixel::isKnownInternalFormatWebGL2(internalFormat) ||
        !Pixel::isKnownFormatWebGL2(format) ||
        !Pixel::isKnownTypeWebGL2(type)) {
        setGLError(GL_INVALID_ENUM,
                   "internalFormat, format, or type is not an accepted token");
        return false;
    }

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

    if (!Pixel::isSizedInternalFormat(internalformat) &&
        !isCompressedFormatEnabled(internalformat)) {
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
        texture->setStorage2D(target, levels, internalformat, width, height);
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

    if (!Pixel::isSizedInternalFormat(internalformat) &&
        !isCompressedFormatEnabled(internalformat)) {
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
        texture->setStorage3D(target, levels, internalformat, width, height,
                              depth);
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

    if (pboOffset < 0) {
        setGLError(GL_INVALID_VALUE, "negative offset");
        return;
    }

    size_t requiredBytes = 0;
    if (!computeRequiredUnpackBytes(width, height, 1, format, type,
                                    requiredBytes)) {
        return;
    }

    GLint bufferSize = 0;
    gl()->getBufferParameteriv(GL_PIXEL_UNPACK_BUFFER, GL_BUFFER_SIZE,
                               &bufferSize);
    if (static_cast<size_t>(pboOffset) + requiredBytes >
        static_cast<size_t>(bufferSize)) {
        setGLError(GL_INVALID_OPERATION, "buffer too small");
        return;
    }

    gl()->texImage2D(target, level, internalformat, width, height, border,
                     format, type, reinterpret_cast<const void*>(pboOffset));
    if (texture && !hasNewGLError()) {
        texture->setImageLevelInfo(target, level, internalformat, width, height,
                                   1);
    }
}

void WebGL2RenderingContext::texImage2D(GLenum target, GLint level,
                                        GLint internalformat, GLsizei width,
                                        GLsizei height, GLint border,
                                        GLenum format, GLenum type,
                                        TexImageSource source)
{
    ENTER_CONTEXT_SCOPE();

    WebGLTexture* texture = getBoundTextureObject(target);
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

    if (texture && !hasNewGLError()) {
        texture->setImageLevelInfo(target, level, internalformat, width, height,
                                   1);
    }
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

    if (texture && !hasNewGLError()) {
        texture->setImageLevelInfo(target, level, internalformat, width, height,
                                   1);
    }
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

    if (pboOffset < 0) {
        setGLError(GL_INVALID_VALUE, "negative offset");
        return;
    }

    size_t requiredBytes = 0;
    if (!computeRequiredUnpackBytes(width, height, depth, format, type,
                                    requiredBytes)) {
        return;
    }

    GLint bufferSize = 0;
    gl()->getBufferParameteriv(GL_PIXEL_UNPACK_BUFFER, GL_BUFFER_SIZE,
                               &bufferSize);
    if (static_cast<size_t>(pboOffset) + requiredBytes >
        static_cast<size_t>(bufferSize)) {
        setGLError(GL_INVALID_OPERATION, "buffer too small");
        return;
    }

    gl()->texImage3D(target, level, internalformat, width, height, depth,
                     border, format, type,
                     reinterpret_cast<const void*>(pboOffset));

    if (texture && !hasNewGLError()) {
        texture->setImageLevelInfo(target, level, internalformat, width, height,
                                   depth);
    }
}

void WebGL2RenderingContext::texImage3D(GLenum target, GLint level,
                                        GLint internalformat, GLsizei width,
                                        GLsizei height, GLsizei depth,
                                        GLint border, GLenum format,
                                        GLenum type, TexImageSource source)
{
    ENTER_CONTEXT_SCOPE();

    WebGLTexture* texture = getBoundTextureObject(target);
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

    if (texture && !hasNewGLError()) {
        texture->setSize(width, height);
        texture->setImageLevelInfo(target, level, internalformat, width, height,
                                   depth);
    }
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

    if (border != 0) {
        setGLError(GL_INVALID_VALUE, "border must be 0.");
        return;
    }

    if (internalformat == 0x8227 /* GL_RG */) {
        setGLError(GL_INVALID_VALUE,
                   "unsized GL_RG format not allowed in texImage3D");
        return;
    }

    if (!checkInternalFormat(internalformat, format, type)) {
        return;
    }

    WebGLTexture* texture = getBoundTextureObject(target);
    if (!hasBoundTexture(target)) {
        setGLError(
            GL_INVALID_OPERATION,
            StringUtils::formatString("target (0x%04X) is not bound.", target)
                .c_str());
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

    if (texture && !hasNewGLError()) {
        texture->setImageLevelInfo(target, level, internalformat, width, height,
                                   depth);
    }
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

    if (border != 0) {
        setGLError(GL_INVALID_VALUE, "border must be 0.");
        return;
    }

    if (internalformat == 0x8227 /* GL_RG */) {
        setGLError(GL_INVALID_VALUE,
                   "unsized GL_RG format not allowed in texImage3D");
        return;
    }

    if (!checkInternalFormat(internalformat, format, type)) {
        return;
    }

    WebGLTexture* texture = getBoundTextureObject(target);
    if (!hasBoundTexture(target)) {
        setGLError(
            GL_INVALID_OPERATION,
            StringUtils::formatString("target (0x%04X) is not bound.", target)
                .c_str());
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

    if (texture && !hasNewGLError()) {
        texture->setSize(width, height);
        texture->setImageLevelInfo(target, level, internalformat, width, height,
                                   depth);
    }
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

    if (pboOffset < 0) {
        setGLError(GL_INVALID_VALUE, "negative offset");
        return;
    }

    size_t requiredBytes = 0;
    if (!computeRequiredUnpackBytes(width, height, depth, format, type,
                                    requiredBytes)) {
        return;
    }

    GLint bufferSize = 0;
    gl()->getBufferParameteriv(GL_PIXEL_UNPACK_BUFFER, GL_BUFFER_SIZE,
                               &bufferSize);
    if (static_cast<size_t>(pboOffset) + requiredBytes >
        static_cast<size_t>(bufferSize)) {
        setGLError(GL_INVALID_OPERATION, "buffer too small");
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

    if (!Pixel::isKnownFormatWebGL2(format) ||
        !Pixel::isKnownTypeWebGL2(type)) {
        setGLError(GL_INVALID_ENUM, "format or type is invalid.");
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

    if (pboOffset < 0) {
        setGLError(GL_INVALID_VALUE, "negative offset");
        return;
    }

    size_t requiredBytes = 0;
    if (!computeRequiredUnpackBytes(width, height, 1, format, type,
                                    requiredBytes)) {
        return;
    }

    GLint bufferSize = 0;
    gl()->getBufferParameteriv(GL_PIXEL_UNPACK_BUFFER, GL_BUFFER_SIZE,
                               &bufferSize);
    if (static_cast<size_t>(pboOffset) + requiredBytes >
        static_cast<size_t>(bufferSize)) {
        setGLError(GL_INVALID_OPERATION, "buffer too small");
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

// The ArrayBufferView overloads read client memory and must not run while a
// PIXEL_UNPACK_BUFFER is bound; the offset overloads read from that buffer.
// https://registry.khronos.org/webgl/specs/latest/2.0/#3.7.6
bool WebGL2RenderingContext::checkUnpackBufferBinding(
    bool fromPixelUnpackBuffer)
{
    Optional<WebGLBuffer*> unpackBuffer =
        getState()->getBoundBuffer(GL_PIXEL_UNPACK_BUFFER);
    bool hasUnpackBuffer = unpackBuffer.hasValue() && unpackBuffer.value();
    if (hasUnpackBuffer != fromPixelUnpackBuffer) {
        setGLError(GL_INVALID_OPERATION,
                   hasUnpackBuffer ? "PIXEL_UNPACK_BUFFER is bound."
                                   : "PIXEL_UNPACK_BUFFER is not bound.");
        return false;
    }
    return true;
}

// srcOffset and srcLengthOverride count elements of srcData; a zero
// srcLengthOverride takes the rest of the view.
bool WebGL2RenderingContext::sliceCompressedTexData(
    ScriptArrayBufferView srcData, unsigned long long srcOffset,
    GLuint srcLengthOverride, const uint8_t*& data, size_t& byteLength)
{
    if (!checkUnpackBufferBinding(false)) {
        return false;
    }

    size_t length = srcData->isDataViewObject() ? srcData->byteLength()
                                                : srcData->arrayLength();
    size_t elementSize = length ? srcData->byteLength() / length : 1;
    if (srcOffset > length || srcLengthOverride > length - srcOffset) {
        setGLError(GL_INVALID_VALUE,
                   "srcOffset or srcLengthOverride is out of range.");
        return false;
    }
    size_t count = srcLengthOverride ? srcLengthOverride : length - srcOffset;
    data = srcData->rawBuffer() + srcOffset * elementSize;
    byteLength = count * elementSize;
    return true;
}

void WebGL2RenderingContext::compressedTexImage2D(
    GLenum target, GLint level, GLenum internalformat, GLsizei width,
    GLsizei height, GLint border, GLsizei imageSize, GLintptr offset)
{
    ENTER_CONTEXT_SCOPE();

    if (checkUnpackBufferBinding(true)) {
        compressedTexImage(target, false, level, internalformat, width, height,
                           1, border, imageSize,
                           reinterpret_cast<const void*>(offset));
    }
}

void WebGL2RenderingContext::compressedTexImage2D(
    GLenum target, GLint level, GLenum internalformat, GLsizei width,
    GLsizei height, GLint border, ScriptArrayBufferView srcData,
    unsigned long long srcOffset, GLuint srcLengthOverride)
{
    ENTER_CONTEXT_SCOPE();

    const uint8_t* data;
    size_t byteLength;
    if (sliceCompressedTexData(srcData, srcOffset, srcLengthOverride, data,
                               byteLength)) {
        compressedTexImage(target, false, level, internalformat, width, height,
                           1, border, byteLength, data);
    }
}

void WebGL2RenderingContext::compressedTexSubImage2D(
    GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width,
    GLsizei height, GLenum format, GLsizei imageSize, GLintptr offset)
{
    ENTER_CONTEXT_SCOPE();

    if (checkUnpackBufferBinding(true)) {
        compressedTexSubImage(target, false, level, xoffset, yoffset, 0, width,
                              height, 1, format, imageSize,
                              reinterpret_cast<const void*>(offset));
    }
}

void WebGL2RenderingContext::compressedTexSubImage2D(
    GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width,
    GLsizei height, GLenum format, ScriptArrayBufferView srcData,
    unsigned long long srcOffset, GLuint srcLengthOverride)
{
    ENTER_CONTEXT_SCOPE();

    const uint8_t* data;
    size_t byteLength;
    if (sliceCompressedTexData(srcData, srcOffset, srcLengthOverride, data,
                               byteLength)) {
        compressedTexSubImage(target, false, level, xoffset, yoffset, 0, width,
                              height, 1, format, byteLength, data);
    }
}

void WebGL2RenderingContext::compressedTexImage3D(GLenum target, GLint level,
                                                  GLenum internalformat,
                                                  GLsizei width, GLsizei height,
                                                  GLsizei depth, GLint border,
                                                  GLsizei imageSize,
                                                  GLintptr offset)
{
    ENTER_CONTEXT_SCOPE();

    if (checkUnpackBufferBinding(true)) {
        compressedTexImage(target, true, level, internalformat, width, height,
                           depth, border, imageSize,
                           reinterpret_cast<const void*>(offset));
    }
}

void WebGL2RenderingContext::compressedTexImage3D(
    GLenum target, GLint level, GLenum internalformat, GLsizei width,
    GLsizei height, GLsizei depth, GLint border, ScriptArrayBufferView srcData,
    unsigned long long srcOffset, GLuint srcLengthOverride)
{
    ENTER_CONTEXT_SCOPE();

    const uint8_t* data;
    size_t byteLength;
    if (sliceCompressedTexData(srcData, srcOffset, srcLengthOverride, data,
                               byteLength)) {
        compressedTexImage(target, true, level, internalformat, width, height,
                           depth, border, byteLength, data);
    }
}

void WebGL2RenderingContext::compressedTexSubImage3D(
    GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
    GLsizei width, GLsizei height, GLsizei depth, GLenum format,
    GLsizei imageSize, GLintptr offset)
{
    ENTER_CONTEXT_SCOPE();

    if (checkUnpackBufferBinding(true)) {
        compressedTexSubImage(target, true, level, xoffset, yoffset, zoffset,
                              width, height, depth, format, imageSize,
                              reinterpret_cast<const void*>(offset));
    }
}

void WebGL2RenderingContext::compressedTexSubImage3D(
    GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
    GLsizei width, GLsizei height, GLsizei depth, GLenum format,
    ScriptArrayBufferView srcData, unsigned long long srcOffset,
    GLuint srcLengthOverride)
{
    ENTER_CONTEXT_SCOPE();

    const uint8_t* data;
    size_t byteLength;
    if (sliceCompressedTexData(srcData, srcOffset, srcLengthOverride, data,
                               byteLength)) {
        compressedTexSubImage(target, true, level, xoffset, yoffset, zoffset,
                              width, height, depth, format, byteLength, data);
    }
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

static bool isSupportedReadPixelsFormat(GLenum format)
{
    switch (format) {
    case GL_ALPHA:
    case GL_RGB:
    case GL_RGBA:
    case GL_RGBA_INTEGER:
    case GL_RGB_INTEGER:
    case GL_RG:
    case GL_RG_INTEGER:
    case GL_RED:
    case GL_RED_INTEGER:
    case GL_DEPTH_COMPONENT:
    case GL_DEPTH_STENCIL:
        return true;
    default:
        return false;
    }
}

static bool isSupportedReadPixelsType(GLenum type, size_t& alignment)
{
    switch (type) {
    case GL_UNSIGNED_BYTE:
    case GL_BYTE:
        alignment = 1;
        return true;
    case GL_UNSIGNED_SHORT:
    case GL_SHORT:
    case GL_HALF_FLOAT:
    case GL_UNSIGNED_SHORT_5_6_5:
    case GL_UNSIGNED_SHORT_4_4_4_4:
    case GL_UNSIGNED_SHORT_5_5_5_1:
        alignment = 2;
        return true;
    case GL_UNSIGNED_INT:
    case GL_INT:
    case GL_FLOAT:
    case GL_UNSIGNED_INT_2_10_10_10_REV:
    case GL_UNSIGNED_INT_10F_11F_11F_REV:
    case GL_UNSIGNED_INT_5_9_9_9_REV:
    case GL_FLOAT_32_UNSIGNED_INT_24_8_REV:
        alignment = 4;
        return true;
    default:
        return false;
    }
}

bool WebGL2RenderingContext::isReadBufferNone()
{
    if (isDefaultFramebufferBound()) {
        return m_defaultReadBuffer == GL_NONE;
    }
    Optional<WebGLFramebuffer*> maybeFb = getState()->webGLFramebuffer();
    if (maybeFb.hasValue() && maybeFb.value()) {
        return maybeFb.value()->readBuffer() == GL_NONE;
    }
    return false;
}

void WebGL2RenderingContext::readPixels(GLint x, GLint y, GLsizei width,
                                        GLsizei height, GLenum format,
                                        GLenum type,
                                        Optional<ScriptArrayBufferView> dstData)
{
    if (isReadBufferNone()) {
        setGLError(GL_INVALID_OPERATION, "readBuffer is GL_NONE");
        return;
    }
    WebGLRenderingContext::readPixels(x, y, width, height, format, type,
                                      dstData);
}

void WebGL2RenderingContext::readPixels(GLint x, GLint y, GLsizei width,
                                        GLsizei height, GLenum format,
                                        GLenum type, GLintptr offset)
{
    ENTER_CONTEXT_SCOPE();

    if (isReadBufferNone()) {
        setGLError(GL_INVALID_OPERATION, "readBuffer is GL_NONE");
        return;
    }

    if (width < 0 || height < 0 || offset < 0) {
        setGLError(GL_INVALID_VALUE);
        return;
    }

    size_t alignment = 1;
    if (!isSupportedReadPixelsFormat(format) ||
        !isSupportedReadPixelsType(type, alignment)) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    GLint packBuffer = 0;
    gl()->getIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &packBuffer);
    if (!packBuffer) {
        setGLError(GL_INVALID_OPERATION, "PIXEL_PACK_BUFFER is not bound");
        return;
    }

    if (offset % alignment != 0) {
        setGLError(GL_INVALID_OPERATION, "Offset not aligned");
        return;
    }

    size_t requiredSize = 0;
    if (!computeRequiredPackBytes(width, height, format, type, requiredSize)) {
        return;
    }
    GLint bufferSize = 0;
    gl()->getBufferParameteriv(GL_PIXEL_PACK_BUFFER, GL_BUFFER_SIZE,
                               &bufferSize);
    if (static_cast<size_t>(offset) + requiredSize >
        static_cast<size_t>(bufferSize)) {
        setGLError(GL_INVALID_OPERATION, "Buffer not large enough");
        return;
    }

    gl()->readPixels(x, y, width, height, format, type,
                     reinterpret_cast<void*>(offset));
}

void WebGL2RenderingContext::readPixels(GLint x, GLint y, GLsizei width,
                                        GLsizei height, GLenum format,
                                        GLenum type,
                                        ScriptArrayBufferView dstData,
                                        unsigned long long dstOffset)
{
    ENTER_CONTEXT_SCOPE();

    if (isReadBufferNone()) {
        setGLError(GL_INVALID_OPERATION, "readBuffer is GL_NONE");
        return;
    }

    if (width < 0 || height < 0) {
        setGLError(GL_INVALID_VALUE);
        return;
    }

    size_t alignment = 1;
    if (!isSupportedReadPixelsFormat(format) ||
        !isSupportedReadPixelsType(type, alignment)) {
        setGLError(GL_INVALID_ENUM);
        return;
    }

    GLint packBuffer = 0;
    gl()->getIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &packBuffer);
    if (packBuffer) {
        setGLError(GL_INVALID_OPERATION, "PIXEL_PACK_BUFFER is bound");
        return;
    }

    ScriptArrayBufferView pixelsView = dstData;
    bool isTypeMatched = false;
    if (type == GL_UNSIGNED_BYTE) {
        isTypeMatched = (pixelsView->isUint8ArrayObject() ||
                         pixelsView->isUint8ClampedArrayObject());
    } else if (type == GL_UNSIGNED_SHORT_5_6_5 ||
               type == GL_UNSIGNED_SHORT_4_4_4_4 ||
               type == GL_UNSIGNED_SHORT_5_5_5_1 || type == GL_UNSIGNED_SHORT ||
               type == GL_HALF_FLOAT) {
        isTypeMatched = pixelsView->isUint16ArrayObject();
    } else if (type == GL_BYTE) {
        isTypeMatched = pixelsView->isInt8ArrayObject();
    } else if (type == GL_SHORT) {
        isTypeMatched = pixelsView->isInt16ArrayObject();
    } else if (type == GL_INT) {
        isTypeMatched = pixelsView->isInt32ArrayObject();
    } else if (type == GL_UNSIGNED_INT) {
        isTypeMatched = pixelsView->isUint32ArrayObject();
    } else if (type == GL_FLOAT) {
        isTypeMatched = pixelsView->isFloat32ArrayObject();
    }

    if (!isTypeMatched) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    bool isIntegerFormat =
        (format == GL_RGBA_INTEGER || format == GL_RGB_INTEGER ||
         format == GL_RG_INTEGER || format == GL_RED_INTEGER);
    bool isIntegerType =
        (type == GL_INT || type == GL_UNSIGNED_INT || type == GL_SHORT ||
         type == GL_UNSIGNED_SHORT || type == GL_BYTE ||
         type == GL_UNSIGNED_BYTE || type == GL_UNSIGNED_INT_2_10_10_10_REV);
    bool isNormalizedFloatFormat = (format == GL_RGBA || format == GL_RGB ||
                                    format == GL_RG || format == GL_RED);
    bool isNormalizedFloatType =
        (type == GL_UNSIGNED_BYTE || type == GL_BYTE || type == GL_FLOAT ||
         type == GL_HALF_FLOAT || type == GL_UNSIGNED_SHORT_5_6_5 ||
         type == GL_UNSIGNED_SHORT_4_4_4_4 ||
         type == GL_UNSIGNED_SHORT_5_5_5_1 ||
         type == GL_UNSIGNED_INT_2_10_10_10_REV);
    bool isValidCombination =
        (isNormalizedFloatFormat && isNormalizedFloatType) ||
        (isIntegerFormat && isIntegerType);

    if (!isValidCombination) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    size_t elementSize = dstData->isDataViewObject()
                             ? 1
                             : dstData->byteLength() / dstData->arrayLength();
    size_t elementCount = dstData->isDataViewObject() ? dstData->byteLength()
                                                      : dstData->arrayLength();

    if (dstOffset > elementCount) {
        setGLError(GL_INVALID_VALUE, "dstOffset is out of bounds");
        return;
    }

    size_t byteLengthOfPixels = 0;
    if (!computeRequiredPackBytes(width, height, format, type,
                                  byteLengthOfPixels)) {
        return;
    }
    size_t remainingBytes = dstData->byteLength() - (dstOffset * elementSize);

    if (remainingBytes < byteLengthOfPixels) {
        setGLError(GL_INVALID_OPERATION, "buffer view is too small");
        return;
    }

    GLvoid* data = dstData->rawBuffer() + dstData->byteOffset() +
                   (dstOffset * elementSize);
    gl()->readPixels(x, y, width, height, format, type, data);
}

void WebGL2RenderingContext::copyTexSubImage3D(GLenum target, GLint level,
                                               GLint xoffset, GLint yoffset,
                                               GLint zoffset, GLint x, GLint y,
                                               GLsizei width, GLsizei height)
{
    ENTER_CONTEXT_SCOPE();

    if (target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY) {
        setGLError(GL_INVALID_ENUM, "Invalid texture target");
        return;
    }

    if (level < 0 || xoffset < 0 || yoffset < 0 || zoffset < 0 || width < 0 ||
        height < 0) {
        setGLError(GL_INVALID_VALUE, "negative offset or dimension");
        return;
    }

    WebGLTexture* texture = getBoundTextureObject(target);
    if (!texture || !hasBoundTexture(target)) {
        setGLError(GL_INVALID_OPERATION, "No texture bound to target");
        return;
    }

    // Check feedback loop with bound read framebuffer
    Optional<WebGLFramebuffer*> maybeFb =
        getBoundFramebuffer(GL_READ_FRAMEBUFFER);
    if (maybeFb.hasValue() && maybeFb.value()) {
        WebGLFramebuffer* fb = maybeFb.value();
        if (fb->hasFeedbackLoop(texture, level, zoffset)) {
            setGLError(GL_INVALID_OPERATION,
                       "Feedback loop detected in copyTexSubImage3D");
            return;
        }
    }

    gl()->copyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y,
                            width, height);
}

} // namespace Starfish

#endif // defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)
