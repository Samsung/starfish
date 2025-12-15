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
