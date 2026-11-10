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

#if defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#include "StarfishConfig.h"
#include "WebGLWEBGL_DrawBuffers.h"
#include "core/dom/canvas/webgl/WebGLRenderingContext.h"

namespace Starfish {

WEBGL_draw_buffers::WEBGL_draw_buffers(ScriptBindingInstance* instance,
                                       WebGLRenderingContext* context)
    : ScriptWrappable(this)
    , m_scriptBindingInstance(instance)
    , m_context(context)
{
}

// drawBuffersWEBGL is WebGL 2's drawBuffers under another name: the
// validation and the GL call are shared with the context implementation.
void WEBGL_draw_buffers::drawBuffersWEBGL(GCAtomicVector<GLenum> buffers)
{
    m_context->drawBuffers(buffers);
}

} // namespace Starfish

#endif
