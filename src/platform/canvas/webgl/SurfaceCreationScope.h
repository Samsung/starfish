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

#ifndef __StarfishSurfaceCreationScope__
#define __StarfishSurfaceCreationScope__

#include <memory>
#include "platform/canvas/webgl/GLES.h" // For GLuint

namespace Starfish {

class TextureCreationDelegate {
public:
    enum class Type {
        FrameBuffer,
    };
    virtual ~TextureCreationDelegate() = default;

    virtual bool create(unsigned bufferWidth, unsigned bufferHeight,
                        GLuint& outTextureId) = 0;
    virtual Type type() = 0;
};

class SurfaceCreationScope {
public:
    explicit SurfaceCreationScope(
        std::shared_ptr<TextureCreationDelegate> delegate);

    ~SurfaceCreationScope();
    SurfaceCreationScope(const SurfaceCreationScope& other) = delete;
    SurfaceCreationScope(SurfaceCreationScope&& other) = delete;
    SurfaceCreationScope& operator=(const SurfaceCreationScope& other) = delete;
    void* operator new(size_t size) = delete;
    void* operator new[](size_t size) = delete;
    void operator delete(void* p) = delete;

private:
    static std::shared_ptr<TextureCreationDelegate> delegate()
    {
        return m_delegate;
    }
    static inline bool hasDelegate()
    {
        return m_delegate != nullptr;
    }
    static std::shared_ptr<TextureCreationDelegate> m_delegate;
    friend class CanvasSurfaceGL;
};

} // namespace Starfish

#endif
