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

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/style/PositionedMaskData.h"

namespace Starfish {

MaskLayer::MaskLayer()
    : m_image(nullptr)
    , m_imageResource(nullptr)
    , m_repeatX(RepeatStyleValue::RepeatRepeatValue)
    , m_repeatY(RepeatStyleValue::RepeatRepeatValue)
    , m_sizeIsLength(true)
{
}

void MaskLayer::setSize(LengthSize size)
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

bool MaskLayer::operator==(const MaskLayer& other)
{
    // TODO: Add implementations for the rest of the CSS masking properties
    if ((m_image && other.m_image) && !(*m_image == *other.m_image)) {
        return false;
    } else if (m_image == nullptr || other.m_image == nullptr) {
        return false;
    }

    if (m_sizeIsLength != other.m_sizeIsLength) {
        return false;
    }

    if (m_sizeIsLength) {
        if ((m_size.m_lengthValue && other.m_size.m_lengthValue) &&
            !(*m_size.m_lengthValue == *other.m_size.m_lengthValue)) {
            return false;
        } else if (m_size.m_lengthValue == nullptr ||
                   other.m_size.m_lengthValue == nullptr) {
            return false;
        }
    } else {
        if (m_size.m_typeValue != other.m_size.m_typeValue) {
            return false;
        }
    }

    if (m_positionX != other.m_positionX || m_positionY != other.m_positionY) {
        return false;
    }

    if (m_repeatX != other.m_repeatX || m_repeatY != other.m_repeatY) {
        return false;
    }

    return true;
}

bool PositionedMaskData::damaged(const PositionedMaskData* lhs,
                                 const PositionedMaskData* rhs,
                                 bool* damagedKeys)
{
    // TODO: Add implementations for the rest of the CSS masking properties
    damagedKeys[CSSStyleValuePair::KeyKind::MaskImage] = false;
    damagedKeys[CSSStyleValuePair::KeyKind::MaskPositionX] = false;
    damagedKeys[CSSStyleValuePair::KeyKind::MaskPositionY] = false;
    damagedKeys[CSSStyleValuePair::KeyKind::MaskRepeatX] = false;
    damagedKeys[CSSStyleValuePair::KeyKind::MaskRepeatY] = false;

    if (!lhs && !rhs) {
        return false;
    }

    PositionedMaskData temp;
    lhs = lhs ? lhs : &temp;
    rhs = rhs ? rhs : &temp;
    uint32_t maxLayer = std::max(lhs->m_maxLayerImage, rhs->m_maxLayerImage);
    bool hasDamage = false;
    for (uint32_t i = 0; i < maxLayer; i++) {
        if (damagedKeys[CSSStyleValuePair::KeyKind::MaskImage] == false) {
            const ImageValue* lImage = lhs->image(i);
            const ImageValue* rImage = rhs->image(i);
            if (lImage != rImage &&
                (!lImage || !rImage || *lImage != *rImage)) {
                damagedKeys[CSSStyleValuePair::KeyKind::MaskImage] = hasDamage =
                    true;
            }

            if (lhs->maskSizeIsLength(i) != rhs->maskSizeIsLength(i)) {
                damagedKeys[CSSStyleValuePair::KeyKind::MaskSize] = hasDamage =
                    true;
            } else if (lhs->maskSizeIsLength(i)) {
                if (lhs->maskSizeLengthValue(i) != rhs->maskSizeLengthValue(i))
                    damagedKeys[CSSStyleValuePair::KeyKind::MaskSize] =
                        hasDamage = true;

            } else if (lhs->maskSizeTypeValue(i) != rhs->maskSizeTypeValue(i)) {
                damagedKeys[CSSStyleValuePair::KeyKind::MaskSize] = hasDamage =
                    true;
            }

            if (lhs->positionX(i) != rhs->positionX(i)) {
                damagedKeys[CSSStyleValuePair::KeyKind::MaskPositionX] =
                    hasDamage = true;
            }

            if (lhs->positionY(i) != rhs->positionY(i)) {
                damagedKeys[CSSStyleValuePair::KeyKind::MaskPositionY] =
                    hasDamage = true;
            }

            if (lhs->repeatX(i) != rhs->repeatX(i)) {
                damagedKeys[CSSStyleValuePair::KeyKind::MaskRepeatX] =
                    hasDamage = true;
            }

            if (lhs->repeatY(i) != rhs->repeatY(i)) {
                damagedKeys[CSSStyleValuePair::KeyKind::MaskRepeatY] =
                    hasDamage = true;
            }
        }
    }
    return hasDamage;
}

PositionedMaskData::PositionedMaskData()
    : m_maxLayerSize(0)
    , m_maxLayerImage(0)
    , m_maxLayerPositionX(0)
    , m_maxLayerPositionY(0)
    , m_maxLayerRepeatX(0)
    , m_maxLayerRepeatY(0)
{
}

ImageValue* PositionedMaskData::image(uint32_t layer) const
{
    if (m_layers.size() <= layer) {
        return nullptr;
    }
    return m_layers[layer].image();
}

Length PositionedMaskData::positionX(uint32_t layer) const
{
    if (m_layers.size() <= layer) {
        return Length();
    }
    return m_layers[layer].positionX();
}

Length PositionedMaskData::positionY(uint32_t layer) const
{
    if (m_layers.size() <= layer) {
        return Length();
    }
    return m_layers[layer].positionY();
}

RepeatStyleValue PositionedMaskData::repeatX(uint32_t layer) const
{
    if (m_layers.size() <= layer) {
        return RepeatRepeatValue;
    }
    return m_layers[layer].repeatX();
}

RepeatStyleValue PositionedMaskData::repeatY(uint32_t layer) const
{
    if (m_layers.size() <= layer) {
        return RepeatRepeatValue;
    }
    return m_layers[layer].repeatY();
}

MaskTypeValue PositionedMaskData::maskType() const
{
    return m_maskType;
}

ImageResource* PositionedMaskData::imageResource(uint32_t layer) const
{
    if (m_layers.size() <= layer) {
        return nullptr;
    }
    return m_layers[layer].imageResource();
}

void PositionedMaskData::setImage(ImageValue* value, uint32_t layer)
{
    // Note: transparent black image layer by default
    uint32_t assured = assureLayerIndexAndSize(layer, m_maxLayerImage);
    m_layers[assured].setImage(value);
}

void PositionedMaskData::setImageResource(ImageResource* imageResource,
                                          uint32_t layer)
{
    uint32_t assured = assureLayerIndexAndSize(layer, m_maxLayerImage);
    m_layers[assured].setImageResource(imageResource);
}

void PositionedMaskData::setSize(BackgroundSizeValue size, uint32_t layer)
{
    uint32_t assured = assureLayerIndexAndSize(layer, m_maxLayerSize);
    m_layers[assured].setSize(size);
}

void PositionedMaskData::setSize(LengthSize size, uint32_t layer)
{
    uint32_t assured = assureLayerIndexAndSize(layer, m_maxLayerSize);
    m_layers[assured].setSize(size);
}

void PositionedMaskData::setPositionX(Length value, uint32_t layer)
{
    uint32_t assured = assureLayerIndexAndSize(layer, m_maxLayerPositionX);
    m_layers[layer].setPositionX(value);
}

void PositionedMaskData::setPositionY(Length value, uint32_t layer)
{
    uint32_t assured = assureLayerIndexAndSize(layer, m_maxLayerPositionY);
    m_layers[layer].setPositionY(value);
}

void PositionedMaskData::setRepeatX(RepeatStyleValue repeat, uint32_t layer)
{
    uint32_t assured = assureLayerIndexAndSize(layer, m_maxLayerRepeatX);
    m_layers[layer].setRepeatX(repeat);
}

void PositionedMaskData::setRepeatY(RepeatStyleValue repeat, uint32_t layer)
{
    uint32_t assured = assureLayerIndexAndSize(layer, m_maxLayerRepeatY);
    m_layers[layer].setRepeatY(repeat);
}

void PositionedMaskData::setMaskType(MaskTypeValue maskType)
{
    m_maskType = maskType;
}

bool PositionedMaskData::maskSizeIsLength(uint32_t layer) const
{
    if (m_layers.size() <= layer) {
        return true;
    }
    return m_layers[layer].m_sizeIsLength;
}

LengthSize PositionedMaskData::maskSizeLengthValue(uint32_t layer) const
{
    if (m_layers.size() <= layer) {
        return LengthSize();
    }
    return m_layers[layer].sizeLengthValue();
}

BackgroundSizeValue PositionedMaskData::maskSizeTypeValue(uint32_t layer) const
{
    if (m_layers.size() <= layer) {
        return BackgroundSizeValue::ContainBackgroundSizeValue;
    }
    return m_layers[layer].sizeTypeValue();
}

void PositionedMaskData::shrinkImages(uint32_t size)
{
    STARFISH_ASSERT(m_layers.size() >= m_maxLayerImage);
    for (uint32_t i = 0; i < m_maxLayerImage; i++) {
        m_layers[i].setImage(nullptr);
    }
    m_maxLayerImage = size;
}

void PositionedMaskData::shrinkSizes(uint32_t size)
{
    STARFISH_ASSERT(m_layers.size() >= m_maxLayerSize);
    for (uint32_t i = 0; i < m_maxLayerSize; i++) {
        m_layers[i].resetSize();
    }
    m_maxLayerSize = size;
}

void PositionedMaskData::shrinkPositionXs(uint32_t size)
{
    STARFISH_ASSERT(m_layers.size() >= m_maxLayerPositionX);
    for (uint16_t i = 0; i < m_maxLayerPositionX; i++) {
        m_layers[i].resetPositionX();
    }
    m_maxLayerPositionX = size;
}

void PositionedMaskData::shrinkPositionYs(uint32_t size)
{
    STARFISH_ASSERT(m_layers.size() >= m_maxLayerPositionY);
    for (uint16_t i = 0; i < m_maxLayerPositionY; i++) {
        m_layers[i].resetPositionY();
    }
    m_maxLayerPositionY = size;
}

void PositionedMaskData::shrinkRepeatXs(uint32_t size)
{
    STARFISH_ASSERT(m_layers.size() >= m_maxLayerRepeatX);
    for (uint16_t i = 0; i < m_maxLayerRepeatX; i++) {
        m_layers[i].resetRepeatX();
    }
    m_maxLayerRepeatX = size;
}

void PositionedMaskData::shrinkRepeatYs(uint32_t size)
{
    STARFISH_ASSERT(m_layers.size() >= m_maxLayerRepeatY);
    for (uint16_t i = 0; i < m_maxLayerRepeatY; i++) {
        m_layers[i].resetRepeatY();
    }
    m_maxLayerRepeatY = size;
}

bool PositionedMaskData::operator==(const PositionedMaskData& other)
{
    if (m_layers.size() != other.m_layers.size()) {
        return false;
    }
    for (uint32_t i = 0; i < m_layers.size(); i++) {
        if (m_layers[i] != other.m_layers[i]) {
            return false;
        }
    }

    return true;
}

void* PositionedMaskData::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(PositionedMaskData));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(PositionedMaskData)] = { 0 };
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(PositionedMaskData, m_layers));
        descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(PositionedMaskData));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

uint32_t PositionedMaskData::assureLayerIndexAndSize(uint32_t layer,
                                                     uint32_t& currentMax)
{
    uint32_t maxSize = layer + 1;
    if (m_layers.size() < maxSize) {
        m_layers.resize(maxSize);
    }

    if (currentMax < maxSize) {
        currentMax = maxSize;
    }

    STARFISH_ASSERT(maxSize > 0);
    return maxSize - 1;
}
} // namespace Starfish
