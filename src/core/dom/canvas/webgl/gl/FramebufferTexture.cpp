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
#include "core/dom/canvas/webgl/WebGLExtensions.h"

#include <algorithm>

namespace Starfish {

// Matches what other engines give an antialiased drawing buffer; every
// OpenGL ES 3.0 driver multisamples the required formats this far.
static constexpr GLint kAntialiasSamples = 4;

bool FramebufferTexture::createFramebuffer(unsigned width, unsigned height,
                                           GLsizei samples,
                                           bool renderToTexture)
{
    GL* gl = m_gl;
    // Temporarily unbind GL_PIXEL_UNPACK_BUFFER so user PBO bindings do not
    // interfere with backing texture allocation.
    struct PixelUnpackRestore {
        GL* gl;
        GLint oldBuffer;
        ~PixelUnpackRestore()
        {
            if (oldBuffer != 0) {
                gl->bindBuffer(GL_PIXEL_UNPACK_BUFFER, oldBuffer);
            }
        }
    } unpackGuard{ gl, 0 };
    gl->getIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &unpackGuard.oldBuffer);
    if (unpackGuard.oldBuffer != 0) {
        gl->bindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }

    // 1. Create a framebuffer object
    gl->genFramebuffers(1, &m_fbo);
    gl->bindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    // 2. Generate a texture and bind it
    GLint oldTextureId = 0;
    gl->getIntegerv(GL_TEXTURE_BINDING_2D, &oldTextureId);
    GLint oldRenderbuffer = 0;
    gl->getIntegerv(GL_RENDERBUFFER_BINDING, &oldRenderbuffer);
    gl->genTextures(1, &m_textureId);
    gl->bindTexture(GL_TEXTURE_2D, m_textureId);

    // 3. Attach a "color buffer" attachment
    gl->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl->texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (m_attributes.alpha) {
        gl->texImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                       GL_UNSIGNED_BYTE, nullptr);
    } else {
        gl->texImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                       GL_UNSIGNED_BYTE, nullptr);
    }

    if (!samples) {
        gl->framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                 GL_TEXTURE_2D, m_textureId, 0);
    } else if (renderToTexture) {
        gl->framebufferTexture2DMultisampleEXT(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_textureId, 0,
            samples);
    } else {
        gl->genRenderbuffers(1, &m_rboColor);
        gl->bindRenderbuffer(GL_RENDERBUFFER, m_rboColor);
        gl->renderbufferStorageMultisample(
            GL_RENDERBUFFER, samples, m_attributes.alpha ? GL_RGBA8 : GL_RGB8,
            width, height);
        gl->framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                    GL_RENDERBUFFER, m_rboColor);
    }

    // EXT_multisampled_render_to_texture takes every attachment from its own
    // entry points, with one sample count across them.
    const auto storage = [&](GLenum format) {
        if (!samples) {
            gl->renderbufferStorage(GL_RENDERBUFFER, format, width, height);
        } else if (renderToTexture) {
            // Deviation: EXT_multisampled_render_to_texture lets a flush, a
            // framebuffer switch or a read resolve this buffer implicitly,
            // and "after such a resolve, the contents of the multisample
            // buffer become undefined", so a driver that discards them does
            // not keep the depth/stencil the drawing buffer must keep until
            // the page clears it (WebGL 1.0 §2.2). Taken anyway for its near
            // zero memory cost.
            gl->renderbufferStorageMultisampleEXT(GL_RENDERBUFFER, samples,
                                                  format, width, height);
        } else {
            gl->renderbufferStorageMultisample(GL_RENDERBUFFER, samples, format,
                                               width, height);
        }
    };

    if (m_attributes.depth && !m_attributes.stencil) {
        // 4-a. Attach a "depth buffer" rbo
        gl->genRenderbuffers(1, &m_rboDepth);
        gl->bindRenderbuffer(GL_RENDERBUFFER, m_rboDepth);
        storage(GL_DEPTH_COMPONENT16);
        gl->framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                    GL_RENDERBUFFER, m_rboDepth);
    } else if (m_attributes.stencil) {
        // 4-b. Attach "depth & stencil buffer" texture or rbo
#if !defined(USE_TEXTURE_FOR_DEPTH_STENCIL)

        // NOTE: If we don't need to read either depth or stencil buffer values,
        // using a renderbuffer object is better for performance. Otherwise,
        // enable USE_TEXTURE_FOR_DEPTH_STENCIL to attach a texture.

        gl->genRenderbuffers(1, &m_rboOrTextureIdForDepthStencil);
        gl->bindRenderbuffer(GL_RENDERBUFFER, m_rboOrTextureIdForDepthStencil);
        storage(GL_DEPTH24_STENCIL8);
        gl->framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                    GL_RENDERBUFFER,
                                    m_rboOrTextureIdForDepthStencil);
