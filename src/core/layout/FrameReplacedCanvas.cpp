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

#include "StarFishConfig.h"
#include "core/dom/Node.h"
#include "core/layout/FrameReplacedCanvas.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"
#include "core/dom/canvas/HTMLCanvasElement.h"

namespace StarFish {

IntrinsicSize FrameReplacedCanvas::intrinsicSize()
{
    IntrinsicSize result;
    return result;
}

void FrameReplacedCanvas::paintContent(PaintingContext& ctx)
{
    FrameReplaced::paintContent(ctx);

    Unit::Rect frameRect =
        Unit::Rect(borderLeft() + paddingLeft(), borderTop() + paddingTop(),
                   width() - borderWidth() - paddingWidth(),
                   height() - borderHeight() - paddingHeight());
    if (!m_canvasImage) {
        m_canvasImage =
            NativeImageData::create(frameRect.width(), frameRect.height());
    }

    if (m_canvasImage->width() != frameRect.width() &&
        m_canvasImage->height() != frameRect.height()) {
        m_canvasImage =
            NativeImageData::create(frameRect.width(), frameRect.height());
    }

    Canvas* canvas = ctx.m_canvas;
    canvas->save();

    Canvas* imageCanvas =
        Canvas::createGenericCanvas(node()->starFish(), m_canvasImage);

    // Run the command buffer.
    Node* n = this->node();
    if (n && n->isHTMLCanvasElement()) {
        HTMLCanvasElement* e = n->asHTMLCanvasElement();
        e->commandBuffer().paintCommands(imageCanvas);
    }

    canvas->drawImage(m_canvasImage, frameRect);

    delete imageCanvas;

    canvas->restore();
}
}
#endif
