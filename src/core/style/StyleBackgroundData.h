/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#include "core/style/Style.h"

namespace StarFish {

class NativeImageData;
class ImageResource;
class ComputedStyle;

class BackgroundLayer : public gc {
    friend class StyleBackgroundData;

public:
    union BackgroundSize {
        BackgroundSizeValue m_typeValue;
        LengthSize* m_lengthValue;

        bool hasLengthValue() const
        {
            static_assert(BackgroundSizeValueEnd < 4,
                          "last value of BackgroundSizeValue should be smaller "
                          "than minium pointer value");
            return m_lengthValue == 0 ||
                   (size_t)m_lengthValue > BackgroundSizeValueEnd;
        }

        BackgroundSize()
            : m_lengthValue(nullptr)
        {
        }

        BackgroundSize(BackgroundSizeValue typeValue)
            : m_typeValue(typeValue)
        {
        }

        BackgroundSize(LengthSize* lengthValue)
            : m_lengthValue(lengthValue)
        {
        }
    };

    BackgroundLayer()
        : m_image(String::emptyString)
        , m_imageResource(NULL)
        , m_repeatX(BackgroundRepeatValue::RepeatRepeatValue)
        , m_repeatY(BackgroundRepeatValue::RepeatRepeatValue)
        , m_positionX(Length(Length::Percent, 0.0f))
        , m_positionY(Length(Length::Percent, 0.0f))
        , m_attachment(
              BackgroundAttachmentValue::ScrollBackgroundAttachmentValue)
        , m_clip(BoxValue::BorderBoxBoxValue)
        , m_origin(BoxValue::PaddingBoxBoxValue)
    {
    }

    ~BackgroundLayer()
    {
    }

    void setSize(LengthSize size)
    {
        if (!m_size.hasLengthValue()) {
            if (size == LengthSize()) {
                m_size.m_lengthValue = nullptr;
                return;
            }
            m_size.m_lengthValue = new LengthSize(size);
        } else {
            if (m_size.m_lengthValue) {
                *m_size.m_lengthValue = size;
            } else {
                m_size.m_lengthValue = new LengthSize(size);
            }
        }
    }

    void setSize(BackgroundSizeValue size)
    {
        // force set length value to nullptr to to clean all of union data area
        m_size.m_lengthValue = nullptr;
        m_size.m_typeValue = size;
    }

    void setImage(String* img)
    {
        m_image = img;
    }

    void setImageResource(ImageResource* data)
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

    void setPositionX(Length position)
    {
        m_positionX = position;
    }

    void setPositionY(Length position)
    {
        m_positionY = position;
    }

    void setAttachment(BackgroundAttachmentValue attachment)
    {
        m_attachment = attachment;
    }

    void setClip(BoxValue clip)
    {
        m_clip = clip;
    }

    void setOrigin(BoxValue origin)
    {
        m_origin = origin;
    }

    String* bgImage() const
    {
        return m_image;
    }

    NativeImageData* imageData() const;

    ImageResource* imageResource() const
    {
        return m_imageResource;
    }

    BackgroundRepeatValue repeatX() const
    {
        return m_repeatX;
    }

    BackgroundRepeatValue repeatY() const
    {
        return m_repeatY;
    }

    Length positionX() const
    {
        return m_positionX;
    }

    Length positionY() const
    {
        return m_positionY;
    }

    BackgroundSizeValue sizeTypeValue() const
    {
        STARFISH_ASSERT(!m_size.hasLengthValue());
        return m_size.m_typeValue;
    }

    LengthSize sizeLengthValue() const
    {
        STARFISH_ASSERT(m_size.hasLengthValue());
        if (m_size.m_lengthValue) {
            return *m_size.m_lengthValue;
        }
        return LengthSize();
    }

    BackgroundAttachmentValue attachment() const
    {
        return m_attachment;
    }

    BoxValue clip() const
    {
        return m_clip;
    }

    BoxValue origin() const
    {
        return m_origin;
    }

