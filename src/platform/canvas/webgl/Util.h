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

#ifndef __StarfishUtil__
#define __StarfishUtil__

#include "platform/canvas/webgl/GLES.h"
#include "platform/canvas/webgl/SurfaceCreationScope.h"
#include <memory>

namespace Starfish {

/**
 * @brief Create a texture for offscreen rendering with a depth buffer
 */
class FramebufferTexture : public TextureCreationDelegate {
public:
    ~FramebufferTexture();

    bool create(unsigned bufferWidth, unsigned bufferHeight,
                GLuint& outTextureId) override;

    Type type() override
    {
        return Type::FrameBuffer;
    }

    inline GLuint fbo()
    {
        return m_fbo;
    }

private:
    GLuint m_fbo{ 0 };
    GLuint m_textureId{ 0 };
    GLuint m_rbo{ 0 };
};

class FBOScope {
public:
    explicit FBOScope(GLuint fbo);
    ~FBOScope();
    FBOScope(const FBOScope& other) = delete;
    FBOScope& operator=(const FBOScope& other) = delete;
    FBOScope(FBOScope&& other) = delete;
    void* operator new(size_t size) = delete;
    void* operator new[](size_t size) = delete;
    void operator delete(void* p) = delete;
};

} // namespace Starfish

#endif

#endif // #if defined(STARFISH_ENABLE_WEBGL)
