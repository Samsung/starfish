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

namespace Starfish {

WebGLProgram::WebGLProgram(ScriptBindingInstance* instance,
                           WebGLRenderingContext* context, GLuint object)
    : WebGLObject(instance, context, object)
{
}

void WebGLProgram::addAttachedShader(WebGLShader* shader)
{
    if (std::find(m_webGLShaders.begin(), m_webGLShaders.end(), shader) ==
        m_webGLShaders.end()) {
        m_webGLShaders.push_back(shader);
    }
}

void WebGLProgram::removeDetachedShader(WebGLShader* shader)
{
    auto it = std::find(m_webGLShaders.begin(), m_webGLShaders.end(), shader);
    if (it != m_webGLShaders.end()) {
        m_webGLShaders.erase(it);
    }
}

} // namespace Starfish

#endif
