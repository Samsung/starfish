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

#ifndef __StarFishOutlineData__
#define __StarFishOutlineData__

#include "core/style/Style.h"

namespace StarFish {

class ComputedStyle;

class OutlineData : public gc {
public:
    OutlineData()
        : m_offset(Length::Fixed, 0)
    {
    }

    BorderValue& border()
    {
        return m_border;
    }

    void setOffset(Length offset)
    {
        m_offset = offset;
    }

    Length offset()
    {
        return m_offset;
    }

    void checkComputed(Length curFontSize, Length rootFontSize, Font* font,
                       LayoutSize windowSize, ComputedStyle* cs)
    {
        m_border.checkComputed(curFontSize, rootFontSize, font, windowSize, cs);
        m_offset.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                       windowSize.width(), windowSize.height(),
                                       cs);
    }

    void* operator new(size_t size)
    {
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word obj_bitmap[GC_BITMAP_SIZE(OutlineData)] = { 0 };
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(OutlineData, m_border));
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(OutlineData, m_offset));
            descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(OutlineData));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new[](size_t size) = delete;

private:
    BorderValue m_border;
    Length m_offset;
};

} /* namespace StarFish */

#endif /* __StarFishOutlineData__ */
