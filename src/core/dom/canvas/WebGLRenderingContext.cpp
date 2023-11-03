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
#include "binding/generated/ArrayBufferOrSharedArrayBufferOrArrayBufferViewUnion.h"
#include <EscargotPublic.h>

#define S1(x) #x
#define S2(x) S1(x)
#define LOCATION " (" __FILE__ ":" S2(__LINE__) ")"

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

#define ENTER_CONTEXT_SCOPE(bailoutValue, ...)                              \
    WebGLContextScope contextScope(m_context, m_framebufferTexture->fbo()); \
    if (contextScope.hasError()) {                                          \
        return bailoutValue;                                                \
    }

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
        m_GLErrors.insert(code);
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
    m_ownerHTMLCanvasElement->setNeedsComposite();
}

void WebGLRenderingContext::clearColor(float red, float green, float blue,
                                       float alpha)
{
    ENTER_CONTEXT_SCOPE();

    glClearColor(red, green, blue, alpha);
    m_ownerHTMLCanvasElement->setNeedsComposite();
}

void WebGLRenderingContext::viewport(uint32_t x, uint32_t y, uint32_t width,
                                     uint32_t height)
{
    ENTER_CONTEXT_SCOPE();

    glViewport(x, y, width, height);
    m_ownerHTMLCanvasElement->setNeedsComposite();
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

WebGLBuffer* WebGLRenderingContext::createBuffer()
{
    ENTER_CONTEXT_SCOPE(nullptr);

    GLuint buffer = 0;
    glGenBuffers(1, &buffer);
    return new WebGLBuffer(scriptBindingInstance(), buffer);
}

WebGLShader* WebGLRenderingContext::createShader(unsigned long type)
{
    ENTER_CONTEXT_SCOPE(nullptr);

    return new WebGLShader(scriptBindingInstance(), glCreateShader(type));
}

void WebGLRenderingContext::shaderSource(WebGLShader* shader, String* source)
{
    ENTER_CONTEXT_SCOPE();

    std::string str = source->toUTF8NonGCString();
    const char* sourceArray[1] = { str.c_str() };

    glShaderSource(shader->glObject(), 1, sourceArray, nullptr);
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

// WebGLRenderingContextOverloads

void WebGLRenderingContext::bufferData(GLenum target, GLsizeiptr size,
                                       GLenum usage)
{
    STARFISH_UNIMPLEMENTED();
}

void WebGLRenderingContext::bufferData(GLenum target,
                                       Nullable<AllowSharedBufferSource*> data,
                                       GLenum usage)
{
    ENTER_CONTEXT_SCOPE();

    if (data.hasValue()) {
        if (data.value()->isArrayBufferValue()) {
            ScriptArrayBuffer buffer = data.value()->getArrayBufferValue();
            glBufferData(target, buffer->byteLength(), buffer->rawBuffer(),
                         usage);
        } else if (data.value()->isArrayBufferViewValue()) {
            ScriptArrayBufferView view =
                data.value()->getArrayBufferViewValue();
            glBufferData(target, view->byteLength(), view->rawBuffer(), usage);
        } else if (data.value()->isSharedArrayBufferValue()) {
            ScriptSharedArrayBuffer buffer =
                data.value()->getSharedArrayBufferValue();
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

} // namespace Starfish

#endif
