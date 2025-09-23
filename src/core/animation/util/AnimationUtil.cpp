/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#include <SkMatrix.h>

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/animation/AnimationTask.h"
#include "core/animation/util/AnimationUtil.h"
#include "core/dom/Document.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameDocument.h"
#include "core/style/ComputedStyle.h"
#include "core/modules/canvas/image/NativeImageData.h"

namespace Starfish {

static void baseSizeForBackground(FrameBox* box, ComputedStyle* style,
                                  uint32_t layer, float& baseW, float& baseH)
{
    baseW = baseH = 0;
    if (style->backgroundImage(layer)->type().isURL()) {
        NativeImageData* id = style->backgroundImageData(layer);
        if (id) {
            baseW = id->width();
            baseH = id->height();
        }
    } else if (style->backgroundImage(layer)->type().isGradient()) {
        // TODO rootOrBodyelement
        Unit::Rect rect = box->makeRect(BoxValue::PaddingBoxBoxValue);
        baseW = rect.width();
        baseH = rect.height();
    }
}

void AnimationUtil::calculateBackgroundBaseData(FrameBox* box,
                                                ComputedStyle* style,
                                                uint32_t layer,
                                                Unit::Size& positioningSize,
                                                Unit::Size& imageSize)
{
    Unit::Rect positioningRect;
    BackgroundAttachmentValue attachment = style->backgroundAttachment(layer);
    if (attachment == FixedBackgroundAttachmentValue) {
        FrameDocument* doc = box->document()->frame()->asFrameDocument();
        positioningRect = doc->makeRect(style->backgroundOrigin(layer));
    } else if (attachment == LocalBackgroundAttachmentValue &&
               box->isFrameBlockBox()) {
        FrameBox scrollBox(box->node(), style);
        scrollBox.copyFrom(box, FrameBox::BorderCopy | FrameBox::PaddingCopy);
        scrollBox.setWidth(box->asFrameBlockBox()->scrollWidth());
        scrollBox.setHeight(box->asFrameBlockBox()->scrollHeight());
        positioningRect = scrollBox.makeRect(style->backgroundOrigin(layer));
    } else {
        positioningRect = box->makeRect(style->backgroundOrigin(layer));
    }

    float positionW = positioningRect.width();
    float positionH = positioningRect.height();
    float boxR = positionW / positionH;
    float baseW = 0, baseH = 0;
    baseSizeForBackground(box, style, layer, baseW, baseH);

    float sizeW = baseW, sizeH = baseH;
    if (baseW && baseH) {
        float imgR = baseW / baseH;
        if (style->backgroundSizeIsLength(layer)) {
            LengthSize bgSize = style->backgroundSizeLengthValue(layer);
            if (bgSize.width().isAuto() && bgSize.height().isAuto()) {
                sizeW = baseW;
                sizeH = baseH;
            } else if (bgSize.width().isAuto() && !bgSize.height().isAuto()) {
                sizeH = bgSize.height().specifiedValue(positionH, box);
                sizeW = sizeH * baseW / baseH;
            } else if (!bgSize.width().isAuto() && bgSize.height().isAuto()) {
                sizeW = bgSize.width().specifiedValue(positionW, box);
                sizeH = sizeW * baseH / baseW;
            } else {
                sizeW = bgSize.width().specifiedValue(positionW, box);
                sizeH = bgSize.height().specifiedValue(positionH, box);
            }
        } else {
            BackgroundSizeValue bgSize = style->backgroundSizeTypeValue(layer);
            if (bgSize == BackgroundSizeValue::CoverBackgroundSizeValue) {
                if (boxR < imgR) {
                    sizeW = positionH * imgR;
                } else {
                    sizeH = positionW / imgR;
                }
            } else {
                STARFISH_ASSERT(
                    bgSize == BackgroundSizeValue::ContainBackgroundSizeValue);
                if (boxR > imgR) {
                    sizeW = positionH * imgR;
                } else {
                    sizeH = positionW / imgR;
                }
            }
        }
    } else if (style->backgroundSizeIsLength(layer)) {
        LengthSize bgSize = style->backgroundSizeLengthValue(layer);
        if (bgSize.width().isDefinite(false)) {
            sizeW = bgSize.width().specifiedValue(0, box);
        }
        if (bgSize.height().isDefinite(false)) {
            sizeH = bgSize.height().specifiedValue(0, box);
        }
    }
    positioningSize.setWidth(positionW);
    positioningSize.setHeight(positionH);
    imageSize.setWidth(sizeW);
    imageSize.setHeight(sizeH);
}

bool AnimationUtil::backgroundSizeToAnimatedValue(
    ComputedStyle* oldStyle, ComputedStyle* newStyle, FrameBox* oldPaintingBox,
    Element* element, AnimatedValue& from, AnimatedValue& to, uint32_t layer)
{
    STARFISH_ASSERT(newStyle->background());
    STARFISH_ASSERT(newStyle->backgroundImage(layer));

    if (!newStyle->backgroundImage(layer)->type().isURL()) {
        // TODO support gradient
        return false;
    }
    if (!newStyle->backgroundSizeIsLength(layer)) {
        return false;
    }
    if (newStyle->background()->equalsSize(oldStyle->background(), layer)) {
        return false;
    }

    const LengthSize& oldSize = oldStyle->backgroundSizeLengthValue(layer);
    const LengthSize& newSize = newStyle->backgroundSizeLengthValue(layer);
    const Length& newW = newSize.width();
    const Length& newH = newSize.height();

    if (((newW.isFixed() || newW.isPercent()) &&
         newW.type() == oldSize.width().type()) &&
        ((newH.isFixed() || newH.isPercent()) &&
         newH.type() == oldSize.height().type())) {
        from = oldSize;
        to = newSize;
        return true;
    }

    // Align types to Fixed/Percent
    Length oldModifiedW, oldModifiedH, newModifiedW, newModifiedH;
    Unit::Size posSize, imgSize;
    bool hasBaseData = false;

#define CALCULATE_BASE_IF_NEED()                                              \
    if (!hasBaseData) {                                                       \
        hasBaseData = true;                                                   \
        calculateBackgroundBaseData(oldPaintingBox, oldStyle, layer, posSize, \
                                    imgSize);                                 \
    }

    if (newW.isDefinite(false)) {
        CALCULATE_BASE_IF_NEED();
        oldModifiedW = Length(Length::Fixed, imgSize.width());
        newModifiedW = Length(Length::Fixed, newW.specifiedValue(0, element));
    } else if (newW.isPercent()) {
        CALCULATE_BASE_IF_NEED();
        oldModifiedW = Length(
            Length::Percent,
            posSize.width() != 0 ? imgSize.width() / posSize.width() : 0);
        newModifiedW = newW;
    } else if (newW.isAuto() && !newH.isAuto()) {
        oldModifiedW = Length();
        newModifiedW = newW;
    } else {
        return false;
    }
    if (newH.isDefinite(false)) {
        CALCULATE_BASE_IF_NEED();
        oldModifiedH = Length(Length::Fixed, imgSize.height());
        newModifiedH = Length(Length::Fixed, newH.specifiedValue(0, element));
    } else if (newH.isPercent()) {
        CALCULATE_BASE_IF_NEED();
        oldModifiedH = Length(
            Length::Percent,
            posSize.height() != 0 ? imgSize.height() / posSize.height() : 0);
        newModifiedH = newH;
    } else if (newH.isAuto()) {
        oldModifiedH = Length();
        newModifiedH = newH;
    } else {
        return false;
    }
#undef CALCULATE_BASE_IF_NEED

    from = LengthSize(oldModifiedW, oldModifiedH);
    to = LengthSize(newModifiedW, newModifiedH);
    return true;
}

bool AnimationUtil::backgroundPosXToAnimatedValue(
    ComputedStyle* oldStyle, ComputedStyle* newStyle, FrameBox* oldPaintingBox,
    Element* element, AnimatedValue& from, AnimatedValue& to, uint32_t layer)
{
    STARFISH_ASSERT(newStyle->backgroundImage(layer));
    ImageValueType newType = newStyle->backgroundImage(layer)->type();
    if (oldStyle->backgroundImage(layer)->type() != newType ||
        !(newType.isURL() || newType.isGradient())) {
        return false;
    }
    const Length& oldPosX = oldStyle->backgroundPositionX(layer);
    const Length& newPosX = newStyle->backgroundPositionX(layer);
    if (oldPosX == newPosX) {
        return false;
    }

    if (newPosX.isDefinite(false)) {
        // Value to Fixed
        if (oldPosX.isDefinite(false)) {
            from = Length(Length::Fixed, oldPosX.specifiedValue(0, element));
        } else {
            Unit::Size posSize, imgSize;
            calculateBackgroundBaseData(oldPaintingBox, oldStyle, layer,
                                        posSize, imgSize);
            from = Length(Length::Fixed,
                          oldPosX.specifiedValue(
                              posSize.width() - imgSize.width(), element));
        }
        to = Length(Length::Fixed, newPosX.specifiedValue(0, element));
    } else if (newPosX.isPercent()) {
        // Value to Percent
        if (oldPosX.isPercent()) {
            from = oldPosX;
        } else {
            Unit::Size posSize, imgSize;
            calculateBackgroundBaseData(oldPaintingBox, oldStyle, layer,
                                        posSize, imgSize);
            float constantA = posSize.width() - imgSize.width();
            float inFixed = oldPosX.specifiedValue(constantA, element);
            from = Length(Length::Percent, inFixed / constantA);
        }
        to = newPosX;
    } else {
        // Does not support non-Length type calc() yet
        return false;
    }
    return true;
}

bool AnimationUtil::backgroundPosYToAnimatedValue(
    ComputedStyle* oldStyle, ComputedStyle* newStyle, FrameBox* oldPaintingBox,
    Element* element, AnimatedValue& from, AnimatedValue& to, uint32_t layer)
{
    STARFISH_ASSERT(newStyle->backgroundImage(layer));
    ImageValueType newType = newStyle->backgroundImage(layer)->type();
    if (oldStyle->backgroundImage(layer)->type() != newType ||
        !(newType.isURL() || newType.isGradient())) {
        return false;
    }
    const Length& oldPosY = oldStyle->backgroundPositionY(layer);
    const Length& newPosY = newStyle->backgroundPositionY(layer);
    if (oldPosY == newPosY) {
        return false;
    }

    if (newPosY.isDefinite(false)) {
        // Value to Fixed
        if (oldPosY.isDefinite(false)) {
            from = Length(Length::Fixed, oldPosY.specifiedValue(0, element));
        } else {
            Unit::Size posSize, imgSize;
            calculateBackgroundBaseData(oldPaintingBox, oldStyle, layer,
                                        posSize, imgSize);
            from = Length(Length::Fixed,
                          oldPosY.specifiedValue(
                              posSize.height() - imgSize.height(), element));
        }
        to = Length(Length::Fixed, newPosY.specifiedValue(0, element));
    } else if (newPosY.isPercent()) {
        // Value to Percent
        if (oldPosY.isPercent()) {
            from = oldPosY;
        } else {
            Unit::Size posSize, imgSize;
            calculateBackgroundBaseData(oldPaintingBox, oldStyle, layer,
                                        posSize, imgSize);
            float constantA = posSize.height() - imgSize.height();
            float inFixed = oldPosY.specifiedValue(constantA, element);
            from = Length(Length::Percent, inFixed / constantA);
        }
        to = newPosY;
    } else {
        // Does not support non-Length type calc() yet
        return false;
    }
    return true;
}

bool AnimationUtil::lengthToAnimatedValue(const Length& oldLength,
                                          const Length& newLength,
                                          Element* element, AnimatedValue& from,
                                          AnimatedValue& to)
{
    if (!oldLength.isDefinite(false) || !newLength.isDefinite(false)) {
        return false;
    }
    from = Length(Length::Fixed, oldLength.specifiedValue(0, element));
    to = Length(Length::Fixed, newLength.specifiedValue(0, element));
    return true;
}

bool AnimationUtil::isPropertyForActiveColorAnimationTask(
    CSSStyleValuePair::KeyKind keyKind)
{
    switch (keyKind) {
    case CSSStyleValuePair::KeyKind::BackgroundColor:
    case CSSStyleValuePair::KeyKind::BorderBottomColor:
    case CSSStyleValuePair::KeyKind::BorderLeftColor:
    case CSSStyleValuePair::KeyKind::BorderRightColor:
    case CSSStyleValuePair::KeyKind::BorderTopColor:
    case CSSStyleValuePair::KeyKind::Color:
    case CSSStyleValuePair::KeyKind::CaretColor:
    case CSSStyleValuePair::KeyKind::OutlineColor:
    case CSSStyleValuePair::KeyKind::TextDecorationColor:
    case CSSStyleValuePair::KeyKind::TextDecoration: // shorthand
        return true;
    default:
        return false;
    }
    return false;
}

bool AnimationUtil::isPropertyForActiveLengthAnimationTask(
    CSSStyleValuePair::KeyKind keyKind)
{
    switch (keyKind) {
    case CSSStyleValuePair::KeyKind::Width:
    case CSSStyleValuePair::KeyKind::Height:
    case CSSStyleValuePair::KeyKind::MinWidth:
    case CSSStyleValuePair::KeyKind::MaxWidth:
    case CSSStyleValuePair::KeyKind::MinHeight:
    case CSSStyleValuePair::KeyKind::MaxHeight:
    case CSSStyleValuePair::KeyKind::MarginTop:
    case CSSStyleValuePair::KeyKind::MarginRight:
    case CSSStyleValuePair::KeyKind::MarginBottom:
    case CSSStyleValuePair::KeyKind::MarginLeft:
    case CSSStyleValuePair::KeyKind::BorderTopWidth:
    case CSSStyleValuePair::KeyKind::BorderRightWidth:
    case CSSStyleValuePair::KeyKind::BorderBottomWidth:
    case CSSStyleValuePair::KeyKind::BorderLeftWidth:
    case CSSStyleValuePair::KeyKind::PaddingTop:
    case CSSStyleValuePair::KeyKind::PaddingRight:
    case CSSStyleValuePair::KeyKind::PaddingBottom:
    case CSSStyleValuePair::KeyKind::PaddingLeft:
    case CSSStyleValuePair::KeyKind::X:
    case CSSStyleValuePair::KeyKind::Y:
    case CSSStyleValuePair::KeyKind::RX:
    case CSSStyleValuePair::KeyKind::RY:
    case CSSStyleValuePair::KeyKind::CX:
    case CSSStyleValuePair::KeyKind::CY:
    case CSSStyleValuePair::KeyKind::Left:
    case CSSStyleValuePair::KeyKind::Right:
    case CSSStyleValuePair::KeyKind::Top:
    case CSSStyleValuePair::KeyKind::Bottom:
    case CSSStyleValuePair::KeyKind::BackgroundPositionX:
    case CSSStyleValuePair::KeyKind::BackgroundPositionY:
    case CSSStyleValuePair::KeyKind::FontSize:
    case CSSStyleValuePair::KeyKind::Font: // shorthand
        return true;
    default:
        return false;
    }
    return false;
}

bool AnimationUtil::isPropertyForActiveLengthAnimationTaskForSVG(
    StaticStrings& ss, AtomicString attrName)
{
    if (ss.m_width == attrName) {
        return true;
    } else if (ss.m_height == attrName) {
        return true;
    } else if (ss.m_x == attrName) {
        return true;
    } else if (ss.m_y == attrName) {
        return true;
    } else if (ss.m_cx == attrName) {
        return true;
    } else if (ss.m_cy == attrName) {
        return true;
    } else if (ss.m_rx == attrName) {
        return true;
    } else if (ss.m_ry == attrName) {
        return true;
    } else if (ss.m_r == attrName) {
        return true;
    }
    return false;
}

bool AnimationUtil::isPropertyForActiveLengthSizeAnimationTask(
    CSSStyleValuePair::KeyKind keyKind)
{
    // Currently only BackgroundSize is supported.
    return keyKind == CSSStyleValuePair::KeyKind::BackgroundSize;
}

} // namespace Starfish
