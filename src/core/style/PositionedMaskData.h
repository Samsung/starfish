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

class MaskLayer : public gc {
public:
    MaskLayer()
        : m_image(nullptr)
        , m_sizeIsLength(true)
    {
    }

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

    void setSize(LengthSize size)
    {
        m_sizeIsLength = true;
        if (!m_size.m_lengthValue) {
            if (size == LengthSize()) {
                return;
            }
            m_size.m_lengthValue = new LengthSize(size);
        } else {
            *m_size.m_lengthValue = size;
        }
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

    ImageValue* m_image;

    // mask-size
    bool m_sizeIsLength;
    MaskSize m_size;
};

class PositionedMaskData : public gc {
public:
    PositionedMaskData()
        : m_maskImage(nullptr)
        , m_maxLayerSize(0)
        , m_maxLayerImage(0)
    {
    }
    static bool damaged(const PositionedMaskData* lhs,
                        const PositionedMaskData* rhs, bool* damagedKeys)
    {
        // TODO : Implement the rest of the css masking properties.
        damagedKeys[CSSStyleValuePair::KeyKind::MaskImage] = false;

        if (!lhs && !rhs) {
            return false;
        }

        PositionedMaskData temp;
        lhs = lhs ? lhs : &temp;
        rhs = rhs ? rhs : &temp;
        uint32_t maxLayer =
            std::max(lhs->m_maxLayerImage, rhs->m_maxLayerImage);
        bool hasDamage = false;
        for (uint32_t i = 0; i < maxLayer; i++) {
            if (damagedKeys[CSSStyleValuePair::KeyKind::MaskImage] == false) {
                const ImageValue* lImage = lhs->image(i);
                const ImageValue* rImage = rhs->image(i);
                if (lImage != rImage &&
                    (!lImage || !rImage || *lImage != *rImage)) {
                    damagedKeys[CSSStyleValuePair::KeyKind::MaskImage] =
                        hasDamage = true;
                }
            }
        }
        return hasDamage;
    }

    ImageValue* image(uint32_t layer) const
    {
        if (m_layers.size() <= layer) {
            return nullptr;
        }
        return m_layers[layer].image();
    }

    void setImage(ImageValue* value, uint32_t layer)
    {
        // Note: transparent black image layer by default
        resizeLayerIfNeeded(layer);
        if (m_maxLayerImage < layer + 1) {
            m_maxLayerImage = layer + 1;
        }
        m_layers[layer].setImage(value);
    }

    void setSize(MaskSizeValue size, uint32_t layer)
    {
        resizeLayerIfNeeded(layer);
        if (m_maxLayerSize < layer + 1) {
            m_maxLayerSize = layer + 1;
        }
        m_layers[layer].setSize(size);
    }

    void setSize(LengthSize size, uint32_t layer)
    {
        resizeLayerIfNeeded(layer);
        if (m_maxLayerSize < layer + 1) {
            m_maxLayerSize = layer + 1;
        }
        m_layers[layer].setSize(size);
    }

    bool maskSizeIsLength(uint32_t layer) const
    {
        if (m_layers.size() <= layer) {
            return true;
        }
        return m_layers[layer].m_sizeIsLength;
    }

    LengthSize maskSizeLengthValue(uint32_t layer) const
    {
        if (m_layers.size() <= layer) {
            return LengthSize();
        }
        return m_layers[layer].sizeLengthValue();
    }

    MaskSizeValue maskSizeTypeValue(uint32_t layer) const
    {
        if (m_layers.size() <= layer) {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
            return MaskSizeValue::ContainMaskSizeValue;
        }
        return m_layers[layer].sizeTypeValue();
    }

    void shrinkImages(uint32_t size)
    {
        STARFISH_ASSERT(m_layers.size() >= m_maxLayerImage);
        for (uint32_t i = 0; i < m_maxLayerImage; i++) {
            m_layers[i].setImage(nullptr);
        }
        m_maxLayerImage = size;
    }

    size_t size() const
    {
        return m_layers.size();
    }

    uint32_t sizeOfLayers()
    {
        STARFISH_ASSERT(m_maxLayerImage <= m_layers.size());
        return m_maxLayerImage;
    }

    bool operator==(const PositionedMaskData& o)
    {
        return m_maskImage == o.m_maskImage;
    }

    bool operator!=(const PositionedMaskData& o)
    {
        return !operator==(o);
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

private:
    void resizeLayerIfNeeded(uint32_t layer)
    {
        if (m_layers.size() <= layer) {
            m_layers.resize(layer + 1);
        }
    }

    NativeImageData* m_maskImage;
    uint32_t m_maxLayerSize;
    uint32_t m_maxLayerImage;

    GCVector<MaskLayer> m_layers;
};
} // namespace Starfish

#endif
