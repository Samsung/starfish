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
#include "core/modules/canvas/image/NativeImageData.h"
#include "platform/canvas/webgl/GLContext.h"
#include "platform/canvas/webgl/XGLPlatform.h"
#include "core/dom/canvas/WebGLBuffer.h"
#include "core/dom/canvas/WebGLShader.h"
#include "core/dom/canvas/WebGLProgram.h"
#include "core/dom/canvas/WebGLTexture.h"
#include "core/dom/canvas/WebGLUniformLocation.h"
#include "core/modules/canvas/Canvas.h"
#include "core/dom/canvas/CanvasRenderingContext.h"
#include "core/dom/canvas/CanvasRenderingContext2DMixIn.h"
#include "binding/generated/ArrayBufferOrSharedArrayBufferOrArrayBufferViewUnion.h"
#include "binding/generated/ImageBitmapOrImageDataOrHTMLImageElementOrHTMLCanvasElementOrHTMLVideoElementUnion.h"
#include <EscargotPublic.h>
#include <sstream>
#include <iomanip>

#define S1(x) #x
#define S2(x) S1(x)
#define LOCATION " (" __FILE__ ":" S2(__LINE__) ")"
#define kMaximumUniformAndAttributeLocationLengths 256
#define kMaximumSupportedStride 255

/* WebGL-specific enums */
static const GLenum kUNPACK_FLIP_Y_WEBGL = 0x9240;
static const GLenum kUNPACK_PREMULTIPLY_ALPHA_WEBGL = 0x9241;
static const GLenum kCONTEXT_LOST_WEBGL = 0x9242;
static const GLenum kUNPACK_COLORSPACE_CONVERSION_WEBGL = 0x9243;
static const GLenum kBROWSER_DEFAULT_WEBGL = 0x9244;

static std::string getHexString(GLenum pname)
{
    std::stringstream ss;
    ss << "0x" << std::hex << std::setw(4) << std::setfill('0')
       << std::uppercase << pname;
    return ss.str();
}

namespace Starfish {

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
            STARFISH_LOG_WARN("GL error detected."); \
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

        glBindTexture(target, texture->glObject());
        m_boundTextures[target] = texture->glObject();
    } else {
        glBindTexture(target, 0);
        m_boundTextures.erase(target);
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

WebGLTexture* WebGLRenderingContext::createTexture()
{
    ENTER_CONTEXT_SCOPE(nullptr);

    GLuint textureId = 0;
    glGenTextures(1, &textureId);
    return new WebGLTexture(scriptBindingInstance(), this, textureId);
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

static GLint getInteger(GLenum pname)
{
    GLint value[1]{};
    glGetIntegerv(pname, value);
    return value[0];
}

ScriptValue WebGLRenderingContext::getParameter(GLenum pname)
{
    ENTER_CONTEXT_SCOPE(scriptNull());

    switch (pname) {
    case GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:
        return ValueRef::create(getInteger(pname));
    default:
        STARFISH_UNIMPLEMENTED("pname: %s", getHexString(pname).c_str());
        return scriptNull();
    }
    return scriptNull();
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

void WebGLRenderingContext::uniform1f(WebGLUniformLocation* location, GLfloat x)
{
    ENTER_CONTEXT_SCOPE();

    glUniform1f(location->glObject(), x);
}

void WebGLRenderingContext::uniform2f(WebGLUniformLocation* location, GLfloat x,
                                      GLfloat y)
{
    ENTER_CONTEXT_SCOPE();

    // NOTE: See https://docs.gl/es3/glUniform for details of verification and
    // error handling in GLES3.

    glUniform2f(location->glObject(), x, y);
}

void WebGLRenderingContext::uniform3f(WebGLUniformLocation* location, GLfloat x,
                                      GLfloat y, GLfloat z)
{
    ENTER_CONTEXT_SCOPE();

    glUniform3f(location->glObject(), x, y, z);
}

void WebGLRenderingContext::uniform4f(WebGLUniformLocation* location, GLfloat x,
                                      GLfloat y, GLfloat z, GLfloat w)
{
    ENTER_CONTEXT_SCOPE();

    glUniform4f(location->glObject(), x, y, z, w);
}

void WebGLRenderingContext::uniform1i(WebGLUniformLocation* location, GLint x)
{
    ENTER_CONTEXT_SCOPE();

    glUniform1i(location->glObject(), x);
}

void WebGLRenderingContext::uniform2i(WebGLUniformLocation* location, GLint x,
                                      GLint y)
{
    ENTER_CONTEXT_SCOPE();

    glUniform2i(location->glObject(), x, y);
}

void WebGLRenderingContext::uniform3i(WebGLUniformLocation* location, GLint x,
                                      GLint y, GLint z)
{
    ENTER_CONTEXT_SCOPE();

    glUniform3i(location->glObject(), x, y, z);
}

void WebGLRenderingContext::uniform4i(WebGLUniformLocation* location, GLint x,
                                      GLint y, GLint z, GLint w)
{
    ENTER_CONTEXT_SCOPE();

    glUniform4i(location->glObject(), x, y, z, w);
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

void WebGLRenderingContext::texImage2D(GLenum target, GLint level,
                                       GLint internalFormat, GLsizei width,
                                       GLsizei height, GLint border,
                                       GLenum format, GLenum type,
                                       Nullable<ScriptArrayBufferView> pixels)
{
    ENTER_CONTEXT_SCOPE();

    STARFISH_UNIMPLEMENTED();
}

class TexImageHelper final {
public:
    // NOTE: Better to use common utilities for image manipulation. Canvas
    // is not possible due to its WebView dependency.

    TexImageHelper(NativeImageData* imageData)
    {
        STARFISH_ASSERT(imageData != nullptr);
        m_sourceImage = imageData;
    }

    ~TexImageHelper()
    {
    }

    void draw(const bool needsFlipY, const bool needsPremultiplyAlpha,
              const bool colorConversion)
    {
        const size_t height = m_sourceImage->height();
        const size_t width = m_sourceImage->width();
        const size_t stride = m_sourceImage->stride();
        const auto image = static_cast<unsigned char*>(m_sourceImage->data());

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
        return m_data.empty() ? m_sourceImage->data() : m_data.data();
    }

private:
    unsigned char multiplyAlpha(unsigned char color, float alpha)
    {
        return ((color / 255.f) * alpha) * 255;
    }

    NativeImageData* m_sourceImage = nullptr;
    std::vector<unsigned char> m_data;
};

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

    if (m_boundTextures.find(target) == m_boundTextures.end()) {
        setGLError(GL_INVALID_OPERATION);
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

    TexImageHelper image(imageData);
    image.draw(m_unpackFlipY, m_unpackPremultiplyAlpha,
               m_unpackColorspaceConversion == kBROWSER_DEFAULT_WEBGL);

    // Uploads the given image data to the currently bound texture.
    glTexImage2D(target, level, internalFormat, width, height, 0,
                 internalFormat, type, image.data());
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
