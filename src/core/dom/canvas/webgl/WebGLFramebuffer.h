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
#include "core/dom/canvas/webgl/WebGLTexture.h"

namespace Starfish {

class WebGLRenderbuffer;

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
        return (attachment == 0x8CE0) ? m_attachedTexture : nullptr;
    }

    WebGLTexture* attachedColorTexture(size_t index) const
    {
        if (index < 16) {
            return m_attachedColorTextures[index];
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
            m_attachedColorTextures[attachment - 0x8CE0] = texture;
        }
    }

    bool hasConsistentDimensions() const
    {
        GLsizei commonWidth = -1;
        GLsizei commonHeight = -1;
        for (size_t i = 0; i < 16; ++i) {
            WebGLTexture* tex = m_attachedColorTextures[i];
            if (tex && !tex->isDeleted() && tex->width() > 0 &&
                tex->height() > 0) {
                if (commonWidth == -1) {
                    commonWidth = tex->width();
                    commonHeight = tex->height();
                } else if (commonWidth != tex->width() ||
                           commonHeight != tex->height()) {
                    return false;
                }
            }
        }
        return true;
    }

private:
    WebGLTexture* m_attachedTexture = nullptr;
    WebGLRenderbuffer* m_attachedRenderBuffer = nullptr;
    GLenum m_drawBuffers[16] = { 0x8CE0 };
    size_t m_drawBufferCount = 1;
    GLenum m_readBuffer = 0x8CE0;
    WebGLTexture* m_attachedColorTextures[16];
};
} // namespace Starfish

#endif
#endif
