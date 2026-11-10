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
    for (GLint i = 0; i < numBlocks; i++) {
        GLint dataSize = 0;
        gl->getActiveUniformBlockiv(glObject(), i, GL_UNIFORM_BLOCK_DATA_SIZE,
                                    &dataSize);
        GLint binding = 0;
        gl->getActiveUniformBlockiv(glObject(), i, GL_UNIFORM_BLOCK_BINDING,
                                    &binding);
        UniformBlockInfo info;
        info.dataSize = dataSize;
        info.binding = binding;
        m_uniformBlocks.push_back(info);
    }
}

// The texture target a sampler type reads, 0 for a type that is no sampler.
static GLenum samplerTarget(GLenum type)
{
    switch (type) {
    case GL_SAMPLER_2D:
    case GL_SAMPLER_2D_SHADOW:
    case GL_INT_SAMPLER_2D:
    case GL_UNSIGNED_INT_SAMPLER_2D:
        return GL_TEXTURE_2D;
    case GL_SAMPLER_CUBE:
    case GL_SAMPLER_CUBE_SHADOW:
    case GL_INT_SAMPLER_CUBE:
    case GL_UNSIGNED_INT_SAMPLER_CUBE:
        return GL_TEXTURE_CUBE_MAP;
    case GL_SAMPLER_3D:
    case GL_INT_SAMPLER_3D:
    case GL_UNSIGNED_INT_SAMPLER_3D:
        return GL_TEXTURE_3D;
    case GL_SAMPLER_2D_ARRAY:
    case GL_SAMPLER_2D_ARRAY_SHADOW:
    case GL_INT_SAMPLER_2D_ARRAY:
    case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
        return GL_TEXTURE_2D_ARRAY;
    default:
        return 0;
    }
}

void WebGLProgram::updateFragColorOutput()
{
    m_writesFragColorToDrawBuffer0Only = false;
    for (WebGLShader* shader : m_attachedShaders) {
        if (shader && shader->writesFragColorToDrawBuffer0Only()) {
            m_writesFragColorToDrawBuffer0Only = true;
        }
    }
}

void WebGLProgram::updateSamplers(GL* gl)
{
    m_samplers.clear();
    GLint count = 0;
    gl->getProgramiv(glObject(), GL_ACTIVE_UNIFORMS, &count);
    GLint maxNameLength = 0;
    gl->getProgramiv(glObject(), GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxNameLength);
    if (count <= 0 || maxNameLength <= 0) {
        return;
    }
    std::vector<char> name(maxNameLength, '\0');
    for (GLint index = 0; index < count; ++index) {
        GLint size = 0;
        GLenum type = 0;
        GLsizei length = 0;
        gl->getActiveUniform(glObject(), index, maxNameLength, &length, &size,
                             &type, name.data());
        const GLenum target = samplerTarget(type);
        if (!target || size <= 0) {
            continue;
        }
        std::string base(name.data(), std::max<GLsizei>(length, 0));
        // The driver names an array, even one of a single element, by its
        // first element.
        const bool isArray =
            base.size() > 3 && base.compare(base.size() - 3, 3, "[0]") == 0;
        if (size > 1 && isArray) {
            base.resize(base.size() - 3);
        }
        // Every element gets an entry, in order, so that `remaining` counts
        // entries; a sampler array is far shorter than 2^16 (it takes one
        // texture unit per element).
        for (GLint element = 0; element < size; ++element) {
            SamplerInfo info;
            info.location = gl->getUniformLocation(
                glObject(),
                size > 1 ? (base + "[" + std::to_string(element) + "]").c_str()
                         : base.c_str());
            info.target = static_cast<uint16_t>(target);
            info.remaining = static_cast<uint16_t>(
                std::min<GLint>(size - element, UINT16_MAX));
            info.unit = 0;
            info.isArray = isArray;
            m_samplers.push_back(info);
        }
    }
}

} // namespace Starfish

#endif
