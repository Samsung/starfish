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
#include "core/dom/canvas/WebGLShader.h"

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

GLenum WebGLRenderingContext::getError()
{
    WebGLContextScope contextScope(m_context, m_framebufferTexture->fbo());
    return glGetError();
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

WebGLShader* WebGLRenderingContext::createShader(unsigned long type)
{
    ENTER_CONTEXT_SCOPE(nullptr);

    GLuint object = glCreateShader(type);

    return new WebGLShader(scriptBindingInstance(), object);
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

} // namespace Starfish

#endif
