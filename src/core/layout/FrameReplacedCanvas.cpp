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
#include "core/dom/Node.h"
#include "core/layout/FrameReplaced.h"
#include "core/modules/canvas/Canvas.h"
#include "core/dom/Document.h"
#include "core/layout/FrameReplacedCanvas.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"
#include "core/dom/canvas/CanvasRenderingContext.h"
#include "binding/generated/CanvasRenderingContext2DOrWebGLRenderingContextOrWebGL2RenderingContextOrImageBitmapRenderingContextUnion.h"
#include "core/dom/canvas/HTMLCanvasElement.h"
#include "core/page/WebView.h"

namespace Starfish {

FrameReplacedCanvas::FrameReplacedCanvas(Node* node)
    : FrameReplaced(node, nullptr)
    , m_emptySurface(nullptr)
{
    computeStyleFlags();
    // This case is just that a empty element is defined.
    m_emptySurface = CanvasSurface::create(
        node->webView()->renderer(), 1, 1, 1,
        static_cast<CanvasSurface::CanvasSurfaceFlag>(
            CanvasSurface::PreferEGLImage |
            CanvasSurface::PreferRetainCPUBufferWhenUnmap));
}

IntrinsicSize FrameReplacedCanvas::intrinsicSize()
{
    IntrinsicSize result;
    result.m_isContentExists = true;
    result.m_hasAspectRatio = true;
    auto canvas = node()->asHTMLCanvasElement();
    double canvasWidth = canvas->width();
    double canvasHeight = canvas->height();
    result.m_intrinsicContentSize = LayoutSize(canvasWidth, canvasHeight);
    return result;
}

void FrameReplacedCanvas::didCompositeStackingContext(Compositor* c)
{
}

void FrameReplacedCanvas::willCompositeStackingContext(Compositor* c)
{
    HTMLCanvasElement* canvasElement = node()->asHTMLCanvasElement();
    auto context = canvasElement->canvasRenderingContext();
    if (context) {
        context->flushInRendering();
    }
}

Optional<CanvasSurface*> FrameReplacedCanvas::contentSurface()
{
    HTMLCanvasElement* canvasElement = node()->asHTMLCanvasElement();
    if (canvasElement->canvasRenderingContext() &&
        canvasElement->canvasRenderingContext()->surface()) {
        return canvasElement->canvasRenderingContext()->surface();
    } else {
        return m_emptySurface;
    }
}

} // namespace Starfish
#endif
