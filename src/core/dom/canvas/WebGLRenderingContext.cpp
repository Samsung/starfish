/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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
#include "core/dom/canvas/WebGLRenderingContext.h"
#include "core/dom/canvas/HTMLCanvasElement.h"
#include "core/dom/canvas/CanvasImageSource.h"
#include "core/dom/HTMLImageElement.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/canvas/WebGLExtensions.h"
#include "core/modules/canvas/image/NativeImageData.h"
#include "core/modules/worker/util/Trace.h"
#include "core/util/String.h"
#include "platform/canvas/webgl/GLContext.h"
#include "platform/canvas/webgl/XGLPlatform.h"
#include "core/dom/canvas/WebGLActiveInfo.h"
#include "core/dom/canvas/WebGLBuffer.h"
#include "core/dom/canvas/WebGLShader.h"
#include "core/dom/canvas/WebGLProgram.h"
#include "core/dom/canvas/WebGLTexture.h"
#include "core/dom/canvas/WebGLUniformLocation.h"
#include "core/modules/canvas/Canvas.h"
#include "core/dom/canvas/CanvasRenderingContext.h"
#include "core/dom/canvas/CanvasRenderingContext2DMixIn.h"
#include "binding/ScriptBindingInstance.h"
#include "binding/generated/Float32ArrayOrSequenceOfGLfloatUnion.h"
#include "binding/generated/Int32ArrayOrSequenceOfGLintUnion.h"
#include "binding/generated/ArrayBufferOrSharedArrayBufferOrArrayBufferViewUnion.h"
#include "binding/generated/ImageBitmapOrImageDataOrHTMLImageElementOrHTMLCanvasElementOrHTMLVideoElementUnion.h"
#include "core/dom/canvas/WebGLOES_VertexArrayObject.h"
#include <EscargotPublic.h>
#include <sstream>
#include <iomanip>

#define kMaximumUniformAndAttributeLocationLengths 256
#define kMaximumSupportedStride 255

/* WebGL-specific enums */
static const GLenum kUNPACK_FLIP_Y_WEBGL = 0x9240;
static const GLenum kUNPACK_PREMULTIPLY_ALPHA_WEBGL = 0x9241;
static const GLenum kCONTEXT_LOST_WEBGL = 0x9242;
static const GLenum kUNPACK_COLORSPACE_CONVERSION_WEBGL = 0x9243;
static const GLenum kBROWSER_DEFAULT_WEBGL = 0x9244;

