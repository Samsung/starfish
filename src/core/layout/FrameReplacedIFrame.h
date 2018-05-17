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

#ifndef __StarFishFrameReplacedIFrame__
#define __StarFishFrameReplacedIFrame__

#include "core/layout/FrameReplaced.h"

namespace StarFish {

class FrameReplacedIFrame : public FrameReplaced {
public:
    FrameReplacedIFrame(Node* node)
        : FrameReplaced(node, nullptr)
    {
        computeStyleFlags();
    }

    virtual void computeStyleFlags() override
    {
        FrameReplaced::computeStyleFlags();
        m_flags.m_isEstablishesStackingContext = true;
    }

    virtual bool isFrameReplacedIFrame() override
    {
        return true;
    }

    virtual const char* name() override
    {
        return "FrameReplacedIFrame";
    }

#ifdef STARFISH_ENABLE_TEST
    virtual void dump(int depth) override;
#endif

    virtual void layout(LayoutContext& ctx,
                        Frame::LayoutWantToResolve resolveWhat) override;

    virtual Frame* hitTest(LayoutUnit x, LayoutUnit y,
                           HitTestStage stage) override;
    virtual Frame* hitTestChildrenWith(LayoutUnit x, LayoutUnit y,
                                       HitTestStage stage) override
    {
        return hitTest(x, y, stage);
    }

    virtual IntrinsicSize intrinsicSize() override;

    virtual void computeVisibleRect(
        FrameBox::ComputeVisibleRectContext& ctx) override;

    virtual void establishesStackingContextIfNeeds() override;

    void* operator new(size_t size)
    {
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word obj_bitmap[GC_BITMAP_SIZE(FrameReplacedIFrame)] = { 0 };
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameReplacedIFrame, m_node));
            GC_set_bit(obj_bitmap,
                       GC_WORD_OFFSET(FrameReplacedIFrame, m_layoutParent));
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameReplacedIFrame,
                                                  m_treeItemModel.m_parent));
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameReplacedIFrame,
                                                  m_treeItemModel.m_previous));
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameReplacedIFrame,
                                                  m_treeItemModel.m_next));
            GC_set_bit(obj_bitmap,
                       GC_WORD_OFFSET(FrameReplacedIFrame,
                                      m_treeItemModel.m_firstChild));
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameReplacedIFrame,
                                                  m_treeItemModel.m_lastChild));
            descr = GC_make_descriptor(obj_bitmap,
                                       GC_WORD_LEN(FrameReplacedIFrame));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new[](size_t size) = delete;

protected:
};
}

#endif
