/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLElement.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/layout/FrameDocument.h"
#include "core/layout/StackingContext.h"
#include "core/modules/canvas/Canvas.h"
#include "core/page/Window.h"

namespace StarFish {

void* FrameDocument::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FrameDocument));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(FrameDocument)] = { 0 };
        FrameDocument::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(FrameDocument));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void FrameDocument::layout(LayoutContext& ctx,
                           Frame::LayoutWantToResolve resolveWhat)
{
    int w = node()->asDocument()->window()->innerWidth();
    int h = node()->asDocument()->window()->innerHeight();
    int ow = 0, oh = 0;
    if (style()->width().isFixed()) {
        ow = style()->width().fixed();
    }
    if (style()->height().isFixed()) {
        oh = style()->height().fixed();
    }

    style()->setWidth(Length(Length::Fixed, w));
    style()->setHeight(Length(Length::Fixed, h));

    if (firstChild()) {
        STARFISH_ASSERT(firstChild() == lastChild());
        STARFISH_ASSERT(firstChild()->isRootElement());

        style()->setDirection(firstChild()->style()->direction());
        if (ow != w) {
            ctx.markViewportWidthDamaged();
        }
        if (oh != h) {
            ctx.markViewportHeightDamaged();
        }
        FrameBlockBox::layout(ctx, Frame::LayoutWantToResolve::ResolveAll);
    }
}

bool FrameDocument::scrollTo(LayoutUnit left, LayoutUnit top)
{
    bool isEffective = false;
    if (left > (scrollWidth() - width())) {
        left = scrollWidth() - width();
    }

    if (left < 0) {
        left = 0;
    }

    if (m_scrollLeft != left) {
        isEffective = true;
    }
    m_scrollLeft = left;

    if (top > (scrollHeight() - height())) {
        top = scrollHeight() - height();
    }

    if (top < 0) {
        top = 0;
    }

    if (m_scrollTop != top) {
        isEffective = true;
    }
    m_scrollTop = top;

    return isEffective;
}

void FrameDocument::paintContent(PaintingContext& ctx)
{
    STARFISH_ASSERT(ctx.m_paintingStage == PaintingStageEnd);
    STARFISH_ASSERT(firstChild() == lastChild());
    if (!firstChild()) {
        return;
    }
}

Frame* FrameDocument::hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage)
{
    STARFISH_ASSERT(stage == HitTestStageEnd);
    STARFISH_ASSERT(firstChild() == lastChild());
    if (!firstChild() || !firstChild()->asFrameBox()->stackingContext()) {
        return nullptr;
    }

    Frame* result =
        firstChild()->asFrameBox()->stackingContext()->hitTestStackingContext(
            x, y, node()->asDocument()->browsingContext());
    if (result) {
        return result;
    }
    return this;
}
}
