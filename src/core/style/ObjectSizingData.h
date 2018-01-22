/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishObjectSizingData__
#define __StarFishObjectSizingData__

#include "Style.h"

namespace StarFish {

class ObjectSizingData : public gc {
public:
    ObjectSizingData()
        : m_objectFit(ObjectFitValue::FillObjectFitValue)
        , m_offsetX(Length(Length::Percent, 0.5f))
        , m_offsetY(Length(Length::Percent, 0.5f))
    {
    }

    bool operator==(const ObjectSizingData& o)
    {
        return (this->m_objectFit == o.m_objectFit &&
                (this->m_offsetX == o.m_offsetX) &&
                (this->m_offsetY == o.m_offsetY));
    }

    bool operator!=(const ObjectSizingData& o)
    {
        return !operator==(o);
    }

    void* operator new(size_t size)
    {
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word obj_bitmap[GC_BITMAP_SIZE(ObjectSizingData)] = { 0 };
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(ObjectSizingData, m_offsetX));
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(ObjectSizingData, m_offsetY));
            descr =
                GC_make_descriptor(obj_bitmap, GC_WORD_LEN(ObjectSizingData));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new[](size_t size) = delete;

    ObjectFitValue objectFit()
    {
        return m_objectFit;
    }

    Length offsetX()
    {
        return m_offsetX;
    }

    Length offsetY()
    {
        return m_offsetY;
    }

    void setObjectFit(ObjectFitValue v)
    {
        m_objectFit = v;
    }

    void setObjectPosition(Length x, Length y)
    {
        m_offsetX = x;
        m_offsetY = y;
    }

private:
    ObjectFitValue m_objectFit;
    Length m_offsetX;
    Length m_offsetY;
};
}

#endif
