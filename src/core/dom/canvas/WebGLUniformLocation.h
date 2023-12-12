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

#ifndef __StarfishWebGLUniformLocation__
#define __StarfishWebGLUniformLocation__

#if defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#include "binding/ScriptWrappable.h"
#include "platform/canvas/webgl/GLESTypes.h"

namespace Starfish {
class WebGLProgram;

class WebGLUniformLocation : public ScriptWrappable {
public:
    WebGLUniformLocation(ScriptBindingInstance* instance, WebGLProgram* program,
                         GLint location)
        : ScriptWrappable(this)
        , m_scriptBindingInstance(instance)
        , m_program(program)
        , m_location(location)
    {
    }

    void init(ScriptBindingInstance* instance, void* domObjectPointer) override;
    bool isWebGLUniformLocation() const override;
    ScriptBindingInstance* scriptBindingInstance() override
    {
        return m_scriptBindingInstance;
    }

    DEFINE_GETTER(WebGLProgram*, program);
    DEFINE_GETTER(GLint, location);

private:
    ScriptBindingInstance* m_scriptBindingInstance;
    WebGLProgram* m_program;
    GLint m_location;
};
} // namespace Starfish

#endif
#endif
