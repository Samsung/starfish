/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 * Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
 *               1999 Waldo Bastian (bastian@kde.org)
 *               2001 Andreas Schlapbach (schlpbch@iam.unibe.ch)
 *               2001-2003 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2002, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights
 * reserved.
 * Copyright (C) 2008 David Smith (catfish.man@gmail.com)
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "StarFishConfig.h"

#include "core/style/Style.h"

#include "StarFish.h"
#include "core/animation/Animation.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/dom/HTMLDocument.h"
#include "core/dom/HTMLElement.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/HTMLIFrameElement.h"
#include "core/dom/HTMLLinkElement.h"
#include "core/dom/HTMLStyleElement.h"
#include "core/dom/HTMLInputElement.h"
#include "core/dom/HTMLOptionElement.h"
#include "core/dom/Text.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameTreeBuilder.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "core/style/AncestorSelectorFilter.h"
#include "core/style/CalcData.h"
#include "core/style/ComputedStyle.h"
#include "core/style/CSSCounterFunction.h"
#include "core/style/CSSGradientValue.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/style/CSSStyleSheet.h"
#include "core/style/FontFaceSrcData.h"
#include "core/style/FlexBasisData.h"
#include "core/style/MediaQueryEvaluator.h"
#include "core/style/MediaQueryResult.h"
#include "core/style/MediaValues.h"
#include "core/style/NamedColors.h"
#include "core/style/Style.h"
#include "core/style/StyleRule.h"
#include "platform/window/PlatformWindow.h"
#include "core/style/ShadowData.h"
#ifdef STARFISH_ENABLE_CSS_VARIABLE
#include "core/style/CSSVariableSyntaxTreeBuilder.h"
#endif

namespace StarFish {

#define TOKEN_IS_STRING(str) \
    ((memcmp(token, str, strlen(str))) == 0 && strlen(str) == strlen(token))

#define VALUE_IS_STRING(str) \
    ((memcmp(value, str, strlen(str))) == 0 && strlen(str) == strlen(value))

#define VALUE_IS_INHERIT() VALUE_IS_STRING("inherit")

#define VALUE_IS_INITIAL() VALUE_IS_STRING("initial")

#define VALUE_IS_UNSET() VALUE_IS_STRING("unset")

#define VALUE_IS_NONE() VALUE_IS_STRING("none")

#define VALUE_IS_AUTO() VALUE_IS_STRING("auto")

#define STRING_VALUE_IS_STRING(str) (value.equals(str))

#define STRING_VALUE_IS_INHERIT() STRING_VALUE_IS_STRING("inherit")

#define STRING_VALUE_IS_INITIAL() STRING_VALUE_IS_STRING("initial")

#define STRING_VALUE_IS_UNSET() STRING_VALUE_IS_STRING("unset")

#define STRING_VALUE_IS_NONE() STRING_VALUE_IS_STRING("none")

#define STRING_VALUE_IS_AUTO() STRING_VALUE_IS_STRING("auto")

static const float fontSizeFactors[8] = { 0.60f, 0.75f, 0.89f, 1.0f,
                                          1.2f,  1.5f,  2.0f,  3.0f };

static FontWeightValue lighterWeight(FontWeightValue weight)
{
    switch (weight) {
    case FontWeightValue::OneHundredFontWeightValue:
    case FontWeightValue::TwoHundredsFontWeightValue:
    case FontWeightValue::ThreeHundredsFontWeightValue:
    case FontWeightValue::NormalFontWeightValue:
    case FontWeightValue::FiveHundredsFontWeightValue:
        return FontWeightValue::OneHundredFontWeightValue;
    case FontWeightValue::SixHundredsFontWeightValue:
    case FontWeightValue::BoldFontWeightValue:
        return FontWeightValue::NormalFontWeightValue; // 400
    case FontWeightValue::EightHundredsFontWeightValue:
    case FontWeightValue::NineHundredsFontWeightValue:
        return FontWeightValue::BoldFontWeightValue; // 700
    default:
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

static FontWeightValue bolderWeight(FontWeightValue weight)
{
    switch (weight) {
    case FontWeightValue::OneHundredFontWeightValue:
    case FontWeightValue::TwoHundredsFontWeightValue:
    case FontWeightValue::ThreeHundredsFontWeightValue:
        return FontWeightValue::NormalFontWeightValue; // 400
    case FontWeightValue::NormalFontWeightValue:
    case FontWeightValue::FiveHundredsFontWeightValue:
        return FontWeightValue::BoldFontWeightValue; // 700
    case FontWeightValue::SixHundredsFontWeightValue:
    case FontWeightValue::BoldFontWeightValue:
    case FontWeightValue::EightHundredsFontWeightValue:
    case FontWeightValue::NineHundredsFontWeightValue:
        return FontWeightValue::NineHundredsFontWeightValue; // 900
    default:
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

static const int strictFontSizeTable[8][8] = {
    { 9, 9, 9, 9, 11, 14, 18, 27 },    { 9, 9, 9, 10, 12, 15, 20, 30 },
    { 9, 9, 10, 11, 13, 17, 22, 33 },  { 9, 9, 10, 12, 14, 18, 24, 36 },
    { 9, 10, 12, 13, 16, 20, 26, 39 }, // fixed font default (13)
    { 9, 10, 12, 14, 17, 21, 28, 42 }, { 9, 10, 13, 15, 18, 23, 30, 45 },
    { 9, 10, 13, 16, 18, 24, 32, 48 } // proportional font default (16)
};

static const int fontSizeTableMax = 16;
static const int fontSizeTableMin = 9;

static Length parseAbsoluteFontSize(int col, float mediumSize)
{
    int row = 0;
    if (mediumSize >= fontSizeTableMin && mediumSize <= fontSizeTableMax) {
        row = mediumSize - fontSizeTableMin;
    }

    return Length(Length::Fixed, strictFontSizeTable[row][col]);
}

static Nullable<Length> convertValueToLength(CSSStyleValuePair::ValueKind kind,
                                             CSSStyleValuePair::ValueData data)
{
    if (kind == CSSStyleValuePair::ValueKind::Auto) {
        return Length();
    } else if (kind == CSSStyleValuePair::ValueKind::Length) {
        return data.m_length.toLength();
    } else if (kind == CSSStyleValuePair::ValueKind::Percentage) {
        return Length(Length::Percent, data.m_floatValue);
    } else if (kind == CSSStyleValuePair::ValueKind::Number) {
        return Length(Length::Fixed, data.m_floatValue);
    } else if (kind == CSSStyleValuePair::ValueKind::CalcValueKind) {
        CalcValueType type = data.m_calc->type();
        if (type.isLength() || type.isPercentage()) {
            return Length(data.m_calc);
        } else {
            return Nullable<Length>();
        }
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

String* transitionPropertyValueToString(TransitionPropertyValue val)
{
    switch (val) {
    case TransitionPropertyAllValue:
        return String::createASCIIString("all");
    case TransitionPropertyBackgroundColorValue:
        return String::createASCIIString("background-color");
    case TransitionPropertyBackgroundPositionValue:
        return String::createASCIIString("background-position");
    case TransitionPropertyBorderBottomColorValue:
        return String::createASCIIString("border-bottom-color");
    case TransitionPropertyBorderBottomWidthValue:
        return String::createASCIIString("border-bottom-width");
    case TransitionPropertyBorderLeftColorValue:
        return String::createASCIIString("border-left-color");
    case TransitionPropertyBorderLeftWidthValue:
        return String::createASCIIString("border-left-width");
    case TransitionPropertyBorderRightColorValue:
        return String::createASCIIString("border-right-color");
    case TransitionPropertyBorderRightWidthValue:
        return String::createASCIIString("border-right-width");
    case TransitionPropertyBorderSpacingValue:
        return String::createASCIIString("border-spacing");
    case TransitionPropertyBorderTopColorValue:
        return String::createASCIIString("border-top-color");
    case TransitionPropertyBorderTopWidthValue:
        return String::createASCIIString("border-top-width");
    case TransitionPropertyBottomValue:
        return String::createASCIIString("bottom");
    case TransitionPropertyClipValue:
        return String::createASCIIString("clip");
    case TransitionPropertyColorValue:
        return String::createASCIIString("color");
    case TransitionPropertyFontSizeValue:
        return String::createASCIIString("font-size");
    case TransitionPropertyFontWeightValue:
        return String::createASCIIString("font-weight");
    case TransitionPropertyHeightValue:
        return String::createASCIIString("height");
    case TransitionPropertyLeftValue:
        return String::createASCIIString("left");
    case TransitionPropertyLetterSpacingValue:
        return String::createASCIIString("letter-spacing");
    case TransitionPropertyLineHeightValue:
        return String::createASCIIString("line-height");
    case TransitionPropertyMarginBottomValue:
        return String::createASCIIString("margin-bottom");
    case TransitionPropertyMarginLeftValue:
        return String::createASCIIString("margin-left");
    case TransitionPropertyMarginRightValue:
        return String::createASCIIString("margin-right");
    case TransitionPropertyMarginTopValue:
        return String::createASCIIString("margin-top");
    case TransitionPropertyMaxHeightValue:
        return String::createASCIIString("max-height");
    case TransitionPropertyMaxWidthValue:
        return String::createASCIIString("max-width");
    case TransitionPropertyMinHeightValue:
        return String::createASCIIString("min-height");
    case TransitionPropertyMinWidthValue:
        return String::createASCIIString("min-width");
    case TransitionPropertyOpacityValue:
        return String::createASCIIString("opacity");
    case TransitionPropertyOutlineColorValue:
        return String::createASCIIString("outline-color");
    case TransitionPropertyOutlineWidthValue:
        return String::createASCIIString("outline-width");
    case TransitionPropertyPaddingBottomValue:
        return String::createASCIIString("padding-bottom");
    case TransitionPropertyPaddingLeftValue:
        return String::createASCIIString("padding-left");
    case TransitionPropertyPaddingRightValue:
        return String::createASCIIString("padding-right");
    case TransitionPropertyPaddingTopValue:
        return String::createASCIIString("padding-top");
    case TransitionPropertyRightValue:
        return String::createASCIIString("right");
    case TransitionPropertyTextIndentValue:
        return String::createASCIIString("text-indent");
    case TransitionPropertyTextShadowValue:
        return String::createASCIIString("text-shadow");
    case TransitionPropertyTransformValue:
        return String::createASCIIString("transform");
    case TransitionPropertyTransformOriginValue:
        return String::createASCIIString("transform-origin");
    case TransitionPropertyTopValue:
        return String::createASCIIString("top");
    case TransitionPropertyVerticalAlignValue:
        return String::createASCIIString("vertical-align");
    case TransitionPropertyVisibilityValue:
        return String::createASCIIString("visibility");
    case TransitionPropertyWidthValue:
        return String::createASCIIString("width");
    case TransitionPropertyWordSpacingValue:
        return String::createASCIIString("word-spacing");
    case TransitionPropertyZIndexValue:
        return String::createASCIIString("z-index");
    default:
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

static void setComputedStyleBackgroundPositionX(ComputedStyle* style,
                                                const CSSStyleValuePair& value,
                                                unsigned int layer = 0)
{
    if (value.valueKind() == CSSStyleValuePair::ValueKind::Initial ||
        value.valueKind() == CSSStyleValuePair::ValueKind::Unset) {
        style->setBackgroundPositionX(Length(Length::Percent, 0.0f), layer);
    } else if (value.valueKind() ==
               CSSStyleValuePair::ValueKind::SideValueKind) {
        SideValue side = value.sideValue();
        if (side == SideValue::LeftSideValue) {
            style->setBackgroundPositionX(Length(Length::Percent, 0.0f), layer);
        } else if (side == SideValue::RightSideValue) {
            style->setBackgroundPositionX(Length(Length::Percent, 1.0f), layer);
        } else if (side == SideValue::CenterSideValue) {
            style->setBackgroundPositionX(Length(Length::Percent, 0.5f), layer);
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    } else if (value.valueKind() ==
               CSSStyleValuePair::ValueKind::ValueListKind) {
        ValueList* list = value.multiValue();
        for (unsigned int i = 0; i < list->size(); i++) {
            setComputedStyleBackgroundPositionX(style, (*list)[i], i);
        }
    } else {
        Nullable<Length> len =
            convertValueToLength(value.valueKind(), value.value());
        if (len.hasValue()) {
            style->setBackgroundPositionX(len.getValue(), layer);
        } else {
            style->setBackgroundPositionX(Length(Length::Percent, 0.0f), layer);
        }
    }
}

static void setComputedStyleBackgroundPositionY(ComputedStyle* style,
                                                const CSSStyleValuePair& value,
                                                unsigned int layer = 0)
{
    if (value.valueKind() == CSSStyleValuePair::ValueKind::Initial ||
        value.valueKind() == CSSStyleValuePair::ValueKind::Unset) {
        style->setBackgroundPositionY(Length(Length::Percent, 0.0f), layer);
    } else if (value.valueKind() ==
               CSSStyleValuePair::ValueKind::SideValueKind) {
        if (value.sideValue() == SideValue::TopSideValue) {
            style->setBackgroundPositionY(Length(Length::Percent, 0.0f), layer);
        } else if (value.sideValue() == SideValue::BottomSideValue) {
            style->setBackgroundPositionY(Length(Length::Percent, 1.0f), layer);
        } else if (value.sideValue() == SideValue::CenterSideValue) {
            style->setBackgroundPositionY(Length(Length::Percent, 0.5f), layer);
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    } else if (value.valueKind() ==
               CSSStyleValuePair::ValueKind::ValueListKind) {
        ValueList* list = value.multiValue();
        for (unsigned int i = 0; i < list->size(); i++) {
            setComputedStyleBackgroundPositionY(style, (*list)[i], i);
        }
    } else {
        Nullable<Length> len =
            convertValueToLength(value.valueKind(), value.value());
        if (len.hasValue()) {
            style->setBackgroundPositionY(len.getValue(), layer);
        } else {
            style->setBackgroundPositionY(Length(Length::Percent, 0.0f), layer);
        }
    }
}

String* CSSTransformFunctions::toString()
{
    StringBuilder builder;
    for (unsigned i = 0; i < size(); i++) {
        CSSTransformFunction item = (*this)[i];
        builder.appendString(item.functionName());
        builder.appendString("(");
        ValueList* values = item.values();
        for (unsigned int j = 0; j < values->size(); j++) {
            const CSSStyleValuePair& subitem = (*values)[j];
            String* newstr = subitem.toString();
            builder.appendString(newstr);
            if (j != values->size() - 1) {
                builder.appendString(", ");
            } else {
                builder.appendString(")");
            }
        }
    }
    return builder.finalize();
}

String* CSSStyleValuePair::keyName() const
{
    switch (keyKind()) {
#define ADD_CASE_FOR_KEYNAME(Name, name, cssname) \
    case Name:                                    \
        return String::createASCIIString(cssname);

        FOR_EACH_STYLE_ATTRIBUTE(ADD_CASE_FOR_KEYNAME);
#undef ADD_CASE_FOR_KEYNAME
    default:
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

static bool isValidVariables(const CSSTokenVector& tokens)
{
#ifdef STARFISH_ENABLE_CSS_VARIABLE
    for (size_t k = 0; k < tokens.size(); k++) {
        CSSVariableSyntaxTreeBuilder variablesSyntaxBuilder;
        CSSTokenValue token(tokens[k]);
        variablesSyntaxBuilder.build(token);

        if (!variablesSyntaxBuilder.isValid())
            return false;
    }

    return true;
#else
    return false;
#endif
}

bool CSSStyleValuePair::updateVarValue(const char* str,
                                       const CSSTokenVector& tokens)
{
#ifdef STARFISH_ENABLE_CSS_VARIABLE
    if (isValidVariables(tokens)) {
        m_keyKind = CSSStyleValuePair::KeyKind::VarValue;
        setVarFunctionValue(String::createASCIIString(str));
        return true;
    }
#endif
    return false;
}

bool CSSStyleValuePair::updateValueCommon(const CSSTokenVector& tokens)
{
    // NOTE: set common value (e.g. initial, inherit, "")
    if (tokens.size() != 1) {
        return false;
    }
    const char* value = tokens[0].data();
    if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_UNSET()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Unset;
    } else {
        return false;
    }
    return true;
}

String* CSSStyleValuePair::urlValue(ResourceURL* urlOfStyleSheet) const
{
    STARFISH_ASSERT(m_valueKind == UrlValueKind);
    return ResourceURL::mergeDocumentURIWithURIString(
        urlOfStyleSheet->baseURI(), m_value.m_stringValue);
}

static String* BorderString(String* width, bool isWidthCombined, String* style,
                            bool isStyleCombined, String* color,
                            bool isColorCombined)
{
    String* space = String::spaceString;
    StringBuilder builder;

    if (width->equals(style) && style->equals(color)) {
        STARFISH_ASSERT(width->equals(String::emptyString) ||
                        width->equals(String::initialString) ||
                        width->equals(String::inheritString));
        return width;
    }

    if (!width->equals(String::emptyString) &&
        !width->equals(String::initialString) && !isWidthCombined) {
        builder.appendString(width);
    }

    if (!style->equals(String::emptyString) &&
        !style->equals(String::initialString) && !isStyleCombined) {
        if (builder.contentLength() > 0) {
            builder.appendString(space);
        }
        builder.appendString(style);
    }

    if (!color->equals(String::emptyString) &&
        !color->equals(String::initialString) && !isColorCombined) {
        if (builder.contentLength() > 0) {
            builder.appendString(space);
        }
        builder.appendString(color);
    }

    return builder.finalize();
}

// Parse backgroundRepeat string in case it has single token
static bool parseBackgroundRepeatShorhand(const CSSTokenValue& tok,
                                          CSSStyleValuePair* retx,
                                          CSSStyleValuePair* rety)
{
    if (retx->updateValueUnitBackgroundRepeat(tok)) {
        *rety = *retx;
    } else if (tok.equals("repeat-x")) {
        *retx = CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::BackgroundRepeatValueKind,
            RepeatRepeatValue);
        *rety = CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::BackgroundRepeatValueKind,
            NoRepeatRepeatValue);
    } else if (tok.equals("repeat-y")) {
        *retx = CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::BackgroundRepeatValueKind,
            NoRepeatRepeatValue);
        *rety = CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::BackgroundRepeatValueKind,
            RepeatRepeatValue);
    } else {
        return false;
    }
    return true;
}

static bool parseBackgroundRepeatShorhand(const CSSTokenVector& tokens,
                                          CSSStyleValuePair* retx,
                                          CSSStyleValuePair* rety,
                                          bool allowComma = true)
{
    // <repeat-style> = repeat-x | repeat-y | [repeat | no-repeat]{1,2}
    // <repeat-style> [, <repeat-style>]*
    size_t len = 0;
    retx->setValueList(new ValueList(ValueList::Separator::CommaSeparator));
    rety->setValueList(new ValueList(ValueList::Separator::CommaSeparator));
    for (unsigned int i = 0; i < tokens.size(); i++) {
        const CSSTokenValue& value = tokens[i];
        if (value.equals(",")) {
            if (!allowComma || i == 0 || i == tokens.size() - 1) {
                return false;
            }
        } else if (i == tokens.size() - 1) {
            len++;
            i++;
        } else {
            len++;
            continue;
        }
        if (len == 1) {
            CSSStyleValuePair x, y;
            if (parseBackgroundRepeatShorhand(tokens[i - 1], &x, &y)) {
                retx->multiValue()->push_back(x);
                rety->multiValue()->push_back(y);
            } else {
                return false;
            }
        } else if (len == 2) {
            CSSStyleValuePair x, y;
            if (x.updateValueUnitBackgroundRepeat(tokens[i - 2]) &&
                y.updateValueUnitBackgroundRepeat(tokens[i - 1])) {
                retx->multiValue()->push_back(x);
                rety->multiValue()->push_back(y);
            } else {
                return false;
            }
        } else {
            return false;
        }
        len = 0;
    }
    return true;
}

static bool parseBackgroundPositionShorhand(const CSSTokenVector& tokens,
                                            CSSStyleValuePair* retx,
                                            CSSStyleValuePair* rety,
                                            bool allowComma = true)
{
    // [ [ <percentage> | <length> | left | center | right ] [ <percentage> |
    // <length> | top | center | bottom ]? ] | [ [ left | center | right ] || [
    // top | center | bottom ] ] | inherit
    size_t len = 0;
    retx->setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
    rety->setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
    retx->setValueList(new ValueList(ValueList::Separator::SpaceSeparator));
    rety->setValueList(new ValueList(ValueList::Separator::SpaceSeparator));

    for (unsigned int i = 0; i < tokens.size(); i++) {
        const CSSTokenValue& value = tokens[i];
        if (value.equals(",")) {
            if (!allowComma || i == 0 || i == tokens.size() - 1) {
                return false;
            }
        } else if (i == tokens.size() - 1) {
            len++;
            i++;
        } else {
            len++;
            continue;
        }

        CSSStyleValuePair ret;
        CSSStyleValuePair x, y;
        if (len == 1) {
            const CSSTokenValue& tok = tokens[i - 1];
            if (x.updateValueUnitBackgroundPositionX(tok)) {
                y = CSSStyleValuePair(
                    CSSStyleValuePair::ValueKind::SideValueKind,
                    SideValue::CenterSideValue);
            } else if (y.updateValueUnitBackgroundPositionY(tok)) {
                x = CSSStyleValuePair(
                    CSSStyleValuePair::ValueKind::SideValueKind,
                    SideValue::CenterSideValue);
            } else {
                return false;
            }
        } else if (len == 2) {
            // !!! NOTICE !!!
            // background-position: right bottom <- (O) [enum-x enum-y]
            // background-position: bottom right <- (O) [enum-y enum-x] !!!
            // background-position: 20px 20px    <- (O) [length length]
            // background-position: right 20px   <- (O) [enum-x length]
            // background-position: 20px bottom  <- (O) [length enum-y]
            // background-position: bottom 20px  <- (X) [enum-y length] !!!
            // background-position: 20px right   <- (X) [length enum-x] !!!
            bool checker = true;
            const CSSTokenValue& tok1 = tokens[i - 2];
            const CSSTokenValue& tok2 = tokens[i - 1];
            CSSStyleValuePair::ValueKind sideKind =
                CSSStyleValuePair::ValueKind::SideValueKind;
            checker &= x.updateValueUnitBackgroundPositionX(tok1);
            checker &= y.updateValueUnitBackgroundPositionY(tok2);

            if (!checker && x.valueKind() == sideKind &&
                y.valueKind() == sideKind) {
                checker = true;
                checker &= x.updateValueUnitBackgroundPositionX(tok2);
                checker &= y.updateValueUnitBackgroundPositionY(tok1);
            }
            if (!checker) {
                return false;
            }
        } else {
            return false;
        }
        retx->multiValue()->push_back(x);
        rety->multiValue()->push_back(y);
        len = 0;
    }

    return true;
}

static bool parseBackgroundShorthand(
    const CSSTokenVector& tokens, CSSStyleValuePair* _Color,
    CSSStyleValuePair* _Image, CSSStyleValuePair* _RepeatX,
    CSSStyleValuePair* _RepeatY, CSSStyleValuePair* _PositionX,
    CSSStyleValuePair* _PositionY, CSSStyleValuePair* _Size,
    CSSStyleValuePair* _Attachment, CSSStyleValuePair* _Origin,
    CSSStyleValuePair* _Clip, bool allowColor)
{
    // - ACCEPT only 1-word_ : bg-color, bg-image, bg-attachment, bg-origin,
    // bg-clip
    // - ACCEPT 1 to 2 words : bg-repeat, position, bg-size

    size_t len = tokens.size();
    if (len < 1 || len > 11) {
        return false;
    }

    _Color->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    _Image->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    _RepeatX->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    _RepeatY->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    _PositionX->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    _PositionY->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    _Size->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    _Attachment->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    _Origin->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    _Clip->setValueKind(CSSStyleValuePair::ValueKind::Initial);

    bool hasColor = false, hasImage = false, hasRepeat = false,
         hasPosition = false, hasSize = false, hasAttachment = false,
         hasOrigin = false, hasClip = false;
    bool hasPositionPrev = false, shouldSize = false;
    CSSStyleValuePair temp, tempX, tempY;
    CSSTokenValue* tok;
    CSSTokenVector toks;

#define SET_SINGLE_PROP(PROP) \
    *_##PROP = temp;          \
    has##PROP = true;
#define SET_DOUBLE_PROP(PROP) \
    *_##PROP##X = tempX;      \
    *_##PROP##Y = tempY;      \
    has##PROP = true;
#define SINGLE_CONTINUE() continue;
#define DOUBLE_CONTINUE() \
    i++;                  \
    continue;

    for (size_t i = 0; i < len; i++) {
        if (hasPositionPrev) {
            hasPositionPrev = false;
            if (tokens[i].equals("/")) {
                shouldSize = true;
                continue;
            }
        }
        // 1. Verify 2 tokens (current and next token at once)
        // e.g. background: top center; -> means background-position(x:top,
        // y:center)
        if (i + 1 < len) {
            toks.clear();
            toks.push_back(tokens[i]);
            toks.push_back(tokens[i + 1]);
            // 1-1. BackgroundSize
            // 1-2. BackgroundPosition, BackgroundRepeat
            if (shouldSize) {
                STARFISH_ASSERT(!hasSize);
                if (temp.updateValueBackgroundSize(toks, false)) {
                    shouldSize = false;
                    SET_SINGLE_PROP(Size)
                    DOUBLE_CONTINUE()
                }
            } else if (!hasPosition && parseBackgroundPositionShorhand(
                                           toks, &tempX, &tempY, false)) {
                hasPositionPrev = true;
                SET_DOUBLE_PROP(Position)
                DOUBLE_CONTINUE()
            } else if (!hasRepeat && parseBackgroundRepeatShorhand(
                                         toks, &tempX, &tempY, false)) {
                SET_DOUBLE_PROP(Repeat)
                DOUBLE_CONTINUE()
            }
        }
        // 2. Verify single token
        tok = const_cast<CSSTokenValue*>(&tokens[i]);
        toks.clear();
        toks.push_back(*tok);
        // 2-1. BackgroundSize
        // 2-2. BackgroundImage, BackgroundPosition,
        // BackgroundRepeat, BackgroundAttachment,
        // BackgroundOrigin, BackgroundClip, BackgroundColor
        if (shouldSize) {
            STARFISH_ASSERT(!hasSize);
            if (temp.updateValueBackgroundSize(toks, false)) {
                shouldSize = false;
                SET_SINGLE_PROP(Size)
                SINGLE_CONTINUE()
            }
        } else if (!hasImage && temp.updateValueBackgroundImage(toks, false)) {
            SET_SINGLE_PROP(Image)
            SINGLE_CONTINUE()
        } else if (!hasPosition && parseBackgroundPositionShorhand(
                                       toks, &tempX, &tempY, false)) {
            hasPositionPrev = true;
            SET_DOUBLE_PROP(Position)
            SINGLE_CONTINUE()
        } else if (!hasRepeat &&
                   parseBackgroundRepeatShorhand(toks, &tempX, &tempY, false)) {
            SET_DOUBLE_PROP(Repeat)
            SINGLE_CONTINUE()
        } else if (!hasAttachment &&
                   temp.updateValueBackgroundAttachment(toks, false)) {
            SET_SINGLE_PROP(Attachment)
            SINGLE_CONTINUE()
        } else if (!hasOrigin && temp.updateValueBox(toks, false)) {
            SET_SINGLE_PROP(Origin)
            SINGLE_CONTINUE()
        } else if (!hasClip && temp.updateValueBox(toks, false)) {
            SET_SINGLE_PROP(Clip)
            SINGLE_CONTINUE()
        } else if (!hasColor && temp.updateValueUnitColor(*tok)) {
            if (!allowColor) {
                return false;
            }
            SET_SINGLE_PROP(Color)
            SINGLE_CONTINUE()
        }

        return false;
    }
#undef SINGLE_CONTINUE
#undef DOUBLE_CONTINUE
#undef SET_SINGLE_PROP
#undef SET_DOUBLE_PROP

    return true;
}

static bool isRepeatTokenValue(const CSSTokenValue& token)
{
    if (token.equals("stretch") || token.equals("repeat") ||
        token.equals("round") || token.equals("space")) {
        return true;
    }
    return false;
}

static bool parseBorderImageShorthand(const CSSTokenVector& tokens,
                                      CSSStyleValuePair* _source,
                                      CSSStyleValuePair* _slice,
                                      CSSStyleValuePair* _width,
                                      CSSStyleValuePair* _outset,
                                      CSSStyleValuePair* _repeat)
{
    size_t len = tokens.size();
    if (len < 1 || len > 18) {
        return false;
    }

    _source->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    _slice->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    _width->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    _outset->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    _repeat->setValueKind(CSSStyleValuePair::ValueKind::Initial);

    bool hasSource = false, hasSlice = false, hasWidth = false,
         hasOutset = false, hasRepeat = false;
    bool hasSlicePrev = false, shouldWidth = false;
    bool hasWidthPrev = false, shouldOutset = false;
    CSSStyleValuePair temp;
    CSSTokenVector toks;

    size_t pos = 0;
    while (pos < len) {
        CSSTokenValue token = tokens[pos++];
        std::transform(token.begin(), token.end(), token.begin(), ::tolower);
        if (hasSlicePrev) {
            hasSlicePrev = false;
            if (token.equals("/")) {
                shouldWidth = true;
                continue;
            }
        }
        if (hasWidthPrev) {
            hasWidthPrev = false;
            if (token.equals("/")) {
                shouldOutset = true;
                continue;
            }
        }

        if (shouldWidth) {
            shouldWidth = false;
            if (token.equals("/")) {
                shouldOutset = true;
                continue;
            }

            toks.clear();
            toks.push_back(token);
            while (pos < len) {
                token = tokens[pos++];
                if (token.equals("/")) {
                    shouldOutset = true;
                    pos--;
                    break;
                } else if (isRepeatTokenValue(token)) {
                    pos--;
                    break;
                }
                toks.push_back(token);
            }

            if (temp.updateValueUnitBorderImageWidth(toks)) {
                hasWidthPrev = true;
                hasWidth = true;
                *_width = temp;
                continue;
            }
        } else if (shouldOutset) {
            shouldOutset = false;
            toks.clear();
            toks.push_back(token);
            while (pos < len) {
                token = tokens[pos++];
                if (isRepeatTokenValue(token)) {
                    pos--;
                    break;
                }
                toks.push_back(token);
            }

            if (temp.updateValueUnitBorderImageOutset(toks)) {
                hasOutset = true;
                *_outset = temp;
                continue;
            }
        } else if (!hasSource && temp.updateValueUnitBorderImageSource(token)) {
            hasSource = true;
            *_source = temp;
            continue;
        } else if (!hasSlice || !hasRepeat) {
            toks.clear();
            toks.push_back(token);
            bool isRepeat = isRepeatTokenValue(token) ? true : false;

            while (pos < len) {
                token = tokens[pos++];
                if (token.equals("/")) {
                    pos--;
                    break;
                } else if (isRepeatTokenValue(token)) {
                    if (isRepeat) {
                        toks.push_back(token);
                    } else {
                        pos--;
                    }
                    break;
                }
                toks.push_back(token);
            }

            if (isRepeat && temp.updateValueUnitBorderImageRepeat(toks)) {
                hasRepeat = true;
                *_repeat = temp;
                continue;
            } else if (temp.updateValueUnitBorderImageSlice(toks)) {
                hasSlicePrev = true;
                hasSlice = true;
                *_slice = temp;
                continue;
            }
        }
        return false;
    }
    return true;
}

static bool parseFontShorthand(const CSSTokenVector& tokens,
                               CSSStyleValuePair* _Style,
                               // UNSUPPORTED CSSStyleValuePair* _Variant,
                               CSSStyleValuePair* _Weight,
                               // UNSUPPORTED CSSStyleValuePair* _Stretch,
                               CSSStyleValuePair* _Size,
                               CSSStyleValuePair* _LineHeight,
                               CSSStyleValuePair* _Family)
{
    // [font-style|font-weight] font-size[/line-height] font-family
    size_t len = tokens.size();
    if (len < 1) {
        return false;
    }

    _Style->setValueKind(CSSStyleValuePair::ValueKind::FontStyleValueKind);
    _Style->setValue(FontStyleValue::NormalFontStyleValue);
    _Weight->setValueKind(CSSStyleValuePair::ValueKind::FontWeightValueKind);
    _Weight->setValue(FontWeightValue::NormalFontWeightValue);
    _LineHeight->setValueKind(CSSStyleValuePair::ValueKind::Normal);

    bool hasStyle = false, hasWeight = false, hasSize = false,
         hasLineHeight = false, hasFamily = false;
    CSSStyleValuePair temp;
    bool hasSizePrev = false, shouldLineHeight = false;
    size_t pos = 0;
    CSSTokenVector fontFamilyCandidate;

    while (pos < len) {
        const CSSTokenValue& orgToken = tokens[pos];
        CSSTokenValue token = tokens[pos++];
        std::transform(token.begin(), token.end(), token.begin(), ::tolower);

        if (hasSizePrev) {
            hasSizePrev = false;
            if (token.equals("/")) {
                shouldLineHeight = true;
                continue;
            }
        }
        if (shouldLineHeight) {
            shouldLineHeight = false;
            if (temp.updateValueUnitLineHeight(token)) {
                hasLineHeight = true;
                *_LineHeight = temp;
                continue;
            }
        } else if (!hasSize && !hasStyle &&
                   temp.updateValueUnitFontStyle(token)) {
            hasStyle = true;
            *_Style = temp;
            continue;
        } else if (!hasSize && !hasWeight &&
                   temp.updateValueUnitFontWeight(token)) {
            hasWeight = true;
            *_Weight = temp;
            continue;
        } else if (!hasSize && temp.updateValueUnitFontSize(token)) {
            hasSizePrev = true;
            hasSize = true;
            *_Size = temp;
            continue;
        } else if (hasSize) {
            if (hasFamily && token == ",") {
                continue;
            }
            if (token[0] == '\'' && token.length() > 2 &&
                token.back() == '\'') {
                fontFamilyCandidate.push_back(
                    orgToken.substr(1, orgToken.length() - 2));
            } else if (token[0] == '"' && token.length() > 2 &&
                       token.back() == '"') {
                fontFamilyCandidate.push_back(
                    orgToken.substr(1, orgToken.length() - 2));
            } else {
                fontFamilyCandidate.push_back(orgToken);
            }
            hasFamily = true;
            continue;
        }
        return false;
    }
    if (!hasSize || !hasFamily) {
        return false;
    }
    if (fontFamilyCandidate.size() == 1) {
        _Family->setKeywordValue(String::fromUTF8(
            fontFamilyCandidate[0].data(), fontFamilyCandidate[0].length()));
    } else {
        ValueList* val = new ValueList(
            ValueList::Separator::CommaSeparatorAppendQuoteWhenMeetWhiteSpace);
        for (size_t i = 0; i < fontFamilyCandidate.size(); i++) {
            auto str = fontFamilyCandidate[i];
            val->emplace_back(CSSStyleValuePair::ValueKind::KeywordValueKind,
                              String::fromUTF8(str.data(), str.length()));
        }
        _Family->setValueList(val);
    }
    return true;
}

unsigned CSSSelector::specificityForOneSelector() const
{
    unsigned specificity = 0;

    switch (m_type) {
    case Tag:
    case PseudoElement:
        return specificity + 0x000001;
    case Id:
        return specificity + 0x010000;
    case Class:
    case PseudoClass:
    case CSSSelector::AttributeExact:   // Example: E[foo="bar"]
    case CSSSelector::AttributeSet:     // Example: E[foo]
    case CSSSelector::AttributeHyphen:  // Example: E[foo|="bar"]
    case CSSSelector::AttributeList:    // Example: E[foo~="bar"]
    case CSSSelector::AttributeContain: // css3: E[foo*="bar"]
    case CSSSelector::AttributeBegin:   // css3: E[foo^="bar"]
    case CSSSelector::AttributeEnd:     // css3: E[foo$="bar"]
        return specificity + 0x000100;
    default:
        break;
    }

    return 0;
}

String* CSSSelectorList::selectorText(CSSSelectorList* list, unsigned idx,
                                      String* rightSide)
{
    StringBuilder str;
    CSSSelector* cs = list->at(idx);

    if (cs->type() == CSSSelector::Tag ||
        cs->type() == CSSSelector::Universal) {
        str.appendString(cs->selectorText().string());
    }

    while (true) {
        if (cs->type() == CSSSelector::Id) {
            str.appendChar('#');
            str.appendString(cs->selectorText().string());
        } else if (cs->type() == CSSSelector::Class) {
            str.appendChar('.');
            str.appendString(cs->selectorText().string());
        } else if (cs->type() == CSSSelector::PseudoClass) {
            str.appendChar(':');
            str.appendString(cs->selectorText().string());

            CSSPseudoSelector* pcs = cs->asCSSPseudoSelector();

            switch (pcs->pseudoType()) {
            case CSSSelector::PseudoNthChild:
            case CSSSelector::PseudoNthLastChild:
            case CSSSelector::PseudoNthOfType:
            case CSSSelector::PseudoNthLastOfType: {
                str.appendChar('(');

                // http://dev.w3.org/csswg/css-syntax/#serializing-anb
                int a = pcs->nthAValue();
                int b = pcs->nthBValue();
                if (a == 0 && b == 0) {
                    str.appendChar('0');
                } else if (a == 0) {
                    str.appendString(String::fromInt(b));
                } else if (b == 0) {
                    str.appendString(String::fromInt(a));
                    str.appendChar('n');
                } else if (b < 0) {
                    str.appendString(String::fromInt(a));
                    str.appendChar('n');
                    str.appendString(String::fromInt(b));
                } else {
                    str.appendString(String::fromInt(a));
                    str.appendString("n+");
                    str.appendString(String::fromInt(b));
                }

                str.appendChar(')');
                break;
            }
            case CSSSelector::PseudoDir:
            case CSSSelector::PseudoLang:
                str.appendString(pcs->argument());
                str.appendChar(')');
                break;
            case CSSSelector::PseudoNot:
                STARFISH_ASSERT(pcs->pseudoSelectorList().size() > 0);
                str.appendString(pcs->pseudoSelectorList().selectorText());
                str.appendChar(')');
                break;
            default:
                break;
            }
        } else if (cs->type() == CSSSelector::PseudoElement) {
            str.appendString("::");
            str.appendString(cs->selectorText().string());
        } else if (cs->isAttributeSelector()) {
            CSSAttributeSelector* acs = cs->asCSSAttributeSelector();
            str.appendChar('[');
            str.appendString(acs->attribute().localName());
            switch (cs->type()) {
            case CSSSelector::AttributeExact:
                str.appendChar('=');
                break;
            case CSSSelector::AttributeSet:
                // set has no operator or value, just the attrName
                str.appendChar(']');
                break;
            case CSSSelector::AttributeList:
                str.appendString("~=");
                break;
            case CSSSelector::AttributeHyphen:
                str.appendString("|=");
                break;
            case CSSSelector::AttributeBegin:
                str.appendString("^=");
                break;
            case CSSSelector::AttributeEnd:
                str.appendString("$=");
                break;
            case CSSSelector::AttributeContain:
                str.appendString("*=");
                break;
            default:
                break;
            }
            if (acs->type() != CSSSelector::AttributeSet) {
                str.appendChar('\"');
                str.appendString(acs->selectorText().string());
                str.appendChar('\"');

                if (acs->attributeMatch() == CSSSelector::CaseInsensitive) {
                    str.appendString(" i");
                }
                str.appendChar(']');
            }
        }

        if (cs->relation() != CSSSelector::SubSelector ||
            list->size() == (idx + 1)) {
            break;
        }
        cs = list->at(++idx);
    }

    if (list->size() > (idx + 1)) {
        StringBuilder desc;
        switch (cs->relation()) {
        case CSSSelector::Descendant:
            desc.appendString(" ");
            break;
        case CSSSelector::Child:
            desc.appendString(" > ");
            break;
        case CSSSelector::AdjacentSibling:
            desc.appendString(" + ");
            break;
        case CSSSelector::GeneralSibling:
            desc.appendString(" ~ ");
            break;
        case CSSSelector::SubSelector:
            STARFISH_ASSERT_NOT_REACHED();
            break;
        default:
            str.appendString(rightSide);
            return str.finalize();
        }

        desc.appendString(str.finalize());
        desc.appendString(rightSide);
        return selectorText(list, ++idx, desc.finalize());
    }

    str.appendString(rightSide);
    return str.finalize();
}

String* CSSSelectorList::selectorText()
{
    return selectorText(this, 0, String::emptyString);
}

bool CSSSelector::isSimple(CSSSelectorList* selectorList)
{
    if ((isPseudoSelector() &&
         asCSSPseudoSelector()->pseudoSelectorList().size()) ||
        type() == CSSSelector::PseudoElement) {
        return false;
    }

    if (selectorList->size() == 1) {
        return true;
    }
    /*
        if (m_selector->match() == CSSSelector::Tag) {
            // We can't check against anyQName() here because namespace may
            // not be nullAtom.
            // Example:
            //     @namespace "http://www.w3.org/2000/svg";
            //     svg:not(:root) { ...
            if (m_selector->tagQName().localName() == starAtom) {
                    return m_tagHistory->isSimple();
            }
        }
    */

    return false;
}

bool CSSPseudoSelector::matchNth(int count)
{
    if (!nthAValue()) {
        return count == nthBValue();
    }
    if (nthAValue() > 0) {
        if (count < nthBValue()) {
            return false;
        }
        return (count - nthBValue()) % nthAValue() == 0;
    }
    if (count > nthBValue()) {
        return false;
    }
    return (nthBValue() - count) % (-nthAValue()) == 0;
}

CSSSelector::PseudoType CSSPseudoSelector::parsePseudoType(
    StarFish* sf, AtomicString pseudoName, bool hasArguments)
{
    if (pseudoName.isEmptyAtomicString() ||
        !pseudoName.string()->containsOnlyASCIIChars()) {
        return CSSSelector::PseudoNone;
    }
    StaticStrings* sstrs = sf->staticStrings();
    if (false) {
    }
#define SET_PSEUDO_TYPE(name, nameLower, ...)              \
    else if (pseudoName == sstrs->m_##nameLower##Selector) \
    {                                                      \
        return CSSSelector::PseudoType::Pseudo##name;      \
    }
    STARFISH_ENUM_PSEUDO_SELECTORS(SET_PSEUDO_TYPE)
#undef SET_PSEUDO_TYPE
    else
    {
        return CSSSelector::PseudoNone;
    }
}

void CSSPseudoSelector::updatePseudoType(StarFish* sf, AtomicString name,
                                         bool hasArguments)
{
    m_selectorText = name;
    m_pseudotype = parsePseudoType(sf, name, hasArguments);

    switch (pseudoType()) {
    case PseudoAfter:
    case PseudoBefore:
    case PseudoFirstLetter:
    case PseudoFirstLine:
        // The spec says some pseudos allow both single and double colons like
        // :before for backwards compatability. Single colon becomes
        // PseudoClass,
        // but should be PseudoElement like double colon.
        if (type() == PseudoClass) {
            m_type = PseudoElement;
        }
    case PseudoBackdrop:
    case PseudoCue:
    case PseudoGrammarError:
    case PseudoMarker:
    case PseudoPlaceholder:
    /*
        case PseudoResizer:
        case PseudoScrollbar:
        case PseudoScrollbarCorner:
        case PseudoScrollbarButton:
        case PseudoScrollbarThumb:
        case PseudoScrollbarTrack:
        case PseudoScrollbarTrackPiece:
    */
    case PseudoSelection:
    case PseudoSpellingError:
        /*
            case PseudoWebKitCustomElement:
            case PseudoContent:
            case PseudoShadow:
            case PseudoSlotted:
        */
        if (type() != PseudoElement) {
            m_pseudotype = PseudoNone;
        }
        break;
    /*
        case PseudoFirstPage:
        case PseudoLeftPage:
        case PseudoRightPage:
            if (type() != PagePseudoClass) {
                setPseudoType(PseudoUnknown);
            }
            break;
    */
    case PseudoActive:
    case PseudoAnyLink:
    /*
        case PseudoAny:
        case PseudoAutofill:
    */
    case PseudoBlank:
    case PseudoChecked:
    // case PseudoCornerPresent:
    case PseudoCurrent:
    /*
        case PseudoDecrement:
    */
    case PseudoDefault:
    case PseudoDir:
    case PseudoDisabled:
    /*
        case PseudoDoubleButton:
        case PseudoDrag:
    */
    case PseudoDrop:
    case PseudoEmpty:
    case PseudoEnabled:
    // case PseudoEnd:
    case PseudoFirstChild:
    case PseudoFirstOfType:
    case PseudoFocus:
    case PseudoFocusWithin:
    case PseudoFocusVisible:
    // case PseudoFullPageMedia:
    case PseudoFullScreen:
    // case PseudoFullScreenAncestor:
    case PseudoFuture:
    /*
        case PseudoFutureCue:
        case PseudoHorizontal:
        case PseudoHost:
        case PseudoHostContext:
    */
    case PseudoHover:
    case PseudoInRange:
    /*
        case PseudoIncrement:
    */
    case PseudoIndeterminate:
    case PseudoInvalid:
    case PseudoLang:
    case PseudoLastChild:
    case PseudoLastOfType:
    case PseudoLink:
    case PseudoLocalLink:
    /*
        case PseudoListBox:
        case PseudoNoButton:
    */
    case PseudoNot:
    case PseudoNthChild:
    case PseudoNthLastChild:
    case PseudoNthLastOfType:
    case PseudoNthOfType:
    case PseudoOnlyChild:
    case PseudoOnlyOfType:
    case PseudoOptional:
    case PseudoOutOfRange:
    case PseudoPast:
    // case PseudoPastCue:
    case PseudoPaused:
    case PseudoPlaceholderShown:
    case PseudoPlaying:
    case PseudoReadOnly:
    case PseudoReadWrite:
    case PseudoRequired:
    case PseudoRoot:
    case PseudoScope:
    /*
        case PseudoSingleButton:
        case PseudoSpatialNavigationFocus:
        case PseudoStart:
    */
    case PseudoTarget:
    case PseudoTargetWithin:
    case PseudoNone:
    // case PseudoUnresolved:
    case PseudoUserInvalid:
    case PseudoValid:
    // case PseudoVertical:
    case PseudoVisited:
        // case PseudoWindowInactive:
        if (type() != PseudoClass) {
            m_pseudotype = PseudoNone;
        }
        break;
    }
}

String* CSSStyleDeclaration::generateCSSText() const
{
    StringBuilder txt;
    for (size_t i = 0; i < m_cssValues.size(); i++) {
        txt.appendString(m_cssValues[i].keyName());
        txt.appendString(": ");
        txt.appendString(m_cssValues[i].toString());
        if (m_cssValues[i].flagImportant()) {
            txt.appendString(" !important");
        }
        txt.appendString("; ");
    }
    return txt.finalize();
}

String* CSSStyleDeclaration::Border()
{
    bool isWidthCombined;
    String* width = BorderWidth(&isWidthCombined);
    bool isStyleCombined;
    String* style = BorderStyle(&isStyleCombined);
    bool isColorCombined;
    String* color = BorderColor(&isColorCombined);
    return BorderString(width, isWidthCombined, style, isStyleCombined, color,
                        isColorCombined);
}

String* CSSStyleDeclaration::BorderTop()
{
    String* width = BorderTopWidth();
    String* style = BorderTopStyle();
    String* color = BorderTopColor();
    return BorderString(width, false, style, false, color, false);
}

String* CSSStyleDeclaration::BorderRight()
{
    String* width = BorderRightWidth();
    String* style = BorderRightStyle();
    String* color = BorderRightColor();
    return BorderString(width, false, style, false, color, false);
}

String* CSSStyleDeclaration::BorderBottom()
{
    String* width = BorderBottomWidth();
    String* style = BorderBottomStyle();
    String* color = BorderBottomColor();
    return BorderString(width, false, style, false, color, false);
}

String* CSSStyleDeclaration::BorderLeft()
{
    String* width = BorderLeftWidth();
    String* style = BorderLeftStyle();
    String* color = BorderLeftColor();
    return BorderString(width, false, style, false, color, false);
}

static void mergeBordeRadiusString(StringBuilder& sb, String* tl, String* tr,
                                   String* br, String* bl)
{
    if (tl->equals(tr) && tr->equals(br) && br->equals(bl)) {
        sb.appendString(tl);
    } else if (tl->equals(br) && tr->equals(bl)) {
        sb.appendString(tl);
        sb.appendChar(' ');
        sb.appendString(tr);
    } else if (tr->equals(bl)) {
        sb.appendString(tl);
        sb.appendChar(' ');
        sb.appendString(tr);
        sb.appendChar(' ');
        sb.appendString(br);
    } else {
        sb.appendString(tl);
        sb.appendChar(' ');
        sb.appendString(tr);
        sb.appendChar(' ');
        sb.appendString(br);
        sb.appendChar(' ');
        sb.appendString(bl);
    }
}

String* CSSStyleDeclaration::BorderRadius()
{
    String* tl = BorderTopLeftRadius();
    String* tr = BorderTopRightRadius();
    String* br = BorderBottomRightRadius();
    String* bl = BorderBottomLeftRadius();

    size_t stl = tl->indexOf(' ');
    size_t str = tr->indexOf(' ');
    size_t sbr = br->indexOf(' ');
    size_t sbl = bl->indexOf(' ');

    bool needsSlash = (stl != SIZE_MAX) || (str != SIZE_MAX) ||
                      (sbr != SIZE_MAX) || (sbl != SIZE_MAX);
    StringBuilder sb;

    if (needsSlash) {
        String* tl2;
        if (stl != SIZE_MAX) {
            tl2 = tl->substring(0, stl);
        } else {
            tl2 = tl;
        }

        String* tr2;
        if (str != SIZE_MAX) {
            tr2 = tr->substring(0, str);
        } else {
            tr2 = tr;
        }

        String* br2;
        if (sbr != SIZE_MAX) {
            br2 = br->substring(0, sbr);
        } else {
            br2 = br;
        }

        String* bl2;
        if (sbl != SIZE_MAX) {
            bl2 = bl->substring(0, sbl);
        } else {
            bl2 = bl;
        }
        mergeBordeRadiusString(sb, tl2, tr2, br2, bl2);
        sb.appendString(" / ");

        if (stl != SIZE_MAX) {
            tl2 = tl->substring(stl + 1, tl->length() - (stl + 1));
        } else {
            tl2 = tl;
        }

        if (str != SIZE_MAX) {
            tr2 = tr->substring(str + 1, tr->length() - (str + 1));
        } else {
            tr2 = tr;
        }

        if (sbr != SIZE_MAX) {
            br2 = br->substring(sbr + 1, br->length() - (sbr + 1));
        } else {
            br2 = br;
        }

        if (sbl != SIZE_MAX) {
            bl2 = bl->substring(sbl + 1, bl->length() - (sbl + 1));
        } else {
            bl2 = bl;
        }
        mergeBordeRadiusString(sb, tl2, tr2, br2, bl2);
    } else {
        mergeBordeRadiusString(sb, tl, tr, br, bl);
    }

    return sb.finalize();
}

static bool parseBorderShorthand(const CSSTokenVector& tokens,
                                 CSSStyleValuePair* width,
                                 CSSStyleValuePair* style,
                                 CSSStyleValuePair* color)
{
    size_t len = tokens.size();
    if (len < 1 || len > 3) {
        return false;
    }

    width->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    style->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    color->setValueKind(CSSStyleValuePair::ValueKind::Initial);

    bool hasWidth = false, hasStyle = false, hasColor = false;
    CSSStyleValuePair temp;
    for (size_t i = 0; i < len; i++) {
        const CSSTokenValue& tok = tokens[i];
        if (!hasWidth && temp.updateValueUnitBorderWidth(tok)) {
            *width = temp;
            hasWidth = true;
        } else if (!hasStyle && temp.updateValueUnitBorderStyle(tok)) {
            *style = temp;
            hasStyle = true;
        } else if (!hasColor && temp.updateValueUnitColor(tok)) {
            *color = temp;
            hasColor = true;
        } else {
            return false;
        }
    }
    return true;
}

#define ADD_REMOVE_BORDER_CSSVALUES(POS, ...)                                 \
    static void removeBorder##POS##CSSValuePairs(CSSStyleDeclaration* target) \
    {                                                                         \
        target->removeCSSValuePair(                                           \
            CSSStyleValuePair::KeyKind::Border##POS##Width);                  \
        target->removeCSSValuePair(                                           \
            CSSStyleValuePair::KeyKind::Border##POS##Style);                  \
        target->removeCSSValuePair(                                           \
            CSSStyleValuePair::KeyKind::Border##POS##Color);                  \
    }

GEN_FOURSIDE(ADD_REMOVE_BORDER_CSSVALUES)
#undef ADD_REMOVE_BORDER_CSSVALUES

#define ADD_ADD_BORDER_CSSVALUES(POS, ...)                          \
    static void addBorder##POS##CSSValuePairs(                      \
        CSSStyleDeclaration* target, CSSStyleValuePair width,       \
        CSSStyleValuePair style, CSSStyleValuePair color)           \
    {                                                               \
        target->addCSSValuePair(                                    \
            CSSStyleValuePair::KeyKind::Border##POS##Width, width); \
        target->addCSSValuePair(                                    \
            CSSStyleValuePair::KeyKind::Border##POS##Style, style); \
        target->addCSSValuePair(                                    \
            CSSStyleValuePair::KeyKind::Border##POS##Color, color); \
    }

GEN_FOURSIDE(ADD_ADD_BORDER_CSSVALUES)
#undef ADD_ADD_BORDER_CSSVALUES

static void removeBorderCSSValuePairs(CSSStyleDeclaration* target)
{
    removeBorderTopCSSValuePairs(target);
    removeBorderRightCSSValuePairs(target);
    removeBorderBottomCSSValuePairs(target);
    removeBorderLeftCSSValuePairs(target);
}

static void addBorderCSSValuePairs(CSSStyleDeclaration* target,
                                   CSSStyleValuePair width,
                                   CSSStyleValuePair style,
                                   CSSStyleValuePair color)
{
    addBorderTopCSSValuePairs(target, width, style, color);
    addBorderRightCSSValuePairs(target, width, style, color);
    addBorderBottomCSSValuePairs(target, width, style, color);
    addBorderLeftCSSValuePairs(target, width, style, color);
}

void CSSStyleDeclaration::setBorder(const char* value, size_t len,
                                    bool isImportant)
{
    if (len == 0) {
        removeBorderCSSValuePairs(this);
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, len);

    CSSStyleValuePair v, width, style, color;
    if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        addBorderCSSValuePairs(this, v, v, v);
    } else if (parseBorderShorthand(tokens, &width, &style, &color)) {
        width.setFlagImportant(isImportant);
        style.setFlagImportant(isImportant);
        color.setFlagImportant(isImportant);
        addBorderCSSValuePairs(this, width, style, color);
    }
}

#define ADD_SET_BORDER(POS, ...)                                            \
    void CSSStyleDeclaration::setBorder##POS(const char* value, size_t len, \
                                             bool isImportant)              \
    {                                                                       \
        if (len == 0) {                                                     \
            removeBorder##POS##CSSValuePairs(this);                         \
            return;                                                         \
        }                                                                   \
                                                                            \
        CSSTokenVector tokens;                                              \
        tokenizeCSSValue(tokens, value, len);                               \
                                                                            \
        CSSStyleValuePair v, width, style, color;                           \
        if (v.updateValueCommon(tokens)) {                                  \
            v.setFlagImportant(isImportant);                                \
            addBorder##POS##CSSValuePairs(this, v, v, v);                   \
        } else if (parseBorderShorthand(tokens, &width, &style, &color)) {  \
            width.setFlagImportant(isImportant);                            \
            style.setFlagImportant(isImportant);                            \
            color.setFlagImportant(isImportant);                            \
            addBorder##POS##CSSValuePairs(this, width, style, color);       \
        }                                                                   \
    }

GEN_FOURSIDE(ADD_SET_BORDER)
#undef ADD_SET_BORDER

#define CLEAR_BORDER_RADIUS()                                       \
    removeCSSValuePair(CSSStyleValuePair::BorderTopLeftRadius);     \
    removeCSSValuePair(CSSStyleValuePair::BorderTopRightRadius);    \
    removeCSSValuePair(CSSStyleValuePair::BorderBottomRightRadius); \
    removeCSSValuePair(CSSStyleValuePair::BorderBottomLeftRadius);

static bool updateBorderRadiusValue(CSSStyleValuePair* self,
                                    const CSSTokenVector& tokens)
{
    size_t len = tokens.size();
    if (len < 1 || len > 2) {
        return false;
    }
    ValueList* list = new ValueList(ValueList::SpaceSeparator);

    for (size_t i = 0; i < tokens.size(); i++) {
        const CSSTokenValue& t = tokens[i];
        const char* value = t.data();
        CSSStyleValuePair pair;
        if (VALUE_IS_INHERIT()) {
            pair.setValueKind(CSSStyleValuePair::ValueKind::Inherit);
        } else if (VALUE_IS_INITIAL()) {
            pair.setValueKind(CSSStyleValuePair::ValueKind::Initial);
        } else {
            auto ret = pair.updateValueUnitLengthOrCalc(
                t, CSSPropertyParser::AllowPercent);
            if (!ret) {
                return false;
            }
        }
        list->push_back(pair);
    }

    self->setValueList(list);
    return true;
}

void CSSStyleDeclaration::setBorderRadius(const char* value, size_t len,
                                          bool isImportant)
{
    CLEAR_BORDER_RADIUS();
    if (len == 0) {
        return;
    }
    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, len, "/", 1);

    // {1~4} / {1~4}
    if (tokens.size() < 1 || tokens.size() > 9) {
        return;
    }

    bool seenSlash = false;
    size_t beforeSlash = 0;
    size_t afterSlash = 0;

    for (size_t i = 0; i < tokens.size(); i++) {
        const auto& t = tokens[i];
        if (t.equals("/")) {
            if (seenSlash) {
                return;
            }
            if (i == 0) {
                return;
            }
            seenSlash = true;
        } else {
            if (seenSlash) {
                afterSlash++;
            } else {
                beforeSlash++;
            }
            if (beforeSlash > 4 || afterSlash > 4) {
                return;
            }
        }
    }

    if (seenSlash && afterSlash == 0) {
        return;
    }

    CSSTokenVector topLeftV;
    CSSTokenVector topRightV;
    CSSTokenVector bottomRightV;
    CSSTokenVector bottomLeftV;

    if (beforeSlash == 1) {
        topLeftV.push_back(tokens[0]);
        topRightV.push_back(tokens[0]);
        bottomRightV.push_back(tokens[0]);
        bottomLeftV.push_back(tokens[0]);
    } else if (beforeSlash == 2) {
        topLeftV.push_back(tokens[0]);
        topRightV.push_back(tokens[1]);
        bottomRightV.push_back(tokens[0]);
        bottomLeftV.push_back(tokens[1]);
    } else if (beforeSlash == 3) {
        topLeftV.push_back(tokens[0]);
        topRightV.push_back(tokens[1]);
        bottomRightV.push_back(tokens[2]);
        bottomLeftV.push_back(tokens[1]);
    } else {
        STARFISH_ASSERT(beforeSlash == 4);
        topLeftV.push_back(tokens[0]);
        topRightV.push_back(tokens[1]);
        bottomRightV.push_back(tokens[2]);
        bottomLeftV.push_back(tokens[3]);
    }

    size_t base = 1 + beforeSlash;
    if (afterSlash == 1) {
        topLeftV.push_back(tokens[base]);
        topRightV.push_back(tokens[base]);
        bottomRightV.push_back(tokens[base]);
        bottomLeftV.push_back(tokens[base]);
    } else if (afterSlash == 2) {
        topLeftV.push_back(tokens[base]);
        topRightV.push_back(tokens[base + 1]);
        bottomRightV.push_back(tokens[base]);
        bottomLeftV.push_back(tokens[base + 1]);
    } else if (afterSlash == 3) {
        topLeftV.push_back(tokens[base]);
        topRightV.push_back(tokens[base + 1]);
        bottomRightV.push_back(tokens[base + 2]);
        bottomLeftV.push_back(tokens[base + 1]);
    } else if (afterSlash == 4) {
        topLeftV.push_back(tokens[base + 0]);
        topRightV.push_back(tokens[base + 1]);
        bottomRightV.push_back(tokens[base + 2]);
        bottomLeftV.push_back(tokens[base + 3]);
    }

    CSSStyleValuePair topLeft;
    CSSStyleValuePair topRight;
    CSSStyleValuePair bottomRight;
    CSSStyleValuePair bottomLeft;

    if (!updateBorderRadiusValue(&topLeft, topLeftV) ||
        !updateBorderRadiusValue(&topRight, topRightV) ||
        !updateBorderRadiusValue(&bottomRight, bottomRightV) ||
        !updateBorderRadiusValue(&bottomLeft, bottomLeftV)) {
        return;
    }

    topLeft.setFlagImportant(isImportant);
    topRight.setFlagImportant(isImportant);
    bottomRight.setFlagImportant(isImportant);
    bottomLeft.setFlagImportant(isImportant);
    addCSSValuePair(CSSStyleValuePair::BorderTopLeftRadius, topLeft);
    addCSSValuePair(CSSStyleValuePair::BorderTopRightRadius, topRight);
    addCSSValuePair(CSSStyleValuePair::BorderBottomRightRadius, bottomRight);
    addCSSValuePair(CSSStyleValuePair::BorderBottomLeftRadius, bottomLeft);
}

String* CSSStyleValuePair::toString() const
{
    switch (valueKind()) {
    case CSSStyleValuePair::ValueKind::Initial:
        return String::initialString;
    case CSSStyleValuePair::ValueKind::Inherit:
        return String::inheritString;
    case CSSStyleValuePair::ValueKind::Unset:
        return String::unsetString;
    case CSSStyleValuePair::ValueKind::Length:
        return cssLengthValue().toString();
    case CSSStyleValuePair::ValueKind::CalcValueKind:
        return calcValue()->toString();
    case CSSStyleValuePair::ValueKind::Percentage: {
        StringBuilder builder;
        builder.appendString(String::fromFloat(percentageValue() * 100.f));
        builder.appendChar('%');
        return builder.finalize();
    }
    case CSSStyleValuePair::ValueKind::Auto:
        return String::fromUTF8("auto");
    case CSSStyleValuePair::ValueKind::None:
        return String::fromUTF8("none");
    case CSSStyleValuePair::ValueKind::Number:
        return String::fromFloat(numberValue());
    case CSSStyleValuePair::ValueKind::Int32:
#if defined(STARFISH_ANDROID)
        return String::fromInt(int32Value());
#else
        return String::fromUTF8(std::to_string(int32Value()).c_str());
#endif
    case CSSStyleValuePair::ValueKind::Angle:
        return angleValue().toString();
    case CSSStyleValuePair::ValueKind::Normal:
        return String::fromUTF8("normal");
    case CSSStyleValuePair::ValueKind::StringValueKind: {
        StringBuilder builder;
        builder.appendString("\"");
        builder.appendString(stringValue());
        builder.appendString("\"");
        return builder.finalize();
    }
    case CSSStyleValuePair::ValueKind::AtomicStringValueKind:
        return atomicStringValue().string();
    case CSSStyleValuePair::ValueKind::KeywordValueKind:
        return keywordValue();
    case CSSStyleValuePair::ValueKind::ColorValueKind:
        return colorValue().toString();
    case CSSStyleValuePair::ValueKind::NamedColorValueKind:
        return NamedColor::namedColorToString(namedColorValue());
    case CSSStyleValuePair::ValueKind::UrlValueKind: {
        StringBuilder builder;
        builder.appendString("url(\"");
        builder.appendString(urlStringValue());
        builder.appendString("\")");
        return builder.finalize();
    }
    case CSSStyleValuePair::ValueKind::DisplayValueKind:
        switch (displayValue()) {
        case DisplayValue::InlineDisplayValue:
            return String::fromUTF8("inline");
        case DisplayValue::BlockDisplayValue:
            return String::fromUTF8("block");
        case DisplayValue::InlineListItemDisplayValue:
            return String::fromUTF8("inline-list-item");
        case DisplayValue::ListItemDisplayValue:
            return String::fromUTF8("list-item");
        case DisplayValue::InlineBlockDisplayValue:
            return String::fromUTF8("inline-block");
        case DisplayValue::TableDisplayValue:
            return String::fromUTF8("table");
        case DisplayValue::InlineTableDisplayValue:
            return String::fromUTF8("inline-table");
        case DisplayValue::TableRowGroupDisplayValue:
            return String::fromUTF8("table-row-group");
        case DisplayValue::TableHeaderGroupDisplayValue:
            return String::fromUTF8("table-header-group");
        case DisplayValue::TableFooterGroupDisplayValue:
            return String::fromUTF8("table-footer-group");
        case DisplayValue::TableRowDisplayValue:
            return String::fromUTF8("table-row");
        case DisplayValue::TableColumnGroupDisplayValue:
            return String::fromUTF8("table-column-group");
        case DisplayValue::TableColumnDisplayValue:
            return String::fromUTF8("table-column");
        case DisplayValue::TableCellDisplayValue:
            return String::fromUTF8("table-cell");
        case DisplayValue::TableCaptionDisplayValue:
            return String::fromUTF8("table-caption");
        case DisplayValue::FlexDisplayValue:
            return String::fromUTF8("flex");
        case DisplayValue::InlineFlexDisplayValue:
            return String::fromUTF8("inline-flex");
        case DisplayValue::GridDisplayValue:
            return String::fromUTF8("grid");
        case DisplayValue::InlineGridDisplayValue:
            return String::fromUTF8("inline-grid");
        case DisplayValue::NoneDisplayValue:
            return String::fromUTF8("none");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::PositionValueKind:
        switch (positionValue()) {
        case PositionValue::StaticPositionValue:
            return String::fromUTF8("static");
        case PositionValue::RelativePositionValue:
            return String::fromUTF8("relative");
        case PositionValue::AbsolutePositionValue:
            return String::fromUTF8("absolute");
        case PositionValue::FixedPositionValue:
            return String::fromUTF8("fixed");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::FloatValueKind:
        switch (floatValue()) {
        case FloatValue::LeftFloatValue:
            return String::fromUTF8("left");
        case FloatValue::RightFloatValue:
            return String::fromUTF8("right");
        case FloatValue::NoneFloatValue:
            return String::fromUTF8("none");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::ClearValueKind:
        switch (clearValue()) {
        case ClearValue::LeftClearValue:
            return String::fromUTF8("left");
        case ClearValue::RightClearValue:
            return String::fromUTF8("right");
        case ClearValue::BothClearValue:
            return String::fromUTF8("both");
        case ClearValue::NoneClearValue:
            return String::fromUTF8("none");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::VerticalAlignValueKind:
        switch (verticalAlignValue()) {
        case VerticalAlignValue::BaselineVAlignValue:
            return String::fromUTF8("baseline");
        case VerticalAlignValue::SubVAlignValue:
            return String::fromUTF8("sub");
        case VerticalAlignValue::SuperVAlignValue:
            return String::fromUTF8("super");
        case VerticalAlignValue::TopVAlignValue:
            return String::fromUTF8("top");
        case VerticalAlignValue::TextTopVAlignValue:
            return String::fromUTF8("text-top");
        case VerticalAlignValue::MiddleVAlignValue:
            return String::fromUTF8("middle");
        case VerticalAlignValue::BottomVAlignValue:
            return String::fromUTF8("bottom");
        case VerticalAlignValue::TextBottomVAlignValue:
            return String::fromUTF8("text-bottom");
        case VerticalAlignValue::NumericVAlignValue:
            // FIXME:mh.byun
            // FIXED: NumericVAlignValue cannot be here. (only used in
            // ComputedStyle)
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
            break;
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::TextAlignValueKind:
        switch (textAlignValue()) {
        case TextAlignValue::StartTextAlignValue:
            return String::fromUTF8("start");
        case TextAlignValue::EndTextAlignValue:
            return String::fromUTF8("end");
        case TextAlignValue::LeftTextAlignValue:
            return String::fromUTF8("left");
        case TextAlignValue::RightTextAlignValue:
            return String::fromUTF8("right");
        case TextAlignValue::CenterTextAlignValue:
            return String::fromUTF8("center");
        case TextAlignValue::StarFishCenterTextAlignValue:
            return String::fromUTF8("-starfish-center");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::TextTransformValueKind:
        switch (textTransformValue()) {
        case TextTransformValue::NoneTextTransformValue:
            return String::fromUTF8("none");
        case TextTransformValue::CapitalizeTextTransformValue:
            return String::fromUTF8("capitalize");
        case TextTransformValue::UppercaseTextTransformValue:
            return String::fromUTF8("uppercase");
        case TextTransformValue::LowercaseTextTransformValue:
            return String::fromUTF8("lowercase");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::SideValueKind:
        switch (sideValue()) {
        case SideValue::LeftSideValue:
            return String::fromUTF8("left");
        case SideValue::RightSideValue:
            return String::fromUTF8("right");
        case SideValue::CenterSideValue:
            return String::fromUTF8("center");
        case SideValue::TopSideValue:
            return String::fromUTF8("top");
        case SideValue::BottomSideValue:
            return String::fromUTF8("bottom");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::DirectionValueKind:
        switch (directionValue()) {
        case LtrDirectionValue:
            return String::fromUTF8("ltr");
        case RtlDirectionValue:
            return String::fromUTF8("rtl");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::WhiteSpaceValueKind:
        switch (whiteSpaceValue()) {
        case NormalWhiteSpaceValue:
            return String::fromUTF8("normal");
        case NoWrapWhiteSpaceValue:
            return String::fromUTF8("nowrap");
        case PreWhiteSpaceValue:
            return String::fromUTF8("pre");
        case PreWrapWhiteSpaceValue:
            return String::fromUTF8("pre-wrap");
        case PreLineWhiteSpaceValue:
            return String::fromUTF8("pre-line");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::ObjectFitValueKind:
        switch (objectFitValue()) {
        case FillObjectFitValue:
            return String::fromUTF8("fill");
        case ContainObjectFitValue:
            return String::fromUTF8("contain");
        case CoverObjectFitValue:
            return String::fromUTF8("cover");
        case NoneObjectFitValue:
            return String::fromUTF8("none");
        case ScaledownObjectFitValue:
            return String::fromUTF8("scale-down");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::BackgroundSizeValueKind:
        switch (backgroundSizeValue()) {
        case CoverBackgroundSizeValue:
            return String::fromUTF8("cover");
        case ContainBackgroundSizeValue:
            return String::fromUTF8("contain");
        }
        break;
    case CSSStyleValuePair::ValueKind::BackgroundRepeatValueKind:
        switch (backgroundRepeatValue()) {
        case RepeatRepeatValue:
            return String::fromUTF8("repeat");
        case NoRepeatRepeatValue:
            return String::fromUTF8("no-repeat");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::BackgroundAttachmentValueKind:
        switch (backgroundAttachmentValue()) {
        case ScrollBackgroundAttachmentValue:
            return String::fromUTF8("scroll");
        case FixedBackgroundAttachmentValue:
            return String::fromUTF8("fixed");
        case LocalBackgroundAttachmentValue:
            return String::fromUTF8("local");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::BoxValueKind:
        switch (boxValue()) {
        case BorderBoxBoxValue:
            return String::fromUTF8("border-box");
        case PaddingBoxBoxValue:
            return String::fromUTF8("padding-box");
        case ContentBoxBoxValue:
            return String::fromUTF8("content-box");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::FontSizeValueKind:
        switch (fontSizeValue()) {
        case FontSizeValue::XXSmallFontSizeValue:
            return String::fromUTF8("xx-small");
        case FontSizeValue::XSmallFontSizeValue:
            return String::fromUTF8("x-small");
        case FontSizeValue::SmallFontSizeValue:
            return String::fromUTF8("small");
        case FontSizeValue::MediumFontSizeValue:
            return String::fromUTF8("medium");
        case FontSizeValue::LargeFontSizeValue:
            return String::fromUTF8("large");
        case FontSizeValue::XLargeFontSizeValue:
            return String::fromUTF8("x-large");
        case FontSizeValue::XXLargeFontSizeValue:
            return String::fromUTF8("xx-large");
        case FontSizeValue::LargerFontSizeValue:
            return String::fromUTF8("larger");
        case FontSizeValue::SmallerFontSizeValue:
            return String::fromUTF8("smaller");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::FontStyleValueKind:
        switch (fontStyleValue()) {
        case FontStyleValue::NormalFontStyleValue:
            return String::fromUTF8("normal");
        case FontStyleValue::ItalicFontStyleValue:
            return String::fromUTF8("italic");
        case FontStyleValue::ObliqueFontStyleValue:
            return String::fromUTF8("oblique");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::FontWeightValueKind:
        switch (fontWeightValue()) {
        case FontWeightValue::NormalFontWeightValue:
            return String::fromUTF8("normal");
        case FontWeightValue::BoldFontWeightValue:
            return String::fromUTF8("bold");
        case FontWeightValue::BolderFontWeightValue:
            return String::fromUTF8("bolder");
        case FontWeightValue::LighterFontWeightValue:
            return String::fromUTF8("lighter");
        case FontWeightValue::OneHundredFontWeightValue:
            return String::fromUTF8("100");
        case FontWeightValue::TwoHundredsFontWeightValue:
            return String::fromUTF8("200");
        case FontWeightValue::ThreeHundredsFontWeightValue:
            return String::fromUTF8("300");
        case FontWeightValue::FourHundredsFontWeightValue:
            return String::fromUTF8("400");
        case FontWeightValue::FiveHundredsFontWeightValue:
            return String::fromUTF8("500");
        case FontWeightValue::SixHundredsFontWeightValue:
            return String::fromUTF8("600");
        case FontWeightValue::SevenHundredsFontWeightValue:
            return String::fromUTF8("700");
        case FontWeightValue::EightHundredsFontWeightValue:
            return String::fromUTF8("800");
        case FontWeightValue::NineHundredsFontWeightValue:
            return String::fromUTF8("900");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::FontKerningValueKind:
        switch (fontKerningValue()) {
        case FontKerningValue::FontKerningAutoValue:
            return String::fromUTF8("auto");
        case FontKerningValue::FontKerningNormalValue:
            return String::fromUTF8("normal");
        case FontKerningValue::FontKerningNoneValue:
            return String::fromUTF8("none");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::WordWrapValueKind:
        switch (wordWrapValue()) {
        case WordWrapValue::NormalWordWrapValue:
            return String::fromUTF8("normal");
        case WordWrapValue::BreakWordWordWrapValue:
            return String::fromUTF8("break-word");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::BorderStyleValueKind:
        switch (borderStyleValue()) {
        case BorderStyleValue::NoneBorderStyleValue:
            return String::fromUTF8("none");
        case BorderStyleValue::HiddenBorderStyleValue:
            return String::fromUTF8("hidden");
        case BorderStyleValue::SolidBorderStyleValue:
            return String::fromUTF8("solid");
        case BorderStyleValue::DashedBorderStyleValue:
            return String::fromUTF8("dashed");
        case BorderStyleValue::DottedBorderStyleValue:
            return String::fromUTF8("dotted");
        case BorderStyleValue::InsetBorderStyleValue:
            return String::fromUTF8("inset");
        case BorderStyleValue::OutsetBorderStyleValue:
            return String::fromUTF8("outset");
        case BorderStyleValue::DoubleBorderStyleValue:
            return String::fromUTF8("double");
        case BorderStyleValue::GrooveBorderStyleValue:
            return String::fromUTF8("groove");
        case BorderStyleValue::RidgeBorderStyleValue:
            return String::fromUTF8("ridge");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::BorderWidthValueKind:
        switch (borderWidthValue()) {
        case BorderWidthValue::ThinBorderWidthValue:
            return String::fromUTF8("thin");
        case BorderWidthValue::MediumBorderWidthValue:
            return String::fromUTF8("medium");
        case BorderWidthValue::ThickBorderWidthValue:
            return String::fromUTF8("thick");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::BorderImageRepeatValueKind:
        switch (borderImageRepeatValue()) {
        case BorderImageRepeatValue::StretchValue:
            return String::fromUTF8("stretch");
        case BorderImageRepeatValue::RepeatValue:
            return String::fromUTF8("repeat");
        case BorderImageRepeatValue::RoundValue:
            return String::fromUTF8("round");
        case BorderImageRepeatValue::SpaceValue:
            return String::fromUTF8("space");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::OverflowValueKind:
        switch (overflowValue()) {
        case OverflowValue::VisibleOverflow:
            return String::fromUTF8("visible");
        case OverflowValue::HiddenOverflow:
            return String::fromUTF8("hidden");
        case OverflowValue::AutoOverflow:
            return String::fromUTF8("auto");
        case OverflowValue::ScrollOverflow:
            return String::fromUTF8("scroll");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::TextDecorationLineValueKind:
        switch (textDecorationLineValue()) {
        case TextDecorationLineValue::NoneTextDecorationLineValue:
            return String::fromUTF8("none");
        case TextDecorationLineValue::UnderlineTextDecorationLineValue:
            return String::fromUTF8("underline");
        case TextDecorationLineValue::OverlineTextDecorationLineValue:
            return String::fromUTF8("overline");
        case TextDecorationLineValue::LineThroughTextDecorationLineValue:
            return String::fromUTF8("line-through");
        case TextDecorationLineValue::BlinkTextDecorationLineValue:
            return String::fromUTF8("blink");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::TextDecorationStyleValueKind:
        switch (textDecorationStyleValue()) {
        case TextDecorationStyleValue::SolidTextDecorationStyleValue:
            return String::fromUTF8("solid");
        case TextDecorationStyleValue::DoubleTextDecorationStyleValue:
            return String::fromUTF8("double");
        case TextDecorationStyleValue::DottedTextDecorationStyleValue:
            return String::fromUTF8("dotted");
        case TextDecorationStyleValue::DashedTextDecorationStyleValue:
            return String::fromUTF8("dashed");
        case TextDecorationStyleValue::WavyTextDecorationStyleValue:
            return String::fromUTF8("wavy");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::TextUnderlinePositionValueKind:
        switch (textUnderlinePositionValue()) {
        case TextUnderlinePositionValue::AutoTextUnderlinePositionValue:
            return String::fromUTF8("auto");
        case TextUnderlinePositionValue::UnderTextUnderlinePositionValue:
            return String::fromUTF8("under");
        case TextUnderlinePositionValue::LeftTextUnderlinePositionValue:
            return String::fromUTF8("left");
        case TextUnderlinePositionValue::RightTextUnderlinePositionValue:
            return String::fromUTF8("right");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::ResizeValueKind:
        switch (resizeValue()) {
        case ResizeValue::NoneResizeValue:
            return String::fromUTF8("none");
        case ResizeValue::BothResizeValue:
            return String::fromUTF8("both");
        case ResizeValue::HorizontalResizeValue:
            return String::fromUTF8("horizontal");
        case ResizeValue::VerticalResizeValue:
            return String::fromUTF8("vertical");
        case ResizeValue::BlockResizeValue:
            return String::fromUTF8("block");
        case ResizeValue::InlineResizeValue:
            return String::fromUTF8("inline");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::VisibilityValueKind:
        switch (visibilityValue()) {
        case VisibilityValue::VisibleVisibilityValue:
            return String::fromUTF8("visible");
        case VisibilityValue::CollapseVisibilityValue:
            return String::fromUTF8("collapse");
        case VisibilityValue::HiddenVisibilityValue:
            return String::fromUTF8("hidden");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::ImageRenderingValueKind:
        switch (imageRenderingValue()) {
        case ImageRenderingValue::ImageRenderingAutoValue:
            return String::fromUTF8("auto");
        case ImageRenderingValue::ImageRenderingPixelatedValue:
            return String::fromUTF8("pixelated");
        case ImageRenderingValue::ImageRenderingCrispEdgesValue:
            return String::fromUTF8("crisp-edges");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::UnicodeBidiValueKind:
        switch (unicodeBidiValue()) {
        case NormalUnicodeBidiValue:
            return String::fromUTF8("normal");
        case EmbedUnicodeBidiValue:
            return String::fromUTF8("embed");
        case IsolateUnicodeBidiValue:
            return String::fromUTF8("isolate");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::TransformFunctions:
        return transformValue()->toString();
    case CSSStyleValuePair::ValueKind::ValueListKind: {
        ValueList* list = multiValue();
        return list->toString();
    }
    case CSSStyleValuePair::ValueKind::TransitionPropertyValueKind:
        return transitionPropertyValueToString(transitionPropertyValue());
    case CSSStyleValuePair::ValueKind::Time:
        return timeValue().toString();
    case CSSStyleValuePair::ValueKind::TransitionTimingFunctionValueKind:
        switch (transitionTimingFunctionValue()) {
        case TransitionTimingFunctionEaseValue:
            return String::fromUTF8("ease");
        case TransitionTimingFunctionLinearValue:
            return String::fromUTF8("linear");
        case TransitionTimingFunctionEaseInValue:
            return String::fromUTF8("ease-in");
        case TransitionTimingFunctionEaseOutValue:
            return String::fromUTF8("ease-out");
        case TransitionTimingFunctionEaseInOutValue:
            return String::fromUTF8("ease-in-out");
        case TransitionTimingFunctionStepStartValue:
            return String::fromUTF8("step-start");
        case TransitionTimingFunctionStepEndValue:
            return String::fromUTF8("step-end");
        case TransitionTimingFunctionStepsValue:
            return String::fromUTF8("steps");
        case TransitionTimingFunctionCubicBezierValue:
            return String::fromUTF8("cubic-bezier");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::BoxSizingValueKind:
        switch (boxSizingValue()) {
        case ContentBoxBoxSizingValue:
            return String::fromUTF8("content-box");
        case BorderBoxBoxSizingValue:
            return String::fromUTF8("border-box");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::FlexDirectionValueKind:
        switch (flexDirectionValue()) {
        case RowFlexDirectionValue:
            return String::fromUTF8("row");
        case RowReverseFlexDirectionValue:
            return String::fromUTF8("row-reverse");
        case ColumnFlexDirectionValue:
            return String::fromUTF8("column");
        case ColumnReverseFlexDirectionValue:
            return String::fromUTF8("column-reverse");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::FlexWrapValueKind:
        switch (flexWrapValue()) {
        case NoWrapFlexWrapValue:
            return String::fromUTF8("nowrap");
        case WrapFlexWrapValue:
            return String::fromUTF8("wrap");
        case WrapReverseFlexWrapValue:
            return String::fromUTF8("wrap-reverse");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::JustifyContentValueKind:
        switch (justifyContentValue()) {
        case FlexStartJustifyContentValue:
            return String::fromUTF8("flex-start");
        case FlexEndJustifyContentValue:
            return String::fromUTF8("flex-end");
        case CenterJustifyContentValue:
            return String::fromUTF8("center");
        case SpaceBetweenJustifyContentValue:
            return String::fromUTF8("space-between");
        case SpaceAroundJustifyContentValue:
            return String::fromUTF8("space-around");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::AlignItemValueKind:
        switch (alignItemValue()) {
        case FlexStartAlignItemValue:
            return String::fromUTF8("flex-start");
        case FlexEndAlignItemValue:
            return String::fromUTF8("flex-end");
        case CenterAlignItemValue:
            return String::fromUTF8("center");
        case BaselineAlignItemValue:
            return String::fromUTF8("baseline");
        case StretchAlignItemValue:
            return String::fromUTF8("stretch");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::AlignContentValueKind:
        switch (alignContentValue()) {
        case FlexStartAlignContentValue:
            return String::fromUTF8("flex-start");
        case FlexEndAlignContentValue:
            return String::fromUTF8("flex-end");
        case SpaceBetweenAlignContentValue:
            return String::fromUTF8("space-between");
        case SpaceAroundAlignContentValue:
            return String::fromUTF8("space-around");
        case StretchAlignContentValue:
            return String::fromUTF8("stretch");
        case CenterAlignContentValue:
            return String::fromUTF8("center");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::FlexBasisValueKind:
        switch (FlexBasisValue()) {
        case ContentFlexBasisValue:
            return String::fromUTF8("content");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::TableLayoutValueKind:
        switch (tableLayoutValue()) {
        case TableLayoutValue::AutoTableLayoutValue:
            return String::fromUTF8("auto");
        case TableLayoutValue::FixedTableLayoutValue:
            return String::fromUTF8("fixed");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::FillRuleValueKind:
        switch (fillRuleValue()) {
        case FillRuleNonZero:
            return String::fromUTF8("nonzero");
        case FillRuleEvenOdd:
            return String::fromUTF8("evenodd");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::FontFaceSrcDataValueKind:
        return fontFaceSrcDataValue()->toString();
    case CSSStyleValuePair::ValueKind::MaskSizeValueKind:
        switch (MaskSizeValue()) {
        case CoverMaskSizeValue:
            return String::fromUTF8("cover");
        case ContainMaskSizeValue:
            return String::fromUTF8("contain");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::ListStylePositionValueKind:
        switch (listStylePositionValue()) {
        case ListStylePositionOutside:
            return String::fromUTF8("outside");
        case ListStylePositionInside:
            return String::fromUTF8("inside");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::RectValueKind:
        if (m_valueKind == CSSStyleValuePair::ValueKind::RectValueKind) {
            return clip()->toString();
        }
        return String::fromUTF8("auto");
    case CSSStyleValuePair::ValueKind::BorderCollapseValueKind:
        switch (borderCollapseValue()) {
        case SeparateBorderCollapseValue:
            return String::fromUTF8("separate");
        case CollapseBorderCollapseValue:
            return String::fromUTF8("collapse");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::CaptionSideValueKind:
        switch (captionSideValue()) {
        case TopCaptionSideValue:
            return String::fromUTF8("top");
        case BottomCaptionSideValue:
            return String::fromUTF8("bottom");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::EmptyCellsValueKind:
        switch (emptyCellsValue()) {
        case ShowEmptyCellsValue:
            return String::fromUTF8("show");
        case HideEmptyCellsValue:
            return String::fromUTF8("hide");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::UserSelectValueKind:
        switch (userSelectValue()) {
        case NoneUserSelectValue:
            return String::fromUTF8("none");
        case TextUserSelectValue:
            return String::fromUTF8("text");
        case ContainUserSelectValue:
            return String::fromUTF8("contain");
        case AllUserSelectValue:
            return String::fromUTF8("all");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::HyphensValueKind:
        switch (hyphensValue()) {
        case NoneHyphensValue:
            return String::fromUTF8("none");
        case ManualHyphensValue:
            return String::fromUTF8("manual");
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case CSSStyleValuePair::ValueKind::LineBreakValueKind:
        switch (lineBreakValue()) {
        case LooseLineBreakValue:
            return String::fromUTF8("loose");
        case NormalLineBreakValue:
            return String::fromUTF8("normal");
        case StrictLineBreakValue:
            return String::fromUTF8("strict");
        }
        break;
    case CSSStyleValuePair::ValueKind::WordBreakValueKind:
        switch (wordBreakValue()) {
        case NormalWordBreakValue:
            return String::fromUTF8("normal");
        case BreakAllWordBreakValue:
            return String::fromUTF8("break-all");
        case KeepAllWordBreakValue:
            return String::fromUTF8("keep-all");
        case BreakWordWordBreakValue:
            return String::fromUTF8("break-word");
        }
        break;
    case CSSStyleValuePair::ValueKind::GridTemplateUnits:
        return GridLength::toStringWithGridLengths(gridTemplateUnits());
    case CSSStyleValuePair::ValueKind::CounterFunctionValueKind:
        return counterFunctionValue()->toString();
    case CSSStyleValuePair::ValueKind::VarFunctionValueKind:
        return varFunctionValue();
    case CSSStyleValuePair::ValueKind::TextOverflowValueKind: {
        auto value = textOverflowValue();
        if (value.hasClipValue()) {
            return String::fromUTF8("auto");
        } else if (value.hasEllipsisValue()) {
            return String::fromUTF8("ellipsis");
        } else if (value.hasStringValue()) {
            StringBuilder builder;
            builder.appendString("\"");
            builder.appendString(value.stringValue());
            builder.appendString("\"");
            return builder.finalize();
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    }
    case CSSStyleValuePair::ValueKind::GradientValueKind:
        return gradientValue()->toString();
    case CSSStyleValuePair::ValueKind::WidthHeightKeywordValueKind:
        switch (widthHeightKeywordValue()) {
        case AvailableValue:
            return String::fromUTF8("available");
        case MinContentValue:
            return String::fromUTF8("min-content");
        case MaxContentValue:
            return String::fromUTF8("max-content");
        case FitContentValue:
            return String::fromUTF8("fit-content");
        }
        break;
    default:
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

String* CSSStyleDeclaration::BackgroundRepeat()
{
    String* repeatX = BackgroundRepeatX();
    String* repeatY = BackgroundRepeatY();
    GCVector<StringView> vRepeatX, vRepeatY;
    StringUtils::tokenize(repeatX, ",", 1, vRepeatX);
    StringUtils::tokenize(repeatY, ",", 1, vRepeatY);

    StringBuilder builder;
    for (size_t i = 0; i < vRepeatX.size(); i++) {
        String* rX = vRepeatX[i].trim();
        String* rY = vRepeatY[i].trim();
        if (rX->equals("repeat") && rY->equals("repeat")) {
            builder.appendString("repeat");
        } else if (rX->equals("repeat") && rY->equals("no-repeat")) {
            builder.appendString("repeat-x");
        } else if (rX->equals("no-repeat") && rY->equals("repeat")) {
            builder.appendString("repeat-y");
        } else if (rX->equals("no-repeat") && rY->equals("no-repeat")) {
            builder.appendString("no-repeat");
        } else if (rX->equals(String::initialString) &&
                   rY->equals(String::initialString)) {
            builder.appendString(String::initialString);
        } else if (rX->equals(String::inheritString) &&
                   rY->equals(String::inheritString)) {
            builder.appendString(String::inheritString);
        } else {
            builder.appendString(String::emptyString);
        }
        if (i != vRepeatX.size() - 1) {
            builder.appendChar(',');
            builder.appendString(String::spaceString);
        }
    }
    return builder.finalize();
}

void CSSStyleDeclaration::setBackgroundRepeat(const char* value, size_t length,
                                              bool isImportant)
{
    if (value == 0) {
        removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundRepeatX);
        removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundRepeatY);
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, length, ",", 1);

    CSSStyleValuePair c, x, y;
    if (c.updateValueCommon(tokens)) {
        c.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundRepeatX, c);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundRepeatY, c);
    } else if (parseBackgroundRepeatShorhand(tokens, &x, &y)) {
        x.setFlagImportant(isImportant);
        y.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundRepeatX, x);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundRepeatY, y);
    }
}

String* CSSStyleDeclaration::BackgroundPosition()
{
    String* positionX = BackgroundPositionX();
    String* positionY = BackgroundPositionY();

    if (positionX->equals(String::emptyString) ||
        positionY->equals(String::emptyString)) {
        return String::emptyString;
    }

    GCVector<StringView> vPositionX, vPositionY;
    StringUtils::tokenize(positionX, ",", 1, vPositionX);
    StringUtils::tokenize(positionY, ",", 1, vPositionY);

    StringBuilder builder;
    for (size_t i = 0; i < vPositionX.size(); i++) {
        String* pX = vPositionX[i].trim();
        String* pY = vPositionY[i].trim();
        if (pX->equals(String::initialString)) {
            if (pY->equals(String::initialString)) {
                builder.appendString(String::initialString);
            } else {
                builder.appendString(String::emptyString);
            }
        } else if (pX->equals(String::inheritString)) {
            if (pY->equals(String::inheritString)) {
                builder.appendString(String::inheritString);
            } else {
                builder.appendString(String::emptyString);
            }
        } else {
            builder.appendString(pX);
            builder.appendString(String::spaceString);
            builder.appendString(pY);
        }
        if (i != vPositionX.size() - 1) {
            builder.appendChar(',');
            builder.appendString(String::spaceString);
        }
    }
    return builder.finalize();
}

void CSSStyleDeclaration::setBackgroundPosition(const char* value,
                                                size_t length, bool isImportant)
{
    if (value == 0) {
        removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundPositionX);
        removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundPositionY);
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, length, ",", 1);

    CSSStyleValuePair c, x, y;
    if (c.updateValueCommon(tokens)) {
        c.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundPositionX, c);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundPositionY, c);
    } else if (parseBackgroundPositionShorhand(tokens, &x, &y)) {
        x.setFlagImportant(isImportant);
        y.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundPositionX, x);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundPositionY, y);
    }
}

static String* printBackground(String* image, String* position, String* size,
                               String* repeat, String* attachment,
                               String* origin, String* clip, String* color)
{
    if (image->equals(String::inheritString) ||
        position->equals(String::inheritString) ||
        size->equals(String::inheritString) ||
        repeat->equals(String::inheritString) ||
        attachment->equals(String::inheritString) ||
        origin->equals(String::inheritString) ||
        clip->equals(String::inheritString) ||
        color->equals(String::inheritString)) {
        return String::emptyString;
    }

    StringBuilder builder;

    if (image->length() != 0 && !image->equals(String::initialString)) {
        builder.appendString(image);
    }
    if (position->length() != 0 && !position->equals(String::initialString)) {
        if (builder.contentLength() > 0) {
            builder.appendString(String::spaceString);
        }
        builder.appendString(position);
    }
    if (size->length() != 0 && !size->equals(String::initialString)) {
        if (builder.contentLength() > 0) {
            builder.appendString(String::spaceString);
        }
        if (position->length() == 0) {
            builder.appendString("0% 0%");
        }
        builder.appendString(" / ");
        builder.appendString(size);
    }
    if (repeat->length() != 0 && !repeat->equals(String::initialString)) {
        if (builder.contentLength() > 0) {
            builder.appendString(String::spaceString);
        }
        builder.appendString(repeat);
    }
    if (attachment->length() != 0 &&
        !attachment->equals(String::initialString)) {
        if (builder.contentLength() > 0) {
            builder.appendString(String::spaceString);
        }
        builder.appendString(attachment);
    }
    if (origin->length() != 0 && !origin->equals(String::initialString)) {
        if (builder.contentLength() > 0) {
            builder.appendString(String::spaceString);
        }
        builder.appendString(origin);
    }
    if (clip->length() != 0 && !clip->equals(String::initialString)) {
        if (builder.contentLength() > 0) {
            builder.appendString(String::spaceString);
        }
        builder.appendString(clip);
    }
    if (color->length() != 0 && !color->equals(String::initialString)) {
        if (builder.contentLength() > 0) {
            builder.appendString(String::spaceString);
        }
        builder.appendString(color);
    }

    return builder.finalize();
}

String* CSSStyleDeclaration::Background()
{
    StringBuilder builder;
    String* images = BackgroundImage();
    String* positions = BackgroundPosition();
    String* sizes = BackgroundSize();
    String* repeats = BackgroundRepeat();
    String* attachments = BackgroundAttachment();
    String* origins = BackgroundOrigin();
    String* clips = BackgroundClip();
    String* color = BackgroundColor();
    GCVector<StringView> vImages, vPositions, vSizes, vRepeats, vAttachments,
        vOrigins, vClips;
    StringUtils::tokenize(images, ",", 1, vImages);
    StringUtils::tokenize(positions, ",", 1, vPositions);
    StringUtils::tokenize(sizes, ",", 1, vSizes);
    StringUtils::tokenize(repeats, ",", 1, vRepeats);
    StringUtils::tokenize(attachments, ",", 1, vAttachments);
    StringUtils::tokenize(origins, ",", 1, vOrigins);
    StringUtils::tokenize(clips, ",", 1, vClips);

    size_t max = vImages.size();
    if (max < vPositions.size()) {
        max = vPositions.size();
    }
    if (max < vSizes.size()) {
        max = vSizes.size();
    }
    if (max < vRepeats.size()) {
        max = vRepeats.size();
    }
    if (max < vAttachments.size()) {
        max = vAttachments.size();
    }
    if (max < vOrigins.size()) {
        max = vOrigins.size();
    }
    if (max < vClips.size()) {
        max = vClips.size();
    }

    for (unsigned int i = 0; i < max; i++) {
        String* image =
            (i < vImages.size()) ? vImages[i].trim() : String::emptyString;
        String* position = (i < vPositions.size()) ? vPositions[i].trim()
                                                   : String::emptyString;
        String* size =
            (i < vSizes.size()) ? vSizes[i].trim() : String::emptyString;
        String* repeat =
            (i < vRepeats.size()) ? vRepeats[i].trim() : String::emptyString;
        String* attachment = (i < vAttachments.size()) ? vAttachments[i].trim()
                                                       : String::emptyString;
        String* origin =
            (i < vOrigins.size()) ? vOrigins[i].trim() : String::emptyString;
        String* clip =
            (i < vClips.size()) ? vClips[i].trim() : String::emptyString;
        builder.appendString(
            printBackground(image, position, size, repeat, attachment, origin,
                            clip, i == max - 1 ? color : String::emptyString));
        if (i != max - 1) {
            builder.appendString(", ");
        }
    }
    return builder.finalize();
}

String* CSSStyleDeclaration::BorderImage()
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return String::emptyString;
}

static void removeBackgroundCSSValuePairs(CSSStyleDeclaration* target)
{
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundColor);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundImage);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundRepeatX);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundRepeatY);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundPositionX);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundPositionY);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundSize);
    target->removeCSSValuePair(
        CSSStyleValuePair::KeyKind::BackgroundAttachment);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundOrigin);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundClip);
}

static void addBackgroundCSSValuePairs(
    CSSStyleDeclaration* target, CSSStyleValuePair color,
    CSSStyleValuePair image, CSSStyleValuePair repeatX,
    CSSStyleValuePair repeatY, CSSStyleValuePair positionX,
    CSSStyleValuePair positionY, CSSStyleValuePair size,
    CSSStyleValuePair attachment, CSSStyleValuePair origin,
    CSSStyleValuePair clip)
{
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundColor, color);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundImage, image);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundRepeatX,
                            repeatX);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundRepeatY,
                            repeatY);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundPositionX,
                            positionX);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundPositionY,
                            positionY);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundSize, size);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundAttachment,
                            attachment);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundOrigin,
                            origin);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundClip, clip);
}

static void removeBorderImageCSSValuePairs(CSSStyleDeclaration* target)
{
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderImageSource);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderImageSlice);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderImageWidth);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderImageOutset);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderImageRepeat);
}

static void addBorderImageCSSValuePairs(CSSStyleDeclaration* target,
                                        CSSStyleValuePair source,
                                        CSSStyleValuePair slice,
                                        CSSStyleValuePair width,
                                        CSSStyleValuePair outset,
                                        CSSStyleValuePair repeat)
{
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::BorderImageSource,
                            source);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::BorderImageSlice,
                            slice);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::BorderImageWidth,
                            width);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::BorderImageOutset,
                            outset);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::BorderImageRepeat,
                            repeat);
}

static bool attributeValueMatches(
    String* attrValue, CSSSelector::Type type, String* selectorValue,
    CSSSelector::AttributeMatchType caseSensitivity)
{
    // For AttributeSet and AttributeExact attribute selectors, the attribute
    // value can be empty string.
    if (type != CSSSelector::AttributeSet &&
        type != CSSSelector::AttributeExact &&
        attrValue->equals(String::emptyString)) {
        return false;
    }

    switch (type) {
    case CSSSelector::AttributeSet: // Example: E[foo]
        return true;
    case CSSSelector::AttributeExact: // Example: E[foo="bar"]
        if (caseSensitivity) {
            return selectorValue->equals(attrValue);
        }
        return selectorValue->equalsIgnoreCase(attrValue);
    case CSSSelector::AttributeList: // Example: E[foo~="bar"]
    {
        if (selectorValue->equals(String::emptyString) ||
            selectorValue->containsWhitespace()) {
            return false;
        }

        unsigned startSearchAt = 0;
        while (true) {
            size_t foundPos =
                attrValue->find(selectorValue, startSearchAt, caseSensitivity);
            if (foundPos == SIZE_MAX) {
                return false;
            }
            if (!foundPos ||
                String::isASCIISpace(attrValue->charAt(foundPos - 1))) {
                unsigned endStr = foundPos + selectorValue->length();
                if (endStr == attrValue->length() ||
                    String::isASCIISpace(attrValue->charAt(endStr))) {
                    break; // We found a match.
                }
            }

            // No match. Keep looking.
            startSearchAt = foundPos + 1;
        }
        return true;
    }
    case CSSSelector::AttributeHyphen: // Example: E[foo|="bar"]
        if (attrValue->length() < selectorValue->length()) {
            return false;
        }
        if (!attrValue->startsWith(selectorValue, caseSensitivity)) {
            return false;
        }
        // It they start the same, check for exact match or following '-':
        if (attrValue->length() != selectorValue->length() &&
            attrValue->charAt(selectorValue->length()) != '-') {
            return false;
        }
        return true;
    case CSSSelector::AttributeContain: // css3: E[foo*="bar"]
        if (selectorValue->equals(String::emptyString)) {
            return false;
        }
        return attrValue->contains(selectorValue, caseSensitivity);
    case CSSSelector::AttributeBegin: // css3: E[foo^="bar"]
        if (selectorValue->equals(String::emptyString)) {
            return false;
        }
        return attrValue->startsWith(selectorValue, caseSensitivity);
    case CSSSelector::AttributeEnd: // css3: E[foo$="bar"]
        if (selectorValue->equals(String::emptyString)) {
            return false;
        }
        return attrValue->endsWith(selectorValue, caseSensitivity);
    default:
        break;
    }

    STARFISH_RELEASE_ASSERT_NOT_REACHED();
    return false;
}

bool StyleResolver::anyAttributeMatches(Element* element,
                                        CSSSelector::Type type,
                                        CSSAttributeSelector* selector,
                                        MatchResult& result)
{
    const QualifiedName& selectorAttr = selector->attribute();
    STARFISH_ASSERT(!(selectorAttr.localName()->equals("*")));

    String* selectorValue = selector->value();

    Nullable<String*> refAttr = element->getAttribute(selectorAttr);
    if (!refAttr.hasValue()) {
        return false;
    }

    CSSSelector::AttributeMatchType caseSensitivity =
        selector->attributeMatch();
    if (attributeValueMatches(refAttr.getValue(), type, selectorValue,
                              caseSensitivity)) {
        return true;
    }

    if (caseSensitivity == CSSSelector::CaseInsensitive) {
        if (!selectorAttr.namespaceURI().hasValue() &&
            selectorAttr.namespaceURI().getValue().string()->equals("*")) {
            return false;
        }
    }

    // Legacy dictates that values of some attributes should be compared in
    // a case-insensitive manner regardless of whether the case insensitive
    // flag is set or not.
    bool legacyCaseInsensitive =
        !HTMLDocument::isCaseSensitiveAttribute(m_document, selectorAttr);

    // If case-insensitive, re-check, and count if result differs.
    // See http://code.google.com/p/chromium/issues/detail?id=327060
    if (legacyCaseInsensitive &&
        attributeValueMatches(refAttr.getValue(), type, selectorValue,
                              CSSSelector::CaseInsensitive)) {
        return true;
    }
    if (!selectorAttr.namespaceURI().hasValue() &&
        selectorAttr.namespaceURI().getValue().string()->equals("*")) {
        return false;
    }

    return false;
}

static void removeTransitionCSSValuePairs(CSSStyleDeclaration* target)
{
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::TransitionProperty);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::TransitionDuration);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::TransitionDelay);
    target->removeCSSValuePair(
        CSSStyleValuePair::KeyKind::TransitionTimingFunction);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::TransitionDelay);
}

static void addTransitionCSSValuePairs(CSSStyleDeclaration* target,
                                       CSSStyleValuePair property,
                                       CSSStyleValuePair duration,
                                       CSSStyleValuePair timingFunction,
                                       CSSStyleValuePair delay)
{
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::TransitionProperty,
                            property);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::TransitionDuration,
                            duration);
    target->addCSSValuePair(
        CSSStyleValuePair::KeyKind::TransitionTimingFunction, timingFunction);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::TransitionDelay, delay);
}

static void addTransitionPropertyCSSValuePairs(CSSStyleDeclaration* target,
                                               CSSStyleValuePair property)
{
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::TransitionProperty,
                            property);
}

static void addTransitionDurationCSSValuePairs(CSSStyleDeclaration* target,
                                               CSSStyleValuePair duration)
{
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::TransitionDuration,
                            duration);
}

static void addTransitionDelayCSSValuePairs(CSSStyleDeclaration* target,
                                            CSSStyleValuePair delay)
{
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::TransitionDelay, delay);
}

bool CSSStyleValuePair::updateValueUnitTransitionProperty(
    const CSSTokenValue& value)
{
    m_valueKind = CSSStyleValuePair::ValueKind::TransitionPropertyValueKind;
    if (STRING_VALUE_IS_STRING("all")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyAllValue;
    } else if (STRING_VALUE_IS_STRING("background-color")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyBackgroundColorValue;
    } else if (STRING_VALUE_IS_STRING("background-position")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyBackgroundPositionValue;
    } else if (STRING_VALUE_IS_STRING("border-bottom-color")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyBorderBottomColorValue;
    } else if (STRING_VALUE_IS_STRING("border-bottom-width")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyBorderBottomWidthValue;
    } else if (STRING_VALUE_IS_STRING("border-left-color")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyBorderLeftColorValue;
    } else if (STRING_VALUE_IS_STRING("border-left-width")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyBorderLeftWidthValue;
    } else if (STRING_VALUE_IS_STRING("border-right-color")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyBorderRightColorValue;
    } else if (STRING_VALUE_IS_STRING("border-right-width")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyBorderRightWidthValue;
    } else if (STRING_VALUE_IS_STRING("border-spacing")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyBorderSpacingValue;
    } else if (STRING_VALUE_IS_STRING("border-top-color")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyBorderTopColorValue;
    } else if (STRING_VALUE_IS_STRING("border-top-width")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyBorderTopWidthValue;
    } else if (STRING_VALUE_IS_STRING("bottom")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyBottomValue;
    } else if (STRING_VALUE_IS_STRING("clip")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyClipValue;
    } else if (STRING_VALUE_IS_STRING("color")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyColorValue;
    } else if (STRING_VALUE_IS_STRING("font-size")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyFontSizeValue;
    } else if (STRING_VALUE_IS_STRING("font-weight")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyFontWeightValue;
    } else if (STRING_VALUE_IS_STRING("height")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyHeightValue;
    } else if (STRING_VALUE_IS_STRING("left")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyLeftValue;
    } else if (STRING_VALUE_IS_STRING("letter-spacing")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyLetterSpacingValue;
    } else if (STRING_VALUE_IS_STRING("line-height")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyLineHeightValue;
    } else if (STRING_VALUE_IS_STRING("margin-bottom")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyMarginBottomValue;
    } else if (STRING_VALUE_IS_STRING("margin-left")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyMarginLeftValue;
    } else if (STRING_VALUE_IS_STRING("margin-right")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyMarginRightValue;
    } else if (STRING_VALUE_IS_STRING("margin-top")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyMarginTopValue;
    } else if (STRING_VALUE_IS_STRING("max-height")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyMaxHeightValue;
    } else if (STRING_VALUE_IS_STRING("max-width")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyMaxWidthValue;
    } else if (STRING_VALUE_IS_STRING("min-height")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyMinHeightValue;
    } else if (STRING_VALUE_IS_STRING("min-width")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyMinWidthValue;
    } else if (STRING_VALUE_IS_STRING("opacity")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyOpacityValue;
    } else if (STRING_VALUE_IS_STRING("outline-color")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyOutlineColorValue;
    } else if (STRING_VALUE_IS_STRING("outline-width")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyOutlineWidthValue;
    } else if (STRING_VALUE_IS_STRING("padding-bottom")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyPaddingBottomValue;
    } else if (STRING_VALUE_IS_STRING("padding-left")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyPaddingLeftValue;
    } else if (STRING_VALUE_IS_STRING("padding-right")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyPaddingRightValue;
    } else if (STRING_VALUE_IS_STRING("padding-top")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyPaddingTopValue;
    } else if (STRING_VALUE_IS_STRING("right")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyRightValue;
    } else if (STRING_VALUE_IS_STRING("text-indent")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyTextIndentValue;
    } else if (STRING_VALUE_IS_STRING("text-shadow")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyTextShadowValue;
    } else if (STRING_VALUE_IS_STRING("transform")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyTransformValue;
    } else if (STRING_VALUE_IS_STRING("transform-origin")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyTransformOriginValue;
    } else if (STRING_VALUE_IS_STRING("top")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyTopValue;
    } else if (STRING_VALUE_IS_STRING("vertical-align")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyVerticalAlignValue;
    } else if (STRING_VALUE_IS_STRING("visibility")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyVisibilityValue;
    } else if (STRING_VALUE_IS_STRING("width")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyWidthValue;
    } else if (STRING_VALUE_IS_STRING("word-spacing")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyWordSpacingValue;
    } else if (STRING_VALUE_IS_STRING("z-index")) {
        m_value.m_transitionProperty =
            TransitionPropertyValue::TransitionPropertyZIndexValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueUnitTransitionTimingFunction(
    const CSSTokenValue& value)
{
    m_valueKind =
        CSSStyleValuePair::ValueKind::TransitionTimingFunctionValueKind;
    if (STRING_VALUE_IS_STRING("ease")) {
        m_value.m_transitionTimingFunction =
            TransitionTimingFunctionValue::TransitionTimingFunctionEaseValue;
    } else if (STRING_VALUE_IS_STRING("linear")) {
        m_value.m_transitionTimingFunction =
            TransitionTimingFunctionValue::TransitionTimingFunctionLinearValue;
    } else if (STRING_VALUE_IS_STRING("ease-in")) {
        m_value.m_transitionTimingFunction =
            TransitionTimingFunctionValue::TransitionTimingFunctionEaseInValue;
    } else if (STRING_VALUE_IS_STRING("ease-out")) {
        m_value.m_transitionTimingFunction =
            TransitionTimingFunctionValue::TransitionTimingFunctionEaseOutValue;
    } else if (STRING_VALUE_IS_STRING("ease-in-out")) {
        m_value.m_transitionTimingFunction = TransitionTimingFunctionValue::
            TransitionTimingFunctionEaseInOutValue;
    } else if (STRING_VALUE_IS_STRING("step-start")) {
        m_value.m_transitionTimingFunction = TransitionTimingFunctionValue::
            TransitionTimingFunctionStepStartValue;
    } else if (STRING_VALUE_IS_STRING("step-end")) {
        m_value.m_transitionTimingFunction =
            TransitionTimingFunctionValue::TransitionTimingFunctionStepEndValue;
    } else {
        return false;
    }
    return true;
}

static bool parseTransitionShorthand(const CSSTokenVector& tokens,
                                     CSSStyleValuePair* property,
                                     CSSStyleValuePair* duration,
                                     CSSStyleValuePair* timingFunction,
                                     CSSStyleValuePair* delay)
{
    // TODO other shorthands
    size_t len = tokens.size();
    if (len < 1) {
        return false;
    }

    property->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    duration->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    timingFunction->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    delay->setValueKind(CSSStyleValuePair::ValueKind::Initial);

    bool isFirstTimeValue = true;
    CSSStyleValuePair temp;

    for (size_t i = 0; i < len; i++) {
        const CSSTokenValue& tok = tokens[i];
        if (temp.updateValueUnitTransitionProperty(tok)) {
            *property = temp;
        } else if (isFirstTimeValue && temp.updateValueUnitTimeOrCalc(tok, 0)) {
            *duration = temp;
            isFirstTimeValue = false;
        } else if (temp.updateValueUnitTransitionTimingFunction(tok)) {
            *timingFunction = temp;
        } else if (!isFirstTimeValue &&
                   temp.updateValueUnitTimeOrCalc(tok, 0)) {
            *delay = temp;
        } else {
            return false;
        }
    }
    return true;
}

void CSSStyleDeclaration::setTransition(const char* value, size_t length,
                                        bool isImportant)
{
    if (length == 0) {
        removeTransitionCSSValuePairs(this);
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, length);

    // TODO comma separation
    CSSStyleValuePair v, property, duration, timingFunction, delay;
    if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        addTransitionCSSValuePairs(this, v, v, v, v);
    } else if (parseTransitionShorthand(tokens, &property, &duration,
                                        &timingFunction, &delay)) {
        property.setFlagImportant(isImportant);
        duration.setFlagImportant(isImportant);
        timingFunction.setFlagImportant(isImportant);
        delay.setFlagImportant(isImportant);
        addTransitionCSSValuePairs(this, property, duration, timingFunction,
                                   delay);
    }
}

void CSSStyleDeclaration::setTransitionTransitionProperty(const char* value,
                                                          size_t length,
                                                          bool isImportant)
{
    if (length == 0) {
        removeTransitionCSSValuePairs(this);
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, length);

    CSSStyleValuePair v, property;
    if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        addTransitionPropertyCSSValuePairs(this, v);
    }
}

void CSSStyleDeclaration::setTransitionTransitionDuration(const char* value,
                                                          size_t length,
                                                          bool isImportant)
{
    if (length == 0) {
        removeTransitionCSSValuePairs(this);
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, length);

    CSSStyleValuePair v, duration;
    if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        addTransitionDurationCSSValuePairs(this, v);
    }
}

void CSSStyleDeclaration::setTransitionTransitionDelay(const char* value,
                                                       size_t length,
                                                       bool isImportant)
{
    if (length == 0) {
        removeTransitionCSSValuePairs(this);
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, length);

    CSSStyleValuePair v;
    if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        addTransitionDelayCSSValuePairs(this, v);
    }
}

static String* createTransitionString(String* property, String* duration,
                                      String* delay)
{
    StringBuilder builder;

    if (!property->equals(String::emptyString) &&
        !property->equals(String::initialString)) {
        builder.appendString(property);
    }

    if (!duration->equals(String::emptyString) &&
        !duration->equals(String::initialString)) {
        if (builder.contentLength() > 0) {
            builder.appendString(String::spaceString);
        }
        builder.appendString(duration);
    }

    if (!delay->equals(String::emptyString) &&
        !delay->equals(String::initialString)) {
        if (builder.contentLength() > 0) {
            builder.appendString(String::spaceString);
        }
        builder.appendString(delay);
    }

    return builder.finalize();
}

String* CSSStyleDeclaration::Transition()
{
    String* property = TransitionProperty();
    String* duration = TransitionDuration();
    String* delay = TransitionDelay();
    return createTransitionString(property, duration, delay);
}

String* CSSStyleDeclaration::TransitionTransitionProperty()
{
    String* property = TransitionProperty();
    return createTransitionString(property, nullptr, nullptr);
}

String* CSSStyleDeclaration::TransitionTransitionDuration()
{
    String* duration = TransitionDuration();
    return createTransitionString(nullptr, duration, nullptr);
}

String* CSSStyleDeclaration::TransitionTransitionDelay()
{
    String* delay = TransitionDelay();
    return createTransitionString(nullptr, nullptr, delay);
}

void CSSStyleDeclaration::setBackground(const char* value, size_t length,
                                        bool isImportant)
{
    if (length == 0) {
        removeBackgroundCSSValuePairs(this);
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, length, ",/", 2);
    if (tokens.size() == 0) {
        return;
    }

    // TODO: should check comma-separated input
    CSSStyleValuePair v, color, image, repeatX, repeatY, positionX, positionY,
        size, attachment, origin, clip;
    if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        addBackgroundCSSValuePairs(this, v, v, v, v, v, v, v, v, v, v);
    } else {
#define APPEND_NEW_LAYER(PROP, NEWPROP)                                     \
    if (PROP.valueKind() != CSSStyleValuePair::ValueKind::ValueListKind) {  \
        CSSStyleValuePair tmp = PROP;                                       \
        PROP.setValueList(                                                  \
            new ValueList(ValueList::Separator::CommaSeparator));           \
        PROP.multiValue()->push_back(tmp);                                  \
    }                                                                       \
    if (NEWPROP.valueKind() == CSSStyleValuePair::ValueKind::ValueListKind) \
        PROP.multiValue()->push_back((*NEWPROP.multiValue())[0]);           \
    else {                                                                  \
        PROP.multiValue()->push_back(NEWPROP);                              \
    }

        CSSTokenVector layer;
        unsigned int cntLayer = 0;
        for (unsigned int t = 0; t < tokens.size(); t++) {
            const CSSTokenValue& val = tokens[t];
            if (val.equals(",")) {
                if (t == 0 || t == tokens.size() - 1)
                    return;
            } else if (t == tokens.size() - 1) {
                layer.push_back(tokens[t]);
            } else {
                layer.push_back(tokens[t]);
                continue;
            }

            if (layer.size() == 0) {
                return;
            }
            CSSStyleValuePair layerImage, layerRepeatX, layerRepeatY,
                layerPositionX, layerPositionY, layerSize, layerAttachment,
                layerOrigin, layerClip;
            // NOTE: Color is allowed only for the last background layer (t ==
            // tokens->size() - 1)
            if (!parseBackgroundShorthand(
                    layer, &color, &layerImage, &layerRepeatX, &layerRepeatY,
                    &layerPositionX, &layerPositionY, &layerSize,
                    &layerAttachment, &layerOrigin, &layerClip,
                    t == tokens.size() - 1)) {
                return;
            }
            if (cntLayer == 0) {
                image = layerImage;
                repeatX = layerRepeatX;
                repeatY = layerRepeatY;
                positionX = layerPositionX;
                positionY = layerPositionY;
                size = layerSize;
                attachment = layerAttachment;
                origin = layerOrigin;
                clip = layerClip;
            } else {
                APPEND_NEW_LAYER(image, layerImage);
                APPEND_NEW_LAYER(repeatX, layerRepeatX);
                APPEND_NEW_LAYER(repeatY, layerRepeatY);
                APPEND_NEW_LAYER(positionX, layerPositionX);
                APPEND_NEW_LAYER(positionY, layerPositionY);
                APPEND_NEW_LAYER(size, layerSize);
                APPEND_NEW_LAYER(attachment, layerAttachment);
                APPEND_NEW_LAYER(origin, layerOrigin);
                APPEND_NEW_LAYER(clip, layerClip);
            }
            cntLayer++;
            layer.clear();
        }
        color.setFlagImportant(isImportant);
        image.setFlagImportant(isImportant);
        repeatX.setFlagImportant(isImportant);
        repeatY.setFlagImportant(isImportant);
        positionX.setFlagImportant(isImportant);
        positionY.setFlagImportant(isImportant);
        size.setFlagImportant(isImportant);
        attachment.setFlagImportant(isImportant);
        origin.setFlagImportant(isImportant);
        clip.setFlagImportant(isImportant);
        addBackgroundCSSValuePairs(this, color, image, repeatX, repeatY,
                                   positionX, positionY, size, attachment,
                                   origin, clip);
    }
#undef APPEND_NEW_LAYER
}

void CSSStyleDeclaration::setBorderImage(const char* value, size_t length,
                                         bool isImportant)
{
    if (length == 0) {
        removeBorderImageCSSValuePairs(this);
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, length, "/", 1);

    CSSStyleValuePair v, source, slice, width, outset, repeat;
    if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        addBorderImageCSSValuePairs(this, v, v, v, v, v);
    } else if (parseBorderImageShorthand(tokens, &source, &slice, &width,
                                         &outset, &repeat)) {
        source.setFlagImportant(isImportant);
        slice.setFlagImportant(isImportant);
        width.setFlagImportant(isImportant);
        outset.setFlagImportant(isImportant);
        repeat.setFlagImportant(isImportant);
        addBorderImageCSSValuePairs(this, source, slice, width, outset, repeat);
    }
}

String* CSSStyleDeclaration::FontFamily()
{
    for (unsigned i = 0; i < m_cssValues.size(); i++) {
        if (m_cssValues[i].keyKind() == CSSStyleValuePair::KeyKind::FontFamily)
            return m_cssValues[i].toString();
    }
    return String::emptyString;
}

String* CSSStyleDeclaration::Src()
{
    for (unsigned i = 0; i < m_cssValues.size(); i++) {
        if (m_cssValues[i].keyKind() == CSSStyleValuePair::KeyKind::Src)
            return m_cssValues[i].toString();
    }
    return String::emptyString;
}

String* CSSStyleDeclaration::Font()
{
    String* style = FontStyle();
    String* weight = FontWeight();
    String* size = FontSize();
    String* lineHeight = LineHeight();

    if (style->length() == 0 || weight->length() == 0 || size->length() == 0 ||
        lineHeight->length() == 0) {
        return String::emptyString;
    }
    int maxCount = 4;
    int initialCount = 0;
    int inheritCount = 0;
    initialCount += (style->equals(String::initialString) ? 1 : 0);
    initialCount += (weight->equals(String::initialString) ? 1 : 0);
    initialCount += (size->equals(String::initialString) ? 1 : 0);
    initialCount += (lineHeight->equals(String::inheritString) ? 1 : 0);
    inheritCount += (style->equals(String::inheritString) ? 1 : 0);
    inheritCount += (weight->equals(String::inheritString) ? 1 : 0);
    inheritCount += (size->equals(String::inheritString) ? 1 : 0);
    inheritCount += (lineHeight->equals(String::inheritString) ? 1 : 0);

    if (initialCount == maxCount) {
        return String::initialString;
    }
    if (inheritCount == maxCount) {
        return String::inheritString;
    }
    if (initialCount > 0 || inheritCount > 0) {
        return String::emptyString;
    }

    StringBuilder builder;
    // 1. style
    if (!style->equals("normal")) {
        builder.appendString(style);
    }
    // 2. weight
    if (!weight->equals("normal")) {
        if (builder.contentLength() > 0) {
            builder.appendString(String::spaceString);
        }
        builder.appendString(weight);
    }
    // 3. size
    if (builder.contentLength() > 0) {
        builder.appendString(String::spaceString);
    }
    builder.appendString(size);
    // 4. lineHeight
    if (!lineHeight->equals("normal")) {
        builder.appendString("/");
        builder.appendString(lineHeight);
    }
    return builder.finalize();
}

void CSSStyleDeclaration::setFontFamily(const char* value, size_t len,
                                        bool isImportant)
{
    if (len == 0) {
        removeCSSValuePair(CSSStyleValuePair::KeyKind::FontFamily);
        return;
    }
    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, len, ",", 1, true);
    CSSStyleValuePair ret;
    if (ret.updateValueCommon(tokens) || ret.updateValueFontFamily(tokens)) {
        ret.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::FontFamily, ret);
    }
}

void CSSStyleDeclaration::setSrc(const char* value, size_t len,
                                 bool isImportant)
{
    if (len == 0) {
        removeCSSValuePair(CSSStyleValuePair::KeyKind::Src);
        return;
    }
    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, len, ",", 1, true);
    CSSStyleValuePair ret;
    if (ret.updateValueSrc(tokens)) {
        ret.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::Src, ret);
    }
}

void CSSStyleDeclaration::setFont(const char* value, size_t length,
                                  bool isImportant)
{
    if (length == 0) {
        removeCSSValuePair(CSSStyleValuePair::KeyKind::FontFamily);
        removeCSSValuePair(CSSStyleValuePair::KeyKind::FontStyle);
        removeCSSValuePair(CSSStyleValuePair::KeyKind::FontWeight);
        removeCSSValuePair(CSSStyleValuePair::KeyKind::FontSize);
        removeCSSValuePair(CSSStyleValuePair::KeyKind::LineHeight);
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, length, "/,", 2);
    if (tokens.size() == 0) {
        return;
    }

    CSSStyleValuePair v, style /*, variant*/, weight /*, stretch*/, size,
        lineHeight, fontFamily;
    if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::FontFamily, v);
        addCSSValuePair(CSSStyleValuePair::KeyKind::FontStyle, v);
        addCSSValuePair(CSSStyleValuePair::KeyKind::FontWeight, v);
        addCSSValuePair(CSSStyleValuePair::KeyKind::FontSize, v);
        addCSSValuePair(CSSStyleValuePair::KeyKind::LineHeight, v);
        return;
    }

    tokens.clear();
    tokenizeCSSValue(tokens, value, length, "/,", 2, true);
    if (parseFontShorthand(tokens, &style, &weight, &size, &lineHeight,
                           &fontFamily)) {
        fontFamily.setFlagImportant(isImportant);
        style.setFlagImportant(isImportant);
        weight.setFlagImportant(isImportant);
        size.setFlagImportant(isImportant);
        lineHeight.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::FontFamily, fontFamily);
        addCSSValuePair(CSSStyleValuePair::KeyKind::FontStyle, style);
        addCSSValuePair(CSSStyleValuePair::KeyKind::FontWeight, weight);
        addCSSValuePair(CSSStyleValuePair::KeyKind::FontSize, size);
        addCSSValuePair(CSSStyleValuePair::KeyKind::LineHeight, lineHeight);
    }
}

#define ADD_PAIRS(PRE, ...)                                                  \
    addCSSValuePair(CSSStyleValuePair::KeyKind::PRE##Top##__VA_ARGS__, top); \
    addCSSValuePair(CSSStyleValuePair::KeyKind::PRE##Right##__VA_ARGS__,     \
                    right);                                                  \
    addCSSValuePair(CSSStyleValuePair::KeyKind::PRE##Bottom##__VA_ARGS__,    \
                    bottom);                                                 \
    addCSSValuePair(CSSStyleValuePair::KeyKind::PRE##Left##__VA_ARGS__, left);
#define RM_PAIRS(PRE, ...)                                                    \
    removeCSSValuePair(CSSStyleValuePair::KeyKind::PRE##Top##__VA_ARGS__);    \
    removeCSSValuePair(CSSStyleValuePair::KeyKind::PRE##Right##__VA_ARGS__);  \
    removeCSSValuePair(CSSStyleValuePair::KeyKind::PRE##Bottom##__VA_ARGS__); \
    removeCSSValuePair(CSSStyleValuePair::KeyKind::PRE##Left##__VA_ARGS__);
#define ATTRIBUTE_SETTER_FOURSIDE(PRE, ...)                        \
    void CSSStyleDeclaration::set##PRE##__VA_ARGS__(               \
        const char* value, size_t length, bool isImportant)        \
    {                                                              \
        if (length == 0) {                                         \
            RM_PAIRS(PRE, __VA_ARGS__);                            \
            return;                                                \
        }                                                          \
        CSSTokenVector tokens;                                     \
        tokenizeCSSValue(tokens, value, length);                   \
                                                                   \
        CSSStyleValuePair c, top, right, bottom, left;             \
        if (c.updateValueCommon(tokens)) {                         \
            c.setFlagImportant(isImportant);                       \
            top = right = bottom = left = c;                       \
            ADD_PAIRS(PRE, __VA_ARGS__);                           \
            return;                                                \
        }                                                          \
        size_t len = tokens.size();                                \
        if (len < 1 || len > 4) {                                  \
            return;                                                \
        }                                                          \
                                                                   \
        GCVector<CSSStyleValuePair> result;                        \
        for (size_t i = 0; i < len; i++) {                         \
            CSSStyleValuePair v;                                   \
            v.setFlagImportant(isImportant);                       \
            if (!v.updateValueUnit##PRE##__VA_ARGS__(tokens[i])) { \
                return;                                            \
            }                                                      \
            result.push_back(v);                                   \
        }                                                          \
        top = result[0];                                           \
        right = len < 2 ? top : result[1];                         \
        bottom = len < 3 ? top : result[2];                        \
        left = len < 4 ? right : result[3];                        \
        top.setFlagImportant(isImportant);                         \
        right.setFlagImportant(isImportant);                       \
        bottom.setFlagImportant(isImportant);                      \
        left.setFlagImportant(isImportant);                        \
        ADD_PAIRS(PRE, __VA_ARGS__);                               \
    }
ATTRIBUTE_SETTER_FOURSIDE(Margin);
ATTRIBUTE_SETTER_FOURSIDE(Padding);
ATTRIBUTE_SETTER_FOURSIDE(Border, Width);
ATTRIBUTE_SETTER_FOURSIDE(Border, Style);
ATTRIBUTE_SETTER_FOURSIDE(Border, Color);
#undef ADD_PAIRS
#undef RM_PAIRS

static bool seperatorContains(const char* seperator, size_t seperatorCount,
                              char ch)
{
    for (size_t i = 0; i < seperatorCount; i++) {
        if (seperator[i] == ch) {
            return true;
        }
    }
    return false;
}

void CSSStyleDeclaration::tokenizeCSSValue(CSSTokenVector& tokens,
                                           const char* data, size_t length,
                                           const char* seperator,
                                           size_t seperatorCount,
                                           bool isCaseSensitive,
                                           bool preserveContentWS)
{
    CSSTokenValue str;
    bool inParenthesis = false;
    size_t numberOfnesting = 0;
    bool inQuotes = false;
    bool isWhiteSpaceState = false;
    for (size_t i = 0; i < length; i++) {
        if (data[i] == '(') {
            inParenthesis = true;
            numberOfnesting++;
        } else if (data[i] == ')') {
            if (numberOfnesting) {
                numberOfnesting--;
            }
        } else if (data[i] == '"' || data[i] == '\'') {
            inQuotes = !inQuotes;
        }

        if (!preserveContentWS && isWhiteSpaceState &&
            String::isSpaceOrNewline(data[i])) {
            continue;
        }

        isWhiteSpaceState = false;
        str += data[i];
        if (!preserveContentWS && (inParenthesis || inQuotes) &&
            String::isSpaceOrNewline(data[i])) {
            str[str.length() - 1] = ' ';
            isWhiteSpaceState = true;
            continue;
        }
        bool hasSepChar = false;
        if (seperatorCount > 0 &&
            seperatorContains(seperator, seperatorCount, data[i])) {
            hasSepChar = true;
        }

        if ((!inParenthesis && !inQuotes &&
             (String::isSpaceOrNewline(data[i]) || hasSepChar)) ||
            (data[i] == '(' && hasSepChar) || (data[i] == ')' && hasSepChar)) {
            str.pop_back();
            bool onlyWhiteSpace = true;
            for (size_t i = 0; i < str.length(); i++) {
                if (!isCaseSensitive)
                    str[i] = ::tolower(str[i]);
                if (!String::isASCIISpace(str[i])) {
                    onlyWhiteSpace = false;
                }
            }

            if (!onlyWhiteSpace && !numberOfnesting) {
                tokens.push_back(CSSTokenValue(std::move(str)));
            }
            isWhiteSpaceState = true;
            if (hasSepChar && !numberOfnesting) {
                tokens.push_back(CSSTokenValue(std::string(data + i, 1)));
            }
        } else if ((inParenthesis && data[i] == ')') || i == length - 1) {
            if (str.length() > 3 && (str[0] == 'u' || str[0] == 'U') &&
                (str[1] == 'r' || str[1] == 'R') &&
                (str[2] == 'l' || str[2] == 'L')) {
                std::transform(str.begin(), str.begin() + 3, str.begin(),
                               ::tolower);
                tokens.push_back(std::move(str));
            } else if (str.length() != 0) {
                if (!isCaseSensitive && !inQuotes) {
                    std::transform(str.begin(), str.end(), str.begin(),
                                   ::tolower);
                }
                if (!numberOfnesting) {
                    tokens.push_back(std::move(str));
                }
            }
            inParenthesis = false;
            isWhiteSpaceState = true;
        }
    }
}

void CSSStyleDeclaration::addCSSValuePair(CSSStyleValuePair::KeyKind name,
                                          CSSStyleValuePair ret)
{
    for (unsigned i = 0; i < m_cssValues.size(); i++) {
        if (m_cssValues[i].keyKind() == name) {
            if (isInlineStyle() || ret.flagImportant() == true ||
                (ret.flagImportant() == false &&
                 m_cssValues[i].flagImportant() == false)) {
                m_cssValues[i].setValueKind(ret.valueKind());
                m_cssValues[i].setValue(ret.value());
                m_cssValues[i].setFlagImportant(ret.flagImportant());
                m_cssValues[i].setTemporaryKeyKind(ret.temporaryKeyKind());
                rootPointerValueIfExists(ret);
                notifyNeedsStyleRecalc();
            }

            return;
        }
    }
    ret.setKeyKind(name);
    m_cssValues.push_back(ret);
    rootPointerValueIfExists(ret);
    notifyNeedsStyleRecalc();
}

void CSSStyleDeclaration::removeCSSValuePair(CSSStyleValuePair::KeyKind name)
{
    unsigned len = m_cssValues.size();
    for (unsigned i = 0; i < len; i++) {
        if (m_cssValues[i].keyKind() == name) {
            m_cssValues.erase(m_cssValues.begin() + i);
            notifyNeedsStyleRecalc();
            return;
        }
    }
}

bool CSSStyleDeclaration::hasCSSValuePair(CSSStyleValuePair::KeyKind name)
{
    unsigned len = m_cssValues.size();
    for (unsigned i = 0; i < len; i++) {
        if (m_cssValues[i].keyKind() == name) {
            return true;
        }
    }
    return false;
}

CSSStyleValuePair CSSStyleDeclaration::getCSSValuePair(
    CSSStyleValuePair::KeyKind name)
{
    unsigned len = m_cssValues.size();
    for (unsigned i = 0; i < len; i++) {
        if (m_cssValues[i].keyKind() == name) {
            return m_cssValues[i];
        }
    }

    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

void CSSStyleDeclaration::notifyNeedsStyleRecalc()
{
    if (m_node->isElement()) {
        m_node->asElement()->notifyInlineStyleChanged();
    }
}

StyleResolver::StyleResolver(Document* document)
    : DocumentHoldable(document)
    , m_mediumFontSize(starFish()->defaultFontSizeMultiplier() *
                       DEFAULT_FONT_SIZE)
    , m_usesFirstLineRule(false)
    , m_mediaQueryEvaluator(nullptr)
    , m_ruleSet(new RuleSet())
{
}

ComputedStyle* StyleResolver::resolveDocumentStyle(Document* doc)
{
    ComputedStyle* ret = new ComputedStyle(m_mediumFontSize);
    ret->m_display = DisplayValue::BlockDisplayValue;
#ifdef STARFISH_TIZEN_WEARABLE
    ret->m_inheritedStyles.m_color = Unit::Color(255, 255, 255, 255);
#else
    ret->m_inheritedStyles.m_color = Unit::Color(0, 0, 0, 255);
#endif
    ret->m_inheritedStyles.m_fontFamilyDatas =
        doc->starFish()->initialFontFamilyDatas();
    ret->m_inheritedStyles.m_textAlign = TextAlignValue::StartTextAlignValue;
    ret->m_inheritedStyles.m_direction = DirectionValue::LtrDirectionValue;
    ret->m_inheritedStyles.m_whiteSpace =
        WhiteSpaceValue::NormalWhiteSpaceValue;
    ret->loadResources(doc);
    return ret;
}

StyleResolveContext::StyleResolveContext(Document* document)
    : m_document(document)
    , m_ancestorSelectorFilter(new AncestorSelectorFilter())
{
}

StyleResolveContext::~StyleResolveContext()
{
}

void StyleResolveContext::pushIntoComputedStylePool(ComputedStyle* b)
{
    STARFISH_ASSERT(b);
    if (!b->usedInAnimator()) {
        memset(b, 0, sizeof(ComputedStyle));
        m_computedStylePool.push_back(b);
#ifndef NDEBUG
        STARFISH_ASSERT(m_dbg.find(b) == m_dbg.end());
        m_dbg.insert(b);
#endif
    }
}

void* StyleResolveContext::allocateComputedStyle()
{
    if (hasItemInComputedStylePool()) {
        return takeFromComputedStylePool();
    } else {
        return ComputedStyle::operator new(sizeof(ComputedStyle));
    }
}

ComputedStyle* StyleResolver::resolveStyle(StyleResolveContext& ctx,
                                           Element* element,
                                           ComputedStyle* parent)
{
    ComputedStyle* style =
        new (ctx.allocateComputedStyle()) ComputedStyle(parent);

    matchAllRules(ctx, element, style, parent);
    style->loadResources(element, element->style());
    style->arrangeStyleValues(parent, element);
    return style;
}

void StyleResolver::apply(Element* element,
                          GCAtomicVector<CSSStyleValuePair>& cssValues,
                          GCVector<MutablePropertyValue>& cssCustomValues,
                          ResourceURL* origin, ComputedStyle* style,
                          ComputedStyle* parentStyle, bool isImportant)
{
    for (unsigned k = 0; k < cssValues.size(); k++) {
        if (isImportant != cssValues[k].flagImportant()) {
            continue;
        }

#ifdef STARFISH_ENABLE_CSS_VARIABLE
        if (cssValues[k].keyKind() == CSSStyleValuePair::KeyKind::VarValue) {
            // TODO: Define the new value againe.
            String* keyword = cssValues[k].varFunctionValue();
            size_t len = keyword->length();
            CSSTokenVector tokens;
            if (UNLIKELY(cssValues[k].temporaryKeyKind() ==
                         CSSStyleValuePair::KeyKind::Content)) {
                CSSStyleDeclaration::tokenizeCSSValue(
                    tokens, keyword->toUTF8NonGCString().data(), len, "", 0,
                    true);
            } else {
                CSSStyleDeclaration::tokenizeCSSValue(
                    tokens, keyword->toUTF8NonGCString().data(), len, ",", 1);
            }

            for (size_t i = 0; i < tokens.size(); i++) {
                CSSVariableSyntaxTreeBuilder variablesSyntaxBuilder;
                CSSTokenValue token(tokens[i]);
                variablesSyntaxBuilder.build(token);
                if (variablesSyntaxBuilder.isValid()) {
                    // Replace a old style with a new style coverted with the
                    // syntax builder.
                    tokens[i] = CSSTokenValue(
                        variablesSyntaxBuilder.generateStyle(cssCustomValues));
                }
            }

#define SET_CASES(name, ...)                                                  \
    case CSSStyleValuePair::KeyKind::name:                                    \
        if (CSSStyleValuePair::KeyKind::name ==                               \
            CSSStyleValuePair::KeyKind::VarValue) {                           \
            break;                                                            \
        }                                                                     \
                                                                              \
        if (ret.updateValueCommon(tokens) || ret.updateValue##name(tokens)) { \
            ret.setKeyKind(CSSStyleValuePair::KeyKind::name);                 \
        }                                                                     \
        break;

            CSSStyleValuePair ret;
            switch (cssValues[k].temporaryKeyKind()) {
                FOR_EACH_STYLE_ATTRIBUTE_BASIC(SET_CASES)
            case CSSStyleValuePair::KeyKind::Empty:
                break;
            default:
                break;
            }

            cssValues[k].setKeyKind(ret.keyKind());
            cssValues[k].setValueKind(ret.valueKind());
            cssValues[k].setValue(ret.value());
        }
#endif

        switch (cssValues[k].keyKind()) {
        case CSSStyleValuePair::KeyKind::Display:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Inherit) {
                style->m_display = parentStyle->m_display;
            } else if ((cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) ||
                       (cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Unset)) {
                style->m_display = DisplayValue::InlineDisplayValue;
            } else {
                STARFISH_ASSERT(
                    CSSStyleValuePair::ValueKind::DisplayValueKind ==
                    cssValues[k].valueKind());
                style->m_display = cssValues[k].displayValue();
            }
            break;
        case CSSStyleValuePair::KeyKind::Position:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Inherit) {
                style->m_position = parentStyle->m_position;
            } else if ((cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) ||
                       (cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Unset)) {
                style->m_position = PositionValue::StaticPositionValue;
            } else {
                STARFISH_ASSERT(
                    CSSStyleValuePair::ValueKind::PositionValueKind ==
                    cssValues[k].valueKind());
                style->m_position = cssValues[k].positionValue();
            }
            break;
        case CSSStyleValuePair::KeyKind::Float:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Inherit) {
                style->m_float = parentStyle->m_float;
            } else if ((cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) ||
                       (cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Unset)) {
                style->m_float = FloatValue::NoneFloatValue;
            } else {
                STARFISH_ASSERT(CSSStyleValuePair::ValueKind::FloatValueKind ==
                                cssValues[k].valueKind());
                style->m_float = cssValues[k].floatValue();
            }
            break;
        case CSSStyleValuePair::KeyKind::Clear:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Inherit) {
                style->m_clear = parentStyle->m_clear;
            } else if ((cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) ||
                       (cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Unset)) {
                style->m_clear = ClearValue::NoneClearValue;
            } else {
                STARFISH_ASSERT(CSSStyleValuePair::ValueKind::ClearValueKind ==
                                cssValues[k].valueKind());
                style->m_clear = cssValues[k].clearValue();
            }
            break;
        case CSSStyleValuePair::KeyKind::Width:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Inherit) {
                style->m_width = parentStyle->m_width;
            } else if ((cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) ||
                       (cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Unset)) {
                style->m_width = Length();
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::
                           WidthHeightKeywordValueKind) {
                STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            } else {
                Nullable<Length> length = convertValueToLength(
                    cssValues[k].valueKind(), cssValues[k].value());
                if (length.hasValue()) {
                    style->m_width = length.getValue();
                } else {
                    style->m_width = Length();
                }
            }
            break;
        case CSSStyleValuePair::KeyKind::MaxWidth:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Inherit) {
                style->setMaxWidth(parentStyle->maxWidth());
            } else if ((cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) ||
                       (cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Unset)) {
                style->setMaxWidth(Length());
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::None) {
                style->setMaxWidth(Length());
            } else {
                Nullable<Length> length = convertValueToLength(
                    cssValues[k].valueKind(), cssValues[k].value());
                if (length.hasValue()) {
                    style->setMaxWidth(length.getValue());
                } else {
                    style->setMaxWidth(Length());
                }
            }
            break;
        case CSSStyleValuePair::KeyKind::MinWidth:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Inherit) {
                style->setMinWidth(parentStyle->minWidth());
            } else if ((cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) ||
                       (cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Unset)) {
                style->setMinWidth(Length());
            } else {
                Nullable<Length> length = convertValueToLength(
                    cssValues[k].valueKind(), cssValues[k].value());
                if (length.hasValue()) {
                    style->setMinWidth(length.getValue());
                } else {
                    style->setMinWidth(Length());
                }
            }
            break;
        case CSSStyleValuePair::KeyKind::Height:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Inherit) {
                style->m_height = parentStyle->m_height;
            } else if ((cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) ||
                       (cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Unset)) {
                style->m_height = Length();
            } else {
                Nullable<Length> length = convertValueToLength(
                    cssValues[k].valueKind(), cssValues[k].value());
                if (length.hasValue()) {
                    style->m_height = length.getValue();
                } else {
                    style->m_height = Length();
                }
            }
            break;
        case CSSStyleValuePair::KeyKind::MaxHeight:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Inherit) {
                style->setMaxHeight(parentStyle->maxHeight());
            } else if ((cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) ||
                       (cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Unset)) {
                style->setMaxHeight(Length());
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::None) {
                style->setMaxHeight(Length());
            } else {
                Nullable<Length> length = convertValueToLength(
                    cssValues[k].valueKind(), cssValues[k].value());
                if (length.hasValue()) {
                    style->setMaxHeight(length.getValue());
                } else {
                    style->setMaxHeight(Length());
                }
            }
            break;
        case CSSStyleValuePair::KeyKind::MinHeight:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Inherit) {
                style->setMinHeight(parentStyle->minHeight());
            } else if ((cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) ||
                       (cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Unset)) {
                style->setMinHeight(Length());
            } else {
                Nullable<Length> length = convertValueToLength(
                    cssValues[k].valueKind(), cssValues[k].value());
                if (length.hasValue()) {
                    style->setMinHeight(length.getValue());
                } else {
                    style->setMinHeight(Length());
                }
            }
            break;
        case CSSStyleValuePair::KeyKind::Color:
            style->m_gotInheritedColor = false;
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Inherit) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->setColor(parentStyle->m_inheritedStyles.m_color);
                style->m_gotInheritedColor = true;
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Initial) {
                style->setColor(Unit::Color(0, 0, 0, 255));
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::ColorValueKind) {
                style->setColor(cssValues[k].colorValue());
            } else {
                STARFISH_ASSERT(
                    cssValues[k].valueKind() ==
                    CSSStyleValuePair::ValueKind::NamedColorValueKind);
                if (cssValues[k].namedColorValue() ==
                    NamedColor::NamedColorValue::currentColor) {
                    style->m_inheritedStyles.m_color =
                        parentStyle->m_inheritedStyles.m_color;
                } else {
                    style->setColor(NamedColor::namedColorToColor(
                        cssValues[k].namedColorValue()));
                }
            }
            break;
        case CSSStyleValuePair::KeyKind::FontSize:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Inherit) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->setFontSize(parentStyle->fontSize());
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Initial) {
                style->setFontSize(
                    parseAbsoluteFontSize(3, this->m_mediumFontSize));
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::FontSizeValueKind) {
                if (cssValues[k].fontSizeValue() ==
                    FontSizeValue::XXSmallFontSizeValue) {
                    style->setFontSize(
                        parseAbsoluteFontSize(0, this->m_mediumFontSize));
                } else if (cssValues[k].fontSizeValue() ==
                           FontSizeValue::XSmallFontSizeValue) {
                    style->setFontSize(
                        parseAbsoluteFontSize(1, this->m_mediumFontSize));
                } else if (cssValues[k].fontSizeValue() ==
                           FontSizeValue::SmallFontSizeValue) {
                    style->setFontSize(
                        parseAbsoluteFontSize(2, this->m_mediumFontSize));
                } else if (cssValues[k].fontSizeValue() ==
                           FontSizeValue::MediumFontSizeValue) {
                    style->setFontSize(
                        parseAbsoluteFontSize(3, this->m_mediumFontSize));
                } else if (cssValues[k].fontSizeValue() ==
                           FontSizeValue::LargeFontSizeValue) {
                    style->setFontSize(
                        parseAbsoluteFontSize(4, this->m_mediumFontSize));
                } else if (cssValues[k].fontSizeValue() ==
                           FontSizeValue::XLargeFontSizeValue) {
                    style->setFontSize(
                        parseAbsoluteFontSize(5, this->m_mediumFontSize));
                } else if (cssValues[k].fontSizeValue() ==
                           FontSizeValue::XXLargeFontSizeValue) {
                    style->setFontSize(
                        parseAbsoluteFontSize(6, this->m_mediumFontSize));
                } else if (cssValues[k].fontSizeValue() ==
                           FontSizeValue::XXXLargeFontSizeValue) {
                    style->setFontSize(
                        Length(Length::Fixed,
                               parseAbsoluteFontSize(6, this->m_mediumFontSize)
                                       .fixed() *
                                   1.5f));
                } else if (cssValues[k].fontSizeValue() ==
                           FontSizeValue::LargerFontSizeValue) {
                    style->setFontSize(parentStyle->fontSize() * 1.2f);
                } else if (cssValues[k].fontSizeValue() ==
                           FontSizeValue::SmallerFontSizeValue) {
                    style->setFontSize(parentStyle->fontSize() / 1.2f);
                }
            } else {
                Nullable<Length> length = convertValueToLength(
                    cssValues[k].valueKind(), cssValues[k].value());
                if (length.hasValue()) {
                    Length l = length.getValue();
                    if (l.isViewportPercent()) {
                        style->m_seenViewPortUnitInStyle = true;
                    }
                    style->setFontSize(l);
                } else {
                    style->setFontSize(
                        parseAbsoluteFontSize(3, this->m_mediumFontSize));
                }
            }
            break;
        case CSSStyleValuePair::KeyKind::FontStyle:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Inherit) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->m_inheritedStyles.m_fontStyle =
                    parentStyle->m_inheritedStyles.m_fontStyle;
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Initial) {
                style->m_inheritedStyles.m_fontStyle =
                    FontStyleValue::NormalFontStyleValue;
            } else {
                STARFISH_ASSERT(
                    cssValues[k].valueKind() ==
                    CSSStyleValuePair::ValueKind::FontStyleValueKind);
                style->m_inheritedStyles.m_fontStyle =
                    cssValues[k].fontStyleValue();
            }
            break;
        case CSSStyleValuePair::KeyKind::FontFamily:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Inherit) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->m_inheritedStyles.m_fontFamilyDatas =
                    parentStyle->m_inheritedStyles.m_fontFamilyDatas;
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Initial) {
                style->m_inheritedStyles.m_fontFamilyDatas =
                    element->document()->starFish()->initialFontFamilyDatas();
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::KeywordValueKind) {
                FontFamilyData* data =
                    (FontFamilyData*)GC_MALLOC(sizeof(FontFamilyData) * 2);
                data[0].m_length = 1;
                data[1].m_familyName = cssValues[k].keywordValue();
                style->m_inheritedStyles.m_fontFamilyDatas = data;
            } else {
                STARFISH_ASSERT(cssValues[k].valueKind() ==
                                CSSStyleValuePair::ValueKind::ValueListKind);
                ValueList* val = cssValues[k].multiValue();
                FontFamilyData* data = (FontFamilyData*)GC_MALLOC(
                    sizeof(FontFamilyData) * (val->size() + 1));
                data[0].m_length = val->size();
                for (size_t i = 0; i < val->size(); i++) {
                    data[i + 1].m_familyName = val->at(i).keywordValue();
                }
                style->m_inheritedStyles.m_fontFamilyDatas = data;
            }
            break;
        case CSSStyleValuePair::KeyKind::FontKerning:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Inherit) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->setFontKerning(parentStyle->fontKerning());
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Initial) {
                style->setFontKerning(FontKerningValue::FontKerningAutoValue);
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::FontKerningValueKind) {
                if (cssValues[k].fontKerningValue() ==
                    FontKerningValue::FontKerningAutoValue) {
                    style->setFontKerning(
                        FontKerningValue::FontKerningAutoValue);
                } else if (cssValues[k].fontKerningValue() ==
                           FontKerningValue::FontKerningNormalValue) {
                    style->setFontKerning(
                        FontKerningValue::FontKerningNormalValue);
                } else {
                    style->setFontKerning(
                        FontKerningValue::FontKerningNoneValue);
                }
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::FontWeight:
            // <normal> | bold | bolder | lighter | 100 | 200 | 300 | 400 | 500
            // | 600 | 700 | 800 | 900 | inherit // initial -> normal
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Inherit) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->m_inheritedStyles.m_fontWeight =
                    parentStyle->m_inheritedStyles.m_fontWeight;
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Initial) {
                style->m_inheritedStyles.m_fontWeight =
                    FontWeightValue::NormalFontWeightValue;
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::FontWeightValueKind) {
                if (cssValues[k].fontWeightValue() ==
                    FontWeightValue::FourHundredsFontWeightValue) {
                    style->m_inheritedStyles.m_fontWeight =
                        FontWeightValue::NormalFontWeightValue;
                } else if (cssValues[k].fontWeightValue() ==
                           FontWeightValue::SevenHundredsFontWeightValue) {
                    style->m_inheritedStyles.m_fontWeight =
                        FontWeightValue::BoldFontWeightValue;
                } else if (cssValues[k].fontWeightValue() ==
                           FontWeightValue::BolderFontWeightValue) {
                    style->m_inheritedStyles.m_fontWeight = bolderWeight(
                        parentStyle->m_inheritedStyles.m_fontWeight);
                } else if (cssValues[k].fontWeightValue() ==
                           FontWeightValue::LighterFontWeightValue) {
                    style->m_inheritedStyles.m_fontWeight = lighterWeight(
                        parentStyle->m_inheritedStyles.m_fontWeight);
                } else {
                    style->m_inheritedStyles.m_fontWeight =
                        cssValues[k].fontWeightValue();
                }
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::WordWrap:
        case CSSStyleValuePair::KeyKind::OverflowWrap:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Inherit) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->m_inheritedStyles.m_wordWrap =
                    parentStyle->m_inheritedStyles.m_wordWrap;
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Initial) {
                style->m_inheritedStyles.m_wordWrap =
                    WordWrapValue::NormalWordWrapValue;
            } else {
                STARFISH_ASSERT(
                    cssValues[k].valueKind() ==
                    CSSStyleValuePair::ValueKind::WordWrapValueKind);
                style->m_inheritedStyles.m_wordWrap =
                    cssValues[k].wordWrapValue();
            }
            break;
        case CSSStyleValuePair::KeyKind::VerticalAlign:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Inherit) {
                style->setVerticalAlign(parentStyle->verticalAlign());
                if (style->isNumericVerticalAlign())
                    style->setVerticalAlignLength(
                        parentStyle->verticalAlignLength());
            } else if ((cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) ||
                       (cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Unset)) {
                style->setVerticalAlign(
                    VerticalAlignValue::BaselineVAlignValue);
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::VerticalAlignValueKind) {
                STARFISH_ASSERT(cssValues[k].verticalAlignValue() !=
                                VerticalAlignValue::NumericVAlignValue);
                style->setVerticalAlign(cssValues[k].verticalAlignValue());
            } else {
                Nullable<Length> length = convertValueToLength(
                    cssValues[k].valueKind(), cssValues[k].value());
                if (length.hasValue()) {
                    style->setVerticalAlignLength(length.getValue());
                } else {
                    style->setVerticalAlign(
                        VerticalAlignValue::BaselineVAlignValue);
                }
            }
            break;
        case CSSStyleValuePair::KeyKind::ImageRendering:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Inherit) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->setImageRendering(parentStyle->imageRendering());
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Initial) {
                style->setImageRendering(
                    ImageRenderingValue::ImageRenderingAutoValue);
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::ImageRenderingValueKind) {
                if (cssValues[k].imageRenderingValue() ==
                    ImageRenderingValue::ImageRenderingAutoValue) {
                    style->setImageRendering(
                        ImageRenderingValue::ImageRenderingAutoValue);
                } else if (cssValues[k].imageRenderingValue() ==
                           ImageRenderingValue::ImageRenderingCrispEdgesValue) {
                    style->setImageRendering(
                        ImageRenderingValue::ImageRenderingCrispEdgesValue);
                } else {
                    style->setImageRendering(
                        ImageRenderingValue::ImageRenderingPixelatedValue);
                }
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::TableLayout:
            // auto | fixed | initial | inherit
            switch (cssValues[k].valueKind()) {
            case CSSStyleValuePair::ValueKind::Inherit:
                style->m_tableLayout = parentStyle->m_tableLayout;
                break;
            case CSSStyleValuePair::ValueKind::Initial:
            case CSSStyleValuePair::ValueKind::Unset:
                style->setTableLayout(TableLayoutValue::AutoTableLayoutValue);
                break;
            default:
                STARFISH_ASSERT(
                    CSSStyleValuePair::ValueKind::TableLayoutValueKind ==
                    cssValues[k].valueKind());
                style->setTableLayout(cssValues[k].tableLayoutValue());
            }
            break;
        case CSSStyleValuePair::KeyKind::TextAlign:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Inherit) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->setTextAlign(parentStyle->textAlign());
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Initial) {
                style->setTextAlign(TextAlignValue::StartTextAlignValue);
            } else {
                STARFISH_ASSERT(
                    cssValues[k].valueKind() ==
                    CSSStyleValuePair::ValueKind::TextAlignValueKind);
                style->setTextAlign(cssValues[k].textAlignValue());
            }
            break;
        case CSSStyleValuePair::KeyKind::TextIndent:
            // length | percentage | inherit
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Inherit) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->setTextIndent(parentStyle->textIndent());
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Initial) {
                style->setTextIndent(Length(Length::Fixed, 0));
            } else {
                Nullable<Length> len = convertValueToLength(
                    cssValues[k].valueKind(), cssValues[k].value());
                if (len.hasValue()) {
                    style->setTextIndent(len.getValue());
                } else {
                    style->setTextIndent(Length(Length::Fixed, 0));
                }
            }
            break;
        case CSSStyleValuePair::KeyKind::TextOverflow:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Inherit)) {
                style->setTextOverflow(parentStyle->textOverflow());
            } else if ((cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) ||
                       (cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Unset)) {
                style->setTextOverflow(TextOverflowData());
            } else {
                style->setTextOverflow(cssValues[k].textOverflowValue());
            }
            break;
        case CSSStyleValuePair::KeyKind::TextDecorationLine:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Inherit) {
                style->setTextDecorationLine(parentStyle->textDecorationLine());
            } else if ((cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) ||
                       (cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Unset)) {
                CSSStyleValuePair p;
                p.setValueKind(CSSStyleValuePair::TextDecorationLineValueKind);
                p.setValue(NoneTextDecorationLineValue);
                style->textDecorationLine()->push_back(p);
            } else {
                STARFISH_ASSERT(cssValues[k].valueKind() ==
                                CSSStyleValuePair::ValueKind::ValueListKind);
                style->setTextDecorationLine(cssValues[k].multiValue());
            }
            break;
        case CSSStyleValuePair::KeyKind::TextDecorationColor:
            switch (cssValues[k].valueKind()) {
            case CSSStyleValuePair::ValueKind::Inherit:
                style->setTextDecorationColor(
                    parentStyle->textDecorationColor());
                break;
            case CSSStyleValuePair::ValueKind::Initial:
            case CSSStyleValuePair::ValueKind::Unset:
                style->setTextDecorationColor(Unit::Color(0, 0, 0, 255));
                break;
            case CSSStyleValuePair::ValueKind::ColorValueKind:
                style->setTextDecorationColor(cssValues[k].colorValue());
                break;
            default:
                STARFISH_ASSERT(
                    cssValues[k].valueKind() ==
                    CSSStyleValuePair::ValueKind::NamedColorValueKind);
                if (cssValues[k].namedColorValue() ==
                    NamedColor::NamedColorValue::currentColor) {
                    style->setTextDecorationColor(
                        parentStyle->textDecorationColor());
                } else {
                    style->setTextDecorationColor(NamedColor::namedColorToColor(
                        cssValues[k].namedColorValue()));
                }
            }
            break;
        case CSSStyleValuePair::KeyKind::TextDecorationStyle:
            switch (cssValues[k].valueKind()) {
            case CSSStyleValuePair::ValueKind::Inherit:
                style->setTextDecorationStyle(
                    parentStyle->textDecorationStyle());
                break;
            case CSSStyleValuePair::ValueKind::Initial:
            case CSSStyleValuePair::ValueKind::Unset:
                style->setTextDecorationStyle(
                    TextDecorationStyleValue::SolidTextDecorationStyleValue);
                break;
            default:
                STARFISH_ASSERT(
                    cssValues[k].valueKind() ==
                    CSSStyleValuePair::ValueKind::TextDecorationStyleValueKind);
                style->setTextDecorationStyle(
                    cssValues[k].textDecorationStyleValue());
            }
            break;
        case CSSStyleValuePair::KeyKind::TextUnderlinePosition:
            switch (cssValues[k].valueKind()) {
            case CSSStyleValuePair::ValueKind::Inherit:
            case CSSStyleValuePair::ValueKind::Unset:
                style->setTextUnderlinePosition(
                    parentStyle->textUnderlinePosition());
                break;
            case CSSStyleValuePair::ValueKind::Initial:
                style->setTextUnderlinePosition(
                    TextUnderlinePositionValue::AutoTextUnderlinePositionValue);
                break;
            default:
                STARFISH_ASSERT(cssValues[k].valueKind() ==
                                CSSStyleValuePair::ValueKind::
                                    TextUnderlinePositionValueKind);
                style->setTextUnderlinePosition(
                    cssValues[k].textUnderlinePositionValue());
            }
            break;
        case CSSStyleValuePair::KeyKind::Resize:
            switch (cssValues[k].valueKind()) {
            case CSSStyleValuePair::ValueKind::Inherit:
                style->setResize(parentStyle->resize());
                break;
            case CSSStyleValuePair::ValueKind::Initial:
            case CSSStyleValuePair::ValueKind::Unset:
                style->setResize(ResizeValue::NoneResizeValue);
                break;
            default:
                STARFISH_ASSERT(cssValues[k].valueKind() ==
                                CSSStyleValuePair::ValueKind::ResizeValueKind);
                style->setResize(cssValues[k].resizeValue());
            }
            break;
        case CSSStyleValuePair::KeyKind::TextShadow: {
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Inherit) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->setTextShadow(parentStyle->textShadow());
            } else if ((cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) ||
                       (cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::None)) {
                style->setTextShadow(ShadowDataList());
            } else {
                STARFISH_ASSERT(cssValues[k].valueKind() ==
                                CSSStyleValuePair::ValueKind::ValueListKind);
                ValueList* shadows = cssValues[k].multiValue();
                style->setTextShadow(ShadowDataList());

                for (size_t i = 0; i < shadows->size(); i++) {
                    STARFISH_ASSERT(
                        (*shadows)[i].valueKind() ==
                        CSSStyleValuePair::ValueKind::ValueListKind);

                    ValueList* shadow = (*shadows)[i].multiValue();
                    ShadowData sd;
                    for (size_t j = 0; j < shadow->size(); j++) {
                        switch ((*shadow)[j].valueKind()) {
                        case CSSStyleValuePair::ValueKind::ValueListKind: {
                            sd.setLengths((*shadow)[j].multiValue());
                        } break;
                        case CSSStyleValuePair::ValueKind::
                            NamedColorValueKind: {
                            Unit::Color color = NamedColor::namedColorToColor(
                                (*shadow)[j].namedColorValue());
                            sd.setColor(color);
                        } break;
                        case CSSStyleValuePair::ValueKind::ColorValueKind: {
                            Unit::Color color = (*shadow)[j].colorValue();
                            sd.setColor(color);
                        } break;
                        case CSSStyleValuePair::ValueKind::KeywordValueKind: {
                            sd.setInset();
                        } break;
                        default:
                            STARFISH_ASSERT_NOT_REACHED();
                            break;
                        }
                    }
                    style->addTextShadow(sd);
                }
            }
        } break;
        case CSSStyleValuePair::KeyKind::BoxShadow: {
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Initial) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->setBoxShadow(ShadowDataList());
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Inherit) {
                style->setBoxShadow(parentStyle->boxShadow());
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::ValueListKind) {
                ValueList* shadows = cssValues[k].multiValue();
                style->setBoxShadow(ShadowDataList());

                for (size_t i = 0; i < shadows->size(); i++) {
                    STARFISH_ASSERT(
                        (*shadows)[i].valueKind() ==
                        CSSStyleValuePair::ValueKind::ValueListKind);

                    ValueList* shadow = (*shadows)[i].multiValue();
                    ShadowData sd;
                    for (size_t j = 0; j < shadow->size(); j++) {
                        switch ((*shadow)[j].valueKind()) {
                        case CSSStyleValuePair::ValueKind::ValueListKind: {
                            sd.setLengths((*shadow)[j].multiValue());
                        } break;
                        case CSSStyleValuePair::ValueKind::
                            NamedColorValueKind: {
                            Unit::Color color = NamedColor::namedColorToColor(
                                (*shadow)[j].namedColorValue());
                            sd.setColor(color);
                        } break;
                        case CSSStyleValuePair::ValueKind::ColorValueKind: {
                            Unit::Color color = (*shadow)[j].colorValue();
                            sd.setColor(color);
                        } break;
                        case CSSStyleValuePair::ValueKind::KeywordValueKind: {
                            sd.setInset();
                        } break;
                        default:
                            STARFISH_ASSERT_NOT_REACHED();
                            break;
                        }
                    }
                    style->addBoxShadow(sd);
                }
            } else {
                style->setBoxShadow(ShadowDataList());
            }
        } break;
        case CSSStyleValuePair::KeyKind::Direction:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Inherit) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->m_inheritedStyles.m_direction =
                    parentStyle->m_inheritedStyles.m_direction;
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Initial) {
                style->m_inheritedStyles.m_direction =
                    DirectionValue::LtrDirectionValue;
            } else {
                STARFISH_ASSERT(
                    cssValues[k].valueKind() ==
                    CSSStyleValuePair::ValueKind::DirectionValueKind);
                style->m_inheritedStyles.m_direction =
                    cssValues[k].directionValue();
            }
            break;
        case CSSStyleValuePair::KeyKind::WhiteSpace:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Inherit) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->m_inheritedStyles.m_whiteSpace =
                    parentStyle->m_inheritedStyles.m_whiteSpace;
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Initial) {
                style->m_inheritedStyles.m_whiteSpace =
                    WhiteSpaceValue::NormalWhiteSpaceValue;
            } else {
                STARFISH_ASSERT(
                    cssValues[k].valueKind() ==
                    CSSStyleValuePair::ValueKind::WhiteSpaceValueKind);
                style->m_inheritedStyles.m_whiteSpace =
                    cssValues[k].whiteSpaceValue();
            }
            break;
        case CSSStyleValuePair::KeyKind::WordSpacing:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Inherit) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->setWordSpacing(parentStyle->wordSpacing());
            } else if (cssValues[k].valueKind() ==
                           CSSStyleValuePair::ValueKind::Initial ||
                       cssValues[k].valueKind() ==
                           CSSStyleValuePair::ValueKind::Normal) {
                style->setWordSpacing(Length(Length::Fixed, 0));
            } else if (cssValues[k].valueKind() ==
                           CSSStyleValuePair::ValueKind::Length ||
                       cssValues[k].valueKind() ==
                           CSSStyleValuePair::ValueKind::CalcValueKind) {
                Nullable<Length> length = convertValueToLength(
                    cssValues[k].valueKind(), cssValues[k].value());
                if (length.hasValue()) {
                    style->setWordSpacing(length.getValue());
                } else {
                    style->setWordSpacing(Length(Length::Fixed, 0));
                }
            }
            break;
        case CSSStyleValuePair::KeyKind::LetterSpacing:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Inherit) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->setLetterSpacing(parentStyle->wordSpacing());
            } else if (cssValues[k].valueKind() ==
                           CSSStyleValuePair::ValueKind::Initial ||
                       cssValues[k].valueKind() ==
                           CSSStyleValuePair::ValueKind::Normal) {
                style->setLetterSpacing(Length(Length::Fixed, 0));
            } else if (cssValues[k].valueKind() ==
                           CSSStyleValuePair::ValueKind::Length ||
                       cssValues[k].valueKind() ==
                           CSSStyleValuePair::ValueKind::CalcValueKind) {
                Nullable<Length> length = convertValueToLength(
                    cssValues[k].valueKind(), cssValues[k].value());
                if (length.hasValue()) {
                    style->setLetterSpacing(length.getValue());
                } else {
                    style->setLetterSpacing(Length(Length::Fixed, 0));
                }
            }
            break;

        case CSSStyleValuePair::KeyKind::BackgroundColor:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Inherit) {
                style->setBackgroundColor(parentStyle->backgroundColor());
            } else if ((cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) ||
                       (cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Unset)) {
                style->setBackgroundColor(Unit::Color(0, 0, 0, 0));
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::ColorValueKind) {
                style->setBackgroundColor(cssValues[k].colorValue());
            } else {
                STARFISH_ASSERT(
                    cssValues[k].valueKind() ==
                    CSSStyleValuePair::ValueKind::NamedColorValueKind);
                if (cssValues[k].namedColorValue() ==
                    NamedColor::NamedColorValue::currentColor) {
                    // currentColor : represents the calculated value of the
                    // element's color property
                    // --> change to valid value when arrangeStyleValues()
                    style->setBackgroundColorToCurrentColor();
                } else {
                    style->setBackgroundColor(NamedColor::namedColorToColor(
                        cssValues[k].namedColorValue()));
                }
            }
            break;
        case CSSStyleValuePair::KeyKind::BackgroundImage:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Initial) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->setBackgroundImage(String::emptyString);
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Inherit) {
                style->setBackgroundImage(parentStyle->backgroundImage());
            } else {
                STARFISH_ASSERT(cssValues[k].valueKind() ==
                                CSSStyleValuePair::ValueKind::ValueListKind);
                ValueList* list = cssValues[k].multiValue();
                for (unsigned int i = 0; i < list->size(); i++) {
                    const CSSStyleValuePair& item = (*list)[i];
                    if (item.valueKind() ==
                        CSSStyleValuePair::ValueKind::None) {
                        style->setBackgroundImage(String::emptyString, i);
                    } else if (item.valueKind() ==
                               CSSStyleValuePair::ValueKind::UrlValueKind) {
                        style->setBackgroundImage(item.urlValue(origin), i);
                    } else if (item.valueKind() ==
                               CSSStyleValuePair::ValueKind::
                                   GradientValueKind) {
                        style->setBackgroundImage(item.gradientValue(), i);
                    } else if (item.valueKind() ==
                               CSSStyleValuePair::ValueKind::Initial) {
                        style->setBackgroundImage(String::emptyString, i);
                    } else if (cssValues[k].valueKind() ==
                               CSSStyleValuePair::ValueKind::Inherit) {
                        style->setBackgroundImage(
                            parentStyle->backgroundImage(), i);
                    } else {
                        STARFISH_RELEASE_ASSERT_NOT_REACHED();
                    }
                }
            }
            break;
        case CSSStyleValuePair::KeyKind::BackgroundPositionX:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Inherit) {
                style->setBackgroundPositionX(
                    parentStyle->backgroundPositionX());
            } else {
                setComputedStyleBackgroundPositionX(style, cssValues[k]);
            }
            break;
        case CSSStyleValuePair::KeyKind::BackgroundPositionY:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Inherit) {
                style->setBackgroundPositionY(
                    parentStyle->backgroundPositionY());
            } else {
                setComputedStyleBackgroundPositionY(style, cssValues[k]);
            }
            break;
        case CSSStyleValuePair::KeyKind::BackgroundSize:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Inherit) {
                if (style->backgroundSizeIsLength()) {
                    style->setBackgroundSize(
                        parentStyle->backgroundSizeLengthValue());
                } else {
                    style->setBackgroundSize(
                        parentStyle->backgroundSizeTypeValue());
                }
            } else if ((cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) ||
                       (cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Unset)) {
                style->setBackgroundSize(LengthSize());
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::ValueListKind) {
                ValueList* layers = cssValues[k].multiValue();
                for (unsigned int l = 0; l < layers->size(); l++) {
                    const CSSStyleValuePair& layer = (*layers)[l];
                    if (layer.valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) {
                        style->setBackgroundSize(LengthSize(), l);
                    } else if (layer.valueKind() ==
                               CSSStyleValuePair::ValueKind::
                                   BackgroundSizeValueKind) {
                        style->setBackgroundSize(layer.backgroundSizeValue(),
                                                 l);
                    } else if (layer.valueKind() ==
                               CSSStyleValuePair::ValueKind::Auto) {
                        style->setBackgroundSize(LengthSize(), l);
                    } else if (layer.valueKind() ==
                               CSSStyleValuePair::ValueListKind) {
                        ValueList* list = layer.multiValue();
                        LengthSize result;
                        if (list->size() >= 1) {
                            Nullable<Length> width = convertValueToLength(
                                (*list)[0].valueKind(), (*list)[0].value());
                            if (width.hasValue()) {
                                result.m_width = width.getValue();
                            }
                        }
                        if (list->size() >= 2) {
                            Nullable<Length> height = convertValueToLength(
                                (*list)[1].valueKind(), (*list)[1].value());
                            if (height.hasValue()) {
                                result.m_height = height.getValue();
                            }
                        }
                        style->setBackgroundSize(result, l);
                    } else {
                        STARFISH_RELEASE_ASSERT_NOT_REACHED();
                    }
                }
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::BackgroundRepeatX:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Inherit) {
                style->setBackgroundRepeatX(parentStyle->backgroundRepeatX());
            } else if ((cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) ||
                       (cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Unset)) {
                style->setBackgroundRepeatX(
                    BackgroundRepeatValue::RepeatRepeatValue);
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::
                           BackgroundRepeatValueKind) {
                style->setBackgroundRepeatX(
                    cssValues[k].backgroundRepeatValue());
            } else {
                STARFISH_ASSERT(cssValues[k].valueKind() ==
                                CSSStyleValuePair::ValueKind::ValueListKind);
                ValueList* list = cssValues[k].multiValue();
                for (unsigned int i = 0; i < list->size(); i++) {
                    const CSSStyleValuePair& item = (*list)[i];
                    if (item.valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) {
                        style->setBackgroundRepeatX(
                            BackgroundRepeatValue::RepeatRepeatValue, i);
                    } else if (item.valueKind() ==
                               CSSStyleValuePair::ValueKind::
                                   BackgroundRepeatValueKind) {
                        style->setBackgroundRepeatX(
                            item.backgroundRepeatValue(), i);
                    }
                }
            }
            break;
        case CSSStyleValuePair::KeyKind::BackgroundRepeatY:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Inherit) {
                style->setBackgroundRepeatY(parentStyle->backgroundRepeatY());
            } else if ((cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) ||
                       (cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Unset)) {
                style->setBackgroundRepeatY(
                    BackgroundRepeatValue::RepeatRepeatValue);
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::
                           BackgroundRepeatValueKind) {
                style->setBackgroundRepeatY(
                    cssValues[k].backgroundRepeatValue());
            } else {
                STARFISH_ASSERT(cssValues[k].valueKind() ==
                                CSSStyleValuePair::ValueKind::ValueListKind);
                ValueList* list = cssValues[k].multiValue();
                for (unsigned int i = 0; i < list->size(); i++) {
                    const CSSStyleValuePair& item = (*list)[i];
                    if (item.valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) {
                        style->setBackgroundRepeatY(
                            BackgroundRepeatValue::RepeatRepeatValue, i);
                    } else if (item.valueKind() ==
                               CSSStyleValuePair::ValueKind::
                                   BackgroundRepeatValueKind) {
                        style->setBackgroundRepeatY(
                            item.backgroundRepeatValue(), i);
                    }
                }
            }
            break;
        case CSSStyleValuePair::KeyKind::BackgroundAttachment:
            switch (cssValues[k].valueKind()) {
            case CSSStyleValuePair::ValueKind::Initial:
            case CSSStyleValuePair::ValueKind::Unset:
                style->setBackgroundAttachment(
                    BackgroundAttachmentValue::ScrollBackgroundAttachmentValue);
                break;
            case CSSStyleValuePair::ValueKind::Inherit:
                style->setBackgroundAttachment(
                    parentStyle->backgroundAttachment());
                break;
            case CSSStyleValuePair::ValueKind::ValueListKind: {
                ValueList* list = cssValues[k].multiValue();
                for (unsigned int i = 0; i < list->size(); i++) {
                    const CSSStyleValuePair& item = (*list)[i];
                    if (item.valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) {
                        style->setBackgroundAttachment(
                            BackgroundAttachmentValue::
                                ScrollBackgroundAttachmentValue);
                    } else {
                        STARFISH_ASSERT(item.valueKind() ==
                                        CSSStyleValuePair::ValueKind::
                                            BackgroundAttachmentValueKind);
                        style->setBackgroundAttachment(
                            item.backgroundAttachmentValue(), i);
                    }
                }
                break;
            }
            default:
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::BackgroundClip:
            switch (cssValues[k].valueKind()) {
            case CSSStyleValuePair::ValueKind::Initial:
            case CSSStyleValuePair::ValueKind::Unset:
                style->setBackgroundClip(BoxValue::BorderBoxBoxValue);
                break;
            case CSSStyleValuePair::ValueKind::Inherit:
                style->setBackgroundClip(parentStyle->backgroundClip());
                break;
            case CSSStyleValuePair::ValueKind::ValueListKind: {
                ValueList* list = cssValues[k].multiValue();
                for (unsigned int i = 0; i < list->size(); i++) {
                    const CSSStyleValuePair& item = (*list)[i];
                    if (item.valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) {
                        style->setBackgroundClip(BoxValue::BorderBoxBoxValue,
                                                 i);
                    } else {
                        STARFISH_ASSERT(
                            item.valueKind() ==
                            CSSStyleValuePair::ValueKind::BoxValueKind);
                        style->setBackgroundClip(item.boxValue(), i);
                    }
                }
                break;
            }
            default:
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::BackgroundOrigin:
            switch (cssValues[k].valueKind()) {
            case CSSStyleValuePair::ValueKind::Initial:
            case CSSStyleValuePair::ValueKind::Unset:
                style->setBackgroundOrigin(BoxValue::PaddingBoxBoxValue);
                break;
            case CSSStyleValuePair::ValueKind::Inherit:
                style->setBackgroundOrigin(parentStyle->backgroundOrigin());
                break;
            case CSSStyleValuePair::ValueKind::ValueListKind: {
                ValueList* list = cssValues[k].multiValue();
                for (unsigned int i = 0; i < list->size(); i++) {
                    const CSSStyleValuePair& item = (*list)[i];
                    if (item.valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) {
                        style->setBackgroundOrigin(BoxValue::PaddingBoxBoxValue,
                                                   i);
                    } else {
                        STARFISH_ASSERT(
                            item.valueKind() ==
                            CSSStyleValuePair::ValueKind::BoxValueKind);
                        style->setBackgroundOrigin(item.boxValue(), i);
                    }
                }
                break;
            }
            default:
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::MaskImage:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Initial) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->setMaskImage(String::emptyString);
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Inherit) {
                style->setMaskImage(parentStyle->maskImage());
            } else {
                if (cssValues[k].valueKind() ==
                    CSSStyleValuePair::ValueKind::None) {
                    style->setMaskImage(String::emptyString);
                } else if (cssValues[k].valueKind() ==
                           CSSStyleValuePair::ValueKind::UrlValueKind) {
                    style->setMaskImage(cssValues[k].urlValue(origin));
                } else if (cssValues[k].valueKind() ==
                           CSSStyleValuePair::ValueKind::GradientValueKind) {
                    style->setMaskImage(cssValues[k].gradientValue());
                } else {
                    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
                }
            }
            break;
        case CSSStyleValuePair::KeyKind::MaskSize:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Initial) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->setMaskSize(LengthSize());
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Inherit) {
                if (style->backgroundSizeIsLength()) {
                    style->setMaskSize(parentStyle->maskSizeLengthValue());
                } else {
                    style->setMaskSize(parentStyle->maskSizeTypeValue());
                }
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::ValueListKind) {
                ValueList* layers = cssValues[k].multiValue();
                for (unsigned int l = 0; l < layers->size(); l++) {
                    const CSSStyleValuePair& layer = (*layers)[l];
                    if (layer.valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) {
                        style->setMaskSize(LengthSize(), l);
                    } else if (layer.valueKind() ==
                               CSSStyleValuePair::ValueKind::
                                   MaskSizeValueKind) {
                        style->setMaskSize(layer.maskSizeValue(), l);
                    } else if (layer.valueKind() ==
                               CSSStyleValuePair::ValueKind::Auto) {
                        style->setMaskSize(LengthSize(), l);
                    } else if (layer.valueKind() ==
                               CSSStyleValuePair::ValueListKind) {
                        ValueList* list = layer.multiValue();
                        LengthSize result;
                        if (list->size() >= 1) {
                            Nullable<Length> width = convertValueToLength(
                                (*list)[0].valueKind(), (*list)[0].value());
                            if (width.hasValue()) {
                                result.m_width = width.getValue();
                            }
                        }
                        if (list->size() >= 2) {
                            Nullable<Length> height = convertValueToLength(
                                (*list)[1].valueKind(), (*list)[1].value());
                            if (height.hasValue()) {
                                result.m_height = height.getValue();
                            }
                        }
                        style->setMaskSize(result, l);
                    } else {
                        STARFISH_RELEASE_ASSERT_NOT_REACHED();
                    }
                }
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::TransitionProperty:
            switch (cssValues[k].valueKind()) {
            case CSSStyleValuePair::ValueKind::Initial:
            case CSSStyleValuePair::ValueKind::Unset:
                style->setTransitionProperty(
                    TransitionPropertyValue::TransitionPropertyAllValue);
                break;
            case CSSStyleValuePair::ValueKind::Inherit:
                style->setTransitionProperty(parentStyle->transitionProperty());
                break;
            case CSSStyleValuePair::ValueKind::TransitionPropertyValueKind:
                style->setTransitionProperty(
                    cssValues[k].transitionPropertyValue());
                break;
            default:
                STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            }
            break;
        case CSSStyleValuePair::KeyKind::TransitionDuration:
            switch (cssValues[k].valueKind()) {
            case CSSStyleValuePair::ValueKind::Initial:
            case CSSStyleValuePair::ValueKind::Unset:
                style->setTransitionDuration(CSSTime(0));
                break;
            case CSSStyleValuePair::ValueKind::Inherit:
                style->setTransitionDuration(parentStyle->transitionDuration());
                break;
            case CSSStyleValuePair::ValueKind::Time:
                style->setTransitionDuration(cssValues[k].timeValue());
                break;
            case CSSStyleValuePair::ValueKind::CalcValueKind: {
                CalcData* calcData = cssValues[k].calcValue();
                CalcValueType type = calcData->type();
                if (type.isTime()) {
                    style->setTransitionDuration(calcData->timeValue());
                } else {
                    style->setTransitionDuration(CSSTime(0));
                }
                break;
            }
            default:
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::TransitionTimingFunction:
            switch (cssValues[k].valueKind()) {
            case CSSStyleValuePair::ValueKind::Initial:
            case CSSStyleValuePair::ValueKind::Unset:
                style->setTransitionTimingFunction(
                    TransitionTimingFunctionEaseValue);
                break;
            case CSSStyleValuePair::ValueKind::Inherit:
                style->setTransitionTimingFunction(
                    parentStyle->transitionTimingFunction());
                break;
            case CSSStyleValuePair::ValueKind::
                TransitionTimingFunctionValueKind:
                style->setTransitionTimingFunction(
                    cssValues[k].transitionTimingFunctionValue());
                break;
            default:
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::TransitionDelay:
            switch (cssValues[k].valueKind()) {
            case CSSStyleValuePair::ValueKind::Initial:
            case CSSStyleValuePair::ValueKind::Unset:
                style->setTransitionDelay(CSSTime(0));
                break;
            case CSSStyleValuePair::ValueKind::Inherit:
                style->setTransitionDelay(parentStyle->transitionDelay());
                break;
            case CSSStyleValuePair::ValueKind::Time:
                style->setTransitionDelay(cssValues[k].timeValue());
                break;
            case CSSStyleValuePair::ValueKind::CalcValueKind: {
                CalcData* calcData = cssValues[k].calcValue();
                CalcValueType type = calcData->type();
                if (type.isTime()) {
                    style->setTransitionDelay(calcData->timeValue());
                } else {
                    style->setTransitionDelay(CSSTime(0));
                }
                break;
            }
            default:
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::BorderImageSlice:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Initial) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->setBorderImageSlices(
                    BorderImageLengthBox(Length(Length::Percent, 1.0)));
                style->setBorderImageSliceFill(false);
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Inherit) {
                style->setBorderImageSlices(
                    parentStyle->border().image().slices());
                style->setBorderImageSliceFill(
                    parentStyle->border().image().sliceFill());
            } else {
                STARFISH_ASSERT(cssValues[k].valueKind() ==
                                CSSStyleValuePair::ValueKind::ValueListKind);
                BorderImageLength t, r, b, l;
                ValueList* values = cssValues[k].multiValue();
                unsigned int size = values->size();
                if ((*values)[size - 1].valueKind() ==
                    CSSStyleValuePair::ValueKind::KeywordValueKind) {
                    style->setBorderImageSliceFill(true);
                    size--;
                }

                if ((*values)[0].valueKind() ==
                    CSSStyleValuePair::ValueKind::Number) {
                    t.setValue((*values)[0].numberValue());
                } else {
                    Nullable<Length> nTop = convertValueToLength(
                        (*values)[0].valueKind(), (*values)[0].value());
                    if (nTop.hasValue()) {
                        t = nTop.getValue();
                    }
                }
                if (size > 1) {
                    if ((*values)[1].valueKind() ==
                        CSSStyleValuePair::ValueKind::Number) {
                        r.setValue((*values)[1].numberValue());
                    } else {
                        Nullable<Length> nRight = convertValueToLength(
                            (*values)[1].valueKind(), (*values)[1].value());
                        if (nRight.hasValue()) {
                            r = nRight.getValue();
                        }
                    }
                } else {
                    r = t;
                }
                if (size > 2) {
                    if ((*values)[2].valueKind() ==
                        CSSStyleValuePair::ValueKind::Number) {
                        b.setValue((*values)[2].numberValue());
                    } else {
                        Nullable<Length> nBottom = convertValueToLength(
                            (*values)[2].valueKind(), (*values)[2].value());
                        if (nBottom.hasValue()) {
                            b = nBottom.getValue();
                        }
                    }
                } else {
                    b = t;
                }
                if (size > 3) {
                    if ((*values)[3].valueKind() ==
                        CSSStyleValuePair::ValueKind::Number) {
                        l.setValue((*values)[3].numberValue());
                    } else {
                        Nullable<Length> nLeft = convertValueToLength(
                            (*values)[3].valueKind(), (*values)[3].value());
                        if (nLeft.hasValue()) {
                            l = nLeft.getValue();
                        }
                    }
                } else {
                    l = r;
                }
                style->setBorderImageSlices(BorderImageLengthBox(l, r, t, b));
            }
            break;
        case CSSStyleValuePair::KeyKind::BorderImageSource:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Inherit) {
                BorderData pBorder = parentStyle->border();
                style->setBorderImageSource(pBorder.image().url());
            } else if ((cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) ||
                       (cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Unset)) {
                style->setBorderImageSource(String::emptyString);
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::None) {
                style->setBorderImageSource(String::emptyString);
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::GradientValueKind) {
                style->setBorderImageSource(cssValues[k].gradientValue());
            } else {
                STARFISH_ASSERT(CSSStyleValuePair::ValueKind::UrlValueKind ==
                                cssValues[k].valueKind());
                style->setBorderImageSource(cssValues[k].urlValue(origin));
            }
            break;
        case CSSStyleValuePair::KeyKind::BorderImageRepeat:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Inherit) {
                BorderImageData borderImage = parentStyle->border().image();
                style->setBorderImageRepeatX(borderImage.repeatX());
                style->setBorderImageRepeatY(borderImage.repeatY());
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Initial) {
                style->setBorderImageRepeatX(
                    BorderImageRepeatValue::StretchValue);
                style->setBorderImageRepeatY(
                    BorderImageRepeatValue::StretchValue);
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::ValueListKind) {
                ValueList* list = cssValues[k].multiValue();
                if (list->size() == 1) {
                    style->setBorderImageRepeatX(
                        (*list)[0].borderImageRepeatValue());
                    style->setBorderImageRepeatY(
                        (*list)[0].borderImageRepeatValue());
                } else if (list->size() == 2) {
                    style->setBorderImageRepeatX(
                        (*list)[0].borderImageRepeatValue());
                    style->setBorderImageRepeatY(
                        (*list)[1].borderImageRepeatValue());
                }
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::BorderImageOutset:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Inherit) {
                style->setBorderImageOutsets(
                    parentStyle->border().image().outsets());
            } else if ((cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) ||
                       (cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Unset)) {
                style->setBorderImageOutsets(
                    BorderImageLengthBox(Length(Length::Fixed, 0)));
            } else {
                STARFISH_ASSERT(cssValues[k].valueKind() ==
                                CSSStyleValuePair::ValueKind::ValueListKind);
                BorderImageLength t, r, b, l;
                ValueList* values = cssValues[k].multiValue();
                unsigned int size = values->size();

                if ((*values)[0].valueKind() ==
                    CSSStyleValuePair::ValueKind::Number) {
                    t.setValue((*values)[0].numberValue());
                } else {
                    t = (*values)[0].toLengthValue();
                }
                if (size > 1) {
                    if ((*values)[1].valueKind() ==
                        CSSStyleValuePair::ValueKind::Number) {
                        r.setValue((*values)[1].numberValue());
                    } else {
                        r = (*values)[1].toLengthValue();
                    }
                } else {
                    r = t;
                }
                if (size > 2) {
                    if ((*values)[2].valueKind() ==
                        CSSStyleValuePair::ValueKind::Number) {
                        b.setValue((*values)[2].numberValue());
                    } else {
                        b = (*values)[2].toLengthValue();
                    }
                } else {
                    b = t;
                }
                if (size > 3) {
                    if ((*values)[3].valueKind() ==
                        CSSStyleValuePair::ValueKind::Number) {
                        l.setValue((*values)[3].numberValue());
                    } else {
                        l = (*values)[3].toLengthValue();
                    }
                } else {
                    l = r;
                }
                style->setBorderImageOutsets(BorderImageLengthBox(l, r, t, b));
            }

            break;
        case CSSStyleValuePair::KeyKind::BorderImageWidth:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Inherit) {
                style->setBorderImageWidths(
                    parentStyle->border().image().widths());
            } else if ((cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) ||
                       (cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Unset)) {
                style->setBorderImageWidths(BorderImageLengthBox(1.0));
            } else {
                STARFISH_ASSERT(cssValues[k].valueKind() ==
                                CSSStyleValuePair::ValueKind::ValueListKind);
                BorderImageLength t, r, b, l;
                ValueList* values = cssValues[k].multiValue();
                unsigned int size = values->size();

                if ((*values)[0].valueKind() ==
                    CSSStyleValuePair::ValueKind::Number) {
                    t.setValue((*values)[0].numberValue());
                } else {
                    Nullable<Length> nTop = convertValueToLength(
                        (*values)[0].valueKind(), (*values)[0].value());
                    if (nTop.hasValue()) {
                        t = nTop.getValue();
                    }
                }
                if (size > 1) {
                    if ((*values)[1].valueKind() ==
                        CSSStyleValuePair::ValueKind::Number) {
                        r.setValue((*values)[1].numberValue());
                    } else {
                        Nullable<Length> nRight = convertValueToLength(
                            (*values)[1].valueKind(), (*values)[1].value());
                        if (nRight.hasValue()) {
                            r = nRight.getValue();
                        }
                    }
                } else {
                    r = t;
                }
                if (size > 2) {
                    if ((*values)[2].valueKind() ==
                        CSSStyleValuePair::ValueKind::Number) {
                        b.setValue((*values)[2].numberValue());
                    } else {
                        Nullable<Length> nBottom = convertValueToLength(
                            (*values)[2].valueKind(), (*values)[2].value());
                        if (nBottom.hasValue()) {
                            b = nBottom.getValue();
                        }
                    }
                } else {
                    b = t;
                }
                if (size > 3) {
                    if ((*values)[3].valueKind() ==
                        CSSStyleValuePair::ValueKind::Number) {
                        l.setValue((*values)[3].numberValue());
                    } else {
                        Nullable<Length> nLeft = convertValueToLength(
                            (*values)[3].valueKind(), (*values)[3].value());
                        if (nLeft.hasValue()) {
                            l = nLeft.getValue();
                        }
                    }
                } else {
                    l = r;
                }
                style->setBorderImageWidths(BorderImageLengthBox(l, r, t, b));
            }

            break;
        case CSSStyleValuePair::KeyKind::BorderCollapse:
            // separate | collapse | initial | inherit

            switch (cssValues[k].valueKind()) {
            case CSSStyleValuePair::ValueKind::Inherit:
            case CSSStyleValuePair::ValueKind::Unset:
                style->m_inheritedStyles.m_borderCollapse =
                    parentStyle->m_inheritedStyles.m_borderCollapse;
                break;
            case CSSStyleValuePair::ValueKind::Initial:
                style->m_inheritedStyles.m_borderCollapse =
                    BorderCollapseValue::SeparateBorderCollapseValue;
                break;
            default:
                STARFISH_ASSERT(
                    CSSStyleValuePair::ValueKind::BorderCollapseValueKind ==
                    cssValues[k].valueKind());
                style->m_inheritedStyles.m_borderCollapse =
                    cssValues[k].borderCollapseValue();
            }
            break;
        case CSSStyleValuePair::KeyKind::BorderSpacing:
            // Length | initial | inherit

            switch (cssValues[k].valueKind()) {
            case CSSStyleValuePair::ValueKind::Inherit:
            case CSSStyleValuePair::ValueKind::Unset:
                style->setHorizontalBorderSpacing(
                    parentStyle->horizontalBorderSpacing());
                style->setVerticalBorderSpacing(
                    parentStyle->verticalBorderSpacing());
                break;
            case CSSStyleValuePair::ValueKind::Initial:
                style->setHorizontalBorderSpacing(Length(Length::Fixed, 0));
                style->setVerticalBorderSpacing(Length(Length::Fixed, 0));
                break;
            case CSSStyleValuePair::ValueKind::Length:
            case CSSStyleValuePair::ValueKind::CalcValueKind: {
                Nullable<Length> length = convertValueToLength(
                    cssValues[k].valueKind(), cssValues[k].value());
                if (length.hasValue()) {
                    style->setHorizontalBorderSpacing(length.getValue());
                    style->setVerticalBorderSpacing(length.getValue());
                } else {
                    style->setHorizontalBorderSpacing(Length(Length::Fixed, 0));
                    style->setVerticalBorderSpacing(Length(Length::Fixed, 0));
                }
                break;
            }
            default:
                STARFISH_ASSERT(CSSStyleValuePair::ValueKind::ValueListKind ==
                                cssValues[k].valueKind());
                ValueList* list = cssValues[k].multiValue();
                Nullable<Length> hbLength = convertValueToLength(
                    (*list)[0].valueKind(), (*list)[0].value());
                Nullable<Length> vbLength = convertValueToLength(
                    (*list)[1].valueKind(), (*list)[1].value());
                if (hbLength.hasValue()) {
                    style->setHorizontalBorderSpacing(hbLength.getValue());
                } else {
                    style->setHorizontalBorderSpacing(Length(Length::Fixed, 0));
                }

                if (vbLength.hasValue()) {
                    style->setVerticalBorderSpacing(vbLength.getValue());
                } else {
                    style->setVerticalBorderSpacing(Length(Length::Fixed, 0));
                }
            }
            break;
        case CSSStyleValuePair::KeyKind::CaptionSide:
            // top | bottom | initial | inherit
            switch (cssValues[k].valueKind()) {
            case CSSStyleValuePair::ValueKind::Inherit:
            case CSSStyleValuePair::ValueKind::Unset:
                style->m_inheritedStyles.m_captionSide =
                    parentStyle->m_inheritedStyles.m_captionSide;
                break;
            case CSSStyleValuePair::ValueKind::Initial:
                style->m_inheritedStyles.m_captionSide =
                    CaptionSideValue::TopCaptionSideValue;
                break;
            default:
                STARFISH_ASSERT(
                    CSSStyleValuePair::ValueKind::CaptionSideValueKind ==
                    cssValues[k].valueKind());
                style->m_inheritedStyles.m_captionSide =
                    cssValues[k].captionSideValue();
            }
            break;
        case CSSStyleValuePair::KeyKind::EmptyCells:
            // show | hide | initial | inherit
            switch (cssValues[k].valueKind()) {
            case CSSStyleValuePair::ValueKind::Inherit:
            case CSSStyleValuePair::ValueKind::Unset:
                style->m_inheritedStyles.m_emptyCells =
                    parentStyle->m_inheritedStyles.m_emptyCells;
                break;
            case CSSStyleValuePair::ValueKind::Initial:
                style->m_inheritedStyles.m_emptyCells =
                    EmptyCellsValue::ShowEmptyCellsValue;
                break;
            default:
                STARFISH_ASSERT(
                    CSSStyleValuePair::ValueKind::EmptyCellsValueKind ==
                    cssValues[k].valueKind());
                style->m_inheritedStyles.m_emptyCells =
                    cssValues[k].emptyCellsValue();
            }
            break;
        case CSSStyleValuePair::KeyKind::LineHeight:
            // <normal> | number | length | percentage | inherit
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Inherit) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->setLineHeight(parentStyle->lineHeight());
            } else if (cssValues[k].valueKind() ==
                           CSSStyleValuePair::ValueKind::Initial ||
                       cssValues[k].valueKind() ==
                           CSSStyleValuePair::ValueKind::Normal) {
                // The compute value should be 'normal'.
                // https://developer.mozilla.org/ko/docs/Web/CSS/line-height.
                style->setLineHeight(Length(Length::Percent, -100));
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Number) {
                // The computed value should be same as the specified value.
                style->setLineHeight(Length(Length::InheritableNumber,
                                            cssValues[k].numberValue()));
            } else {
                Nullable<Length> length = convertValueToLength(
                    cssValues[k].valueKind(), cssValues[k].value());
                if (length.hasValue()) {
                    style->setLineHeight(length.getValue());
                } else {
                    style->setLineHeight(Length(Length::Percent, -100));
                }
            }
            break;
#define ADD_RESOLVE_STYLE_POS(POS, pos)                                   \
    case CSSStyleValuePair::KeyKind::POS:                                 \
        if (cssValues[k].valueKind() ==                                   \
            CSSStyleValuePair::ValueKind::Inherit) {                      \
            style->set##POS(parentStyle->pos());                          \
        } else if (cssValues[k].valueKind() ==                            \
                       CSSStyleValuePair::ValueKind::Initial ||           \
                   cssValues[k].valueKind() ==                            \
                       CSSStyleValuePair::ValueKind::Unset ||             \
                   cssValues[k].valueKind() ==                            \
                       CSSStyleValuePair::ValueKind::Auto) {              \
            style->set##POS(Length());                                    \
        } else if (cssValues[k].valueKind() ==                            \
                   CSSStyleValuePair::ValueKind::Length) {                \
            style->set##POS(cssValues[k].cssLengthValue().toLength());    \
        } else if (cssValues[k].valueKind() ==                            \
                   CSSStyleValuePair::ValueKind::Percentage) {            \
            style->set##POS(                                              \
                Length(Length::Percent, cssValues[k].percentageValue())); \
        } else {                                                          \
            STARFISH_RELEASE_ASSERT_NOT_REACHED();                        \
        }                                                                 \
        break;
            ADD_RESOLVE_STYLE_POS(Top, top)
            ADD_RESOLVE_STYLE_POS(Right, right)
            ADD_RESOLVE_STYLE_POS(Bottom, bottom)
            ADD_RESOLVE_STYLE_POS(Left, left)
#undef ADD_RESOLVE_STYLE_POS
#define ADD_RESOLVE_STYLE_BORDER_STYLE(POS, pos)                           \
    case CSSStyleValuePair::KeyKind::Border##POS##Style:                   \
        if (cssValues[k].valueKind() ==                                    \
            CSSStyleValuePair::ValueKind::Inherit) {                       \
            BorderData pBorder = parentStyle->border();                    \
            style->setBorder##POS##Style(pBorder.pos().style());           \
        } else if ((cssValues[k].valueKind() ==                            \
                    CSSStyleValuePair::ValueKind::Initial) ||              \
                   (cssValues[k].valueKind() ==                            \
                    CSSStyleValuePair::ValueKind::Unset)) {                \
            style->setBorder##POS##Style(                                  \
                BorderStyleValue::NoneBorderStyleValue);                   \
        } else if (cssValues[k].valueKind() ==                             \
                   CSSStyleValuePair::ValueKind::BorderStyleValueKind) {   \
            style->setBorder##POS##Style(cssValues[k].borderStyleValue()); \
        } else {                                                           \
            STARFISH_RELEASE_ASSERT_NOT_REACHED();                         \
        }                                                                  \
        break;
            ADD_RESOLVE_STYLE_BORDER_STYLE(Top, top)
            ADD_RESOLVE_STYLE_BORDER_STYLE(Right, right)
            ADD_RESOLVE_STYLE_BORDER_STYLE(Bottom, bottom)
            ADD_RESOLVE_STYLE_BORDER_STYLE(Left, left)
#undef ADD_RESOLVE_STYLE_BORDER_STYLE
#define ADD_RESOLVE_STYLE_BORDER_WIDTH(POS, pos)                         \
    case CSSStyleValuePair::KeyKind::Border##POS##Width:                 \
        if (cssValues[k].valueKind() ==                                  \
            CSSStyleValuePair::ValueKind::Inherit) {                     \
            BorderData pBorder = parentStyle->border();                  \
            style->setBorder##POS##Width(pBorder.pos().width());         \
        } else if ((cssValues[k].valueKind() ==                          \
                    CSSStyleValuePair::ValueKind::Initial) ||            \
                   (cssValues[k].valueKind() ==                          \
                    CSSStyleValuePair::ValueKind::Unset)) {              \
            style->setBorder##POS##Width(Length(Length::Fixed, 3));      \
        } else if (cssValues[k].valueKind() ==                           \
                   CSSStyleValuePair::ValueKind::Length) {               \
            style->setBorder##POS##Width(                                \
                cssValues[k].cssLengthValue().toLength());               \
        } else if (cssValues[k].valueKind() ==                           \
                   CSSStyleValuePair::ValueKind::BorderWidthValueKind) { \
            if (cssValues[k].borderWidthValue() ==                       \
                BorderWidthValue::ThinBorderWidthValue) {                \
                style->setBorder##POS##Width(Length(Length::Fixed, 1));  \
            } else if (cssValues[k].borderWidthValue() ==                \
                       BorderWidthValue::MediumBorderWidthValue) {       \
                style->setBorder##POS##Width(Length(Length::Fixed, 3));  \
            } else if (cssValues[k].borderWidthValue() ==                \
                       BorderWidthValue::ThickBorderWidthValue) {        \
                style->setBorder##POS##Width(Length(Length::Fixed, 5));  \
            }                                                            \
        } else {                                                         \
            STARFISH_RELEASE_ASSERT_NOT_REACHED();                       \
        }                                                                \
        break;
            ADD_RESOLVE_STYLE_BORDER_WIDTH(Top, top)
            ADD_RESOLVE_STYLE_BORDER_WIDTH(Right, right)
            ADD_RESOLVE_STYLE_BORDER_WIDTH(Bottom, bottom)
            ADD_RESOLVE_STYLE_BORDER_WIDTH(Left, left)
#undef ADD_RESOLVE_STYLE_BORDER_WIDTH
#define ADD_RESOLVE_STYLE_BORDER_COLOR(POS, pos)                            \
    case CSSStyleValuePair::KeyKind::Border##POS##Color:                    \
        if (cssValues[k].valueKind() ==                                     \
            CSSStyleValuePair::ValueKind::Inherit) {                        \
            BorderData pBorder = parentStyle->border();                     \
            style->setBorder##POS##Color(pBorder.pos().color());            \
        } else if ((cssValues[k].valueKind() ==                             \
                    CSSStyleValuePair::ValueKind::Initial) ||               \
                   (cssValues[k].valueKind() ==                             \
                    CSSStyleValuePair::ValueKind::Unset)) {                 \
            style->clearBorder##POS##Color();                               \
        } else if (cssValues[k].valueKind() ==                              \
                   CSSStyleValuePair::ValueKind::ColorValueKind) {          \
            style->setBorder##POS##Color(cssValues[k].colorValue());        \
        } else {                                                            \
            STARFISH_ASSERT(                                                \
                cssValues[k].valueKind() ==                                 \
                CSSStyleValuePair::ValueKind::NamedColorValueKind);         \
            if (cssValues[k].namedColorValue() ==                           \
                NamedColor::NamedColorValue::currentColor) {                \
                style->clearBorder##POS##Color();                           \
            } else {                                                        \
                style->setBorder##POS##Color(NamedColor::namedColorToColor( \
                    cssValues[k].namedColorValue()));                       \
            }                                                               \
        }                                                                   \
        break;
            ADD_RESOLVE_STYLE_BORDER_COLOR(Top, top)
            ADD_RESOLVE_STYLE_BORDER_COLOR(Right, right)
            ADD_RESOLVE_STYLE_BORDER_COLOR(Bottom, bottom)
            ADD_RESOLVE_STYLE_BORDER_COLOR(Left, left)
#undef ADD_RESOLVE_STYLE_BORDER_COLOR
#define ADD_RESOLVE_STYLE_MARGIN(POS, pos)                       \
    case CSSStyleValuePair::KeyKind::Margin##POS:                \
        if (cssValues[k].valueKind() ==                          \
            CSSStyleValuePair::ValueKind::Inherit) {             \
            LengthData pMargin = parentStyle->margin();          \
            style->setMargin##POS(pMargin.pos());                \
        } else if ((cssValues[k].valueKind() ==                  \
                    CSSStyleValuePair::ValueKind::Initial) ||    \
                   (cssValues[k].valueKind() ==                  \
                    CSSStyleValuePair::ValueKind::Unset)) {      \
            style->setMargin##POS(Length(Length::Fixed, 0));     \
        } else {                                                 \
            Nullable<Length> length = convertValueToLength(      \
                cssValues[k].valueKind(), cssValues[k].value()); \
            if (length.hasValue()) {                             \
                style->setMargin##POS(length.getValue());        \
            } else {                                             \
                style->setMargin##POS(Length(Length::Fixed, 0)); \
            }                                                    \
        }                                                        \
        break;
            ADD_RESOLVE_STYLE_MARGIN(Top, top)
            ADD_RESOLVE_STYLE_MARGIN(Right, right)
            ADD_RESOLVE_STYLE_MARGIN(Bottom, bottom)
            ADD_RESOLVE_STYLE_MARGIN(Left, left)
#undef ADD_RESOLVE_STYLE_MARGIN
#define ADD_RESOLVE_STYLE_PADDING(POS, pos)                       \
    case CSSStyleValuePair::KeyKind::Padding##POS:                \
        if (cssValues[k].valueKind() ==                           \
            CSSStyleValuePair::ValueKind::Inherit) {              \
            LengthData pPadding = parentStyle->padding();         \
            style->setPadding##POS(pPadding.pos());               \
        } else if ((cssValues[k].valueKind() ==                   \
                    CSSStyleValuePair::ValueKind::Initial) ||     \
                   (cssValues[k].valueKind() ==                   \
                    CSSStyleValuePair::ValueKind::Unset)) {       \
            style->setPadding##POS(Length(Length::Fixed, 0));     \
        } else {                                                  \
            Nullable<Length> length = convertValueToLength(       \
                cssValues[k].valueKind(), cssValues[k].value());  \
            if (length.hasValue()) {                              \
                style->setPadding##POS(length.getValue());        \
            } else {                                              \
                style->setPadding##POS(Length(Length::Fixed, 0)); \
            }                                                     \
        }                                                         \
        break;
            ADD_RESOLVE_STYLE_PADDING(Top, top)
            ADD_RESOLVE_STYLE_PADDING(Right, right)
            ADD_RESOLVE_STYLE_PADDING(Bottom, bottom)
            ADD_RESOLVE_STYLE_PADDING(Left, left)
#undef ADD_RESOLVE_STYLE_PADDING
        case CSSStyleValuePair::KeyKind::Opacity:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Inherit) {
                style->setOpacity(parentStyle->opacity());
            } else if ((cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) ||
                       (cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Unset)) {
                style->setOpacity(1.0);
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Number) {
                float beforeClip = cssValues[k].numberValue();
                style->setOpacity(
                    beforeClip < 0 ? 0 : (beforeClip > 1.0 ? 1.0 : beforeClip));
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::OverflowX:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Inherit) {
                style->m_overflowX = parentStyle->overflowX();
            } else if ((cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) ||
                       (cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Unset)) {
                style->m_overflowX = OverflowValue::VisibleOverflow;
            } else {
                style->m_overflowX = cssValues[k].overflowValue();
            }
            break;
        case CSSStyleValuePair::KeyKind::OverflowY:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Inherit) {
                style->m_overflowY = parentStyle->overflowY();
            } else if ((cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) ||
                       (cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Unset)) {
                style->m_overflowY = OverflowValue::VisibleOverflow;
            } else {
                style->m_overflowY = cssValues[k].overflowValue();
            }
            break;
        case CSSStyleValuePair::KeyKind::Visibility:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Inherit) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->m_inheritedStyles.m_visibility =
                    parentStyle->visibility();
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Initial) {
                style->m_inheritedStyles.m_visibility =
                    VisibilityValue::VisibleVisibilityValue;
            } else {
                style->m_inheritedStyles.m_visibility =
                    cssValues[k].visibilityValue();
            }
            break;
        case CSSStyleValuePair::KeyKind::ZIndex:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Inherit) {
                style->setZIndex(parentStyle->zIndex());
                style->m_zIndexSpecifiedByUser = false;
            } else if (cssValues[k].valueKind() ==
                           CSSStyleValuePair::ValueKind::Initial ||
                       cssValues[k].valueKind() ==
                           CSSStyleValuePair::ValueKind::Unset ||
                       cssValues[k].valueKind() ==
                           CSSStyleValuePair::ValueKind::Auto) {
                style->setZIndex(0);
                style->m_zIndexSpecifiedByUser = false;
            } else {
                style->setZIndex(cssValues[k].int32Value());
                style->m_zIndexSpecifiedByUser = true;
            }
            break;
        case CSSStyleValuePair::KeyKind::Transform:
            style->clearTransform();
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Inherit) {
                style->setTransform(parentStyle->transforms());
            } else if (cssValues[k].valueKind() ==
                           CSSStyleValuePair::ValueKind::Initial ||
                       cssValues[k].valueKind() ==
                           CSSStyleValuePair::ValueKind::Unset ||
                       cssValues[k].valueKind() ==
                           CSSStyleValuePair::ValueKind::None) {
            } else {
                STARFISH_ASSERT(
                    cssValues[k].valueKind() ==
                    CSSStyleValuePair::ValueKind::TransformFunctions);
                CSSTransformFunctions* funcs = cssValues[k].transformValue();
                for (unsigned c = 0; c < funcs->size(); c++) {
                    CSSTransformFunction f = (*funcs)[c];
                    int valueSize = f.values()->size();
                    float dValues[valueSize];
                    for (int i = 0; i < valueSize; i++) {
                        const CSSStyleValuePair& item = (*f.values())[i];
                        if (item.valueKind() ==
                            CSSStyleValuePair::ValueKind::Number) {
                            dValues[i] = item.numberValue();
                        } else if (item.valueKind() ==
                                   CSSStyleValuePair::ValueKind::Angle) {
                            dValues[i] = item.angleValue().toDegreeValue();
                        } else if (item.valueKind() ==
                                   CSSStyleValuePair::ValueKind::
                                       CalcValueKind) {
                            CalcData* calcData = item.calcValue();
                            CalcValueType type = calcData->type();
                            if (type.isAngle()) {
                                dValues[i] =
                                    calcData->angleValue().toDegreeValue();
                            }
                        }
                    }

                    switch (f.kind()) {
                    case CSSTransformFunction::Kind::Matrix:
                        style->setTransformMatrix(dValues[0], dValues[1],
                                                  dValues[2], dValues[3],
                                                  dValues[4], dValues[5]);

                        if (dValues[0] != dValues[3] || dValues[1] ||
                            dValues[2]) {
                            style->m_rareComputedStyleData.ensureTransforms()
                                ->m_hasComplexTransform = true;
                        }
                        break;
                    case CSSTransformFunction::Kind::Translate: {
                        Length a, b(Length::Fixed, 0);
                        Nullable<Length> nA =
                            convertValueToLength((*f.values())[0].valueKind(),
                                                 (*f.values())[0].value());
                        if (nA.hasValue()) {
                            a = nA.getValue();
                        } else {
                            break;
                        }
                        if (valueSize > 1) {
                            Nullable<Length> nB = convertValueToLength(
                                (*f.values())[1].valueKind(),
                                (*f.values())[1].value());
                            if (nB.hasValue()) {
                                b = nB.getValue();
                            } else {
                                break;
                            }
                        }
                        style->setTransformTranslate(a, b);
                        break;
                    }
                    case CSSTransformFunction::Kind::TranslateX: {
                        Nullable<Length> a =
                            convertValueToLength((*f.values())[0].valueKind(),
                                                 (*f.values())[0].value());
                        if (a.hasValue()) {
                            style->setTransformTranslate(
                                a.getValue(), Length(Length::Fixed, 0));
                        }
                    } break;
                    case CSSTransformFunction::Kind::TranslateY: {
                        Nullable<Length> a =
                            convertValueToLength((*f.values())[0].valueKind(),
                                                 (*f.values())[0].value());
                        if (a.hasValue()) {
                            style->setTransformTranslate(
                                Length(Length::Fixed, 0), a.getValue());
                        }
                    } break;
                    case CSSTransformFunction::Kind::Scale:
                        if (valueSize == 1) {
                            style->setTransformScale(dValues[0], dValues[0]);
                        } else if (valueSize == 2) {
                            style->setTransformScale(dValues[0], dValues[1]);
                        } else {
                            STARFISH_RELEASE_ASSERT_NOT_REACHED();
                        }
                        break;
                    case CSSTransformFunction::Kind::ScaleX:
                        style->setTransformScale(dValues[0], 1);
                        break;
                    case CSSTransformFunction::Kind::ScaleY:
                        style->setTransformScale(1, dValues[0]);
                        break;
                    case CSSTransformFunction::Kind::Rotate:
                        style->setTransformRotate(dValues[0]);
                        style->m_rareComputedStyleData.ensureTransforms()
                            ->m_hasComplexTransform = true;
                        break;
                    case CSSTransformFunction::Kind::Skew:
                        if (valueSize == 2) {
                            style->setTransformSkew(dValues[0], dValues[1]);
                            style->m_rareComputedStyleData.ensureTransforms()
                                ->m_hasComplexTransform = true;
                            break;
                        }
                    case CSSTransformFunction::Kind::SkewX:
                        style->setTransformSkew(dValues[0], 0);
                        style->m_rareComputedStyleData.ensureTransforms()
                            ->m_hasComplexTransform = true;
                        break;
                    case CSSTransformFunction::Kind::SkewY:
                        style->setTransformSkew(0, dValues[0]);
                        style->m_rareComputedStyleData.ensureTransforms()
                            ->m_hasComplexTransform = true;
                        break;
                    default:
                        style->m_rareComputedStyleData.ensureTransforms()
                            ->m_hasComplexTransform = true;
                        style->m_rareComputedStyleData.ensureTransforms()
                            ->m_has3DTransform = true;
                        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
                    }
                }
            }
            break;
        case CSSStyleValuePair::KeyKind::TransformOrigin:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Inherit) {
                style->setTransformOrigin(parentStyle->transformOrigin());
            } else if ((cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) ||
                       (cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Unset)) {
                style->setTransformOriginValue(Length(Length::Percent, 0.5f),
                                               Length(Length::Percent, 0.5f),
                                               Length(Length::Fixed, 0.f));
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::ValueListKind) {
                ValueList* list = cssValues[k].multiValue();
                Length xAxis, yAxis, zAxis;

                xAxis = Length(Length::Percent, 0.5f);
                yAxis = Length(Length::Percent, 0.5f);
                zAxis = Length(Length::Fixed, 0.f);

                for (unsigned int i = 0; i < std::min(list->size(), (size_t)2);
                     i++) {
                    const CSSStyleValuePair& item = (*list)[i];
                    if (item.valueKind() ==
                        CSSStyleValuePair::ValueKind::SideValueKind) {
                        if (item.sideValue() == SideValue::LeftSideValue) {
                            xAxis = Length(Length::Percent, 0.0f);
                        } else if (item.sideValue() ==
                                   SideValue::RightSideValue) {
                            xAxis = Length(Length::Percent, 1.0f);
                        } else if (item.sideValue() ==
                                   SideValue::CenterSideValue) {
                        } else if (item.sideValue() ==
                                   SideValue::TopSideValue) {
                            yAxis = Length(Length::Percent, 0.0f);
                        } else if (item.sideValue() ==
                                   SideValue::BottomSideValue) {
                            yAxis = Length(Length::Percent, 1.0f);
                        }
                    } else {
                        if (i == 0) {
                            Nullable<Length> nXAxis = convertValueToLength(
                                item.valueKind(), item.value());
                            if (nXAxis.hasValue()) {
                                xAxis = nXAxis.getValue();
                            }
                        } else {
                            Nullable<Length> nYAxis = convertValueToLength(
                                item.valueKind(), item.value());
                            if (nYAxis.hasValue()) {
                                yAxis = nYAxis.getValue();
                            }
                        }
                    }
                }

                if (list->size() == 3) {
                    Nullable<Length> nZAxis = convertValueToLength(
                        (*list)[2].valueKind(), (*list)[2].value());
                    if (nZAxis.hasValue()) {
                        zAxis = nZAxis.getValue();
                    }
                }

                style->setTransformOriginValue(xAxis, yAxis, zAxis);
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }

            break;
        case CSSStyleValuePair::KeyKind::UnicodeBidi:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Inherit) {
                style->m_unicodeBidi = parentStyle->m_unicodeBidi;
            } else if ((cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) ||
                       (cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Unset)) {
                style->m_unicodeBidi = UnicodeBidiValue::NormalUnicodeBidiValue;
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::UnicodeBidiValueKind) {
                style->setUnicodeBidi(cssValues[k].unicodeBidiValue());
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::BoxSizing:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Inherit) {
                style->m_boxSizing = parentStyle->m_boxSizing;
            } else if ((cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) ||
                       (cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Unset)) {
                style->m_boxSizing = BoxSizingValue::ContentBoxBoxSizingValue;
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::BoxSizingValueKind) {
                style->setBoxSizing(cssValues[k].boxSizingValue());
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::Content:
            // Initial value is normal and it computes to 'none' for the
            // :before and :after pseudo-elements.
            style->clearContent();
            if (cssValues[k].valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial ||
                cssValues[k].valueKind() ==
                    CSSStyleValuePair::ValueKind::Inherit ||
                cssValues[k].valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset) {
            } else {
                STARFISH_ASSERT(cssValues[k].valueKind() ==
                                CSSStyleValuePair::ValueKind::ValueListKind);
                ValueList* list = cssValues[k].multiValue();
                for (unsigned int i = 0; i < list->size(); i++) {
                    const CSSStyleValuePair& item = (*list)[i];
                    if (item.valueKind() ==
                            CSSStyleValuePair::ValueKind::None ||
                        item.valueKind() ==
                            CSSStyleValuePair::ValueKind::Normal) {
                        return;
                    } else if (item.valueKind() ==
                               CSSStyleValuePair::ValueKind::UrlValueKind) {
                        style->setContentImage(item.urlValue(origin));
                    } else if (item.valueKind() ==
                               CSSStyleValuePair::ValueKind::StringValueKind) {
                        style->setContentText(item.stringValue());
                    } else if (item.valueKind() ==
                               CSSStyleValuePair::ValueKind::Attr) {
                        Nullable<String*> attrValue = element->getAttribute(
                            element->document()->createAttributeName(
                                item.attrValue()));
                        if (attrValue.hasValue()) {
                            style->setContentText(attrValue.getValue());
                        }
                        style->m_styleDamageSource =
                            (StyleDamageSource)(style->m_styleDamageSource |
                                                StyleDamageFromAttribute);

                        m_ruleSetAttrFilter.push_back(
                            element->document()
                                ->createAttributeName(item.attrValue())
                                .localNameAtomic());
                    } else if (item.valueKind() ==
                               CSSStyleValuePair::ValueKind::
                                   CounterFunctionValueKind) {
                        CSSCounterFunction* v = item.counterFunctionValue();
                        Nullable<String*> sp = v->separator();
                        Nullable<AtomicString> counterName = v->style();
                        const CounterStyle* counter = nullptr;
                        if (counterName.hasValue()) {
                            counter = CounterStyle::getKnownCounter(
                                counterName.getValue().string());
                        }
                        if (!counter) {
                            counter = CounterStyle::getDecimalCounter();
                        }
                        if (sp.hasValue()) {
                            style->setContentCounters(v->name(), sp.getValue(),
                                                      counter);
                        } else {
                            style->setContentCounter(v->name(), counter);
                        }
                    } else {
                        STARFISH_RELEASE_ASSERT_NOT_REACHED();
                    }
                }
            }
            break;
        case CSSStyleValuePair::KeyKind::FlexDirection:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Initial) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->m_flexDirection =
                    FlexDirectionValue::RowFlexDirectionValue;
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Inherit) {
                style->m_flexDirection = parentStyle->m_flexDirection;
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::FlexDirectionValueKind) {
                style->setFlexDirection(cssValues[k].flexDirectionValue());
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::FlexWrap:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Initial) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->m_flexWrap = FlexWrapValue::NoWrapFlexWrapValue;
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Inherit) {
                style->m_flexWrap = parentStyle->m_flexWrap;
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::FlexWrapValueKind) {
                style->setFlexWrap(cssValues[k].flexWrapValue());
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::Order:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Initial) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->setOrder(0);
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Inherit) {
                style->setOrder(parentStyle->order());
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Int32) {
                style->setOrder(cssValues[k].int32Value());
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::JustifyContent:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Initial) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->m_justifyContent =
                    JustifyContentValue::FlexStartJustifyContentValue;
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Inherit) {
                style->m_justifyContent = parentStyle->m_justifyContent;
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::JustifyContentValueKind) {
                style->setJustifyContent(cssValues[k].justifyContentValue());
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::AlignItems:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Initial) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->m_alignItems = AlignItemValue::StretchAlignItemValue;
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Inherit) {
                style->m_alignItems = parentStyle->m_alignItems;
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::AlignItemValueKind) {
                style->setAlignItems(cssValues[k].alignItemValue());
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::AlignSelf:
            if (cssValues[k].valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial ||
                cssValues[k].valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset ||
                cssValues[k].valueKind() ==
                    CSSStyleValuePair::ValueKind::Auto) {
                style->m_alignSelfSpecifiedByUser = false;
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Inherit) {
                style->m_alignSelfSpecifiedByUser = false;
                style->m_alignSelf = parentStyle->m_alignSelf;
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::AlignItemValueKind) {
                style->m_alignSelfSpecifiedByUser = true;
                style->setAlignSelf(cssValues[k].alignItemValue());
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::AlignContent:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Initial) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->m_alignContent =
                    AlignContentValue::StretchAlignContentValue;
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Inherit) {
                style->m_alignContent = parentStyle->m_alignContent;
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::AlignContentValueKind) {
                style->setAlignContent(cssValues[k].alignContentValue());
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::FlexGrow:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Initial) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->setFlexGrow(0);
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Inherit) {
                style->setFlexGrow(parentStyle->flexGrow());
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Number) {
                style->setFlexGrow(cssValues[k].numberValue());
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::FlexShrink:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Initial) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->setFlexShrink(1);
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Inherit) {
                style->setFlexShrink(parentStyle->flexShrink());
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Number) {
                style->setFlexShrink(cssValues[k].numberValue());
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::FlexBasis:
            if (cssValues[k].valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial ||
                cssValues[k].valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset ||
                cssValues[k].valueKind() ==
                    CSSStyleValuePair::ValueKind::Auto) {
                style->setFlexBasis(FlexBasisData(false));
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Inherit) {
                style->setFlexBasis(parentStyle->flexBasis());
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::FlexBasisValueKind) {
                style->setFlexBasis(FlexBasisData(true));
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Length) {
                Length length = cssValues[k].cssLengthValue().toLength();
                if (length.isAuto()) {
                    style->setFlexBasis(FlexBasisData(false));
                } else {
                    style->setFlexBasis(FlexBasisData(false, length));
                }
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Percentage) {
                Length length =
                    Length(Length::Percent, cssValues[k].percentageValue());
                style->setFlexBasis(FlexBasisData(false, length));
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::Fill:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Initial) {
                style->setFill(StylePaintData());
            } else if ((cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Inherit) ||
                       (cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Unset)) {
                style->setFill(parentStyle->fill());
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::None) {
                style->setFill(StylePaintData(Unit::Color(0, 0, 0, 0)));
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::ColorValueKind) {
                style->setFill(StylePaintData(cssValues[k].colorValue()));
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::NamedColorValueKind) {
                if (cssValues[k].namedColorValue() ==
                    NamedColor::currentColor) {
                    style->setFill(StylePaintData(NamedColor::currentColor));
                } else {
                    style->setFill(StylePaintData(NamedColor::namedColorToColor(
                        cssValues[k].namedColorValue())));
                }
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::FillRule:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Initial) {
                style->setFillRule(FillRuleValue::FillRuleNonZero);
            } else if ((cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Inherit) ||
                       (cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Unset)) {
                style->setFillRule(parentStyle->fillRule());
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::FillRuleValueKind) {
                style->setFillRule(cssValues[k].fillRuleValue());
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::FillOpacity:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Initial) {
                style->setFillOpacity(1);
            } else if ((cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Inherit) ||
                       (cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Unset)) {
                style->setFillOpacity(parentStyle->fillOpacity());
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Number) {
                style->setFillOpacity(cssValues[k].numberValue());
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::Stroke:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Initial) {
                style->setStroke(StylePaintData());
            } else if ((cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Inherit) ||
                       (cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Unset)) {
                style->setStroke(parentStyle->stroke());
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::None) {
                style->setStroke(StylePaintData(Unit::Color(0, 0, 0, 0)));
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::ColorValueKind) {
                style->setStroke(StylePaintData(cssValues[k].colorValue()));
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::NamedColorValueKind) {
                if (cssValues[k].namedColorValue() ==
                    NamedColor::currentColor) {
                    style->setStroke(StylePaintData(NamedColor::currentColor));
                } else {
                    style->setStroke(
                        StylePaintData(NamedColor::namedColorToColor(
                            cssValues[k].namedColorValue())));
                }
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::StrokeWidth:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Initial) {
                style->setStrokeWidth(Length(Length::Fixed, 1));
            } else if ((cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Inherit) ||
                       (cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Unset)) {
                style->setStrokeWidth(parentStyle->strokeWidth());
            } else {
                Nullable<Length> length = convertValueToLength(
                    cssValues[k].valueKind(), cssValues[k].value());
                if (length.hasValue()) {
                    style->setStrokeWidth(length.getValue());
                } else {
                    style->setStrokeWidth(Length());
                }
            }
            break;
        case CSSStyleValuePair::KeyKind::X:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Initial) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->setX(Length(Length::Fixed, 0));
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Inherit) {
                style->setX(parentStyle->x());
            } else {
                Nullable<Length> length = convertValueToLength(
                    cssValues[k].valueKind(), cssValues[k].value());
                if (length.hasValue()) {
                    style->setX(length.getValue());
                } else {
                    style->setX(Length());
                }
            }
            break;
        case CSSStyleValuePair::KeyKind::Y:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Initial) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->setY(Length(Length::Fixed, 0));
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Inherit) {
                style->setY(parentStyle->y());
            } else {
                Nullable<Length> length = convertValueToLength(
                    cssValues[k].valueKind(), cssValues[k].value());
                if (length.hasValue()) {
                    style->setY(length.getValue());
                } else {
                    style->setY(Length());
                }
            }
            break;
        case CSSStyleValuePair::KeyKind::R:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Initial) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->setR(Length(Length::Fixed, 0));
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Inherit) {
                style->setR(parentStyle->r());
            } else {
                Nullable<Length> length = convertValueToLength(
                    cssValues[k].valueKind(), cssValues[k].value());
                if (length.hasValue()) {
                    style->setR(length.getValue());
                } else {
                    style->setR(Length());
                }
            }
            break;
        case CSSStyleValuePair::KeyKind::D:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Initial) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->setD(String::emptyString);
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Inherit) {
                style->setD(parentStyle->d());
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::StringValueKind) {
                style->setD(cssValues[k].stringValue());
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::CX:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Initial) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->setCX(Length(Length::Fixed, 0));
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Inherit) {
                style->setCX(parentStyle->cx());
            } else {
                Nullable<Length> length = convertValueToLength(
                    cssValues[k].valueKind(), cssValues[k].value());
                if (length.hasValue()) {
                    style->setCX(length.getValue());
                } else {
                    style->setCX(Length());
                }
            }
            break;
        case CSSStyleValuePair::KeyKind::CY:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Initial) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->setCY(Length(Length::Fixed, 0));
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Inherit) {
                style->setCY(parentStyle->cy());
            } else {
                Nullable<Length> length = convertValueToLength(
                    cssValues[k].valueKind(), cssValues[k].value());
                if (length.hasValue()) {
                    style->setCY(length.getValue());
                } else {
                    style->setCY(Length());
                }
            }
            break;
        case CSSStyleValuePair::KeyKind::RX:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Initial) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->setRX(Length(Length::Fixed, 0));
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Inherit) {
                style->setRX(parentStyle->rx());
            } else {
                Nullable<Length> length = convertValueToLength(
                    cssValues[k].valueKind(), cssValues[k].value());
                if (length.hasValue()) {
                    style->setRX(length.getValue());
                } else {
                    style->setRX(Length());
                }
            }
            break;
        case CSSStyleValuePair::KeyKind::RY:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Initial) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->setRY(Length(Length::Fixed, 0));
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Inherit) {
                style->setRY(parentStyle->ry());
            } else {
                Nullable<Length> length = convertValueToLength(
                    cssValues[k].valueKind(), cssValues[k].value());
                if (length.hasValue()) {
                    style->setRY(length.getValue());
                } else {
                    style->setRY(Length());
                }
            }
            break;
        case CSSStyleValuePair::KeyKind::ObjectFit:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Initial) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->setObjectFit(ObjectFitValue::FillObjectFitValue);
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Inherit) {
                style->setObjectFit(parentStyle->objectFit());
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::ObjectFitValueKind) {
                style->setObjectFit(cssValues[k].objectFitValue());
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::ObjectPosition:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Inherit) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->setObjectPosition(parentStyle->objectPositionX(),
                                         parentStyle->objectPositionY());
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Initial) {
                style->setObjectPosition(Length(Length::Percent, 0.5f),
                                         Length(Length::Percent, 0.5f));
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::ValueListKind) {
                ValueList* list = cssValues[k].multiValue();
                Length x, y;

                x = Length(Length::Percent, 0.5f);
                y = Length(Length::Percent, 0.5f);

                for (unsigned int i = 0; i < std::min(list->size(), (size_t)2);
                     i++) {
                    const CSSStyleValuePair& item = (*list)[i];
                    if (item.valueKind() ==
                        CSSStyleValuePair::ValueKind::SideValueKind) {
                        if (item.sideValue() == SideValue::LeftSideValue) {
                            x = Length(Length::Percent, 0.0f);
                        } else if (item.sideValue() ==
                                   SideValue::RightSideValue) {
                            x = Length(Length::Percent, 1.0f);
                        } else if (item.sideValue() ==
                                   SideValue::CenterSideValue) {
                        } else if (item.sideValue() ==
                                   SideValue::TopSideValue) {
                            y = Length(Length::Percent, 0.0f);
                        } else if (item.sideValue() ==
                                   SideValue::BottomSideValue) {
                            y = Length(Length::Percent, 1.0f);
                        }
                    } else if (item.valueKind() ==
                               CSSStyleValuePair::ValueKind::ValuePairKind) {
                        const CSSStyleValuePair& first =
                            item.pairValue()->first();
                        const CSSStyleValuePair& second =
                            item.pairValue()->second();
                        if (first.valueKind() ==
                            CSSStyleValuePair::ValueKind::SideValueKind) {
                            if (i == 0) {
                                if (first.sideValue() ==
                                    SideValue::LeftSideValue) {
                                    Nullable<Length> nX = convertValueToLength(
                                        second.valueKind(), second.value());
                                    if (nX.hasValue()) {
                                        x = nX.getValue();
                                    }
                                } else if (first.sideValue() ==
                                           SideValue::RightSideValue) {
                                    CalcData* data = new CalcData();
                                    CalcValue val;
                                    CalcTerm* term1 = new CalcTerm();
                                    if (second.valueKind() ==
                                        CSSStyleValuePair::ValueKind::
                                            Percentage) {
                                        val.setType(CalcValueType::Percentage);
                                        val.setValue(-1 *
                                                     second.percentageValue());
                                    } else {
                                        val.setType(CalcValueType::Length);
                                        val.setValue(-1 *
                                                     second.cssLengthValue());
                                    }
                                    term1->appendValue(val);

                                    CalcTerm* term2 = new CalcTerm();
                                    val.setType(CalcValueType::Percentage);
                                    val.setValue(1.0f);
                                    term2->appendValue(val);

                                    data->appendTerm(term1);
                                    data->appendTerm(term2);
                                    x = Length(data);
                                }
                            } else {
                                if (first.sideValue() ==
                                    SideValue::TopSideValue) {
                                    Nullable<Length> nY = convertValueToLength(
                                        second.valueKind(), second.value());
                                    if (nY.hasValue()) {
                                        y = nY.getValue();
                                    }
                                } else if (first.sideValue() ==
                                           SideValue::BottomSideValue) {
                                    CalcData* data = new CalcData();
                                    CalcValue val;
                                    CalcTerm* term1 = new CalcTerm();
                                    if (second.valueKind() ==
                                        CSSStyleValuePair::ValueKind::
                                            Percentage) {
                                        val.setType(CalcValueType::Percentage);
                                        val.setValue(-1 *
                                                     second.percentageValue());
                                    } else {
                                        val.setType(CalcValueType::Length);
                                        val.setValue(-1 *
                                                     second.cssLengthValue());
                                    }
                                    term1->appendValue(val);

                                    CalcTerm* term2 = new CalcTerm();
                                    val.setType(CalcValueType::Percentage);
                                    val.setValue(1.0f);
                                    term2->appendValue(val);

                                    data->appendTerm(term1);
                                    data->appendTerm(term2);
                                    y = Length(data);
                                }
                            }
                        }
                    } else {
                        if (i == 0) {
                            Nullable<Length> nX = convertValueToLength(
                                item.valueKind(), item.value());
                            if (nX.hasValue()) {
                                x = nX.getValue();
                            }
                        } else {
                            Nullable<Length> nY = convertValueToLength(
                                item.valueKind(), item.value());
                            if (nY.hasValue()) {
                                y = nY.getValue();
                            }
                        }
                    }
                }

                style->setObjectPosition(x, y);
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::OutlineWidth:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Initial) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->setOutlineWidth(Length(Length::Fixed, 2));
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Inherit) {
                style->setOutlineWidth(parentStyle->outlineWidth());
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Length) {
                style->setOutlineWidth(cssValues[k].lengthValue());
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::BorderWidthValueKind) {
                if (cssValues[k].borderWidthValue() ==
                    BorderWidthValue::ThinBorderWidthValue) {
                    style->setOutlineWidth(Length(Length::Fixed, 1));
                } else if (cssValues[k].borderWidthValue() ==
                           BorderWidthValue::MediumBorderWidthValue) {
                    style->setOutlineWidth(Length(Length::Fixed, 3));
                } else if (cssValues[k].borderWidthValue() ==
                           BorderWidthValue::ThickBorderWidthValue) {
                    style->setOutlineWidth(Length(Length::Fixed, 5));
                }
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::OutlineColor:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Initial) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                if (style->hasRareComputeStyleData()) {
                    OutlineData* outline =
                        style->rareComputedStyleData()->outline();
                    if (outline) {
                        outline->border().clearColor();
                    }
                }
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Inherit) {
                style->setOutlineColor(parentStyle->outlineColor());
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::ColorValueKind) {
                style->setOutlineColor(cssValues[k].colorValue());
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::NamedColorValueKind) {
                if (cssValues[k].namedColorValue() ==
                    NamedColor::currentColor) {
                    if (style->hasRareComputeStyleData()) {
                        OutlineData* outline =
                            style->rareComputedStyleData()->outline();
                        if (outline) {
                            outline->border().clearColor();
                        }
                    }
                } else {
                    style->setOutlineColor(NamedColor::namedColorToColor(
                        cssValues[k].namedColorValue()));
                }
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::OutlineStyle:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::Inherit) {
                style->setOutlineStyle(parentStyle->outlineStyle());
            } else if ((cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Initial) ||
                       (cssValues[k].valueKind() ==
                        CSSStyleValuePair::ValueKind::Unset)) {
                style->setOutlineStyle(BorderStyleValue::NoneBorderStyleValue);
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::BorderStyleValueKind) {
                style->setOutlineStyle(cssValues[k].borderStyleValue());
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::OutlineOffset:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Initial) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->setOutlineOffset(Length(Length::Fixed, 0));
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Inherit) {
                style->setOutlineOffset(parentStyle->outlineOffset());
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Length) {
                style->setOutlineOffset(cssValues[k].lengthValue());
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        case CSSStyleValuePair::KeyKind::TextTransform:
            if ((cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Initial) ||
                (cssValues[k].valueKind() ==
                 CSSStyleValuePair::ValueKind::Unset)) {
                style->setTextTransform(NoneTextTransformValue);
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Inherit) {
                style->setTextTransform(parentStyle->textTransform());
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::TextTransformValueKind) {
                style->setTextTransform(cssValues[k].textTransformValue());
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            break;

#define BORDER_RADIUS_APPLY(AB, ab, AA, BB)                                   \
    case CSSStyleValuePair::KeyKind::Border##AB##Radius:                      \
        if ((cssValues[k].valueKind() ==                                      \
             CSSStyleValuePair::ValueKind::Initial) ||                        \
            (cssValues[k].valueKind() ==                                      \
             CSSStyleValuePair::ValueKind::Unset)) {                          \
            style->setBorder##AB##Radius(Length(Length::Fixed, 0),            \
                                         Length(Length::Fixed, 0));           \
        } else if (cssValues[k].valueKind() ==                                \
                   CSSStyleValuePair::ValueKind::Inherit) {                   \
            auto p = parentStyle->borderRadius();                             \
            style->setBorder##AB##Radius(p.m_##ab##AA, p.m_##ab##BB);         \
        } else if (cssValues[k].valueKind() ==                                \
                   CSSStyleValuePair::ValueKind::ValueListKind) {             \
            auto vl = cssValues[k].multiValue();                              \
            STARFISH_ASSERT(vl->size() == 1 || vl->size() == 2);              \
            Length v1;                                                        \
            Length v2;                                                        \
            auto v = vl->at(0);                                               \
            if (v.valueKind() == CSSStyleValuePair::ValueKind::Initial) {     \
                v1 = v2 = Length(Length::Fixed, 0);                           \
            } else if (v.valueKind() ==                                       \
                       CSSStyleValuePair::ValueKind::Inherit) {               \
                v1 = parentStyle->borderRadius().m_##ab##AA;                  \
                v2 = parentStyle->borderRadius().m_##ab##BB;                  \
            } else {                                                          \
                v1 = v2 = v.toLengthValue();                                  \
            }                                                                 \
            if (vl->size() == 2) {                                            \
                auto v = vl->at(1);                                           \
                if (v.valueKind() == CSSStyleValuePair::ValueKind::Initial) { \
                    v2 = Length(Length::Fixed, 0);                            \
                } else if (v.valueKind() ==                                   \
                           CSSStyleValuePair::ValueKind::Inherit) {           \
                    v2 = parentStyle->borderRadius().m_##ab##BB;              \
                } else {                                                      \
                    v2 = v.toLengthValue();                                   \
                }                                                             \
            }                                                                 \
            style->setBorder##AB##Radius(v1, v2);                             \
        } else {                                                              \
            STARFISH_RELEASE_ASSERT_NOT_REACHED();                            \
        }                                                                     \
        break;
        case CSSStyleValuePair::KeyKind::Clip:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::RectValueKind) {
                style->setClip(cssValues[k].clip());
            }
            break;
            BORDER_RADIUS_APPLY(TopLeft, topLeft, Horizontal, Vertical)
            BORDER_RADIUS_APPLY(TopRight, topRight, Horizontal, Vertical)
            BORDER_RADIUS_APPLY(BottomRight, bottomRight, Horizontal, Vertical)
            BORDER_RADIUS_APPLY(BottomLeft, bottomLeft, Horizontal, Vertical)

        case CSSStyleValuePair::KeyKind::ListStyleType:
            if (cssValues[k].valueKind() == CSSStyleValuePair::Inherit ||
                cssValues[k].valueKind() == CSSStyleValuePair::Unset) {
                style->setListStyleType(
                    parentStyle->listStyleData().typeData());
            } else if (cssValues[k].valueKind() == CSSStyleValuePair::None) {
                style->setListStyleType(CounterStyle::getNoneCounter());
            } else if (cssValues[k].valueKind() == CSSStyleValuePair::Initial) {
                style->setListStyleType(CounterStyle::getDiscCounter());
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::AtomicStringValueKind) {
                const AtomicString& counterName =
                    cssValues[k].atomicStringValue();
                auto counterStyle =
                    CounterStyle::getKnownCounter(counterName.string());
                if (counterStyle) {
                    style->setListStyleType(counterStyle);
                } else {
                    style->setListStyleType(CounterStyle::getDecimalCounter());
                }
            } else {
                style->setListStyleType(cssValues[k].stringValue());
            }
            break;
        case CSSStyleValuePair::KeyKind::ListStyleImage:
            if (cssValues[k].valueKind() ==
                    CSSStyleValuePair::ValueKind::Inherit ||
                cssValues[k].valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset) {
                style->setListStyleImage(parentStyle->listStyleData().image());
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::None) {
                style->setListStyleImage(String::emptyString);
            } else {
                style->setListStyleImage(cssValues[k].urlValue(origin));
            }
            break;
        case CSSStyleValuePair::KeyKind::ListStylePosition:
            if (cssValues[k].valueKind() ==
                    CSSStyleValuePair::ValueKind::Inherit ||
                cssValues[k].valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset) {
                style->setListStylePosition(parentStyle->listStylePosition());
            } else if (cssValues[k].valueKind() ==
                       CSSStyleValuePair::ValueKind::Initial) {
                style->setListStylePosition(
                    ListStylePositionValue::ListStylePositionOutside);
            } else {
                style->setListStylePosition(
                    cssValues[k].listStylePositionValue());
            }
            break;
        case CSSStyleValuePair::KeyKind::UserSelect:
            switch (cssValues[k].valueKind()) {
            case CSSStyleValuePair::ValueKind::Inherit:
                style->setUserSelect(parentStyle->userSelect());
                break;
            case CSSStyleValuePair::ValueKind::Initial:
            case CSSStyleValuePair::ValueKind::Unset:
            case CSSStyleValuePair::ValueKind::Auto:
                style->setUserSelect(UserSelectValue::NoneUserSelectValue);
                break;
            default:
                STARFISH_ASSERT(
                    CSSStyleValuePair::ValueKind::UserSelectValueKind ==
                    cssValues[k].valueKind());
                style->setUserSelect(cssValues[k].userSelectValue());
                break;
            }
            break;
        case CSSStyleValuePair::KeyKind::Hyphens:
            switch (cssValues[k].valueKind()) {
            case CSSStyleValuePair::ValueKind::Inherit:
            case CSSStyleValuePair::ValueKind::Unset:
                style->setHyphens(parentStyle->hyphens());
                break;
            case CSSStyleValuePair::ValueKind::Initial:
            case CSSStyleValuePair::ValueKind::Auto:
                style->setHyphens(HyphensValue::NoneHyphensValue);
                break;
            default:
                STARFISH_ASSERT(
                    CSSStyleValuePair::ValueKind::HyphensValueKind ==
                    cssValues[k].valueKind());
                style->setHyphens(cssValues[k].hyphensValue());
                break;
            }
            break;
        case CSSStyleValuePair::KeyKind::LineBreak:
            switch (cssValues[k].valueKind()) {
            case CSSStyleValuePair::ValueKind::Inherit:
                style->setLineBreak(parentStyle->lineBreak());
                break;
            case CSSStyleValuePair::ValueKind::Initial:
            case CSSStyleValuePair::ValueKind::Auto:
            case CSSStyleValuePair::ValueKind::Unset:
                style->setLineBreak(LineBreakValue::NormalLineBreakValue);
                break;
            default:
                STARFISH_ASSERT(
                    CSSStyleValuePair::ValueKind::LineBreakValueKind ==
                    cssValues[k].valueKind());
                style->setLineBreak(cssValues[k].lineBreakValue());
            }
            break;
        case CSSStyleValuePair::KeyKind::WordBreak:
            switch (cssValues[k].valueKind()) {
            case CSSStyleValuePair::ValueKind::Inherit:
            case CSSStyleValuePair::ValueKind::Unset:
                style->setWordBreak(parentStyle->wordBreak());
                break;
            case CSSStyleValuePair::ValueKind::Initial:
                style->setWordBreak(WordBreakValue::NormalWordBreakValue);
                break;
            default:
                STARFISH_ASSERT(
                    CSSStyleValuePair::ValueKind::WordBreakValueKind ==
                    cssValues[k].valueKind());
                style->setWordBreak(cssValues[k].wordBreakValue());
            }
            break;
        case CSSStyleValuePair::KeyKind::GridTemplateColumns:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::GridTemplateUnits) {
                style->setGridTemplateColumns(cssValues[k].gridTemplateUnits());
            }
            break;
        case CSSStyleValuePair::KeyKind::GridTemplateRows:
            if (cssValues[k].valueKind() ==
                CSSStyleValuePair::ValueKind::GridTemplateUnits) {
                style->setGridTemplateRows(cssValues[k].gridTemplateUnits());
            }
            break;
        case CSSStyleValuePair::KeyKind::CaretColor:
            switch (cssValues[k].valueKind()) {
            case CSSStyleValuePair::ValueKind::Inherit:
            case CSSStyleValuePair::ValueKind::Unset:
                style->setCaretColor(parentStyle->caretColor());
                break;
            case CSSStyleValuePair::ValueKind::Initial:
                style->setCaretColor(Unit::Color(0, 0, 0, 255));
                break;
            case CSSStyleValuePair::ValueKind::ColorValueKind:
                style->setCaretColor(cssValues[k].colorValue());
                break;
            default:
                STARFISH_ASSERT(
                    cssValues[k].valueKind() ==
                    CSSStyleValuePair::ValueKind::NamedColorValueKind);
                if (cssValues[k].namedColorValue() ==
                    NamedColor::NamedColorValue::currentColor) {
                    style->ensureInheritedRareData()->m_caretColor =
                        parentStyle->ensureInheritedRareData()->m_caretColor;
                } else {
                    style->setCaretColor(NamedColor::namedColorToColor(
                        cssValues[k].namedColorValue()));
                }
            }
            break;
        case CSSStyleValuePair::KeyKind::CounterReset:
            switch (cssValues[k].valueKind()) {
            case CSSStyleValuePair::ValueKind::Inherit:
            case CSSStyleValuePair::ValueKind::Unset:
                style->setCounterReset(parentStyle->counterReset());
                break;
            case CSSStyleValuePair::ValueKind::Initial:
            case CSSStyleValuePair::ValueKind::None:
                style->setCounterReset(nullptr);
                break;
            default:
                ValueList* list = cssValues[k].multiValue();
                size_t size = list->size();
                STARFISH_ASSERT(size % 2 == 0);
                for (size_t i = 0; i < size; i += 2) {
                    style->setCounterResetItem(list->at(i).atomicStringValue(),
                                               list->at(i + 1).int32Value());
                }
                break;
            }
            break;
        case CSSStyleValuePair::KeyKind::CounterIncrement:
            switch (cssValues[k].valueKind()) {
            case CSSStyleValuePair::ValueKind::Inherit:
            case CSSStyleValuePair::ValueKind::Unset:
                style->setCounterIncrement(parentStyle->counterIncrement());
                break;
            case CSSStyleValuePair::ValueKind::Initial:
            case CSSStyleValuePair::ValueKind::None:
                style->setCounterIncrement(nullptr);
                break;
            default:
                ValueList* list = cssValues[k].multiValue();
                size_t size = list->size();
                STARFISH_ASSERT(size % 2 == 0);
                for (size_t i = 0; i < size; i += 2) {
                    style->setCounterIncrementItem(
                        list->at(i).atomicStringValue(),
                        list->at(i + 1).int32Value());
                }
                break;
            }
            break;
        case CSSStyleValuePair::KeyKind::GridColumnStart:
            switch (cssValues[k].valueKind()) {
            case CSSStyleValuePair::ValueKind::Int32:
                style->setGridColumnStart(cssValues[k].int32Value());
                break;
            default:
                break;
            }
            break;
        case CSSStyleValuePair::KeyKind::GridColumnEnd:
            switch (cssValues[k].valueKind()) {
            case CSSStyleValuePair::ValueKind::Int32:
                style->setGridColumnEnd(cssValues[k].int32Value());
                break;
            default:
                break;
            }
            break;
        case CSSStyleValuePair::KeyKind::GridRowStart:
            switch (cssValues[k].valueKind()) {
            case CSSStyleValuePair::ValueKind::Int32:
                style->setGridRowStart(cssValues[k].int32Value());
                break;
            default:
                break;
            }
            break;
        case CSSStyleValuePair::KeyKind::GridRowEnd:
            switch (cssValues[k].valueKind()) {
            case CSSStyleValuePair::ValueKind::Int32:
                style->setGridRowEnd(cssValues[k].int32Value());
                break;
            default:
                break;
            }
            break;
        case CSSStyleValuePair::KeyKind::Empty:
            break;
        default:
            break;
        }
    }
}

void StyleResolver::collectMatchingRulesFromAuthorSheet(
    StyleResolveContext& ctx,
    const GCUnorderedMultiMap<
        AtomicString, std::pair<StyleRule*, ResourceURL*>>::iterator& begin,
    const GCUnorderedMultiMap<
        AtomicString, std::pair<StyleRule*, ResourceURL*>>::iterator& end,
    CSSSelector::Type type, Element* element, AtomicString elementName,
    AtomicString elementId, const GCVector<AtomicString>& elementClasses,
    MatchedStyleRules<32>& authorRules, ComputedStyle* ret,
    PseudoElementType pseudoElementType)
{
    bool canUseAncestorSelectorFilter =
        ctx.m_ancestorSelectorFilter->canUseAncestorSelectorFilter(element);

    for (auto it = begin; it != end; ++it) {
        StyleRule* rule = it->second.first;
        ResourceURL* url = it->second.second;

        if (pseudoElementType == PseudoElementType::PseudoElementNone) {
            if (type == CSSSelector::Type::Id && rule->isSimpleIDSelector()) {
                authorRules.push_back(std::make_pair(rule, url));
                continue;
            }
            if (type == CSSSelector::Type::Class &&
                rule->isSimpleClassSelector()) {
                authorRules.push_back(std::make_pair(rule, url));
                continue;
            }
            if (type == CSSSelector::Type::Tag && rule->isSimpleTagSelector()) {
                authorRules.push_back(std::make_pair(rule, url));
                continue;
            }
        }

        if (canUseAncestorSelectorFilter &&
            ctx.m_ancestorSelectorFilter->canIgnoreSelector(rule, element)) {
            continue;
        }

        const CSSSelectorList& selectorList = rule->selectorList();

        MatchResult result;
        if (matchSelector(element, elementName, elementId, elementClasses,
                          selectorList, 0, result) == Match::SelectorMatches) {
            if (result.pseudoType != PseudoElementType::PseudoElementNone) {
                if (result.pseudoType == pseudoElementType) {
                    ret->setPseudoType(pseudoElementType);
                    authorRules.push_back(std::make_pair(rule, url));
                } else {
                    if (result.pseudoType ==
                        PseudoElementType::PseudoElementFirstLine) {
                        ret->m_seenPseudoElementFirstLine = true;
                    } else if (result.pseudoType ==
                               PseudoElementType::PseudoElementFirstLetter) {
                        ret->m_seenPseudoElementFirstLetter = true;
                    } else if (result.pseudoType ==
                               PseudoElementType::PseudoElementBefore) {
                        ret->m_seenPseudoElementBefore = true;
                    } else if (result.pseudoType ==
                               PseudoElementType::PseudoElementAfter) {
                        ret->m_seenPseudoElementAfter = true;
                    } else if (result.pseudoType ==
                               PseudoElementType::PseudoElementFirstLine) {
                        ret->m_seenPseudoElementFirstLine = true;
                    }
                }
            } else if (pseudoElementType ==
                       PseudoElementType::PseudoElementNone) {
                authorRules.push_back(std::make_pair(rule, url));
            }
        }
        if (result.seenCombinator) {
            // This is used to determine whether to recalculate the children's
            // style when the attributes of the element is changed.
            ret->setStyleDamageSource(result.styleDamageFrom);
        } else {
            ret->setStyleDamageSource((StyleDamageSource)(
                result.styleDamageFrom & StyleDamageFromDOMTree));
        }
    }
}

static bool comparingRules(const std::pair<StyleRule*, ResourceURL*>& r1,
                           const std::pair<StyleRule*, ResourceURL*>& r2)
{
    const auto& a = r1.first;
    const auto& b = r2.first;

    bool aUA = a->isUARule();
    bool aUB = b->isUARule();

    if (aUA && aUB) {
    } else if (aUA) {
        return true;
    } else if (aUB) {
        return false;
    }

    if (a->selectorList().specificity() == b->selectorList().specificity()) {
        return a->order() < b->order();
    }
    return a->selectorList().specificity() < b->selectorList().specificity();
}

template <typename Iter, typename Func>
void sortVector(Iter begin, Iter end, Func matchingRule)
{
    std::stable_sort(begin, end, matchingRule);
}

void StyleResolver::matchAllRules(StyleResolveContext& ctx, Element* element,
                                  ComputedStyle* ret, ComputedStyle* parent,
                                  PseudoElementType pseudoElementType)
{
    AtomicString elementName = element->name().localNameAtomic();
    AtomicString elementId = element->atomicId();
    const GCVector<AtomicString>& elementClasses = element->classNames();

    const size_t matchedRulesInlineStorageSize = 32;
    MatchedStyleRules<matchedRulesInlineStorageSize> matchedRules;

    if (element->hasId()) {
        auto& rules = m_ruleSet->idRules();
        auto range = rules.equal_range(elementId);
        collectMatchingRulesFromAuthorSheet(
            ctx, range.first, range.second, CSSSelector::Type::Id, element,
            elementName, elementId, elementClasses, matchedRules, ret,
            pseudoElementType);
    }

    if (element->hasClass()) {
        auto& rules = m_ruleSet->classRules();
        size_t classLen = elementClasses.size();
        for (unsigned k = 0; k < classLen; k++) {
            auto range = rules.equal_range(elementClasses[k]);
            collectMatchingRulesFromAuthorSheet(
                ctx, range.first, range.second, CSSSelector::Type::Class,
                element, elementName, elementId, elementClasses, matchedRules,
                ret, pseudoElementType);
        }
    }

    {
        auto& rules = m_ruleSet->tagRules();
        auto range = rules.equal_range(elementName);
        collectMatchingRulesFromAuthorSheet(
            ctx, range.first, range.second, CSSSelector::Type::Tag, element,
            elementName, elementId, elementClasses, matchedRules, ret,
            pseudoElementType);
    }

    {
        auto& rules = m_ruleSet->universalRules();
        collectMatchingRulesFromAuthorSheet(
            ctx, rules.begin(), rules.end(), CSSSelector::Type::UnKnown,
            element, elementName, elementId, elementClasses, matchedRules, ret,
            pseudoElementType);
    }

    auto begin = &matchedRules[0];
    auto end = matchedRules.data() + matchedRules.size();
    auto authorSheetBegin = end;

    sortVector(begin, end, comparingRules);

    {
        auto iter = begin;
        while (iter != end) {
            if (!iter->first->isUARule()) {
                authorSheetBegin = iter;
                break;
            }
            iter++;
        }
    }

    // Apply ua-rules
    // We disallow ua !important rules due to performance now
    {
        auto iter = begin;
        while (iter != authorSheetBegin) {
            apply(element, iter->first->styleDeclaration()->m_cssValues,
                  iter->first->styleDeclaration()->m_cssCustomValues,
                  iter->second, ret, parent, false);
            iter++;
        }
    }

    // Apply presentation attribute's style
    CSSStyleValuePairVectorHolder cssValues;
    element->styleForPresentationAttribute(cssValues);
    // FIXME : clean up to remove this vector called by empty.
    GCVector<MutablePropertyValue> empty;
    apply(element, cssValues.mutableData(), empty, nullptr, ret, parent, false);

    // Apply non-important author-rules
    {
        auto iter = authorSheetBegin;
        while (iter != end) {
            apply(element, iter->first->styleDeclaration()->m_cssValues,
                  iter->first->styleDeclaration()->m_cssCustomValues,
                  iter->second, ret, parent, false);
            iter++;
        }
    }

    // inline style
    if (pseudoElementType == PseudoElementNone &&
        element->inlineStyleWithoutCreation()) {
        apply(element, element->inlineStyleWithoutCreation()->m_cssValues,
              element->inlineStyleWithoutCreation()->m_cssCustomValues,
              element->document()->baseURL(), ret, parent, false);
    }

    // Apply important author-rules
    {
        auto iter = authorSheetBegin;
        while (iter != end) {
            apply(element, iter->first->styleDeclaration()->m_cssValues,
                  iter->first->styleDeclaration()->m_cssCustomValues,
                  iter->second, ret, parent, true);
            iter++;
        }
    }

    // inline style
    if (pseudoElementType == PseudoElementNone &&
        element->inlineStyleWithoutCreation()) {
        apply(element, element->inlineStyleWithoutCreation()->m_cssValues,
              element->inlineStyleWithoutCreation()->m_cssCustomValues,
              element->document()->baseURL(), ret, parent, true);
    }
}

StyleResolver::Match StyleResolver::matchSelector(
    Element* element, AtomicString elementName, AtomicString elementId,
    const GCVector<AtomicString>& elementClasses,
    const CSSSelectorList& selectorList, unsigned idx, MatchResult& result,
    bool isQueryingSelector)
{
    STARFISH_ASSERT(idx < selectorList.size());

    CSSSelector* selector = selectorList[idx];
    if (!checkOne(element, elementName, elementId, elementClasses, selector,
                  result, isQueryingSelector)) {
        return Match::SelectorFailsLocally;
    }

    if (selector->isLastInTagHistory()) {
        return Match::SelectorMatches;
    }

    Match match;
    if (selector->relation() == CSSSelector::RelationType::SubSelector) {
        match = matchSelector(element, elementName, elementId, elementClasses,
                              selectorList, ++idx, result, isQueryingSelector);
    } else {
        result.seenCombinator = true;
        match =
            matchForRelation(element, elementName, elementId, elementClasses,
                             selectorList, selector->relation(), ++idx, result);
    }
    return match;
}

StyleResolver::Match StyleResolver::matchForRelation(
    Element* element, AtomicString elementName, AtomicString elementId,
    const GCVector<AtomicString>& elementClasses,
    const CSSSelectorList& selectorList, CSSSelector::RelationType relation,
    unsigned idx, MatchResult& result)
{
    STARFISH_ASSERT(idx < selectorList.size());

    CSSSelector* selector = selectorList[idx];
    switch (relation) {
    case CSSSelector::RelationType::Descendant: {
        Element* parent = element->parentElement();
        while (parent) {
            AtomicString elementName = parent->name().localNameAtomic();
            AtomicString elementId = parent->atomicId();
            const GCVector<AtomicString>& elementClasses = parent->classNames();
            if (matchSelector(parent, elementName, elementId, elementClasses,
                              selectorList, idx,
                              result) == Match::SelectorMatches) {
                return Match::SelectorMatches;
            }
            parent = parent->parentElement();
        }

        return Match::SelectorFailsCompletely;
    }
    case CSSSelector::RelationType::Child: {
        Element* parent = element->parentElement();
        if (parent) {
            AtomicString elementName = parent->name().localNameAtomic();
            AtomicString elementId = parent->atomicId();
            const GCVector<AtomicString>& elementClasses = parent->classNames();
            if (matchSelector(parent, elementName, elementId, elementClasses,
                              selectorList, idx,
                              result) == Match::SelectorMatches) {
                return Match::SelectorMatches;
            }
            return Match::SelectorFailsCompletely;
        } else {
            return Match::SelectorFailsCompletely;
        }
    }
    case CSSSelector::RelationType::AdjacentSibling: {
        Element* previousSibling = element->previousElementSibling();
        if (previousSibling) {
            AtomicString elementName =
                previousSibling->name().localNameAtomic();
            AtomicString elementId = previousSibling->atomicId();
            const GCVector<AtomicString>& elementClasses =
                previousSibling->classNames();
            if (matchSelector(previousSibling, elementName, elementId,
                              elementClasses, selectorList, idx,
                              result) == Match::SelectorMatches) {
                return Match::SelectorMatches;
            } else {
                return Match::SelectorFailsCompletely;
            }
        } else {
            return Match::SelectorFailsCompletely;
        }
    }
    case CSSSelector::RelationType::GeneralSibling: {
        Element* previousSibling = element->previousElementSibling();
        while (previousSibling) {
            AtomicString elementName =
                previousSibling->name().localNameAtomic();
            AtomicString elementId = previousSibling->atomicId();
            const GCVector<AtomicString>& elementClasses =
                previousSibling->classNames();
            if (matchSelector(previousSibling, elementName, elementId,
                              elementClasses, selectorList, idx,
                              result) == Match::SelectorMatches) {
                return Match::SelectorMatches;
            }
            previousSibling = previousSibling->previousElementSibling();
        }
        return Match::SelectorFailsCompletely;
    }
    default:
        return Match::SelectorFailsCompletely;
    }
}

bool StyleResolver::checkOne(Element* element, AtomicString elementName,
                             AtomicString elementId,
                             const GCVector<AtomicString>& elementClasses,
                             CSSSelector* selector, MatchResult& result,
                             bool isQueryingSelector)
{
    auto selectorType = selector->type();
    if (selectorType == CSSSelector::Type::Class) {
        result.styleDamageFrom =
            (StyleDamageSource)(result.styleDamageFrom | StyleDamageFromClass);
        auto txt = selector->selectorText();
        size_t len = elementClasses.size();
        for (unsigned i = 0; i < len; i++) {
            if (txt == elementClasses[i]) {
                return true;
            }
        }
        return false;
    } else if (selectorType == CSSSelector::Type::Tag) {
        return (elementName == selector->selectorText());
    } else if (selectorType == CSSSelector::Type::Id) {
        STARFISH_ASSERT(!selector->selectorText().isEmptyAtomicString());
        result.styleDamageFrom =
            (StyleDamageSource)(result.styleDamageFrom | StyleDamageFromID);
        return elementId == selector->selectorText();
    } else {
        switch (selectorType) {
        case CSSSelector::Type::Universal:
            return true;
        case CSSSelector::AttributeExact:   // Example: E[foo="bar"]
        case CSSSelector::AttributeSet:     // Example: E[foo]
        case CSSSelector::AttributeHyphen:  // Example: E[foo|="bar"]
        case CSSSelector::AttributeList:    // Example: E[foo~="bar"]
        case CSSSelector::AttributeContain: // css3: E[foo*="bar"]
        case CSSSelector::AttributeBegin:   // css3: E[foo^="bar"]
        case CSSSelector::AttributeEnd:     // css3: E[foo$="bar"]
            result.styleDamageFrom = (StyleDamageSource)(
                result.styleDamageFrom | StyleDamageFromAttribute);
            return anyAttributeMatches(element, selector->type(),
                                       selector->asCSSAttributeSelector(),
                                       result);
        case CSSSelector::Type::PseudoClass:
            return checkPseudoClass(element, selector->asCSSPseudoSelector(),
                                    result);
        case CSSSelector::Type::PseudoElement:
            // while the use of pseudo-elements in selectors of querySelector is
            // permitted, they will not match any elements in the document, and
            // thus
            // would not result in any elements being returned.
            return isQueryingSelector
                       ? false
                       : checkPseudoElement(
                             element, selector->asCSSPseudoSelector(), result);
        default:
            return false;
        }
    }
}

static bool isFirstChild(Element* element)
{
    return element->parentElement() ? !element->previousElementSibling()
                                    : false;
}

static bool isLastChild(Element* element)
{
    return element->parentElement() ? !element->nextElementSibling() : false;
}

static bool isFirstOfType(Element* element)
{
    Node* sibling = element->previousSibling();
    while (sibling) {
        if (sibling->isElement() &&
            sibling->asElement()->name().localName()->equalsIgnoreCase(
                element->name().localName())) {
            return false;
        }
        sibling = sibling->previousSibling();
    }

    return true;
}

static bool isLastOfType(Element* element)
{
    Node* sibling = element->nextSibling();
    while (sibling) {
        if (sibling->isElement() &&
            sibling->asElement()->name().localName()->equalsIgnoreCase(
                element->name().localName())) {
            return false;
        }
        sibling = sibling->nextSibling();
    }

    return true;
}

static bool isEmpty(Element* element)
{
    Node* child = element->firstChild();
    while (child) {
        if (child->isElement()) {
            return false;
        } else if (child->isText()) {
            STARFISH_ASSERT(child->asText()->textContent().hasValue());
            if (child->textContent().getValue()->length() > 0) {
                return false;
            }
        }

        child = child->nextSibling();
    }

    return true;
}

static unsigned nthChildIndex(Element* element)
{
    unsigned index = 1;

    Node* sibling = element->previousSibling();
    while (sibling) {
        if (sibling->isElement()) {
            ++index;
        }
        sibling = sibling->previousSibling();
    }

    return index;
}

static unsigned nthOfTypeIndex(Element* element)
{
    unsigned index = 1;
    String* tag = element->tagName();

    Node* sibling = element->previousSibling();
    while (sibling) {
        if (sibling->isElement() &&
            sibling->asElement()->tagName()->equals(tag)) {
            ++index;
        }
        sibling = sibling->previousSibling();
    }

    return index;
}

static unsigned nthLastChildIndex(Element* element)
{
    unsigned index = 1;

    Node* sibling = element->nextSibling();
    while (sibling) {
        if (sibling->isElement()) {
            ++index;
        }
        sibling = sibling->nextSibling();
    }

    return index;
}

static unsigned nthLastOfTypeIndex(Element* element)
{
    int index = 1;
    String* tag = element->tagName();

    Node* sibling = element->nextSibling();
    while (sibling) {
        if (sibling->isElement() &&
            sibling->asElement()->tagName()->equals(tag)) {
            ++index;
        }
        sibling = sibling->nextSibling();
    }

    return index;
}

bool StyleResolver::checkPseudoClass(Element* element,
                                     CSSPseudoSelector* selector,
                                     MatchResult& result)
{
    switch (selector->pseudoType()) {
    case CSSSelector::PseudoType::PseudoHover:
        result.styleDamageFrom = (StyleDamageSource)(
            result.styleDamageFrom | StyleDamageFromElementState);
        return element->state() & Node::NodeState::NodeStateHovered;
    case CSSSelector::PseudoType::PseudoActive:
        result.styleDamageFrom = (StyleDamageSource)(
            result.styleDamageFrom | StyleDamageFromElementState);
        return element->state() & Node::NodeState::NodeStateActive;
    case CSSSelector::PseudoType::PseudoFocus:
        result.styleDamageFrom = (StyleDamageSource)(
            result.styleDamageFrom | StyleDamageFromElementState);
        return element->state() & Node::NodeState::NodeStateFocused;
    case CSSSelector::PseudoType::PseudoTarget:
        result.styleDamageFrom = (StyleDamageSource)(
            result.styleDamageFrom | StyleDamageFromElementState);
        return element->state() & Node::NodeState::NodeStateTarget;
    case CSSSelector::PseudoType::PseudoRoot:
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromDOMTree);
        return element == element->document()->documentElement();
    case CSSSelector::PseudoType::PseudoFirstChild:
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromDOMTree);
        return isFirstChild(element);
    case CSSSelector::PseudoType::PseudoLastChild:
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromDOMTree);
        return isLastChild(element);
    case CSSSelector::PseudoType::PseudoFirstOfType:
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromDOMTree);
        return element->parentElement() && isFirstOfType(element);
    case CSSSelector::PseudoType::PseudoLastOfType:
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromDOMTree);
        return element->parentElement() && isLastOfType(element);
    case CSSSelector::PseudoType::PseudoOnlyChild:
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromDOMTree);
        return isFirstChild(element) && isLastChild(element);
    case CSSSelector::PseudoType::PseudoOnlyOfType:
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromDOMTree);
        return element->parentElement() && isFirstOfType(element) &&
               isLastOfType(element);
    case CSSSelector::PseudoType::PseudoPlaceholderShown:
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromAttribute);
        return element->isHTMLElement() && element->isHTMLFormControl() &&
               element->asHTMLFormControl()->isPlaceholderVisible();
    case CSSSelector::PseudoType::PseudoEmpty:
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromDOMTree);
        return isEmpty(element);
    case CSSSelector::PseudoNthChild:
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromDOMTree);
        return element->parentElement() &&
               selector->matchNth(nthChildIndex(element));
    case CSSSelector::PseudoNthOfType:
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromDOMTree);
        return element->parentElement() &&
               selector->matchNth(nthOfTypeIndex(element));
    case CSSSelector::PseudoNthLastChild:
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromDOMTree);
        return element->parentElement() &&
               selector->matchNth(nthLastChildIndex(element));
    case CSSSelector::PseudoNthLastOfType:
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromDOMTree);
        return element->parentElement() &&
               selector->matchNth(nthLastOfTypeIndex(element));
    case CSSSelector::PseudoType::PseudoDir: {
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromAttribute);
        String* value = element->getDir();
        String* argument = selector->argument();
        if (value->equals(argument) ||
            (value->equals(String::emptyString) && argument->equals("ltr"))) {
            return true;
        }
        return false;
    }
    case CSSSelector::PseudoType::PseudoLang: {
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromAttribute);
        String* value = element->getLaunguage();
        String* argument = selector->argument();

        if (value->equals(String::emptyString) ||
            !value->startsWith(argument, false)) {
            break;
        }
        if (value->length() != argument->length() &&
            value->charAt(argument->length()) != '-') {
            break;
        }

        return true;
    }
    case CSSSelector::PseudoType::PseudoNot: {
        STARFISH_ASSERT(selector->pseudoSelectorList().size() == 1);
        result.styleDamageFrom = StyleDamageFromAll;
        AtomicString elementName = element->name().localNameAtomic();
        AtomicString elementId = element->atomicId();
        const GCVector<AtomicString>& elementClasses = element->classNames();
        return !checkOne(element, elementName, elementId, elementClasses,
                         selector->pseudoSelectorList()[0], result);
    }
    case CSSSelector::PseudoEnabled: {
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromAttribute);
        if (element->isHTMLAnchorElement()) {
            if (element->getAttribute(starFish()->staticStrings()->m_href)
                    .hasValue()) {
                return true;
            }
        } else if (element->isHTMLElement()) {
            if (!element->asHTMLElement()->disabled()) {
                return true;
            }
        }
        return false;
    }
    case CSSSelector::PseudoDisabled: {
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromAttribute);
        if (element->isHTMLElement()) {
            if (element->asHTMLElement()->disabled()) {
                return true;
            } else {
                return false;
            }
        }
        return false;
    }
    case CSSSelector::PseudoChecked: {
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromAttribute);
        if (element->isHTMLElement()) {
            if (element->isHTMLInputElement()) {
                if (element->asHTMLInputElement()->type()->equals("radio") ||
                    element->asHTMLInputElement()->type()->equals("checkbox")) {
                    if (element->asHTMLInputElement()->checked()) {
                        return true;
                    }
                }
            } else if (element->isHTMLOptionElement()) {
                if (element->asHTMLOptionElement()->selected()) {
                    return true;
                }
            }
        }
        return false;
    }
    default:
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        break;
    }
    return false;
}

bool StyleResolver::checkPseudoElement(Element* element,
                                       CSSPseudoSelector* selector,
                                       MatchResult& result)
{
    switch (selector->pseudoType()) {
    case CSSSelector::PseudoType::PseudoFirstLine:
        result.pseudoType = PseudoElementType::PseudoElementFirstLine;
        m_usesFirstLineRule = true;
        return true;
    case CSSSelector::PseudoType::PseudoFirstLetter:
        result.pseudoType = PseudoElementType::PseudoElementFirstLetter;
        return true;
    case CSSSelector::PseudoType::PseudoBefore:
        result.pseudoType = PseudoElementType::PseudoElementBefore;
        return true;
    case CSSSelector::PseudoType::PseudoAfter:
        result.pseudoType = PseudoElementType::PseudoElementAfter;
        return true;
    default:
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        return false;
    }
}

static ComputedStyleDamage resolveElementStyle(StyleResolveContext& ctx,
                                               StyleResolver* resolver,
                                               Element* element,
                                               ComputedStyle* parentStyle,
                                               bool inheritedStyleChanged)
{
    ComputedStyleDamage damage = ComputedStyleDamage::ComputedStyleDamageNone;

    if (element->needsStyleRecalc() || inheritedStyleChanged) {
        ComputedStyle* style =
            resolver->resolveStyle(ctx, element, parentStyle);
        bool damagedKeys[CSSStyleValuePair::KeyKindSize] = {
            false,
        };

        if (!element->style()) {
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::ComputedStyleDamageInherited |
                ComputedStyleDamage::ComputedStyleDamageRebuildFrame);
        } else {
            if (!element->frame()) {
                damage = (ComputedStyleDamage)(
                    ComputedStyleDamage::ComputedStyleDamageRebuildFrame);
            }
            damage = (ComputedStyleDamage)(
                damage | compareStyle(element->style(), style, damagedKeys));
        }

        if (damage & ComputedStyleDamage::ComputedStyleDamageInherited) {
            inheritedStyleChanged = inheritedStyleChanged | true;
        }

        Frame* oldFrame = element->frame();
        if (damage & ComputedStyleDamage::ComputedStyleDamageRebuildFrame) {
            if (style->display() != DisplayValue::NoneDisplayValue &&
                element->frame() == nullptr && element->parentElement()) {
                // special path for Node::appendChild

                Element* e = element->parentElement();
                while (e) {
                    if (e->style()->hasBlockLikeDisplay()) {
                        break;
                    }
                    e = e->parentElement();
                }

                if (e && e->frame() && !e->frame()->isFrameDocument()) {
                    e->window()->browsingContext()->setNeedsFrameTreeBuild();
                    FrameTreeBuilder::
                        needsFrameTreeBuildFromChildrenOfThisFrame(e->frame());
                }
            } else {
                element->setNeedsFrameTreeBuild();
            }
        }

        if (damage & ComputedStyleDamage::ComputedStyleDamageLayout) {
            element->setNeedsLayout();
        }

        if (damage & ComputedStyleDamage::
                         ComputedStyleDamageComputeStackingContextProperties) {
            element->webView()->setNeedsComputeStackingContextProperties();
        }

        if (damage & ComputedStyleDamage::ComputedStyleDamagePainting) {
            element->setNeedsPainting();
        }

        if (damage & ComputedStyleDamage::ComputedStyleDamageComposite) {
            element->setNeedsComposite();
        }

        ComputedStyle* oldStyle = element->style();
        element->setStyle(style);

        if (oldStyle && !style->transitionDuration().isZero() &&
            (damage != ComputedStyleDamage::ComputedStyleDamageNone)) {
            if (needsToApplyTransition(style, damagedKeys)) {
                if (!element->webView()->inRendering()) {
                    element->document()
                        ->animationExecutor()
                        ->addPendingAnimation(element, oldStyle, style,
                                              oldFrame);
                } else {
                    applyTransition(element, oldStyle, oldFrame, style,
                                    damagedKeys);
                }
            }
        } else if (element->webView()->inRendering()) {
            element->document()->animationExecutor()->cancelAnimation(element);
        }

        element->clearNeedsStyleRecalc();
    }

    return damage;
}

void StyleResolver::resolveChildrenStyle(StyleResolveContext& ctx,
                                         StyleResolver* resolver,
                                         Node* parentElement,
                                         ComputedStyle* parentElementStyle,
                                         bool inheritedStyleChanged)
{
    ComputedStyle* childTextNodeStyle = nullptr;
    bool inheritedStyleChangedForTextNode = inheritedStyleChanged;

    if (parentElement->isElement()) {
        ctx.m_ancestorSelectorFilter->pushElement(parentElement->asElement());
    }

    Node* child = parentElement->firstChild();
    while (child) {
        if (child->isElement()) {
            ComputedStyle* oldStyle = child->style();
            bool childIsHiddenBefore =
                oldStyle ? oldStyle->display() == NoneDisplayValue : true;
            auto damage =
                resolveElementStyle(ctx, resolver, child->asElement(),
                                    parentElementStyle, inheritedStyleChanged);
            if (child->style()->display() == DisplayValue::NoneDisplayValue) {
                child->asElement()->clearChildNeedsStyleRecalc();
            } else if (damage & ComputedStyleDamageInherited) {
                inheritedStyleChanged = true;
            } else if (childIsHiddenBefore) {
                inheritedStyleChanged = true;
            }
            if (oldStyle && oldStyle != child->style()) {
                ctx.pushIntoComputedStylePool(oldStyle);
            }

        } else {
            if (inheritedStyleChangedForTextNode || child->needsStyleRecalc()) {
                if (childTextNodeStyle == nullptr) {
                    childTextNodeStyle = new (ctx.allocateComputedStyle())
                        ComputedStyle(parentElementStyle);
                    childTextNodeStyle->loadResources(parentElement);
                    childTextNodeStyle->arrangeStyleValues(parentElementStyle,
                                                           child);
                }
                child->setStyle(childTextNodeStyle);
                child->clearNeedsStyleRecalc();
                child->clearChildNeedsStyleRecalc();

                if (!child->frame()) {
                    if (parentElement->frame() && parentElement->isElement()) {
                        Element* e = parentElement->asElement();
                        while (e) {
                            if (e->style()->hasBlockLikeDisplay()) {
                                break;
                            }
                            e = e->parentElement();
                        }
                        if (e && e->frame() && !e->frame()->isFrameDocument()) {
                            window()
                                ->browsingContext()
                                ->setNeedsFrameTreeBuild();
                            FrameTreeBuilder::
                                needsFrameTreeBuildFromChildrenOfThisFrame(
                                    e->frame());
                        }
                    }
                }
            }
        }
        STARFISH_ASSERT(!child->needsStyleRecalc());
        child = child->nextSibling();
    }

    child = parentElement->firstChild();
    while (child) {
        if (child->isElement() &&
            (child->childNeedsStyleRecalc() || inheritedStyleChanged)) {
            resolveChildrenStyle(ctx, resolver, child->asElement(),
                                 child->style(), inheritedStyleChanged);
        }
        child = child->nextSibling();
    }

#ifndef NDEBUG
    child = parentElement->firstChild();
    while (child) {
        STARFISH_ASSERT(!child->needsStyleRecalc());
        STARFISH_ASSERT(!child->childNeedsStyleRecalc());
        child = child->nextSibling();
    }
#endif

    parentElement->clearChildNeedsStyleRecalc();

    if (parentElement->isElement()) {
        ctx.m_ancestorSelectorFilter->popElement();
    }
}

void StyleResolver::resolveDOMStyle(Document* document, bool force)
{
    StyleResolveContext ctx(document);
    resolveChildrenStyle(ctx, &document->styleResolver(), document,
                         document->style(), force);
}

bool StyleResolver::tryAddSheet(Node* node, CSSStyleSheet* sheet)
{
    if (node->isHTMLElement()) {
        HTMLElement* htmlElement = node->asHTMLElement();
        CSSStyleSheet* nSheet = nullptr;
        if (htmlElement->isHTMLStyleElement()) {
            nSheet = htmlElement->asHTMLStyleElement()->generatedSheet();
        } else if (htmlElement->isHTMLLinkElement()) {
            nSheet = htmlElement->asHTMLLinkElement()->generatedSheet();
        }

        if (nSheet) {
            auto iter = std::find(m_sheets.begin(), m_sheets.end(), nSheet);
            STARFISH_ASSERT(iter != m_sheets.end());
            m_sheets.insert(iter, sheet);
            return true;
        }
    }
    return false;
}

bool StyleResolver::traverseAndTryAddSheet(Node* parent, CSSStyleSheet* sheet,
                                           bool& originFound)
{
    Node* child = parent->firstChild();
    while (child) {
        if (!originFound && child == sheet->origin()) {
            originFound = true;
        } else if (originFound) {
            if (tryAddSheet(child, sheet)) {
                return true;
            }
        } else {
            if (traverseAndTryAddSheet(child, sheet, originFound)) {
                return true;
            }
        }
        child = child->nextSibling();
    }

    return false;
}

void StyleResolver::addSheet(CSSStyleSheet* sheet)
{
    bool originFound = false;
    if (!traverseAndTryAddSheet(m_document, sheet, originFound)) {
        m_sheets.push_back(sheet);
    }
}

void StyleResolver::removeAllRules()
{
    m_ruleSet->clear();
    m_styleSheetWithAllRules = nullptr;
    m_ruleSetAttrFilter.clear();
}

static void extractValuesforSelector(const CSSSelector* selector,
                                     AtomicString& id, AtomicString& className,
                                     AtomicString& tagName)
{
    switch (selector->type()) {
    case CSSSelector::Id:
        id = selector->selectorText();
        break;
    case CSSSelector::Class:
        className = selector->selectorText();
        break;
    case CSSSelector::Tag:
        if (!selector->selectorText().string()->equals("*")) {
            tagName = selector->selectorText();
        }
        break;
    default:
        break;
    }
}

void StyleResolver::addToRuleSet(std::pair<StyleRule*, ResourceURL*> rule)
{
    rule.first->initFlagsRelatedWithSelectorList();
    auto selectorList = rule.first->selectorList();

    AtomicString id;
    AtomicString className;
    AtomicString tagName;

    unsigned size = selectorList.size();

    auto relation = selectorList[0]->relation();
    extractValuesforSelector(selectorList[0], id, className, tagName);

    unsigned i = 1;
    for (; i < size && relation == CSSSelector::SubSelector; i++) {
        relation = selectorList[i]->relation();
        extractValuesforSelector(selectorList[i], id, className, tagName);
    }

    for (i = 0; i < size; i++) {
        if (selectorList[i]->isAttributeSelector()) {
            if (!mayHaveAttrSelectorWithName(selectorList[i]
                                                 ->asCSSAttributeSelector()
                                                 ->attribute()
                                                 .localNameAtomic())) {
                m_ruleSetAttrFilter.push_back(selectorList[i]
                                                  ->asCSSAttributeSelector()
                                                  ->attribute()
                                                  .localNameAtomic());
            }
        }
    }

    if (!id.isEmptyAtomicString()) {
        m_ruleSet->idRules().insert(std::make_pair(id, rule));
        return;
    }
    if (!className.isEmptyAtomicString()) {
        m_ruleSet->classRules().insert(std::make_pair(className, rule));
        return;
    }
    if (!tagName.isEmptyAtomicString()) {
        m_ruleSet->tagRules().insert(std::make_pair(tagName, rule));
        return;
    }

    m_ruleSet->universalRules().insert(std::make_pair(AtomicString(), rule));
}

const MediaQueryEvaluator& StyleResolver::mediaQueryEvaluator()
{
    if (!m_mediaQueryEvaluator) {
        m_mediaQueryEvaluator = new MediaQueryEvaluator(
            String::fromUTF8("screen"), new MediaValues(document()->frame()));
    }
    return *m_mediaQueryEvaluator;
}

bool StyleResolver::mediaQueryAffectedByViewportChange()
{
    auto evaluator = mediaQueryEvaluator();
    auto results = viewportDependentMediaQueryResults();
    for (size_t i = 0; i < results.size(); i++) {
        if (evaluator.eval(results[i]->expression()) != results[i]->result()) {
            return true;
        }
    }
    return false;
}

bool StyleResolver::mediaQueryAffectedByDeviceChange()
{
    auto evaluator = mediaQueryEvaluator();
    auto results = deviceDependentMediaQueryResults();
    for (size_t i = 0; i < results.size(); i++) {
        if (evaluator.eval(results[i]->expression()) != results[i]->result()) {
            return true;
        }
    }
    return false;
}

bool CSSStyleValuePair::updateValueUnitColor(const CSSTokenValue& token)
{
    if (CSSPropertyParser::parseNonNamedColor(token, this)) {
        return true;
    } else {
        return CSSPropertyParser::parseNamedColor(token, this);
    }
}

bool CSSStyleValuePair::updateValueUnitBorderColor(const CSSTokenValue& token)
{
    return updateValueUnitColor(token);
}

bool CSSStyleValuePair::updateValueUnitMargin(const CSSTokenValue& value)
{
    return updateValueUnitLengthOrCalc(value,
                                       CSSPropertyParser::AllowNegative |
                                           CSSPropertyParser::AllowPercent |
                                           CSSPropertyParser::AllowAuto);
}

bool CSSStyleValuePair::updateValueUnitPadding(const CSSTokenValue& value)
{
    return updateValueUnitLengthOrCalc(CSSTokenValue(value),
                                       CSSPropertyParser::AllowPercent);
}

bool CSSStyleValuePair::updateValueColor(Document* document,
                                         const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    return updateValueUnitColor(tokens[0]);
}

bool CSSStyleValuePair::updateValueBackgroundColor(Document* document,
                                                   const CSSTokenVector& tokens)
{
    return updateValueColor(document, tokens);
}

bool CSSStyleValuePair::updateValueCaretColor(Document* document,
                                              const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::ColorValueKind;
    if (STRING_VALUE_IS_STRING("auto")) {
        CSSTokenValue token("currentcolor");
        return CSSPropertyParser::parseNamedColor(token, this);
    } else if (STRING_VALUE_IS_STRING("transparent")) {
        m_value.m_color = Unit::Color(0, 0, 0, 0);
        return true;
    } else {
        return updateValueColor(document, tokens);
    }

    return false;
}

#define UPDATE_VALUE_BORDER_COLOR(POS, ...)                \
    bool CSSStyleValuePair::updateValueBorder##POS##Color( \
        Document* document, const CSSTokenVector& tokens)  \
    {                                                      \
        return updateValueColor(document, tokens);         \
    }
GEN_FOURSIDE(UPDATE_VALUE_BORDER_COLOR)
#undef UPDATE_VALUE_BORDER_COLOR

bool CSSStyleValuePair::updateValueUnitBorderStyle(const CSSTokenValue& value)
{
    m_valueKind = CSSStyleValuePair::ValueKind::BorderStyleValueKind;
    if (STRING_VALUE_IS_STRING("none")) {
        m_value.m_borderStyle = BorderStyleValue::NoneBorderStyleValue;
    } else if (STRING_VALUE_IS_STRING("hidden")) {
        m_value.m_borderStyle = BorderStyleValue::HiddenBorderStyleValue;
    } else if (STRING_VALUE_IS_STRING("solid") ||
               STRING_VALUE_IS_STRING("auto")) {
        m_value.m_borderStyle = BorderStyleValue::SolidBorderStyleValue;
    } else if (STRING_VALUE_IS_STRING("dashed")) {
        m_value.m_borderStyle = BorderStyleValue::DashedBorderStyleValue;
    } else if (STRING_VALUE_IS_STRING("inset")) {
        m_value.m_borderStyle = BorderStyleValue::InsetBorderStyleValue;
    } else if (STRING_VALUE_IS_STRING("outset")) {
        m_value.m_borderStyle = BorderStyleValue::OutsetBorderStyleValue;
    } else if (STRING_VALUE_IS_STRING("dotted")) {
        m_value.m_borderStyle = BorderStyleValue::DottedBorderStyleValue;
    } else if (STRING_VALUE_IS_STRING("double")) {
        m_value.m_borderStyle = BorderStyleValue::DoubleBorderStyleValue;
    } else if (STRING_VALUE_IS_STRING("groove")) {
        m_value.m_borderStyle = BorderStyleValue::GrooveBorderStyleValue;
    } else if (STRING_VALUE_IS_STRING("ridge")) {
        m_value.m_borderStyle = BorderStyleValue::RidgeBorderStyleValue;
    } else {
        return false;
    }
    return true;
}

#define UPDATE_VALUE_BORDER_STYLE(POS, ...)                \
    bool CSSStyleValuePair::updateValueBorder##POS##Style( \
        Document* document, const CSSTokenVector& tokens)  \
    {                                                      \
        if (tokens.size() != 1) {                          \
            return false;                                  \
        }                                                  \
        return updateValueUnitBorderStyle(tokens[0]);      \
    }
GEN_FOURSIDE(UPDATE_VALUE_BORDER_STYLE)
#undef UPDATE_VALUE_BORDER_STYLE

bool CSSStyleValuePair::updateValueBorderTopLeftRadius(
    Document* document, const CSSTokenVector& tokens)
{
    return updateBorderRadiusValue(this, tokens);
}

bool CSSStyleValuePair::updateValueBorderTopRightRadius(
    Document* document, const CSSTokenVector& tokens)
{
    return updateBorderRadiusValue(this, tokens);
}

bool CSSStyleValuePair::updateValueBorderBottomLeftRadius(
    Document* document, const CSSTokenVector& tokens)
{
    return updateBorderRadiusValue(this, tokens);
}

bool CSSStyleValuePair::updateValueBorderBottomRightRadius(
    Document* document, const CSSTokenVector& tokens)
{
    return updateBorderRadiusValue(this, tokens);
}

bool CSSStyleValuePair::updateValueDirection(Document* document,
                                             const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::DirectionValueKind;
    if (STRING_VALUE_IS_STRING("ltr")) {
        m_value.m_direction = DirectionValue::LtrDirectionValue;
    } else if (STRING_VALUE_IS_STRING("rtl")) {
        m_value.m_direction = DirectionValue::RtlDirectionValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueWhiteSpace(Document* document,
                                              const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::WhiteSpaceValueKind;
    if (STRING_VALUE_IS_STRING("normal")) {
        m_value.m_whiteSpace = WhiteSpaceValue::NormalWhiteSpaceValue;
    } else if (STRING_VALUE_IS_STRING("nowrap")) {
        m_value.m_whiteSpace = WhiteSpaceValue::NoWrapWhiteSpaceValue;
    } else if (STRING_VALUE_IS_STRING("pre")) {
        m_value.m_whiteSpace = WhiteSpaceValue::PreWhiteSpaceValue;
    } else if (STRING_VALUE_IS_STRING("pre-wrap")) {
        m_value.m_whiteSpace = WhiteSpaceValue::PreWrapWhiteSpaceValue;
    } else if (STRING_VALUE_IS_STRING("pre-line")) {
        m_value.m_whiteSpace = WhiteSpaceValue::PreLineWhiteSpaceValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueObjectFit(Document* document,
                                             const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::ObjectFitValueKind;
    if (STRING_VALUE_IS_STRING("fill")) {
        m_value.m_objectFit = ObjectFitValue::FillObjectFitValue;
    } else if (STRING_VALUE_IS_STRING("contain")) {
        m_value.m_objectFit = ObjectFitValue::ContainObjectFitValue;
    } else if (STRING_VALUE_IS_STRING("cover")) {
        m_value.m_objectFit = ObjectFitValue::CoverObjectFitValue;
    } else if (STRING_VALUE_IS_STRING("none")) {
        m_value.m_objectFit = ObjectFitValue::NoneObjectFitValue;
    } else if (STRING_VALUE_IS_STRING("scale-down")) {
        m_value.m_objectFit = ObjectFitValue::ScaledownObjectFitValue;
    } else {
        return false;
    }
    return true;
}

static bool isPositionValue(const CSSTokenVector& tokens, unsigned int idx,
                            bool& isXAxis, bool& isYAxis,
                            CSSStyleValuePair& result)
{
    if (tokens.size() <= idx) {
        return false;
    }
    const CSSTokenValue& value = tokens[idx];
    result.setValueKind(CSSStyleValuePair::ValueKind::SideValueKind);
    if (STRING_VALUE_IS_STRING("left")) {
        if (isXAxis) {
            return false;
        }
        result.setValue(SideValue::LeftSideValue);
        isXAxis = true;
    } else if (STRING_VALUE_IS_STRING("right")) {
        if (isXAxis) {
            return false;
        }
        result.setValue(SideValue::RightSideValue);
        isXAxis = true;
    } else if (STRING_VALUE_IS_STRING("center")) {
        result.setValue(SideValue::CenterSideValue);
    } else if (STRING_VALUE_IS_STRING("top")) {
        if (isYAxis) {
            return false;
        }
        result.setValue(SideValue::TopSideValue);
        isYAxis = true;
    } else if (STRING_VALUE_IS_STRING("bottom")) {
        if (isYAxis) {
            return false;
        }
        result.setValue(SideValue::BottomSideValue);
        isYAxis = true;
    } else {
        uint8_t option =
            CSSPropertyParser::AllowPercent | CSSPropertyParser::AllowNegative;
        return result.updateValueUnitLengthOrCalc(value, option);
    }
    return true;
}

static bool isSideTokenValue(const CSSTokenValue& value)
{
    return value.equals("left") || value.equals("right") ||
           value.equals("center") || value.equals("top") ||
           value.equals("bottom");
}

static bool isVerticalValue(const CSSStyleValuePair& value)
{
    if (!value.isSideValueKind()) {
        return false;
    }
    return value.sideValue() == SideValue::TopSideValue ||
           value.sideValue() == SideValue::BottomSideValue;
}

static bool isCenterValue(const CSSStyleValuePair& value)
{
    if (!value.isSideValueKind()) {
        return false;
    }
    return value.sideValue() == SideValue::CenterSideValue;
}

static bool isHorizontalValue(const CSSStyleValuePair& value)
{
    if (!value.isSideValueKind()) {
        return false;
    }
    return value.sideValue() == SideValue::LeftSideValue ||
           value.sideValue() == SideValue::RightSideValue;
}

static bool updatePositionValue(const CSSStyleValuePair& value,
                                CSSStyleValuePair& xPair,
                                CSSStyleValuePair& yPair)
{
    bool isYValue = isVerticalValue(value);
    xPair = value;
    yPair.setValueKind(CSSStyleValuePair::ValueKind::SideValueKind);
    yPair.setValue(SideValue::CenterSideValue);
    if (isYValue) {
        std::swap(xPair, yPair);
    }
    return true;
}

static bool updatePositionValue(const CSSStyleValuePair& value1,
                                const CSSStyleValuePair& value2,
                                CSSStyleValuePair& xPair,
                                CSSStyleValuePair& yPair)
{
    bool isXY = isHorizontalValue(value1) || isVerticalValue(value2) ||
                !value1.isSideValueKind() || !value2.isSideValueKind();
    bool isYX = isVerticalValue(value1) || isHorizontalValue(value2);

    if (isXY && isYX) {
        return false;
    }
    xPair = value1;
    yPair = value2;
    if (isYX) {
        std::swap(xPair, yPair);
    }
    return true;
}

static bool updatePositionValue(const GCVector<CSSStyleValuePair>& values,
                                CSSStyleValuePair& xPair,
                                CSSStyleValuePair& yPair)
{
    CSSStyleValuePair center;
    for (unsigned int i = 0; i < values.size(); i++) {
        CSSStyleValuePair current = values[i];

        if (current.valueKind() == CSSStyleValuePair::ValueKind::None) {
            break;
        }

        if (!current.isSideValueKind()) {
            return false;
        }

        SideValue id = current.sideValue();

        if (isCenterValue(current)) {
            if (center.isSideValueKind()) {
                return false;
            }
            center = current;
            continue;
        }

        CSSStyleValuePair result;
        if (i + 1 < values.size() &&
            values[i + 1].valueKind() != CSSStyleValuePair::ValueKind::None &&
            !values[i + 1].isSideValueKind()) {
            CSSStyleValuePair second = values[++i];
            ValuePair* pair = new ValuePair(current, second);
            result.setValuePair(pair);
        } else {
            result = current;
        }

        if (isHorizontalValue(current)) {
            if (xPair.valueKind() != CSSStyleValuePair::ValueKind::None) {
                return false;
            }
            xPair = result;
        } else {
            STARFISH_ASSERT(isVerticalValue(current));
            if (yPair.valueKind() != CSSStyleValuePair::ValueKind::None) {
                return false;
            }
            yPair = result;
        }
    }

    if (center.isSideValueKind()) {
        if (xPair.valueKind() != CSSStyleValuePair::ValueKind::None &&
            yPair.valueKind() != CSSStyleValuePair::ValueKind::None) {
            return false;
        }
        if (xPair.valueKind() == CSSStyleValuePair::ValueKind::None) {
            xPair.setValueKind(CSSStyleValuePair::ValueKind::SideValueKind);
            xPair.setValue(SideValue::CenterSideValue);
        } else {
            yPair.setValueKind(CSSStyleValuePair::ValueKind::SideValueKind);
            yPair.setValue(SideValue::CenterSideValue);
        }
    }

    return true;
}

bool CSSStyleValuePair::updateValueObjectPosition(Document* document,
                                                  const CSSTokenVector& tokens)
{
    CSSStyleValuePair xPair;
    CSSStyleValuePair yPair;

    if (updateValueObjectPosition(tokens, xPair, yPair)) {
        m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
        m_value.m_multiValue =
            new ValueList(ValueList::Separator::SpaceSeparator);
        m_value.m_multiValue->push_back(xPair);
        m_value.m_multiValue->push_back(yPair);
        return true;
    }
    return false;
}
bool CSSStyleValuePair::updateValueObjectPosition(const CSSTokenVector& tokens,
                                                  CSSStyleValuePair& xPair,
                                                  CSSStyleValuePair& yPair)
{
    if (tokens.size() < 0 || tokens.size() > 4) {
        return false;
    }

    bool isXAxis = false;
    bool isYAxis = false;

    CSSStyleValuePair value1;
    bool ret = isPositionValue(tokens, 0, isXAxis, isYAxis, value1);
    if (!ret) {
        return false;
    }

    CSSStyleValuePair value2;
    ret = isPositionValue(tokens, 1, isXAxis, isYAxis, value2);
    if (!ret) {
        return updatePositionValue(value1, xPair, yPair);
    }

    CSSStyleValuePair value3;
    ret = false;
    if (value1.isSideValueKind() &&
        ((value2.isSideValueKind()) != isSideTokenValue(tokens[2])) &&
        (!isCenterValue(value2.isSideValueKind() ? value2 : value1))) {
        ret = isPositionValue(tokens, 2, isXAxis, isYAxis, value3);
    }

    if (!ret) {
        if (isYAxis && !value2.isSideValueKind()) {
            return updatePositionValue(value1, xPair, yPair);
        }
        return updatePositionValue(value1, value2, xPair, yPair);
    }

    CSSStyleValuePair value4;
    ret = false;
    if (value3.isSideValueKind() && !isCenterValue(value3) &&
        !isSideTokenValue(tokens[3])) {
        ret = isPositionValue(tokens, 3, isXAxis, isYAxis, value4);
    }

    if (!ret) {
        if (tokens.size() < 3) {
            if (isYAxis && !value2.isSideValueKind()) {
                return updatePositionValue(value1, xPair, yPair);
            }
            return updatePositionValue(value1, value2, xPair, yPair);
        }
    }

    CSSStyleValuePair value5;
    GCVector<CSSStyleValuePair> values;
    values.push_back(value1);
    values.push_back(value2);
    values.push_back(value3);
    values.push_back(value4);

    return updatePositionValue(values, xPair, yPair);
}

bool CSSStyleValuePair::updateValueWordSpacing(Document* document,
                                               const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    return updateValueUnitWordSpacing(tokens[0]);
}

bool CSSStyleValuePair::updateValueUnitWordSpacing(const CSSTokenValue& value)
{
    // <normal> | length | initial | inherit
    float result = 0.f;
    if (STRING_VALUE_IS_STRING("normal")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Normal;
        return true;
    } else {
        return updateValueUnitLengthOrCalc(value,
                                           CSSPropertyParser::AllowNegative);
    }
}

bool CSSStyleValuePair::updateValueLetterSpacing(Document* document,
                                                 const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    // <normal> | length | initial | inherit
    const CSSTokenValue& value = tokens[0];
    float result = 0.f;
    if (STRING_VALUE_IS_STRING("normal")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Normal;
        return true;
    } else {
        return updateValueUnitLengthOrCalc(value,
                                           CSSPropertyParser::AllowNegative);
    }
}

bool CSSStyleValuePair::updateValueDisplay(Document* document,
                                           const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::DisplayValueKind;
    if (STRING_VALUE_IS_STRING("block")) {
        m_value.m_display = DisplayValue::BlockDisplayValue;
    } else if (STRING_VALUE_IS_STRING("inline")) {
        m_value.m_display = DisplayValue::InlineDisplayValue;
    } else if (STRING_VALUE_IS_STRING("inline-list-item")) {
        m_value.m_display = DisplayValue::InlineListItemDisplayValue;
    } else if (STRING_VALUE_IS_STRING("list-item")) {
        m_value.m_display = DisplayValue::ListItemDisplayValue;
    } else if (STRING_VALUE_IS_STRING("inline-block")) {
        m_value.m_display = DisplayValue::InlineBlockDisplayValue;
    } else if (STRING_VALUE_IS_STRING("table")) {
        m_value.m_display = DisplayValue::TableDisplayValue;
    } else if (STRING_VALUE_IS_STRING("inline-table")) {
        m_value.m_display = DisplayValue::InlineTableDisplayValue;
    } else if (STRING_VALUE_IS_STRING("table-row-group")) {
        m_value.m_display = DisplayValue::TableRowGroupDisplayValue;
    } else if (STRING_VALUE_IS_STRING("table-header-group")) {
        m_value.m_display = DisplayValue::TableHeaderGroupDisplayValue;
    } else if (STRING_VALUE_IS_STRING("table-footer-group")) {
        m_value.m_display = DisplayValue::TableFooterGroupDisplayValue;
    } else if (STRING_VALUE_IS_STRING("table-row")) {
        m_value.m_display = DisplayValue::TableRowDisplayValue;
    } else if (STRING_VALUE_IS_STRING("table-column-group")) {
        m_value.m_display = DisplayValue::TableColumnGroupDisplayValue;
    } else if (STRING_VALUE_IS_STRING("table-column")) {
        m_value.m_display = DisplayValue::TableColumnDisplayValue;
    } else if (STRING_VALUE_IS_STRING("table-cell")) {
        m_value.m_display = DisplayValue::TableCellDisplayValue;
    } else if (STRING_VALUE_IS_STRING("table-caption")) {
        m_value.m_display = DisplayValue::TableCaptionDisplayValue;
    } else if (STRING_VALUE_IS_STRING("flex")) {
        m_value.m_display = DisplayValue::FlexDisplayValue;
    } else if (STRING_VALUE_IS_STRING("inline-flex")) {
        m_value.m_display = DisplayValue::InlineFlexDisplayValue;
    } else if (STRING_VALUE_IS_STRING("grid")) {
        m_value.m_display = DisplayValue::GridDisplayValue;
    } else if (STRING_VALUE_IS_STRING("inline-grid")) {
        m_value.m_display = DisplayValue::InlineGridDisplayValue;
    } else if (STRING_VALUE_IS_STRING("none")) {
        m_value.m_display = DisplayValue::NoneDisplayValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueFloat(Document* document,
                                         const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::FloatValueKind;
    if (STRING_VALUE_IS_STRING("none")) {
        m_value.m_float = FloatValue::NoneFloatValue;
    } else if (STRING_VALUE_IS_STRING("left")) {
        m_value.m_float = FloatValue::LeftFloatValue;
    } else if (STRING_VALUE_IS_STRING("right")) {
        m_value.m_float = FloatValue::RightFloatValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueClear(Document* document,
                                         const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::ClearValueKind;
    if (STRING_VALUE_IS_STRING("none")) {
        m_value.m_clear = ClearValue::NoneClearValue;
    } else if (STRING_VALUE_IS_STRING("left")) {
        m_value.m_clear = ClearValue::LeftClearValue;
    } else if (STRING_VALUE_IS_STRING("right")) {
        m_value.m_clear = ClearValue::RightClearValue;
    } else if (STRING_VALUE_IS_STRING("both")) {
        m_value.m_clear = ClearValue::BothClearValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueFontStyle(Document* document,
                                             const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    return updateValueUnitFontStyle(tokens[0]);
}

bool CSSStyleValuePair::updateValueFontKerning(Document* document,
                                               const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::FontKerningValueKind;
    if (STRING_VALUE_IS_STRING("auto")) {
        m_value.m_fontKerning = FontKerningValue::FontKerningAutoValue;
    } else if (STRING_VALUE_IS_STRING("normal")) {
        m_value.m_fontKerning = FontKerningValue::FontKerningNormalValue;
    } else if (STRING_VALUE_IS_STRING("none")) {
        m_value.m_fontKerning = FontKerningValue::FontKerningNoneValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueUnitFontStyle(const CSSTokenValue& value)
{
    m_valueKind = CSSStyleValuePair::ValueKind::FontStyleValueKind;
    if (STRING_VALUE_IS_STRING("normal")) {
        m_value.m_fontStyle = FontStyleValue::NormalFontStyleValue;
    } else if (STRING_VALUE_IS_STRING("italic")) {
        m_value.m_fontStyle = FontStyleValue::ItalicFontStyleValue;
    } else if (STRING_VALUE_IS_STRING("oblique")) {
        m_value.m_fontStyle = FontStyleValue::ObliqueFontStyleValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueUnitBackgroundRepeat(
    const CSSTokenValue& value)
{
    m_valueKind = CSSStyleValuePair::ValueKind::BackgroundRepeatValueKind;
    if (STRING_VALUE_IS_STRING("no-repeat")) {
        m_value.m_backgroundRepeat = BackgroundRepeatValue::NoRepeatRepeatValue;
    } else if (STRING_VALUE_IS_STRING("repeat")) {
        m_value.m_backgroundRepeat = BackgroundRepeatValue::RepeatRepeatValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueBackgroundRepeatX(
    Document* document, const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitBackgroundRepeat(tokens[0]);
}

bool CSSStyleValuePair::updateValueBackgroundRepeatY(
    Document* document, const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitBackgroundRepeat(tokens[0]);
}

bool CSSStyleValuePair::updateValueUnitUrlOrNone(const CSSTokenValue& value)
{
    if (STRING_VALUE_IS_NONE()) {
        m_valueKind = CSSStyleValuePair::ValueKind::None;
    } else {
        return CSSPropertyParser::parseUrl(value.data(), this);
    }

    return true;
}

bool CSSStyleValuePair::updateValueUnitGradient(const CSSTokenValue& value)
{
    auto ss = value.trim();
    CSSPropertyParser parser((char*)ss.data(), ss.length());
    parser.consumeString(CSSPropertyParser::AllowNegative);
    String* type = parser.parsedString();
    if (type->equals("linear-gradient") && parser.consumeIfNext('(')) {
        // linear-gradient() = linear-gradient(
        //   [ <angle> | to <side-or-corner> ]?
        //   <color-stop-list>
        // )
        CSSAngle angle;
        CSSStyleValuePair s;
        parser.consumeWhitespaces();
        parser.consumeString(CSSPropertyParser::AllowNegative);
        String* str = parser.parsedString();
        // TODO: Parse 'to <side-or-corner>' and <color-stop-list>
        if (parser.parseAngle(str->toUTF8NonGCString().c_str(),
                              CSSPropertyParser::AllowNegative, &s)) {
            angle = s.angleValue();
        } else {
            // Set the default value
            angle = CSSAngle(180);
        }
        CSSLinearGradientValue* linearGradientValue =
            new CSSLinearGradientValue(angle);
        m_value.m_gradientValue = linearGradientValue;
    } else if (type->equals("radial-gradient") && parser.consumeIfNext('(')) {
        // TODO: Consider the radial gradient
        return false;
    } else {
        return false;
    }
    m_valueKind = CSSStyleValuePair::ValueKind::GradientValueKind;
    return true;
}

bool CSSStyleValuePair::updateValueBackgroundImage(const CSSTokenVector& tokens,
                                                   bool allowComma)
{
    bool shouldBeComma = false;
    ValueList* values = new ValueList(ValueList::Separator::CommaSeparator);
    for (unsigned int i = 0; i < tokens.size(); i++) {
        const CSSTokenValue& value = tokens[i];
        if (value.equals(",")) {
            if (!allowComma || !shouldBeComma) {
                return false;
            }
            shouldBeComma = false;
            continue;
        }
        CSSStyleValuePair ret;
        if (shouldBeComma || (!ret.updateValueUnitUrlOrNone(value) &&
                              !ret.updateValueUnitGradient(value))) {
            return false;
        }
        shouldBeComma = true;
        values->push_back(ret);
    }
    m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
    m_value.m_multiValue = values;
    return shouldBeComma;
}

bool CSSStyleValuePair::updateValueBackgroundImage(Document* document,
                                                   const CSSTokenVector& tokens)
{
    return updateValueBackgroundImage(tokens, true);
}

bool CSSStyleValuePair::updateValueCursor(Document* document,
                                          const CSSTokenVector& tokens)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return true;
}

static bool parseCounter(Document* document, const CSSTokenValue& s,
                         CSSStyleValuePair* pair)
{
    auto ss = s.trim();
    CSSPropertyParser parser((char*)ss.data(), ss.length());
    parser.consumeString(0);
    if (!parser.parsedString()->equals("counter")) {
        return false;
    }
    if (!parser.consumeIfNext('(')) {
        return false;
    }
    // Argument: name
    parser.consumeWhitespaces();
    parser.consumeString(CSSPropertyParser::AllowNegative |
                         CSSPropertyParser::AllowUnderline);
    String* name = parser.parsedString();
    if (!CSSPropertyParser::stringIsIdent(name)) {
        return false;
    }
    AtomicString aname =
        AtomicString::createAtomicString(document->starFish(), name);
    parser.consumeWhitespaces();
    if (parser.consumeIfNext(')') && parser.isEnd()) {
        pair->setCounterFunctionValue(new CSSCounterFunction(aname));
        return true;
    }
    // Argument: style
    if (!parser.consumeIfNext(',')) {
        return false;
    }
    parser.consumeWhitespaces();
    parser.consumeString(CSSPropertyParser::AllowNegative |
                         CSSPropertyParser::AllowUnderline);
    String* style = parser.parsedString();
    if (!CSSPropertyParser::stringIsIdent(style)) {
        return false;
    }
    AtomicString astyle =
        AtomicString::createAtomicString(document->starFish(), style);
    parser.consumeWhitespaces();
    if (!parser.consumeIfNext(')') || !parser.isEnd()) {
        return false;
    }
    auto value = new CSSCounterFunction(aname);
    value->setStyle(astyle);
    pair->setCounterFunctionValue(value);
    return true;
}

static bool parseCounters(Document* document, const CSSTokenValue& s,
                          CSSStyleValuePair* pair)
{
    auto ss = s.trim();
    CSSPropertyParser parser((char*)ss.data(), ss.length());
    parser.consumeString(0);
    if (!parser.parsedString()->equals("counters")) {
        return false;
    }
    if (!parser.consumeIfNext('(')) {
        return false;
    }
    // Argument: name
    parser.consumeWhitespaces();
    parser.consumeString(CSSPropertyParser::AllowNegative |
                         CSSPropertyParser::AllowUnderline);
    String* name = parser.parsedString();
    if (!CSSPropertyParser::stringIsIdent(name)) {
        return false;
    }
    AtomicString aname =
        AtomicString::createAtomicString(document->starFish(), name);
    parser.consumeWhitespaces();
    // Argument: separator
    if (!parser.consumeIfNext(',')) {
        return false;
    }
    if (!parser.consumeContentString()) {
        return false;
    }
    String* separator = parser.parsedString();
    parser.consumeWhitespaces();
    if (parser.consumeIfNext(')') && parser.isEnd()) {
        auto value = new CSSCounterFunction(aname);
        value->setSeparator(separator);
        pair->setCounterFunctionValue(value);
        return true;
    }
    // Argument: style
    if (!parser.consumeIfNext(',')) {
        return false;
    }
    parser.consumeWhitespaces();
    parser.consumeString(CSSPropertyParser::AllowNegative |
                         CSSPropertyParser::AllowUnderline);
    String* style = parser.parsedString();
    if (!CSSPropertyParser::stringIsIdent(style)) {
        return false;
    }
    AtomicString astyle =
        AtomicString::createAtomicString(document->starFish(), style);
    parser.consumeWhitespaces();
    if (!parser.consumeIfNext(')') || !parser.isEnd()) {
        return false;
    }
    auto value = new CSSCounterFunction(aname);
    value->setSeparator(separator);
    value->setStyle(astyle);
    pair->setCounterFunctionValue(value);
    return true;
}

bool CSSStyleValuePair::updateValueContent(Document* document,
                                           const CSSTokenVector& tokens)
{
    ValueList* values = new ValueList(ValueList::SpaceSeparator);
    for (unsigned int i = 0; i < tokens.size(); i++) {
        const CSSTokenValue& value = tokens[i];
        CSSStyleValuePair ret;
        if (!ret.updateValueUnitUrlOrNone(value)) {
            CSSPropertyParser parser((char*)tokens[i].data());
            if (value.equals("normal")) {
                ret.m_valueKind = CSSStyleValuePair::ValueKind::Normal;
            } else if (parser.parseContentString(
                           value.data(), value.length(),
                           &(ret.m_value.m_stringValue))) {
                ret.m_valueKind = CSSStyleValuePair::ValueKind::StringValueKind;
            } else if (parser.parseAttr(value.data(), value.length(),
                                        &(ret.m_value.m_stringValue))) {
                ret.m_valueKind = CSSStyleValuePair::ValueKind::Attr;
            } else if (!parseCounter(document, value, &ret) &&
                       !parseCounters(document, value, &ret)) {
                // TODO: Consider various value types of the 'content' property.
                // https://www.w3.org/TR/CSS2/generate.html#content
                STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
                return false;
            }
        }
        values->push_back(ret);
    }

    m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
    m_value.m_multiValue = values;
    return true;
}

bool CSSStyleValuePair::updateValueBorderImageRepeat(
    Document* document, const CSSTokenVector& tokens)
{
    return updateValueUnitBorderImageRepeat(tokens);
}

bool CSSStyleValuePair::updateValueUnitBorderImageRepeat(
    const CSSTokenVector& tokens)
{
    size_t len = tokens.size();
    if (len < 1 || len > 2) {
        return false;
    }
    ValueList* values = new ValueList(ValueList::SpaceSeparator);
    for (size_t i = 0; i < tokens.size(); i++) {
        const CSSTokenValue& value = tokens[i];
        CSSStyleValuePair pair;
        if (STRING_VALUE_IS_STRING("stretch")) {
            pair.setBorderImageRepeatValue(
                BorderImageRepeatValue::StretchValue);
        } else if (STRING_VALUE_IS_STRING("repeat")) {
            pair.setBorderImageRepeatValue(BorderImageRepeatValue::RepeatValue);
        } else if (STRING_VALUE_IS_STRING("round")) {
            pair.setBorderImageRepeatValue(BorderImageRepeatValue::RoundValue);
        } else if (STRING_VALUE_IS_STRING("space")) {
            pair.setBorderImageRepeatValue(BorderImageRepeatValue::SpaceValue);
        } else {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            return false;
        }
        values->push_back(pair);
    }

    setValueList(values);

    return true;
}

bool CSSStyleValuePair::updateValueBorderImageSource(
    Document* document, const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    return updateValueUnitBorderImageSource(tokens[0]);
}

bool CSSStyleValuePair::updateValueUnitBorderImageSource(
    const CSSTokenValue& value)
{
    if (updateValueUnitUrlOrNone(value) || updateValueUnitGradient(value)) {
        return true;
    }
    return false;
}

bool CSSStyleValuePair::updateValueUnitBorderWidth(const CSSTokenValue& value)
{
    m_valueKind = CSSStyleValuePair::ValueKind::BorderWidthValueKind;
    if (STRING_VALUE_IS_STRING("thick")) {
        m_value.m_borderWidth = BorderWidthValue::ThickBorderWidthValue;
    } else if (STRING_VALUE_IS_STRING("thin")) {
        m_valueKind = CSSStyleValuePair::ValueKind::BorderWidthValueKind;
        m_value.m_borderWidth = BorderWidthValue::ThinBorderWidthValue;
    } else if (STRING_VALUE_IS_STRING("medium")) {
        m_valueKind = CSSStyleValuePair::ValueKind::BorderWidthValueKind;
        m_value.m_borderWidth = BorderWidthValue::MediumBorderWidthValue;
    } else {
        return CSSPropertyParser::parseLength(value.c_str(), 0, this);
    }
    return true;
}

#define UPDATE_VALUE_BORDER_WIDTH(POS, ...)                \
    bool CSSStyleValuePair::updateValueBorder##POS##Width( \
        Document* document, const CSSTokenVector& tokens)  \
    {                                                      \
        if (tokens.size() != 1) {                          \
            return false;                                  \
        }                                                  \
        return updateValueUnitBorderWidth(tokens[0]);      \
    }
GEN_FOURSIDE(UPDATE_VALUE_BORDER_WIDTH)
#undef UPDATE_VALUE_BORDER_WIDTH

bool CSSStyleValuePair::updateValueNumber(const CSSTokenVector& tokens,
                                          uint8_t option)
{
    if (tokens.size() != 1) {
        return false;
    }

    return updateValueUnitNumber(tokens[0], option);
}

bool CSSStyleValuePair::updateValueUnitNumber(const CSSTokenValue& token,
                                              uint8_t option)
{
    float f;
    m_valueKind = CSSStyleValuePair::ValueKind::Number;
    if (CSSPropertyParser::parseNumber(token.data(), option, &f)) {
        m_value.m_floatValue = f;
        return true;
    }
    return false;
}

bool CSSStyleValuePair::updateValueUnitCalc(const CSSTokenValue& token,
                                            uint8_t calcParserOption,
                                            uint8_t parserOption)
{
    bool isLenParser =
        (calcParserOption == CSSStyleValuePair::CalcParserOption::LengthParser);
    bool isAngleParser =
        (calcParserOption == CSSStyleValuePair::CalcParserOption::AngleParser);
    bool isTimeParser =
        (calcParserOption == CSSStyleValuePair::CalcParserOption::TimeParser);

    CSSPropertyParser parser((char*)token.data());

    parser.consumeString(0);
    if (parser.parsedString()->equals("calc")) {
        if (!parser.consumeIfNext('(')) {
            return false;
        }

        CalcData* data = new CalcData();
        bool isPlus = true;
        while (true) {
            CalcTerm* term = new CalcTerm();
            bool isMul = false;
            bool unitParsed = false;
            while (!parser.isEnd()) {
                parser.consumeWhitespaces();

                CSSStyleValuePair ret;
                char* pos = parser.curPos();
                parser.consumeString(CSSPropertyParser::AllowDot |
                                     CSSPropertyParser::AllowPlus |
                                     CSSPropertyParser::AllowNegative |
                                     CSSPropertyParser::AllowPercent);
                auto str = parser.parsedString()->toUTF8NonGCString();
                CalcValue val;
                if (isLenParser &&
                    ret.updateValueUnitLength(CSSTokenValue(str.c_str()),
                                              parserOption)) {
                    if (unitParsed) {
                        return false;
                    }
                    unitParsed = true;
                    if (isPlus) {
                        if (ret.valueKind() ==
                            CSSStyleValuePair::ValueKind::Percentage) {
                            val.setType(CalcValueType::Percentage);
                            val.setValue(ret.percentageValue());
                        } else {
                            val.setType(CalcValueType::Length);
                            val.setValue(ret.cssLengthValue());
                        }
                    } else {
                        if (ret.valueKind() ==
                            CSSStyleValuePair::ValueKind::Percentage) {
                            val.setType(CalcValueType::Percentage);
                            val.setValue(-1 * ret.percentageValue());
                        } else {
                            val.setType(CalcValueType::Length);
                            val.setValue(-1 * ret.cssLengthValue());
                        }
                    }
                } else if (isTimeParser &&
                           ret.updateValueUnitTime(CSSTokenValue(str.c_str()),
                                                   parserOption)) {
                    if (unitParsed) {
                        return false;
                    }
                    unitParsed = true;
                    val.setType(CalcValueType::Time);
                    if (isPlus) {
                        val.setValue(ret.timeValue());
                    } else {
                        val.setValue(-1 * ret.timeValue());
                    }
                } else if (isAngleParser &&
                           ret.updateValueUnitAngle(CSSTokenValue(str.c_str()),
                                                    parserOption)) {
                    if (unitParsed) {
                        return false;
                    }
                    unitParsed = true;
                    val.setType(CalcValueType::Angle);
                    if (isPlus) {
                        val.setValue(ret.angleValue());
                    } else {
                        val.setValue(-1 * ret.angleValue());
                    }
                } else {
                    parser.swap(pos);
                    if (parser.consumeNumber()) {
                        float num = parser.parsedNumber();
                        val.setType(CalcValueType::Number);
                        if (isPlus) {
                            val.setValue(num);
                        } else {
                            val.setValue(-1 * num);
                        }
                    } else {
                        return false;
                    }
                }

                if (term->hasValue()) {
                    term->appendValue(isMul, val);
                } else {
                    term->appendValue(val);
                }

                parser.consumeWhitespaces();

                if (parser.consumeIfNext('*')) {
                    isMul = true;
                } else if (parser.consumeIfNext('/')) {
                    isMul = false;
                } else {
                    if (!unitParsed) {
                        return false;
                    }
                    break;
                }
            }

            data->appendTerm(term);

            parser.consumeWhitespaces();

            if (parser.consumeIfNext('+')) {
                isPlus = true;
            } else if (parser.consumeIfNext('-')) {
                isPlus = false;
            } else if (parser.consumeIfNext(')')) {
                break;
            }

            if (parser.isEnd()) {
                return false;
            }
        }

        if (parser.isEnd()) {
            m_valueKind = CSSStyleValuePair::ValueKind::CalcValueKind;
            m_value = data;
            return true;
        }
    }

    return false;
}

bool CSSStyleValuePair::updateValueUnitLength(const CSSTokenValue& token,
                                              uint8_t option)
{
    bool allowAuto = (option & CSSPropertyParser::ParserOption::AllowAuto);
    bool allowNone = (option & CSSPropertyParser::ParserOption::AllowNone);

    if (allowAuto) {
        if (token.equals("auto")) {
            m_valueKind = CSSStyleValuePair::ValueKind::Auto;
            return true;
        }
    }

    if (allowNone) {
        if (token.equals("none")) {
            m_valueKind = CSSStyleValuePair::ValueKind::None;
            return true;
        }
    }

    return CSSPropertyParser::parseLength(token.data(), option, this);
}

bool CSSStyleValuePair::updateValueUnitLengthOrCalc(const CSSTokenValue& token,
                                                    uint8_t option)
{
    if (updateValueUnitLength(token, option)) {
        return true;
    } else {
        option = option & ~CSSPropertyParser::ParserOption::AllowAuto;
        option = option & ~CSSPropertyParser::ParserOption::AllowNone;
        option = option & ~CSSPropertyParser::ParserOption::AllowWithoutUnit;

        return updateValueUnitCalc(
            token, CSSStyleValuePair::CalcParserOption::LengthParser, option);
    }
}

bool CSSStyleValuePair::updateValueLength(const CSSTokenVector& tokens,
                                          uint8_t option)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitLengthOrCalc(tokens[0], option);
}

bool CSSStyleValuePair::updateValueUnitTime(const CSSTokenValue& token,
                                            uint8_t option)
{
    return CSSPropertyParser::parseTime(token.data(), option, this);
}

bool CSSStyleValuePair::updateValueUnitTimeOrCalc(const CSSTokenValue& token,
                                                  uint8_t option)
{
    if (updateValueUnitTime(token, option)) {
        return true;
    } else {
        return updateValueUnitCalc(
            token, CSSStyleValuePair::CalcParserOption::TimeParser, option);
    }
}

bool CSSStyleValuePair::updateValueTime(const CSSTokenVector& tokens,
                                        uint8_t option)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitTimeOrCalc(tokens[0], option);
}

bool CSSStyleValuePair::updateValueUnitAngle(const CSSTokenValue& token,
                                             uint8_t option)
{
    return CSSPropertyParser::parseAngle(token.data(), option, this);
}

bool CSSStyleValuePair::updateValueUnitAngleOrCalc(const CSSTokenValue& token,
                                                   uint8_t option)
{
    if (updateValueUnitAngle(token, option)) {
        return true;
    } else {
        return updateValueUnitCalc(
            token, CSSStyleValuePair::CalcParserOption::AngleParser, option);
    }
}

bool CSSStyleValuePair::updateValueAngle(const CSSTokenVector& tokens,
                                         uint8_t option)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitAngleOrCalc(tokens[0], option);
}

bool CSSStyleValuePair::updateValueBorderImageOutset(
    Document* document, const CSSTokenVector& tokens)
{
    return updateValueUnitBorderImageOutset(tokens);
}

bool CSSStyleValuePair::updateValueUnitBorderImageOutset(
    const CSSTokenVector& tokens)
{
    // [ <length> | <number>]{1,4}
    size_t size = tokens.size();
    if (size < 1 || size > 4) {
        return false;
    }

    m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
    ValueList* values = new ValueList(ValueList::Separator::SpaceSeparator);

    float result = 0.f;
    for (unsigned int i = 0; i < size; i++) {
        CSSTokenValue value = tokens[i];
        if (CSSPropertyParser::parseNumber(value.data(), 0, &result)) {
            values->push_back(CSSStyleValuePair(
                CSSStyleValuePair::ValueKind::Number, (float)result));
        } else {
            CSSStyleValuePair ret;
            if (!ret.updateValueUnitLengthOrCalc(value, 0)) {
                return false;
            }
            values->push_back(ret);
        }
    }
    m_value.m_multiValue = values;

    return true;
}

bool CSSStyleValuePair::updateValueBorderImageWidth(
    Document* document, const CSSTokenVector& tokens)
{
    return updateValueUnitBorderImageWidth(tokens);
}
bool CSSStyleValuePair::updateValueUnitBorderImageWidth(
    const CSSTokenVector& tokens)
{
    // [ <length-percentage> | <number> | auto ]{1,4}
    size_t size = tokens.size();
    if (size < 1 || size > 4) {
        return false;
    }

    m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
    ValueList* values = new ValueList(ValueList::Separator::SpaceSeparator);

    float result = 0.f;
    for (unsigned int i = 0; i < size; i++) {
        CSSTokenValue value = tokens[i];
        if (CSSPropertyParser::parseNumber(value.data(), 0, &result)) {
            values->push_back(CSSStyleValuePair(
                CSSStyleValuePair::ValueKind::Number, (float)result));
        } else {
            CSSStyleValuePair ret;
            if (!ret.updateValueUnitLengthOrCalc(
                    value, CSSPropertyParser::AllowPercent |
                               CSSPropertyParser::AllowAuto)) {
                return false;
            }
            values->push_back(ret);
        }
    }
    m_value.m_multiValue = values;

    return true;
}

bool CSSStyleValuePair::updateValueUnitBackgroundPositionX(
    const CSSTokenValue& value)
{
    m_valueKind = CSSStyleValuePair::ValueKind::SideValueKind;
    if (STRING_VALUE_IS_STRING("left")) {
        m_value.m_side = SideValue::LeftSideValue;
    } else if (STRING_VALUE_IS_STRING("right")) {
        m_value.m_side = SideValue::RightSideValue;
    } else if (STRING_VALUE_IS_STRING("center")) {
        m_value.m_side = SideValue::CenterSideValue;
    } else if (updateValueUnitLengthOrCalc(
                   value, CSSPropertyParser::AllowNegative |
                              CSSPropertyParser::AllowPercent)) {
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueUnitBackgroundPositionY(
    const CSSTokenValue& value)
{
    m_valueKind = CSSStyleValuePair::ValueKind::SideValueKind;
    if (STRING_VALUE_IS_STRING("top")) {
        m_value.m_side = SideValue::TopSideValue;
    } else if (STRING_VALUE_IS_STRING("bottom")) {
        m_value.m_side = SideValue::BottomSideValue;
    } else if (STRING_VALUE_IS_STRING("center")) {
        m_value.m_side = SideValue::CenterSideValue;
    } else if (updateValueUnitLengthOrCalc(
                   value, CSSPropertyParser::AllowNegative |
                              CSSPropertyParser::AllowPercent)) {
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueBackgroundPositionX(
    Document* document, const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitBackgroundPositionX(tokens[0]);
}

bool CSSStyleValuePair::updateValueBackgroundPositionY(
    Document* document, const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitBackgroundPositionY(tokens[0]);
}

bool CSSStyleValuePair::updateValueUnitBackgroundAttachment(
    const CSSTokenValue& value)
{
    m_valueKind = CSSStyleValuePair::ValueKind::BackgroundAttachmentValueKind;
    if (STRING_VALUE_IS_STRING("scroll")) {
        m_value.m_backgroundAttachment =
            BackgroundAttachmentValue::ScrollBackgroundAttachmentValue;
    } else if (STRING_VALUE_IS_STRING("fixed")) {
        m_value.m_backgroundAttachment =
            BackgroundAttachmentValue::FixedBackgroundAttachmentValue;
    } else if (STRING_VALUE_IS_STRING("local")) {
        m_value.m_backgroundAttachment =
            BackgroundAttachmentValue::LocalBackgroundAttachmentValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueBackgroundAttachment(
    Document* document, const CSSTokenVector& tokens)
{
    return updateValueBackgroundAttachment(tokens, true);
}

bool CSSStyleValuePair::updateValueBackgroundAttachment(
    const CSSTokenVector& tokens, bool allowComma)
{
    size_t len = 0;
    m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
    setValueList(new ValueList(ValueList::Separator::CommaSeparator));

    for (unsigned int i = 0; i < tokens.size(); i++) {
        const CSSTokenValue& value = tokens[i];
        if (value.equals(",")) {
            if (!allowComma || i == 0 || i == tokens.size() - 1)
                return false;
        } else if (i == tokens.size() - 1) {
            len++;
            i++;
        } else {
            len++;
            continue;
        }

        CSSStyleValuePair ret;
        if (len == 1) {
            if (!ret.updateValueUnitBackgroundAttachment(tokens[i - 1])) {
                return false;
            }
        } else {
            return false;
        }
        len = 0;
        multiValue()->push_back(ret);
    }

    return true;
}

bool CSSStyleValuePair::updateValueUnitBox(const CSSTokenValue& value)
{
    m_valueKind = CSSStyleValuePair::ValueKind::BoxValueKind;
    if (STRING_VALUE_IS_STRING("border-box")) {
        m_value.m_box = BoxValue::BorderBoxBoxValue;
    } else if (STRING_VALUE_IS_STRING("padding-box")) {
        m_value.m_box = BoxValue::PaddingBoxBoxValue;
    } else if (STRING_VALUE_IS_STRING("content-box")) {
        m_value.m_box = BoxValue::ContentBoxBoxValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueBackgroundClip(Document* document,
                                                  const CSSTokenVector& tokens)
{
    return updateValueBox(tokens, true);
}

bool CSSStyleValuePair::updateValueBackgroundOrigin(
    Document* document, const CSSTokenVector& tokens)
{
    return updateValueBox(tokens, true);
}

bool CSSStyleValuePair::updateValueBox(const CSSTokenVector& tokens,
                                       bool allowComma)
{
    size_t len = 0;
    m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
    setValueList(new ValueList(ValueList::Separator::CommaSeparator));

    for (unsigned int i = 0; i < tokens.size(); i++) {
        const CSSTokenValue& value = tokens[i];
        if (value.equals(",")) {
            if (!allowComma || i == 0 || i == tokens.size() - 1)
                return false;
        } else if (i == tokens.size() - 1) {
            len++;
            i++;
        } else {
            len++;
            continue;
        }

        CSSStyleValuePair ret;
        if (len == 1) {
            if (!ret.updateValueUnitBox(tokens[i - 1])) {
                return false;
            }
        } else {
            return false;
        }
        len = 0;
        multiValue()->push_back(ret);
    }

    return true;
}

bool CSSStyleValuePair::updateValueBackgroundSize(Document* document,
                                                  const CSSTokenVector& tokens)
{
    return updateValueBackgroundSize(tokens, true);
}

bool CSSStyleValuePair::updateValueBackgroundSize(const CSSTokenVector& tokens,
                                                  bool allowComma)
{
    // [length | percentage | auto]{1, 2} | cover | contain // initial value ->
    // auto
    size_t len = 0;
    m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
    setValueList(new ValueList(ValueList::Separator::CommaSeparator));

    for (unsigned int i = 0; i < tokens.size(); i++) {
        const CSSTokenValue& value = tokens[i];
        if (value.equals(",")) {
            if (!allowComma || i == 0 || i == tokens.size() - 1)
                return false;
        } else if (i == tokens.size() - 1) {
            len++;
            i++;
        } else {
            len++;
            continue;
        }

        uint8_t option =
            CSSPropertyParser::AllowPercent | CSSPropertyParser::AllowAuto;
        CSSStyleValuePair ret;
        if (len == 1) {
            if (value.equals("cover")) {
                ret.setBackgroundSizeValue(
                    BackgroundSizeValue::CoverBackgroundSizeValue);
            } else if (value.equals("contain")) {
                ret.setBackgroundSizeValue(
                    BackgroundSizeValue::ContainBackgroundSizeValue);
            } else {
                ret.setValueList(
                    new ValueList(ValueList::Separator::SpaceSeparator));
                CSSStyleValuePair r;
                if (!r.updateValueUnitLengthOrCalc(tokens[i - 1], option)) {
                    return false;
                }
                ret.multiValue()->push_back(r);
            }
        } else if (len == 2) {
            ret.setValueList(
                new ValueList(ValueList::Separator::SpaceSeparator));
            CSSStyleValuePair r1, r2;
            if (!r1.updateValueUnitLengthOrCalc(tokens[i - 2], option) ||
                !r2.updateValueUnitLengthOrCalc(tokens[i - 1], option)) {
                return false;
            }
            ret.multiValue()->push_back(r1);
            ret.multiValue()->push_back(r2);
        } else {
            return false;
        }
        len = 0;
        multiValue()->push_back(ret);
    }

    return true;
}

bool CSSStyleValuePair::updateValueBorderImageSlice(
    Document* document, const CSSTokenVector& tokens)
{
    return updateValueUnitBorderImageSlice(tokens);
}

bool CSSStyleValuePair::updateValueUnitBorderImageSlice(
    const CSSTokenVector& tokens)
{
    // [<number> | <percentage>]{1,4} && fill?
    size_t size = tokens.size();
    if (size < 1 || size > 5) {
        return false;
    }

    CSSStyleValuePair fill;
    bool isFill = false;
    if (tokens[size - 1].equals("fill")) {
        if (size == 1) {
            return false;
        }
        isFill = true;
        fill.setKeywordValue(String::fromUTF8("fill"));
        size--;
    }

    m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
    ValueList* values = new ValueList(ValueList::Separator::SpaceSeparator);

    float result = 0.f;
    for (unsigned int i = 0; i < size; i++) {
        CSSTokenValue value = tokens[i];
        if (value.equals("fill")) {
            return false;
        } else if (CSSPropertyParser::parseNumber(value.data(), 0, &result)) {
            values->push_back(CSSStyleValuePair(
                CSSStyleValuePair::ValueKind::Number, (float)result));
        } else {
            CSSStyleValuePair ret;
            if (!ret.updateValueUnitLengthOrCalc(
                    value, CSSPropertyParser::AllowPercent)) {
                return false;
            }
            if (ret.valueKind() != CSSStyleValuePair::ValueKind::Percentage) {
                return false;
            }
            values->push_back(ret);
        }
    }

    if (isFill) {
        values->push_back(fill);
    }

    m_value.m_multiValue = values;

    return true;
}

bool CSSStyleValuePair::updateValueFontSize(Document* document,
                                            const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitFontSize(tokens[0]);
}

bool CSSStyleValuePair::updateValueUnitFontSize(const CSSTokenValue& value)
{
    // absolute-size | relative-size | length | percentage | inherit // initial
    // value -> medium
    //        O      |       O       |   O    |    O       |    O
    m_valueKind = CSSStyleValuePair::ValueKind::FontSizeValueKind;
    if (STRING_VALUE_IS_STRING("xx-small")) {
        m_value.m_fontSize = FontSizeValue::XXSmallFontSizeValue;
    } else if (STRING_VALUE_IS_STRING("x-small")) {
        m_value.m_fontSize = FontSizeValue::XSmallFontSizeValue;
    } else if (STRING_VALUE_IS_STRING("small")) {
        m_value.m_fontSize = FontSizeValue::SmallFontSizeValue;
    } else if (STRING_VALUE_IS_STRING("medium")) {
        m_value.m_fontSize = FontSizeValue::MediumFontSizeValue;
    } else if (STRING_VALUE_IS_STRING("large")) {
        m_value.m_fontSize = FontSizeValue::LargeFontSizeValue;
    } else if (STRING_VALUE_IS_STRING("x-large")) {
        m_value.m_fontSize = FontSizeValue::XLargeFontSizeValue;
    } else if (STRING_VALUE_IS_STRING("xx-large")) {
        m_value.m_fontSize = FontSizeValue::XXLargeFontSizeValue;
    } else if (STRING_VALUE_IS_STRING("larger")) {
        m_value.m_fontSize = FontSizeValue::LargerFontSizeValue;
    } else if (STRING_VALUE_IS_STRING("smaller")) {
        m_value.m_fontSize = FontSizeValue::SmallerFontSizeValue;
    } else {
        return updateValueUnitLengthOrCalc(value,
                                           CSSPropertyParser::AllowPercent);
    }
    return true;
}

bool CSSStyleValuePair::updateValueLineHeight(Document* document,
                                              const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitLineHeight(tokens[0]);
}

bool CSSStyleValuePair::updateValueUnitLineHeight(const CSSTokenValue& value)
{
    // <normal> | number | length | percentage | inherit
    float result = 0.f;
    if (STRING_VALUE_IS_STRING("normal")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Normal;
        return true;
    } else if (CSSPropertyParser::parseNumber(value.data(), 0, &result)) {
        m_valueKind = CSSStyleValuePair::ValueKind::Number;
        m_value.m_floatValue = result;
        return true;
    } else {
        return updateValueUnitLengthOrCalc(value,
                                           CSSPropertyParser::AllowPercent);
    }
}

#define UPDATE_VALUE_PADDING(POS, ...)                                     \
    bool CSSStyleValuePair::updateValuePadding##POS(                       \
        Document* document, const CSSTokenVector& tokens)                  \
    {                                                                      \
        return updateValueLength(tokens, CSSPropertyParser::AllowPercent); \
    }
GEN_FOURSIDE(UPDATE_VALUE_PADDING)
#undef UPDATE_VALUE_PADDING

#define UPDATE_VALUE_SIDE(POS, ...)                                            \
    bool CSSStyleValuePair::updateValue##POS(Document* document,               \
                                             const CSSTokenVector& tokens)     \
    {                                                                          \
        return updateValueLength(tokens, CSSPropertyParser::AllowNegative |    \
                                             CSSPropertyParser::AllowPercent | \
                                             CSSPropertyParser::AllowAuto);    \
    }
GEN_FOURSIDE(UPDATE_VALUE_SIDE)
#undef UPDATE_VALUE_SIDE

#define UPDATE_VALUE_MARGIN(POS, ...)                                          \
    bool CSSStyleValuePair::updateValueMargin##POS(                            \
        Document* document, const CSSTokenVector& tokens)                      \
    {                                                                          \
        return updateValueLength(tokens, CSSPropertyParser::AllowNegative |    \
                                             CSSPropertyParser::AllowPercent | \
                                             CSSPropertyParser::AllowAuto);    \
    }
GEN_FOURSIDE(UPDATE_VALUE_MARGIN)
#undef UPDATE_VALUE_MARGIN

bool CSSStyleValuePair::updateValueWidthHeightKeyword(
    const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitWidthHeightKeyword(tokens[0]);
}

bool CSSStyleValuePair::updateValueUnitWidthHeightKeyword(
    const CSSTokenValue& value)
{
    if (STRING_VALUE_IS_STRING("available")) {
        m_value.m_widthHeightKeywordValue =
            WidthHeightKeywordValue::AvailableValue;
    } else if (STRING_VALUE_IS_STRING("min-content")) {
        m_value.m_widthHeightKeywordValue =
            WidthHeightKeywordValue::MinContentValue;
    } else if (STRING_VALUE_IS_STRING("max-content")) {
        m_value.m_widthHeightKeywordValue =
            WidthHeightKeywordValue::MaxContentValue;
    } else if (STRING_VALUE_IS_STRING("fit-content")) {
        m_value.m_widthHeightKeywordValue =
            WidthHeightKeywordValue::FitContentValue;
    } else {
        return false;
    }
    m_valueKind = CSSStyleValuePair::ValueKind::WidthHeightKeywordValueKind;
    return true;
}

bool CSSStyleValuePair::updateValueWidth(Document* document,
                                         const CSSTokenVector& tokens)
{
    return updateValueWidthHeightKeyword(tokens) ||
           updateValueLength(tokens, CSSPropertyParser::AllowPercent |
                                         CSSPropertyParser::AllowAuto);
}

bool CSSStyleValuePair::updateValueMaxWidth(Document* document,
                                            const CSSTokenVector& tokens)
{
    return updateValueWidthHeightKeyword(tokens) ||
           updateValueLength(tokens, CSSPropertyParser::AllowPercent |
                                         CSSPropertyParser::AllowNone);
}

bool CSSStyleValuePair::updateValueMinWidth(Document* document,
                                            const CSSTokenVector& tokens)
{
    return updateValueWidthHeightKeyword(tokens) ||
           updateValueLength(tokens, CSSPropertyParser::AllowPercent |
                                         CSSPropertyParser::AllowAuto);
}

bool CSSStyleValuePair::updateValueHeight(Document* document,
                                          const CSSTokenVector& tokens)
{
    return updateValueWidthHeightKeyword(tokens) ||
           updateValueLength(tokens, CSSPropertyParser::AllowPercent |
                                         CSSPropertyParser::AllowAuto);
}

bool CSSStyleValuePair::updateValueMaxHeight(Document* document,
                                             const CSSTokenVector& tokens)
{
    return updateValueWidthHeightKeyword(tokens) ||
           updateValueLength(tokens, CSSPropertyParser::AllowPercent |
                                         CSSPropertyParser::AllowNone);
}

bool CSSStyleValuePair::updateValueMinHeight(Document* document,
                                             const CSSTokenVector& tokens)
{
    return updateValueWidthHeightKeyword(tokens) ||
           updateValueLength(tokens, CSSPropertyParser::AllowPercent |
                                         CSSPropertyParser::AllowAuto);
}

bool CSSStyleValuePair::updateValueVerticalAlign(Document* document,
                                                 const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    if (STRING_VALUE_IS_STRING("baseline")) {
        m_valueKind = CSSStyleValuePair::ValueKind::VerticalAlignValueKind;
        m_value.m_verticalAlign = VerticalAlignValue::BaselineVAlignValue;
    } else if (STRING_VALUE_IS_STRING("sub")) {
        m_valueKind = CSSStyleValuePair::ValueKind::VerticalAlignValueKind;
        m_value.m_verticalAlign = VerticalAlignValue::SubVAlignValue;
    } else if (STRING_VALUE_IS_STRING("super")) {
        m_valueKind = CSSStyleValuePair::ValueKind::VerticalAlignValueKind;
        m_value.m_verticalAlign = VerticalAlignValue::SuperVAlignValue;
    } else if (STRING_VALUE_IS_STRING("top")) {
        m_valueKind = CSSStyleValuePair::ValueKind::VerticalAlignValueKind;
        m_value.m_verticalAlign = VerticalAlignValue::TopVAlignValue;
    } else if (STRING_VALUE_IS_STRING("text-top")) {
        m_valueKind = CSSStyleValuePair::ValueKind::VerticalAlignValueKind;
        m_value.m_verticalAlign = VerticalAlignValue::TextTopVAlignValue;
    } else if (STRING_VALUE_IS_STRING("middle")) {
        m_valueKind = CSSStyleValuePair::ValueKind::VerticalAlignValueKind;
        m_value.m_verticalAlign = VerticalAlignValue::MiddleVAlignValue;
    } else if (STRING_VALUE_IS_STRING("bottom")) {
        m_valueKind = CSSStyleValuePair::ValueKind::VerticalAlignValueKind;
        m_value.m_verticalAlign = VerticalAlignValue::BottomVAlignValue;
    } else if (STRING_VALUE_IS_STRING("text-bottom")) {
        m_valueKind = CSSStyleValuePair::ValueKind::VerticalAlignValueKind;
        m_value.m_verticalAlign = VerticalAlignValue::TextBottomVAlignValue;
    } else {
        return updateValueUnitLengthOrCalc(value,
                                           CSSPropertyParser::AllowNegative |
                                               CSSPropertyParser::AllowPercent);
    }
    return true;
}

static bool parseRectFunctionPart(const CSSTokenValue& s, size_t* ret,
                                  GCVector<String*>& units)
{
    auto ss = s.trim();
    CSSPropertyParser parser((char*)ss.data(), ss.length());

    bool hasPoint = false;
    parser.consumeWhitespaces();
    if (!parser.consumeNumber(&hasPoint)) {
        return false;
    }

    float number = parser.parsedNumber();

    parser.consumeString(CSSPropertyParser::AllowWithoutUnit);

    String* str = parser.parsedString();

    if (str->length() != 0 && !CSSPropertyParser::isLengthUnit(str)) {
        return false;
    }

    *ret = number;
    units.push_back(str);

    return true;
}

bool CSSStyleValuePair::updateValueClip(Document* document,
                                        const CSSTokenVector& tokens)
{
    // https://www.w3.org/TR/css-masking-1/#clip-property
    if (tokens.size() != 1)
        return false;

    CSSTokenValue str = tokens[0];

    if (str.equals("auto")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Auto;
    } else {
        // rect
        bool maybeRect = str.startsWith("rect(");

        if (!maybeRect)
            return false;

        size_t s1 = str.indexOf('(');
        size_t s2 = str.indexOf(')');

        if (s1 == SIZE_MAX || s2 != str.length() - 1 || s1 >= s2) {
            return false;
        }

        CSSTokenValue sub = str.substring(s1 + 1, s2 - s1 - 1);

        std::vector<CSSTokenValue> v;
        sub.split(',', v);

        size_t size = v.size();

        if (size != 4) {
            v.clear();
            sub.split(' ', v);
            size = v.size();
            if (size != 4) {
                return false;
            }
        }

        size_t value[4];
        GCVector<String*> units;
        for (size_t i = 0; i < size; i++) {
            if (!parseRectFunctionPart(v[i], &value[i], units)) {
                return false;
            }
        }

        setClipData(new RectData(CSSLength(units[0], value[0]).toLength(),
                                 CSSLength(units[1], value[1]).toLength(),
                                 CSSLength(units[2], value[2]).toLength(),
                                 CSSLength(units[3], value[3]).toLength()));
    }

    return true;
}

static bool parseGridTemplateColumnsAndRows(const CSSTokenVector& tokens,
                                            GCVector<GridLength>* v)
{
    for (size_t i = 0; i < tokens.size(); i++) {
        auto ss = tokens[i].trim();
        CSSPropertyParser parser((char*)ss.data(), ss.length());

        bool hasPoint = false;
        parser.consumeWhitespaces();
        if (!parser.consumeNumber(&hasPoint)) {
            return false;
        }

        float number = parser.parsedNumber();
        parser.consumeString(CSSPropertyParser::AllowWithoutUnit);
        String* str = parser.parsedString();
        if (str->length() != 0 &&
            (!CSSPropertyParser::isLengthUnit(str) && !str->equals("fr"))) {
            return false;
        }

        // TODO : Add the GridLine, GridArea and Repeat
        // Create GridLength and push back into vector.
        if (str->equals("fr")) {
            GridLength g(number);
            v->push_back(g);
        } else {
            GridLength g(CSSLength(str, number).toLength());
            v->push_back(g);
        }
    }

    return true;
}

bool CSSStyleValuePair::updateValueGridTemplateColumns(
    Document* document, const CSSTokenVector& tokens)
{
    if (!tokens.size())
        return false;

    if (tokens.size() == 1 && tokens[0].equals("none")) {
        m_valueKind = CSSStyleValuePair::ValueKind::None;
        return true;
    }

    GCVector<GridLength>* v = new GCVector<GridLength>();
    ValueList* v1 = new ValueList(ValueList::Separator::SpaceSeparator);

    if (!parseGridTemplateColumnsAndRows(tokens, v)) {
        return false;
    }

    setGridTemplateUnits(v);

    return true;
}

bool CSSStyleValuePair::updateValueGridTemplateRows(
    Document* document, const CSSTokenVector& tokens)
{
    if (!tokens.size())
        return false;

    if (tokens.size() == 1 && tokens[0].equals("none")) {
        m_valueKind = CSSStyleValuePair::ValueKind::None;
        return true;
    }

    GCVector<GridLength>* v = new GCVector<GridLength>();

    if (!parseGridTemplateColumnsAndRows(tokens, v)) {
        return false;
    }

    setGridTemplateUnits(v);

    return true;
}

bool CSSStyleValuePair::updateValueGridRowStart(Document* document,
                                                const CSSTokenVector& tokens)
{
    if (!tokens.size()) {
        return false;
    }

    auto ss = tokens[0].trim();
    CSSPropertyParser parser((char*)ss.data(), ss.length());

    bool hasPoint = false;
    parser.consumeWhitespaces();
    if (!parser.consumeNumber(&hasPoint)) {
        return false;
    }

    float number = parser.parsedNumber();

    if (number >= 1.0f) {
        setInt32Value(number);
        return true;
    }

    return false;
}

bool CSSStyleValuePair::updateValueGridRowEnd(Document* document,
                                              const CSSTokenVector& tokens)
{
    if (!tokens.size()) {
        return false;
    }

    auto ss = tokens[0].trim();
    CSSPropertyParser parser((char*)ss.data(), ss.length());

    bool hasPoint = false;
    parser.consumeWhitespaces();
    if (!parser.consumeNumber(&hasPoint)) {
        return false;
    }

    float number = parser.parsedNumber();

    if (number >= 1.0f) {
        setInt32Value(number);
        return true;
    }

    return false;
}

bool CSSStyleValuePair::updateValueGridColumnStart(Document* document,
                                                   const CSSTokenVector& tokens)
{
    if (!tokens.size()) {
        return false;
    }

    auto ss = tokens[0].trim();
    CSSPropertyParser parser((char*)ss.data(), ss.length());

    bool hasPoint = false;
    parser.consumeWhitespaces();
    if (!parser.consumeNumber(&hasPoint)) {
        return false;
    }

    float number = parser.parsedNumber();

    if (number >= 1.0f) {
        setInt32Value(number);
        return true;
    }

    return false;
}

bool CSSStyleValuePair::updateValueGridColumnEnd(Document* document,
                                                 const CSSTokenVector& tokens)
{
    if (!tokens.size()) {
        return false;
    }

    auto ss = tokens[0].trim();
    CSSPropertyParser parser((char*)ss.data(), ss.length());

    bool hasPoint = false;
    parser.consumeWhitespaces();
    if (!parser.consumeNumber(&hasPoint)) {
        return false;
    }

    float number = parser.parsedNumber();

    if (number >= 1.0f) {
        setInt32Value(number);
        return true;
    }

    return false;
}

bool CSSStyleValuePair::updateValueTransformOrigin(Document* document,
                                                   const CSSTokenVector& tokens)
{
    //  [ left | center | right | top | bottom | <percentage> | <length> ] |
    //  [ left | center | right | <percentage> | <length> ]
    //  [ top | center | bottom | <percentage> | <length> ] <length>? |
    //  [ center | [ left | right ] ] && [ center | [ top | bottom ] ] <length>?

    if (tokens.size() != 1 && tokens.size() != 2 && tokens.size() != 3) {
        return false;
    }

    if (tokens.size() >= 2) {
        const CSSTokenValue& f = tokens[0];
        const CSSTokenValue& s = tokens[1];
        if ((f.equals("left") && s.equals("right")) ||
            (f.equals("right") && s.equals("left")) ||
            (f.equals("top") && s.equals("bottom")) ||
            (f.equals("bottom") && s.equals("top")))
            return false;
    }

    m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
    ValueList* values = new ValueList(ValueList::Separator::SpaceSeparator);

    CSSStyleValuePair xPair(CSSStyleValuePair::ValueKind::SideValueKind,
                            SideValue::CenterSideValue);
    CSSStyleValuePair yPair(CSSStyleValuePair::ValueKind::SideValueKind,
                            SideValue::CenterSideValue);
    CSSStyleValuePair zPair(CSSStyleValuePair::ValueKind::Length, CSSLength(0));

    uint8_t option =
        CSSPropertyParser::AllowPercent | CSSPropertyParser::AllowNegative;
    for (unsigned int i = 0; i < std::min(tokens.size(), (size_t)2); i++) {
        const CSSTokenValue& value = tokens[i];
        if (STRING_VALUE_IS_STRING("left")) {
            xPair.setValue(SideValue::LeftSideValue);
        } else if (STRING_VALUE_IS_STRING("right")) {
            xPair.setValue(SideValue::RightSideValue);
        } else if (STRING_VALUE_IS_STRING("center")) {
        } else if (STRING_VALUE_IS_STRING("top")) {
            yPair.setValue(SideValue::TopSideValue);
        } else if (STRING_VALUE_IS_STRING("bottom")) {
            yPair.setValue(SideValue::BottomSideValue);
        } else {
            if (i == 0) {
                xPair.setValueKind(CSSStyleValuePair::ValueKind::None);
            } else {
                yPair.setValueKind(CSSStyleValuePair::ValueKind::None);
            }

            if (tokens.size() == 2) {
                if (i == 0) {
                    if (tokens[1].equals("left") || tokens[1].equals("right")) {
                        return false;
                    }
                } else {
                    if (tokens[0].equals("top") || tokens[0].equals("bottom")) {
                        return false;
                    }
                }
            }

            CSSStyleValuePair ret;
            if (!ret.updateValueUnitLengthOrCalc(value, option)) {
                return false;
            }
            values->push_back(ret);
        }
    }

    if (tokens.size() == 3) {
        const CSSTokenValue& s = tokens[2];
        if (!zPair.updateValueUnitLengthOrCalc(s, option)) {
            return false;
        }
        if (zPair.valueKind() == CSSStyleValuePair::ValueKind::Percentage) {
            return false;
        }
    }

    if (xPair.valueKind() == CSSStyleValuePair::ValueKind::SideValueKind) {
        values->push_back(xPair);
    }
    if (yPair.valueKind() == CSSStyleValuePair::ValueKind::SideValueKind) {
        values->push_back(yPair);
    }

    values->push_back(zPair);

    m_value.m_multiValue = values;
    return true;
}

bool CSSStyleValuePair::updateValueTransform(Document* document,
                                             const CSSTokenVector& tokens)
{
    return updateValueTransform(tokens, false);
}

bool CSSStyleValuePair::updateValueOpacity(Document* document,
                                           const CSSTokenVector& tokens)
{
    return updateValueNumber(tokens, CSSPropertyParser::AllowNegative);
}

bool CSSStyleValuePair::updateValueFontWeight(Document* document,
                                              const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitFontWeight(tokens[0]);
}

bool CSSStyleValuePair::updateValueUnitFontWeight(const CSSTokenValue& value)
{
    m_valueKind = CSSStyleValuePair::ValueKind::FontWeightValueKind;

    // <normal> | bold | bolder | lighter | 100 | 200 | 300 | 400 | 500 | 600 |
    // 700 | 800 | 900 | inherit // initial -> normal
    if (STRING_VALUE_IS_STRING("normal")) {
        m_value.m_fontWeight = FontWeightValue::NormalFontWeightValue;
    } else if (STRING_VALUE_IS_STRING("bold")) {
        m_value.m_fontWeight = FontWeightValue::BoldFontWeightValue;
    } else if (STRING_VALUE_IS_STRING("bolder")) {
        m_value.m_fontWeight = FontWeightValue::BolderFontWeightValue;
    } else if (STRING_VALUE_IS_STRING("lighter")) {
        m_value.m_fontWeight = FontWeightValue::LighterFontWeightValue;
    } else if (STRING_VALUE_IS_STRING("100")) {
        m_value.m_fontWeight = FontWeightValue::OneHundredFontWeightValue;
    } else if (STRING_VALUE_IS_STRING("200")) {
        m_value.m_fontWeight = FontWeightValue::TwoHundredsFontWeightValue;
    } else if (STRING_VALUE_IS_STRING("300")) {
        m_value.m_fontWeight = FontWeightValue::ThreeHundredsFontWeightValue;
    } else if (STRING_VALUE_IS_STRING("400")) {
        m_value.m_fontWeight = FontWeightValue::FourHundredsFontWeightValue;
    } else if (STRING_VALUE_IS_STRING("500")) {
        m_value.m_fontWeight = FontWeightValue::FiveHundredsFontWeightValue;
    } else if (STRING_VALUE_IS_STRING("600")) {
        m_value.m_fontWeight = FontWeightValue::SixHundredsFontWeightValue;
    } else if (STRING_VALUE_IS_STRING("700")) {
        m_value.m_fontWeight = FontWeightValue::SevenHundredsFontWeightValue;
    } else if (STRING_VALUE_IS_STRING("800")) {
        m_value.m_fontWeight = FontWeightValue::EightHundredsFontWeightValue;
    } else if (STRING_VALUE_IS_STRING("900")) {
        m_value.m_fontWeight = FontWeightValue::NineHundredsFontWeightValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueFontFamily(const CSSTokenVector& tokens)
{
    if (tokens.size() == 1) {
        const std::string& str = tokens[0];
        if (str[0] == '\'' && str.length() > 2 && str.back() == '\'') {
            setKeywordValue(String::fromUTF8(str.data() + 1, str.length() - 2));
        } else if (str[0] == '"' && str.length() > 2 && str.back() == '"') {
            setKeywordValue(String::fromUTF8(str.data() + 1, str.length() - 2));
        } else {
            setKeywordValue(String::fromUTF8(str.data(), str.length()));
        }
        return true;
    }
    ValueList* val = new ValueList(
        ValueList::Separator::CommaSeparatorAppendQuoteWhenMeetWhiteSpace);
    bool seenComma = false;
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i] == ",") {
            if (seenComma) {
                return false;
            }
            seenComma = true;
        } else {
            const std::string& str = tokens[i];
            if (str[0] == '\'' && str.length() > 2 && str.back() == '\'') {
                val->emplace_back(
                    ValueKind::KeywordValueKind,
                    String::fromUTF8(str.data() + 1, str.length() - 2));
            } else if (str[0] == '"' && str.length() > 2 && str.back() == '"') {
                val->emplace_back(
                    ValueKind::KeywordValueKind,
                    String::fromUTF8(str.data() + 1, str.length() - 2));
            } else {
                val->emplace_back(ValueKind::KeywordValueKind,
                                  String::fromUTF8(str.data(), str.length()));
            }
            seenComma = false;
        }
    }
    if (seenComma)
        return false;

    setValueList(val);
    return true;
}

bool CSSStyleValuePair::updateValueUnitWordWrap(const CSSTokenValue& value)
{
    m_valueKind = CSSStyleValuePair::ValueKind::WordWrapValueKind;

    if (STRING_VALUE_IS_STRING("normal")) {
        m_value.m_wordWrap = WordWrapValue::NormalWordWrapValue;
    } else if (STRING_VALUE_IS_STRING("break-word")) {
        m_value.m_wordWrap = WordWrapValue::BreakWordWordWrapValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueWordWrap(Document* document,
                                            const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    return updateValueUnitWordWrap(tokens[0]);
}

bool CSSStyleValuePair::updateValueOverflowWrap(Document* document,
                                                const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    return updateValueUnitWordWrap(tokens[0]);
}

bool CSSStyleValuePair::updateValueOverflowX(Document* document,
                                             const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    return updateValueUnitOverflowX(tokens[0]);
}

bool CSSStyleValuePair::updateValueUnitOverflowX(const CSSTokenValue& value)
{
    m_valueKind = CSSStyleValuePair::ValueKind::OverflowValueKind;

    if (STRING_VALUE_IS_STRING("visible")) {
        m_value.m_overflow = OverflowValue::VisibleOverflow;
    } else if (STRING_VALUE_IS_STRING("hidden")) {
        m_value.m_overflow = OverflowValue::HiddenOverflow;
    } else if (STRING_VALUE_IS_STRING("auto")) {
        m_value.m_overflow = OverflowValue::AutoOverflow;
    } else if (STRING_VALUE_IS_STRING("scroll")) {
        m_value.m_overflow = OverflowValue::ScrollOverflow;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueOverflowY(Document* document,
                                             const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    return updateValueUnitOverflowY(tokens[0]);
}

bool CSSStyleValuePair::updateValueUnitOverflowY(const CSSTokenValue& value)
{
    m_valueKind = CSSStyleValuePair::ValueKind::OverflowValueKind;

    if (STRING_VALUE_IS_STRING("visible")) {
        m_value.m_overflow = OverflowValue::VisibleOverflow;
    } else if (STRING_VALUE_IS_STRING("hidden")) {
        m_value.m_overflow = OverflowValue::HiddenOverflow;
    } else if (STRING_VALUE_IS_STRING("auto")) {
        m_value.m_overflow = OverflowValue::AutoOverflow;
    } else if (STRING_VALUE_IS_STRING("scroll")) {
        m_value.m_overflow = OverflowValue::ScrollOverflow;
    } else {
        return false;
    }
    return true;
}

static void removeOverflowCSSValuePairs(CSSStyleDeclaration* target)
{
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::OverflowX);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::OverflowY);
}

static void addOverflowCSSValuePairs(CSSStyleDeclaration* target,
                                     CSSStyleValuePair overflowX,
                                     CSSStyleValuePair overflowY)
{
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::OverflowX, overflowX);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::OverflowY, overflowY);
}

static bool parseOverflowShorthand(const CSSTokenVector& tokens,
                                   CSSStyleValuePair* overflowX,
                                   CSSStyleValuePair* overflowY)
{
    size_t len = tokens.size();
    if (len < 1) {
        return false;
    }

    overflowX->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    overflowY->setValueKind(CSSStyleValuePair::ValueKind::Initial);

    CSSStyleValuePair temp;

    const CSSTokenValue& tok = tokens[0];

    if (temp.updateValueUnitOverflowX(tokens[0])) {
        *overflowX = temp;
        *overflowY = temp;
    } else {
        return false;
    }
    return true;
}

void CSSStyleDeclaration::setOverflow(const char* value, size_t length,
                                      bool isImportant)
{
    if (length == 0) {
        removeOverflowCSSValuePairs(this);
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, length);

    // TODO comma separation
    CSSStyleValuePair v, overflowX, overflowY;
    if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        addOverflowCSSValuePairs(this, v, v);
    } else if (parseOverflowShorthand(tokens, &overflowX, &overflowY)) {
        overflowX.setFlagImportant(isImportant);
        overflowY.setFlagImportant(isImportant);
        addOverflowCSSValuePairs(this, overflowX, overflowY);
    }
}

String* CSSStyleDeclaration::Overflow()
{
    // TODO: Should find the specific rule for composing overflow
    String* overflowX = OverflowX();
    String* overflowY = OverflowY();

    if (overflowX->equals(overflowY)) {
        return overflowX;
    } else {
        return String::createASCIIString("auto");
    }
}

bool CSSStyleValuePair::updateValuePosition(Document* document,
                                            const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    // <static> | relative | absolute | inherit
    m_valueKind = CSSStyleValuePair::ValueKind::PositionValueKind;

    if (STRING_VALUE_IS_STRING("static")) {
        m_value.m_position = PositionValue::StaticPositionValue;
    } else if (STRING_VALUE_IS_STRING("relative")) {
        m_value.m_position = PositionValue::RelativePositionValue;
    } else if (STRING_VALUE_IS_STRING("absolute")) {
        m_value.m_position = PositionValue::AbsolutePositionValue;
    } else if (STRING_VALUE_IS_STRING("fixed")) {
        m_value.m_position = PositionValue::FixedPositionValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueShadow(const CSSTokenVector& tokens,
                                          bool boxShadow)
{
    if (tokens.size() < 1) {
        return false;
    }
    if (tokens.size() == 1) {
        const CSSTokenValue& t = tokens[0];
        const char* value = t.data();
        if (VALUE_IS_NONE()) {
            m_valueKind = CSSStyleValuePair::ValueKind::None;
        } else {
            return false;
        }
        return true;
    } else {
        m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;

        setValueList(new ValueList(ValueList::Separator::CommaSeparator));
        uint8_t option = CSSPropertyParser::AllowNegative |
                         CSSPropertyParser::AllowWithoutUnit;

        // Value : none | <shadow>#
        // <shadow> = <color>? && <length>{2,4} && inset?
        // 1st <length> =  horizontal offset
        // 2nd <lennth> =  vertical offset
        // 3rd <length> =  blur radius
        // 4th <length> = spread distance

        size_t lengthSizeLimit = 0;
        size_t shadowSizeLimit = 0;
        if (boxShadow) {
            lengthSizeLimit = 4;
            shadowSizeLimit = lengthSizeLimit + 2;
        } else {
            lengthSizeLimit = 3;
            shadowSizeLimit = lengthSizeLimit + 1;
        }

        bool isValid = false;
        for (size_t i = 0; i < tokens.size();) {
            if (tokens[i].equals(",")) {
                i++;
                isValid = false;
                continue;
            }

            bool hasInset = false;
            bool hasColor = false;
            bool didParseLength = false;

            CSSStyleValuePair shadow;
            shadow.setValueList(
                new ValueList(ValueList::Separator::SpaceSeparator));

            size_t j = i;
            size_t currentShadowSize = 0;

            while ((j < tokens.size() && !tokens[j].equals(",")) &&
                   currentShadowSize < shadowSizeLimit) {
                CSSStyleValuePair temp;

                if (temp.updateValueUnitLength(tokens[j], option)) {
                    if (didParseLength) {
                        return false;
                    }
                    didParseLength = true;
                    CSSStyleValuePair lengths;
                    lengths.setValueList(
                        new ValueList(ValueList::Separator::SpaceSeparator));

                    CSSStyleValuePair length;
                    size_t len2 = 1;
                    for (; (j < tokens.size()) && (len2 <= lengthSizeLimit) &&
                           length.updateValueUnitLength(tokens[j], option);
                         j++, len2++) {
                        std::string str = tokens[j];
                        lengths.multiValue()->push_back(length);
                        currentShadowSize++;
                    }

                    if (lengths.multiValue()->size() < 2) {
                        return false;
                    }

                    shadow.multiValue()->push_back(lengths);

                } else if (temp.updateValueUnitColor(tokens[j])) {
                    // color
                    if (hasColor) {
                        return false;
                    }
                    shadow.multiValue()->push_back(temp);
                    hasColor = true;
                    currentShadowSize++;
                    j++;

                } else if (tokens[j].equals("inset")) {
                    if (hasInset || !boxShadow) {
                        return false;
                    }
                    temp.setKeywordValue(String::fromUTF8(tokens[j].data()));
                    shadow.multiValue()->push_back(temp);
                    hasInset = true;
                    currentShadowSize++;
                    j++;
                } else {
                    return false;
                }
            }

            if (!didParseLength) {
                return false;
            }

            multiValue()->push_back(shadow);
            isValid = true;
            i = j;
        }
        return isValid;
    }
    return false;
}

bool CSSStyleValuePair::updateValueTextShadow(Document* document,
                                              const CSSTokenVector& tokens)
{
    // none | [ <offset-x> <offset-y> <blur-radius>? && <color>? ]#
    // initial : none
    return updateValueShadow(tokens, false);
}

bool CSSStyleValuePair::updateValueBoxShadow(Document* document,
                                             const CSSTokenVector& tokens)
{
    //  none | [inset? && [ <offset-x> <offset-y> <blur-radius>?
    //  <spread-radius>? <color>? ] ]#
    // initial : none
    return updateValueShadow(tokens, true);
}

void CSSStyleDeclaration::setTextDecoration(const char* value, size_t len,
                                            bool isImportant)
{
    if (len == 0) {
        removeCSSValuePair(CSSStyleValuePair::KeyKind::TextDecorationLine);
        removeCSSValuePair(CSSStyleValuePair::KeyKind::TextDecorationStyle);
        removeCSSValuePair(CSSStyleValuePair::KeyKind::TextDecorationColor);
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, len, "", 0);

    CSSStyleValuePair ret;
    if (ret.updateValueCommon(tokens)) {
        ret.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::TextDecorationLine, ret);
        addCSSValuePair(CSSStyleValuePair::KeyKind::TextDecorationStyle, ret);
        addCSSValuePair(CSSStyleValuePair::KeyKind::TextDecorationColor, ret);
        return;
    }

    bool doneStyle = false;
    bool doneColor = false;
    bool doneLine = false;
    CSSTokenVector singleTokenList;
    for (size_t i = 0; i < tokens.size(); i++) {
        singleTokenList.clear();
        singleTokenList.push_back(tokens[i]);

        CSSStyleValuePair p;
        p.setFlagImportant(isImportant);
        if (!doneStyle &&
            p.updateValueTextDecorationStyle(nullptr, singleTokenList)) {
            addCSSValuePair(CSSStyleValuePair::KeyKind::TextDecorationStyle, p);
            doneStyle = true;
        } else if (!doneColor &&
                   p.updateValueTextDecorationColor(nullptr, singleTokenList)) {
            addCSSValuePair(CSSStyleValuePair::KeyKind::TextDecorationColor, p);
            doneColor = true;
        } else if (!doneLine &&
                   p.updateValueTextDecorationLine(nullptr, singleTokenList)) {
            // text-decoration-line can accept consecutive multi-values
            std::set<TextDecorationLineValue> set;
            const CSSStyleValuePair& lineVal = p.multiValue()->at(0);
            set.insert(lineVal.textDecorationLineValue());
            for (size_t j = i + 1; j < tokens.size(); j++) {
                singleTokenList.clear();
                singleTokenList.push_back(tokens[j]);
                bool ok = false;
                CSSStyleValuePair tmpVal;
                if (tmpVal.updateValueTextDecorationLine(nullptr,
                                                         singleTokenList)) {
                    const CSSStyleValuePair& nextVal =
                        tmpVal.multiValue()->at(0);
                    if (set.find(nextVal.textDecorationLineValue()) ==
                        set.end()) {
                        i = j;
                        set.insert(nextVal.textDecorationLineValue());
                        p.multiValue()->push_back(nextVal);
                        ok = true;
                    }
                }

                if (!ok) {
                    break;
                }
            }
            addCSSValuePair(CSSStyleValuePair::KeyKind::TextDecorationLine, p);
            doneLine = true;
        } else {
            // Error: each style, color, and line can appear only once OR
            // an unknown token received
            removeCSSValuePair(CSSStyleValuePair::KeyKind::TextDecorationLine);
            removeCSSValuePair(CSSStyleValuePair::KeyKind::TextDecorationStyle);
            removeCSSValuePair(CSSStyleValuePair::KeyKind::TextDecorationColor);
            return;
        }
    }
}

String* CSSStyleDeclaration::TextDecoration()
{
    StringBuilder b;
    for (unsigned i = 0; i < m_cssValues.size(); i++) {
        CSSStyleValuePair& p = m_cssValues[i];
        switch (p.keyKind()) {
        case CSSStyleValuePair::KeyKind::TextDecorationLine:
            b.appendString(p.multiValue()->toString());
            break;
        case CSSStyleValuePair::KeyKind::TextDecorationStyle:
        case CSSStyleValuePair::KeyKind::TextDecorationColor:
            b.appendString(p.toString());
            break;
        default:
            break;
        }
    }

    return b.finalize();
}

bool CSSStyleValuePair::updateValueTextDecorationLine(
    Document* document, const CSSTokenVector& tokens)
{
    if (tokens.size() < 1) {
        return false;
    }

    m_valueKind = CSSStyleValuePair::ValueListKind;
    m_value.m_multiValue = new ValueList(ValueList::Separator::SpaceSeparator);
    std::set<TextDecorationLineValue> set;
    for (size_t i = 0; i < tokens.size(); i++) {
        const CSSTokenValue& value = tokens[i];
        TextDecorationLineValue v;

        if (STRING_VALUE_IS_NONE()) {
            v = TextDecorationLineValue::NoneTextDecorationLineValue;
        } else if (STRING_VALUE_IS_STRING("underline")) {
            v = TextDecorationLineValue::UnderlineTextDecorationLineValue;
        } else if (STRING_VALUE_IS_STRING("line-through")) {
            v = TextDecorationLineValue::LineThroughTextDecorationLineValue;
        } else if (STRING_VALUE_IS_STRING("overline")) {
            v = TextDecorationLineValue::OverlineTextDecorationLineValue;
            return false; // unsupported yet
        } else if (STRING_VALUE_IS_STRING("blink")) {
            v = TextDecorationLineValue::BlinkTextDecorationLineValue;
            return false; // unsupported yet
        } else {
            return false;
        }

        if (set.find(v) == set.end()) {
            CSSStyleValuePair p;
            p.setTextDecorationLineValue(v);
            m_value.m_multiValue->push_back(p);
            set.insert(v);
        } else {
            return false; // invalid when duplicate inputs are given
        }
    }
    return true;
}

bool CSSStyleValuePair::updateValueTextDecorationColor(
    Document* document, const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::ColorValueKind;

    if (STRING_VALUE_IS_STRING("currentcolor")) {
        return CSSPropertyParser::parseNamedColor(value, this);
    } else if (STRING_VALUE_IS_STRING("transparent")) {
        m_value.m_color = Unit::Color(0, 0, 0, 0);
        return true;
    } else {
        return updateValueColor(document, tokens);
    }
    return false;
}

bool CSSStyleValuePair::updateValueTextDecorationStyle(
    Document* document, const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::TextDecorationStyleValueKind;

    if (STRING_VALUE_IS_STRING("solid")) {
        m_value.m_textDecorationStyle =
            TextDecorationStyleValue::SolidTextDecorationStyleValue;
    } else if (STRING_VALUE_IS_STRING("double")) {
        m_value.m_textDecorationStyle =
            TextDecorationStyleValue::DoubleTextDecorationStyleValue;
        return false; // unsupported yet
    } else if (STRING_VALUE_IS_STRING("dotted")) {
        m_value.m_textDecorationStyle =
            TextDecorationStyleValue::DottedTextDecorationStyleValue;
        return false; // unsupported yet
    } else if (STRING_VALUE_IS_STRING("dashed")) {
        m_value.m_textDecorationStyle =
            TextDecorationStyleValue::DashedTextDecorationStyleValue;
        return false; // unsupported yet
    } else if (STRING_VALUE_IS_STRING("wavy")) {
        m_value.m_textDecorationStyle =
            TextDecorationStyleValue::WavyTextDecorationStyleValue;
        return false; // unsupported yet
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueTextUnderlinePosition(
    Document* document, const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::TextUnderlinePositionValueKind;

    if (STRING_VALUE_IS_STRING("auto")) {
        m_value.m_textUnderlinePosition =
            TextUnderlinePositionValue::AutoTextUnderlinePositionValue;
    } else if (STRING_VALUE_IS_STRING("under")) {
        m_value.m_textUnderlinePosition =
            TextUnderlinePositionValue::UnderTextUnderlinePositionValue;
        return false; // unsupported yet
    } else if (STRING_VALUE_IS_STRING("left")) {
        m_value.m_textUnderlinePosition =
            TextUnderlinePositionValue::LeftTextUnderlinePositionValue;
        return false; // unsupported yet
    } else if (STRING_VALUE_IS_STRING("right")) {
        m_value.m_textUnderlinePosition =
            TextUnderlinePositionValue::RightTextUnderlinePositionValue;
        return false; // unsupported yet
    } else {
        return false;
    }

    return true;
}

bool CSSStyleValuePair::updateValueResize(Document* document,
                                          const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::ResizeValueKind;

    if (STRING_VALUE_IS_STRING("none")) {
        m_value.m_resize = ResizeValue::NoneResizeValue;
    } else if (STRING_VALUE_IS_STRING("both")) {
        m_value.m_resize = ResizeValue::BothResizeValue;
        return false; // unsupported yet
    } else if (STRING_VALUE_IS_STRING("horizontal")) {
        m_value.m_resize = ResizeValue::HorizontalResizeValue;
        return false; // unsupported yet
    } else if (STRING_VALUE_IS_STRING("vertical")) {
        m_value.m_resize = ResizeValue::VerticalResizeValue;
        return false; // unsupported yet
    } else if (STRING_VALUE_IS_STRING("block")) {
        m_value.m_resize = ResizeValue::BlockResizeValue;
        return false; // unsupported yet
    } else if (STRING_VALUE_IS_STRING("inline")) {
        m_value.m_resize = ResizeValue::InlineResizeValue;
        return false; // unsupported yet
    } else {
        return false;
    }

    return true;
}

bool CSSStyleValuePair::updateValueTextAlign(Document* document,
                                             const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    if (STRING_VALUE_IS_STRING("start")) {
        m_valueKind = CSSStyleValuePair::ValueKind::TextAlignValueKind;
        m_value.m_textAlign = TextAlignValue::StartTextAlignValue;
    } else if (STRING_VALUE_IS_STRING("end")) {
        m_valueKind = CSSStyleValuePair::ValueKind::TextAlignValueKind;
        m_value.m_textAlign = TextAlignValue::EndTextAlignValue;
    } else if (STRING_VALUE_IS_STRING("left")) {
        m_valueKind = CSSStyleValuePair::ValueKind::TextAlignValueKind;
        m_value.m_textAlign = TextAlignValue::LeftTextAlignValue;
    } else if (STRING_VALUE_IS_STRING("center")) {
        m_valueKind = CSSStyleValuePair::ValueKind::TextAlignValueKind;
        m_value.m_textAlign = TextAlignValue::CenterTextAlignValue;
    } else if (STRING_VALUE_IS_STRING("right")) {
        m_valueKind = CSSStyleValuePair::ValueKind::TextAlignValueKind;
        m_value.m_textAlign = TextAlignValue::RightTextAlignValue;
    } else if (STRING_VALUE_IS_STRING("-starfish-center")) {
        m_valueKind = CSSStyleValuePair::ValueKind::TextAlignValueKind;
        m_value.m_textAlign = TextAlignValue::StarFishCenterTextAlignValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueTextTransform(Document* document,
                                                 const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    if (STRING_VALUE_IS_STRING("none")) {
        m_valueKind = CSSStyleValuePair::ValueKind::TextTransformValueKind;
        m_value.m_textTransform = TextTransformValue::NoneTextTransformValue;
    } else if (STRING_VALUE_IS_STRING("capitalize")) {
        m_valueKind = CSSStyleValuePair::ValueKind::TextTransformValueKind;
        m_value.m_textTransform =
            TextTransformValue::CapitalizeTextTransformValue;
    } else if (STRING_VALUE_IS_STRING("uppercase")) {
        m_valueKind = CSSStyleValuePair::ValueKind::TextTransformValueKind;
        m_value.m_textTransform =
            TextTransformValue::UppercaseTextTransformValue;
    } else if (STRING_VALUE_IS_STRING("lowercase")) {
        m_valueKind = CSSStyleValuePair::ValueKind::TextTransformValueKind;
        m_value.m_textTransform =
            TextTransformValue::LowercaseTextTransformValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueTextOverflow(Document* document,
                                                const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    CSSPropertyParser parser(value);
    m_valueKind = CSSStyleValuePair::ValueKind::TextOverflowValueKind;

    String* stringValue = String::emptyString;
    if (value.equals("clip")) {
        m_value.m_textOverflowData =
            new TextOverflowData(TextOverflowValue::TextOverflowClipValue);
    } else if (value.equals("ellipsis")) {
        m_value.m_textOverflowData =
            new TextOverflowData(TextOverflowValue::TextOverflowEllipsisValue);
    } else if (parser.parseContentString(value.data(), value.length(),
                                         &(stringValue))) {
        m_value.m_textOverflowData = new TextOverflowData(stringValue);
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueTextIndent(Document* document,
                                              const CSSTokenVector& tokens)
{
    return updateValueLength(tokens, CSSPropertyParser::AllowNegative |
                                         CSSPropertyParser::AllowPercent);
}

bool CSSStyleValuePair::updateValueUnicodeBidi(Document* document,
                                               const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    if (STRING_VALUE_IS_STRING("normal")) {
        m_value.m_unicodeBidi = UnicodeBidiValue::NormalUnicodeBidiValue;
        m_valueKind = CSSStyleValuePair::ValueKind::UnicodeBidiValueKind;
    } else if (STRING_VALUE_IS_STRING("embed")) {
        m_value.m_unicodeBidi = UnicodeBidiValue::EmbedUnicodeBidiValue;
        m_valueKind = CSSStyleValuePair::ValueKind::UnicodeBidiValueKind;
    } else if (STRING_VALUE_IS_STRING("isolate")) {
        m_value.m_unicodeBidi = UnicodeBidiValue::IsolateUnicodeBidiValue;
        m_valueKind = CSSStyleValuePair::ValueKind::UnicodeBidiValueKind;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueVisibility(Document* document,
                                              const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    m_valueKind = CSSStyleValuePair::ValueKind::VisibilityValueKind;
    const CSSTokenValue& value = tokens[0];
    if (STRING_VALUE_IS_STRING("visible")) {
        m_value.m_visibility = VisibilityValue::VisibleVisibilityValue;
    } else if (STRING_VALUE_IS_STRING("hidden")) {
        m_value.m_visibility = VisibilityValue::HiddenVisibilityValue;
    } else if (STRING_VALUE_IS_STRING("collapse")) {
        m_value.m_visibility = VisibilityValue::CollapseVisibilityValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueImageRendering(Document* document,
                                                  const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    m_valueKind = CSSStyleValuePair::ValueKind::ImageRenderingValueKind;
    const CSSTokenValue& value = tokens[0];
    if (STRING_VALUE_IS_STRING("auto")) {
        m_value.m_imageRendering = ImageRenderingValue::ImageRenderingAutoValue;
    } else if (STRING_VALUE_IS_STRING("crisp-edges")) {
        m_value.m_imageRendering =
            ImageRenderingValue::ImageRenderingCrispEdgesValue;
    } else if (STRING_VALUE_IS_STRING("pixelated")) {
        m_value.m_imageRendering =
            ImageRenderingValue::ImageRenderingPixelatedValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueZIndex(Document* document,
                                          const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const char* token = tokens[0].data();
    int32_t val = 0;
    if (TOKEN_IS_STRING("auto")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Auto;
    } else {
        return CSSPropertyParser::parseInt32(
            token, CSSPropertyParser::AllowNegative, this);
    }

    return true;
}

bool CSSStyleValuePair::updateValueBorderCollapse(Document* document,
                                                  const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::BorderCollapseValueKind;
    if (STRING_VALUE_IS_STRING("separate")) {
        m_value.m_borderCollapse =
            BorderCollapseValue::SeparateBorderCollapseValue;
    } else if (STRING_VALUE_IS_STRING("collapse")) {
        m_value.m_borderCollapse =
            BorderCollapseValue::CollapseBorderCollapseValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueBorderSpacing(Document* document,
                                                 const CSSTokenVector& tokens)
{
    // <length> <length>? | inherit,
    // Initial : 0, Percentages: N/A, lengths may not be negative.
    if (tokens.size() == 1) {
        const CSSTokenValue& first = tokens[0];
        return CSSPropertyParser::parseLength(first.data(), 0, this);
    } else if (tokens.size() == 2) {
        const CSSTokenValue& first = tokens[0];
        const CSSTokenValue& second = tokens[1];
        ValueData firstData = { 0 };
        ValueData secondData = { 0 };
        if (CSSPropertyParser::parseLength(first.data(), 0, this)) {
            firstData.m_length = m_value.m_length;
            if (CSSPropertyParser::parseLength(second.data(), 0, this)) {
                secondData.m_length = m_value.m_length;
                m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
                m_value.m_multiValue =
                    new ValueList(ValueList::Separator::SpaceSeparator);
                m_value.m_multiValue->emplace_back(
                    CSSStyleValuePair::ValueKind::Length, firstData);
                m_value.m_multiValue->emplace_back(
                    CSSStyleValuePair::ValueKind::Length, secondData);
                return true;
            }
        }
    }
    return false;
}

bool CSSStyleValuePair::updateValueCaptionSide(Document* document,
                                               const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::CaptionSideValueKind;
    if (STRING_VALUE_IS_STRING("top")) {
        m_value.m_captionSide = CaptionSideValue::TopCaptionSideValue;
    } else if (STRING_VALUE_IS_STRING("bottom")) {
        m_value.m_captionSide = CaptionSideValue::BottomCaptionSideValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueEmptyCells(Document* document,
                                              const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::EmptyCellsValueKind;
    if (STRING_VALUE_IS_STRING("show")) {
        m_value.m_emptyCells = EmptyCellsValue::ShowEmptyCellsValue;
    } else if (STRING_VALUE_IS_STRING("hide")) {
        m_value.m_emptyCells = EmptyCellsValue::HideEmptyCellsValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueTableLayout(Document* document,
                                               const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::TableLayoutValueKind;
    if (STRING_VALUE_IS_STRING("auto")) {
        m_value.m_tableLayout = TableLayoutValue::AutoTableLayoutValue;
    } else if (STRING_VALUE_IS_STRING("fixed")) {
        m_value.m_tableLayout = TableLayoutValue::FixedTableLayoutValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueTransitionProperty(
    Document* document, const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    const CSSTokenValue& value = tokens[0];
    return updateValueUnitTransitionProperty(value);
}

bool CSSStyleValuePair::updateValueTransitionDuration(
    Document* document, const CSSTokenVector& tokens)
{
    return updateValueTime(tokens, 0);
}

bool CSSStyleValuePair::updateValueTransitionDelay(Document* document,
                                                   const CSSTokenVector& tokens)
{
    return updateValueTime(tokens, 0);
}

bool CSSStyleValuePair::updateValueTransitionTimingFunction(
    Document* document, const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    // TODO parse steps, cubic-bezier function
    const CSSTokenValue& value = tokens[0];
    m_valueKind =
        CSSStyleValuePair::ValueKind::TransitionTimingFunctionValueKind;
    if (STRING_VALUE_IS_STRING("ease")) {
        m_value.m_transitionTimingFunction =
            TransitionTimingFunctionValue::TransitionTimingFunctionEaseValue;
    } else if (STRING_VALUE_IS_STRING("linear")) {
        m_value.m_transitionTimingFunction =
            TransitionTimingFunctionValue::TransitionTimingFunctionLinearValue;
    } else if (STRING_VALUE_IS_STRING("ease-in")) {
        m_value.m_transitionTimingFunction =
            TransitionTimingFunctionValue::TransitionTimingFunctionEaseInValue;
    } else if (STRING_VALUE_IS_STRING("ease-out")) {
        m_value.m_transitionTimingFunction =
            TransitionTimingFunctionValue::TransitionTimingFunctionEaseOutValue;
    } else if (STRING_VALUE_IS_STRING("ease-in-out")) {
        m_value.m_transitionTimingFunction = TransitionTimingFunctionValue::
            TransitionTimingFunctionEaseInOutValue;
    } else if (STRING_VALUE_IS_STRING("step-start")) {
        m_value.m_transitionTimingFunction = TransitionTimingFunctionValue::
            TransitionTimingFunctionStepStartValue;
    } else if (STRING_VALUE_IS_STRING("step-end")) {
        m_value.m_transitionTimingFunction =
            TransitionTimingFunctionValue::TransitionTimingFunctionStepEndValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueBoxSizing(Document* document,
                                             const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::BoxSizingValueKind;
    if (STRING_VALUE_IS_STRING("content-box")) {
        m_value.m_boxSizing = BoxSizingValue::ContentBoxBoxSizingValue;
    } else if (STRING_VALUE_IS_STRING("border-box")) {
        m_value.m_boxSizing = BoxSizingValue::BorderBoxBoxSizingValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueUnitFlexDirection(const CSSTokenValue& value)
{
    m_valueKind = CSSStyleValuePair::ValueKind::FlexDirectionValueKind;
    if (STRING_VALUE_IS_STRING("row")) {
        m_value.m_flexDirection = FlexDirectionValue::RowFlexDirectionValue;
    } else if (STRING_VALUE_IS_STRING("row-reverse")) {
        m_value.m_flexDirection =
            FlexDirectionValue::RowReverseFlexDirectionValue;
    } else if (STRING_VALUE_IS_STRING("column")) {
        m_value.m_flexDirection = FlexDirectionValue::ColumnFlexDirectionValue;
    } else if (STRING_VALUE_IS_STRING("column-reverse")) {
        m_value.m_flexDirection =
            FlexDirectionValue::ColumnReverseFlexDirectionValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueFlexDirection(Document* document,
                                                 const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    return updateValueUnitFlexDirection(value);
}

bool CSSStyleValuePair::updateValueUnitFlexWrap(const CSSTokenValue& value)
{
    m_valueKind = CSSStyleValuePair::ValueKind::FlexWrapValueKind;
    if (STRING_VALUE_IS_STRING("nowrap")) {
        m_value.m_flexWrap = FlexWrapValue::NoWrapFlexWrapValue;
    } else if (STRING_VALUE_IS_STRING("wrap")) {
        m_value.m_flexWrap = FlexWrapValue::WrapFlexWrapValue;
    } else if (STRING_VALUE_IS_STRING("wrap-reverse")) {
        m_value.m_flexWrap = FlexWrapValue::WrapReverseFlexWrapValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueFlexWrap(Document* document,
                                            const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    return updateValueUnitFlexWrap(value);
}

static void removeFlexFlowCSSValuePairs(CSSStyleDeclaration* target)
{
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::FlexDirection);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::FlexWrap);
}

static void addFlexFlowCSSValuePairs(CSSStyleDeclaration* target,
                                     CSSStyleValuePair flexDirection,
                                     CSSStyleValuePair flexWrap)
{
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::FlexDirection,
                            flexDirection);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::FlexWrap, flexWrap);
}

static bool parseFlexFlowShorthand(const CSSTokenVector& tokens,
                                   CSSStyleValuePair* flexDirection,
                                   CSSStyleValuePair* flexWrap)
{
    size_t len = tokens.size();
    if (len < 1) {
        return false;
    }

    flexDirection->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    flexWrap->setValueKind(CSSStyleValuePair::ValueKind::Initial);

    bool hasFlexDirection = false, hasFlexWrap = false;
    CSSStyleValuePair temp;
    size_t pos = 0;

    while (pos < len) {
        const CSSTokenValue& token = tokens[pos++];
        if (!hasFlexDirection && temp.updateValueUnitFlexDirection(token)) {
            hasFlexDirection = true;
            *flexDirection = temp;
            continue;
        } else if (temp.updateValueUnitFlexWrap(token)) {
            hasFlexWrap = true;
            *flexWrap = temp;
            break;
        }
        return false;
    }

    return hasFlexDirection || hasFlexWrap;
}

void CSSStyleDeclaration::setFlexFlow(const char* value, size_t length,
                                      bool isImportant)
{
    if (length == 0) {
        removeFlexFlowCSSValuePairs(this);
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, length);

    // TODO comma separation
    CSSStyleValuePair v, flexDirection, flexWrap;
    if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        addFlexFlowCSSValuePairs(this, v, v);
    } else if (parseFlexFlowShorthand(tokens, &flexDirection, &flexWrap)) {
        flexDirection.setFlagImportant(isImportant);
        flexWrap.setFlagImportant(isImportant);
        addFlexFlowCSSValuePairs(this, flexDirection, flexWrap);
    }
}

String* CSSStyleDeclaration::FlexFlow()
{
    String* flexDirection = FlexDirection();
    String* flexWrap = FlexWrap();

    String* space = String::spaceString;
    StringBuilder builder;
    builder.appendString(flexDirection);
    builder.appendString(space);
    builder.appendString(flexWrap);

    return builder.finalize();
}

bool CSSStyleValuePair::updateValueOrder(Document* document,
                                         const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const char* token = tokens[0].data();
    return CSSPropertyParser::parseInt32(
        token, CSSPropertyParser::AllowNegative, this);
}

bool CSSStyleValuePair::updateValueJustifyContent(Document* document,
                                                  const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::JustifyContentValueKind;
    if (STRING_VALUE_IS_STRING("flex-start")) {
        m_value.m_justifyContent =
            JustifyContentValue::FlexStartJustifyContentValue;
    } else if (STRING_VALUE_IS_STRING("flex-end")) {
        m_value.m_justifyContent =
            JustifyContentValue::FlexEndJustifyContentValue;
    } else if (STRING_VALUE_IS_STRING("center")) {
        m_value.m_justifyContent =
            JustifyContentValue::CenterJustifyContentValue;
    } else if (STRING_VALUE_IS_STRING("space-between")) {
        m_value.m_justifyContent =
            JustifyContentValue::SpaceBetweenJustifyContentValue;
    } else if (STRING_VALUE_IS_STRING("space-around")) {
        m_value.m_justifyContent =
            JustifyContentValue::SpaceAroundJustifyContentValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueUnitAlignItem(const CSSTokenValue& value)
{
    m_valueKind = CSSStyleValuePair::ValueKind::AlignItemValueKind;
    if (STRING_VALUE_IS_STRING("flex-start")) {
        m_value.m_alignItem = AlignItemValue::FlexStartAlignItemValue;
    } else if (STRING_VALUE_IS_STRING("flex-end")) {
        m_value.m_alignItem = AlignItemValue::FlexEndAlignItemValue;
    } else if (STRING_VALUE_IS_STRING("center")) {
        m_value.m_alignItem = AlignItemValue::CenterAlignItemValue;
    } else if (STRING_VALUE_IS_STRING("baseline")) {
        m_value.m_alignItem = AlignItemValue::BaselineAlignItemValue;
    } else if (STRING_VALUE_IS_STRING("stretch")) {
        m_value.m_alignItem = AlignItemValue::StretchAlignItemValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueAlignItems(Document* document,
                                              const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    return updateValueUnitAlignItem(value);
}

bool CSSStyleValuePair::updateValueAlignSelf(Document* document,
                                             const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    if (STRING_VALUE_IS_STRING("auto")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Auto;
        return true;
    }
    return updateValueAlignItems(document, tokens);
}

bool CSSStyleValuePair::updateValueAlignContent(Document* document,
                                                const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::AlignContentValueKind;
    if (STRING_VALUE_IS_STRING("flex-start")) {
        m_value.m_alignContent = AlignContentValue::FlexStartAlignContentValue;
    } else if (STRING_VALUE_IS_STRING("flex-end")) {
        m_value.m_alignContent = AlignContentValue::FlexEndAlignContentValue;
    } else if (STRING_VALUE_IS_STRING("center")) {
        m_value.m_alignContent = AlignContentValue::CenterAlignContentValue;
    } else if (STRING_VALUE_IS_STRING("space-between")) {
        m_value.m_alignContent =
            AlignContentValue::SpaceBetweenAlignContentValue;
    } else if (STRING_VALUE_IS_STRING("space-around")) {
        m_value.m_alignContent =
            AlignContentValue::SpaceAroundAlignContentValue;
    } else if (STRING_VALUE_IS_STRING("stretch")) {
        m_value.m_alignContent = AlignContentValue::StretchAlignContentValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueFlexGrow(Document* document,
                                            const CSSTokenVector& tokens)
{
    return updateValueNumber(tokens, 0);
}

bool CSSStyleValuePair::updateValueUnitFlexGrow(const CSSTokenValue& value)
{
    return updateValueUnitNumber(value, 0);
}

bool CSSStyleValuePair::updateValueFlexShrink(Document* document,
                                              const CSSTokenVector& tokens)
{
    return updateValueNumber(tokens, 0);
}

bool CSSStyleValuePair::updateValueUnitFlexShrink(const CSSTokenValue& value)
{
    return updateValueUnitNumber(value, 0);
}

bool CSSStyleValuePair::updateValueFlexBasis(Document* document,
                                             const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    return updateValueUnitFlexBasis(value);
}

bool CSSStyleValuePair::updateValueUnitFlexBasis(const CSSTokenValue& value)
{
    if (STRING_VALUE_IS_STRING("content")) {
        m_valueKind = CSSStyleValuePair::ValueKind::FlexBasisValueKind;
        m_value.m_flexBasis = FlexBasisValue::ContentFlexBasisValue;
    } else {
        return updateValueUnitLengthOrCalc(value,
                                           CSSPropertyParser::AllowPercent |
                                               CSSPropertyParser::AllowAuto);
    }
    return true;
}
bool CSSStyleValuePair::updateValueTransform(const CSSTokenVector& tokens,
                                             bool canIgnoreUnit)
{
    if (tokens.size() == 1 && tokens[0].equals("none")) {
        m_valueKind = CSSStyleValuePair::ValueKind::None;
        return true;
    } else {
        m_valueKind = CSSStyleValuePair::ValueKind::TransformFunctions;
        CSSTransformFunction::Kind fkind;
        m_value.m_transforms = new CSSTransformFunctions();

        for (unsigned i = 0; i < tokens.size(); i++) {
            CSSPropertyParser parser((char*)tokens[i].data(),
                                     tokens[i].length());
            bool res = parser.consumeString(0) && parser.consumeIfNext('(');
            if (!res) {
                return false;
            }
            String* name = parser.parsedString();

            enum TransformUnit {
                Number,           // <number>
                Angle,            // <angle>
                TranslationValue, // <translation-value>: percentage or length
                Length            // <length>: length
            };

            TransformUnit units[16] = {
                Number,
            };

            int minArgCnt = 1, maxArgCnt = 1;
            if (name->equals("matrix")) {
                fkind = CSSTransformFunction::Kind::Matrix;
                units[0] = units[1] = units[2] = units[3] = units[4] =
                    units[5] = Number;
                minArgCnt = maxArgCnt = 6;
            } else if (name->equals("matrix3d")) {
                fkind = CSSTransformFunction::Kind::Matrix3D;
                units[0] = units[1] = units[2] = units[3] = units[4] =
                    units[5] = Number;
                units[6] = units[7] = units[8] = units[9] = units[10] =
                    units[11] = Number;
                units[12] = units[13] = units[14] = units[15] = Number;
                minArgCnt = maxArgCnt = 16;
            } else if (name->equals("translate")) {
                fkind = CSSTransformFunction::Kind::Translate;
                maxArgCnt = 2;
                units[0] = units[1] = TranslationValue;
            } else if (name->equals("translate3d")) {
                fkind = CSSTransformFunction::Kind::Translate3D;
                maxArgCnt = 3;
                units[0] = units[1] = TranslationValue;
                units[2] = Length;
            } else if (name->equals("translatex")) {
                fkind = CSSTransformFunction::Kind::TranslateX;
                units[0] = TranslationValue;
            } else if (name->equals("translatey")) {
                fkind = CSSTransformFunction::Kind::TranslateY;
                units[0] = TranslationValue;
            } else if (name->equals("translatez")) {
                fkind = CSSTransformFunction::Kind::TranslateZ;
                units[0] = Length;
            } else if (name->equals("scale")) {
                maxArgCnt = 2;
                fkind = CSSTransformFunction::Kind::Scale;
                units[0] = units[1] = Number;
            } else if (name->equals("scale3d")) {
                maxArgCnt = 3;
                fkind = CSSTransformFunction::Kind::Scale3D;
                units[0] = units[1] = units[2] = Number;
            } else if (name->equals("scalex")) {
                fkind = CSSTransformFunction::Kind::ScaleX;
                units[0] = Number;
            } else if (name->equals("scaley")) {
                fkind = CSSTransformFunction::Kind::ScaleY;
                units[0] = Number;
            } else if (name->equals("rotate")) {
                fkind = CSSTransformFunction::Kind::Rotate;
                units[0] = Angle;
            } else if (name->equals("rotate3d")) {
                fkind = CSSTransformFunction::Kind::Rotate3D;
                minArgCnt = 4;
                maxArgCnt = 4;
                units[0] = units[1] = units[2] = Number;
                units[3] = Angle;
            } else if (name->equals("skew")) {
                fkind = CSSTransformFunction::Kind::Skew;
                maxArgCnt = 2;
                units[0] = units[1] = Angle;
            } else if (name->equals("skewx")) {
                fkind = CSSTransformFunction::Kind::SkewX;
                units[0] = Angle;
            } else if (name->equals("skewy")) {
                fkind = CSSTransformFunction::Kind::SkewY;
                units[0] = Angle;
            } else if (name->equals("perspective")) {
                fkind = CSSTransformFunction::Kind::Perspective;
                units[0] = Number;
            } else {
                return false;
            }

            ValueList* values =
                new ValueList(ValueList::Separator::CommaSeparator);
            int idx = -1;
            uint8_t option = 0;
            if (canIgnoreUnit) {
                option = CSSPropertyParser::AllowWithoutUnit;
            }
            for (idx = 0; idx < maxArgCnt; idx++) {
                parser.consumeWhitespaces();
                parser.consumeString(CSSPropertyParser::AllowDot |
                                     CSSPropertyParser::AllowPlus |
                                     CSSPropertyParser::AllowNegative |
                                     CSSPropertyParser::AllowPercent);

                TransformUnit unit = units[idx];
                auto str = parser.parsedString()->toUTF8NonGCString();
                CSSStyleValuePair ret;
                if (unit == Number &&
                    ret.updateValueUnitNumber(
                        CSSTokenValue(str.c_str()),
                        CSSPropertyParser::AllowNegative)) {
                    values->emplace_back(ret);
                } else if (unit == Angle &&
                           ret.updateValueUnitAngleOrCalc(
                               CSSTokenValue(str.c_str()),
                               option | CSSPropertyParser::AllowNegative)) {
                    values->emplace_back(ret);
                } else if (unit == Length &&
                           ret.updateValueUnitLengthOrCalc(
                               CSSTokenValue(str.c_str()),
                               option | CSSPropertyParser::AllowNegative)) {
                    values->emplace_back(ret);
                } else if (unit == TranslationValue &&
                           ret.updateValueUnitLengthOrCalc(
                               CSSTokenValue(str.c_str()),
                               option | CSSPropertyParser::AllowNegative |
                                   CSSPropertyParser::AllowPercent)) {
                    values->emplace_back(ret);
                } else {
                    return false;
                }
                parser.consumeWhitespaces();
                if (!parser.consumeIfNext(',')) {
                    break;
                }
            }
            if (!(parser.consumeIfNext(')') && parser.isEnd()) ||
                idx + 1 < minArgCnt) {
                return false;
            }
            m_value.m_transforms->emplace_back(fkind, values);
        }
    }
    return true;
}

bool CSSStyleValuePair::updateValueFill(Document* document,
                                        const CSSTokenVector& tokens)
{
    if (tokens.size() == 1) {
        const CSSTokenValue& value = tokens[0];
        if (STRING_VALUE_IS_STRING("none")) {
            m_valueKind = CSSStyleValuePair::ValueKind::None;
            return true;
        }
    }
    return updateValueUnitColor(tokens[0]);
}

bool CSSStyleValuePair::updateValueFillOpacity(Document* document,
                                               const CSSTokenVector& tokens)
{
    return updateValueOpacity(document, tokens);
}

bool CSSStyleValuePair::updateValueFillRule(Document* document,
                                            const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::FillRuleValueKind;
    if (STRING_VALUE_IS_STRING("nonzero")) {
        m_value = FillRuleValue::FillRuleNonZero;
    } else if (STRING_VALUE_IS_STRING("evenodd")) {
        m_value = FillRuleValue::FillRuleEvenOdd;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueStroke(Document* document,
                                          const CSSTokenVector& tokens)
{
    if (tokens.size() == 1) {
        const CSSTokenValue& value = tokens[0];
        if (STRING_VALUE_IS_STRING("none")) {
            m_valueKind = CSSStyleValuePair::ValueKind::None;
            return true;
        }
    }

    return updateValueUnitColor(tokens[0]);
}

bool CSSStyleValuePair::updateValueStrokeWidth(Document* document,
                                               const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitLengthOrCalc(
        tokens[0], CSSPropertyParser::ParserOption::AllowPercent |
                       CSSPropertyParser::ParserOption::AllowWithoutUnit);
}

bool CSSStyleValuePair::updateValueX(Document* document,
                                     const CSSTokenVector& tokens)
{
    return updateValueLength(tokens, CSSPropertyParser::AllowPercent);
}

bool CSSStyleValuePair::updateValueY(Document* document,
                                     const CSSTokenVector& tokens)
{
    return updateValueLength(tokens, CSSPropertyParser::AllowPercent);
}

bool CSSStyleValuePair::updateValueR(Document* document,
                                     const CSSTokenVector& tokens)
{
    return updateValueLength(tokens, CSSPropertyParser::AllowPercent);
}

bool CSSStyleValuePair::updateValueRX(Document* document,
                                      const CSSTokenVector& tokens)
{
    return updateValueLength(tokens, CSSPropertyParser::AllowPercent);
}

bool CSSStyleValuePair::updateValueRY(Document* document,
                                      const CSSTokenVector& tokens)
{
    return updateValueLength(tokens, CSSPropertyParser::AllowPercent);
}

bool CSSStyleValuePair::updateValueCX(Document* document,
                                      const CSSTokenVector& tokens)
{
    return updateValueLength(tokens, CSSPropertyParser::AllowPercent);
}

bool CSSStyleValuePair::updateValueCY(Document* document,
                                      const CSSTokenVector& tokens)
{
    return updateValueLength(tokens, CSSPropertyParser::AllowPercent);
}

void CSSStyleDeclaration::setD(const char* value, size_t len, bool isImportant)
{
    if (len == 0) {
        removeCSSValuePair(CSSStyleValuePair::KeyKind::D);
        return;
    }

    if (VALUE_IS_INHERIT()) {
        CSSStyleValuePair pair;
        pair.setFlagImportant(isImportant);
        pair.setKeyKind(CSSStyleValuePair::KeyKind::D);
        pair.setValueKind(CSSStyleValuePair::ValueKind::Inherit);
        addCSSValuePair(CSSStyleValuePair::KeyKind::D, pair);
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, len, "()", 2, true);

    // path, (, "data", )
    if (tokens.size() != 4) {
        removeCSSValuePair(CSSStyleValuePair::KeyKind::D);
        return;
    }
    if (tokens[0] != "path" || tokens[1] != "(" || tokens[3] != ")" ||
        tokens[2].length() < 2) {
        removeCSSValuePair(CSSStyleValuePair::KeyKind::D);
        return;
    }
    if ((tokens[2][0] == '"' && tokens[2].back() == '"') ||
        (tokens[2][0] == '\'' && tokens[2].back() == '\'')) {
    } else {
        removeCSSValuePair(CSSStyleValuePair::KeyKind::D);
        return;
    }

    // TODO validate path data
    CSSStyleValuePair pair;
    pair.setFlagImportant(isImportant);
    pair.setKeyKind(CSSStyleValuePair::KeyKind::D);
    pair.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
    pair.setStringValue(
        String::fromUTF8(tokens[2].data() + 1, tokens[2].length() - 1));
    addCSSValuePair(CSSStyleValuePair::KeyKind::D, pair);
}

String* CSSStyleDeclaration::D()
{
    for (unsigned i = 0; i < m_cssValues.size(); i++) {
        if (m_cssValues[i].keyKind() == CSSStyleValuePair::KeyKind::D) {
            StringBuilder sb;
            sb.appendString("path('");
            sb.appendString(m_cssValues[i].toString());
            sb.appendString(")");
            return sb.finalize();
        }
    }
    return String::emptyString;
}

bool CSSStyleValuePair::updateValueOutlineColor(Document* document,
                                                const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitColor(tokens[0]);
}

bool CSSStyleValuePair::updateValueOutlineWidth(Document* document,
                                                const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitBorderWidth(tokens[0]);
}

bool CSSStyleValuePair::updateValueOutlineStyle(Document* document,
                                                const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitBorderStyle(tokens[0]);
}

bool CSSStyleValuePair::updateValueOutlineOffset(Document* document,
                                                 const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitLengthOrCalc(
        tokens[0], CSSPropertyParser::ParserOption::AllowNegative);
}

String* CSSStyleDeclaration::Outline()
{
    String* width = OutlineWidth();
    String* style = OutlineStyle();
    String* color = OutlineColor();
    return BorderString(width, false, style, false, color, false);
}

void CSSStyleDeclaration::setOutline(const char* value, size_t length,
                                     bool isImportant)
{
    if (length == 0) {
        removeCSSValuePair(CSSStyleValuePair::KeyKind::OutlineWidth);
        removeCSSValuePair(CSSStyleValuePair::KeyKind::OutlineStyle);
        removeCSSValuePair(CSSStyleValuePair::KeyKind::OutlineColor);
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, length);

    CSSStyleValuePair v, width, style, color;
    if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::OutlineWidth, v);
        addCSSValuePair(CSSStyleValuePair::KeyKind::OutlineStyle, v);
        addCSSValuePair(CSSStyleValuePair::KeyKind::OutlineColor, v);
    } else if (parseBorderShorthand(tokens, &width, &style, &color)) {
        width.setFlagImportant(isImportant);
        style.setFlagImportant(isImportant);
        color.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::OutlineWidth, width);
        addCSSValuePair(CSSStyleValuePair::KeyKind::OutlineStyle, style);
        addCSSValuePair(CSSStyleValuePair::KeyKind::OutlineColor, color);
    }
}

bool CSSStyleValuePair::updateValueSrc(const CSSTokenVector& tokens)
{
    FontFaceSrcData* src = new FontFaceSrcData;
    int mode = 0;
    // 0 -> expect url,local function
    // 1 -> expect format or comma
    // 2 -> expect comma

    FontFaceSrcData::LoadFrom loadFrom;
    FontFaceSrcData::Format format;
    String* srcStr;

    for (size_t i = 0; i < tokens.size(); i++) {
        const auto& token = tokens[i];
        if (mode == 0) {
            CSSTokenValue s;
            if (token.startsWith("url(")) {
                loadFrom = FontFaceSrcData::URL;
                s = token.substring(4, token.size() - 5);
                mode = 1;
            } else if (token.startsWith("local(")) {
                loadFrom = FontFaceSrcData::Local;
                s = token.substring(6, token.size() - 7);
                mode = 1;
            } else {
                return false;
            }
            if (s.length() > 2 && s.startsWith("\"")) {
                s = s.substring(1, s.length() - 2);
            } else if (s.length() > 2 && s.startsWith("'")) {
                s = s.substring(1, s.length() - 2);
            }

            srcStr = String::fromUTF8(s.data(), s.length());

            if (i == tokens.size() - 1) {
                src->m_data.push_back(std::make_tuple(
                    srcStr, loadFrom, FontFaceSrcData::NotSpecified));
            }
        } else if (mode == 1) {
            if (token == ",") {
                src->m_data.push_back(std::make_tuple(
                    srcStr, loadFrom, FontFaceSrcData::NotSpecified));
                mode = 0;
                continue;
            } else if (!token.startsWith("format(")) {
                return false;
            }
            CSSTokenValue s = token;
            std::transform(s.begin(), s.end(), s.begin(), ::tolower);

            if (s.find("truetype") != std::string::npos) {
                format = FontFaceSrcData::Format::TrueType;
            } else if (s.find("woff2") != std::string::npos) {
                format = FontFaceSrcData::Format::WOFF2;
            } else if (s.find("woff") != std::string::npos) {
                format = FontFaceSrcData::Format::WOFF;
            } else if (s.find("opentype") != std::string::npos) {
                format = FontFaceSrcData::Format::OpenType;
            } else if (s.find("embedded-opentype") != std::string::npos) {
                format = FontFaceSrcData::Format::EmbeddedOpenType;
            } else if (s.find("svg") != std::string::npos) {
                format = FontFaceSrcData::Format::SVG;
            } else {
                format = FontFaceSrcData::Format::Unknown;
            }

            if (i == tokens.size() - 1) {
                src->m_data.push_back(
                    std::make_tuple(srcStr, loadFrom, format));
            }
            mode = 2;
        } else if (mode == 2) {
            if (token == ",") {
                src->m_data.push_back(
                    std::make_tuple(srcStr, loadFrom, format));
                mode = 0;
            } else {
                return false;
            }
        }
    }

    if (!src->data().size()) {
        return false;
    }

    setKeyKind(KeyKind::Src);
    setFontFaceSrcData(src);

    return true;
}

static void removeFlexCSSValuePairs(CSSStyleDeclaration* target)
{
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::FlexGrow);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::FlexShrink);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::FlexBasis);
}

static void addFlexCSSValuePairs(CSSStyleDeclaration* target,
                                 CSSStyleValuePair flexGrow,
                                 CSSStyleValuePair flexShrink,
                                 CSSStyleValuePair flexBasis)
{
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::FlexGrow, flexGrow);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::FlexShrink, flexShrink);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::FlexBasis, flexBasis);
}

static bool parseFlexShorthand(const CSSTokenVector& tokens,
                               CSSStyleValuePair* flexGrow,
                               CSSStyleValuePair* flexShrink,
                               CSSStyleValuePair* flexBasis)
{
    size_t len = tokens.size();
    if (len < 1) {
        return false;
    }

    flexGrow->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    flexShrink->setNumberValue(1);
    flexBasis->setPercentageValue(0);

    bool hasFlexGrow = false, hasFlexShrink = false, hasFlexBasis = false;
    CSSStyleValuePair temp;
    size_t pos = 0;

    while (pos < len) {
        const CSSTokenValue& token = tokens[pos++];
        if (!hasFlexGrow && temp.updateValueUnitFlexGrow(token)) {
            hasFlexGrow = true;
            *flexGrow = temp;
            continue;
        } else if (hasFlexGrow && !hasFlexShrink &&
                   temp.updateValueUnitFlexShrink(token)) {
            hasFlexShrink = true;
            *flexShrink = temp;
            continue;
        } else if (temp.updateValueUnitFlexBasis(token)) {
            hasFlexBasis = true;
            *flexBasis = temp;
            break;
        }
        return false;
    }

    return hasFlexGrow || hasFlexShrink || hasFlexBasis;
}

void CSSStyleDeclaration::setFlex(const char* str, size_t length,
                                  bool isImportant)
{
    if (length == 0) {
        removeFlexCSSValuePairs(this);
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, str, length);
    const CSSTokenValue& value = tokens[0];

    // TODO comma separation
    CSSStyleValuePair v, flexGrow, flexShrink, flexBasis;
    float f;
    if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        addFlexCSSValuePairs(this, v, v, v);
    } else if (STRING_VALUE_IS_AUTO()) {
        flexGrow.setFlagImportant(isImportant);
        flexShrink.setFlagImportant(isImportant);
        flexBasis.setFlagImportant(isImportant);
        flexGrow.setValueKind(CSSStyleValuePair::ValueKind::Number);
        flexGrow.setValue(1.0f);
        flexShrink.setValueKind(CSSStyleValuePair::ValueKind::Number);
        flexShrink.setValue(1.0f);
        flexBasis.setValueKind(CSSStyleValuePair::ValueKind::Auto);
        addFlexCSSValuePairs(this, flexGrow, flexShrink, flexBasis);
    } else if (STRING_VALUE_IS_NONE()) {
        flexGrow.setFlagImportant(isImportant);
        flexShrink.setFlagImportant(isImportant);
        flexBasis.setFlagImportant(isImportant);
        flexGrow.setValueKind(CSSStyleValuePair::ValueKind::Number);
        flexGrow.setValue(0.0f);
        flexShrink.setValueKind(CSSStyleValuePair::ValueKind::Number);
        flexShrink.setValue(0.0f);
        flexBasis.setValueKind(CSSStyleValuePair::ValueKind::Auto);
        addFlexCSSValuePairs(this, flexGrow, flexShrink, flexBasis);
    } else if (parseFlexShorthand(tokens, &flexGrow, &flexShrink, &flexBasis)) {
        flexGrow.setFlagImportant(isImportant);
        flexShrink.setFlagImportant(isImportant);
        flexBasis.setFlagImportant(isImportant);
        addFlexCSSValuePairs(this, flexGrow, flexShrink, flexBasis);
    }
}

String* CSSStyleDeclaration::Flex()
{
    String* flexGrow = FlexGrow();
    String* flexShrink = FlexShrink();
    String* flexBasis = FlexBasis();

    String* space = String::spaceString;
    StringBuilder builder;
    builder.appendString(flexGrow);
    builder.appendString(space);
    builder.appendString(flexShrink);
    builder.appendString(space);
    builder.appendString(flexBasis);

    return builder.finalize();
}

bool CSSStyleValuePair::updateValueMaskImage(Document* document,
                                             const CSSTokenVector& tokens)
{
    // none | <image> | <url>
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    if (updateValueUnitUrlOrNone(value) || updateValueUnitGradient(value)) {
        return true;
    }

    return false;
}

bool CSSStyleValuePair::updateValueMaskSize(Document* document,
                                            const CSSTokenVector& tokens)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return false;
}

String* CSSStyleDeclaration::ListStyle()
{
    String* t = ListStyleType();
    String* p = ListStylePosition();
    String* i = ListStyleImage();

    if (!t->length() || !p->length() || !i->length()) {
        return String::emptyString;
    }

    bool isTGlobal = (t == String::initialString) ||
                     (t == String::inheritString) || (t == String::unsetString);
    bool isPGlobal = (p == String::initialString) ||
                     (p == String::inheritString) || (p == String::unsetString);
    bool isIGlobal = (i == String::initialString) ||
                     (i == String::inheritString) || (i == String::unsetString);

    if (isTGlobal && isPGlobal && isIGlobal) {
        if (t == p && p == i) {
            // e.g. initial initial initial -> intial
            // e.g. inherit inherit inherit -> inherit
            // e.g. unset unset unset -> unset
            return t;
        }
        return String::emptyString;
    }

    // Sequence: position image type
    StringBuilder builder;
    builder.appendString(p);
    builder.appendString(String::spaceString);
    builder.appendString(i);
    builder.appendString(String::spaceString);
    builder.appendString(t);
    return builder.finalize();
}

static bool parseListStyleType(Document* document, const CSSTokenValue& value,
                               CSSStyleValuePair* pair)
{
    String* parsed = String::emptyString;
    if (STRING_VALUE_IS_NONE()) {
        pair->setValueKind(CSSStyleValuePair::None);
    } else if (CSSPropertyParser::parseContentString(value.data(),
                                                     value.length(), &parsed)) {
        pair->setValueKind(CSSStyleValuePair::StringValueKind);
        pair->setValue(parsed);
    } else {
        String* customIdent = String::fromUTF8(value.data(), value.length());
        if (!CSSPropertyParser::stringIsIdent(customIdent)) {
            return false;
        }
        AtomicString aCustomIdent =
            AtomicString::createAtomicString(document->starFish(), customIdent);
        pair->setAtomicStringValue(aCustomIdent);
    }
    return true;
}

static bool parseListStylePosition(const CSSTokenValue& value,
                                   CSSStyleValuePair* pair)
{
    pair->setValueKind(CSSStyleValuePair::ListStylePositionValueKind);
    if (STRING_VALUE_IS_STRING("inside")) {
        pair->setValue(ListStylePositionValue::ListStylePositionInside);
        return true;
    }
    if (STRING_VALUE_IS_STRING("outside")) {
        pair->setValue(ListStylePositionValue::ListStylePositionOutside);
        return true;
    }
    return false;
}

static bool parseListStyleImage(const CSSTokenValue& value,
                                CSSStyleValuePair* pair)
{
    if (STRING_VALUE_IS_NONE()) {
        pair->setValueKind(CSSStyleValuePair::None);
        return true;
    }
    return CSSPropertyParser::parseUrl(value.data(), pair);
}

static bool parseListStyleShorhand(Document* document,
                                   const CSSTokenVector& tokens,
                                   CSSStyleValuePair* type,
                                   CSSStyleValuePair* position,
                                   CSSStyleValuePair* image)
{
    size_t size = tokens.size();
    if (size == 1 && tokens[0].equals("none")) {
        type->setValueKind(CSSStyleValuePair::None);
        image->setValueKind(CSSStyleValuePair::None);
        position->setValueKind(CSSStyleValuePair::ListStylePositionValueKind);
        position->setValue(ListStylePositionValue::ListStylePositionOutside);
        return true;
    }
    if (size != 3) {
        return false;
    }
    bool foundType = false, foundPos = false, foundImg = false;
    size_t noneCount = 0;
    // NOTE Consider corner cases
    //   none none inside       (O)
    //   url(...) inside inside (O)
    //   none url(...) inside   (O)
    for (size_t i = 0; i < size; i++) {
        const CSSTokenValue& token = tokens[i];
        if (!foundPos && parseListStylePosition(token, position)) {
            foundPos = true;
            continue;
        }
        if (!foundImg && parseListStyleImage(token, image)) {
            if (image->valueKind() != CSSStyleValuePair::None) {
                foundImg = true;
            } else {
                noneCount++;
            }
            continue;
        }
        if (!foundType && parseListStyleType(document, token, type)) {
            foundType = true;
            continue;
        }
        return false;
    }
    if (!foundImg && noneCount > 0) {
        noneCount--;
        image->setValueKind(CSSStyleValuePair::None);
        foundImg = true;
    }
    if (!foundType && noneCount > 0) {
        noneCount--;
        type->setValueKind(CSSStyleValuePair::None);
        foundType = true;
    }
    return foundPos && foundImg && foundType;
}

void CSSStyleDeclaration::setListStyle(const char* value, size_t len,
                                       bool isImportant)
{
    if (len == 0) {
        removeCSSValuePair(CSSStyleValuePair::KeyKind::ListStyleType);
        removeCSSValuePair(CSSStyleValuePair::KeyKind::ListStylePosition);
        removeCSSValuePair(CSSStyleValuePair::KeyKind::ListStyleImage);
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, len, ",", 1);
    CSSStyleValuePair c, t, p, i;
    if (c.updateValueCommon(tokens)) {
        c.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::ListStyleType, c);
        addCSSValuePair(CSSStyleValuePair::KeyKind::ListStylePosition, c);
        addCSSValuePair(CSSStyleValuePair::KeyKind::ListStyleImage, c);
    } else if (parseListStyleShorhand(m_node->document(), tokens, &t, &p, &i)) {
        t.setFlagImportant(isImportant);
        p.setFlagImportant(isImportant);
        i.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::ListStyleType, t);
        addCSSValuePair(CSSStyleValuePair::KeyKind::ListStylePosition, p);
        addCSSValuePair(CSSStyleValuePair::KeyKind::ListStyleImage, i);
    }
}

bool CSSStyleValuePair::updateValueListStyleType(Document* document,
                                                 const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return parseListStyleType(document, tokens[0], this);
}

bool CSSStyleValuePair::updateValueListStylePosition(
    Document* document, const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return parseListStylePosition(tokens[0], this);
}

bool CSSStyleValuePair::updateValueListStyleImage(Document* document,
                                                  const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return parseListStyleImage(tokens[0], this);
}

static bool parseCounterPairList(Document* document,
                                 const CSSTokenVector& tokens,
                                 int32_t defaultValue, CSSStyleValuePair* pair)
{
    size_t size = tokens.size();
    if (size == 1 && tokens[0].equals("none")) {
        pair->setValueKind(CSSStyleValuePair::None);
        return true;
    }
    // NOTE Use temp containers to prevent making unnecessary AtomicStrings
    GCVector<String*> tempIdent;
    GCAtomicVector<int32_t> tempInt;
    bool nameTurn = true;
    for (size_t i = 0; i < size; i++, nameTurn = !nameTurn) {
        const CSSTokenValue& token = tokens[i];
        if (nameTurn) {
            String* ident = String::emptyString;
            if (CSSPropertyParser::parseCustomIdent(token.data(),
                                                    token.length(), &ident) &&
                !ident->equals("none")) {
                tempIdent.push_back(ident);
            } else {
                return false;
            }
        } else {
            int32_t number;
            if (CSSPropertyParser::parseInt32(
                    token.data(), CSSPropertyParser::AllowNegative, number)) {
                tempInt.push_back(number);
            } else {
                tempInt.push_back(defaultValue);
                i--;
            }
        }
    }
    size = tempIdent.size();
    size_t intSize = tempInt.size();
    ValueList* list = new ValueList(ValueList::Separator::SpaceSeparator);
    for (size_t i = 0; i < size; i++) {
        list->emplace_back(CSSStyleValuePair::AtomicStringValueKind,
                           AtomicString::createAtomicString(
                               document->starFish(), tempIdent[i]));
        if (i < intSize) {
            list->emplace_back(CSSStyleValuePair::Int32, tempInt[i]);
        } else {
            list->emplace_back(CSSStyleValuePair::Int32, defaultValue);
        }
    }
    pair->setValueList(list);
    return true;
}

bool CSSStyleValuePair::updateValueCounterReset(Document* document,
                                                const CSSTokenVector& tokens)
{
    return parseCounterPairList(document, tokens, 0, this);
}

bool CSSStyleValuePair::updateValueCounterIncrement(
    Document* document, const CSSTokenVector& tokens)
{
    return parseCounterPairList(document, tokens, 1, this);
}

bool CSSStyleValuePair::updateValueUserSelect(Document* document,
                                              const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    if (STRING_VALUE_IS_STRING("auto")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Auto;
        return true;
    }

    m_valueKind = CSSStyleValuePair::ValueKind::UserSelectValueKind;
    if (STRING_VALUE_IS_STRING("none")) {
        m_value.m_userSelect = UserSelectValue::NoneUserSelectValue;
    } else if (STRING_VALUE_IS_STRING("text")) {
        // TODO: enable the comment below when supporting this value
        // m_value.m_userSelect = UserSelectValue::TextUserSelectValue;
        return false;
    } else if (STRING_VALUE_IS_STRING("contain")) {
        // TODO: enable the comment below when supporting this value
        // m_value.m_userSelect = UserSelectValue::ContainUserSelectValue;
        return false;
    } else if (STRING_VALUE_IS_STRING("all")) {
        // TODO: enable the comment below when supporting this value
        // m_value.m_userSelect = UserSelectValue::AllUserSelectValue;
        return false;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueHyphens(Document* document,
                                           const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    if (STRING_VALUE_IS_STRING("auto")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Auto;
        return true;
    }

    m_valueKind = CSSStyleValuePair::ValueKind::HyphensValueKind;
    if (STRING_VALUE_IS_STRING("none")) {
        m_value.m_hyphens = HyphensValue::NoneHyphensValue;
    } else if (STRING_VALUE_IS_STRING("manual")) {
        // TODO: enable the comment below when supporting this value
        // m_value.m_hyphens = HyphensValue::ManualHyphensValue;
        return false;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueLineBreak(Document* document,
                                             const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    if (STRING_VALUE_IS_STRING("auto")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Auto;
        return true;
    }

    m_valueKind = CSSStyleValuePair::ValueKind::LineBreakValueKind;
    if (STRING_VALUE_IS_STRING("loose")) {
        m_value.m_lineBreak = LineBreakValue::LooseLineBreakValue;
    } else if (STRING_VALUE_IS_STRING("normal")) {
        m_value.m_lineBreak = LineBreakValue::NormalLineBreakValue;
    } else if (STRING_VALUE_IS_STRING("strict")) {
        m_value.m_lineBreak = LineBreakValue::StrictLineBreakValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueWordBreak(Document* document,
                                             const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::WordBreakValueKind;
    if (STRING_VALUE_IS_STRING("normal")) {
        m_value.m_wordBreak = WordBreakValue::NormalWordBreakValue;
    } else if (STRING_VALUE_IS_STRING("break-all")) {
        m_value.m_wordBreak = WordBreakValue::BreakAllWordBreakValue;
        return false; // unsupported yet
    } else if (STRING_VALUE_IS_STRING("keep-all")) {
        m_value.m_wordBreak = WordBreakValue::KeepAllWordBreakValue;
    } else if (STRING_VALUE_IS_STRING("break-word")) {
        m_value.m_wordBreak = WordBreakValue::BreakWordWordBreakValue;
        return false; // unsupported yet
    } else {
        return false;
    }
    return true;
}

#ifdef STARFISH_ENABLE_TEST
void dump(Node* node, unsigned depth)
{
    if (node->isElement()) {
        for (unsigned i = 0; i < depth; i++) {
            printf("  ");
        }

        node->asElement()->dumpStyle();
        printf("\n");
    }

    Node* child = node->firstChild();
    while (child) {
        if (child->isElement()) {
            dump(child, depth + 1);
            if (child->isHTMLIFrameElement() &&
                child->asHTMLIFrameElement()->browsingContext()) {
                dump(child->asHTMLIFrameElement()
                         ->browsingContext()
                         ->document()
                         ->asNode(),
                     depth + 1);
            }
        }
        child = child->nextSibling();
    }
}

void StyleResolver::dumpDOMStyle(Document* document)
{
    dump(document->asNode(), 0);
    printf("\n");
}
#endif
}
