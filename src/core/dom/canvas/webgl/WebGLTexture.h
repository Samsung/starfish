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
    bool isDefined = false;

    TextureImageLevelInfo() = default;
    TextureImageLevelInfo(GLenum target, GLint level, GLenum internalFormat,
                          GLsizei width, GLsizei height, GLsizei depth,
                          bool isDefined)
        : target(target)
        , level(level)
        , internalFormat(internalFormat)
        , width(width)
        , height(height)
        , depth(depth)
        , isDefined(isDefined)
    {
    }
};

class WebGLTexture : public WebGLObject {
public:
    WebGLTexture(ScriptBindingInstance* instance,
                 WebGLRenderingContext* context, GLuint object);
    void init(ScriptBindingInstance* instance, void* domObjectPointer) override;
    bool isWebGLTexture() const override;

    bool isImmutable() const
    {
        return m_isImmutable;
    }
    void setImmutable(bool immutable)
    {
        m_isImmutable = immutable;
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
    }

    GLint baseLevel() const
    {
        return m_baseLevel;
    }
    void setBaseLevel(GLint level)
    {
        m_baseLevel = level;
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
                           GLsizei depth = 1)
    {
        for (auto& entry : m_imageLevels) {
            if (entry.target == imageTarget && entry.level == level) {
                entry.internalFormat = internalFormat;
                entry.width = width;
                entry.height = height;
                entry.depth = depth;
                entry.isDefined = true;
                return;
            }
        }
        m_imageLevels.push_back(TextureImageLevelInfo{
            imageTarget, level, internalFormat, width, height, depth, true });
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

    void setStorage2D(GLenum target, GLsizei levels, GLenum internalformat,
                      GLsizei width, GLsizei height)
    {
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
        m_isImmutable = true;
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
    bool m_isImmutable = false;
    GLsizei m_width = 0;
    GLsizei m_height = 0;
    GLint m_baseLevel = 0;
    GLint m_maxLevel = 1000;
    GCAtomicVector<TextureImageLevelInfo> m_imageLevels;
};
} // namespace Starfish

#endif
#endif
