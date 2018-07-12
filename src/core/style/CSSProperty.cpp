/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include "core/style/Style.h"
#include "core/style/CSSProperty.h"

namespace StarFish {
bool CSSPropertyHelper::isAnimatable(CSSStyleValuePair::KeyKind property)
{
    switch (property) {
    // Shorthand
    case CSSStyleValuePair::All:
    case CSSStyleValuePair::Background:
    case CSSStyleValuePair::BackgroundPosition:
    case CSSStyleValuePair::BorderBottom:
    case CSSStyleValuePair::BorderColor:
    case CSSStyleValuePair::BorderLeft:
    case CSSStyleValuePair::BorderRight:
    case CSSStyleValuePair::BorderTop:
    case CSSStyleValuePair::Bottom:
    case CSSStyleValuePair::Flex:
    case CSSStyleValuePair::Font:
    case CSSStyleValuePair::Margin:
    case CSSStyleValuePair::Outline:
    case CSSStyleValuePair::Padding:
    case CSSStyleValuePair::TextDecoration:
    // Normal
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

String* CSSPropertyHelper::toGCString(CSSStyleValuePair::KeyKind property)
{
    return String::createASCIIString(toString(property));
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
    case CSSStyleValuePair::VarValue:
        break;
    }
    return "";
}

String* CSSPropertyHelper::toCamelCaseGCString(
    CSSStyleValuePair::KeyKind property)
{
    return String::createASCIIString(toCamelCaseString(property));
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
    case CSSStyleValuePair::VarValue:
        break;
    }
    return "";
}
}
