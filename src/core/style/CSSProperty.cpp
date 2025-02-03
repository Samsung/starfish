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

#include "StarfishConfig.h"
#include "core/style/Style.h"
#include "core/style/CSSProperty.h"

namespace Starfish {
bool CSSPropertyHelper::isAnimatableProperty(
    CSSStyleValuePair::KeyKind property)
{
    if (isAnimatableShorthandProperty(property)) {
        return true;
    } else if (isAnimatableLonghandProperty(property)) {
        return true;
    }
    return false;
}

bool CSSPropertyHelper::isAnimatableShorthandProperty(
    CSSStyleValuePair::KeyKind property)
{
    // Shorthand properties.
    switch (property) {
    case CSSStyleValuePair::All:
    case CSSStyleValuePair::Background:
    case CSSStyleValuePair::BackgroundPosition:
    case CSSStyleValuePair::Border:
    case CSSStyleValuePair::BorderColor:
    case CSSStyleValuePair::BorderWidth:
    case CSSStyleValuePair::BorderBottom:
    case CSSStyleValuePair::BorderLeft:
    case CSSStyleValuePair::BorderRight:
    case CSSStyleValuePair::BorderTop:
    case CSSStyleValuePair::Flex:
    case CSSStyleValuePair::Font:
    case CSSStyleValuePair::Margin:
    case CSSStyleValuePair::Outline:
    case CSSStyleValuePair::Padding:
    case CSSStyleValuePair::TextDecoration:
        return true;
    default:
        break;
    }
    return false;
}

bool CSSPropertyHelper::isAnimatableLonghandProperty(
    CSSStyleValuePair::KeyKind property)
{
    // Longhand properties.
    switch (property) {
    case CSSStyleValuePair::Bottom:
    case CSSStyleValuePair::BackgroundColor:
    case CSSStyleValuePair::BackgroundPositionX:
    case CSSStyleValuePair::BackgroundPositionY:
    case CSSStyleValuePair::BackgroundSize:
    case CSSStyleValuePair::BorderBottomColor:
    case CSSStyleValuePair::BorderBottomLeftRadius:
    case CSSStyleValuePair::BorderBottomRightRadius:
    case CSSStyleValuePair::BorderBottomWidth:
    case CSSStyleValuePair::BorderLeftColor:
    case CSSStyleValuePair::BorderLeftWidth:
    case CSSStyleValuePair::BorderRightColor:
    case CSSStyleValuePair::BorderRightWidth:
    case CSSStyleValuePair::BorderTopColor:
    case CSSStyleValuePair::BorderTopLeftRadius:
    case CSSStyleValuePair::BorderTopRightRadius:
    case CSSStyleValuePair::BorderTopWidth:
    case CSSStyleValuePair::BoxShadow:
    case CSSStyleValuePair::CaretColor:
    case CSSStyleValuePair::Clip:
    case CSSStyleValuePair::Color:
    case CSSStyleValuePair::CX:
    case CSSStyleValuePair::CY:
    case CSSStyleValuePair::D:
    case CSSStyleValuePair::Fill:
    case CSSStyleValuePair::FillOpacity:
    case CSSStyleValuePair::FlexBasis:
    case CSSStyleValuePair::FlexGrow:
    case CSSStyleValuePair::FlexShrink:
    case CSSStyleValuePair::FontSize:
    case CSSStyleValuePair::FontWeight:
    case CSSStyleValuePair::Height:
    case CSSStyleValuePair::Left:
    case CSSStyleValuePair::LetterSpacing:
    case CSSStyleValuePair::LineHeight:
    case CSSStyleValuePair::MarginBottom:
    case CSSStyleValuePair::MarginLeft:
    case CSSStyleValuePair::MarginRight:
    case CSSStyleValuePair::MarginTop:
    case CSSStyleValuePair::MaxHeight:
    case CSSStyleValuePair::MaxWidth:
    case CSSStyleValuePair::MinHeight:
    case CSSStyleValuePair::MinWidth:
    case CSSStyleValuePair::ObjectPosition:
    case CSSStyleValuePair::Opacity:
    case CSSStyleValuePair::Order:
    case CSSStyleValuePair::OutlineColor:
    case CSSStyleValuePair::OutlineOffset:
    case CSSStyleValuePair::OutlineWidth:
    case CSSStyleValuePair::PaddingBottom:
    case CSSStyleValuePair::PaddingLeft:
    case CSSStyleValuePair::PaddingRight:
    case CSSStyleValuePair::PaddingTop:
    case CSSStyleValuePair::R:
    case CSSStyleValuePair::Right:
    case CSSStyleValuePair::RX:
    case CSSStyleValuePair::RY:
    case CSSStyleValuePair::Stroke:
    case CSSStyleValuePair::StrokeWidth:
    case CSSStyleValuePair::TextDecorationColor:
    case CSSStyleValuePair::TextIndent:
    case CSSStyleValuePair::TextShadow:
    case CSSStyleValuePair::Top:
    case CSSStyleValuePair::Transform:
    case CSSStyleValuePair::TransformOrigin:
    case CSSStyleValuePair::VerticalAlign:
    case CSSStyleValuePair::Visibility:
    case CSSStyleValuePair::Width:
    case CSSStyleValuePair::WordSpacing:
    case CSSStyleValuePair::X:
    case CSSStyleValuePair::Y:
    case CSSStyleValuePair::ZIndex:
        return true;
    default:
        break;
    }
    return false;
}

std::pair<std::vector<CSSStyleValuePair::KeyKind>,
          std::vector<CSSStyleValuePair::KeyKind>>
CSSPropertyHelper::decomposeIntoConstituentAnimatableProperties(
    CSSStyleValuePair::KeyKind property)
{
    // Note:
    // Each case(shorthand property) is listed from the current
    // animation(transition) implementation.
    // The decomposition is also based on the current implementation too.
    // Therefore, if you add a new animation for the shorthand property, this
    // method must be updated as well.

    std::vector<CSSStyleValuePair::KeyKind> shorthands;
    std::vector<CSSStyleValuePair::KeyKind> longhands;

    switch (property) {
    case CSSStyleValuePair::KeyKind::Background:
        shorthands.push_back(CSSStyleValuePair::KeyKind::BackgroundPosition);
        longhands.push_back(CSSStyleValuePair::KeyKind::BackgroundColor);
        longhands.push_back(CSSStyleValuePair::KeyKind::BackgroundSize);
        break;
    case CSSStyleValuePair::KeyKind::BackgroundPosition:
        longhands.push_back(CSSStyleValuePair::KeyKind::BackgroundPositionX);
        longhands.push_back(CSSStyleValuePair::KeyKind::BackgroundPositionY);
        break;
    case CSSStyleValuePair::KeyKind::Border:
        shorthands.push_back(CSSStyleValuePair::KeyKind::BorderColor);
        shorthands.push_back(CSSStyleValuePair::KeyKind::BorderWidth);
        break;
    case CSSStyleValuePair::KeyKind::BorderColor:
        longhands.push_back(CSSStyleValuePair::KeyKind::BorderLeftColor);
        longhands.push_back(CSSStyleValuePair::KeyKind::BorderTopColor);
        longhands.push_back(CSSStyleValuePair::KeyKind::BorderRightColor);
        longhands.push_back(CSSStyleValuePair::KeyKind::BorderBottomColor);
        break;
    case CSSStyleValuePair::KeyKind::BorderWidth:
        longhands.push_back(CSSStyleValuePair::KeyKind::BorderLeftWidth);
        longhands.push_back(CSSStyleValuePair::KeyKind::BorderTopWidth);
        longhands.push_back(CSSStyleValuePair::KeyKind::BorderRightWidth);
        longhands.push_back(CSSStyleValuePair::KeyKind::BorderBottomWidth);
        break;
    case CSSStyleValuePair::KeyKind::BorderBottom:
        longhands.push_back(CSSStyleValuePair::KeyKind::BorderBottomColor);
        longhands.push_back(CSSStyleValuePair::KeyKind::BorderBottomWidth);
        break;
    case CSSStyleValuePair::KeyKind::BorderLeft:
        longhands.push_back(CSSStyleValuePair::KeyKind::BorderLeftColor);
        longhands.push_back(CSSStyleValuePair::KeyKind::BorderLeftWidth);
        break;
    case CSSStyleValuePair::KeyKind::BorderRight:
        longhands.push_back(CSSStyleValuePair::KeyKind::BorderRightColor);
        longhands.push_back(CSSStyleValuePair::KeyKind::BorderRightWidth);
        break;
    case CSSStyleValuePair::KeyKind::BorderTop:
        longhands.push_back(CSSStyleValuePair::KeyKind::BorderTopColor);
        longhands.push_back(CSSStyleValuePair::KeyKind::BorderTopWidth);
        break;
    case CSSStyleValuePair::KeyKind::Font:
        longhands.push_back(CSSStyleValuePair::KeyKind::FontSize);
        break;
    case CSSStyleValuePair::KeyKind::Margin:
        longhands.push_back(CSSStyleValuePair::KeyKind::MarginLeft);
        longhands.push_back(CSSStyleValuePair::KeyKind::MarginTop);
        longhands.push_back(CSSStyleValuePair::KeyKind::MarginRight);
        longhands.push_back(CSSStyleValuePair::KeyKind::MarginBottom);
        break;
    case CSSStyleValuePair::KeyKind::Outline:
        longhands.push_back(CSSStyleValuePair::KeyKind::OutlineColor);
        break;
    case CSSStyleValuePair::KeyKind::Padding:
        longhands.push_back(CSSStyleValuePair::KeyKind::PaddingLeft);
        longhands.push_back(CSSStyleValuePair::KeyKind::PaddingTop);
        longhands.push_back(CSSStyleValuePair::KeyKind::PaddingRight);
        longhands.push_back(CSSStyleValuePair::KeyKind::PaddingBottom);
        break;
    case CSSStyleValuePair::KeyKind::TextDecoration:
        longhands.push_back(CSSStyleValuePair::KeyKind::TextDecorationColor);
        break;
    default:
        STARFISH_UNIMPLEMENTED();
        break;
    }

    return { std::move(shorthands), std::move(longhands) };
}

static bool isAnimatableBackgroundProperty(CSSStyleValuePair::KeyKind property)
{
    switch (property) {
    // Shorthand
    case CSSStyleValuePair::Background:
    case CSSStyleValuePair::BackgroundPosition:
    case CSSStyleValuePair::BackgroundColor:
    case CSSStyleValuePair::BackgroundPositionX:
    case CSSStyleValuePair::BackgroundPositionY:
    case CSSStyleValuePair::BackgroundSize:
        return true;
    default:
        break;
    }
    return false;
}

String* CSSPropertyHelper::toGCString(CSSStyleValuePair::KeyKind property)
{
    const char* str = toString(property);
    STARFISH_ASSERT(str != nullptr);
    return String::createASCIIString(str, strlen(str));
}

const char* CSSPropertyHelper::toString(CSSStyleValuePair::KeyKind property)
{
    switch (property) {
#define ADD_CSS_NAME(ENUM, CAMEL, CSSNAME) \
    case CSSStyleValuePair::ENUM:          \
        return CSSNAME;
        FOR_EACH_STYLE_ATTRIBUTE_TOTAL(ADD_CSS_NAME)
#undef ADD_CSS_NAME
    // Non CSS property
    case CSSStyleValuePair::CustomProperty:
    case CSSStyleValuePair::KeyKindSize:
    case CSSStyleValuePair::Unknown:
        break;
    }
    return "";
}

String* CSSPropertyHelper::toCamelCaseGCString(
    CSSStyleValuePair::KeyKind property)
{
    const char* str = toCamelCaseString(property);
    STARFISH_ASSERT(str != nullptr);
    return String::createASCIIString(str, strlen(str));
}

const char* CSSPropertyHelper::toCamelCaseString(
    CSSStyleValuePair::KeyKind property)
{
    switch (property) {
#define ADD_CSS_CAMEL_NAME(ENUM, CAMEL, CSSNAME) \
    case CSSStyleValuePair::ENUM:                \
        return #CAMEL;
        FOR_EACH_STYLE_ATTRIBUTE_TOTAL(ADD_CSS_CAMEL_NAME)
#undef ADD_CSS_CAMEL_NAME
    // Non CSS property
    case CSSStyleValuePair::CustomProperty:
    case CSSStyleValuePair::KeyKindSize:
    case CSSStyleValuePair::Unknown:
        break;
    }
    return "";
}
} // namespace Starfish
