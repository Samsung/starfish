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

    bool usesFragColor() const
    {
        return m_usesFragColor;
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

    void setLinkFailed(bool linkFailed)
    {
        m_linkFailed = linkFailed;
    }

    bool linkFailed() const
    {
        return m_linkFailed;
    }

    BEGIN_IMPLEMENT_NEW_WITH_GC_DESC(WebGLProgram, WebGLObject);
    GC_set_bit(desc, GC_WORD_OFFSET(WebGLProgram, m_attachedShaders));
    GC_set_bit(desc, GC_WORD_OFFSET(WebGLProgram, m_attachedShaders) + 1);
    FILL_GC_POINTER(WebGLProgram, m_uniformBlocks);
    END_IMPLEMENT_NEW_WITH_GC_DESC();

private:
    WebGLShader* m_attachedShaders[2] = { nullptr, nullptr };
    GCAtomicVector<UniformBlockInfo> m_uniformBlocks;
    bool m_usesFragColor = false;
    bool m_linkFailed = false;
};
} // namespace Starfish

#endif
#endif