#else
        gl->genTextures(1, &m_rboOrTextureIdForDepthStencil);
        gl->bindTexture(GL_TEXTURE_2D, m_rboOrTextureIdForDepthStencil);
        gl->texImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0,
                       GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
        gl->framebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                 GL_TEXTURE_2D, m_rboOrTextureIdForDepthStencil,
                                 0);
#endif
    }

    // 5. Verify that setting fbo is complete
    GLenum status = gl->checkFramebufferStatus(GL_FRAMEBUFFER);

    // 6. The single-sampled image a multisampled renderbuffer resolves to
    if (status == GL_FRAMEBUFFER_COMPLETE && samples && !renderToTexture) {
        gl->genFramebuffers(1, &m_resolveFbo);
        gl->bindFramebuffer(GL_FRAMEBUFFER, m_resolveFbo);
        gl->framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                 GL_TEXTURE_2D, m_textureId, 0);
        status = gl->checkFramebufferStatus(GL_FRAMEBUFFER);
    }

    // Put the page's bindings back on failure too: destroy() deletes the
    // texture, which would otherwise unbind it from the page's unit.
    gl->bindTexture(GL_TEXTURE_2D, oldTextureId);
    gl->bindFramebuffer(GL_FRAMEBUFFER, 0);
    gl->bindRenderbuffer(GL_RENDERBUFFER, oldRenderbuffer);

    if (status != GL_FRAMEBUFFER_COMPLETE) {
        STARFISH_LOG_ERROR("Error: creating framebuffer is incomplete. (0x%x)",
                           status);
        return false;
    }
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

    m_width = bufferWidth;
    m_height = bufferHeight;
    if (m_attributes.antialias) {
        GLint maxSamples = 0; // MAX_SAMPLES_EXT is the same token
        m_gl->getIntegerv(GL_MAX_SAMPLES, &maxSamples);
        m_samples = std::max(0, std::min(kAntialiasSamples, maxSamples));
        // A drawing buffer the driver cannot multisample (out of memory, a
        // missing entry point) is made single-sampled, and isMultisampled()
        // tells the context to report antialias false.
        if (m_samples &&
            !createFramebuffer(bufferWidth, bufferHeight, m_samples,
                               WebGLExtensionRegistry::instance()
                                   .hasEXT_multisampled_render_to_texture())) {
            STARFISH_LOG_WARN(
                "The multisampled drawing buffer could not be made; using a "
                "single-sampled one.");
            destroy();
            m_samples = 0;
        }
    }
    if (!m_samples && !createFramebuffer(bufferWidth, bufferHeight, 0, false)) {
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
    // Ensure no FBO is bound. The renderbuffer binding is the page's: GL
    // reverts it by itself if it is one of the renderbuffers deleted here.
    m_gl->bindFramebuffer(GL_FRAMEBUFFER, 0);

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
    m_gl->deleteRenderbuffers(1, &m_rboColor);
    m_gl->deleteFramebuffers(1, &m_resolveFbo);
    m_gl->deleteFramebuffers(1, &m_fbo);
    // create() destroys a multisampled attempt before it makes another.
    m_fbo = m_resolveFbo = m_textureId = m_rboColor = m_rboDepth =
        m_rboOrTextureIdForDepthStencil = 0;
    return true;
};

void FramebufferTexture::resolve()
{
    if (!m_resolveFbo || !m_needsResolve) {
        return;
    }
    m_needsResolve = false;
    GLint readFbo = 0;
    GLint drawFbo = 0;
    GLint readBuffer = GL_COLOR_ATTACHMENT0;
    m_gl->getIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFbo);
    m_gl->getIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFbo);
    const bool scissorTest = m_gl->isEnabled(GL_SCISSOR_TEST);
    m_gl->bindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);
    m_gl->getIntegerv(GL_READ_BUFFER, &readBuffer);
    m_gl->bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_resolveFbo);
    // The scissor test applies to a blit and a NONE read buffer makes it copy
    // nothing (OpenGL ES 3.0.6 §4.3.3); both are the page's, not the canvas's.
    if (scissorTest) {
        m_gl->disable(GL_SCISSOR_TEST);
    }
    if (readBuffer == GL_NONE) {
        m_gl->readBuffer(GL_COLOR_ATTACHMENT0);
    }
    m_gl->blitFramebuffer(0, 0, m_width, m_height, 0, 0, m_width, m_height,
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);
    if (readBuffer == GL_NONE) {
        m_gl->readBuffer(GL_NONE);
    }
    if (scissorTest) {
        m_gl->enable(GL_SCISSOR_TEST);
    }
    m_gl->bindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(readFbo));
    m_gl->bindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(drawFbo));
}

} // namespace Starfish

#endif // #if defined(STARFISH_ENABLE_WEBGL)
