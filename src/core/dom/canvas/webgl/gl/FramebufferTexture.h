/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_WEBGL)

#ifndef __StarfishGLUtil__
#define __StarfishGLUtil__

#include "platform/canvas/gl/GLTypes.h"
#include "core/dom/canvas/webgl/gl/SurfaceCreationScope.h"
#include <memory>

namespace Starfish {

class Renderer;
class GL;

struct FrameBufferAttributes {
    bool antialias = true;
    bool depth = true;
    bool stencil = false;
};

/**
 * @brief Create a texture for offscreen rendering with a depth buffer
 */
class FramebufferTexture : public TextureCreationDelegate {
public:
    FramebufferTexture(Renderer* renderer);
    ~FramebufferTexture();

    bool create(unsigned bufferWidth, unsigned bufferHeight,
                GLuint& outTextureId) override;

    bool destory() override;

    Type type() override
    {
        return Type::FrameBuffer;
    }

    inline GLuint fbo()
    {
        return m_fbo;
    }

    void setAttributes(FrameBufferAttributes attributes)
    {
        m_attributes = attributes;
    }

private:
    GLuint m_fbo{ 0 };
    GLuint m_textureId{ 0 };
    GLuint m_rboDepth{ 0 };
    GLuint m_rboOrTextureIdForDepthStencil{ 0 };
    FrameBufferAttributes m_attributes;
    Renderer* m_renderer{ nullptr };
    GL* m_gl{ nullptr };
};

} // namespace Starfish

#endif

#endif // #if defined(STARFISH_ENABLE_WEBGL)
