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
        : m_image(nullptr)
        , m_imageResource(nullptr)
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

    void setSize(const LengthSize& size)
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

    void setImage(ImageValue* image)
    {
        m_image = image;
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

    void resetSize()
    {
        m_size.m_lengthValue = nullptr;
    }

    void resetImage()
    {
        m_image = nullptr;
        m_imageResource = nullptr;
    }

    void resetRepeatX()
    {
        m_repeatX = RepeatRepeatValue;
    }

    void resetRepeatY()
    {
        m_repeatY = RepeatRepeatValue;
    }

    void resetPositionX()
    {
        m_positionX = Length(Length::Percent, 0.0f);
    }

    void resetPositionY()
    {
        m_positionY = Length(Length::Percent, 0.0f);
    }

    void resetAttachment()
    {
        m_attachment = ScrollBackgroundAttachmentValue;
    }

    void resetClip()
    {
        m_clip = BorderBoxBoxValue;
    }

    void resetOrigin()
    {
        m_origin = PaddingBoxBoxValue;
    }

    ImageValue* image() const
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
                       LayoutSize windowSize, ComputedStyle* cs);

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

    ImageValue* m_image;
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
        , m_maxLayerImage(0)
        , m_maxLayerRepeatX(0)
        , m_maxLayerRepeatY(0)
        , m_maxLayerSize(0)
        , m_maxLayerPositionX(0)
        , m_maxLayerPositionY(0)
        , m_maxLayerAttachment(0)
        , m_maxLayerClip(0)
        , m_maxLayerOrigin(0)
    {
    }

    ~StyleBackgroundData()
    {
    }

    static bool damaged(const StyleBackgroundData* before,
                        const StyleBackgroundData* after, bool& attach,
                        bool& clip, bool& img, bool& origin, bool& size,
                        bool& repX, bool& repY, bool& posX, bool& posY)
    {
        attach = clip = img = origin = size = repX = repY = posX = posY = false;
        if (!before && !after) {
            return false;
        }
        const StyleBackgroundData tempFiller;
        before = before ? before : &tempFiller;
        after = after ? after : &tempFiller;
        size_t maxLayer =
            std::max(before->m_maxLayerImage, after->m_maxLayerImage);
        bool hasDamage = false;
        for (size_t i = 0; i < maxLayer; i++) {
            if (!attach && before->attachment(i) != after->attachment(i)) {
                attach = hasDamage = true;
            }
            if (!clip && before->clip(i) != after->clip(i)) {
                clip = hasDamage = true;
            }
            if (!img) {
                const ImageValue* aimg = before->image(i);
                const ImageValue* bimg = after->image(i);
                if (aimg != bimg && (!aimg || !bimg || *aimg != *bimg)) {
                    img = hasDamage = true;
                }
            }
            if (!origin && before->origin(i) != after->origin(i)) {
                origin = hasDamage = true;
            }
            if (!size && !before->equalsSize(after, i)) {
                size = hasDamage = true;
            }
            if (!repX && before->repeatX(i) != after->repeatX(i)) {
                repX = hasDamage = true;
            }
            if (!repY && before->repeatY(i) != after->repeatY(i)) {
                repY = hasDamage = true;
            }
            if (!posX && before->positionX(i) != after->positionX(i)) {
                posX = hasDamage = true;
            }
            if (!posY && before->positionY(i) != after->positionY(i)) {
                posY = hasDamage = true;
            }
        }
        return hasDamage;
    }

    void setColor(Unit::Color color)
    {
        m_color = color;
        m_bgColorNeedToUpdate = false;
    }

    void setColorToCurrentColor()
    {
        m_bgColorNeedToUpdate = true;
    }

    uint16_t assureLayerIndex(uint32_t index) const
    {
        uint16_t maxsize = narrow_cast<uint32_t, uint16_t>(index + 1);
        return maxsize - 1;
    }

    uint16_t assureLayerIndexAndSize(uint32_t index, uint16_t& currentMax)
    {
        uint16_t maxsize = narrow_cast<uint32_t, uint16_t>(index + 1);
        if (m_layers.size() < maxsize) {
            m_layers.resize(maxsize);
        }
        if (currentMax < maxsize) {
            currentMax = maxsize;
        }
        STARFISH_ASSERT(maxsize > 0);
        return maxsize - 1;
    }

    void setSize(BackgroundSizeValue size, uint32_t index)
    {
        uint16_t assured = assureLayerIndexAndSize(index, m_maxLayerSize);
        m_layers[assured].setSize(size);
    }

    void setSize(const LengthSize& size, uint32_t index)
    {
        uint16_t assured = assureLayerIndexAndSize(index, m_maxLayerSize);
        m_layers[assured].setSize(size);
    }

    void setImage(ImageValue* image, uint32_t index)
    {
        uint16_t assured = assureLayerIndexAndSize(index, m_maxLayerImage);
        m_layers[assured].setImage(image);
    }

    void setImageResource(ImageResource* data, uint32_t index)
    {
        uint16_t assured = assureLayerIndexAndSize(index, m_maxLayerImage);
        m_layers[assured].setImageResource(data);
    }

    void setRepeatX(BackgroundRepeatValue repeat, uint32_t index)
    {
        uint16_t assured = assureLayerIndexAndSize(index, m_maxLayerRepeatX);
        m_layers[assured].setRepeatX(repeat);
    }

    void setRepeatY(BackgroundRepeatValue repeat, uint32_t index)
    {
        uint16_t assured = assureLayerIndexAndSize(index, m_maxLayerRepeatY);
        m_layers[assured].setRepeatY(repeat);
    }

    void setPositionX(Length position, uint32_t index)
    {
        uint16_t assured = assureLayerIndexAndSize(index, m_maxLayerPositionX);
        m_layers[assured].setPositionX(position);
    }

    void setPositionY(Length position, uint32_t index)
    {
        uint16_t assured = assureLayerIndexAndSize(index, m_maxLayerPositionY);
        m_layers[assured].setPositionY(position);
    }

    void setAttachment(BackgroundAttachmentValue attachment, uint32_t index)
    {
        uint16_t assured = assureLayerIndexAndSize(index, m_maxLayerAttachment);
        m_layers[assured].setAttachment(attachment);
    }

    void setClip(BoxValue clip, uint32_t index)
    {
        uint16_t assured = assureLayerIndexAndSize(index, m_maxLayerClip);
        m_layers[assured].setClip(clip);
    }

    void setOrigin(BoxValue origin, uint32_t index)
    {
        uint16_t assured = assureLayerIndexAndSize(index, m_maxLayerOrigin);
        m_layers[assured].setOrigin(origin);
    }

    Unit::Color color()
    {
        return m_color;
    }

    ImageValue* image(uint32_t index) const
    {
        uint16_t assured = assureLayerIndex(index);
        if (assured < m_maxLayerImage) {
            return m_layers[assured].image();
        }
        return nullptr;
    }

    NativeImageData* imageData(uint32_t index) const
    {
        uint16_t assured = assureLayerIndex(index);
        if (assured < m_maxLayerImage) {
            return m_layers[assured].imageData();
        }
        return nullptr;
    }

    ImageResource* imageResource(uint32_t index) const
    {
        uint16_t assured = assureLayerIndex(index);
        if (assured < m_maxLayerImage) {
            return m_layers[assured].imageResource();
        }
        return nullptr;
    }

    // NOTE for property getters
    // If a property doesn’t have enough values to match the number of layers,
    // the UA must calculate its used value by repeating the list of values
    // until there are enough.

    BackgroundRepeatValue repeatX(uint32_t index) const
    {
        if (m_maxLayerRepeatX == 0) {
            return RepeatRepeatValue;
        }
        uint16_t p = assureLayerIndex(index) % m_maxLayerRepeatX;
        return m_layers[p].repeatX();
    }

    BackgroundRepeatValue repeatY(uint32_t index) const
    {
        if (m_maxLayerRepeatY == 0) {
            return RepeatRepeatValue;
        }
        uint16_t p = assureLayerIndex(index) % m_maxLayerRepeatY;
        return m_layers[p].repeatY();
    }

    bool sizeIsLength(uint32_t index) const
    {
        if (m_maxLayerSize == 0) {
            return true;
        }
        uint16_t p = assureLayerIndex(index) % m_maxLayerSize;
        return m_layers[p].m_size.hasLengthValue();
    }

    BackgroundSizeValue sizeTypeValue(uint32_t index) const
    {
        STARFISH_ASSERT(!sizeIsLength(index));
        uint16_t p = assureLayerIndex(index) % m_maxLayerSize;
        return m_layers[p].m_size.m_typeValue;
    }

    LengthSize sizeLengthValue(uint32_t index) const
    {
        if (m_maxLayerSize == 0) {
            return LengthSize();
        }
        uint16_t p = assureLayerIndex(index) % m_maxLayerSize;
        return m_layers[p].sizeLengthValue();
    }

    BackgroundAttachmentValue attachment(uint32_t index) const
    {
        if (m_maxLayerAttachment == 0) {
            return ScrollBackgroundAttachmentValue;
        }
        uint16_t p = assureLayerIndex(index) % m_maxLayerAttachment;
        return m_layers[p].attachment();
    }

    BoxValue clip(uint32_t index) const
    {
        if (m_maxLayerClip == 0) {
            return BorderBoxBoxValue;
        }
        uint16_t p = assureLayerIndex(index) % m_maxLayerClip;
        return m_layers[p].clip();
    }

    BoxValue origin(uint32_t index) const
    {
        if (m_maxLayerOrigin == 0) {
            return PaddingBoxBoxValue;
        }
        uint16_t p = assureLayerIndex(index) % m_maxLayerOrigin;
        return m_layers[p].origin();
    }

    Length positionX(uint32_t index) const
    {
        if (m_maxLayerPositionX == 0) {
            return Length(Length::Percent, 0.0f);
        }
        uint16_t p = assureLayerIndex(index) % m_maxLayerPositionX;
        return m_layers[p].positionX();
    }

    Length positionY(uint32_t index) const
    {
        if (m_maxLayerPositionY == 0) {
            return Length(Length::Percent, 0.0f);
        }
        uint16_t p = assureLayerIndex(index) % m_maxLayerPositionY;
        return m_layers[p].positionY();
    }

    void shrinkImages(uint32_t newsize)
    {
        STARFISH_ASSERT(m_layers.size() >= m_maxLayerImage);
        uint16_t assuredSize = narrow_cast<uint32_t, uint16_t>(newsize);
        for (uint16_t i = assuredSize; i < m_maxLayerImage; i++) {
            m_layers[i].resetImage();
        }
        m_maxLayerImage = assuredSize;
    }

    void shrinkRepeatXs(uint32_t newsize)
    {
        STARFISH_ASSERT(m_layers.size() >= m_maxLayerRepeatX);
        uint16_t assuredSize = narrow_cast<uint32_t, uint16_t>(newsize);
        for (uint16_t i = assuredSize; i < m_maxLayerRepeatX; i++) {
            m_layers[i].resetRepeatX();
        }
        m_maxLayerRepeatX = assuredSize;
    }

    void shrinkRepeatYs(uint32_t newsize)
    {
        STARFISH_ASSERT(m_layers.size() >= m_maxLayerRepeatY);
        uint16_t assuredSize = narrow_cast<uint32_t, uint16_t>(newsize);
        for (uint16_t i = assuredSize; i < m_maxLayerRepeatY; i++) {
            m_layers[i].resetRepeatY();
        }
        m_maxLayerRepeatY = assuredSize;
    }

    void shrinkSizes(uint32_t newsize)
    {
        STARFISH_ASSERT(m_layers.size() >= m_maxLayerSize);
        uint16_t assuredSize = narrow_cast<uint32_t, uint16_t>(newsize);
        for (uint16_t i = assuredSize; i < m_maxLayerSize; i++) {
            m_layers[i].resetSize();
        }
        m_maxLayerSize = assuredSize;
    }

    void shrinkPositionXs(uint32_t newsize)
    {
        STARFISH_ASSERT(m_layers.size() >= m_maxLayerPositionX);
        uint16_t assuredSize = narrow_cast<uint32_t, uint16_t>(newsize);
        for (uint16_t i = assuredSize; i < m_maxLayerPositionX; i++) {
            m_layers[i].resetPositionX();
        }
        m_maxLayerPositionX = assuredSize;
    }

    void shrinkPositionYs(uint32_t newsize)
    {
        STARFISH_ASSERT(m_layers.size() >= m_maxLayerPositionY);
        uint16_t assuredSize = narrow_cast<uint32_t, uint16_t>(newsize);
        for (uint16_t i = assuredSize; i < m_maxLayerPositionY; i++) {
            m_layers[i].resetPositionY();
        }
        m_maxLayerPositionY = assuredSize;
    }

    void shrinkAttachments(uint32_t newsize)
    {
        STARFISH_ASSERT(m_layers.size() >= m_maxLayerAttachment);
        uint16_t assuredSize = narrow_cast<uint32_t, uint16_t>(newsize);
        for (uint16_t i = assuredSize; i < m_maxLayerAttachment; i++) {
            m_layers[i].resetAttachment();
        }
        m_maxLayerAttachment = assuredSize;
    }

    void shrinkClips(uint32_t newsize)
    {
        STARFISH_ASSERT(m_layers.size() >= m_maxLayerClip);
        uint16_t assuredSize = narrow_cast<uint32_t, uint16_t>(newsize);
        for (uint16_t i = assuredSize; i < m_maxLayerClip; i++) {
            m_layers[i].resetClip();
        }
        m_maxLayerClip = assuredSize;
    }

    void shrinkOrigins(uint32_t newsize)
    {
        STARFISH_ASSERT(m_layers.size() >= m_maxLayerOrigin);
        uint16_t assuredSize = narrow_cast<uint32_t, uint16_t>(newsize);
        for (uint16_t i = assuredSize; i < m_maxLayerOrigin; i++) {
            m_layers[i].resetOrigin();
        }
        m_maxLayerOrigin = assuredSize;
    }

    bool equalsSize(const StyleBackgroundData* b, uint32_t layer) const
    {
        bool isLength = sizeIsLength(layer);
        if (!b) {
            return isLength && sizeLengthValue(layer) == LengthSize();
        }
        if (isLength != b->sizeIsLength(layer)) {
            return false;
        }
        if (isLength) {
            return sizeLengthValue(layer) == b->sizeLengthValue(layer);
        }
        return sizeTypeValue(layer) == b->sizeTypeValue(layer);
    }

    void checkComputed(Unit::Color color)
    {
        // background-color
        // - default : transparent
        // - currentColor : represents the "calculated" value of the element's
        // color property
        if (m_bgColorNeedToUpdate) {
            setColor(color);
        }
    }

    void checkComputed(Length curFontSize, Length rootFontSize, Font* font,
                       LayoutSize windowSize, ComputedStyle* cs)
    {
        if (m_layers.size()) {
            for (uint32_t i = 0; i < m_layers.size(); i++) {
                m_layers[i].checkComputed(curFontSize, rootFontSize, font,
                                          windowSize, cs);
            }
        }
    }

    uint32_t sizeOfLayers()
    {
        STARFISH_ASSERT(m_maxLayerImage <= m_layers.size());
        return m_maxLayerImage;
    }

private:
    friend inline bool operator==(const StyleBackgroundData& a,
                                  const StyleBackgroundData& b);
    friend inline bool operator!=(const StyleBackgroundData& a,
                                  const StyleBackgroundData& b);

    Unit::Color m_color;
    // background-color type
    bool m_bgColorNeedToUpdate : 1;
    uint16_t m_maxLayerImage;
    uint16_t m_maxLayerRepeatX;
    uint16_t m_maxLayerRepeatY;
    uint16_t m_maxLayerSize;
    uint16_t m_maxLayerPositionX;
    uint16_t m_maxLayerPositionY;
    uint16_t m_maxLayerAttachment;
    uint16_t m_maxLayerClip;
    uint16_t m_maxLayerOrigin;

    GCVector<BackgroundLayer> m_layers;
};

bool operator==(const BackgroundLayer& a, const BackgroundLayer& b)
{
    if (a.m_image == b.m_image) {
    } else if (a.m_image == nullptr || b.m_image == nullptr) {
        return false;
    } else if (!(*a.m_image == *b.m_image)) {
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
