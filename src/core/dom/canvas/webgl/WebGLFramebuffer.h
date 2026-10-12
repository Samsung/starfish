/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishWebGLFramebuffer__
#define __StarfishWebGLFramebuffer__

#if defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#include "core/dom/canvas/webgl/WebGLObject.h"
#include "core/dom/canvas/webgl/WebGLRenderbuffer.h"
#include "core/dom/canvas/webgl/WebGLTexture.h"

namespace Starfish {

class WebGLFramebuffer : public WebGLObject {
public:
    WebGLFramebuffer(ScriptBindingInstance* instance,
                     WebGLRenderingContext* context, GLuint object)
        : WebGLObject(instance, context, object)
    {
        m_drawBuffers[0] = 0x8CE0;
        m_drawBufferCount = 1;
        for (size_t i = 0; i < 16; ++i) {
            m_attachedColorTextures[i] = nullptr;
            m_attachedColorRenderbuffers[i] = nullptr;
        }
    }
    void init(ScriptBindingInstance* instance, void* domObjectPointer) override;
    bool isWebGLFramebuffer() const override;

    DEFINE_GETTER_SETTER(WebGLTexture*, attachedTexture, AttachedTexture);
    DEFINE_GETTER_SETTER(WebGLRenderbuffer*, attachedRenderBuffer,
                         AttachedRenderBuffer);

    WebGLTexture* attachedTexture(GLenum attachment) const
    {
        if (attachment >= 0x8CE0 && attachment < 0x8CE0 + 16) {
            return m_attachedColorTextures[attachment - 0x8CE0];
        }
        if (attachment == 0x8D00 /* GL_DEPTH_ATTACHMENT */) {
            return m_attachedDepthTexture;
        }
        if (attachment == 0x8D20 /* GL_STENCIL_ATTACHMENT */) {
            return m_attachedStencilTexture;
        }
        if (attachment == 0x821A /* GL_DEPTH_STENCIL_ATTACHMENT */) {
            if (m_attachedDepthTexture &&
                m_attachedDepthTexture == m_attachedStencilTexture) {
                return m_attachedDepthTexture;
            }
        }
        return nullptr;
    }

    WebGLRenderbuffer* attachedRenderbuffer(GLenum attachment) const
    {
        if (attachment >= 0x8CE0 && attachment < 0x8CE0 + 16) {
            return m_attachedColorRenderbuffers[attachment - 0x8CE0];
        }
        if (attachment == 0x8D00 /* GL_DEPTH_ATTACHMENT */) {
            return m_attachedDepthRenderbuffer;
        }
        if (attachment == 0x8D20 /* GL_STENCIL_ATTACHMENT */) {
            return m_attachedStencilRenderbuffer;
        }
        if (attachment == 0x821A /* GL_DEPTH_STENCIL_ATTACHMENT */) {
            if (m_attachedDepthRenderbuffer &&
                m_attachedDepthRenderbuffer == m_attachedStencilRenderbuffer) {
                return m_attachedDepthRenderbuffer;
            }
        }
        return nullptr;
    }

    WebGLTexture* attachedColorTexture(size_t index) const
    {
        if (index < 16) {
            return m_attachedColorTextures[index];
        }
        return nullptr;
    }

    WebGLRenderbuffer* attachedColorRenderbuffer(size_t index) const
    {
        if (index < 16) {
            return m_attachedColorRenderbuffers[index];
        }
        return nullptr;
    }

    const GLenum* drawBuffers() const
    {
        return m_drawBuffers;
    }
    size_t drawBufferCount() const
    {
        return m_drawBufferCount;
    }
    void setDrawBuffers(const GCAtomicVector<GLenum>& buffers)
    {
        m_drawBufferCount = std::min(buffers.size(), static_cast<size_t>(16));
        for (size_t i = 0; i < m_drawBufferCount; ++i) {
            m_drawBuffers[i] = buffers[i];
        }
    }

    GLenum readBuffer() const
    {
        return m_readBuffer;
    }
    void setReadBuffer(GLenum buffer)
    {
        m_readBuffer = buffer;
    }

