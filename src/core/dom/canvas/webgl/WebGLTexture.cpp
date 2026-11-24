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

#if defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#include "StarfishConfig.h"
#include "WebGLTexture.h"
#include "core/dom/canvas/webgl/WebGLUtils.h"

namespace Starfish {

// floor(log2(maxsize)) of OpenGL ES 3.0.6 §3.8.10 (Mipmapping): how many
// levels follow a levelbase image of this size. None when it has no size.
static Optional<GLint> levelsAfter(const TextureImageLevelInfo* base, bool is3D)
{
    if (!base || base->width <= 0 || base->height <= 0 || base->depth <= 0) {
        return NullOption;
    }
    GLsizei size = std::max(base->width, base->height);
    if (is3D) {
        size = std::max(size, base->depth);
    }
    GLint levels = 0;
    for (; size > 1; size >>= 1) {
        ++levels;
    }
    return levels;
}

// The components an internal format stores, as the format that uploads to
// it (OpenGL ES 3.0.6 tables 3.2 and 3.3).
static GLenum componentsOf(GLenum internalFormat)
{
    GLenum format = 0;
    GLenum type = 0;
    if (Pixel::formatAndTypeOf(internalFormat, format, type)) {
        return format;
    }
    // An image source may be recorded in the byte order it uploads in.
    return internalFormat == GL_BGRA_EXT ? GL_RGBA : internalFormat;
}

static bool differInEffectiveFormat(GLenum a, GLenum b)
{
    if (a == b) {
        return false;
    }
    // OpenGL ES 3.0.6 §3.8.3: a sized internal format is the effective one.
    if (Pixel::isSizedInternalFormat(a) && Pixel::isSizedInternalFormat(b)) {
        return true;
    }
    return componentsOf(a) != componentsOf(b);
}

WebGLTexture::WebGLTexture(ScriptBindingInstance* instance,
                           WebGLRenderingContext* context, GLuint object)
    : WebGLObject(instance, context, object)
    , m_isImmutable(false)
{
}

Optional<GLint> WebGLTexture::effectiveMaxLevel() const
{
    const GLenum face = m_target == GL_TEXTURE_CUBE_MAP
                            ? GL_TEXTURE_CUBE_MAP_POSITIVE_X
                            : m_target;
    Optional<GLint> levels = levelsAfter(getImageLevelInfo(face, m_baseLevel),
                                         m_target == GL_TEXTURE_3D);
    if (!levels) {
        return NullOption;
    }
    // Differences only: levelbase + levels may not fit.
    return levels.value() > m_maxLevel - m_baseLevel
               ? m_maxLevel
               : m_baseLevel + levels.value();
}

bool WebGLTexture::isMipmapComplete() const
{
    if (m_isImmutable) {
        return true;
    }
    if (m_baseLevel > m_maxLevel) {
        return false;
    }
    const bool isCube = m_target == GL_TEXTURE_CUBE_MAP;
    const bool is3D = m_target == GL_TEXTURE_3D;
    const GLenum first = isCube ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : m_target;
    const GLenum last = isCube ? GL_TEXTURE_CUBE_MAP_NEGATIVE_Z : m_target;
    for (GLenum face = first; face <= last; ++face) {
        const TextureImageLevelInfo* base =
            getImageLevelInfo(face, m_baseLevel);
        Optional<GLint> levels = levelsAfter(base, is3D);
        if (!levels) {
            return false;
        }
        // Offsets from levelbase: levelbase + levels may not fit.
        const GLint count = std::min(levels.value(), m_maxLevel - m_baseLevel);
        for (GLint i = 1; i <= count; ++i) {
            const TextureImageLevelInfo* info =
                getImageLevelInfo(face, m_baseLevel + i);
            if (!info || info->width != std::max(1, base->width >> i) ||
                info->height != std::max(1, base->height >> i) ||
                info->depth !=
                    (is3D ? std::max(1, base->depth >> i) : base->depth) ||
                differInEffectiveFormat(info->internalFormat,
                                        base->internalFormat)) {
                return false;
            }
        }
    }
    return true;
}

} // namespace Starfish

#endif
