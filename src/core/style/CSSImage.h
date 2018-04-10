/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

// https://www.w3.org/TR/css3-images/#image-type

#ifndef __StarFishCSSImage__
#define __StarFishCSSImage__

namespace StarFish {

class String;
class CSSGradientValue;

class CSSImageValueType {
public:
    enum class ValueType {
        None,
        Invalid, // Only for type checking
        URL,
        // ImageList,
        // ElementReference,
        Gradient
    };

    // STARFISH_MAKE_STACK_ALLOCATED();

    CSSImageValueType()
        : m_type(ValueType::None)
    {
    }

    CSSImageValueType(ValueType type)
        : m_type(type)
    {
    }

    bool isNone() const
    {
        return m_type == ValueType::None;
    }

    bool isURL() const
    {
        return m_type == ValueType::URL;
    }

    bool isGradient() const
    {
        return m_type == ValueType::Gradient;
    }

    bool operator==(CSSImageValueType& other) const
    {
        return m_type == other.m_type;
    }

    bool operator!=(CSSImageValueType& other) const
    {
        return !(operator==(other));
    }

    ValueType m_type;
};

class CSSImage : public gc {
public:
    union CSSImageValueData {
        String* m_url;
        CSSGradientValue* m_gradient;

        CSSImageValueData()
        {
        }

        CSSImageValueData(String* url)
            : m_url(url)
        {
        }

        CSSImageValueData(CSSGradientValue* gradient)
            : m_gradient(gradient)
        {
        }
    };

    CSSImage()
        : m_valueType(CSSImageValueType::ValueType::None)
    {
    }

    CSSImage(String* url)
        : m_valueType(CSSImageValueType::ValueType::URL)
        , m_valueData(url)
    {
    }

    CSSImage(CSSGradientValue* gradient)
        : m_valueType(CSSImageValueType::ValueType::Gradient)
        , m_valueData(gradient)
    {
    }

    CSSImageValueType type() const
    {
        return m_valueType;
    }

    String* urlValue() const;
    CSSGradientValue* gradientValue() const;

    String* toString() const;

    void* operator new(size_t size)
    {
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word obj_bitmap[GC_BITMAP_SIZE(CSSImage)] = { 0 };
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(CSSImage, m_valueData));
            descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(CSSImage));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }

    void* operator new(size_t size, CSSImage* cssImage)
    {
        return cssImage;
    }

    void* operator new[](size_t size) = delete;

private:
    CSSImageValueType m_valueType;
    CSSImageValueData m_valueData;
};
}

#endif
