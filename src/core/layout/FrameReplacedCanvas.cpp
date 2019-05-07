/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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
#include "binding/CanvasRenderingContext2DOrWebGLRenderingContextOrImageBitmapRenderingContextUnion.h"
#include "core/dom/canvas/HTMLCanvasElement.h"
#include "core/page/WebView.h"

namespace Starfish {

FrameReplacedCanvas::FrameReplacedCanvas(Node* node)
    : FrameReplaced(node, nullptr)
    , m_emptySurface(nullptr)
{
    computeStyleFlags();
    // This case is just that a empty element is defined.
    m_emptySurface =
        CanvasSurface::create(node->webView()->platformWindow(), 1, 1,
                              CanvasSurface::CanvasSurfaceFlag::CanvasElement);
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

void FrameReplacedCanvas::didCompsiteStackingContext(Compositor* c)
{
}

void FrameReplacedCanvas::willCompsiteStackingContext(Compositor* c)
{
    HTMLCanvasElement* canvasElement = node()->asHTMLCanvasElement();
    auto context = canvasElement->canvasRenderingContext();
    if (context) {
        context->flush();
        CanvasSurface* surface = context->surface();
        if (surface) {
            surface->unmapBufferAndNotifyUpdatedRegion(
                0, 0, surface->bufferWidth(), surface->bufferHeight());
        }
    }
}

void FrameReplacedCanvas::createGraphicsBuffer(CanvasSurface** surfaceHolder,
                                               size_t visibleWidth,
                                               size_t visibleHeight)
{
    HTMLCanvasElement* canvasElement = node()->asHTMLCanvasElement();
    if (canvasElement->canvasRenderingContext() &&
        canvasElement->canvasRenderingContext()->surface()) {
        *surfaceHolder = canvasElement->canvasRenderingContext()->surface();
    } else {
        *surfaceHolder = m_emptySurface;
    }
}
}
#endif
