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

#if defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#include "StarfishConfig.h"
#include "WebGLProgram.h"
#include "WebGLShader.h"
#include "platform/canvas/gl/GL.h"
#include "platform/canvas/gl/IncludeGL.h"

namespace Starfish {

WebGLProgram::WebGLProgram(ScriptBindingInstance* instance,
                           WebGLRenderingContext* context, GLuint object)
    : WebGLObject(instance, context, object)
{
}

void WebGLProgram::addAttachedShader(WebGLShader* shader)
{
    if (shader) {
        for (size_t i = 0; i < 2; ++i) {
            if (m_attachedShaders[i] == shader) {
                return;
            }
            if (m_attachedShaders[i] == nullptr) {
                m_attachedShaders[i] = shader;
                if (shader->source().find("gl_FragColor") !=
                    std::string::npos) {
                    m_usesFragColor = true;
                }
                return;
            }
        }
    }
}

void WebGLProgram::removeDetachedShader(WebGLShader* shader)
{
    for (size_t i = 0; i < 2; ++i) {
        if (m_attachedShaders[i] == shader) {
            m_attachedShaders[i] = nullptr;
        }
    }
}

Optional<GCVector<WebGLShader*>> WebGLProgram::getAttachedShaders() const
{
    GCVector<WebGLShader*> shaders;
    for (size_t i = 0; i < 2; ++i) {
        if (m_attachedShaders[i]) {
            shaders.push_back(m_attachedShaders[i]);
        }
    }
    return shaders;
}

void WebGLProgram::updateUniformBlocks(GL* gl)
{
    m_uniformBlocks.clear();
    GLint numBlocks = 0;
    gl->getProgramiv(glObject(), GL_ACTIVE_UNIFORM_BLOCKS, &numBlocks);
    if (numBlocks <= 0) {
        return;
    }
    m_uniformBlocks.resize(numBlocks);
    for (GLint i = 0; i < numBlocks; i++) {
        GLint dataSize = 0;
        gl->getActiveUniformBlockiv(glObject(), i, GL_UNIFORM_BLOCK_DATA_SIZE,
                                    &dataSize);
        GLint binding = 0;
        gl->getActiveUniformBlockiv(glObject(), i, GL_UNIFORM_BLOCK_BINDING,
                                    &binding);
        m_uniformBlocks[i].dataSize = dataSize;
        m_uniformBlocks[i].binding = binding;
    }
}

} // namespace Starfish

#endif
