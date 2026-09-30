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

#ifndef __StarfishWebGLTexture__
#define __StarfishWebGLTexture__

#if defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#include "core/dom/canvas/webgl/WebGLObject.h"

namespace Starfish {

class WebGLTexture : public WebGLObject {
public:
    WebGLTexture(ScriptBindingInstance* instance,
                 WebGLRenderingContext* context, GLuint object);
    void init(ScriptBindingInstance* instance, void* domObjectPointer) override;
    bool isWebGLTexture() const override;

    bool isImmutable() const
    {
        return m_isImmutable;
    }
    void setImmutable(bool immutable)
    {
        m_isImmutable = immutable;
    }

    GLsizei width() const
    {
        return m_width;
    }
    GLsizei height() const
    {
        return m_height;
    }
    void setSize(GLsizei w, GLsizei h)
    {
        m_width = w;
        m_height = h;
    }

    BEGIN_IMPLEMENT_NEW_WITH_GC_DESC(WebGLTexture, WebGLObject);
    END_IMPLEMENT_NEW_WITH_GC_DESC();

private:
    bool m_isImmutable = false;
    GLsizei m_width = 0;
    GLsizei m_height = 0;
};
} // namespace Starfish

#endif
#endif
