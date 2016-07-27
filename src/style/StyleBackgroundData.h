/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

#ifndef __StarFishStyleBackgroundData__
#define __StarFishStyleBackgroundData__

#include "style/Style.h"
#include "loader/ImageResource.h"

namespace StarFish {

class ImageResource;

class BackgroundLayer : public gc {
public:
    BackgroundLayer()
        : m_image(String::emptyString)
        , m_imageResource(NULL)
        , m_repeatX(BackgroundRepeatValue::RepeatRepeatValue)
        , m_repeatY(BackgroundRepeatValue::RepeatRepeatValue)
        , m_sizeType(BackgroundSizeType::SizeValue)
        , m_positionValue(nullptr)
        , m_sizeValue(nullptr)
    {
    }

    ~BackgroundLayer()
    {
    }

    void setPositionValue(LengthPosition position)
    {
        m_positionValue = new LengthPosition(position);
    }

    void setSizeType(BackgroundSizeType type)
    {
        m_sizeType = type;
    }

    void setSizeValue(LengthSize size)
    {
        m_sizeType = BackgroundSizeType::SizeValue;
        if (!m_sizeValue) {
            if (size == LengthSize())
                return;
            m_sizeValue = new LengthSize(size);
        } else
            *m_sizeValue = size;
    }

    void setBgImage(String* img)
    {
        m_image = img;
    }

    void setBgImageResource(ImageResource* data)
    {
        m_imageResource = data;
    }

    void setRepeatX(BackgroundRepeatValue repeat)
    {
        m_repeatX = repeat;
    }

    void setRepeatY(BackgroundRepeatValue repeat)
    {
        m_repeatY = repeat;
    }

    String* bgImage()
    {
        return m_image;
    }

    ImageData* bgImageData()
    {
        if (m_imageResource)
            return m_imageResource->imageData();
        return nullptr;
    }

    ImageResource* bgImageResource()
    {
        return m_imageResource;
    }

    BackgroundSizeType sizeType()
    {
        return m_sizeType;
    }

    BackgroundRepeatValue repeatX()
    {
        return m_repeatX;
    }

    BackgroundRepeatValue repeatY()
    {
        return m_repeatY;
    }

    LengthSize sizeValue() const
    {
        STARFISH_ASSERT(m_sizeType == BackgroundSizeType::SizeValue);
        if (m_sizeValue)
            return *m_sizeValue;
        return LengthSize();
    }

    bool hasPositionValue()
    {
        return m_positionValue;
    }

    LengthPosition positionValue() const
    {
        if (m_positionValue == NULL)
            return LengthPosition(Length(Length::Percent, 0.0f), Length(Length::Percent, 0.0f));
        return *m_positionValue;
    }

    void checkComputed(Length fontSize, Font* font)
    {
        if (m_sizeValue)
            m_sizeValue->checkComputed(fontSize, font);

        if (m_positionValue)
            m_positionValue->checkComputed(fontSize, font);
    }

private:
    friend inline bool operator==(const BackgroundLayer& a, const BackgroundLayer& b);
    friend inline bool operator!=(const BackgroundLayer& a, const BackgroundLayer& b);

    String* m_image;
    ImageResource* m_imageResource;

    // background-repeat
    BackgroundRepeatValue m_repeatX : 1;
    BackgroundRepeatValue m_repeatY : 1;
    // background-size
    BackgroundSizeType m_sizeType : 2;

    // background-position
    LengthPosition* m_positionValue;
    // background-size
    LengthSize* m_sizeValue;
};

class StyleBackgroundData : public gc {
public:
    StyleBackgroundData()
        : m_bgColorNeedToUpdate(false)
        , m_maxLayerImages(0)
        , m_maxLayerRepeats(0)
        , m_maxLayerSizes(0)
        , m_maxLayerPositions(0)
    {
    }

    ~StyleBackgroundData()
    {
    }

    void setBgColor(Color color)
    {
        m_color = color;
        m_bgColorNeedToUpdate = false;
    }