    void checkComputed(Length curFontSize, Length rootFontSize, Font* font,
                       LayoutSize windowSize, ComputedStyle* cs)
    {
        if (m_size.hasLengthValue()) {
            if (m_size.m_lengthValue) {
                m_size.m_lengthValue->checkComputed(curFontSize, rootFontSize,
                                                    font, windowSize, cs);
            }
        }

        m_positionX.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                          windowSize.width(),
                                          windowSize.height(), cs);
        m_positionY.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                          windowSize.width(),
                                          windowSize.height(), cs);
    }

    void* operator new(size_t size)
    {
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word obj_bitmap[GC_BITMAP_SIZE(BackgroundLayer)] = { 0 };
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(BackgroundLayer, m_image));
            GC_set_bit(obj_bitmap,
                       GC_WORD_OFFSET(BackgroundLayer, m_imageResource));
            GC_set_bit(obj_bitmap,
                       GC_WORD_OFFSET(BackgroundLayer, m_positionX));
            GC_set_bit(obj_bitmap,
                       GC_WORD_OFFSET(BackgroundLayer, m_positionY));
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(BackgroundLayer, m_size));
            descr =
                GC_make_descriptor(obj_bitmap, GC_WORD_LEN(BackgroundLayer));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new(size_t size, BackgroundLayer* layer)
    {
        return layer;
    }
    void* operator new[](size_t size) = delete;

