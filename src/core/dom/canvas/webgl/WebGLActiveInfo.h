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

#ifndef __StarfishWebGLActiveInfo__
#define __StarfishWebGLActiveInfo__

#if defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#include "binding/ScriptWrappable.h"
#include "platform/canvas/gl/GLTypes.h"
#include "core/util/GCDescriptor.h"

namespace Starfish {

class ScriptBindingInstance;
class String;

class WebGLActiveInfo : public ScriptWrappable {
public:
    WebGLActiveInfo(ScriptBindingInstance* instance, GLint size, GLenum type,
                    String* name)
        : ScriptWrappable(this)
        , m_scriptBindingInstance(instance)
        , m_size(size)
        , m_type(type)
        , m_name(name)
    {
    }
    void init(ScriptBindingInstance* instance, void* domObjectPointer) override;
    bool isWebGLActiveInfo() const override;
    ScriptBindingInstance* scriptBindingInstance() override
    {
        return m_scriptBindingInstance;
    }

    DEFINE_GETTER(GLint, size);
    DEFINE_GETTER(GLenum, type);
    DEFINE_GETTER(String*, name);

    BEGIN_IMPLEMENT_NEW_WITH_GC_DESC(WebGLActiveInfo);
    FILL_GC_POINTER(WebGLActiveInfo, m_name);
    END_IMPLEMENT_NEW_WITH_GC_DESC();

private:
    ScriptBindingInstance* m_scriptBindingInstance;
    GLint m_size;
    GLenum m_type;
    String* m_name;
};
} // namespace Starfish

#endif
#endif
