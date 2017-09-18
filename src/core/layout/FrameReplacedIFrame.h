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

    virtual void computeStyleFlags()
    {
        FrameReplaced::computeStyleFlags();
    }

    virtual bool isFrameReplacedIFrame()
    {
        return true;
    }

    virtual const char* name()
    {
        return "FrameReplacedIFrame";
    }

#ifdef STARFISH_ENABLE_TEST
    virtual void dump(int depth) override;
#endif

    virtual void layout(LayoutContext& ctx,
                        Frame::LayoutWantToResolve resolveWhat) override;

    virtual Frame* hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage)
    {
        if (x >= 0 && x < m_frameRect.width() && y >= 0 &&
            y < m_frameRect.height()) {
            return this;
        }
        return nullptr;
    }

    virtual Frame* hitTestChildrenWith(LayoutUnit x, LayoutUnit y,
                                       HitTestStage stage)
    {
        return this;
    }

    virtual IntrinsicSize intrinsicSize();

    virtual void willCompsiteStackingContext(Canvas* c)
    {
    }

    virtual void didCompsiteStackingContext(Canvas* c)
    {
    }

    virtual void compsitingStackingContext(Canvas* c)
    {
    }

    virtual void establishesStackingContextIfNeeds();

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
