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

#ifndef __StarFishImageMaskData__
#define __StarFishImageMaskData__

#include "core/style/Style.h"

namespace StarFish {

class MaskLayer : public gc {
public:
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

    String* image()
    {
        return m_image;
    }

    void setImage(String* uri)
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

    String* m_image;

    // mask-size
    bool m_sizeIsLength;
    MaskSize m_size;
};

class PositionedMaskData : public gc {
public:
    PositionedMaskData()
        : m_maskImage(nullptr)
        , m_maxLayerSizes(0)
        , m_maxLayerImages(0)
    {
    }

    String* image(unsigned int layer = 0)
    {
        if (m_layers.size() <= layer) {
            return String::emptyString;
        }
        return m_layers[layer].image();
    }

    void setImage(String* url, unsigned int layer = 0)
    {
        // Note: transparent black image layer by default
        resizeLayerIfNeeded(layer);
        if (m_maxLayerImages < layer + 1) {
            m_maxLayerImages = layer + 1;
        }
        m_layers[layer].setImage(url);
    }

    void setSize(MaskSizeValue size, unsigned int layer)
    {
        resizeLayerIfNeeded(layer);
        if (m_maxLayerSizes < layer + 1) {
            m_maxLayerSizes = layer + 1;
        }
        m_layers[layer].setSize(size);
    }

    void setSize(LengthSize size, unsigned int layer)
    {
        resizeLayerIfNeeded(layer);
        if (m_maxLayerSizes < layer + 1) {
            m_maxLayerSizes = layer + 1;
        }
        m_layers[layer].setSize(size);
    }

    LengthSize maskSizeLengthValue(unsigned int layer = 0) const
    {
        if (m_layers.size() <= layer) {
            return LengthSize();
        }
        return m_layers[layer].sizeLengthValue();
    }

    MaskSizeValue maskSizeTypeValue(unsigned int layer = 0) const
    {
        if (m_layers.size() <= layer) {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
            return MaskSizeValue::ContainMaskSizeValue;
        }
        return m_layers[layer].sizeTypeValue();
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
    void resizeLayerIfNeeded(unsigned int layer)
    {
        if (m_layers.size() <= layer) {
            m_layers.resize(layer + 1);
        }
    }

    NativeImageData* m_maskImage;
    unsigned int m_maxLayerSizes;
    unsigned int m_maxLayerImages;

    GCVector<MaskLayer> m_layers;
};
}

#endif
