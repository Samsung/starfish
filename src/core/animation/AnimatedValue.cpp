/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#include "AnimatedValue.h"

#include "core/style/ComputedStyle.h"
#include "core/style/CalcData.h"
#include "core/style/Length.h"
#include "core/style/LengthUtil.h"

namespace Starfish {

Optional<AnimatedValue*> AnimatedValue::create(
    ComputedStyle* style, Element* element, const CSSStyleValuePair& property,
    const CSSStyleValuePair::KeyKind& keyKind, size_t layer,
    bool neededOriginProperty)
{
    STARFISH_ASSERT(style != nullptr);

    switch (keyKind) {
    case CSSStyleValuePair::Color:
        if (neededOriginProperty) {
            return new AnimatedValue(style->color());
        }
        return AnimatedValue::createAnimatedValueFromColor(property);
    case CSSStyleValuePair::BackgroundColor:
        if (neededOriginProperty) {
            return new AnimatedValue(style->backgroundColor());
        }
        return AnimatedValue::createAnimatedValueFromColor(property);
    case CSSStyleValuePair::BorderBottomColor:
        if (neededOriginProperty) {
            return new AnimatedValue(style->border().bottom().color());
        }
        return AnimatedValue::createAnimatedValueFromColor(property);
    case CSSStyleValuePair::BorderLeftColor:
        if (neededOriginProperty) {
            return new AnimatedValue(style->border().left().color());
        }
        return AnimatedValue::createAnimatedValueFromColor(property);
    case CSSStyleValuePair::BorderRightColor:
        if (neededOriginProperty) {
            return new AnimatedValue(style->border().right().color());
        }
        return AnimatedValue::createAnimatedValueFromColor(property);
    case CSSStyleValuePair::BorderTopColor:
        if (neededOriginProperty) {
            return new AnimatedValue(style->border().top().color());
        }
        return AnimatedValue::createAnimatedValueFromColor(property);
    case CSSStyleValuePair::CaretColor:
        if (neededOriginProperty) {
            return new AnimatedValue(style->caretColor());
        }
        return AnimatedValue::createAnimatedValueFromColor(property);
    case CSSStyleValuePair::OutlineColor:
        if (neededOriginProperty) {
            return new AnimatedValue(style->outlineColor());
        }
        return AnimatedValue::createAnimatedValueFromColor(property);
    case CSSStyleValuePair::TextDecorationColor:
        if (neededOriginProperty) {
            return new AnimatedValue(style->textDecorationColor());
        }
        return AnimatedValue::createAnimatedValueFromColor(property);
    case CSSStyleValuePair::Width:
        if (neededOriginProperty) {
            return new AnimatedValue(style->width());
        }
        return AnimatedValue::createAnimatedValueFromLength(property);
    case CSSStyleValuePair::MaxWidth:
        if (neededOriginProperty) {
            return new AnimatedValue(style->maxWidth());
        }
        return AnimatedValue::createAnimatedValueFromLength(property);
    case CSSStyleValuePair::MinWidth:
        if (neededOriginProperty) {
            return new AnimatedValue(style->minWidth());
        }
        return AnimatedValue::createAnimatedValueFromLength(property);
    case CSSStyleValuePair::MarginTop:
        if (neededOriginProperty) {
            return new AnimatedValue(style->margin().top());
        }
        return AnimatedValue::createAnimatedValueFromLength(property);
    case CSSStyleValuePair::MarginRight:
        if (neededOriginProperty) {
            return new AnimatedValue(style->margin().right());
        }
        return AnimatedValue::createAnimatedValueFromLength(property);
    case CSSStyleValuePair::MarginBottom:
        if (neededOriginProperty) {
            return new AnimatedValue(style->margin().bottom());
        }
        return AnimatedValue::createAnimatedValueFromLength(property);
    case CSSStyleValuePair::MarginLeft:
        if (neededOriginProperty) {
            return new AnimatedValue(style->margin().left());
        }
        return AnimatedValue::createAnimatedValueFromLength(property);
    case CSSStyleValuePair::BorderTopWidth:
        if (neededOriginProperty) {
            return new AnimatedValue(style->border().top().width());
        }
        return AnimatedValue::createAnimatedValueFromLength(property);
    case CSSStyleValuePair::BorderRightWidth:
        if (neededOriginProperty) {
            return new AnimatedValue(style->border().right().width());
        }
        return AnimatedValue::createAnimatedValueFromLength(property);
    case CSSStyleValuePair::BorderBottomWidth:
        if (neededOriginProperty) {
            return new AnimatedValue(style->border().bottom().width());
        }
        return AnimatedValue::createAnimatedValueFromLength(property);
    case CSSStyleValuePair::BorderLeftWidth:
        if (neededOriginProperty) {
            return new AnimatedValue(style->border().left().width());
        }
        return AnimatedValue::createAnimatedValueFromLength(property);
    case CSSStyleValuePair::PaddingTop:
        if (neededOriginProperty) {
            return new AnimatedValue(style->padding().top());
        }
        return AnimatedValue::createAnimatedValueFromLength(property);
    case CSSStyleValuePair::PaddingRight:
        if (neededOriginProperty) {
            return new AnimatedValue(style->padding().right());
        }
        return AnimatedValue::createAnimatedValueFromLength(property);
    case CSSStyleValuePair::PaddingBottom:
        if (neededOriginProperty) {
            return new AnimatedValue(style->padding().bottom());
        }
        return AnimatedValue::createAnimatedValueFromLength(property);
    case CSSStyleValuePair::PaddingLeft:
        if (neededOriginProperty) {
            return new AnimatedValue(style->padding().left());
        }
        return AnimatedValue::createAnimatedValueFromLength(property);
    case CSSStyleValuePair::Height:
        if (neededOriginProperty) {
            return new AnimatedValue(style->height());
        }
        return AnimatedValue::createAnimatedValueFromLength(property);
    case CSSStyleValuePair::MaxHeight:
        if (neededOriginProperty) {
            return new AnimatedValue(style->maxHeight());
        }
        return AnimatedValue::createAnimatedValueFromLength(property);
    case CSSStyleValuePair::MinHeight:
        if (neededOriginProperty) {
            return new AnimatedValue(style->minHeight());
        }
        return AnimatedValue::createAnimatedValueFromLength(property);
    case CSSStyleValuePair::Left:
        if (neededOriginProperty) {
            return new AnimatedValue(style->left());
        }
        return AnimatedValue::createAnimatedValueFromLength(property);
    case CSSStyleValuePair::Right:
        if (neededOriginProperty) {
            return new AnimatedValue(style->right());
        }
        return AnimatedValue::createAnimatedValueFromLength(property);
    case CSSStyleValuePair::Top:
        if (neededOriginProperty) {
            return new AnimatedValue(style->top());
        }
        return AnimatedValue::createAnimatedValueFromLength(property);
    case CSSStyleValuePair::Bottom:
        if (neededOriginProperty) {
            return new AnimatedValue(style->bottom());
        }
        return AnimatedValue::createAnimatedValueFromLength(property);
    case CSSStyleValuePair::FontSize:
        if (neededOriginProperty) {
            return new AnimatedValue(style->fontSize());
        }
        return AnimatedValue::createAnimatedValueFromLength(property);
        break;
    case CSSStyleValuePair::BackgroundPositionX:
        if (neededOriginProperty) {
            return new AnimatedValue(style->backgroundPositionX(layer));
        }

        if (style->backgroundLayerSize() > 0) {
            return AnimatedValue::createAnimatedValueFromBackgroundPosition(
                property, layer);
        } else {
            // TODO: Consider how to handle in this case.
            STARFISH_UNIMPLEMENTED();
        }
        break;
    case CSSStyleValuePair::BackgroundPositionY:
        if (neededOriginProperty) {
            return new AnimatedValue(style->backgroundPositionY(layer));
        }

        if (style->backgroundLayerSize() > 0) {
            return AnimatedValue::createAnimatedValueFromBackgroundPosition(
                property, layer);
        } else {
            // TODO: Consider how to handle in this case.
            STARFISH_UNIMPLEMENTED();
        }
        break;
    case CSSStyleValuePair::BackgroundSize:
        if (style->backgroundLayerSize() > 0) {
            if (neededOriginProperty) {
                return new AnimatedValue(
                    style->backgroundSizeLengthValue(layer));
            }
            return AnimatedValue::createAnimatedValueFromBackgroundSize(
                property, layer);
        } else {
            // TODO: Consider how to handle in this case.
            STARFISH_UNIMPLEMENTED();
        }
        break;
    case CSSStyleValuePair::Opacity:
        if (neededOriginProperty) {
            return new AnimatedValue(style->opacity());
        }

        if (property.valueKind() == CSSStyleValuePair::ValueKind::Number) {
            return new AnimatedValue(property.numberValue());
        } else {
            STARFISH_UNIMPLEMENTED();
            return nullptr;
        }
        break;
    case CSSStyleValuePair::Transform:
        if (neededOriginProperty) {
            StyleTransformDataGroup* transform = style->transforms();
            if (transform) {
                return new AnimatedValue(transform);
            } else {
                return new AnimatedValue(new StyleTransformDataGroup());
            }
        }

        if (property.valueKind() ==
            CSSStyleValuePair::ValueKind::TransformFunctions) {
            auto transformValue = property.transformValue();
            ComputedStyle receiver(style);
            transformValue->toTransformDataGroup(element, &receiver);
            STARFISH_ASSERT(receiver.transforms() != nullptr);
            return new AnimatedValue(receiver.transforms());
        } else {
            return nullptr;
        }
        break;
    case CSSStyleValuePair::KeyKind::X:
        if (neededOriginProperty) {
            return new AnimatedValue(style->x());
        }
        return AnimatedValue::createAnimatedValueFromLength(property);
    case CSSStyleValuePair::KeyKind::Y:
        if (neededOriginProperty) {
            return new AnimatedValue(style->y());
        }
        return AnimatedValue::createAnimatedValueFromLength(property);
    case CSSStyleValuePair::KeyKind::CX:
        if (neededOriginProperty) {
            return new AnimatedValue(style->cx());
        }
        return AnimatedValue::createAnimatedValueFromLength(property);
    case CSSStyleValuePair::KeyKind::CY:
        if (neededOriginProperty) {
            return new AnimatedValue(style->cy());
        }
        return AnimatedValue::createAnimatedValueFromLength(property);
    case CSSStyleValuePair::KeyKind::RX:
        if (neededOriginProperty) {
            return new AnimatedValue(style->rx());
        }
        return AnimatedValue::createAnimatedValueFromLength(property);
    case CSSStyleValuePair::KeyKind::RY:
        if (neededOriginProperty) {
            return new AnimatedValue(style->ry());
        }
        return AnimatedValue::createAnimatedValueFromLength(property);
    default:
        break;
    }

    return Optional<AnimatedValue*>();
}

Optional<AnimatedValue*> AnimatedValue::createAnimatedValueFromColor(
    const CSSStyleValuePair& property)
{
    if (property.valueKind() == CSSStyleValuePair::ValueKind::ColorValueKind) {
        return new AnimatedValue(property.colorValue());
    } else if (property.valueKind() ==
               CSSStyleValuePair::ValueKind::NamedColorValueKind) {
        return new AnimatedValue(
            NamedColor::namedColorToColor(property.namedColorValue()));
    } else {
        // FIXME: Leave it as is to avoid regression. However, at some point it
        // will have to be replaced by an empty optional.

        // TODO: Consider how to handle in this case.
        STARFISH_UNIMPLEMENTED();
        return new AnimatedValue(Unit::Color(0, 0, 0, 0));
    }
}

Optional<AnimatedValue*> AnimatedValue::createAnimatedValueFromLength(
    const CSSStyleValuePair& property)
{
    if (property.valueKind() == CSSStyleValuePair::ValueKind::Length) {
        return new AnimatedValue(property.lengthValue());
    } else if (property.valueKind() ==
               CSSStyleValuePair::ValueKind::CalcValueKind) {
        return new AnimatedValue(property.lengthValue());
    } else if (property.valueKind() == CSSStyleValuePair::ValueKind::Auto) {
        return new AnimatedValue(Length());
    } else if (property.valueKind() ==
               CSSStyleValuePair::ValueKind::Percentage) {
        return new AnimatedValue(
            Length(Length::Percent, property.percentageValue()));
    } else {
        // FIXME: Leave it as is to avoid regression. However, at some point it
        // will have to be replaced by an empty optional.

        // TODO: Consider how to handle in this case.
        STARFISH_UNIMPLEMENTED();
        return new AnimatedValue(Length(Length::Fixed, 0));
    }
}

Optional<AnimatedValue*>
AnimatedValue::createAnimatedValueFromBackgroundPosition(
    const CSSStyleValuePair& property, size_t layer)
{
    Optional<Length> maybeLength;
    if (property.valueKind() == CSSStyleValuePair::ValueKind::ValueListKind) {
        ValueList* list = property.multiValue();
        if (list->size() > layer) {
            maybeLength =
                LengthUtil::backgroundPositionToLength((*list)[layer]);
        } else {
            maybeLength = LengthUtil::backgroundPositionToLength(
                (*list)[list->size() - 1]);
        }
    }

    if (maybeLength) {
        return new AnimatedValue(maybeLength.value());
    } else {
        STARFISH_UNIMPLEMENTED();
    }

    return Optional<AnimatedValue*>();
}

Optional<AnimatedValue*> AnimatedValue::createAnimatedValueFromBackgroundSize(
    const CSSStyleValuePair& property, size_t layer)
{
    Optional<LengthSize> maybeLengthSize;
    if (property.valueKind() == CSSStyleValuePair::ValueKind::ValueListKind) {
        ValueList* list = property.multiValue();
        if (list->size() > layer) {
            maybeLengthSize =
                LengthUtil::backgroundSizeToLengthSize((*list)[layer]);
        } else {
            maybeLengthSize = LengthUtil::backgroundSizeToLengthSize(
                (*list)[list->size() - 1]);
        }
    }

    if (maybeLengthSize) {
        return new AnimatedValue(maybeLengthSize.value());
    } else {
        STARFISH_UNIMPLEMENTED();
    }

    return Optional<AnimatedValue*>();
}

void AnimatedValue::changeToFixedIfNeeded(Length curFontSize,
                                          Length rootFontSize, Font* font,
                                          LayoutUnit viewportWidth,
                                          LayoutUnit viewportHeight,
                                          Optional<ComputedStyle*> cs)
{
    if (isLength()) {
        if (!getLength().isFixed()) {
            Length length = getLength();
            length.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                         viewportWidth, viewportHeight, cs);
            setLength(length);
        }
    }
}

} // namespace Starfish
