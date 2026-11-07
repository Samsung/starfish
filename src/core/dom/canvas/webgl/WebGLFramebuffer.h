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
#include "core/dom/canvas/webgl/WebGLRenderingContext.h"
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
            m_attachedColorTextargets[i] = 0;
            m_attachedColorLevels[i] = 0;
            m_attachedColorLayers[i] = 0;
            m_attachedColorIsLayers[i] = false;
        }
    }
    void init(ScriptBindingInstance* instance, void* domObjectPointer) override;
    bool isWebGLFramebuffer() const override;

    DEFINE_GETTER_SETTER(WebGLTexture*, attachedTexture, AttachedTexture);
    DEFINE_GETTER_SETTER(WebGLRenderbuffer*, attachedRenderBuffer,
                         AttachedRenderBuffer);

    // A deleted image stays attached to a framebuffer that was not bound when
    // it was deleted (OpenGL ES 2.0 §4.4.3), so attachments are not filtered
    // by isDeleted(); delete* detaches them from the bound framebuffers.
    WebGLTexture* attachedTexture(GLenum attachment) const
    {
        WebGLTexture* tex = nullptr;
        if (attachment >= 0x8CE0 && attachment < 0x8CE0 + 16) {
            tex = m_attachedColorTextures[attachment - 0x8CE0];
        } else if (attachment == 0x8D00 /* GL_DEPTH_ATTACHMENT */) {
            tex = m_attachedDepthTexture;
        } else if (attachment == 0x8D20 /* GL_STENCIL_ATTACHMENT */) {
            tex = m_attachedStencilTexture;
        } else if (attachment == 0x821A /* GL_DEPTH_STENCIL_ATTACHMENT */) {
            if (!context()->isWebGL2RenderingContext()) {
                tex = m_attachedDepthStencilTexture;
            } else if (m_attachedDepthTexture &&
                       m_attachedDepthTexture == m_attachedStencilTexture) {
                tex = m_attachedDepthTexture;
            }
        }
        return tex;
    }

    WebGLRenderbuffer* attachedRenderbuffer(GLenum attachment) const
    {
        WebGLRenderbuffer* rb = nullptr;
        if (attachment >= 0x8CE0 && attachment < 0x8CE0 + 16) {
            rb = m_attachedColorRenderbuffers[attachment - 0x8CE0];
        } else if (attachment == 0x8D00 /* GL_DEPTH_ATTACHMENT */) {
            rb = m_attachedDepthRenderbuffer;
        } else if (attachment == 0x8D20 /* GL_STENCIL_ATTACHMENT */) {
            rb = m_attachedStencilRenderbuffer;
        } else if (attachment == 0x821A /* GL_DEPTH_STENCIL_ATTACHMENT */) {
            if (!context()->isWebGL2RenderingContext()) {
                rb = m_attachedDepthStencilRenderbuffer;
            } else if (m_attachedDepthRenderbuffer &&
                       m_attachedDepthRenderbuffer ==
                           m_attachedStencilRenderbuffer) {
                rb = m_attachedDepthRenderbuffer;
            }
        }
        return rb;
    }

    void detachTexture(WebGLTexture* texture)
    {
        if (!texture) {
            return;
        }
        for (size_t i = 0; i < 16; ++i) {
            if (m_attachedColorTextures[i] == texture) {
                m_attachedColorTextures[i] = nullptr;
                if (i == 0) {
                    m_attachedTexture = nullptr;
                }
            }
        }
        if (m_attachedDepthTexture == texture) {
            m_attachedDepthTexture = nullptr;
        }
        if (m_attachedStencilTexture == texture) {
            m_attachedStencilTexture = nullptr;
        }
        if (m_attachedDepthStencilTexture == texture) {
            m_attachedDepthStencilTexture = nullptr;
        }
        invalidateFramebufferStatus();
    }

    void detachRenderbuffer(WebGLRenderbuffer* renderbuffer)
    {
        if (!renderbuffer) {
            return;
        }
        for (size_t i = 0; i < 16; ++i) {
            if (m_attachedColorRenderbuffers[i] == renderbuffer) {
                m_attachedColorRenderbuffers[i] = nullptr;
                if (i == 0) {
                    m_attachedRenderBuffer = nullptr;
                }
            }
        }
        if (m_attachedDepthRenderbuffer == renderbuffer) {
            m_attachedDepthRenderbuffer = nullptr;
        }
        if (m_attachedStencilRenderbuffer == renderbuffer) {
            m_attachedStencilRenderbuffer = nullptr;
        }
        if (m_attachedDepthStencilRenderbuffer == renderbuffer) {
            m_attachedDepthStencilRenderbuffer = nullptr;
        }
        invalidateFramebufferStatus();
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
        setAttachmentTexture2D(attachment, texture, 0x0DE1 /* GL_TEXTURE_2D */,
                               0);
    }

    // A null texture detaches the renderbuffer too (OpenGL ES 2.0 §4.4.3).
    void setAttachmentTexture2D(GLenum attachment, WebGLTexture* texture,
                                GLenum textarget, GLint level)
    {
        invalidateFramebufferStatus();
        if (attachment >= 0x8CE0 && attachment < 0x8CE0 + 16) {
            size_t idx = attachment - 0x8CE0;
            m_attachedColorTextures[idx] = texture;
            m_attachedColorTextargets[idx] = textarget;
            m_attachedColorLevels[idx] = level;
            m_attachedColorLayers[idx] = 0;
            m_attachedColorIsLayers[idx] = false;
            m_attachedColorRenderbuffers[idx] = nullptr;
            if (idx == 0) {
                m_attachedTexture = texture;
            }
        } else if (attachment == 0x8D00 /* GL_DEPTH_ATTACHMENT */) {
            m_attachedDepthTexture = texture;
            m_attachedDepthTextarget = textarget;
            m_attachedDepthLevel = level;
            m_attachedDepthLayer = 0;
            m_attachedDepthIsLayer = false;
            m_attachedDepthRenderbuffer = nullptr;
        } else if (attachment == 0x8D20 /* GL_STENCIL_ATTACHMENT */) {
            m_attachedStencilTexture = texture;
            m_attachedStencilTextarget = textarget;
            m_attachedStencilLevel = level;
            m_attachedStencilLayer = 0;
            m_attachedStencilIsLayer = false;
            m_attachedStencilRenderbuffer = nullptr;
        } else if (attachment == 0x821A /* GL_DEPTH_STENCIL_ATTACHMENT */ &&
                   !context()->isWebGL2RenderingContext()) {
            m_attachedDepthStencilTexture = texture;
            m_attachedDepthStencilTextarget = static_cast<uint16_t>(textarget);
            m_attachedDepthStencilRenderbuffer = nullptr;
        } else if (attachment == 0x821A /* GL_DEPTH_STENCIL_ATTACHMENT */) {
            m_attachedDepthTexture = texture;
            m_attachedDepthTextarget = textarget;
            m_attachedDepthLevel = level;
            m_attachedDepthLayer = 0;
            m_attachedDepthIsLayer = false;
            m_attachedStencilTexture = texture;
            m_attachedStencilTextarget = textarget;
            m_attachedStencilLevel = level;
            m_attachedStencilLayer = 0;
            m_attachedStencilIsLayer = false;
            m_attachedDepthRenderbuffer = nullptr;
            m_attachedStencilRenderbuffer = nullptr;
        }
    }

    void setAttachmentTextureLayer(GLenum attachment, WebGLTexture* texture,
                                   GLint level, GLint layer)
    {
        invalidateFramebufferStatus();
        GLenum tgt =
            texture && texture->target() != 0 ? texture->target() : 0x806F;
        if (attachment >= 0x8CE0 && attachment < 0x8CE0 + 16) {
            size_t idx = attachment - 0x8CE0;
            m_attachedColorTextures[idx] = texture;
            m_attachedColorTextargets[idx] = tgt;
            m_attachedColorLevels[idx] = level;
            m_attachedColorLayers[idx] = layer;
            m_attachedColorIsLayers[idx] = true;
            m_attachedColorRenderbuffers[idx] = nullptr;
            if (idx == 0) {
                m_attachedTexture = texture;
            }
        } else if (attachment == 0x8D00 /* GL_DEPTH_ATTACHMENT */) {
            m_attachedDepthTexture = texture;
            m_attachedDepthTextarget = tgt;
            m_attachedDepthLevel = level;
            m_attachedDepthLayer = layer;
            m_attachedDepthIsLayer = true;
            m_attachedDepthRenderbuffer = nullptr;
        } else if (attachment == 0x8D20 /* GL_STENCIL_ATTACHMENT */) {
            m_attachedStencilTexture = texture;
            m_attachedStencilTextarget = tgt;
            m_attachedStencilLevel = level;
            m_attachedStencilLayer = layer;
            m_attachedStencilIsLayer = true;
            m_attachedStencilRenderbuffer = nullptr;
        } else if (attachment == 0x821A /* GL_DEPTH_STENCIL_ATTACHMENT */) {
            m_attachedDepthTexture = texture;
            m_attachedDepthTextarget = tgt;
            m_attachedDepthLevel = level;
            m_attachedDepthLayer = layer;
            m_attachedDepthIsLayer = true;
            m_attachedStencilTexture = texture;
            m_attachedStencilTextarget = tgt;
            m_attachedStencilLevel = level;
            m_attachedStencilLayer = layer;
            m_attachedStencilIsLayer = true;
            m_attachedDepthRenderbuffer = nullptr;
            m_attachedStencilRenderbuffer = nullptr;
        }
    }

    static bool isSignedIntegerInternalFormat(GLenum format)
    {
        switch (format) {
        case 0x8D82: /* GL_RGBA32I */
        case 0x8D88: /* GL_RGBA16I */
        case 0x8D8E: /* GL_RGBA8I */
        case 0x8D83: /* GL_RGB32I */
        case 0x8D89: /* GL_RGB16I */
        case 0x8D8F: /* GL_RGB8I */
        case 0x823B: /* GL_RG32I */
        case 0x8239: /* GL_RG16I */
        case 0x8237: /* GL_RG8I */
        case 0x8235: /* GL_R32I */
        case 0x8233: /* GL_R16I */
        case 0x8231: /* GL_R8I */
            return true;
        default:
            return false;
        }
    }

    static bool isUnsignedIntegerInternalFormat(GLenum format)
    {
        switch (format) {
        case 0x8D70: /* GL_RGBA32UI */
        case 0x8D76: /* GL_RGBA16UI */
        case 0x8D7C: /* GL_RGBA8UI */
        case 0x8D71: /* GL_RGB32UI */
        case 0x8D77: /* GL_RGB16UI */
        case 0x8D7D: /* GL_RGB8UI */
        case 0x823C: /* GL_RG32UI */
        case 0x823A: /* GL_RG16UI */
        case 0x8238: /* GL_RG8UI */
        case 0x8236: /* GL_R32UI */
        case 0x8234: /* GL_R16UI */
        case 0x8232: /* GL_R8UI */
        case 0x906F: /* GL_RGB10_A2UI */
            return true;
        default:
            return false;
        }
    }

    static bool isFloatColorFormat(GLenum format)
    {
        switch (format) {
        case 0x822D: /* GL_R16F */
        case 0x822F: /* GL_RG16F */
        case 0x881A: /* GL_RGBA16F */
        case 0x822E: /* GL_R32F */
        case 0x8230: /* GL_RG32F */
        case 0x8814: /* GL_RGBA32F */
        case 0x8C3A: /* GL_R11F_G11F_B10F */
            return true;
        default:
            return false;
        }
    }

    static bool isIntegerInternalFormat(GLenum format)
    {
        return isSignedIntegerInternalFormat(format) ||
               isUnsignedIntegerInternalFormat(format);
    }

    GLenum getColorAttachmentFormat(size_t index) const
    {
        if (index >= 16) {
            return 0;
        }
        if (m_attachedColorRenderbuffers[index]) {
            return m_attachedColorRenderbuffers[index]->internalFormat();
        }
        if (m_attachedColorTextures[index]) {
            WebGLTexture* tex = m_attachedColorTextures[index];
            GLenum tgt = m_attachedColorIsLayers[index]
                             ? 0x806F /* GL_TEXTURE_3D */
                             : m_attachedColorTextargets[index];
            const auto* info =
                tex->getImageLevelInfo(tgt, m_attachedColorLevels[index]);
            if (info) {
                return info->internalFormat;
            }
        }
        return 0;
    }

    bool getColorAttachmentSize(size_t index, GLsizei& width,
                                GLsizei& height) const
    {
        if (index >= 16) {
            return false;
        }
        if (m_attachedColorRenderbuffers[index]) {
            width = m_attachedColorRenderbuffers[index]->width();
            height = m_attachedColorRenderbuffers[index]->height();
            return true;
        }
        if (m_attachedColorTextures[index]) {
            // A layer's target is its texture's (setAttachmentTextureLayer).
            const auto* info =
                m_attachedColorTextures[index]->getImageLevelInfo(
                    m_attachedColorTextargets[index],
                    m_attachedColorLevels[index]);
            if (info) {
                width = info->width;
                height = info->height;
                return true;
            }
        }
        return false;
    }

    GLsizei getColorAttachmentSamples(size_t index) const
    {
        if (index >= 16) {
            return 0;
        }
        if (m_attachedColorRenderbuffers[index]) {
            return m_attachedColorRenderbuffers[index]->samples();
        }
        return 0;
    }

    void* getColorAttachmentObject(size_t index) const
    {
        if (index >= 16) {
            return nullptr;
        }
        if (m_attachedColorRenderbuffers[index]) {
            return m_attachedColorRenderbuffers[index];
        }
        if (m_attachedColorTextures[index]) {
            return m_attachedColorTextures[index];
        }
        return nullptr;
    }

    // A null renderbuffer detaches a texture too (OpenGL ES 2.0 §4.4.2).
    void setAttachmentRenderbuffer(GLenum attachment, WebGLRenderbuffer* rb)
    {
        invalidateFramebufferStatus();
        if (attachment >= 0x8CE0 && attachment < 0x8CE0 + 16) {
            size_t idx = attachment - 0x8CE0;
            m_attachedColorRenderbuffers[idx] = rb;
            m_attachedColorTextures[idx] = nullptr;
            if (idx == 0) {
                m_attachedRenderBuffer = rb;
            }
        } else if (attachment == 0x8D00 /* GL_DEPTH_ATTACHMENT */) {
            m_attachedDepthRenderbuffer = rb;
            m_attachedDepthTexture = nullptr;
        } else if (attachment == 0x8D20 /* GL_STENCIL_ATTACHMENT */) {
            m_attachedStencilRenderbuffer = rb;
            m_attachedStencilTexture = nullptr;
        } else if (attachment == 0x821A /* GL_DEPTH_STENCIL_ATTACHMENT */ &&
                   !context()->isWebGL2RenderingContext()) {
            m_attachedDepthStencilRenderbuffer = rb;
            m_attachedDepthStencilTexture = nullptr;
        } else if (attachment == 0x821A /* GL_DEPTH_STENCIL_ATTACHMENT */) {
            m_attachedDepthRenderbuffer = rb;
            m_attachedStencilRenderbuffer = rb;
            m_attachedDepthTexture = nullptr;
            m_attachedStencilTexture = nullptr;
        }
    }

    bool isSameImageAs(size_t myIndex, const WebGLFramebuffer* other,
                       size_t otherIndex) const
    {
        if (myIndex >= 16 || otherIndex >= 16 || other == nullptr) {
            return false;
        }
        if (m_attachedColorRenderbuffers[myIndex] != nullptr &&
            m_attachedColorRenderbuffers[myIndex] ==
                other->m_attachedColorRenderbuffers[otherIndex]) {
            return true;
        }
        WebGLTexture* t1 = m_attachedColorTextures[myIndex];
        WebGLTexture* t2 = other->m_attachedColorTextures[otherIndex];
        if (t1 != nullptr && t1 == t2) {
            if (m_attachedColorLevels[myIndex] ==
                other->m_attachedColorLevels[otherIndex]) {
                if (m_attachedColorIsLayers[myIndex] &&
                    other->m_attachedColorIsLayers[otherIndex]) {
                    return m_attachedColorLayers[myIndex] ==
                           other->m_attachedColorLayers[otherIndex];
                } else if (!m_attachedColorIsLayers[myIndex] &&
                           !other->m_attachedColorIsLayers[otherIndex]) {
                    return m_attachedColorTextargets[myIndex] ==
                           other->m_attachedColorTextargets[otherIndex];
                }
            }
        }
        return false;
    }

    // Whether a level of `texture` for which reads(level) holds is attached
    // at any attachment point: the 16 color ones, depth and stencil.
    template <typename Reads>
    bool attachesReadLevel(const WebGLTexture* texture, Reads reads) const
    {
        for (size_t i = 0; i < 16; ++i) {
            if (m_attachedColorTextures[i] == texture &&
                reads(m_attachedColorLevels[i])) {
                return true;
            }
        }
        return (m_attachedDepthTexture == texture &&
                reads(m_attachedDepthLevel)) ||
               (m_attachedStencilTexture == texture &&
                reads(m_attachedStencilLevel)) ||
               (m_attachedDepthStencilTexture == texture && reads(0));
    }

    // Whether the read buffer is `level` of `texture` at `target`
    // (TEXTURE_2D or a cube face) or, for a layer of 0 or more, at `layer`.
    bool readBufferIs(const WebGLTexture* texture, GLenum target, GLint level,
                      GLint layer) const
    {
        if (m_readBuffer < 0x8CE0 || m_readBuffer >= 0x8CE0 + 16) {
            return false; // NONE
        }
        const size_t i = m_readBuffer - 0x8CE0;
        if (m_attachedColorTextures[i] != texture ||
            m_attachedColorLevels[i] != level) {
            return false;
        }
        return layer >= 0 ? m_attachedColorIsLayers[i] &&
                                m_attachedColorLayers[i] == layer
                          : !m_attachedColorIsLayers[i] &&
                                m_attachedColorTextargets[i] == target;
    }

    bool hasAnyAttachment() const
    {
        for (size_t i = 0; i < 16; ++i) {
            if (m_attachedColorTextures[i] || m_attachedColorRenderbuffers[i]) {
                return true;
            }
        }
        return m_attachedDepthTexture || m_attachedDepthRenderbuffer ||
               m_attachedStencilTexture || m_attachedStencilRenderbuffer ||
               m_attachedDepthStencilTexture ||
               m_attachedDepthStencilRenderbuffer;
    }

    // The image a WebGL 1 framebuffer has at the driver's DEPTH (or, with
    // `stencil`, STENCIL) point: its DEPTH_STENCIL_ATTACHMENT image if it has
    // one, else its DEPTH_ATTACHMENT (STENCIL_ATTACHMENT) image.
    void webGL1DepthStencilImage(bool stencil, WebGLTexture*& texture,
                                 WebGLRenderbuffer*& renderbuffer,
                                 GLenum& textarget, GLint& level) const
    {
        if (m_attachedDepthStencilTexture ||
            m_attachedDepthStencilRenderbuffer) {
            texture = m_attachedDepthStencilTexture;
            renderbuffer = m_attachedDepthStencilRenderbuffer;
            textarget = m_attachedDepthStencilTextarget;
            level = 0;
        } else if (stencil) {
            texture = m_attachedStencilTexture;
            renderbuffer = m_attachedStencilRenderbuffer;
            textarget = m_attachedStencilTextarget;
            level = m_attachedStencilLevel;
        } else {
            texture = m_attachedDepthTexture;
            renderbuffer = m_attachedDepthRenderbuffer;
            textarget = m_attachedDepthTextarget;
            level = m_attachedDepthLevel;
        }
    }

    // checkWebGLStatus(), computed again only after a change it can see.
    GLenum webGLStatus()
    {
        const uint32_t epoch = context()->framebufferStatusEpoch();
        if (!m_webGLStatus || m_webGLStatusEpoch != epoch) {
            m_webGLStatus = static_cast<uint16_t>(checkWebGLStatus());
            m_webGLStatusEpoch = epoch;
        }
        // A setter that misses invalidateFramebufferStatus() shows up here.
        STARFISH_ASSERT(m_webGLStatus == checkWebGLStatus());
        return m_webGLStatus;
    }

    GLenum checkWebGLStatus() const
    {
        if (!hasAnyAttachment()) {
            return 0x8CD7; /* GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT */
        }

        const bool isWebGL2 = context()->isWebGL2RenderingContext();
        // WebGL 1.0 §6.8: an error to attach images to more than one of
        // DEPTH, STENCIL and DEPTH_STENCIL_ATTACHMENT at once.
        if (!isWebGL2 &&
            (m_attachedDepthTexture || m_attachedDepthRenderbuffer) +
                    (m_attachedStencilTexture ||
                     m_attachedStencilRenderbuffer) +
                    (m_attachedDepthStencilTexture ||
                     m_attachedDepthStencilRenderbuffer) >
                1) {
            return 0x8CDD; /* GL_FRAMEBUFFER_UNSUPPORTED */
        }

        // WebGL2: Check if identical image is attached to more than one color
        // attachment
        for (size_t i = 0; i < 16; ++i) {
            for (size_t j = i + 1; j < 16; ++j) {
                if (m_attachedColorRenderbuffers[i] &&
                    m_attachedColorRenderbuffers[i] ==
                        m_attachedColorRenderbuffers[j]) {
                    return 0x8CDD; /* GL_FRAMEBUFFER_UNSUPPORTED */
                }
                WebGLTexture* t1 = m_attachedColorTextures[i];
                WebGLTexture* t2 = m_attachedColorTextures[j];
                if (t1 && t1 == t2) {
                    if (m_attachedColorLevels[i] == m_attachedColorLevels[j]) {
                        if (m_attachedColorIsLayers[i] &&
                            m_attachedColorIsLayers[j]) {
                            if (m_attachedColorLayers[i] ==
                                m_attachedColorLayers[j]) {
                                return 0x8CDD; /* GL_FRAMEBUFFER_UNSUPPORTED */
                            }
                        } else if (!m_attachedColorIsLayers[i] &&
                                   !m_attachedColorIsLayers[j]) {
                            if (m_attachedColorTextargets[i] ==
                                m_attachedColorTextargets[j]) {
                                return 0x8CDD; /* GL_FRAMEBUFFER_UNSUPPORTED */
                            }
                        }
                    }
                }
            }
        }

        GLsizei commonWidth = -1;
        GLsizei commonHeight = -1;

        // `required`: the one internal format the attachment point takes, 0
        // for any.
        auto checkImage = [&](GLsizei w, GLsizei h, GLenum format,
                              bool isColorAttachment,
                              GLenum required) -> GLenum {
            if (w == 0 || h == 0) {
                return 0x8CD6; /* GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT */
            }
            if (required && format != required) {
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
                    format == 0x8D48 /* GL_STENCIL_INDEX8 */ ||
                    format == 0x881B /* GL_RGB16F */ ||
                    format == 0x8815 /* GL_RGB32F */) {
                    return 0x8CD6; /* GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT */
                }
                if (isFloatColorFormat(format)) {
                    if (isWebGL2 && !context()->isExtensionEnabled(
                                        "EXT_color_buffer_float")) {
                        return 0x8CD6; /* GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT
                                        */
                    }
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

        auto checkTexture =
            [&](WebGLTexture* tex, GLenum textarget, GLint level, bool isLayer,
                bool isColorAttachment, GLenum required) -> GLenum {
            GLsizei w = tex->width();
            GLsizei h = tex->height();
            GLenum fmt = isColorAttachment ? 0x1908 /* GL_RGBA */ : 0;
            GLenum tgt = isLayer ? (tex->target() != 0 ? tex->target() : 0x806F)
                                 : textarget;
            const auto* info = tex->getImageLevelInfo(tgt, level);
            if (info) {
                w = info->width;
                h = info->height;
                fmt = info->internalFormat;
            } else if (level > 0) {
                const auto* info0 = tex->getImageLevelInfo(tgt, 0);
                if (info0) {
                    w = std::max(1, info0->width >> level);
                    h = std::max(1, info0->height >> level);
                    fmt = info0->internalFormat;
                }
            }
            // WebGL 1.0 §6.36, for both versions: a face of a cube map that
            // is not cube complete is not attachment complete. OpenGL ES
            // 3.0.6 §4.4.4.1 asks it only of a level other than the base.
            if (tgt >= 0x8515 /* GL_TEXTURE_CUBE_MAP_POSITIVE_X */ &&
                tgt <= 0x851A /* GL_TEXTURE_CUBE_MAP_NEGATIVE_Z */ &&
                !tex->isCubeComplete()) {
                return 0x8CD6; /* GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT */
            }
            return checkImage(w, h, fmt, isColorAttachment, required);
        };

        for (size_t i = 0; i < 16; ++i) {
            if (WebGLTexture* tex = m_attachedColorTextures[i]) {
                GLenum res = checkTexture(tex, m_attachedColorTextargets[i],
                                          m_attachedColorLevels[i],
                                          m_attachedColorIsLayers[i], true, 0);
                if (res != 0x8CD5) {
                    return res;
                }
            }
            if (WebGLRenderbuffer* rb = m_attachedColorRenderbuffers[i]) {
                GLenum res = checkImage(rb->width(), rb->height(),
                                        rb->internalFormat(), true, 0);
                if (res != 0x8CD5) {
                    return res;
                }
            }
        }

        // WebGL 1.0 §6.8 makes a WebGL 1 depth, stencil or depth-stencil
        // image of another format FRAMEBUFFER_UNSUPPORTED; such an image
        // breaks the attachment completeness of OpenGL ES 2.0.25 §4.4.5,
        // which is what the conformance tests and ANGLE report:
        // INCOMPLETE_ATTACHMENT. WebGL 2 leaves the formats to the driver.
        struct DepthStencilPoint {
            WebGLTexture* texture;
            GLenum textarget;
            GLint level;
            bool isLayer;
            WebGLRenderbuffer* renderbuffer;
            GLenum textureFormat;
            GLenum renderbufferFormat;
        };
        const DepthStencilPoint points[] = {
            { m_attachedDepthTexture, m_attachedDepthTextarget,
              m_attachedDepthLevel, m_attachedDepthIsLayer,
              m_attachedDepthRenderbuffer, 0x1902 /* GL_DEPTH_COMPONENT */,
              0x81A5 /* GL_DEPTH_COMPONENT16 */ },
            { m_attachedStencilTexture, m_attachedStencilTextarget,
              m_attachedStencilLevel, m_attachedStencilIsLayer,
              m_attachedStencilRenderbuffer, 0x8D48 /* GL_STENCIL_INDEX8 */,
              0x8D48 /* GL_STENCIL_INDEX8 */ },
            { m_attachedDepthStencilTexture, m_attachedDepthStencilTextarget, 0,
              false, m_attachedDepthStencilRenderbuffer,
              0x84F9 /* GL_DEPTH_STENCIL */, 0x84F9 /* GL_DEPTH_STENCIL */ },
        };
        for (const DepthStencilPoint& point : points) {
            if (point.texture) {
                GLenum res = checkTexture(point.texture, point.textarget,
                                          point.level, point.isLayer, false,
                                          isWebGL2 ? 0 : point.textureFormat);
                if (res != 0x8CD5) {
                    return res;
                }
            }
            if (WebGLRenderbuffer* rb = point.renderbuffer) {
                GLenum res =
                    checkImage(rb->width(), rb->height(), rb->internalFormat(),
                               false, isWebGL2 ? 0 : point.renderbufferFormat);
                if (res != 0x8CD5) {
                    return res;
                }
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
    FILL_GC_POINTER(WebGLFramebuffer, m_attachedDepthStencilTexture);
    FILL_GC_POINTER(WebGLFramebuffer, m_attachedDepthStencilRenderbuffer);
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
    GLenum m_attachedColorTextargets[16];
    GLint m_attachedColorLevels[16];
    GLint m_attachedColorLayers[16];
    bool m_attachedColorIsLayers[16];
    WebGLTexture* m_attachedDepthTexture = nullptr;
    GLenum m_attachedDepthTextarget = 0;
    GLint m_attachedDepthLevel = 0;
    GLint m_attachedDepthLayer = 0;
    bool m_attachedDepthIsLayer = false;
    WebGLRenderbuffer* m_attachedDepthRenderbuffer = nullptr;
    WebGLTexture* m_attachedStencilTexture = nullptr;
    GLenum m_attachedStencilTextarget = 0;
    GLint m_attachedStencilLevel = 0;
    GLint m_attachedStencilLayer = 0;
    bool m_attachedStencilIsLayer = false;
    WebGLRenderbuffer* m_attachedStencilRenderbuffer = nullptr;
    // WebGL 1's DEPTH_STENCIL_ATTACHMENT, a point of its own (WebGL 1.0
    // §6.8); WebGL 2 makes it an alias of DEPTH and STENCIL (WebGL 2.0
    // §5.1.5) and leaves these null. Its texture level is 0.
    WebGLTexture* m_attachedDepthStencilTexture = nullptr;
    WebGLRenderbuffer* m_attachedDepthStencilRenderbuffer = nullptr;
    // Every target and status token fits 16 bits.
    uint16_t m_attachedDepthStencilTextarget = 0;
    uint16_t m_webGLStatus = 0; // 0: not computed yet
    uint32_t m_webGLStatusEpoch = 0;
};
} // namespace Starfish

#endif
#endif
