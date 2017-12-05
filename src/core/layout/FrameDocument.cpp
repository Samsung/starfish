/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
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
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(FrameDocument)] = { 0 };
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameDocument, m_node));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameDocument, m_layoutParent));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameDocument, m_treeItemModel.m_parent));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameDocument, m_treeItemModel.m_previous));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameDocument, m_treeItemModel.m_next));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameDocument, m_treeItemModel.m_firstChild));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameDocument, m_treeItemModel.m_lastChild));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameDocument, m_lineBoxes));
        descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(FrameDocument));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void FrameDocument::layout(LayoutContext& ctx,
                           Frame::LayoutWantToResolve resolveWhat)
{
    int w = node()->asDocument()->window()->width();
    int h = node()->asDocument()->window()->height();

    style()->setWidth(Length(Length::Fixed, w));
    style()->setHeight(Length(Length::Fixed, h));

    if (firstChild()) {
        STARFISH_ASSERT(firstChild() == lastChild());
        STARFISH_ASSERT(firstChild()->isRootElement());

        style()->setDirection(firstChild()->style()->direction());
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
    ctx.m_canvas->save();
    ctx.m_canvas->translate(firstChild()->asFrameBox()->x(),
                            firstChild()->asFrameBox()->y());
    firstChild()->asFrameBox()->stackingContext()->paintStackingContext(
        ctx.m_canvas, false);
    ctx.m_canvas->restore();
}

Frame* FrameDocument::hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage)
{
    STARFISH_ASSERT(stage == HitTestStageEnd);
    STARFISH_ASSERT(firstChild() == lastChild());
    if (!firstChild()) {
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
