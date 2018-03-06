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

#ifndef __StarFishFrameReplacedImage__
#define __StarFishFrameReplacedImage__

#include "core/layout/FrameReplaced.h"

namespace StarFish {

class FrameReplacedImage : public FrameReplaced {
public:
    FrameReplacedImage(Node* node)
        : FrameReplaced(node, nullptr)
    {
    }

    virtual bool isFrameReplacedImage()
    {
        return true;
    }

    virtual const char* name()
    {
        return "FrameReplacedImage";
    }

    virtual void paintReplaced(Canvas* canvas);
    virtual IntrinsicSize intrinsicSize();

    void* operator new(size_t size)
    {
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word obj_bitmap[GC_BITMAP_SIZE(FrameReplacedImage)] = { 0 };
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameReplacedImage, m_node));
            GC_set_bit(obj_bitmap,
                       GC_WORD_OFFSET(FrameReplacedImage, m_layoutParent));
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameReplacedImage,
                                                  m_treeItemModel.m_parent));
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameReplacedImage,
                                                  m_treeItemModel.m_previous));
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameReplacedImage,
                                                  m_treeItemModel.m_next));
            GC_set_bit(obj_bitmap,
                       GC_WORD_OFFSET(FrameReplacedImage,
                                      m_treeItemModel.m_firstChild));
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameReplacedImage,
                                                  m_treeItemModel.m_lastChild));
            descr =
                GC_make_descriptor(obj_bitmap, GC_WORD_LEN(FrameReplacedImage));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new[](size_t size) = delete;
};
}

#endif
