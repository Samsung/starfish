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