namespace Starfish {

inline static std::string hex(GLenum name)
{
    return StringUtils::formatString("0x%04X", name);
}

static void copyFloat32List(ScriptBindingInstance* instance,
                            const Float32List& variant,
                            std::vector<float>& vector)
{
    Evaluator::EvaluatorResult evaluated = Evaluator::execute(
        instance->scriptContext(),
        [](ExecutionStateRef* state, const Float32List* variant,
           std::vector<float>* vector) -> ValueRef* {
            if (variant->isFloat32ArrayValue()) {
                Float32ArrayObjectRef* values = variant->getFloat32ArrayValue();
                const size_t arrayLength = values->arrayLength();

                for (size_t i = 0; i < arrayLength; ++i) {
                    vector->push_back(values->get(state, ValueRef::create(i))
                                          ->toNumber(state));
                }
            } else {
                STARFISH_ASSERT(variant->isSequenceOfGLfloatValue());

                for (const auto& value : variant->getSequenceOfGLfloatValue()) {
                    vector->push_back(static_cast<float>(value));
                }
            }
            return ValueRef::createUndefined();
        },
        &variant, &vector);

    STARFISH_ASSERT(evaluated.isSuccessful());
}

static void copyInt32List(ScriptBindingInstance* instance,
                          const Int32List& variant,
                          std::vector<int32_t>& vector)
{
    Evaluator::EvaluatorResult evaluated = Evaluator::execute(
        instance->scriptContext(),
        [](ExecutionStateRef* state, const Int32List* variant,
           std::vector<int32_t>* vector) -> ValueRef* {
            if (variant->isInt32ArrayValue()) {
                Int32ArrayObjectRef* values = variant->getInt32ArrayValue();
                const size_t arrayLength = values->arrayLength();

                for (size_t i = 0; i < arrayLength; ++i) {
                    vector->push_back(values->get(state, ValueRef::create(i))
                                          ->toNumber(state));
                }
            } else {
                STARFISH_ASSERT(variant->isSequenceOfGLintValue());

                for (const auto& value : variant->getSequenceOfGLintValue()) {
                    vector->push_back(static_cast<int32_t>(value));
                }
            }
            return ValueRef::createUndefined();
        },
        &variant, &vector);

    STARFISH_ASSERT(evaluated.isSuccessful());
}

static GLint getCurrentProgram()
{
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    return program;
}

WebGLRenderingContext::WebGLRenderingContext(HTMLCanvasElement* canvasElement)
    : WebGLRenderingContextBaseMixIn(canvasElement)
{
    m_unpackFlipY = false;
    m_unpackPremultiplyAlpha = false;
    m_unpackColorspaceConversion = kBROWSER_DEFAULT_WEBGL;
}

WebGLRenderingContext::~WebGLRenderingContext()
{
}

ScriptBindingInstance* WebGLRenderingContext::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

void WebGLRenderingContext::initialize()
{
    WebGLRenderingContextBaseMixIn::initialize();

    viewport(0, 0, m_canvasSurface->width(), m_canvasSurface->height());
}

#define ENTER_CONTEXT_SCOPE_IMPL(bailoutValue, ...)                         \
    WebGLContextScope contextScope(m_context, m_framebufferTexture->fbo()); \
    if (contextScope.hasError()) {                                          \
        TRACE(WEBGL, "GL Context error detected.");                         \
        return bailoutValue;                                                \
    }                                                                       \
    m_ownerHTMLCanvasElement                                                \
        ->setNeedsComposite(); // TODO: Use CanvasElement::setNeedsComposite
                               // only when really necessary.

#ifdef NDEBUG
#define ENTER_CONTEXT_SCOPE(bailoutValue, ...) \
    ENTER_CONTEXT_SCOPE_IMPL(bailoutValue);
#else
#define ENTER_CONTEXT_SCOPE(bailoutValue, ...)       \
    ENTER_CONTEXT_SCOPE_IMPL(bailoutValue);          \
    auto onScopeLeave = OnScopeLeave::create([&]() { \
        if (hasGLError()) {                          \
            TRACE(WEBGL, "GL error detected.");      \
        }                                            \
    });
#endif

/*
Note: Use hasGLError() to internally check for GL errors. When `glGetError` is
called, the code returned is cleared inside it. If we use `glGetError` directly,
users would not be able to get error code properly. So, we first store the code
from `glGetError`, and then use it. The error code stored will be cleared when
users call gl.getError().
*/
bool WebGLRenderingContext::hasGLError()
{
    updateGLError();

    if (!m_GLErrors.empty()) {
        return true;
    }
    return false;
}

void WebGLRenderingContext::updateGLError()
{
    GLenum code = glGetError();
    if (code != GL_NO_ERROR) {
        TRACE(WEBGL, "Error:", hex(code));
        setGLError(code);
    }
}

GLenum WebGLRenderingContext::getError()
{
    WebGLContextScope contextScope(m_context, m_framebufferTexture->fbo());

    updateGLError();

    GLenum code = GL_NO_ERROR;
    if (!m_GLErrors.empty()) {
        code = *m_GLErrors.begin();
        m_GLErrors.erase(m_GLErrors.begin());
    }
    return code;
}

void WebGLRenderingContext::setGLError(GLenum code, const char* message)
{
    m_GLErrors.insert(code);
    if (message) {
        TRACE(WEBGL, "Error(%s): %s", webglErrorString(code), message);
    }
}

GLsizei WebGLRenderingContext::drawingBufferWidth() const
{
    return m_canvasSurface->bufferWidth();
}

GLsizei WebGLRenderingContext::drawingBufferHeight() const
{
    return m_canvasSurface->bufferHeight();
}

Nullable<WebGLContextAttributes> WebGLRenderingContext::getContextAttributes()
{
    ENTER_CONTEXT_SCOPE(Nullable<WebGLContextAttributes>());

    return m_attributes;
}

Nullable<GCVector<String*>> WebGLRenderingContext::getSupportedExtensions()
{
    ENTER_CONTEXT_SCOPE(Nullable<GCVector<String*>>());

    return WebGLExtensionRegistry::instance().getSupportedExtensions();
}

bool WebGLRenderingContext::isExtensionEnabled(const char* name)
{
    const auto& iter = m_enabledExtensions.find(name);
    if (iter != m_enabledExtensions.end()) {
        return true;
    }
    return false;
}

Nullable<ScriptObject> WebGLRenderingContext::getExtension(
    String* requestedName)
{
    ENTER_CONTEXT_SCOPE(Nullable<ScriptObject>());

    // TODO: An attempt to use any features of an extension without first
    // calling getExtension to enable it must generate an appropriate GL
    // error and must not make use of the feature.

    std::string name = requestedName->toUTF8NonGCString();
    const auto& iter = m_enabledExtensions.find(name);
    if (iter != m_enabledExtensions.end()) {
        // Multiple calls to getExtension with the same extension
        // string, taking into account case-insensitive comparison, must
        // return the same object as long as the extension is enabled.
        return iter->second;
    }

    Optional<ExtensionGenerator> maybeGenerator =
        WebGLExtensionRegistry::instance().getGenerator(name);

    if (!maybeGenerator.hasValue()) {
        return Nullable<ScriptObject>();
    }

    ScriptObject object = maybeGenerator.value()(scriptBindingInstance(), this);
    m_enabledExtensions.insert({ name, object });
    return object;
}

void WebGLRenderingContext::activeTexture(GLenum texture)
{
    ENTER_CONTEXT_SCOPE();

    glActiveTexture(texture);
}

void WebGLRenderingContext::attachShader(WebGLProgram* program,
                                         WebGLShader* shader)
{
    if (!checkWebGLObject(program) || !checkWebGLObject(shader)) {
        return;
    }

    ENTER_CONTEXT_SCOPE();

    glAttachShader(program->glObject(), shader->glObject());
}

void WebGLRenderingContext::bindAttribLocation(WebGLProgram* program,
                                               GLuint index, String* name)
{
    ENTER_CONTEXT_SCOPE();

    if (!checkWebGLObject(program)) {
        return;
    }

    if (!checkAttribOrUniformName(name)) {
        return;
    }

    glBindAttribLocation(program->glObject(), index, CSTR(name));
}

void WebGLRenderingContext::bindBuffer(GLenum target,
                                       Nullable<WebGLBuffer*> buffer)
{
    ENTER_CONTEXT_SCOPE();

    if (buffer.hasValue()) {
        WebGLBuffer* value = buffer.value();

        if (value->target() != GL_NONE && value->target() != target) {
            // An attempt to bind a buffer object to the other target will
            // generate an INVALID_OPERATION error, and the current binding will
            // remain untouched. (Note: This isn't a GLES Spec., but WebGL one.
            // We need to set it directly, not use a retrieved value.)
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        glBindBuffer(target, value->glObject());

        // A given WebGLBuffer object may only be bound to one of the
        // ARRAY_BUFFER or ELEMENT_ARRAY_BUFFER target in its lifetime.
        value->setTargetOnce(target);
    } else {
        // If the buffer is null then any buffer currently bound is unbound.
        glBindBuffer(target, 0);
    }
}

void WebGLRenderingContext::bindTexture(GLenum target,
                                        Nullable<WebGLTexture*> maybeTexture)
{
    ENTER_CONTEXT_SCOPE();

    if (maybeTexture.hasValue()) {
        WebGLTexture* texture = maybeTexture.value();

        if (!checkWebGLObject(texture)) {
            return;
        }

        if (texture->isDeleted()) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        TRACE(WEBGL, "target", hex(target), "ON");
        glBindTexture(target, texture->glObject());
        m_boundTextures[target] = texture->glObject();
    } else {
        TRACE(WEBGL, "target", hex(target), "OFF");
        glBindTexture(target, 0);
        m_boundTextures.erase(target);
    }
}

void WebGLRenderingContext::clear(uint32_t mask)
{
    ENTER_CONTEXT_SCOPE();

    glClear(mask);
}

void WebGLRenderingContext::clearColor(float red, float green, float blue,
                                       float alpha)
{
    ENTER_CONTEXT_SCOPE();

    glClearColor(red, green, blue, alpha);
}

void WebGLRenderingContext::clearDepth(GLclampf depth)
{
    ENTER_CONTEXT_SCOPE();

    glClearDepthf(depth);
}

void WebGLRenderingContext::clearStencil(GLint s)
{
    ENTER_CONTEXT_SCOPE();

    glClearStencil(s);
}

void WebGLRenderingContext::colorMask(GLboolean red, GLboolean green,
                                      GLboolean blue, GLboolean alpha)
{
    ENTER_CONTEXT_SCOPE();

    glColorMask(red, green, blue, alpha);
}

void WebGLRenderingContext::compileShader(WebGLShader* shader)
{
    ENTER_CONTEXT_SCOPE();

    if (!checkWebGLObject(shader)) {
        return;
    }

    glCompileShader(shader->glObject());
}

WebGLBuffer* WebGLRenderingContext::createBuffer()
{
    ENTER_CONTEXT_SCOPE(nullptr);

    GLuint buffer = 0;
    glGenBuffers(1, &buffer);
    return new WebGLBuffer(scriptBindingInstance(), this, buffer);
}

WebGLProgram* WebGLRenderingContext::createProgram()
{
    ENTER_CONTEXT_SCOPE(nullptr);
    return new WebGLProgram(scriptBindingInstance(), this, glCreateProgram());
}

WebGLShader* WebGLRenderingContext::createShader(unsigned long type)
{
    ENTER_CONTEXT_SCOPE(nullptr);

    return new WebGLShader(scriptBindingInstance(), this, glCreateShader(type));
}

WebGLTexture* WebGLRenderingContext::createTexture()
{
    ENTER_CONTEXT_SCOPE(nullptr);

    GLuint textureId = 0;
    glGenTextures(1, &textureId);
    return new WebGLTexture(scriptBindingInstance(), this, textureId);
}

void WebGLRenderingContext::deleteShader(WebGLShader* shader)
{
    ENTER_CONTEXT_SCOPE();

    if (!checkWebGLObject(shader) || shader->isDeleted()) {
        return;
    }

    glDeleteShader(shader->glObject());
    shader->markDeleted();
}

void WebGLRenderingContext::depthMask(GLboolean flag)
{
    ENTER_CONTEXT_SCOPE();

    glDepthMask(flag);
}

void WebGLRenderingContext::depthFunc(GLenum func)
{
    ENTER_CONTEXT_SCOPE();

    glDepthFunc(func);
}

void WebGLRenderingContext::disable(GLenum cap)
{
    ENTER_CONTEXT_SCOPE();

    glDisable(cap);
}

void WebGLRenderingContext::cullFace(GLenum mode)
{
    ENTER_CONTEXT_SCOPE();

    glCullFace(mode);
}

void WebGLRenderingContext::drawArrays(GLenum mode, GLint first, GLsizei count)
{
    ENTER_CONTEXT_SCOPE();

    if (first < 0) {
        // If first is negative, an INVALID_VALUE error will be generated.
        setGLError(GL_INVALID_VALUE);
        return;
    }

    if (!getCurrentProgram()) {
        // If the CURRENT_PROGRAM is null, an INVALID_OPERATION error will be
        // generated.
        setGLError(GL_INVALID_OPERATION);
    }

    glDrawArrays(mode, first, count);
}

void WebGLRenderingContext::drawElements(GLenum mode, GLsizei count,
                                         GLenum type, GLintptr offset)
{
    ENTER_CONTEXT_SCOPE();

    if (count > 0) {
        // TODO: verify a non-null WebGLBuffer is bound to the
        // ELEMENT_ARRAY_BUFFER binding point if count is greater than zero. If
        // not, an INVALID_OPERATION error will be generated.
    }

    if (offset < 0) {
        // the offset must be non-negative or an INVALID_VALUE error will be
        // generated.
        setGLError(GL_INVALID_VALUE);
        return;
    }

    if (!getCurrentProgram()) {
        // If the CURRENT_PROGRAM is null, an INVALID_OPERATION error will be
        // generated.
        setGLError(GL_INVALID_OPERATION);
    }

    glDrawElements(mode, count, type, reinterpret_cast<void*>(offset));
}

void WebGLRenderingContext::enable(GLenum cap)
{
    ENTER_CONTEXT_SCOPE();

    glEnable(cap);
}

void WebGLRenderingContext::enableVertexAttribArray(GLuint index)
{
    ENTER_CONTEXT_SCOPE();
    /*
        NOTE: No idea to handle the following for now. It may already be handled
        in GLES3: WebGL imposes additional rules beyond OpenGL ES 2.0 regarding
        enabled vertex attributes; see Enabled Vertex Attributes and Range
        Checking.
    */
    glEnableVertexAttribArray(index);
}

void WebGLRenderingContext::frontFace(GLenum mode)
{
    ENTER_CONTEXT_SCOPE();

    glFrontFace(mode);
}

void WebGLRenderingContext::shaderSource(WebGLShader* shader, String* source)
{
    ENTER_CONTEXT_SCOPE();

    std::string str = source->toUTF8NonGCString();
    const char* sourceArray[1] = { str.c_str() };

    glShaderSource(shader->glObject(), 1, sourceArray, nullptr);
}

void WebGLRenderingContext::stencilMask(GLuint mask)
{
    ENTER_CONTEXT_SCOPE();

    glStencilMask(mask);
}

ScriptValue WebGLRenderingContext::getParameter(GLenum pname)
{
    ENTER_CONTEXT_SCOPE(scriptNull());

    switch (pname) {
    case GL_ALPHA_BITS:
    case GL_BLUE_BITS:
    case GL_DEPTH_BITS:
    case GL_GREEN_BITS:
    case GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS:
    case GL_MAX_CUBE_MAP_TEXTURE_SIZE:
    case GL_MAX_FRAGMENT_UNIFORM_VECTORS:
    case GL_MAX_RENDERBUFFER_SIZE:
    case GL_MAX_TEXTURE_IMAGE_UNITS:
    case GL_MAX_TEXTURE_SIZE:
    case GL_MAX_VARYING_VECTORS:
    case GL_MAX_VERTEX_ATTRIBS:
    case GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:
    case GL_MAX_VERTEX_UNIFORM_VECTORS:
    case GL_PACK_ALIGNMENT:
    case GL_RED_BITS:
    case GL_SAMPLE_BUFFERS:
    case GL_SAMPLES:
    case GL_STENCIL_BACK_REF:
    case GL_STENCIL_BITS:
    case GL_STENCIL_CLEAR_VALUE:
    case GL_STENCIL_REF:
    case GL_SUBPIXEL_BITS:
    case GL_UNPACK_ALIGNMENT: {
        std::vector<int> values(1);
        glGetIntegerv(pname, &values[0]);
        return ValueRef::create(values[0]);
    }
    case GL_VERTEX_ARRAY_BINDING: {
        // GL_VERTEX_ARRAY_BINDING_OES
        if (!isExtensionEnabled("OES_vertex_array_object")) {
            setGLError(GL_INVALID_ENUM);
            return scriptNull();
        }
        GLint value = -1;
        glGetIntegerv(pname, &value);
        if (value == 0) {
            return scriptNull();
        }

        Nullable<WebGLVertexArrayObjectOES*> maybe =
            m_state.webGLVertexArrayObjectOES();

        if (!maybe.hasValue() || maybe.value()->isDeleted()) {
            return scriptNull();
        }

        STARFISH_ASSERT(static_cast<GLint>(maybe.value()->glObject()) == value);
        return maybe.value()->scriptValue();
    }


    case GL_RENDERER:
    case GL_SHADING_LANGUAGE_VERSION:
    case GL_VERSION:
    case GL_VENDOR: {
        const std::string output =
            reinterpret_cast<const char*>(glGetString(pname));
        return StringRef::createFromASCII(output.c_str(), output.length());
    }
    case GL_MAX_VIEWPORT_DIMS: {
        std::vector<int> values(2);
        glGetIntegerv(pname, &values[0]);
        return createTypedArray<Int32ArrayObjectRef>(scriptBindingInstance(),
                                                     values);
    }
    case GL_SCISSOR_BOX:
    case GL_VIEWPORT: {
        std::vector<int> values(4);
        glGetIntegerv(pname, &values[0]);
        return createTypedArray<Int32ArrayObjectRef>(scriptBindingInstance(),
                                                     values);
    }
    default:
        STARFISH_UNIMPLEMENTED("pname: 0x%04X", pname);
        return scriptNull();
    }
    return scriptNull();
}

WebGLActiveInfo* WebGLRenderingContext::getActiveAttrib(WebGLProgram* program,
                                                        GLuint index)
{
    ENTER_CONTEXT_SCOPE(nullptr);

    GLint maxNameLength;
    glGetProgramiv(program->glObject(), GL_ACTIVE_ATTRIBUTE_MAX_LENGTH,
                   &maxNameLength);

    GLint size;
    GLenum type;
    GLsizei length;

    std::vector<char> name;
    name.resize(maxNameLength, '\0');
    glGetActiveAttrib(program->glObject(), index, maxNameLength, &length, &size,
                      &type, &name[0]);

    if (hasGLError()) {
        // a) If the passed index is out of range, generates an INVALID_VALUE
        // error and returns null. b) Returns null if any OpenGL errors are
        // generated during the execution of this function.
        return nullptr;
    }

    return new WebGLActiveInfo(scriptBindingInstance(), size, type,
                               String::createASCIIString(name.data(), length));
}

WebGLActiveInfo* WebGLRenderingContext::getActiveUniform(WebGLProgram* program,
                                                         GLuint index)
{
    ENTER_CONTEXT_SCOPE(nullptr);

    GLint maxNameLength;
    glGetProgramiv(program->glObject(), GL_ACTIVE_UNIFORM_MAX_LENGTH,
                   &maxNameLength);

    GLint size;
    GLenum type;
    GLsizei length;

    std::vector<char> name;
    name.resize(maxNameLength, '\0');
    glGetActiveUniform(program->glObject(), index, maxNameLength, &length,
                       &size, &type, &name[0]);

    if (hasGLError()) {
        // a) If the passed index is out of range, generates an INVALID_VALUE
        // error and returns null. b) Returns null if any OpenGL errors are
        // generated during the execution of this function.
        return nullptr;
    }

    return new WebGLActiveInfo(scriptBindingInstance(), size, type,
                               String::createASCIIString(name.data(), length));
}

GLint WebGLRenderingContext::getAttribLocation(WebGLProgram* program,
                                               String* name)
{
    ENTER_CONTEXT_SCOPE(-1);

    if (!checkWebGLObject(program)) {
        return -1;
    }

    if (!checkAttribOrUniformName(name)) {
        return -1;
    }

    // TODO: Returns -1 if the context's webgl context lost flag is set.

    if (program->invalidated()) {
        // If the invalidated flag of the passed program is set, generates an
        // INVALID_OPERATION error and returns -1.
        setGLError(GL_INVALID_OPERATION);
        return -1;
    }

    return glGetAttribLocation(program->glObject(), CSTR(name));
}

ScriptValue WebGLRenderingContext::getProgramParameter(WebGLProgram* program,
                                                       GLenum pname)
{
    ENTER_CONTEXT_SCOPE(scriptNull());

    if (!checkWebGLObject(program)) {
        return scriptNull();
    }

    GLint params = 0;
    glGetProgramiv(program->glObject(), pname, &params);

    if (hasGLError()) {
        /*
         - GL_INVALID_ENUM if pname is not an accepted value.
         - GL_INVALID_VALUE if program is not a value generated by OpenGL.
         - GL_INVALID_OPERATION if program does not refer to a program object.
        */
        return scriptNull();
    }

    switch (pname) {
    case GL_DELETE_STATUS:
    case GL_LINK_STATUS:
    case GL_VALIDATE_STATUS:
        return Escargot::ValueRef::create(static_cast<bool>(params));
    case GL_ATTACHED_SHADERS:
    case GL_ACTIVE_ATTRIBUTES:
    case GL_ACTIVE_UNIFORMS:
        return Escargot::ValueRef::create(static_cast<GLint>(params));
    default:
        break;
    }
    return scriptNull();
}

String* WebGLRenderingContext::getProgramInfoLog(WebGLProgram* program)
{
    ENTER_CONTEXT_SCOPE(nullptr);

    GLsizei length = 0, bufferSize = 0;
    glGetProgramiv(program->glObject(), GL_INFO_LOG_LENGTH, &bufferSize);

    std::string buffer;
    buffer.reserve(bufferSize);
    glGetProgramInfoLog(program->glObject(), bufferSize, &length, &buffer[0]);

    if (hasGLError()) {
        return nullptr;
    }

    TRACE(WEBGL, buffer);

    return String::fromUTF8(buffer.data(), length);
}

ScriptValue WebGLRenderingContext::getShaderParameter(WebGLShader* shader,
                                                      GLenum pname)
{
    ENTER_CONTEXT_SCOPE(scriptNull());

    if (!checkWebGLObject(shader)) {
        return scriptNull();
    }

    GLint params = 0;
    glGetShaderiv(shader->glObject(), pname, &params);

    if (hasGLError()) {
        /*
        - GL_INVALID_ENUM if pname is not an accepted value.
        - GL_INVALID_VALUE if shader is not a value generated by OpenGL.
        - GL_INVALID_OPERATION if shader does not refer to a shader object.
        */
        return scriptNull();
    }

    switch (pname) {
    case GL_SHADER_TYPE:
        return Escargot::ValueRef::create(static_cast<GLenum>(params));
    case GL_DELETE_STATUS:
    case GL_COMPILE_STATUS:
        return Escargot::ValueRef::create(static_cast<bool>(params));
    default:
        break;
    }
    return scriptNull();
}

String* WebGLRenderingContext::getShaderInfoLog(WebGLShader* shader)
{
    ENTER_CONTEXT_SCOPE(nullptr);

    GLsizei length = 0, bufferSize = 0;
    glGetShaderiv(shader->glObject(), GL_INFO_LOG_LENGTH, &bufferSize);

    std::string buffer;
    buffer.reserve(bufferSize);
    glGetShaderInfoLog(shader->glObject(), bufferSize, &length, &buffer[0]);

    if (hasGLError()) {
        return nullptr;
    }

    TRACE(WEBGL, buffer);

    return String::fromUTF8(buffer.data(), length);
}

String* WebGLRenderingContext::getShaderSource(WebGLShader* shader)
{
    ENTER_CONTEXT_SCOPE(nullptr);

    GLsizei length = 0, bufferSize = 0;
    glGetShaderiv(shader->glObject(), GL_SHADER_SOURCE_LENGTH, &bufferSize);

    std::string buffer;
    buffer.reserve(bufferSize);
    glGetShaderSource(shader->glObject(), bufferSize, &length, &buffer[0]);

    if (hasGLError()) {
        return nullptr;
    }

    return String::fromUTF8(buffer.data(), length);
}

WebGLUniformLocation* WebGLRenderingContext::getUniformLocation(
    WebGLProgram* program, String* name)
{
    ENTER_CONTEXT_SCOPE(nullptr);

    if (!checkWebGLObject(program)) {
        return nullptr;
    }

    if (!checkAttribOrUniformName(name)) {
        return nullptr;
    }

    GLint location = glGetUniformLocation(program->glObject(), CSTR(name));
    if (location == -1) {
        /*
          - WebGL: The return value is null if name does not correspond to an
            active uniform variable in the passed program.
          - GLES: glGetUniformLocation returns -1 if name does not correspond to
            an active uniform variable in program or if name is associated with
            a named uniform block.
        */

        return nullptr;
    }

    if (hasGLError()) {
        // Returns null if any OpenGL errors are generated during the execution
        // of this function.
        return nullptr;
    }

    return new WebGLUniformLocation(scriptBindingInstance(), program, location);
}
ScriptValue WebGLRenderingContext::getVertexAttrib(GLuint index, GLenum pname)
{
    ENTER_CONTEXT_SCOPE(scriptNull());

    switch (pname) {
    case GL_CURRENT_VERTEX_ATTRIB: {
        std::vector<float> values(4);
        glGetVertexAttribfv(index, pname, &values[0]);
        return createTypedArray<Float32ArrayObjectRef>(scriptBindingInstance(),
                                                       values);
    }
    case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
    case GL_VERTEX_ATTRIB_ARRAY_SIZE:
    case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
    case GL_VERTEX_ATTRIB_ARRAY_TYPE:
    case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
    case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
        STARFISH_UNIMPLEMENTED("pname: 0x%04X", pname);
        break;
    default:
        break;
    }
    return scriptNull();
}

void WebGLRenderingContext::linkProgram(WebGLProgram* program)
{
    ENTER_CONTEXT_SCOPE();

    if (!checkWebGLObject(program)) {
        return;
    }

    /*
        NOTE: No idea to handle the following for now. It may already be handled
        in GLES3: 6.26 Packing Restrictions for Uniforms and Varyings: The WebGL
        API further requires that if the packing algorithm fails either for the
        uniform variables of a shader or for the varying variables of a program,
        compilation or linking must fail.
    */

    glLinkProgram(program->glObject());

    if (hasGLError()) {
        /*
            NOTE: No idea to handle the following for now. It may already be
            handled in GLES3: If the given program is also the the current
            program object in use as defined by useProgram, then: If the program
            is not linked successfully, the executable code referenced by the
            current rendering state is immediately invalidated. Further draw
            calls that utilize the current program generate an INVALID_OPERATION
            error. See Current program invalidated upon unsuccessful
            link(https://registry.khronos.org/webgl/specs/latest/1.0/#6.43).
        */
        STARFISH_UNIMPLEMENTED();
    }
}

void WebGLRenderingContext::pixelStorei(GLenum pname, GLint param)
{
    ENTER_CONTEXT_SCOPE();

    switch (pname) {
    case kUNPACK_FLIP_Y_WEBGL:
        m_unpackFlipY = static_cast<bool>(param);
        break;
    case kUNPACK_PREMULTIPLY_ALPHA_WEBGL:
        m_unpackPremultiplyAlpha = static_cast<bool>(param);
        break;
    case kUNPACK_COLORSPACE_CONVERSION_WEBGL:
        if (param == kBROWSER_DEFAULT_WEBGL || param == GL_NONE) {
            m_unpackColorspaceConversion = param;
        }
        break;
    default:
        glPixelStorei(pname, param);
        break;
    }
}

void WebGLRenderingContext::texParameteri(GLenum target, GLenum pname,
                                          GLint param)
{
    ENTER_CONTEXT_SCOPE();

    if (m_boundTextures.find(target) == m_boundTextures.end()) {
        // If an attempt is made to call this function with no WebGLTexture
        // bound, an INVALID_OPERATION error is generated.
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    glTexParameteri(target, pname, param);
}

void WebGLRenderingContext::uniform1f(WebGLUniformLocation* uniform, GLfloat x)
{
    ENTER_CONTEXT_SCOPE();

    // Each of the uniform* functions sets the specified uniform or uniforms to
    // the values provided.
    if (!isFromCurrentProgram(uniform)) {
        // If the passed location is not null and was not obtained from the
        // currently used program via an earlier call to getUniformLocation, an
        // INVALID_OPERATION error will be generated.
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    // If the passed location is null, the data passed in will be silently
    // ignored and no uniform variables will be changed.
    glUniform1f(uniform->location(), x);
}

void WebGLRenderingContext::uniform2f(WebGLUniformLocation* uniform, GLfloat x,
                                      GLfloat y)
{
    ENTER_CONTEXT_SCOPE();

    if (!isFromCurrentProgram(uniform)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    glUniform2f(uniform->location(), x, y);
}

void WebGLRenderingContext::uniform3f(WebGLUniformLocation* uniform, GLfloat x,
                                      GLfloat y, GLfloat z)
{
    ENTER_CONTEXT_SCOPE();

    if (!isFromCurrentProgram(uniform)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    glUniform3f(uniform->location(), x, y, z);
}

void WebGLRenderingContext::uniform4f(WebGLUniformLocation* uniform, GLfloat x,
                                      GLfloat y, GLfloat z, GLfloat w)
{
    ENTER_CONTEXT_SCOPE();

    if (!isFromCurrentProgram(uniform)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    glUniform4f(uniform->location(), x, y, z, w);
}

void WebGLRenderingContext::uniform1i(WebGLUniformLocation* uniform, GLint x)
{
    ENTER_CONTEXT_SCOPE();

    if (!isFromCurrentProgram(uniform)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    glUniform1i(uniform->location(), x);
}

void WebGLRenderingContext::uniform2i(WebGLUniformLocation* uniform, GLint x,
                                      GLint y)
{
    ENTER_CONTEXT_SCOPE();

    if (!isFromCurrentProgram(uniform)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    glUniform2i(uniform->location(), x, y);
}

void WebGLRenderingContext::uniform3i(WebGLUniformLocation* uniform, GLint x,
                                      GLint y, GLint z)
{
    ENTER_CONTEXT_SCOPE();

    if (!isFromCurrentProgram(uniform)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    glUniform3i(uniform->location(), x, y, z);
}

void WebGLRenderingContext::uniform4i(WebGLUniformLocation* uniform, GLint x,
                                      GLint y, GLint z, GLint w)
{
    ENTER_CONTEXT_SCOPE();

    if (!isFromCurrentProgram(uniform)) {
        setGLError(GL_INVALID_OPERATION);
        return;
    }

    glUniform4i(uniform->location(), x, y, z, w);
}

void WebGLRenderingContext::useProgram(WebGLProgram* program)
{
    ENTER_CONTEXT_SCOPE();

    if (!checkWebGLObject(program)) {
        return;
    }

    glUseProgram(program->glObject());
}

void WebGLRenderingContext::vertexAttrib1f(GLuint index, GLfloat x)
{
    ENTER_CONTEXT_SCOPE();

    glVertexAttrib1f(index, x);
}

void WebGLRenderingContext::vertexAttrib2f(GLuint index, GLfloat x, GLfloat y)
{
    ENTER_CONTEXT_SCOPE();

    glVertexAttrib2f(index, x, y);
}

void WebGLRenderingContext::vertexAttrib3f(GLuint index, GLfloat x, GLfloat y,
                                           GLfloat z)
{
    ENTER_CONTEXT_SCOPE();

    glVertexAttrib3f(index, x, y, z);
}

void WebGLRenderingContext::vertexAttrib4f(GLuint index, GLfloat x, GLfloat y,
                                           GLfloat z, GLfloat w)
{
    ENTER_CONTEXT_SCOPE();

    glVertexAttrib4f(index, x, y, z, w);
}

#define IMPLEMENT_VERTEX_ATTRIB_NFV(N)                                   \
    void WebGLRenderingContext::vertexAttrib##N##fv(GLuint index,        \
                                                    Float32List variant) \
    {                                                                    \
        ENTER_CONTEXT_SCOPE();                                           \
        std::vector<float> vector;                                       \
        copyFloat32List(scriptBindingInstance(), variant, vector);       \
        if (!vector.empty()) {                                           \
            glVertexAttrib##N##fv(index, vector.data());                 \
        }                                                                \
    }

IMPLEMENT_VERTEX_ATTRIB_NFV(1)
IMPLEMENT_VERTEX_ATTRIB_NFV(2)
IMPLEMENT_VERTEX_ATTRIB_NFV(3)
IMPLEMENT_VERTEX_ATTRIB_NFV(4)

#undef IMPLEMENT_VERTEX_ATTRIB_NFV

void WebGLRenderingContext::vertexAttribPointer(GLuint index, GLint size,
                                                GLenum type,
                                                GLboolean normalized,
                                                GLsizei stride, GLintptr offset)
{
    ENTER_CONTEXT_SCOPE();

    const uint8_t* zeroOffset = nullptr;

    if (stride > kMaximumSupportedStride) {
        // In WebGL, the maximum supported stride is 255; see Vertex Attribute
        // Data Stride.
        setGLError(GL_INVALID_VALUE);
        return;
    }

    if (offset < 0) {
        // see Buffer Offset and Stride Requirements. If offset is negative, an
        // INVALID_VALUE error will be generated.
        setGLError(GL_INVALID_VALUE);
        return;
    }

    glVertexAttribPointer(index, size, type, normalized, stride,
                          zeroOffset + offset);

    /*
        The following errors are handled in GLES3.
        (https://docs.gl/es3/glVertexAttribPointer)
        We don't do any additional validation for them:

        - GL_INVALID_VALUE if size is not 1, 2, 3 or 4.
        - GL_INVALID_ENUM if type is not an accepted value.
        - GL_INVALID_VALUE if stride is negative.
        - GL_INVALID_VALUE if index is greater than or equal to
            GL_MAX_VERTEX_ATTRIBS.
        - GL_INVALID_OPERATION if type is GL_INT_2_10_10_10_REV or
            GL_UNSIGNED_INT_2_10_10_10_REV and size is not 4.
        - GL_INVALID_OPERATION a non-zero vertex array object is bound, zero is
            bound to the GL_ARRAY_BUFFER buffer object binding point and the
            pointer argument is not NULL.
    */
}

void WebGLRenderingContext::viewport(uint32_t x, uint32_t y, uint32_t width,
                                     uint32_t height)
{
    ENTER_CONTEXT_SCOPE();

    glViewport(x, y, width, height);
}

// WebGLRenderingContextOverloads

void WebGLRenderingContext::bufferData(GLenum target, GLsizeiptr size,
                                       GLenum usage)
{
    STARFISH_UNIMPLEMENTED();
}

void WebGLRenderingContext::bufferData(GLenum target,
                                       Nullable<AllowSharedBufferSource> data,
                                       GLenum usage)
{
    ENTER_CONTEXT_SCOPE();

    if (data.hasValue()) {
        if (data.value().isArrayBufferValue()) {
            ScriptArrayBuffer buffer = data.value().getArrayBufferValue();
            glBufferData(target, buffer->byteLength(), buffer->rawBuffer(),
                         usage);
        } else if (data.value().isArrayBufferViewValue()) {
            ScriptArrayBufferView view = data.value().getArrayBufferViewValue();
            glBufferData(target, view->byteLength(), view->rawBuffer(), usage);
        } else if (data.value().isSharedArrayBufferValue()) {
            ScriptSharedArrayBuffer buffer =
                data.value().getSharedArrayBufferValue();
            glBufferData(target, buffer->byteLength(), buffer->rawBuffer(),
                         usage);
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
    } else {
        //  If data is null, then the contents of the buffer object’s data store
        //  are undefined.
        glBufferData(target, 0, nullptr, usage);
    }
}

class TexImageHelper final {
public:
    // NOTE: Better to use common utilities for image manipulation. Canvas
    // is not possible due to its WebView dependency.
    struct ImageData {
        ImageData() = default;
        size_t width = 0;
        size_t height = 0;
        size_t stride = 0;
        unsigned char* data = nullptr;
    };

    TexImageHelper(size_t width, size_t height, size_t stride, void* data)
    {
        STARFISH_ASSERT(data != nullptr);

        m_sourceImage.width = width;
        m_sourceImage.height = height;
        m_sourceImage.stride = stride;
        m_sourceImage.data = static_cast<unsigned char*>(data);
    }

    TexImageHelper(NativeImageData* imageData)
    {
        STARFISH_ASSERT(imageData != nullptr);

        m_sourceImage.width = imageData->width();
        m_sourceImage.height = imageData->height();
        m_sourceImage.stride = imageData->stride();
        m_sourceImage.data = static_cast<unsigned char*>(imageData->data());
    }

    ~TexImageHelper()
    {
    }

    void draw(const bool needsFlipY, const bool needsPremultiplyAlpha,
              const bool colorConversion)
    {
        const size_t width = m_sourceImage.width;
        const size_t height = m_sourceImage.height;
        const size_t stride = m_sourceImage.stride;
        const unsigned char* image = m_sourceImage.data;

        size_t offset = 0, newOffset = 0, srcOffset = 0, destOffset = 0;

        m_data.resize(height * stride);

        bool needsColorConversion = true;

#if defined(PORT_PIXEL_ORDER_RGBA)
        needsColorConversion = false;
#elif defined(PORT_PIXEL_ORDER_BGRA)
        needsColorConversion = colorConversion;
#else
        STARFISH_ASSERT_NOT_REACHED();
#endif
        std::vector<uint8_t> order;

        if (needsColorConversion) {
            order = { 2, 1, 0, 3 };
        } else {
            order = { 0, 1, 2, 3 };
        }

        for (size_t row = 0; row < height; row++) {
            // Calculate the memory offset for the current row
            newOffset = offset = row * stride;

            if (needsFlipY || needsPremultiplyAlpha || needsColorConversion) {
                if (needsFlipY) {
                    newOffset = (height - row - 1) * stride;
                }

                for (size_t column = 0; column < width; column++) {
                    // Calculate the memory offset for the current pixel
                    srcOffset = offset + column * 4;
                    destOffset = newOffset + column * 4;

                    if (needsPremultiplyAlpha) {
                        float alpha = image[srcOffset + order[3]] / 255.f;
                        m_data[destOffset + 0] =
                            multiplyAlpha(image[srcOffset + order[0]], alpha);
                        m_data[destOffset + 1] =
                            multiplyAlpha(image[srcOffset + order[1]], alpha);
                        m_data[destOffset + 2] =
                            multiplyAlpha(image[srcOffset + order[2]], alpha);
                        m_data[destOffset + 3] = image[srcOffset + order[3]];
                    } else {
                        m_data[destOffset + 0] = image[srcOffset + order[0]];
                        m_data[destOffset + 1] = image[srcOffset + order[1]];
                        m_data[destOffset + 2] = image[srcOffset + order[2]];
                        m_data[destOffset + 3] = image[srcOffset + order[3]];
                    }
                }
            } else {
                std::memcpy(&m_data[newOffset], &image[offset], stride);
            }
        }
    }

    const void* data()
    {
        return m_data.empty() ? m_sourceImage.data : m_data.data();
    }

private:
    unsigned char multiplyAlpha(unsigned char color, float alpha)
    {
        return ((color / 255.f) * alpha) * 255;
    }

    ImageData m_sourceImage;
    std::vector<unsigned char> m_data;
};

void WebGLRenderingContext::texImage2D(GLenum target, GLint level,
                                       GLint internalFormat, GLsizei width,
                                       GLsizei height, GLint border,
                                       GLenum format, GLenum type,
                                       Nullable<ScriptArrayBufferView> pixels)
{
    ENTER_CONTEXT_SCOPE();

    if (m_boundTextures.find(target) == m_boundTextures.end() &&
        !isBoundCubeMapTexture(target)) {
        setGLError(
            GL_INVALID_OPERATION,
            StringUtils::formatString("target (0x%04X) is not bound.", target)
                .c_str());
        return;
    }

    if (pixels.hasValue()) {
        ArrayBufferViewRef* pixelsView = pixels.getValue();

        if (type == GL_UNSIGNED_BYTE &&
            (!pixelsView->isUint8ArrayObject() &&
             !pixelsView->isUint8ClampedArrayObject())) {
            // If it is UNSIGNED_BYTE, a Uint8Array or Uint8ClampedArray
            // must be supplied.
            setGLError(GL_INVALID_OPERATION);
            return;
        } else if ((type == GL_UNSIGNED_SHORT_5_6_5 ||
                    type == GL_UNSIGNED_SHORT_4_4_4_4 ||
                    type == GL_UNSIGNED_SHORT_5_5_5_1) &&
                   !pixelsView->isUint16ArrayObject()) {
            // If it is UNSIGNED_SHORT_5_6_5, UNSIGNED_SHORT_4_4_4_4, or
            // UNSIGNED_SHORT_5_5_5_1, a Uint16Array must be supplied.
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        // If pixels is non-null but its size is less than what is required by
        // the specified width, height, format, type, and pixel storage
        // parameters, generates an INVALID_OPERATION error.
        size_t bytesPerPixel = Pixel::getBytesPerPixel(format, type);
        size_t byteLengthOfPixels = width * height * bytesPerPixel;
        size_t byteLengthOfView = pixels->byteLength();

        TRACEF(WEBGL, "\n%s",
               StringUtils::createTableString(20, KV(hex(target)), KV(width),
                                              KV(height), KV(bytesPerPixel),
                                              KV(byteLengthOfPixels)));

        if (byteLengthOfView < byteLengthOfPixels) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        GLvoid* data = pixelsView->rawBuffer() + pixelsView->byteOffset();

        // Handle WebGL-specific pixel storage parameters that affect the
        // behavior of this function.
        TexImageHelper image(width, height, width * bytesPerPixel, data);
        image.draw(m_unpackFlipY, m_unpackPremultiplyAlpha,
                   m_unpackColorspaceConversion == kBROWSER_DEFAULT_WEBGL);

        glTexImage2D(target, level, internalFormat, width, height, 0, format,
                     type, image.data());
    } else {
        // TODO: If pixels is null, a buffer of sufficient size initialized to 0
        // is passed.
        STARFISH_UNIMPLEMENTED();
    }
}

void WebGLRenderingContext::readPixels(GLint x, GLint y, GLsizei width,
                                       GLsizei height, GLenum format,
                                       GLenum type,
                                       Nullable<ScriptArrayBufferView> pixels)
{
    ENTER_CONTEXT_SCOPE();

    if (pixels.hasValue()) {
        ArrayBufferViewRef* pixelsView = pixels.getValue();

        // 1. If the types don't match, an INVALID_OPERATION error is generated.
        if (type == GL_UNSIGNED_BYTE &&
            (!pixelsView->isUint8ArrayObject() &&
             !pixelsView->isUint8ClampedArrayObject())) {
            // If it is UNSIGNED_BYTE, a Uint8Array or Uint8ClampedArray
            // must be supplied.
            setGLError(GL_INVALID_OPERATION);
            return;
        } else if ((type == GL_UNSIGNED_SHORT_5_6_5 ||
                    type == GL_UNSIGNED_SHORT_4_4_4_4 ||
                    type == GL_UNSIGNED_SHORT_5_5_5_1) &&
                   !pixelsView->isUint16ArrayObject()) {
            // If it is UNSIGNED_SHORT_5_6_5, UNSIGNED_SHORT_4_4_4_4, or
            // UNSIGNED_SHORT_5_5_5_1, a Uint16Array must be supplied.
            setGLError(GL_INVALID_OPERATION);
            return;
        } else if ((type == GL_FLOAT) && !pixelsView->isFloat32ArrayObject()) {
            // if it is FLOAT, a Float32Array must be supplied.
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        // 2. Only two combinations of format and type are accepted. The first
        //    is format RGBA and type UNSIGNED_BYTE. The second is an
        //    implementation-chosen format.

        // As for webgl/1.0.3/conformance/reading/read-pixels-test.html:162,
        // GL_INVALID_ENUM needs to be set for the luminance.
        if ((format == GL_LUMINANCE || format == GL_LUMINANCE_ALPHA) &&
            type == GL_UNSIGNED_BYTE) {
            setGLError(GL_INVALID_ENUM);
            return;
        }

        // NOTE: Our implementation-chosen is a combination of RGBA and
        // UNSIGNED_BYTE. See kIMPLEMENTATION_COLOR_READ_TYPE and
        // kIMPLEMENTATION_COLOR_READ_FORMAT.
        if (format != GL_RGBA && type != GL_UNSIGNED_BYTE) {
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        size_t bytesPerPixel = Pixel::getBytesPerPixel(format, type);
        size_t byteLengthOfPixels = width * height * bytesPerPixel;
        size_t byteLengthOfView = pixels->byteLength();

        TRACEF(WEBGL, "\n%s",
               StringUtils::createTableString(
                   20, KV(width), KV(height), KV(bytesPerPixel),
                   KV(byteLengthOfView), KV(byteLengthOfPixels)));

        if (byteLengthOfView < byteLengthOfPixels) {
            // If pixels is non-null, but is not large enough to retrieve all of
            // the pixels in the specified rectangle taking into account pixel
            // store modes, an INVALID_OPERATION error is generated.
            setGLError(GL_INVALID_OPERATION);
            return;
        }

        /*
            For any pixel lying outside the frame buffer, the corresponding
            destination buffer range remains untouched; see Reading Pixels
            Outside the Framebuffer.

            TODO: 6.11 Reading Pixels Outside the Framebuffer

            For [Read Operations], reads from out-of-bounds pixels sub-areas do
            not touch their corresponding destination sub-areas.

            WebGL (behaves as if it) pre-initializes resources to zeros.
            Therefore for example copyTexImage2D will have zeros in sub-areas
            that correspond to out-of-bounds framebuffer reads.
        */

        if (x != 0 || y != 0) {
            STARFISH_UNIMPLEMENTED("Reading Pixels Outside the Framebuffer");
        }

        /*
            TODO: 6.28 Reading From a Missing Attachment

            In the OpenGL ES 2.0 API, it is not specified what happens when a
            command tries to source data from a missing attachment, such as
            ReadPixels of color data from a complete framebuffer that does not
            have a color attachment.

            In the WebGL API, any [Read Operations] that require data from an
            attachment that is missing will generate an INVALID_OPERATION error.
        */

        /*
            TODO: 6.29 Drawing To a Missing Attachment

            If this function attempts to read from a complete framebuffer with a
            missing color attachment, an INVALID_OPERATION error is generated
            per Reading from a Missing Attachment.

            In the OpenGL ES 2.0 API, it is not specified what happens when a
            command tries to draw to a missing attachment, such as clearing a
            draw buffer from a complete framebuffer that does not have a color
            attachment.

            In the WebGL API, any [Draw Operations] that draw to an attachment
            that is missing will draw nothing to that attachment. No error is
            generated.
        */

        GLvoid* data = pixelsView->rawBuffer() + pixelsView->byteOffset();

        glReadPixels(x, y, width, height, format, type, data);

    } else {
        // If pixels is null, an INVALID_VALUE error is generated.
        setGLError(GL_INVALID_VALUE);
    }
}

void WebGLRenderingContext::texImage2D(GLenum target, GLint level,
                                       GLint internalFormat, GLenum format,
                                       GLenum type, TexImageSource source)
{
    ENTER_CONTEXT_SCOPE();

    // TODO: handle DOM exception with referring to CanvasImageSource. If this
    // function is called with an HTMLImageElement or HTMLVideoElement whose
    // origin differs from the origin of the containing Document, or with an
    // HTMLCanvasElement, ImageBitmap or OffscreenCanvas whose bitmap's
    // origin-clean flag is set to false, a SECURITY_ERR exception must be
    // thrown. See Origin Restrictions.

    if (m_boundTextures.find(target) == m_boundTextures.end() &&
        !isBoundCubeMapTexture(target)) {
        setGLError(
            GL_INVALID_OPERATION,
            StringUtils::formatString("target (0x%04X) is not bound.", target)
                .c_str());
        return;
    }

    GLsizei width = 0;
    GLsizei height = 0;
    GLsizei stride = 0;
    NativeImageData* imageData = nullptr;

    if (source.isNoneValue()) {
        setGLError(GL_INVALID_VALUE);
        return;
    } else if (source.isImageBitmapValue()) {
        STARFISH_UNIMPLEMENTED("ImageBitmap");
    } else if (source.isImageDataValue()) {
        STARFISH_UNIMPLEMENTED("ImageData");
    } else if (source.isHTMLImageElementValue()) {
        HTMLImageElement* element = source.getHTMLImageElementValue();
        imageData = element->imageData();
        if (imageData->isSVGNativeImageData()) {
            STARFISH_UNIMPLEMENTED("SVGNativeImageData");
            width = element->width();
            height = element->height();
        } else {
            width = imageData->width();
            height = imageData->height();
        }
    } else if (source.isHTMLCanvasElementValue()) {
        // TODO: Consider using CanvasImageSourceUtils::toNativeImageData
        HTMLCanvasElement* element = source.getHTMLCanvasElementValue();
        CanvasRenderingContext* context = element->canvasRenderingContext();
        STARFISH_ASSERT(context != nullptr);
        auto context2d = static_cast<CanvasRenderingContext2DMixIn*>(context);
        context2d->flush();
        imageData = NativeImageData::attach(context2d->canvas());
        width = element->width();
        height = element->height();
    }
#ifdef STARFISH_ENABLE_MULTIMEDIA
    else if (source.isHTMLVideoElementValue()) {
        STARFISH_UNIMPLEMENTED("HTMLVideoElement");
    }
#endif
    else {
        STARFISH_ASSERT_NOT_REACHED();
    }

    STARFISH_ASSERT(imageData != nullptr);

    // Handle WebGL-specific pixel storage parameters that affect the behavior
    // of this function.
    TexImageHelper image(imageData);
    image.draw(m_unpackFlipY, m_unpackPremultiplyAlpha,
               m_unpackColorspaceConversion == kBROWSER_DEFAULT_WEBGL);

    // Uploads the given image data to the currently bound texture.
    glTexImage2D(target, level, internalFormat, width, height, 0, format, type,
                 image.data());
}

#define IMPLEMENT_UNIFORM_NXV(PostFix, SrcType, DestType)           \
    void WebGLRenderingContext::uniform##PostFix(                   \
        WebGLUniformLocation* location, SrcType value)              \
    {                                                               \
        ENTER_CONTEXT_SCOPE();                                      \
        if (location == nullptr) {                                  \
            return;                                                 \
        }                                                           \
        if (!isFromCurrentProgram(location)) {                      \
            setGLError(GL_INVALID_OPERATION);                       \
            return;                                                 \
        }                                                           \
        std::vector<DestType> vector;                               \
        copy##SrcType(scriptBindingInstance(), value, vector);      \
        if (vector.empty()) {                                       \
            glUniform##PostFix(location->location(), vector.size(), \
                               vector.data());                      \
        }                                                           \
    }

IMPLEMENT_UNIFORM_NXV(1fv, Float32List, float)
IMPLEMENT_UNIFORM_NXV(2fv, Float32List, float)
IMPLEMENT_UNIFORM_NXV(3fv, Float32List, float)
IMPLEMENT_UNIFORM_NXV(4fv, Float32List, float)
IMPLEMENT_UNIFORM_NXV(1iv, Int32List, int32_t)
IMPLEMENT_UNIFORM_NXV(2iv, Int32List, int32_t)
IMPLEMENT_UNIFORM_NXV(3iv, Int32List, int32_t)
IMPLEMENT_UNIFORM_NXV(4iv, Int32List, int32_t)

#undef IMPLEMENT_UNIFORM_NXV

#define IMPLEMENT_UNIFORM_MATRIX_NFV(N)                                 \
    void WebGLRenderingContext::uniformMatrix##N##fv(                   \
        WebGLUniformLocation* location, GLboolean transpose,            \
        Float32List value)                                              \
    {                                                                   \
        ENTER_CONTEXT_SCOPE();                                          \
        if (location == nullptr) {                                      \
            return;                                                     \
        }                                                               \
        if (!isFromCurrentProgram(location)) {                          \
            setGLError(GL_INVALID_OPERATION);                           \
            return;                                                     \
        }                                                               \
        std::vector<float> vector;                                      \
        copyFloat32List(scriptBindingInstance(), value, vector);        \
        if (vector.empty()) {                                           \
            glUniformMatrix##N##fv(location->location(), vector.size(), \
                                   transpose, vector.data());           \
        }                                                               \
    }

IMPLEMENT_UNIFORM_MATRIX_NFV(2)
IMPLEMENT_UNIFORM_MATRIX_NFV(3)
IMPLEMENT_UNIFORM_MATRIX_NFV(4)

#undef IMPLEMENT_UNIFORM_MATRIX_NFV

bool WebGLRenderingContext::executeInContextScope(
    std::function<void()> callback)
{
    ENTER_CONTEXT_SCOPE(false);
    callback();
    return true;
}

WebGLRenderingContextState* WebGLRenderingContext::getState()
{
    return &m_state;
}

bool WebGLRenderingContext::checkWebGLObject(WebGLObject* object)
{
    if (object->context() != this) {
        // If object was generated by a different WebGLRenderingContext than
        // this one, generates an INVALID_OPERATION error.
        setGLError(GL_INVALID_OPERATION);
        return false;
    }
    return true;
}

bool WebGLRenderingContext::checkAttribOrUniformName(String* name)
{
    // If the passed name is longer than the restriction defined in Maximum
    // Uniform and Attribute Location Lengths, generates an INVALID_VALUE error
    // and returns -1
    if (name->length() > kMaximumUniformAndAttributeLocationLengths) {
        setGLError(GL_INVALID_VALUE);
        return false;
    }

    // Returns -1 if name starts with one of the reserved WebGL prefixes per
    // GLSL Constructs.
    if (name->startsWith("webgl_") || name->startsWith("_webgl_")) {
        return false;
    }

    // TODO?: See Characters Outside the GLSL Source Character Set for
    // additional validation performed by WebGL implementations. WebGL
    // implementations generally must ensure that the shader source sent to a
    // GLSL driver only contains ASCII for safety.
    return true;
}

bool WebGLRenderingContext::isBoundCubeMapTexture(GLenum target)
{
    if (target > GL_TEXTURE_BINDING_CUBE_MAP &&
        target < GL_MAX_CUBE_MAP_TEXTURE_SIZE) {
        if (m_boundTextures.find(GL_TEXTURE_CUBE_MAP) !=
            m_boundTextures.end()) {
            return true;
        }
    }
    return false;
}

bool WebGLRenderingContext::isFromCurrentProgram(WebGLUniformLocation* location)
{
    // Consider caching this program id when useProgram is called.
    GLint program = getCurrentProgram();
    if (program == 0) {
        // no program object is active.
        return false;
    }

    if (location->program()->context() != this ||
        location->program()->glObject() != static_cast<GLuint>(program)) {
        // If program were generated by a different WebGLRenderingContext than
        // this one, and location were generated by a different program.
        return false;
    }
    return true;
}

} // namespace Starfish

#endif
