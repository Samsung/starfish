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

#ifndef __StarFishStyleTransformOrigin__
#define __StarFishStyleTransformOrigin__

#include "core/style/Style.h"

namespace StarFish {

class TransformOriginData : public gc {
public:
    TransformOriginData()
        : m_xaxis(Length(Length::Percent, 0.5f))
        , m_yaxis(Length(Length::Percent, 0.5f))
        , m_zaxis(Length())
    {
    }

    TransformOriginData(Length x, Length y, Length z)
        : m_xaxis(x)
        , m_yaxis(y)
        , m_zaxis(z)
    {
    }

    ~TransformOriginData()
    {
    }

    void* operator new(size_t size)
    {
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word obj_bitmap[GC_BITMAP_SIZE(TransformOriginData)] = { 0 };
            GC_set_bit(obj_bitmap,
                       GC_WORD_OFFSET(TransformOriginData, m_xaxis));
            GC_set_bit(obj_bitmap,
                       GC_WORD_OFFSET(TransformOriginData, m_yaxis));
            GC_set_bit(obj_bitmap,
                       GC_WORD_OFFSET(TransformOriginData, m_zaxis));
            descr = GC_make_descriptor(obj_bitmap,
                                       GC_WORD_LEN(TransformOriginData));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new[](size_t size) = delete;

    void setData(TransformOriginData* data)
    {
        m_xaxis = data->m_xaxis;
        m_yaxis = data->m_yaxis;
        m_zaxis = data->m_zaxis;
    }

    void setData(Length a, Length b, Length c)
    {
        m_xaxis = a;
        m_yaxis = b;
        m_zaxis = c;
    }

    Length getXAxis()
    {
        return m_xaxis;
    }

    Length getYAxis()
    {
        return m_yaxis;
    }

    Length getZAxis()
    {
        return m_zaxis;
    }

    TransformOriginData* getData()
    {
        return this;
    }

private:
    Length m_xaxis, m_yaxis, m_zaxis;
};

class StyleTransformOrigin : public gc {
public:
    StyleTransformOrigin()
        : m_originValue(NULL)
    {
    }

    ~StyleTransformOrigin()
    {
    }

    void setOrigin(StyleTransformOrigin* origin)
    {
        if (!m_originValue) {
            m_originValue = new TransformOriginData();
        }
        m_originValue->setData(origin->originValue());
    }

    void setOriginValue(Length x, Length y, Length z)
    {
        if (!m_originValue) {
            m_originValue = new TransformOriginData(x, y, z);
        }
        m_originValue->setData(x, y, z);
    }

    TransformOriginData* originValue()
    {
        return m_originValue;
    }

    String* dumpString()
    {
        StringBuilder builder;
        builder.appendChar('(');
        builder.appendString(m_originValue->getXAxis().dumpString());
        builder.appendString(String::spaceString);
        builder.appendString(m_originValue->getYAxis().dumpString());
        builder.appendString(String::spaceString);
        builder.appendString(m_originValue->getXAxis().dumpString());
        builder.appendChar(')');

        return builder.finalize();
    }

    bool operator==(const StyleTransformOrigin& origin)
    {
        return m_originValue->getXAxis() == origin.m_originValue->getXAxis() &&
               m_originValue->getYAxis() == origin.m_originValue->getYAxis() &&
               m_originValue->getZAxis() == origin.m_originValue->getZAxis();
    }

    bool operator!=(const StyleTransformOrigin& origin)
    {
        return !(this->operator==(origin));
    }

    void* operator new(size_t size)
    {
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word obj_bitmap[GC_BITMAP_SIZE(StyleTransformOrigin)] = { 0 };
            GC_set_bit(obj_bitmap,
                       GC_WORD_OFFSET(StyleTransformOrigin, m_originValue));
            descr = GC_make_descriptor(obj_bitmap,
                                       GC_WORD_LEN(StyleTransformOrigin));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new[](size_t size) = delete;

private:
    TransformOriginData* m_originValue;
};
}
#endif /* STYLETRANSFORMORIGIN_H_ */
