/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#include "core/dom/Node.h"
#include "core/layout/FrameTableCaptionBox.h"

namespace StarFish {

FrameTableCaptionBox::FrameTableCaptionBox(Node* node, ComputedStyle* style)
    : FrameTableObjectBox(node, style)
{
}

void FrameTableCaptionBox::layoutWidth(LayoutContext& ctx)
{
    FrameBox* cb = containingBlock(this);
    LayoutUnit parentContentWidth = cb->contentWidth();
    computeBorderMarginPadding(ctx, parentContentWidth);
    LayoutUnit contentWidth;
    Length width = style()->width();
    if (width.isAuto()) {
        PreferredWidthMainContext mainContext;
        PreferredWidthContext p(ctx, mainContext, this, this,
                                parentContentWidth - mbpWidth());
        p.computePreferredWidth();
        contentWidth = p.preferredWidth();
    } else {
        contentWidth = width.specifiedValue(parentContentWidth, this);
        contentWidth = contentWidthApplyingBoxSizing(contentWidth);
    }

    applyMinMaxWidthIfNeeds(ctx, contentWidth, parentContentWidth);
}

void* FrameTableCaptionBox::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(FrameTableCaptionBox)] = { 0 };
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableCaptionBox, m_node));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameTableCaptionBox, m_layoutParent));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableCaptionBox,
                                              m_treeItemModel.m_parent));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableCaptionBox,
                                              m_treeItemModel.m_previous));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableCaptionBox,
                                              m_treeItemModel.m_next));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableCaptionBox,
                                              m_treeItemModel.m_firstChild));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableCaptionBox,
                                              m_treeItemModel.m_lastChild));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameTableCaptionBox, m_lineBoxes));
        descr =
            GC_make_descriptor(obj_bitmap, GC_WORD_LEN(FrameTableCaptionBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}
}