    void setBgColorToCurrentColor()
    {
        m_bgColorNeedToUpdate = true;
    }

    void resizeLayerIfNeeded(unsigned int layer)
    {
        if (m_layers.size() <= layer)
            m_layers.resize(layer + 1);
    }

    void setPositionValue(LengthPosition position, unsigned int layer)
    {
        resizeLayerIfNeeded(layer);
        if (m_maxLayerPositions < layer + 1)
            m_maxLayerPositions = layer + 1;
        m_layers[layer].setPositionValue(position);
    }

    void setSizeType(BackgroundSizeType type, unsigned int layer)
    {
        resizeLayerIfNeeded(layer);
        if (m_maxLayerSizes < layer + 1)
            m_maxLayerSizes = layer + 1;
        m_layers[layer].setSizeType(type);
    }

    void setSizeValue(LengthSize size, unsigned int layer)
    {
        resizeLayerIfNeeded(layer);
        if (m_maxLayerSizes < layer + 1)
            m_maxLayerSizes = layer + 1;
        m_layers[layer].setSizeValue(size);
    }

    void setBgImage(String* img, unsigned int layer)
    {
        resizeLayerIfNeeded(layer);
        if (m_maxLayerImages < layer + 1)
            m_maxLayerImages = layer + 1;
        m_layers[layer].setBgImage(img);
    }

    void setBgImageResource(ImageResource* data, unsigned int layer)
    {
        resizeLayerIfNeeded(layer);
        if (m_maxLayerImages < layer + 1)
            m_maxLayerImages = layer + 1;
        m_layers[layer].setBgImageResource(data);
    }

    void setRepeatX(BackgroundRepeatValue repeat, unsigned int layer = 0)
    {
        resizeLayerIfNeeded(layer);
        if (m_maxLayerRepeats < layer + 1)
            m_maxLayerRepeats = layer + 1;
        m_layers[layer].setRepeatX(repeat);
    }

    void setRepeatY(BackgroundRepeatValue repeat, unsigned int layer = 0)
    {
        resizeLayerIfNeeded(layer);
        if (m_maxLayerRepeats < layer + 1)
            m_maxLayerRepeats = layer + 1;
        m_layers[layer].setRepeatY(repeat);
    }

    Color bgColor()
    {
        return m_color;
    }

    String* bgImage(unsigned int layer = 0)
    {
        if (m_layers.size() <= layer)
            return String::emptyString;
        return m_layers[layer].bgImage();
    }

    ImageData* bgImageData(unsigned int layer = 0)
    {
        if (m_layers.size() <= layer)
            return nullptr;
        return m_layers[layer].bgImageData();
    }

    ImageResource* bgImageResource(unsigned int layer = 0)
    {
        if (m_layers.size() <= layer)
            return nullptr;
        return m_layers[layer].bgImageResource();
    }

    BackgroundSizeType sizeType(unsigned int layer = 0)
    {
        if (m_layers.size() <= layer)
            return BackgroundSizeType::SizeValue;
        return m_layers[layer].sizeType();
    }

    BackgroundRepeatValue repeatX(unsigned int layer = 0)
    {
        if (m_layers.size() <= layer)
            return BackgroundRepeatValue::RepeatRepeatValue;
        return m_layers[layer].repeatX();
    }

    BackgroundRepeatValue repeatY(unsigned int layer = 0)
    {
        if (m_layers.size() <= layer)
            return BackgroundRepeatValue::RepeatRepeatValue;
        return m_layers[layer].repeatY();
    }

    LengthSize sizeValue(unsigned int layer = 0) const
    {
        if (m_layers.size() <= layer)
            return LengthSize();
        return m_layers[layer].sizeValue();
    }

    bool hasPositionValue(unsigned int layer = 0)
    {
        if (m_layers.size() <= layer)
            return false;
        return m_layers[layer].hasPositionValue();
    }

