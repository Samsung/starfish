/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
