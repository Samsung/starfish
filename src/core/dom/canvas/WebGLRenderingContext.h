/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishWebGLRenderingContext__
#define __StarfishWebGLRenderingContext__

#if defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#include "core/dom/canvas/WebGLRenderingContextBaseMixIn.h"
#include "platform/canvas/webgl/GLESTypes.h"
#include <unordered_set>

namespace Starfish {

class WebGLBuffer;
class WebGLShader;
class ArrayBufferOrSharedArrayBufferOrArrayBufferView;

using AllowSharedBufferSource = ArrayBufferOrSharedArrayBufferOrArrayBufferView;

class WebGLRenderingContext : public WebGLRenderingContextBaseMixIn {
public:
    WebGLRenderingContext(HTMLCanvasElement* canvasElement);
    virtual ~WebGLRenderingContext();

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(WebGLRenderingContext);

    // Implement WebGLRenderingContextBase
    void bindBuffer(GLenum target, Nullable<WebGLBuffer*> buffer);

    void clear(uint32_t mask);
    void clearColor(float red, float green, float blue, float alpha);
    WebGLBuffer* createBuffer();
    WebGLShader* createShader(unsigned long type);

    GLenum getError();
    String* getShaderSource(WebGLShader* shader);

    void shaderSource(WebGLShader* shader, String* source);
    void viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

    // Implement WebGLRenderingContextOverloads
    void bufferData(GLenum target, GLsizeiptr size, GLenum usage);
    void bufferData(GLenum target, Nullable<AllowSharedBufferSource*> data,
                    GLenum usage);

private:
    bool hasGLError(const char* message = "");
    void setGLError(GLenum code);
    void updateGLError();
    std::unordered_set<GLenum> m_GLErrors;
};
} // namespace Starfish

#endif
#endif
