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

#ifndef __StarFishFrameReplacedVideo__
#define __StarFishFrameReplacedVideo__

#include "core/layout/FrameReplaced.h"

namespace StarFish {

class FrameReplacedVideo : public FrameReplaced {
public:
    FrameReplacedVideo(Node* node)
        : FrameReplaced(node, nullptr)
    {
        computeStyleFlags();
    }

    virtual void computeStyleFlags()
    {
        FrameReplaced::computeStyleFlags();
        m_flags.m_isEstablishesStackingContext = true;
        m_flags.m_needsGraphicsBuffer = true;
    }

    virtual bool isFrameReplacedVideo()
    {
        return true;
    }

    virtual const char* name()
    {
        return "FrameReplacedVideo";
    }

    virtual IntrinsicSize intrinsicSize();
    virtual void didCompsiteStackingContext(Compositor* c);
    virtual void createGraphicsBuffer(CanvasSurface** surfaceHolder,
                                      size_t visibleWidth,
                                      size_t visibleHeight);

    void* operator new(size_t size)
    {
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word obj_bitmap[GC_BITMAP_SIZE(FrameReplacedVideo)] = { 0 };
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameReplacedVideo, m_node));
            GC_set_bit(obj_bitmap,
                       GC_WORD_OFFSET(FrameReplacedVideo, m_layoutParent));
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameReplacedVideo,
                                                  m_treeItemModel.m_parent));
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameReplacedVideo,
                                                  m_treeItemModel.m_previous));
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameReplacedVideo,
                                                  m_treeItemModel.m_next));
            GC_set_bit(obj_bitmap,
                       GC_WORD_OFFSET(FrameReplacedVideo,
                                      m_treeItemModel.m_firstChild));
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameReplacedVideo,
                                                  m_treeItemModel.m_lastChild));
            descr =
                GC_make_descriptor(obj_bitmap, GC_WORD_LEN(FrameReplacedVideo));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new[](size_t size) = delete;

protected:
};
}

#endif
