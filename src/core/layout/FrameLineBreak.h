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

#ifndef __StarFishFrameLineBreak__
#define __StarFishFrameLineBreak__

#include "core/layout/Frame.h"

namespace StarFish {

class FrameLineBreak : public Frame {
public:
    FrameLineBreak(Node* node)
        : Frame(node, nullptr)
    {
    }

    virtual bool isFrameLineBreak()
    {
        return true;
    }

    virtual const char* name()
    {
        return "FrameLineBreak";
    }

    virtual void computePreferredWidth(PreferredWidthContext& ctx);
    virtual void layoutInline(LineFormattingContext& ctx);

#ifdef STARFISH_ENABLE_TEST
    virtual void dump(int depth)
    {
        Frame::dump(depth);
    }
#endif

    void* operator new(size_t size)
    {
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word obj_bitmap[GC_BITMAP_SIZE(FrameLineBreak)] = { 0 };
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameLineBreak, m_node));
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameLineBreak,
                                                  m_treeItemModel.m_parent));
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameLineBreak,
                                                  m_treeItemModel.m_previous));
            GC_set_bit(obj_bitmap,
                       GC_WORD_OFFSET(FrameLineBreak, m_treeItemModel.m_next));
            GC_set_bit(
                obj_bitmap,
                GC_WORD_OFFSET(FrameLineBreak, m_treeItemModel.m_firstChild));
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameLineBreak,
                                                  m_treeItemModel.m_lastChild));
            descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(FrameLineBreak));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new[](size_t size) = delete;

protected:
    virtual bool hasFrameTreeItemModel()
    {
        return true;
    }

    virtual FrameTreeItemModel* frameTreeItemModel()
    {
        return &m_treeItemModel;
    }

    FrameTreeItemModel m_treeItemModel;
};
}

#endif