private:
    friend inline bool operator==(const BackgroundLayer& a,
                                  const BackgroundLayer& b);
    friend inline bool operator!=(const BackgroundLayer& a,
                                  const BackgroundLayer& b);

    String* m_image;
    ImageResource* m_imageResource;

    // background-repeat
    BackgroundRepeatValue m_repeatX : 1;
    BackgroundRepeatValue m_repeatY : 1;

    // background-position
    Length m_positionX;
    Length m_positionY;
    BackgroundSize m_size;
    // background-attachment
    BackgroundAttachmentValue m_attachment;
    // background-clip
    BoxValue m_clip;
    // background-origin
    BoxValue m_origin;
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

    void setBgColor(Unit::Color color)
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
        if (m_layers.size() <= layer) {
            m_layers.resize(layer + 1);
        }
    }

    void setSize(BackgroundSizeValue size, unsigned int layer)
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

    void setBgImage(String* img, unsigned int layer)
    {
        resizeLayerIfNeeded(layer);
        if (m_maxLayerImages < layer + 1) {
            m_maxLayerImages = layer + 1;
        }
        m_layers[layer].setImage(img);
    }

    void setBgImageResource(ImageResource* data, unsigned int layer)
    {
        resizeLayerIfNeeded(layer);
        if (m_maxLayerImages < layer + 1) {
            m_maxLayerImages = layer + 1;
        }
        m_layers[layer].setImageResource(data);
    }

    void setRepeatX(BackgroundRepeatValue repeat, unsigned int layer = 0)
    {
        resizeLayerIfNeeded(layer);
        if (m_maxLayerRepeats < layer + 1) {
            m_maxLayerRepeats = layer + 1;
        }
        m_layers[layer].setRepeatX(repeat);
    }

    void setRepeatY(BackgroundRepeatValue repeat, unsigned int layer = 0)
    {
        resizeLayerIfNeeded(layer);
        if (m_maxLayerRepeats < layer + 1) {
            m_maxLayerRepeats = layer + 1;
        }
        m_layers[layer].setRepeatY(repeat);
    }

    void setPositionX(Length position, unsigned int layer)
    {
        resizeLayerIfNeeded(layer);
        if (m_maxLayerPositions < layer + 1) {
            m_maxLayerPositions = layer + 1;
        }
        m_layers[layer].setPositionX(position);
    }

    void setPositionY(Length position, unsigned int layer)
    {
        resizeLayerIfNeeded(layer);
        if (m_maxLayerPositions < layer + 1) {
            m_maxLayerPositions = layer + 1;
        }
        m_layers[layer].setPositionY(position);
    }

    void setAttachment(BackgroundAttachmentValue attachment, unsigned int layer)
    {
        resizeLayerIfNeeded(layer);
        if (m_maxLayerPositions < layer + 1) {
            m_maxLayerPositions = layer + 1;
        }
        m_layers[layer].setAttachment(attachment);
    }

    void setClip(BoxValue clip, unsigned int layer)
    {
        resizeLayerIfNeeded(layer);
        if (m_maxLayerPositions < layer + 1) {
            m_maxLayerPositions = layer + 1;
        }
        m_layers[layer].setClip(clip);
    }

    void setOrigin(BoxValue origin, unsigned int layer)
    {
        resizeLayerIfNeeded(layer);
        if (m_maxLayerPositions < layer + 1) {
            m_maxLayerPositions = layer + 1;
        }
        m_layers[layer].setOrigin(origin);
    }

    Unit::Color bgColor()
    {
        return m_color;
    }

    String* bgImage(unsigned int layer = 0) const
    {
        if (m_layers.size() <= layer) {
            return String::emptyString;
        }
        return m_layers[layer].bgImage();
    }

    NativeImageData* bgImageData(unsigned int layer = 0) const
    {
        if (m_layers.size() <= layer) {
            return nullptr;
        }
        return m_layers[layer].imageData();
    }

    ImageResource* imageResource(unsigned int layer = 0) const
    {
        if (m_layers.size() <= layer) {
            return nullptr;
        }
        return m_layers[layer].imageResource();
    }

    BackgroundRepeatValue repeatX(unsigned int layer = 0) const
    {
        if (m_layers.size() <= layer) {
            return BackgroundRepeatValue::RepeatRepeatValue;
        }
        return m_layers[layer].repeatX();
    }

    BackgroundRepeatValue repeatY(unsigned int layer = 0) const
    {
        if (m_layers.size() <= layer) {
            return BackgroundRepeatValue::RepeatRepeatValue;
        }
        return m_layers[layer].repeatY();
    }

    bool sizeIsLength(unsigned int layer = 0) const
    {
        if (m_layers.size() <= layer) {
            return true;
        }
        return m_layers[layer].m_size.hasLengthValue();
    }

    BackgroundSizeValue sizeTypeValue(unsigned int layer = 0) const
    {
        if (m_layers.size() <= layer) {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
            return BackgroundSizeValue::ContainBackgroundSizeValue;
        }
        STARFISH_ASSERT(!m_layers[layer].m_size.hasLengthValue());
        return m_layers[layer].m_size.m_typeValue;
    }

    LengthSize sizeLengthValue(unsigned int layer = 0) const
    {
        if (m_layers.size() <= layer) {
            return LengthSize();
        }
        return m_layers[layer].sizeLengthValue();
    }

    BackgroundAttachmentValue attachment(unsigned int layer = 0) const
    {
        if (m_layers.size() <= layer) {
            return BackgroundAttachmentValue::ScrollBackgroundAttachmentValue;
        }
        return m_layers[layer].attachment();
    }

    BoxValue clip(unsigned int layer = 0) const
    {
        if (m_layers.size() <= layer) {
            return BoxValue::BorderBoxBoxValue;
        }
        return m_layers[layer].clip();
    }

    BoxValue origin(unsigned int layer = 0) const
    {
        if (m_layers.size() <= layer) {
            return BoxValue::PaddingBoxBoxValue;
        }
        return m_layers[layer].origin();
    }

    Length positionX(unsigned int layer = 0)
    {
        if (m_layers.size() <= layer) {
            return Length(Length::Percent, 0.0f);
        }
        return m_layers[layer].positionX();
    }

    Length positionY(unsigned int layer = 0)
    {
        if (m_layers.size() <= layer) {
            return Length(Length::Percent, 0.0f);
        }
        return m_layers[layer].positionY();
    }

    void checkComputed(Unit::Color color)
    {
        // NOTE: To support background layer
        if (m_layers.size() > m_maxLayerImages) {
            m_layers.resize(m_maxLayerImages);
        }

        if (m_maxLayerPositions > 0 &&
            m_maxLayerPositions + 1 < m_layers.size()) {
            unsigned int i = m_maxLayerPositions;
            while (i < m_layers.size()) {
                for (unsigned int p = 0;
                     p < m_maxLayerPositions && i < m_layers.size(); p++, i++) {
                    m_layers[i].setPositionX(m_layers[p].positionX());
                    m_layers[i].setPositionY(m_layers[p].positionY());
                }
            }
        }
        if (m_maxLayerSizes > 0 && m_maxLayerSizes + 1 < m_layers.size()) {
            unsigned int i = m_maxLayerSizes;
            while (i < m_layers.size()) {
                for (unsigned int p = 0;
                     p < m_maxLayerSizes && i < m_layers.size(); p++, i++) {
                    if (m_layers[p].m_size.hasLengthValue()) {
                        m_layers[i].setSize(m_layers[p].sizeLengthValue());
                    } else {
                        m_layers[i].setSize(m_layers[p].sizeTypeValue());
                    }
                }
            }
        }
        if (m_maxLayerRepeats > 0 && m_maxLayerRepeats + 1 < m_layers.size()) {
            unsigned int i = m_maxLayerRepeats;
            while (i < m_layers.size()) {
                for (unsigned int p = 0;
                     p < m_maxLayerRepeats && i < m_layers.size(); p++, i++) {
                    m_layers[i].setRepeatX(m_layers[p].repeatX());
                    m_layers[i].setRepeatY(m_layers[p].repeatY());
                }
            }
        }

        // background-color
        // - default : transparent
        // - currentColor : represents the "calculated" value of the element's
        // color property
        if (m_bgColorNeedToUpdate) {
            setBgColor(color);
        }
    }

    void checkComputed(Length curFontSize, Length rootFontSize, Font* font,
                       LayoutSize windowSize, ComputedStyle* cs)
    {
        if (m_layers.size()) {
            for (unsigned int i = 0; i < m_layers.size(); i++) {
                m_layers[i].checkComputed(curFontSize, rootFontSize, font,
                                          windowSize, cs);
            }
        }
    }

    unsigned int sizeOfLayers()
    {
        return m_layers.size();
    }

