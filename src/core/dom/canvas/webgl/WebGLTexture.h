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

#ifndef __StarfishWebGLTexture__
#define __StarfishWebGLTexture__

#if defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#include "core/dom/canvas/webgl/WebGLObject.h"
#include "platform/canvas/gl/IncludeGL.h"
#include <algorithm>

namespace Starfish {

struct TextureImageLevelInfo {
    GLenum target = 0;
    GLint level = 0;
    GLenum internalFormat = 0;
    GLsizei width = 0;
    GLsizei height = 0;
    GLsizei depth = 0;
    // The type the level was defined with, 0 when not known (WebGL 1
    // texSubImage2D must match it). Every type token fits in 16 bits, which
    // keeps the struct at its size.
    uint16_t type = 0;
    bool isDefined = false;

    TextureImageLevelInfo() = default;
    TextureImageLevelInfo(GLenum target, GLint level, GLenum internalFormat,
                          GLsizei width, GLsizei height, GLsizei depth,
                          uint16_t type, bool isDefined)
        : target(target)
        , level(level)
        , internalFormat(internalFormat)
        , width(width)
        , height(height)
        , depth(depth)
        , type(type)
        , isDefined(isDefined)
    {
    }
};

static_assert(sizeof(TextureImageLevelInfo) <= 7 * sizeof(GLenum),
              "the type takes the padding after depth");

class WebGLTexture : public WebGLObject {
public:
    WebGLTexture(ScriptBindingInstance* instance,
                 WebGLRenderingContext* context, GLuint object);
    void init(ScriptBindingInstance* instance, void* domObjectPointer) override;
    bool isWebGLTexture() const override;

    GLenum target() const
    {
        return m_target;
    }
    void setTarget(GLenum target)
    {
        if (m_target == 0) {
            m_target = target;
            invalidateFramebufferStatus();
        }
    }

    bool isImmutable() const
    {
        return m_isImmutable;
    }
    void setImmutable(bool immutable)
    {
        m_isImmutable = immutable;
        invalidateFramebufferStatus();
    }

    GLsizei width() const
    {
        return m_width;
    }
    GLsizei height() const
    {
        return m_height;
    }
    void setSize(GLsizei w, GLsizei h)
    {
        m_width = w;
        m_height = h;
        invalidateFramebufferStatus();
    }

    // TEXTURE_MIN_FILTER as last set, and whether MAG_FILTER is NEAREST and
    // WRAP_S and WRAP_T are CLAMP_TO_EDGE; other names are ignored.
    void setSamplingParameter(GLenum pname, GLint value)
    {
        if (pname == GL_TEXTURE_MIN_FILTER) {
            m_minFilter = static_cast<uint16_t>(value);
        } else if (pname == GL_TEXTURE_MAG_FILTER) {
            setSamplingFlag(kMagNearest, value == GL_NEAREST);
        } else if (pname == GL_TEXTURE_WRAP_S) {
            setSamplingFlag(kWrapSClamp, value == GL_CLAMP_TO_EDGE);
        } else if (pname == GL_TEXTURE_WRAP_T) {
            setSamplingFlag(kWrapTClamp, value == GL_CLAMP_TO_EDGE);
        }
    }
    GLenum minFilter() const
    {
        return m_minFilter;
    }
    bool magFilterIsNearest() const
    {
        return m_samplingFlags & kMagNearest;
    }
    bool wrapsClampToEdge() const
    {
        return (m_samplingFlags & (kWrapSClamp | kWrapTClamp)) ==
               (kWrapSClamp | kWrapTClamp);
    }

    GLint baseLevel() const
    {
        return m_baseLevel;
    }
    void setBaseLevel(GLint level)
    {
        m_baseLevel = level;
        invalidateFramebufferStatus();
    }

    GLint maxLevel() const
    {
        return m_maxLevel;
    }
    void setMaxLevel(GLint level)
    {
        m_maxLevel = level;
    }

    void setImageLevelInfo(GLenum imageTarget, GLint level,
                           GLenum internalFormat, GLsizei width, GLsizei height,
                           GLsizei depth = 1, GLenum type = 0)
    {
        invalidateFramebufferStatus();
        for (auto& entry : m_imageLevels) {
            if (entry.target == imageTarget && entry.level == level) {
                entry.internalFormat = internalFormat;
                entry.width = width;
                entry.height = height;
                entry.depth = depth;
                entry.type = static_cast<uint16_t>(type);
                entry.isDefined = true;
                return;
            }
        }
        m_imageLevels.push_back(TextureImageLevelInfo{
            imageTarget, level, internalFormat, width, height, depth,
            static_cast<uint16_t>(type), true });
    }

    const TextureImageLevelInfo* getImageLevelInfo(GLenum imageTarget,
                                                   GLint level) const
    {
        for (const auto& entry : m_imageLevels) {
            if (entry.target == imageTarget && entry.level == level &&
                entry.isDefined) {
                return &entry;
            }
        }
        return nullptr;
    }

