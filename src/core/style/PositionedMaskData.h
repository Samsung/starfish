/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#ifndef __StarfishImageMaskData__
#define __StarfishImageMaskData__

#include "core/style/Style.h"

namespace Starfish {

class ImageResource;

class MaskLayer : public gc {
public:
    MaskLayer();

    union MaskSize {
        MaskSizeValue m_typeValue;
        LengthSize* m_lengthValue;

        MaskSize()
            : m_lengthValue(nullptr)
        {
        }

        MaskSize(MaskSizeValue typeValue)
            : m_typeValue(typeValue)
        {
        }

        MaskSize(LengthSize* lengthValue)
            : m_lengthValue(lengthValue)
        {
        }
    };

    ImageValue* image() const
    {
        return m_image;
    }

    void setImage(ImageValue* uri)
    {
        m_image = uri;
    }

    void setImageResource(ImageResource* imageResource)
    {
        m_imageResource = imageResource;
    }

    ImageResource* imageResource() const
    {
        return m_imageResource;
    }

    void setSize(LengthSize size);

    Length positionX() const
    {
        return m_positionX;
    }

    void setPositionX(Length value)
    {
        m_positionX = value;
    }

    void resetPositionX()
    {
        m_positionX = Length(Length::Percent, 0.0f);
    }

    Length positionY() const
    {
        return m_positionY;
    }

    void setPositionY(Length value)
    {
        m_positionY = value;
    }

    void resetPositionY()
    {
        m_positionY = Length(Length::Percent, 0.0f);
    }

    void setSize(MaskSizeValue size)
    {
        m_sizeIsLength = false;
        m_size.m_typeValue = size;
    }

    LengthSize sizeLengthValue() const
    {
        STARFISH_ASSERT(m_sizeIsLength);
        if (m_size.m_lengthValue) {
            return *m_size.m_lengthValue;
        }
        return LengthSize();
    }

    MaskSizeValue sizeTypeValue() const
    {
        STARFISH_ASSERT(!m_sizeIsLength);
        return m_size.m_typeValue;
    }

    bool operator==(const MaskLayer& other);

    bool operator!=(const MaskLayer& other)
    {
        return !operator==(other);
    }

    ImageValue* m_image = nullptr;
    ImageResource* m_imageResource = nullptr;

    // mask-size
    bool m_sizeIsLength = true;
    MaskSize m_size;
    Length m_positionX;
    Length m_positionY;
};

class PositionedMaskData : public gc {
public:
    static bool damaged(const PositionedMaskData* lhs,
                        const PositionedMaskData* rhs, bool* damagedKeys);

    PositionedMaskData();

    ImageValue* image(uint32_t layer) const;

    Length positionX(uint32_t layer) const;

    Length positionY(uint32_t layer) const;

    ImageResource* imageResource(uint32_t layer) const;

    void setImage(ImageValue* value, uint32_t layer);

    void setImageResource(ImageResource* imageResource, uint32_t layer);

    void setSize(MaskSizeValue size, uint32_t layer);

    void setSize(LengthSize size, uint32_t layer);

    void setPositionX(Length value, uint32_t layer);

    void setPositionY(Length value, uint32_t layer);

    bool maskSizeIsLength(uint32_t layer) const;

    LengthSize maskSizeLengthValue(uint32_t layer) const;

    MaskSizeValue maskSizeTypeValue(uint32_t layer) const;

    void shrinkImages(uint32_t size);

    void shrinkPositionXs(uint32_t size);

    void shrinkPositionYs(uint32_t size);

    size_t size() const
    {
        return m_layers.size();
    }

    uint32_t sizeOfLayers()
    {
        STARFISH_ASSERT(m_maxLayerImage <= m_layers.size());
        return m_maxLayerImage;
    }

    bool operator==(const PositionedMaskData& other);

    bool operator!=(const PositionedMaskData& other)
    {
        return !operator==(other);
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

private:
    uint32_t assureLayerIndexAndSize(uint32_t layer, uint32_t& currentMax);

    uint32_t m_maxLayerSize = 0;
    uint32_t m_maxLayerImage = 0;
    uint32_t m_maxLayerPositionX = 0;
    uint32_t m_maxLayerPositionY = 0;

    GCVector<MaskLayer> m_layers;
};
} // namespace Starfish

#endif
