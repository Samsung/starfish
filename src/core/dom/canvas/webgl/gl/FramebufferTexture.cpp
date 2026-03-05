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

static bool createFrameBufferObject(GL* gl, const unsigned width,
                                    const unsigned height, GLuint& outFbo,
                                    GLuint& outTextureId, GLuint& outRboDepth,
                                    GLuint& outRboOrTextureIdForDepthStencil,
                                    const bool needAlphaBuffer,
                                    const bool needDepthBuffer,
                                    const bool needStencilBuffer)
{
    // 1. Create a framebuffer object
    gl->genFramebuffers(1, &outFbo);
    gl->bindFramebuffer(GL_FRAMEBUFFER, outFbo);

    // 2. Generate a texture and bind it
    GLint oldTextureId = 0;
    gl->getIntegerv(GL_TEXTURE_BINDING_2D, &oldTextureId);
    gl->genTextures(1, &outTextureId);
    gl->bindTexture(GL_TEXTURE_2D, outTextureId);

    // 3. Attach a "color buffer" attachment
    gl->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (needAlphaBuffer) {
        gl->texImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                       GL_UNSIGNED_BYTE, nullptr);
    } else {
        gl->texImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                       GL_UNSIGNED_BYTE, nullptr);
    }

    gl->framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D, outTextureId, 0);

    if (needDepthBuffer && !needStencilBuffer) {
        // 4-a. Attach a "depth buffer" rbo
        gl->genRenderbuffers(1, &outRboDepth);
        gl->bindRenderbuffer(GL_RENDERBUFFER, outRboDepth);
        gl->renderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width,
                                height);
        gl->framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                    GL_RENDERBUFFER, outRboDepth);
    } else if (needStencilBuffer) {
        // 4-b. Attach "depth & stencil buffer" texture or rbo
#if !defined(USE_TEXTURE_FOR_DEPTH_STENCIL)

        // NOTE: If we don't need to read either depth or stencil buffer values,
        // using a renderbuffer object is better for performance. Otherwise,
        // enable USE_TEXTURE_FOR_DEPTH_STENCIL to attach a texture.

        gl->genRenderbuffers(1, &outRboOrTextureIdForDepthStencil);
        gl->bindRenderbuffer(GL_RENDERBUFFER, outRboOrTextureIdForDepthStencil);
        gl->renderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width,
                                height);
        gl->framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                    GL_RENDERBUFFER,
                                    outRboOrTextureIdForDepthStencil);
#else
        gl->genTextures(1, &outRboOrTextureIdForDepthStencil);
        gl->bindTexture(GL_TEXTURE_2D, outRboOrTextureIdForDepthStencil);
        gl->texImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0,
                       GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
        gl->framebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                 GL_TEXTURE_2D,
                                 outRboOrTextureIdForDepthStencil, 0);
#endif
    }

    // 5. Verify that setting fbo is complete
    if (gl->checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        STARFISH_LOG_ERROR("Error: creating framebuffer is incomplete. (0x%x)",
                           glCheckFramebufferStatus(GL_FRAMEBUFFER));
        return false;
    }

    gl->bindTexture(GL_TEXTURE_2D, oldTextureId);
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
    destroy();
}

bool FramebufferTexture::create(unsigned bufferWidth, unsigned bufferHeight,
                                GLuint& outTextureId)
{
    GLRevertableContextScope scope(GLContextScope::getCurrentGLContext(),
                                   m_renderer);

    if (!createFrameBufferObject(
            m_gl, bufferWidth, bufferHeight, m_fbo, m_textureId, m_rboDepth,
            m_rboOrTextureIdForDepthStencil, m_attributes.alpha,
            m_attributes.depth, m_attributes.stencil)) {
        STARFISH_LOG_ERROR("No frame buffer assigned.");
        return false;
    }

    outTextureId = m_textureId;

    if (!m_attributes.stencil && (m_attributes.depth && m_rboDepth == 0)) {
        STARFISH_LOG_ERROR("No depth buffer assigned.");
        return false;
    } else if (m_attributes.stencil && m_rboOrTextureIdForDepthStencil == 0) {
        STARFISH_LOG_ERROR("No stencil buffer assigned.");
        return false;
    }

    return true;
};

bool FramebufferTexture::destroy()
{
    // Ensure no FBO is bound.
    m_gl->bindFramebuffer(GL_FRAMEBUFFER, 0);
    m_gl->bindRenderbuffer(GL_RENDERBUFFER, 0);

    m_gl->deleteTextures(1, &m_textureId);
    if (m_rboDepth != 0) {
        m_gl->deleteRenderbuffers(1, &m_rboDepth);
    }

    if (m_rboOrTextureIdForDepthStencil != 0) {
#if !defined(USE_TEXTURE_FOR_DEPTH_STENCIL)
        m_gl->deleteRenderbuffers(1, &m_rboOrTextureIdForDepthStencil);
#else
        m_gl->deleteTextures(1, &m_rboOrTextureIdForDepthStencil);
#endif
    }
    m_gl->deleteFramebuffers(1, &m_fbo);
    return true;
};

} // namespace Starfish

#endif // #if defined(STARFISH_ENABLE_WEBGL)
