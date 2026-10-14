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
        setAttachmentTexture2D(attachment, texture, 0x0DE1 /* GL_TEXTURE_2D */,
                               0);
    }

    void setAttachmentTexture2D(GLenum attachment, WebGLTexture* texture,
                                GLenum textarget, GLint level)
    {
        if (attachment >= 0x8CE0 && attachment < 0x8CE0 + 16) {
            size_t idx = attachment - 0x8CE0;
            m_attachedColorTextures[idx] = texture;
            m_attachedColorTextargets[idx] = textarget;
            m_attachedColorLevels[idx] = level;
            m_attachedColorLayers[idx] = 0;
            m_attachedColorIsLayers[idx] = false;
            if (texture) {
                m_attachedColorRenderbuffers[idx] = nullptr;
            }
            if (idx == 0) {
                m_attachedTexture = texture;
            }
        } else if (attachment == 0x8D00 /* GL_DEPTH_ATTACHMENT */) {
            m_attachedDepthTexture = texture;
            m_attachedDepthTextarget = textarget;
            m_attachedDepthLevel = level;
            m_attachedDepthLayer = 0;
            m_attachedDepthIsLayer = false;
            if (texture) {
                m_attachedDepthRenderbuffer = nullptr;
            }
        } else if (attachment == 0x8D20 /* GL_STENCIL_ATTACHMENT */) {
            m_attachedStencilTexture = texture;
            m_attachedStencilTextarget = textarget;
            m_attachedStencilLevel = level;
            m_attachedStencilLayer = 0;
            m_attachedStencilIsLayer = false;
            if (texture) {
                m_attachedStencilRenderbuffer = nullptr;
            }
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
            if (texture) {
                m_attachedDepthRenderbuffer = nullptr;
                m_attachedStencilRenderbuffer = nullptr;
            }
        }
    }

    void setAttachmentTextureLayer(GLenum attachment, WebGLTexture* texture,
                                   GLint level, GLint layer)
    {
        GLenum tgt =
            texture && texture->target() != 0 ? texture->target() : 0x806F;
        if (attachment >= 0x8CE0 && attachment < 0x8CE0 + 16) {
            size_t idx = attachment - 0x8CE0;
            m_attachedColorTextures[idx] = texture;
            m_attachedColorTextargets[idx] = tgt;
            m_attachedColorLevels[idx] = level;
            m_attachedColorLayers[idx] = layer;
            m_attachedColorIsLayers[idx] = true;
            if (texture) {
                m_attachedColorRenderbuffers[idx] = nullptr;
            }
            if (idx == 0) {
                m_attachedTexture = texture;
            }
        } else if (attachment == 0x8D00 /* GL_DEPTH_ATTACHMENT */) {
            m_attachedDepthTexture = texture;
            m_attachedDepthTextarget = tgt;
            m_attachedDepthLevel = level;
            m_attachedDepthLayer = layer;
            m_attachedDepthIsLayer = true;
            if (texture) {
                m_attachedDepthRenderbuffer = nullptr;
            }
        } else if (attachment == 0x8D20 /* GL_STENCIL_ATTACHMENT */) {
            m_attachedStencilTexture = texture;
            m_attachedStencilTextarget = tgt;
            m_attachedStencilLevel = level;
            m_attachedStencilLayer = layer;
            m_attachedStencilIsLayer = true;
            if (texture) {
                m_attachedStencilRenderbuffer = nullptr;
            }
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
            if (texture) {
                m_attachedDepthRenderbuffer = nullptr;
                m_attachedStencilRenderbuffer = nullptr;
            }
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
        if (m_attachedColorRenderbuffers[index] &&
            !m_attachedColorRenderbuffers[index]->isDeleted()) {
            return m_attachedColorRenderbuffers[index]->internalFormat();
        }
        if (m_attachedColorTextures[index] &&
            !m_attachedColorTextures[index]->isDeleted()) {
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

    GLsizei getColorAttachmentSamples(size_t index) const
    {
        if (index >= 16) {
            return 0;
        }
        if (m_attachedColorRenderbuffers[index] &&
            !m_attachedColorRenderbuffers[index]->isDeleted()) {
            return m_attachedColorRenderbuffers[index]->samples();
        }
        return 0;
    }

    void* getColorAttachmentObject(size_t index) const
    {
        if (index >= 16) {
            return nullptr;
        }
        if (m_attachedColorRenderbuffers[index] &&
            !m_attachedColorRenderbuffers[index]->isDeleted()) {
            return m_attachedColorRenderbuffers[index];
        }
        if (m_attachedColorTextures[index] &&
            !m_attachedColorTextures[index]->isDeleted()) {
            return m_attachedColorTextures[index];
        }
        return nullptr;
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

    bool hasFeedbackLoop(WebGLTexture* texture, GLint level,
                         GLint layer = -1) const
    {
        if (!texture) {
            return false;
        }
        for (size_t i = 0; i < 16; ++i) {
            if (m_attachedColorTextures[i] == texture &&
                m_attachedColorLevels[i] == level) {
                if (layer >= 0 && m_attachedColorIsLayers[i]) {
                    if (m_attachedColorLayers[i] == layer) {
                        return true;
                    }
                } else if (!m_attachedColorIsLayers[i]) {
                    return true;
                }
            }
        }
        return false;
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

        // WebGL2: Check if identical image is attached to more than one color
        // attachment
        for (size_t i = 0; i < 16; ++i) {
            for (size_t j = i + 1; j < 16; ++j) {
                if (m_attachedColorRenderbuffers[i] &&
                    m_attachedColorRenderbuffers[i] ==
                        m_attachedColorRenderbuffers[j] &&
                    !m_attachedColorRenderbuffers[i]->isDeleted()) {
                    return 0x8CDD; /* GL_FRAMEBUFFER_UNSUPPORTED */
                }
                WebGLTexture* t1 = m_attachedColorTextures[i];
                WebGLTexture* t2 = m_attachedColorTextures[j];
                if (t1 && t1 == t2 && !t1->isDeleted()) {
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
                    format == 0x8D48 /* GL_STENCIL_INDEX8 */ ||
                    format == 0x881B /* GL_RGB16F */ ||
                    format == 0x8815 /* GL_RGB32F */) {
                    return 0x8CD6; /* GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT */
                }
                if (isFloatColorFormat(format)) {
                    if (context() && context()->isWebGL2RenderingContext() &&
                        !context()->isExtensionEnabled(
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

        for (size_t i = 0; i < 16; ++i) {
            WebGLTexture* tex = m_attachedColorTextures[i];
            if (tex && !tex->isDeleted()) {
                GLsizei w = tex->width();
                GLsizei h = tex->height();
                GLenum fmt = 0x1908;
                GLenum tgt = m_attachedColorIsLayers[i]
                                 ? (tex->target() != 0 ? tex->target() : 0x806F)
                                 : m_attachedColorTextargets[i];
                const auto* info =
                    tex->getImageLevelInfo(tgt, m_attachedColorLevels[i]);
                if (info) {
                    w = info->width;
                    h = info->height;
                    fmt = info->internalFormat;
                } else if (m_attachedColorLevels[i] > 0) {
                    const auto* info0 = tex->getImageLevelInfo(tgt, 0);
                    if (info0) {
                        w = std::max(1,
                                     info0->width >> m_attachedColorLevels[i]);
                        h = std::max(1,
                                     info0->height >> m_attachedColorLevels[i]);
                        fmt = info0->internalFormat;
                    }
                }
                GLenum res = checkImage(w, h, fmt, true);
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
            WebGLTexture* tex = m_attachedDepthTexture;
            GLsizei w = tex->width();
            GLsizei h = tex->height();
            GLenum tgt = m_attachedDepthIsLayer
                             ? (tex->target() != 0 ? tex->target() : 0x806F)
                             : m_attachedDepthTextarget;
            const auto* info =
                tex->getImageLevelInfo(tgt, m_attachedDepthLevel);
            if (info) {
                w = info->width;
                h = info->height;
            } else if (m_attachedDepthLevel > 0) {
                const auto* info0 = tex->getImageLevelInfo(tgt, 0);
                if (info0) {
                    w = std::max(1, info0->width >> m_attachedDepthLevel);
                    h = std::max(1, info0->height >> m_attachedDepthLevel);
                }
            }
            GLenum res = checkImage(w, h, 0, false);
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
            WebGLTexture* tex = m_attachedStencilTexture;
            GLsizei w = tex->width();
            GLsizei h = tex->height();
            GLenum tgt = m_attachedStencilIsLayer
                             ? (tex->target() != 0 ? tex->target() : 0x806F)
                             : m_attachedStencilTextarget;
            const auto* info =
                tex->getImageLevelInfo(tgt, m_attachedStencilLevel);
            if (info) {
                w = info->width;
                h = info->height;
            } else if (m_attachedStencilLevel > 0) {
                const auto* info0 = tex->getImageLevelInfo(tgt, 0);
                if (info0) {
                    w = std::max(1, info0->width >> m_attachedStencilLevel);
                    h = std::max(1, info0->height >> m_attachedStencilLevel);
                }
            }
            GLenum res = checkImage(w, h, 0, false);
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
};
} // namespace Starfish

#endif
#endif
