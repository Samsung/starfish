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

#ifndef __StarfishWebGLShader__
#define __StarfishWebGLShader__

#if defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#include "core/dom/canvas/webgl/WebGLObject.h"

namespace Starfish {

class WebGLShader : public WebGLObject {
public:
    WebGLShader(ScriptBindingInstance* instance, WebGLRenderingContext* context,
                GLuint object);
    void init(ScriptBindingInstance* instance, void* domObjectPointer) override;
    bool isWebGLShader() const override;

    void setSource(const std::string& source)
    {
        m_source = source;
    }
    const std::string& source() const
    {
        return m_source;
    }

    BEGIN_IMPLEMENT_NEW_WITH_GC_DESC(WebGLShader, WebGLObject);
    END_IMPLEMENT_NEW_WITH_GC_DESC();

private:
    std::string m_source;
};
} // namespace Starfish

#endif
#endif
