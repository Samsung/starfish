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

namespace Starfish {

static AnimatedValue* animatedColorValue(const CSSStyleValuePair& property)
{
    if (property.valueKind() == CSSStyleValuePair::ValueKind::ColorValueKind) {
        return new AnimatedValue(property.colorValue());
    } else if (property.valueKind() ==
               CSSStyleValuePair::ValueKind::NamedColorValueKind) {
        return new AnimatedValue(
            NamedColor::namedColorToColor(property.namedColorValue()));
    } else {
        // TODO: Consider how to handle in this case.
        STARFISH_UNIMPLEMENTED();
        return new AnimatedValue(Unit::Color(0, 0, 0, 0));
    }
}

static AnimatedValue* animatedLengthValue(const CSSStyleValuePair& property)
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
        // TODO: Consider how to handle in this case.
        STARFISH_UNIMPLEMENTED();
        return new AnimatedValue(Length(Length::Fixed, 0));
    }
}

static Length backgroundPositionToLength(const CSSStyleValuePair& property)
{
    Length value;
    if (property.valueKind() == CSSStyleValuePair::ValueKind::SideValueKind) {
        SideValue side = property.sideValue();
        if (side == SideValue::LeftSideValue) {
            value = Length(Length::Percent, 0.0f);
        } else if (side == SideValue::RightSideValue) {
            value = Length(Length::Percent, 1.0f);
        } else if (side == SideValue::TopSideValue) {
            value = Length(Length::Percent, 0.0f);
        } else if (side == SideValue::BottomSideValue) {
            value = Length(Length::Percent, 1.0f);
        } else if (side == SideValue::CenterSideValue) {
            value = Length(Length::Percent, 0.5f);
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
    } else if (property.valueKind() == CSSStyleValuePair::ValueKind::Length) {
        value = property.lengthValue();
    } else if (property.valueKind() ==
               CSSStyleValuePair::ValueKind::CalcValueKind) {
        value = property.lengthValue();
    } else if (property.valueKind() ==
               CSSStyleValuePair::ValueKind::Percentage) {
        value = Length(Length::Percent, property.percentageValue());
    } else {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    return value;
}

static AnimatedValue* backgroundPositionToAnimatedValue(
    ComputedStyle* style, Element* element, const CSSStyleValuePair& property,
    size_t layer)
{
    if (property.valueKind() == CSSStyleValuePair::ValueKind::ValueListKind) {
        ValueList* list = property.multiValue();
        if (list->size() > layer) {
            return new AnimatedValue(
                backgroundPositionToLength((*list)[layer]));
        } else {
            return new AnimatedValue(
                backgroundPositionToLength((*list)[list->size() - 1]));
        }
    } else {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    return nullptr;
}

static Optional<Length> convertValueToLength(const CSSStyleValuePair& property)
{
    CSSStyleValuePair::ValueKind kind = property.valueKind();
    CSSStyleValuePair::ValueData data = property.value();
    if (kind == CSSStyleValuePair::ValueKind::Auto) {
        return Length();
    } else if (kind == CSSStyleValuePair::ValueKind::Length) {
        return data.m_length.toLength();
    } else if (kind == CSSStyleValuePair::ValueKind::Percentage) {
        return Length(Length::Percent, data.m_floatValue);
    } else if (kind == CSSStyleValuePair::ValueKind::Number) {
        return Length(Length::Fixed, data.m_floatValue);
    } else if (kind == CSSStyleValuePair::ValueKind::CalcValueKind) {
        CalcValueType type = data.m_calc->calcValueType();
        if (type.isLength() || type.isPercentage() || type.isNumber()) {
            return Length(data.m_calc);
        } else {
            return Optional<Length>();
        }
    } else {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
    return Optional<Length>();
}

static LengthSize backgroundSizeToLengthSize(const CSSStyleValuePair& property)
{
    LengthSize result;
    if (property.valueKind() == CSSStyleValuePair::ValueListKind) {
        ValueList* list = property.multiValue();
        if (list->size() >= 1) {
            Optional<Length> width = convertValueToLength((*list)[0]);
            if (width.hasValue()) {
                result.m_width = width.getValue();
            }
        }
        if (list->size() >= 2) {
            Optional<Length> height = convertValueToLength((*list)[1]);
            if (height.hasValue()) {
                result.m_height = height.getValue();
            }
        }
    } else {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    return result;
}

static AnimatedValue* backgroundSizeToAnimatedValue(
    ComputedStyle* style, Element* element, const CSSStyleValuePair& property,
    size_t layer)
{
    if (property.valueKind() == CSSStyleValuePair::ValueKind::ValueListKind) {
        ValueList* list = property.multiValue();
        if (list->size() > layer) {
            return new AnimatedValue(
                backgroundSizeToLengthSize((*list)[layer]));
        } else {
            return new AnimatedValue(
                backgroundSizeToLengthSize((*list)[list->size() - 1]));
        }
    } else {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    return nullptr;
}

AnimatedValue* AnimatedValue::create(ComputedStyle* style, Element* element,
                                     const CSSStyleValuePair& property,
                                     const CSSStyleValuePair::KeyKind& keyKind,
                                     size_t layer, bool neededOriginProperty)
{
    STARFISH_ASSERT(style != nullptr);

    switch (keyKind) {
    case CSSStyleValuePair::Color:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->color());
        }
        return animatedColorValue(property);
    case CSSStyleValuePair::BackgroundColor:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->backgroundColor());
        }
        return animatedColorValue(property);
    case CSSStyleValuePair::BorderBottomColor:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->border().bottom().color());
        }
        return animatedColorValue(property);
    case CSSStyleValuePair::BorderLeftColor:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->border().left().color());
        }
        return animatedColorValue(property);
    case CSSStyleValuePair::BorderRightColor:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->border().right().color());
        }
        return animatedColorValue(property);
    case CSSStyleValuePair::BorderTopColor:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->border().top().color());
        }
        return animatedColorValue(property);
    case CSSStyleValuePair::CaretColor:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->caretColor());
        }
        return animatedColorValue(property);
    case CSSStyleValuePair::OutlineColor:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->outlineColor());
        }
        return animatedColorValue(property);
    case CSSStyleValuePair::TextDecorationColor:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->textDecorationColor());
        }
        return animatedColorValue(property);
    case CSSStyleValuePair::Width:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->width());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::MaxWidth:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->maxWidth());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::MinWidth:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->minWidth());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::MarginTop:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->margin().top());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::MarginRight:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->margin().right());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::MarginBottom:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->margin().bottom());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::MarginLeft:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->margin().left());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::BorderTopWidth:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->border().top().width());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::BorderRightWidth:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->border().right().width());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::BorderBottomWidth:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->border().bottom().width());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::BorderLeftWidth:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->border().left().width());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::PaddingTop:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->padding().top());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::PaddingRight:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->padding().right());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::PaddingBottom:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->padding().bottom());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::PaddingLeft:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->padding().left());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::Height:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->height());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::MaxHeight:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->maxHeight());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::MinHeight:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->minHeight());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::Left:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->left());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::Right:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->right());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::Top:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->top());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::Bottom:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->bottom());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::FontSize:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->fontSize());
        }
        return animatedLengthValue(property);
        break;
    case CSSStyleValuePair::BackgroundPositionX:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->backgroundPositionX(layer));
        }

        if (style->backgroundLayerSize() > 0) {
            return backgroundPositionToAnimatedValue(style, element, property,
                                                     layer);
        } else {
            // TODO: Consider how to handle in this case.
            STARFISH_UNIMPLEMENTED();
        }
        break;
    case CSSStyleValuePair::BackgroundPositionY:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->backgroundPositionY(layer));
        }

        if (style->backgroundLayerSize() > 0) {
            return backgroundPositionToAnimatedValue(style, element, property,
                                                     layer);
        } else {
            // TODO: Consider how to handle in this case.
            STARFISH_UNIMPLEMENTED();
        }
        break;
    case CSSStyleValuePair::BackgroundSize:
        if (style->backgroundLayerSize() > 0) {
            if (neededOriginProperty == true) {
                return new AnimatedValue(
                    style->backgroundSizeLengthValue(layer));
            }
            return backgroundSizeToAnimatedValue(style, element, property,
                                                 layer);
        } else {
            // TODO: Consider how to handle in this case.
            STARFISH_UNIMPLEMENTED();
        }
        break;
    case CSSStyleValuePair::Opacity:
        if (neededOriginProperty == true) {
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
        if (neededOriginProperty == true) {
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
    case CSSStyleValuePair::KeyKind::CX:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->cx());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::KeyKind::CY:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->cy());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::KeyKind::RX:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->rx());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::KeyKind::RY:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->ry());
        }
        return animatedLengthValue(property);
    default:
        break;
    }

    return nullptr;
}

} // namespace Starfish
