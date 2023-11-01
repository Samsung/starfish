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

#ifndef __StarfishWebGLObject__
#define __StarfishWebGLObject__

#if defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#include "binding/ScriptWrappable.h"
#include "platform/canvas/webgl/GLESTypes.h"

namespace Starfish {

class WebGLObject : public ScriptWrappable {
public:
    WebGLObject(ScriptBindingInstance* instance, GLuint object);
    void init(ScriptBindingInstance* instance, void* data) override;
    bool isWebGLObject() const override;
    ScriptBindingInstance* scriptBindingInstance() override
    {
        return m_scriptBindingInstance;
    }

    GLuint glObject() const
    {
        return m_glObject;
    }

private:
    ScriptBindingInstance* m_scriptBindingInstance;
    GLuint m_glObject;
};
} // namespace Starfish

#endif
#endif