    // OpenGL ES 3.0.6 §3.8.13: the six levelbase images are defined, square
    // and of one size and effective internal format; texStorage2D makes them
    // so.
    // shortcut: the formats are left to the driver, as the recorded ones are
    // not effective formats (unsized next to sized, the BGRA a null WebGL 1
    // upload records next to RGBA); compare them once the record maps every
    // level to its effective format.
    bool isCubeComplete() const
    {
        if (m_isImmutable) {
            return true;
        }
        const TextureImageLevelInfo* first =
            getImageLevelInfo(GL_TEXTURE_CUBE_MAP_POSITIVE_X, m_baseLevel);
        if (!first || first->width <= 0 || first->width != first->height) {
            return false;
        }
        for (GLenum face = GL_TEXTURE_CUBE_MAP_POSITIVE_X + 1;
             face <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z; ++face) {
            const TextureImageLevelInfo* info =
                getImageLevelInfo(face, m_baseLevel);
            if (!info || info->width != first->width ||
                info->height != first->height) {
                return false;
            }
        }
        return true;
    }

    // The levels a successful generateMipmap defines (OpenGL ES 3.0.6
    // §3.8.11): each like the base level at half the size of the one before,
    // down to 1 or up to MAX_LEVEL. An immutable texture has every level
    // already.
    void setMipmapLevels(GLenum target)
    {
        if (m_isImmutable) {
            return;
        }
        const bool isCube = target == GL_TEXTURE_CUBE_MAP;
        const GLenum first = isCube ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : target;
        const GLenum last = isCube ? GL_TEXTURE_CUBE_MAP_NEGATIVE_Z : target;
        for (GLenum face = first; face <= last; ++face) {
            const TextureImageLevelInfo* base =
                getImageLevelInfo(face, m_baseLevel);
            if (!base) {
                continue;
            }
            // A copy: setImageLevelInfo() may move the entries.
            TextureImageLevelInfo info = *base;
            for (GLint level = m_baseLevel;
                 level < m_maxLevel &&
                 (info.width > 1 || info.height > 1 ||
                  (target == GL_TEXTURE_3D && info.depth > 1));) {
                ++level;
                info.width = std::max(1, info.width >> 1);
                info.height = std::max(1, info.height >> 1);
                if (target == GL_TEXTURE_3D) {
                    info.depth = std::max(1, info.depth >> 1);
                }
                setImageLevelInfo(face, level, info.internalFormat, info.width,
                                  info.height, info.depth, info.type);
            }
        }
    }

    void setStorage2D(GLenum target, GLsizei levels, GLenum internalformat,
                      GLsizei width, GLsizei height)
    {
        invalidateFramebufferStatus();
        m_isImmutable = true;
        m_width = width;
        m_height = height;
        for (GLsizei l = 0; l < levels; ++l) {
            GLsizei w = std::max(1, width >> l);
            GLsizei h = std::max(1, height >> l);
            if (target == GL_TEXTURE_CUBE_MAP) {
                for (GLenum face = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
                     face <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z; ++face) {
                    setImageLevelInfo(face, l, internalformat, w, h, 1);
                }
            } else {
                setImageLevelInfo(target, l, internalformat, w, h, 1);
            }
        }
    }

    void setStorage3D(GLenum target, GLsizei levels, GLenum internalformat,
                      GLsizei width, GLsizei height, GLsizei depth)
    {
        invalidateFramebufferStatus();
        m_isImmutable = true;
        m_target = target;
        m_width = width;
        m_height = height;
        for (GLsizei l = 0; l < levels; ++l) {
            GLsizei w = std::max(1, width >> l);
            GLsizei h = std::max(1, height >> l);
            GLsizei d = (target == GL_TEXTURE_2D_ARRAY)
                            ? depth
                            : std::max(1, depth >> l);
            setImageLevelInfo(target, l, internalformat, w, h, d);
        }
    }

    BEGIN_IMPLEMENT_NEW_WITH_GC_DESC(WebGLTexture, WebGLObject);
    FILL_GC_POINTER(WebGLTexture, m_imageLevels);
    END_IMPLEMENT_NEW_WITH_GC_DESC();

private:
    enum : uint8_t {
        kMagNearest = 1,
        kWrapSClamp = 2,
        kWrapTClamp = 4,
    };

    void setSamplingFlag(uint8_t flag, bool on)
    {
        m_samplingFlags = on ? m_samplingFlags | flag : m_samplingFlags & ~flag;
    }

    GLenum m_target = 0;
    bool m_isImmutable = false;
    // In the padding after m_isImmutable. The OpenGL ES defaults: LINEAR
    // magnification, REPEAT wraps (no flag set), NEAREST_MIPMAP_LINEAR
    // minification; every filter token fits 16 bits.
    uint8_t m_samplingFlags = 0;
    uint16_t m_minFilter = GL_NEAREST_MIPMAP_LINEAR;
    GLsizei m_width = 0;
    GLsizei m_height = 0;
    GLint m_baseLevel = 0;
    GLint m_maxLevel = 1000;
    GCAtomicVector<TextureImageLevelInfo> m_imageLevels;
};
// The sampling state lives in padding: a texture is as large as without it.
static_assert(sizeof(void*) != 8 || sizeof(WebGLTexture) <= 88,
              "WebGLTexture grew");
} // namespace Starfish

#endif
#endif
