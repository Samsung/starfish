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

#ifndef __StarfishWebGLProgram__
#define __StarfishWebGLProgram__

#if defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#include "core/dom/canvas/webgl/WebGLObject.h"
#include "core/dom/canvas/webgl/WebGLShader.h"

namespace Starfish {

class WebGLShader;
class GL;

class WebGLProgram : public WebGLObject {
public:
    struct UniformBlockInfo {
        GLint dataSize = 0;
        GLint binding = 0;
    };

    // One element of an active sampler uniform, an array element included.
    struct SamplerInfo {
        GLint location = -1;
        // GL_TEXTURE_2D, _CUBE_MAP, _3D or _2D_ARRAY; every one fits 16 bits.
        uint16_t target = 0;
        // This element and the ones after it in its array, consecutive here.
        uint16_t remaining = 1;
        // The texture unit, below MAX_COMBINED_TEXTURE_IMAGE_UNITS (so a
        // GLint that fits 31 bits), and whether the uniform is an array.
        uint32_t unit : 31;
        uint32_t isArray : 1;
    };

    WebGLProgram(ScriptBindingInstance* instance,
                 WebGLRenderingContext* context, GLuint object);
    void init(ScriptBindingInstance* instance, void* domObjectPointer) override;
    bool isWebGLProgram() const override;
    void addAttachedShader(WebGLShader* shader);
    void removeDetachedShader(WebGLShader* shader);

    Optional<GCVector<WebGLShader*>> getAttachedShaders() const;

    GCVector<WebGLShader*> getWebGLShaders() const
    {
        Optional<GCVector<WebGLShader*>> maybe = getAttachedShaders();
        if (maybe.hasValue()) {
            return maybe.value();
        }
        return GCVector<WebGLShader*>();
    }

    // The executable of the last successful link writes gl_FragColor to
    // DRAW_BUFFER0 alone when a shader linked into it does
    // (WebGLShader::writesFragColorToDrawBuffer0Only()).
    bool writesFragColorToDrawBuffer0Only() const
    {
        return m_writesFragColorToDrawBuffer0Only;
    }

    bool hasActiveUniformBlocks() const
    {
        return !m_uniformBlocks.empty();
    }

    const GCAtomicVector<UniformBlockInfo>& uniformBlocks() const
    {
        return m_uniformBlocks;
    }

    void setUniformBlockBinding(GLuint index, GLuint binding)
    {
        if (index < m_uniformBlocks.size()) {
            m_uniformBlocks[index].binding = binding;
        }
    }

    void updateUniformBlocks(GL* gl);

    const GCAtomicVector<SamplerInfo>& samplers() const
    {
        return m_samplers;
    }
    Optional<size_t> samplerIndex(GLint location) const
    {
        for (size_t i = 0; i < m_samplers.size(); ++i) {
            if (m_samplers[i].location == location) {
                return i;
            }
        }
        return NullOption;
    }
    // How many of count values from the sampler at index land in its array.
    size_t samplerSpan(size_t index, size_t count) const
    {
        return std::min({ count,
                          static_cast<size_t>(m_samplers[index].remaining),
                          m_samplers.size() - index });
    }
    void setSamplerUnits(size_t index, const GLint* units, size_t count)
    {
        const size_t span = samplerSpan(index, count);
        for (size_t i = 0; i < span; ++i) {
            m_samplers[index + i].unit = static_cast<uint32_t>(units[i]);
        }
    }
    // Rebuilds the sampler table from the driver's last successful link,
    // every unit 0 (OpenGL ES 2.0 §2.10.4).
    void updateSamplers(GL* gl);
    // Takes writesFragColorToDrawBuffer0Only() from the shaders the driver
    // just linked.
    void updateFragColorOutput();

    void setLinkFailed(bool linkFailed)
    {
        m_linkFailed = linkFailed;
    }

    bool linkFailed() const
    {
        return m_linkFailed;
    }

    void setTransformFeedbackBufferMode(GLenum mode)
    {
        m_transformFeedbackBufferMode = mode;
    }
    GLenum transformFeedbackBufferMode() const
    {
        return m_transformFeedbackBufferMode;
    }

    void setTransformFeedbackVaryingsCount(GLint count)
    {
        m_transformFeedbackVaryingsCount = count;
    }
    GLint transformFeedbackVaryingsCount() const
    {
        return m_transformFeedbackVaryingsCount;
    }

    uint32_t linkCount() const
    {
        return m_linkCount;
    }
    void incrementLinkCount()
    {
        m_linkCount++;
    }

    BEGIN_IMPLEMENT_NEW_WITH_GC_DESC(WebGLProgram, WebGLObject);
    GC_set_bit(desc, GC_WORD_OFFSET(WebGLProgram, m_attachedShaders));
    GC_set_bit(desc, GC_WORD_OFFSET(WebGLProgram, m_attachedShaders) + 1);
    FILL_GC_POINTER(WebGLProgram, m_uniformBlocks);
    FILL_GC_POINTER(WebGLProgram, m_samplers);
    END_IMPLEMENT_NEW_WITH_GC_DESC();

private:
    WebGLShader* m_attachedShaders[2] = { nullptr, nullptr };
    GCAtomicVector<UniformBlockInfo> m_uniformBlocks;
    GCAtomicVector<SamplerInfo> m_samplers;
    bool m_writesFragColorToDrawBuffer0Only = false;
    bool m_linkFailed = false;
    GLenum m_transformFeedbackBufferMode = 0x8C80; // GL_INTERLEAVED_ATTRIBS
    GLint m_transformFeedbackVaryingsCount = 0;
    uint32_t m_linkCount = 0;
};
} // namespace Starfish

#endif
#endif
