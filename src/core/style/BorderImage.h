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

#ifndef __StarFishBorderImage__
#define __StarFishBorderImage__

#include "core/style/BorderImageLength.h"
#include "core/style/Style.h"

namespace StarFish {

#define DEFAULT_VALUE_IMAGE_WIDTH() (BorderImageLengthBox(1.0))
#define DEFAULT_VALUE_IMAGE_OUTSET() \
    (BorderImageLengthBox(Length(Length::Fixed, 0)))
#define DEFAULT_VALUE_IMAGE_SLICE() \
    (BorderImageLengthBox(Length(Length::Percent, 1.0)))
#define DEFAULT_VALUE_IMAGE_REPEAT() (StretchValue)

class NativeImageData;
class ImageResource;
class ComputedStyle;

class BorderImageImpl : public gc {
public:
    BorderImageImpl()
        : m_repeatX(DEFAULT_VALUE_IMAGE_REPEAT())
        , m_repeatY(DEFAULT_VALUE_IMAGE_REPEAT())
        , m_url(String::emptyString)
        , m_sliceFill(false)
        , m_slices(DEFAULT_VALUE_IMAGE_SLICE())
        , m_widths(DEFAULT_VALUE_IMAGE_WIDTH())
        , m_outsets(DEFAULT_VALUE_IMAGE_OUTSET())
        , m_imageResource(NULL)
    {
    }

    void checkComputed(Length curFontSize, Length rootFontSize, Font* font,
                       LayoutSize windowSize, ComputedStyle* cs)
    {
        m_slices.checkComputed(curFontSize, rootFontSize, font, windowSize, cs);
    }

    bool operator==(const BorderImageImpl& o)
    {
        return m_repeatX == o.m_repeatX && m_repeatY == o.m_repeatY &&
               m_url->equals(o.m_url) && m_sliceFill == o.m_sliceFill &&
               m_slices == o.m_slices && m_widths == o.m_widths;
    }

    bool operator!=(const BorderImageImpl& o)
    {
        return !operator==(o);
    }

    void* operator new(size_t size)
    {
        STARFISH_ASSERT(size == sizeof(BorderImageImpl));
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word obj_bitmap[GC_BITMAP_SIZE(BorderImageImpl)] = { 0 };
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(BorderImageImpl, m_url));
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(BorderImageImpl, m_slices));
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(BorderImageImpl, m_widths));
            GC_set_bit(obj_bitmap,
                       GC_WORD_OFFSET(BorderImageImpl, m_imageResource));
            descr =
                GC_make_descriptor(obj_bitmap, GC_WORD_LEN(BorderImageImpl));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new[](size_t size) = delete;

public:
    // NOTE: current spec does not support border-image-repeat
    BorderImageRepeatValue m_repeatX; // [border-image-repeat]
    BorderImageRepeatValue m_repeatY; // [border-image-repeat]

    // TODO: Need Image Data Structure
    String* m_url;                  // [border-image-source]
    bool m_sliceFill;               // [border-image-slice]
    BorderImageLengthBox m_slices;  // [border-image-slice]
    BorderImageLengthBox m_widths;  // [border-image-width]
    BorderImageLengthBox m_outsets; // [border-image-outset]
    ImageResource* m_imageResource;
};

class BorderImageData {
public:
    BorderImageData()
        : m_data(nullptr)
    {
    }

    STARFISH_MAKE_STACK_ALLOCATED();

    String* url()
    {
        return isNull() ? String::emptyString : m_data->m_url;
    }
    BorderImageLengthBox slices()
    {
        return isNull() ? DEFAULT_VALUE_IMAGE_SLICE() : m_data->m_slices;
    }
    bool sliceFill()
    {
        return isNull() ? false : m_data->m_sliceFill;
    }
    BorderImageRepeatValue repeatX() const
    {
        return isNull() ? DEFAULT_VALUE_IMAGE_REPEAT() : m_data->m_repeatX;
    }
    BorderImageRepeatValue repeatY() const
    {
        return isNull() ? DEFAULT_VALUE_IMAGE_REPEAT() : m_data->m_repeatY;
    }
    BorderImageLengthBox widths() const
    {
        return isNull() ? DEFAULT_VALUE_IMAGE_WIDTH() : m_data->m_widths;
    }
    BorderImageLengthBox outsets() const
    {
        return isNull() ? DEFAULT_VALUE_IMAGE_OUTSET() : m_data->m_outsets;
    }
    NativeImageData* imageData();

    ImageResource* imageResource()
    {
        if (!isNull() && data()->m_imageResource) {
            return data()->m_imageResource;
        }
        return NULL;
    }

    void setUrl(String* url)
    {
        data()->m_url = url;
    }
    void setSlices(const BorderImageLengthBox& slices)
    {
        data()->m_slices = slices;
    }
    void setSliceFill(bool fill)
    {
        data()->m_sliceFill = fill;
    }
    void setRepeatX(BorderImageRepeatValue value)
    {
        data()->m_repeatX = value;
    }
    void setRepeatY(BorderImageRepeatValue value)
    {
        data()->m_repeatY = value;
    }
    void setWidths(BorderImageLengthBox value)
    {
        data()->m_widths = value;
    }
    void setOutsets(BorderImageLengthBox value)
    {
        data()->m_outsets = value;
    }
    void setImageResource(ImageResource* value)
    {
        data()->m_imageResource = value;
    }

    void checkComputed(Length curFontSize, Length rootFontSize, Font* font,
                       LayoutSize windowSize, ComputedStyle* cs)
    {
        if (m_data) {
            m_data->checkComputed(curFontSize, rootFontSize, font, windowSize,
                                  cs);
        }
    }

    bool operator==(const BorderImageData& o)
    {
        if (m_data == NULL && o.m_data == NULL) {
        } else if (m_data == NULL || o.m_data == NULL) {
            return false;
        } else if (*m_data != *o.m_data) {
            return false;
        }
        return true;
    }

    bool operator!=(const BorderImageData& o)
    {
        return !operator==(o);
    }

    bool isNull() const
    {
        return m_data == nullptr;
    }

private:
    BorderImageImpl* data()
    {
        if (isNull())
            m_data = new BorderImageImpl();
        return m_data;
    }

private:
    BorderImageImpl* m_data;
};

} /* namespace StarFish */

#endif /* __StarBorderImage__ */