private:
    friend inline bool operator==(const StyleBackgroundData& a,
                                  const StyleBackgroundData& b);
    friend inline bool operator!=(const StyleBackgroundData& a,
                                  const StyleBackgroundData& b);

    Unit::Color m_color;
    // background-color type
    bool m_bgColorNeedToUpdate : 1;
    unsigned int m_maxLayerImages;
    unsigned int m_maxLayerRepeats;
    unsigned int m_maxLayerSizes;
    unsigned int m_maxLayerPositions;

    GCVector<BackgroundLayer> m_layers;
};

bool operator==(const BackgroundLayer& a, const BackgroundLayer& b)
{
    if (!a.m_image->equals(b.m_image)) {
        return false;
    }

    if (a.m_size.hasLengthValue() != b.m_size.hasLengthValue()) {
        return false;
    }

    if (a.m_size.hasLengthValue()) {
        if (a.sizeLengthValue() != b.sizeLengthValue()) {
            return false;
        }
    } else {
        if (a.sizeTypeValue() != b.sizeTypeValue()) {
            return false;
        }
    }

    if (a.m_repeatX != b.m_repeatX) {
        return false;
    }

    if (a.m_repeatY != b.m_repeatY) {
        return false;
    }

    if (a.m_positionX != b.m_positionX) {
        return false;
    }

    if (a.m_positionY != b.m_positionY) {
        return false;
    }

    if (a.m_attachment != b.m_attachment) {
        return false;
    }

    if (a.m_clip != b.m_clip) {
        return false;
    }

    if (a.m_origin != b.m_origin) {
        return false;
    }

    return true;
}

bool operator!=(const BackgroundLayer& a, const BackgroundLayer& b)
{
    return !operator==(a, b);
}

bool operator==(const StyleBackgroundData& a, const StyleBackgroundData& b)
{
    if (a.m_color != b.m_color) {
        return false;
    }

    if (a.m_layers.size() != b.m_layers.size()) {
        return false;
    }

    for (unsigned int i = 0; i < a.m_layers.size(); i++) {
        if (a.m_layers[i] != b.m_layers[i]) {
            return false;
        }
    }

    return true;
}

bool operator!=(const StyleBackgroundData& a, const StyleBackgroundData& b)
{
    return !operator==(a, b);
}
}

#endif
