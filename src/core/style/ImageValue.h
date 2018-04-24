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

// https://www.w3.org/TR/3-images/#image-type

#ifndef __StarFishImage__
#define __StarFishImage__

namespace StarFish {

class String;
class GradientData;

class ImageValueType {
public:
    enum class ValueType {
        None,
        Invalid, // Only for type checking
        URL,
        // ImageList,
        // ElementReference,
        Gradient
    };

    STARFISH_MAKE_STACK_ALLOCATED();

    ImageValueType()
        : m_type(ValueType::None)
    {
    }

    ImageValueType(ValueType type)
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

    bool operator==(const ImageValueType& other) const
    {
        return m_type == other.m_type;
    }

    bool operator!=(const ImageValueType& other) const
    {
        return !(operator==(other));
    }

    ValueType m_type;
};

class ImageValue : public gc {
public:
    union ImageValueData {
        String* m_url;
        GradientData* m_gradient;

        ImageValueData()
            : m_url(nullptr)
        {
        }

        ImageValueData(String* url)
            : m_url(url)
        {
        }

        ImageValueData(GradientData* gradient)
            : m_gradient(gradient)
        {
        }
    };

    ImageValue()
        : m_valueType(ImageValueType::ValueType::None)
    {
    }

    ImageValue(String* url)
        : m_valueData(url)
        , m_valueType(ImageValueType::ValueType::URL)
    {
    }

    ImageValue(GradientData* gradient)
        : m_valueData(gradient)
        , m_valueType(ImageValueType::ValueType::Gradient)
    {
    }

    ImageValueType type() const
    {
        return m_valueType;
    }

    String* urlValue() const;
    GradientData* gradientValue() const;

    void applyOriginToURL(const ResourceURL* origin);

    void* operator new(size_t size)
    {
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word obj_bitmap[GC_BITMAP_SIZE(ImageValue)] = { 0 };
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(ImageValue, m_valueData));
            descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(ImageValue));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }

    void* operator new(size_t size, ImageValue* imageValue)
    {
        return imageValue;
    }

    void* operator new[](size_t size) = delete;

    bool operator==(const ImageValue& other) const;

    bool operator!=(const ImageValue& other) const
    {
        return !(operator==(other));
    }

private:
    ImageValueData m_valueData;
    ImageValueType m_valueType;
};
}

#endif