    void setAttachmentTexture(GLenum attachment, WebGLTexture* texture)
    {
        if (attachment >= 0x8CE0 && attachment < 0x8CE0 + 16) {
            size_t idx = attachment - 0x8CE0;
            m_attachedColorTextures[idx] = texture;
            if (texture) {
                m_attachedColorRenderbuffers[idx] = nullptr;
            }
            if (idx == 0) {
                m_attachedTexture = texture;
            }
        } else if (attachment == 0x8D00 /* GL_DEPTH_ATTACHMENT */) {
            m_attachedDepthTexture = texture;
            if (texture) {
                m_attachedDepthRenderbuffer = nullptr;
            }
        } else if (attachment == 0x8D20 /* GL_STENCIL_ATTACHMENT */) {
            m_attachedStencilTexture = texture;
            if (texture) {
                m_attachedStencilRenderbuffer = nullptr;
            }
        } else if (attachment == 0x821A /* GL_DEPTH_STENCIL_ATTACHMENT */) {
            m_attachedDepthTexture = texture;
            m_attachedStencilTexture = texture;
            if (texture) {
                m_attachedDepthRenderbuffer = nullptr;
                m_attachedStencilRenderbuffer = nullptr;
            }
        }
    }

    void setAttachmentRenderbuffer(GLenum attachment, WebGLRenderbuffer* rb)
    {
        if (attachment >= 0x8CE0 && attachment < 0x8CE0 + 16) {
            size_t idx = attachment - 0x8CE0;
            m_attachedColorRenderbuffers[idx] = rb;
            if (rb) {
                m_attachedColorTextures[idx] = nullptr;
            }
            if (idx == 0) {
                m_attachedRenderBuffer = rb;
            }
        } else if (attachment == 0x8D00 /* GL_DEPTH_ATTACHMENT */) {
            m_attachedDepthRenderbuffer = rb;
            if (rb) {
                m_attachedDepthTexture = nullptr;
            }
        } else if (attachment == 0x8D20 /* GL_STENCIL_ATTACHMENT */) {
            m_attachedStencilRenderbuffer = rb;
            if (rb) {
                m_attachedStencilTexture = nullptr;
            }
        } else if (attachment == 0x821A /* GL_DEPTH_STENCIL_ATTACHMENT */) {
            m_attachedDepthRenderbuffer = rb;
            m_attachedStencilRenderbuffer = rb;
            if (rb) {
                m_attachedDepthTexture = nullptr;
                m_attachedStencilTexture = nullptr;
            }
        }
    }

    bool hasAnyAttachment() const
    {
        for (size_t i = 0; i < 16; ++i) {
            if ((m_attachedColorTextures[i] &&
                 !m_attachedColorTextures[i]->isDeleted()) ||
                (m_attachedColorRenderbuffers[i] &&
                 !m_attachedColorRenderbuffers[i]->isDeleted())) {
                return true;
            }
        }
        return (m_attachedDepthTexture &&
                !m_attachedDepthTexture->isDeleted()) ||
               (m_attachedDepthRenderbuffer &&
                !m_attachedDepthRenderbuffer->isDeleted()) ||
               (m_attachedStencilTexture &&
                !m_attachedStencilTexture->isDeleted()) ||
               (m_attachedStencilRenderbuffer &&
                !m_attachedStencilRenderbuffer->isDeleted());
    }

