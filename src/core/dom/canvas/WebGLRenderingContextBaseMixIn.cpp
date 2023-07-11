/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifdef STARFISH_ENABLE_CANVAS

#include "StarfishConfig.h"
#include "WebGLRenderingContextBaseMixIn.h"
#include "core/modules/canvas/Canvas.h"
#include "core/dom/canvas/HTMLCanvasElement.h"

namespace Starfish {

WebGLRenderingContextBaseMixIn::WebGLRenderingContextBaseMixIn(
    HTMLCanvasElement* ownerHTMLCanvasElement)
    : CanvasRenderingContext(ownerHTMLCanvasElement->executionContext())
    , m_ownerHTMLCanvasElement(ownerHTMLCanvasElement)
{
    initialize();
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            WebGLRenderingContextBaseMixIn* c =
                (WebGLRenderingContextBaseMixIn*)obj;
            c->finalize();
        },
        NULL, NULL, NULL);
}

void WebGLRenderingContextBaseMixIn::initialize()
{
    STARFISH_UNIMPLEMENTED();
}

void WebGLRenderingContextBaseMixIn::finalize()
{
    STARFISH_UNIMPLEMENTED();
}

void WebGLRenderingContextBaseMixIn::flush()
{
    STARFISH_UNIMPLEMENTED();
}

void WebGLRenderingContextBaseMixIn::onResize()
{
    STARFISH_UNIMPLEMENTED();
    finalize();
    initialize();
    m_ownerHTMLCanvasElement->setNeedsComposite();
}

CanvasSurface* WebGLRenderingContextBaseMixIn::surface()
{
    STARFISH_UNIMPLEMENTED();
    return nullptr;
}

} // namespace Starfish
#endif