    LengthPosition positionValue(unsigned int layer = 0) const
    {
        if (m_layers.size() <= layer)
            return LengthPosition(Length(Length::Percent, 0.0f), Length(Length::Percent, 0.0f));
        return m_layers[layer].positionValue();
    }

    void checkComputed(Length fontSize, Font* font, Color color)
    {
        // NOTE: To support background layer
        if (m_layers.size() > m_maxLayerImages)
            m_layers.resize(m_maxLayerImages);

        if (m_maxLayerPositions + 1 < m_layers.size()) {
            unsigned int i = m_maxLayerPositions;
            while (i < m_layers.size()) {
                for (unsigned int p = 0; p < m_maxLayerPositions && i < m_layers.size(); p++, i++) {
                    m_layers[i].setPositionValue(m_layers[p].positionValue());
                }
            }
        }
        if (m_maxLayerSizes + 1 < m_layers.size()) {
            unsigned int i = m_maxLayerSizes;
            while (i < m_layers.size()) {
                for (unsigned int p = 0; p < m_maxLayerSizes && i < m_layers.size(); p++, i++) {
                    m_layers[i].setSizeValue(m_layers[p].sizeValue());
                    m_layers[i].setSizeType(m_layers[p].sizeType());
                }
            }
        }
        if (m_maxLayerRepeats + 1 < m_layers.size()) {
            unsigned int i = m_maxLayerRepeats;
            while (i < m_layers.size()) {
                for (unsigned int p = 0; p < m_maxLayerRepeats && i < m_layers.size(); p++, i++) {
                    m_layers[i].setRepeatX(m_layers[p].repeatX());
                    m_layers[i].setRepeatY(m_layers[p].repeatY());
                }
            }
        }

        if (m_layers.size()) {
            for (unsigned int i = 0; i < m_layers.size(); i++)
                m_layers[i].checkComputed(fontSize, font);
        }

        // background-color
        // - default : transparent
        // - currentColor : represents the "calculated" value of the element's color property
        if (m_bgColorNeedToUpdate)
            setBgColor(color);
    }

    unsigned int sizeOfLayers()
    {
        return m_layers.size();
    }

private:
    friend inline bool operator==(const StyleBackgroundData& a, const StyleBackgroundData& b);
    friend inline bool operator!=(const StyleBackgroundData& a, const StyleBackgroundData& b);

    Color m_color;
    // background-color type
    bool m_bgColorNeedToUpdate : 1;
    unsigned int m_maxLayerImages;
    unsigned int m_maxLayerRepeats;
    unsigned int m_maxLayerSizes;
    unsigned int m_maxLayerPositions;

    std::vector<BackgroundLayer, gc_allocator<BackgroundLayer> > m_layers;

};

bool operator==(const BackgroundLayer& a, const BackgroundLayer& b)
{
    if (a.m_sizeType != b.m_sizeType)
        return false;

    if (a.m_image != b.m_image)
        return false;

    if (a.positionValue() != b.positionValue())
        return false;

    if (a.m_sizeType != b.m_sizeType)
        return false;

    if (a.m_sizeType == BackgroundSizeType::SizeValue && a.sizeValue() != b.sizeValue())
        return false;

    if (a.m_repeatX != b.m_repeatX)
        return false;

    if (a.m_repeatY != b.m_repeatY)
        return false;

    return true;
}

bool operator!=(const BackgroundLayer& a, const BackgroundLayer& b)
{
    return !operator==(a, b);
}

bool operator==(const StyleBackgroundData& a, const StyleBackgroundData& b)
{
    if (a.m_color != b.m_color)
        return false;

    if (a.m_layers.size() != b.m_layers.size())
        return false;

    for (unsigned int i = 0; i < a.m_layers.size(); i++) {
        if (a.m_layers[i] != b.m_layers[i])
            return false;
    }

    return true;
}

bool operator!=(const StyleBackgroundData& a, const StyleBackgroundData& b)
{
    return !operator==(a, b);
}
}

#endif
