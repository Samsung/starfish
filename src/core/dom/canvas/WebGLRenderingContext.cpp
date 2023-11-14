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
#include "core/dom/ExecutionContext.h"
#include "platform/canvas/webgl/GLContext.h"
#include "platform/canvas/webgl/XGLPlatform.h"
#include "core/dom/canvas/WebGLBuffer.h"
#include "core/dom/canvas/WebGLShader.h"
#include "core/dom/canvas/WebGLProgram.h"
#include "core/dom/canvas/WebGLUniformLocation.h"
#include "core/modules/canvas/Canvas.h"
#include "binding/generated/ArrayBufferOrSharedArrayBufferOrArrayBufferViewUnion.h"
#include <EscargotPublic.h>

#define S1(x) #x
#define S2(x) S1(x)
#define LOCATION " (" __FILE__ ":" S2(__LINE__) ")"
#define kMaximumUniformAndAttributeLocationLengths 256
#define kMaximumSupportedStride 255

namespace Starfish {

WebGLRenderingContext::WebGLRenderingContext(HTMLCanvasElement* canvasElement)
    : WebGLRenderingContextBaseMixIn(canvasElement)
{
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

    // Initial clear color is black per the spec, but some browsers may use
    // white instead, which is browser-specific.
    clearColor(1.f, 1.f, 1.f, 1.f);
    clear(GL_COLOR_BUFFER_BIT);
}

#define ENTER_CONTEXT_SCOPE(bailoutValue, ...)                              \
    WebGLContextScope contextScope(m_context, m_framebufferTexture->fbo()); \
    if (contextScope.hasError()) {                                          \
        return bailoutValue;                                                \
    }                                                                       \
    m_ownerHTMLCanvasElement->setNeedsComposite();

// TODO: Use CanvasElement::setNeedsComposite only when absolutely necessary.

/*
Note: Use hasGLError() to internally check for GL errors. When `glGetError` is
called, the code returned is cleared inside it. If we use `glGetError` directly,
users would not be able to get error code properly. So, we first store the code
from `glGetError`, and then use it. The error code stored will be cleared when
users call gl.getError().
*/
bool WebGLRenderingContext::hasGLError(const char* message)
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

void WebGLRenderingContext::setGLError(GLenum code)
{
    m_GLErrors.insert(code);
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

void WebGLRenderingContext::viewport(uint32_t x, uint32_t y, uint32_t width,
                                     uint32_t height)
{
    ENTER_CONTEXT_SCOPE();

    glViewport(x, y, width, height);
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

void WebGLRenderingContext::bindBuffer(GLenum target,
                                       Nullable<WebGLBuffer*> buffer)
{
    ENTER_CONTEXT_SCOPE();

    if (buffer.hasValue()) {
        WebGLBuffer* value = buffer.value();

        if (value->target() != GL_NONE) {
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

void WebGLRenderingContext::drawArrays(GLenum mode, GLint first, GLsizei count)
{
    ENTER_CONTEXT_SCOPE();

    if (first < 0) {
        // If first is negative, an INVALID_VALUE error will be generated.
        setGLError(GL_INVALID_VALUE);
        return;
    }

    // TODO:If the CURRENT_PROGRAM is null, an INVALID_OPERATION error will be
    // generated.

    glDrawArrays(mode, first, count);
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

void WebGLRenderingContext::shaderSource(WebGLShader* shader, String* source)
{
    ENTER_CONTEXT_SCOPE();

    std::string str = source->toUTF8NonGCString();
    const char* sourceArray[1] = { str.c_str() };

    glShaderSource(shader->glObject(), 1, sourceArray, nullptr);
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
    glGetProgramiv(program->glObject(), GL_LINK_STATUS, &params);

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

String* WebGLRenderingContext::getShaderSource(WebGLShader* shader)
{
    ENTER_CONTEXT_SCOPE(nullptr);

    GLsizei length = 0, bufferSize = 0;
    glGetShaderiv(shader->glObject(), GL_SHADER_SOURCE_LENGTH, &bufferSize);

    std::string buffer;
    buffer.reserve(bufferSize);
    glGetShaderSource(shader->glObject(), bufferSize, &length, &buffer[0]);
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

    return new WebGLUniformLocation(scriptBindingInstance(), this, location);
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

void WebGLRenderingContext::uniform2f(WebGLUniformLocation* location, GLfloat x,
                                      GLfloat y)
{
    ENTER_CONTEXT_SCOPE();

    // NOTE: See https://docs.gl/es3/glUniform for details of verification and
    // error handling in GLES3.

    glUniform2f(location->glObject(), x, y);
}

void WebGLRenderingContext::useProgram(WebGLProgram* program)
{
    ENTER_CONTEXT_SCOPE();

    if (!checkWebGLObject(program)) {
        return;
    }

    glUseProgram(program->glObject());
}

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

} // namespace Starfish

#endif
