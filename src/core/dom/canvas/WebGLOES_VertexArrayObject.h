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

#ifndef __StarfishWebGLOES_VertexArrayObject__
#define __StarfishWebGLOES_VertexArrayObject__

#if defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#include "binding/ScriptWrappable.h"
#include "platform/canvas/webgl/GLESTypes.h"
#include "core/dom/canvas/WebGLObject.h"

namespace Starfish {

class WebGLRenderingContext;

class WebGLVertexArrayObjectOES : public WebGLObject {
public:
    WebGLVertexArrayObjectOES(ScriptBindingInstance* instance,
                              WebGLRenderingContext* context, GLuint object)
        : WebGLObject(instance, context, object)
    {
    }
    void init(ScriptBindingInstance* instance, void* domObjectPointer) override;
    bool isWebGLVertexArrayObjectOES() const override;
    bool hasEverBound()
    {
        return m_hasEverBound;
    }
    void setHasEverBound()
    {
        m_hasEverBound = true;
    }

private:
    bool m_hasEverBound = false;
};

// References:
// binding/generated/OES_vertex_array_objectBinding.cpp

class OES_vertex_array_object : public ScriptWrappable {
public:
    OES_vertex_array_object(ScriptBindingInstance* instance,
                            WebGLRenderingContext* context);
    void init(ScriptBindingInstance* instance, void* domObjectPointer) override;
    bool isOES_vertex_array_object() const override;
    ScriptBindingInstance* scriptBindingInstance() override
    {
        return m_scriptBindingInstance;
    }

    WebGLVertexArrayObjectOES* createVertexArrayOES();
    void deleteVertexArrayOES(Nullable<WebGLVertexArrayObjectOES*> arrayObject);
    GLboolean isVertexArrayOES(
        Nullable<WebGLVertexArrayObjectOES*> arrayObject);
    void bindVertexArrayOES(Nullable<WebGLVertexArrayObjectOES*> arrayObject);

private:
    ScriptBindingInstance* m_scriptBindingInstance;
    WebGLRenderingContext* m_context;
};
} // namespace Starfish

#endif
#endif
