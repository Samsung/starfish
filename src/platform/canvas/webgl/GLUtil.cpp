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

#include "StarfishBase.h"
#include "platform/canvas/webgl/GLUtil.h"
#include "platform/canvas/webgl/GLContext.h"

namespace Starfish {

static GLuint createFrameBufferTexture2D(const unsigned width,
                                         const unsigned height)
{
    GLuint textureUnitId;
    glGenTextures(1, &textureUnitId);
    glBindTexture(GL_TEXTURE_2D, textureUnitId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);

    glBindTexture(GL_TEXTURE_2D, 0);
    return textureUnitId;
}

/**
 * @param width of texture
 * @param height of texture
 * @param outFbo frame buffer object
 * @param outTextureId texture unit id (rgb) of frame buffer object
 * @param outRbo render buffer object (depth) of frame buffer object
 *
 * @return result
 */
static bool createFrameBufferObject(const unsigned width, const unsigned height,
                                    GLuint& outFbo, GLuint& outTextureId,
                                    GLuint& outRbo)
{
    GLuint fbo, textureId, rbo;

    // 1. Create a framebuffer object
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // 2. Attach a "color buffer" attachment
    textureId = createFrameBufferTexture2D(width, height);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           textureId, 0);

    // 3. Attach a "depth buffer" attachment
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, rbo);

    // 4. Verify that setting fbo is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        STARFISH_LOG_ERROR("Error: creating framebuffer is incomplete. (0x%x)",
                           glCheckFramebufferStatus(GL_FRAMEBUFFER));
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    outFbo = fbo;
    outRbo = rbo;
    outTextureId = textureId;

    return true;
}

FramebufferTexture::~FramebufferTexture()
{
    destory();
}

bool FramebufferTexture::create(unsigned bufferWidth, unsigned bufferHeight,
                                GLuint& outTextureId)
{
    GLRevertableContextScope scope(GLContextScope::getCurrentXGLContext());

    if (!createFrameBufferObject(bufferWidth, bufferHeight, m_fbo, m_textureId,
                                 m_rbo)) {
        STARFISH_ASSERT(false);
    }

    outTextureId = m_textureId;
    return true;
};

bool FramebufferTexture::destory()
{
    glDeleteTextures(1, &m_textureId);
    glDeleteRenderbuffers(1, &m_rbo);
    glDeleteFramebuffers(1, &m_fbo);
    return true;
};

FBOScope::FBOScope(GLuint fbo)
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

FBOScope::~FBOScope()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

size_t Pixel::getBytesPerPixel(GLenum format, GLenum type)
{
    // Format      Type                Bytes per Pixel
    // ------------------------------------------------
    // RGBA        UNSIGNED_BYTE            4
    // RGB         UNSIGNED_BYTE            3
    // RGBA        UNSIGNED_SHORT_4_4_4_4   2
    // RGBA        UNSIGNED_SHORT_5_5_5_1   2
    // RGB         UNSIGNED_SHORT_5_6_5     2
    // LUMINANCE_ALPHA  UNSIGNED_BYTE       2
    // LUMINANCE   UNSIGNED_BYTE            1
    // ALPHA       UNSIGNED_BYTE            1
    //
    // Refs: Table 3.4: Valid pixel format and type combinations.
    // https://registry.khronos.org/OpenGL/specs/es/2.0/es_full_spec_2.0.pdf

    if (type == GL_UNSIGNED_BYTE || type == GL_FLOAT) {
        if (format == GL_RGBA || format == GL_BGRA_EXT) {
            return 4;
        } else if (format == GL_RGB) {
            return 3;
        } else if (format == GL_LUMINANCE_ALPHA) {
            return 2;
        } else if (format == GL_LUMINANCE || format == GL_ALPHA) {
            return 1;
        }
    } else if (type == GL_UNSIGNED_SHORT_4_4_4_4) {
        if (format == GL_RGBA || format == GL_BGRA_EXT) {
            return 2;
        }
    } else if (type == GL_UNSIGNED_SHORT_5_5_5_1) {
        if (format == GL_RGBA || format == GL_BGRA_EXT) {
            return 2;
        }
    } else if (type == GL_UNSIGNED_SHORT_5_6_5) {
        if (format == GL_RGB) {
            return 2;
        }
    }

    STARFISH_UNIMPLEMENTED("format: 0x%04X, type: 0x%04X", format, type);
    STARFISH_ASSERT_NOT_REACHED();
    return 0;
}

} // namespace Starfish

#endif // #if defined(STARFISH_ENABLE_WEBGL)
