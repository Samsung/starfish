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

#ifndef __StarFishStyleSurroundData__
#define __StarFishStyleSurroundData__

#include "core/style/BorderData.h"
#include "core/style/LengthData.h"

namespace StarFish {

class StyleSurroundData : public gc {
public:
    StyleSurroundData()
        : m_margin(Length(Length::Fixed, 0))
        , m_padding(Length(Length::Fixed, 0))
        , m_offset(Length())
    {
    }

    virtual ~StyleSurroundData()
    {
    }

    bool operator==(const StyleSurroundData& o)
    {
        return m_border == o.m_border && m_margin == o.m_margin &&
               m_padding == o.m_padding && m_offset == o.m_offset;
    }

    bool operator!=(const StyleSurroundData& o)
    {
        return !operator==(o);
    }

    void* operator new(size_t size)
    {
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word obj_bitmap[GC_BITMAP_SIZE(StyleSurroundData)] = { 0 };
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(StyleSurroundData, m_border));
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(StyleSurroundData, m_margin));
            GC_set_bit(obj_bitmap,
                       GC_WORD_OFFSET(StyleSurroundData, m_padding));
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(StyleSurroundData, m_offset));
            descr =
                GC_make_descriptor(obj_bitmap, GC_WORD_LEN(StyleSurroundData));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new[](size_t size) = delete;

    BorderData m_border;
    LengthData m_margin;
    LengthData m_padding;
    LengthData m_offset;
};

} /* namespace StarFish */

#endif /* __StarFishStyleSurroundData__ */
