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
