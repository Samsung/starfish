/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishWebGLFramebuffer__
#define __StarfishWebGLFramebuffer__

#if defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#include "core/dom/canvas/webgl/WebGLObject.h"

namespace Starfish {

class WebGLTexture;
class WebGLRenderbuffer;

class WebGLFramebuffer : public WebGLObject {
public:
    WebGLFramebuffer(ScriptBindingInstance* instance,
                     WebGLRenderingContext* context, GLuint object)
        : WebGLObject(instance, context, object)
    {
    }
    void init(ScriptBindingInstance* instance, void* domObjectPointer) override;
    bool isWebGLFramebuffer() const override;

    DEFINE_GETTER_SETTER(WebGLTexture*, attachedTexture, AttachedTexture);
    DEFINE_GETTER_SETTER(WebGLRenderbuffer*, attachedRenderBuffer,
                         AttachedRenderBuffer);

private:
    WebGLTexture* m_attachedTexture = nullptr;
    WebGLRenderbuffer* m_attachedRenderBuffer = nullptr;
};
} // namespace Starfish

#endif
#endif
