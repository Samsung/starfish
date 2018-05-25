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

#ifndef __StarFishBorderRadiusData__
#define __StarFishBorderRadiusData__

namespace StarFish {

class BorderRadiusData : public gc {
public:
    Length m_topLeftHorizontal;
    Length m_topLeftVertical;
    Length m_topRightHorizontal;
    Length m_topRightVertical;
    Length m_bottomRightHorizontal;
    Length m_bottomRightVertical;
    Length m_bottomLeftHorizontal;
    Length m_bottomLeftVertical;

    BorderRadiusData()
        : m_topLeftHorizontal(Length::Fixed, 0)
        , m_topLeftVertical(Length::Fixed, 0)
        , m_topRightHorizontal(Length::Fixed, 0)
        , m_topRightVertical(Length::Fixed, 0)
        , m_bottomRightHorizontal(Length::Fixed, 0)
        , m_bottomRightVertical(Length::Fixed, 0)
        , m_bottomLeftHorizontal(Length::Fixed, 0)
        , m_bottomLeftVertical(Length::Fixed, 0)
    {
    }

    void* operator new(size_t size)
    {
        STARFISH_ASSERT(size == sizeof(BorderRadiusData));
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word obj_bitmap[GC_BITMAP_SIZE(BorderRadiusData)] = { 0 };
            GC_set_bit(obj_bitmap,
                       GC_WORD_OFFSET(BorderRadiusData, m_topLeftHorizontal));
            GC_set_bit(obj_bitmap,
                       GC_WORD_OFFSET(BorderRadiusData, m_topLeftVertical));
            GC_set_bit(obj_bitmap,
                       GC_WORD_OFFSET(BorderRadiusData, m_topRightHorizontal));
            GC_set_bit(obj_bitmap,
                       GC_WORD_OFFSET(BorderRadiusData, m_topRightVertical));
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(BorderRadiusData,
                                                  m_bottomRightHorizontal));
            GC_set_bit(obj_bitmap,
                       GC_WORD_OFFSET(BorderRadiusData, m_bottomRightVertical));
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(BorderRadiusData,
                                                  m_bottomLeftHorizontal));
            GC_set_bit(obj_bitmap,
                       GC_WORD_OFFSET(BorderRadiusData, m_bottomLeftVertical));
            descr =
                GC_make_descriptor(obj_bitmap, GC_WORD_LEN(BorderRadiusData));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }

    void* operator new[](size_t size) = delete;

    bool operator==(const BorderRadiusData& o)
    {
        return this->m_topLeftHorizontal == o.m_topLeftHorizontal &&
               this->m_topLeftVertical == o.m_topLeftVertical &&
               this->m_topRightHorizontal == o.m_topRightHorizontal &&
               this->m_topRightVertical == o.m_topRightVertical &&
               this->m_bottomRightHorizontal == o.m_bottomRightHorizontal &&
               this->m_bottomRightVertical == o.m_bottomRightVertical &&
               this->m_bottomLeftHorizontal == o.m_bottomLeftHorizontal &&
               this->m_bottomLeftVertical == o.m_bottomLeftVertical;
    }

    bool operator!=(const BorderRadiusData& o)
    {
        return !operator==(o);
    }
};

} /* namespace StarFish */

#endif /* __StarFishBorderRadiusData__ */
