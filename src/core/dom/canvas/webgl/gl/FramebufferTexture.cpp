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

#include "StarfishConfig.h"

#if defined(STARFISH_ENABLE_WEBGL)

#include "FramebufferTexture.h"
#include "GLContext.h"
#include "platform/canvas/gl/IncludeGL.h"
#include "platform/canvas/gl/GL.h"
#include "core/modules/renderer/Renderer.h"

namespace Starfish {

static GLuint createFrameBufferTexture2D(GL* gl, const unsigned width,
                                         const unsigned height)
{
    GLuint textureUnitId;
    gl->genTextures(1, &textureUnitId);
    gl->bindTexture(GL_TEXTURE_2D, textureUnitId);
    gl->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl->texImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                   GL_UNSIGNED_BYTE, nullptr);

    gl->bindTexture(GL_TEXTURE_2D, 0);
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
static bool createFrameBufferObject(GL* gl, const unsigned width,
                                    const unsigned height, GLuint& outTextureId,
                                    GLuint& outFbo, GLuint& outRboDepth,
                                    GLuint& outRboStencil,
                                    const FrameBufferAttributes& attr)
{
    // 1. Create a framebuffer object
    gl->genFramebuffers(1, &outFbo);
    gl->bindFramebuffer(GL_FRAMEBUFFER, outFbo);

    // 2. Attach a "color buffer" attachment
    outTextureId = createFrameBufferTexture2D(gl, width, height);
    gl->bindTexture(GL_TEXTURE_2D, outTextureId);
    gl->framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D, outTextureId, 0);

    // 3. Attach a "depth buffer" attachment
    if (attr.depth) {
        gl->genRenderbuffers(1, &outRboDepth);
        gl->bindRenderbuffer(GL_RENDERBUFFER, outRboDepth);
        gl->renderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width,
                                height);
        gl->framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                    GL_RENDERBUFFER, outRboDepth);
    }

    // 4. Attach a "stencil buffer" attachment
    if (attr.stencil) {
        STARFISH_UNIMPLEMENTED("Attach a stencil buffer attachment");
    }

    // 5. Verify that setting fbo is complete
    if (gl->checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        STARFISH_LOG_ERROR("Error: creating framebuffer is incomplete. (0x%x)",
                           glCheckFramebufferStatus(GL_FRAMEBUFFER));
        return false;
    }

    gl->bindTexture(GL_TEXTURE_2D, 0);
    gl->bindFramebuffer(GL_FRAMEBUFFER, 0);
    gl->bindRenderbuffer(GL_RENDERBUFFER, 0);

    return true;
}

FramebufferTexture::FramebufferTexture(Renderer* renderer)
    : m_renderer(renderer)
    , m_gl(renderer->gl())
{
}

FramebufferTexture::~FramebufferTexture()
{
    // NOTE: Add a guard to set the GL context used at creation if necessary.
    // Refs: m_framebufferTexture.reset() in WebGLRenderingContextBaseMixIn.
    destory();
}

bool FramebufferTexture::create(unsigned bufferWidth, unsigned bufferHeight,
                                GLuint& outTextureId)
{
    GLRevertableContextScope scope(GLContextScope::getCurrentGLContext(),
                                   m_renderer);

    if (!createFrameBufferObject(m_gl, bufferWidth, bufferHeight, m_textureId,
                                 m_fbo, m_rboDepth, m_rboStencil,
                                 m_attributes)) {
        // TODO: Handle this error handling.
        STARFISH_ASSERT(false);
    }

    outTextureId = m_textureId;

    if (m_attributes.depth && m_rboDepth == 0) {
        STARFISH_LOG_WARN("No depth buffer assigned.");
        return false;
    }
    if (m_attributes.stencil && m_rboStencil == 0) {
        // TODO: STARFISH_LOG_WARN("No stencil buffer assigned.");
    }
    return true;
};

bool FramebufferTexture::destory()
{
    // Ensure no FBO is bound.
    m_gl->bindFramebuffer(GL_FRAMEBUFFER, 0);
    m_gl->bindRenderbuffer(GL_RENDERBUFFER, 0);

    m_gl->deleteTextures(1, &m_textureId);
    if (m_rboDepth != 0) {
        m_gl->deleteRenderbuffers(1, &m_rboDepth);
    }
    if (m_rboStencil != 0) {
        m_gl->deleteRenderbuffers(1, &m_rboStencil);
    }
    m_gl->deleteFramebuffers(1, &m_fbo);
    return true;
};

} // namespace Starfish

#endif // #if defined(STARFISH_ENABLE_WEBGL)