    GLenum checkWebGLStatus() const
    {
        if (!hasAnyAttachment()) {
            return 0x8CD7; /* GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT */
        }

        GLsizei commonWidth = -1;
        GLsizei commonHeight = -1;

        auto checkImage = [&](GLsizei w, GLsizei h, GLenum format,
                              bool isColorAttachment) -> GLenum {
            if (w == 0 || h == 0) {
                return 0x8CD6; /* GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT */
            }
            if (isColorAttachment) {
                if (format == 0x1902 /* GL_DEPTH_COMPONENT */ ||
                    format == 0x81A5 /* GL_DEPTH_COMPONENT16 */ ||
                    format == 0x81A6 /* GL_DEPTH_COMPONENT24 */ ||
                    format == 0x8CAC /* GL_DEPTH_COMPONENT32F */ ||
                    format == 0x84F9 /* GL_DEPTH_STENCIL */ ||
                    format == 0x88F0 /* GL_DEPTH24_STENCIL8 */ ||
                    format == 0x8CAD /* GL_DEPTH32F_STENCIL8 */ ||
                    format == 0x8D48 /* GL_STENCIL_INDEX8 */) {
                    return 0x8CD6; /* GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT */
                }
            }
            if (commonWidth == -1) {
                commonWidth = w;
                commonHeight = h;
            } else if (commonWidth != w || commonHeight != h) {
                return 0x8CD9; /* GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS */
            }
            return 0x8CD5; /* GL_FRAMEBUFFER_COMPLETE */
        };

        for (size_t i = 0; i < 16; ++i) {
            WebGLTexture* tex = m_attachedColorTextures[i];
            if (tex && !tex->isDeleted()) {
                GLenum res =
                    checkImage(tex->width(), tex->height(), 0x1908, true);
                if (res != 0x8CD5) {
                    return res;
                }
            }
            WebGLRenderbuffer* rb = m_attachedColorRenderbuffers[i];
            if (rb && !rb->isDeleted()) {
                GLenum res = checkImage(rb->width(), rb->height(),
                                        rb->internalFormat(), true);
                if (res != 0x8CD5) {
                    return res;
                }
            }
        }

        if (m_attachedDepthTexture && !m_attachedDepthTexture->isDeleted()) {
            GLenum res = checkImage(m_attachedDepthTexture->width(),
                                    m_attachedDepthTexture->height(), 0, false);
            if (res != 0x8CD5) {
                return res;
            }
        }
        if (m_attachedDepthRenderbuffer &&
            !m_attachedDepthRenderbuffer->isDeleted()) {
            GLenum res = checkImage(
                m_attachedDepthRenderbuffer->width(),
                m_attachedDepthRenderbuffer->height(),
                m_attachedDepthRenderbuffer->internalFormat(), false);
            if (res != 0x8CD5) {
                return res;
            }
        }
        if (m_attachedStencilTexture &&
            !m_attachedStencilTexture->isDeleted()) {
            GLenum res =
                checkImage(m_attachedStencilTexture->width(),
                           m_attachedStencilTexture->height(), 0, false);
            if (res != 0x8CD5) {
                return res;
            }
        }
        if (m_attachedStencilRenderbuffer &&
            !m_attachedStencilRenderbuffer->isDeleted()) {
            GLenum res = checkImage(
                m_attachedStencilRenderbuffer->width(),
                m_attachedStencilRenderbuffer->height(),
                m_attachedStencilRenderbuffer->internalFormat(), false);
            if (res != 0x8CD5) {
                return res;
            }
        }

        return 0x8CD5; /* GL_FRAMEBUFFER_COMPLETE */
    }

    bool hasConsistentDimensions() const
    {
        return checkWebGLStatus() == 0x8CD5;
    }

    BEGIN_IMPLEMENT_NEW_WITH_GC_DESC(WebGLFramebuffer, WebGLObject);
    FILL_GC_POINTER(WebGLFramebuffer, m_attachedTexture);
    FILL_GC_POINTER(WebGLFramebuffer, m_attachedRenderBuffer);
    FILL_GC_POINTER(WebGLFramebuffer, m_attachedDepthTexture);
    FILL_GC_POINTER(WebGLFramebuffer, m_attachedDepthRenderbuffer);
    FILL_GC_POINTER(WebGLFramebuffer, m_attachedStencilTexture);
    FILL_GC_POINTER(WebGLFramebuffer, m_attachedStencilRenderbuffer);
    for (size_t i = 0; i < 16; ++i) {
        GC_set_bit(desc,
                   GC_WORD_OFFSET(WebGLFramebuffer, m_attachedColorTextures) +
                       i);
        GC_set_bit(
            desc,
            GC_WORD_OFFSET(WebGLFramebuffer, m_attachedColorRenderbuffers) + i);
    }
    END_IMPLEMENT_NEW_WITH_GC_DESC();

private:
    WebGLTexture* m_attachedTexture = nullptr;
    WebGLRenderbuffer* m_attachedRenderBuffer = nullptr;
    GLenum m_drawBuffers[16] = { 0x8CE0 };
    size_t m_drawBufferCount = 1;
    GLenum m_readBuffer = 0x8CE0;
    WebGLTexture* m_attachedColorTextures[16];
    WebGLRenderbuffer* m_attachedColorRenderbuffers[16];
    WebGLTexture* m_attachedDepthTexture = nullptr;
    WebGLRenderbuffer* m_attachedDepthRenderbuffer = nullptr;
    WebGLTexture* m_attachedStencilTexture = nullptr;
    WebGLRenderbuffer* m_attachedStencilRenderbuffer = nullptr;
};
} // namespace Starfish

#endif
#endif
