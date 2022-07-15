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

#include "StarfishConfig.h"
#include "core/style/Style.h"
#include "core/style/ComputedStyle.h"
#include "core/dom/Node.h"
#include "core/dom/svg/SVGTextElement.h"
#include "FrameSVGTextBox.h"
#include "core/dom/Document.h"
#include "core/layout/FrameBlockBox.h"

namespace Starfish {

void* FrameSVGTextBox::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FrameSVGTextBox));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(FrameSVGTextBox)] = { 0 };
        FrameSVGTextBox::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(FrameSVGTextBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void FrameSVGTextBox::layoutSVG()
{
    LayoutContext ctx(node()->starfish(),
                      node()->document()->frame()->asFrameDocument());
    firstChild()->layout(ctx, Frame::LayoutWantToResolve::ResolveAll);
    firstChild()->establishesStackingContextIfNeedsAndComputingPaintingFlags();
}

void FrameSVGTextBox::paintSVG(PaintingContext& ctx)
{
    PaintingContext newCtx(ctx.m_canvas);
    newCtx.m_paintingStage = PaintingNormalFlowInline;
    newCtx.m_canvas->save();

    if (node() && node()->isSVGTextElement() &&
        !firstChild()->asFrameBlockBox()->lineBoxes().empty()) {
        SVGTextElement* textElement = (SVGTextElement*)node();

        // text-anchor
        float textBoxWidth = firstChild()
                                 ->asFrameBlockBox()
                                 ->lineBoxes()[0]
                                 ->firstInlineBox()
                                 ->width()
                                 .toFloat();
        if (textElement->textAnchor() == SVGTextElement::TextAnchor::MIDDLE) {
            newCtx.m_canvas->translate(-textBoxWidth / 2, 0);
        } else if (textElement->textAnchor() ==
                   SVGTextElement::TextAnchor::END) {
            newCtx.m_canvas->translate(-textBoxWidth, 0);
        }

        // alignment-baseline
        if (textElement->alignmentBaseline() ==
            SVGTextElement::AlignmentBaseline::MIDDLE) {
            newCtx.m_canvas->translate(
                0, -(float)style()->font()->metrics().m_fontHeight / 2);
            newCtx.m_canvas->translate(
                0, -((float)style()->font()->metrics().m_xheightRate *
                     style()->font()->size()) /
                       2);
        } else {
            newCtx.m_canvas->translate(
                0, -(float)style()->font()->metrics().m_ascender);
        }
    } else if (node() && node()->isSVGTSpanElement()) {
        newCtx.m_canvas->translate(
            0, -(float)style()->font()->metrics().m_ascender);
    }
    firstChild()->asFrameBlockBox()->paintContent(newCtx);
    newCtx.m_canvas->restore();
}
} // namespace Starfish
