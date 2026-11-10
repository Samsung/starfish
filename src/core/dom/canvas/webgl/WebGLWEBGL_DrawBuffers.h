/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishWebGLWEBGL_DrawBuffers__
#define __StarfishWebGLWEBGL_DrawBuffers__

#if defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#include "binding/ScriptWrappable.h"
#include "core/util/GCDescriptor.h"
#include "platform/canvas/gl/GLTypes.h"
#include "StarfishBase.h" // GCAtomicVector

namespace Starfish {

class WebGLRenderingContext;

// References:
// binding/generated/WEBGL_draw_buffersBinding.cpp

class WEBGL_draw_buffers : public ScriptWrappable {
public:
    WEBGL_draw_buffers(ScriptBindingInstance* instance,
                       WebGLRenderingContext* context);
    void init(ScriptBindingInstance* instance, void* domObjectPointer) override;
    bool isWEBGL_draw_buffers() const override;
    ScriptBindingInstance* scriptBindingInstance() override
    {
        return m_scriptBindingInstance;
    }

    void drawBuffersWEBGL(GCAtomicVector<GLenum> buffers);

    BEGIN_IMPLEMENT_NEW_WITH_GC_DESC(WEBGL_draw_buffers);
    FILL_GC_POINTER(WEBGL_draw_buffers, m_object); // ScriptWrappable
    FILL_GC_POINTER(WEBGL_draw_buffers, m_scriptBindingInstance);
    FILL_GC_POINTER(WEBGL_draw_buffers, m_context);
    END_IMPLEMENT_NEW_WITH_GC_DESC();

private:
    ScriptBindingInstance* m_scriptBindingInstance;
    WebGLRenderingContext* m_context;
};

} // namespace Starfish

#endif

#endif
