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

#include "StarFishConfig.h"

#include "Style.h"
#include "CSSParser.h"
#include "ComputedStyle.h"
#include "NamedColors.h"

#include "dom/DOM.h"

#include "layout/Frame.h"
#include "layout/FrameTreeBuilder.h"

namespace StarFish {

#define TOKEN_IS_STRING(str) \
    ((memcmp(token, str, strlen(str))) == 0 && strlen(str) == strlen(token))

#define VALUE_IS_STRING(str) \
    ((memcmp(value, str, strlen(str))) == 0 && strlen(str) == strlen(value))

#define VALUE_IS_INHERIT() \
    VALUE_IS_STRING("inherit")

#define VALUE_IS_INITIAL() \
    VALUE_IS_STRING("initial")

#define VALUE_IS_NONE() \
    VALUE_IS_STRING("none")

#define VALUE_IS_AUTO() \
    VALUE_IS_STRING("auto")

#define STRING_VALUE_IS_STRING(str) \
    (value->equals(str))

#define STRING_VALUE_IS_INHERIT() \
    STRING_VALUE_IS_STRING("inherit")

#define STRING_VALUE_IS_INITIAL() \
    STRING_VALUE_IS_STRING("initial")

#define STRING_VALUE_IS_NONE() \
    STRING_VALUE_IS_STRING("none")

#define STRING_VALUE_IS_AUTO() \
    STRING_VALUE_IS_STRING("auto")

static const int strictFontSizeTable[8][8] = {
    { 9, 9, 9, 9, 11, 14, 18, 27 },
    { 9, 9, 9, 10, 12, 15, 20, 30 },
    { 9, 9, 10, 11, 13, 17, 22, 33 },
    { 9, 9, 10, 12, 14, 18, 24, 36 },
    { 9, 10, 12, 13, 16, 20, 26, 39 }, // fixed font default (13)
    { 9, 10, 12, 14, 17, 21, 28, 42 },
    { 9, 10, 13, 15, 18, 23, 30, 45 },
    { 9, 10, 13, 16, 18, 24, 32, 48 } // proportional font default (16)
};
static const float fontSizeFactors[8] = { 0.60f, 0.75f, 0.89f, 1.0f, 1.2f, 1.5f, 2.0f, 3.0f };

static const int fontSizeTableMax = 16;
static const int fontSizeTableMin = 9;

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

static Length parseAbsoluteFontSize(int col)
{
    int mediumSize = DEFAULT_FONT_SIZE;
    int row = -1;
    if (mediumSize >= fontSizeTableMin && mediumSize <= fontSizeTableMax)
        row = mediumSize - fontSizeTableMin;

    if (row >= 0)
        return Length(Length::Fixed, strictFontSizeTable[row][col]);
    else
        return Length(Length::Fixed, fontSizeFactors[col] * mediumSize);
}

static Length convertValueToLength(CSSStyleValuePair::ValueKind kind, CSSStyleValuePair::ValueData data)
{
    if (kind == CSSStyleValuePair::ValueKind::Auto)
        return Length();
    else if (kind == CSSStyleValuePair::ValueKind::Length)
        return data.m_length.toLength();
    else if (kind == CSSStyleValuePair::ValueKind::Percentage)
        return Length(Length::Percent, data.m_floatValue);
    else if (kind == CSSStyleValuePair::ValueKind::Number)
        return Length(Length::Fixed, data.m_floatValue);
    else
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

static String* namedColorToString(NamedColorValue namedColor)
{
    switch (namedColor) {
#define ADD_COLOR_ITEM(name, ...) \
    case NamedColorValue::name##NamedColor: \
        return String::createASCIIString(#name);

        NAMED_COLOR_FOR_EACH(ADD_COLOR_ITEM)
#undef ADD_COLOR_ITEM
    case NamedColorValue::currentColor:
        return String::createASCIIString("currentColor");
    default:
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

static Color namedColorToColor(NamedColorValue namedColor)
{
    switch (namedColor) {
#define ADD_COLOR_ITEM(name, value) \
    case NamedColorValue::name##NamedColor: \
        { \
            char r = (value & 0xff0000) >> 16; \
            char g = (value & 0xff00) >> 8; \
            char b = (value & 0xff); \
            char a = 255; \
            return Color(r, g, b, a); \
        }

        NAMED_COLOR_FOR_EACH(ADD_COLOR_ITEM)
#undef ADD_COLOR_ITEM
    default:
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    return Color();
}

String* CSSStyleValuePair::keyName()
{
    switch (keyKind()) {
#define ADD_CASE_FOR_KEYNAME(Name, name, cssname) \
    case Name: \
        return String::createASCIIString(cssname);

        FOR_EACH_STYLE_ATTRIBUTE(ADD_CASE_FOR_KEYNAME);
#undef ADD_CASE_FOR_KEYNAME
    default:
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

bool CSSStyleValuePair::setValueCommon(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // NOTE: set common value (e.g. initial, inherit, "")
    if (tokens->size() != 1)
        return false;
    const char* value = tokens->at(0)->utf8Data();
    if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else {
        return false;
    }
    return true;
}

static String* BorderString(String* width, String* style, String* color)
{
    String* space = String::spaceString;
    String* sum = String::emptyString;

    if (!width->equals(String::emptyString) && !width->equals(String::initialString)) {
        sum = width;
    }
    if (!style->equals(String::emptyString) && !style->equals(String::initialString)) {
        if (sum->length())
            sum = sum->concat(space);
        sum = sum->concat(style);
    }
    if (!color->equals(String::emptyString) && !color->equals(String::initialString)) {
        if (sum->length())
            sum = sum->concat(space);
        sum = sum->concat(color);
    }

    return sum;
}

URL CSSStyleSheet::url()
{
    if (m_origin->isElement() && m_origin->asElement()->isHTMLElement() && m_origin->asElement()->asHTMLElement()->isHTMLLinkElement()) {
        return m_origin->asElement()->asHTMLElement()->asHTMLLinkElement()->href();
    }
    return URL(m_origin->document()->documentURI());
}

void CSSStyleSheet::parseSheetIfneeds()
{
    if (m_sourceString != String::emptyString) {
        CSSParser parser(m_origin->document());
        parser.parseStyleSheet(m_sourceString, this);
        m_sourceString = String::emptyString;
    }
}

String* CSSStyleDeclaration::generateCSSText()
{
    String* txt = String::emptyString;
    for (size_t i = 0; i < m_cssValues.size(); i++) {
        txt = txt->concat(m_cssValues[i].keyName());
        txt = txt->concat(String::createASCIIString(":"));
        txt = txt->concat(m_cssValues[i].toString());
        txt = txt->concat(String::createASCIIString(";"));
    }
    return txt;
}

String* CSSStyleDeclaration::Border()
{
    String* width = BorderWidth();
    String* style = BorderStyle();
    String* color = BorderColor();
    return BorderString(width, style, color);
}

String* CSSStyleDeclaration::BorderTop()
{
    String* width = BorderTopWidth();
    String* style = BorderTopStyle();
    String* color = BorderTopColor();
    return BorderString(width, style, color);
}

String* CSSStyleDeclaration::BorderRight()
{
    String* width = BorderRightWidth();
    String* style = BorderRightStyle();
    String* color = BorderRightColor();
    return BorderString(width, style, color);
}

String* CSSStyleDeclaration::BorderBottom()
{
    String* width = BorderBottomWidth();
    String* style = BorderBottomStyle();
    String* color = BorderBottomColor();
    return BorderString(width, style, color);
}

String* CSSStyleDeclaration::BorderLeft()
{
    String* width = BorderLeftWidth();
    String* style = BorderLeftStyle();
    String* color = BorderLeftColor();
    return BorderString(width, style, color);
}

static bool parseBorderWidthStyleColor(std::vector<String*, gc_allocator<String*> >* tokens, CSSStyleValuePair* width, CSSStyleValuePair* style, CSSStyleValuePair* color)
{
    size_t len = tokens->size();
    if (len < 1 || len > 3)
        return false;

    bool hasWidth = false, hasStyle = false, hasColor = false;
    CSSStyleValuePair temp;
    std::vector<String*, gc_allocator<String*> > tok;
    for (size_t i = 0; i < len; i++) {
        tok.push_back(tokens->at(i));
        if (temp.updateValueBorderUnitWidth(&tok)) {
            if (hasWidth)
                return false;
            *width = temp;
            hasWidth = true;
        } else if (temp.updateValueBorderUnitStyle(&tok)) {
            if (hasStyle)
                return false;
            *style = temp;
            hasStyle = true;
        } else if (temp.updateValueColor(&tok)) {
            if (hasColor)
                return false;
            *color = temp;
            hasColor = true;
        } else {
            return false;
        }
        tok.pop_back();
    }
    if (!hasWidth) {
        width->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    }
    if (!hasStyle) {
        style->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    }
    if (!hasColor) {
        color->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    }
    return true;
}

void CSSStyleDeclaration::setBorder(String* value)
{
    if (value->length() == 0) {
        setBorderWidth(String::emptyString);
        setBorderStyle(String::emptyString);
        setBorderColor(String::emptyString);
        return;
    }
    std::vector<String*, gc_allocator<String*> > tokens;
    tokenizeCSSValue(&tokens, value);
    CSSStyleValuePair width, style, color;
    if (parseBorderWidthStyleColor(&tokens, &width, &style, &color)) {
        addCSSValuePair(CSSStyleValuePair::KeyKind::BorderTopWidth, width);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BorderTopStyle, style);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BorderTopColor, color);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BorderRightWidth, width);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BorderRightStyle, style);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BorderRightColor, color);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BorderBottomWidth, width);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BorderBottomStyle, style);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BorderBottomColor, color);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BorderLeftWidth, width);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BorderLeftStyle, style);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BorderLeftColor, color);
        notifyNeedsStyleRecalc();
    }
}

#define ADD_SET_BORDER(POS, ...) \
void CSSStyleDeclaration::setBorder##POS(String* value) \
{ \
    if (value->length() == 0) { \
        setBorder##POS##Width(String::emptyString); \
        setBorder##POS##Style(String::emptyString); \
        setBorder##POS##Color(String::emptyString); \
        return; \
    } \
    std::vector<String*, gc_allocator<String*> > tokens; \
    tokenizeCSSValue(&tokens, value); \
    CSSStyleValuePair width, style, color; \
    if (parseBorderWidthStyleColor(&tokens, &width, &style, &color)) { \
        addCSSValuePair(CSSStyleValuePair::KeyKind::Border##POS##Width, width); \
        addCSSValuePair(CSSStyleValuePair::KeyKind::Border##POS##Style, style); \
        addCSSValuePair(CSSStyleValuePair::KeyKind::Border##POS##Color, color); \
        notifyNeedsStyleRecalc(); \
    } \
}

GEN_FOURSIDE(ADD_SET_BORDER)
#undef ADD_SET_BORDER

String* CSSStyleValuePair::toString()
{
    ValueKind value_kind = valueKind();
    if (value_kind == CSSStyleValuePair::ValueKind::Initial)
        return String::initialString;
    else if (value_kind == CSSStyleValuePair::ValueKind::Inherit)
        return String::inheritString;

    switch (keyKind()) {
    case Color:
    case BackgroundColor:
    case BorderTopColor:
    case BorderRightColor:
    case BorderBottomColor:
    case BorderLeftColor: {
        // <color>
        switch (value_kind) {
        case CSSStyleValuePair::ValueKind::ColorValueKind:
            return colorValue().toString();
        case CSSStyleValuePair::ValueKind::NamedColorValueKind:
            return namedColorToString(namedColorValue());
        default:
            return String::emptyString;
        }
    }
    break;
    case PaddingTop:
    case PaddingRight:
    case PaddingBottom:
    case PaddingLeft: {
        // <length> | <percentage>
        return lengthOrPercentageOrKeywordToString();
    }
    break;
    case MarginTop:
    case MarginRight:
    case MarginBottom:
    case MarginLeft:
    case Top:
    case Bottom:
    case Left:
    case Right:
    case Height:
    case Width: {
        // <length> | <percentage> | auto
        return lengthOrPercentageOrKeywordToString();
    }
    break;
    case BackgroundImage:
    case BorderImageSource: {
        // <image> | none
        if (m_valueKind == CSSStyleValuePair::ValueKind::None) {
            return String::fromUTF8("none");
        } else if (m_valueKind == CSSStyleValuePair::ValueKind::UrlValueKind) {
            String* str = String::fromUTF8("url(\"");
            str = str->concat(urlStringValue())->concat(String::fromUTF8("\")"));
            return str;
        } else {
            STARFISH_LOG_INFO("[BackgroundImage/BorderImageSource] Unsupported value");
        }
    }
    break;
    case BackgroundRepeatX:
    case BackgroundRepeatY:
        if (m_valueKind == CSSStyleValuePair::ValueKind::BackgroundRepeatValueKind) {
            if (backgroundRepeatValue() == RepeatRepeatValue)
                return String::fromUTF8("repeat");
            else
                return String::fromUTF8("no-repeat");
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case BackgroundPosition:
        switch (m_valueKind) {
        case CSSStyleValuePair::ValueKind::ValueListKind: {
            String* str = String::emptyString;
            ValueList* vals = multiValue();
            for (unsigned int i = 0; i < vals->size(); i++) {
                CSSStyleValuePair& item = vals->atIndex(i);
                if (item.valueKind() == CSSStyleValuePair::ValueKind::SideValueKind) {
                    if (item.sideValue() == SideValue::LeftSideValue)
                        str = str->concat(String::fromUTF8("left"));
                    else if (item.sideValue() == SideValue::RightSideValue)
                        str = str->concat(String::fromUTF8("right"));
                    else if (item.sideValue() == SideValue::CenterSideValue)
                        str = str->concat(String::fromUTF8("center"));
                    else if (item.sideValue() == SideValue::TopSideValue)
                        str = str->concat(String::fromUTF8("top"));
                    else if (item.sideValue() == SideValue::BottomSideValue)
                        str = str->concat(String::fromUTF8("bottom"));
                } else {
                    str = str->concat(valueToString(item.valueKind(), item.value()));
                }

                if (i < vals->size() - 1) {
                    str = str->concat(String::spaceString);
                }
            }
            return str;
        }
        default:
            return String::emptyString;
        }
        break;
    case BackgroundSize: {
        // [length | percentage | auto]{1, 2} | cover | contain // initial value -> auto
        switch (m_valueKind) {
        case CSSStyleValuePair::ValueKind::Cover:
            return String::fromUTF8("cover");
        case CSSStyleValuePair::ValueKind::Contain:
            return String::fromUTF8("contain");
        case CSSStyleValuePair::ValueKind::Auto:
            return String::fromUTF8("auto");
        case CSSStyleValuePair::ValueKind::ValueListKind: {
            String* str = String::emptyString;
            ValueList* vals = multiValue();
            for (unsigned int i = 0; i < vals->size(); i++) {
                CSSStyleValuePair& item = vals->atIndex(i);
                str = str->concat(valueToString(item.valueKind(), item.value()));
                if (i < vals->size() - 1) {
                    str = str->concat(String::spaceString);
                }
            }
            return str;
        }
        default:
            return String::emptyString;
        }
    }
    case Direction: {
        switch (directionValue()) {
        case LtrDirectionValue:
            return String::fromUTF8("ltr");
        case RtlDirectionValue:
            return String::fromUTF8("rtl");
        default:
            return String::emptyString;
        }
    }
    case LineHeight:
        if (m_valueKind == CSSStyleValuePair::ValueKind::Normal) {
            return String::fromUTF8("normal");
        } else {
            return valueToString();
        }
        break;

    case Position: {
        switch (positionValue()) {
        case PositionValue::StaticPositionValue:
            return String::fromUTF8("static");
        case PositionValue::RelativePositionValue:
            return String::fromUTF8("relative");
        case PositionValue::AbsolutePositionValue:
            return String::fromUTF8("absolute");
        default:
            return String::emptyString;
        }
    }
    case TextDecoration: {
        switch (textDecoration()) {
        case TextDecorationValue::NoneTextDecorationValue:
            return String::fromUTF8("none");
        case TextDecorationValue::UnderLineTextDecorationValue:
            return String::fromUTF8("underline");
        case TextDecorationValue::OverLineTextDecorationValue:
            return String::fromUTF8("overline");
        case TextDecorationValue::LineThroughTextDecorationValue:
            return String::fromUTF8("line-through");
        case TextDecorationValue::BlinkTextDecorationValue:
            return String::fromUTF8("blink");
        default:
            return String::emptyString;
        }
    }
    case FontSize: {
        switch (valueKind()) {
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
            }
        default:
            return lengthOrPercentageOrKeywordToString();
        }
    }
    case FontStyle: {
        switch (valueKind()) {
        case CSSStyleValuePair::ValueKind::FontStyleValueKind:
            switch (fontStyleValue()) {
            case FontStyleValue::NormalFontStyleValue:
                return String::fromUTF8("normal");
            case FontStyleValue::ItalicFontStyleValue:
                return String::fromUTF8("italic");
            case FontStyleValue::ObliqueFontStyleValue:
                return String::fromUTF8("oblique");
            }
        default:
            return lengthOrPercentageOrKeywordToString();
        }
    }
    case FontWeight: {
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
    }
    case Display: {
        switch (valueKind()) {
        case CSSStyleValuePair::ValueKind::DisplayValueKind:
            switch (displayValue()) {
            case DisplayValue::InlineDisplayValue:
                return String::fromUTF8("inline");
            case DisplayValue::BlockDisplayValue:
                return String::fromUTF8("block");
            case DisplayValue::InlineBlockDisplayValue:
                return String::fromUTF8("inline-block");
            case DisplayValue::NoneDisplayValue:
                return String::fromUTF8("none");
            }
        default:
            return lengthOrPercentageOrKeywordToString();
        }
    }
    case BorderImageSlice: {
        switch (valueKind()) {
        case CSSStyleValuePair::ValueKind::ValueListKind: {
            ValueList* values = multiValue();
            String* s = String::emptyString;
            for (unsigned int i = 0; i < values->size(); i++) {
                String* newstr;
                CSSStyleValuePair& item = values->atIndex(i);
                if (item.valueKind() == CSSStyleValuePair::ValueKind::StringValueKind)
                    newstr = String::fromUTF8("fill");
                else
                    newstr = valueToString(item.valueKind(), item.value());
                s = s->concat(newstr);
                if (i != values->size() - 1)
                    s = s->concat(String::spaceString);
            }
            return s;
        }
        default:
            // initial or inherit
            return lengthOrPercentageOrKeywordToString();
        }
    }
    case BorderImageWidth: {
        switch (valueKind()) {
        case CSSStyleValuePair::ValueKind::ValueListKind: {
            ValueList* values = multiValue();
            String* s = String::emptyString;
            for (unsigned int i = 0; i < values->size(); i++) {
                CSSStyleValuePair& item = values->atIndex(i);
                String* newstr = valueToString(item.valueKind(), item.value());
                s = s->concat(newstr);
                if (i != values->size() - 1)
                    s = s->concat(String::spaceString);
            }
            return s;
        }
        default:
            return lengthOrPercentageOrKeywordToString();
        }
    }
    case BorderTopStyle:
    case BorderRightStyle:
    case BorderBottomStyle:
    case BorderLeftStyle: {
        switch (valueKind()) {
        case CSSStyleValuePair::ValueKind::BorderStyleValueKind:
            switch (borderStyleValue()) {
            case BorderStyleValue::NoneBorderStyleValue:
                return String::fromUTF8("none");
            case BorderStyleValue::SolidBorderStyleValue:
                return String::fromUTF8("solid");
            }
        default:
            return String::emptyString;
        }
    }
    case BorderTopWidth:
    case BorderBottomWidth:
    case BorderRightWidth:
    case BorderLeftWidth: {
        switch (valueKind()) {
        case CSSStyleValuePair::ValueKind::BorderWidthValueKind:
            switch (borderWidthValue()) {
            case BorderWidthValue::ThinBorderWidthValue:
                return String::fromUTF8("thin");
            case BorderWidthValue::MediumBorderWidthValue:
                return String::fromUTF8("medium");
            case BorderWidthValue::ThickBorderWidthValue:
                return String::fromUTF8("thick");
            }
        default:
            return lengthOrPercentageOrKeywordToString();
        }
    }
    case TextAlign: {
        switch (sideValue()) {
        case SideValue::NoneSideValue:
            return String::fromUTF8("left");
        case SideValue::LeftSideValue:
            return String::fromUTF8("left");
        case SideValue::RightSideValue:
            return String::fromUTF8("right");
        case SideValue::CenterSideValue:
            return String::fromUTF8("center");
        default:
            return String::emptyString;
        }
    }
    case Visibility: {
        switch (visibility()) {
        case VisibilityValue::VisibleVisibilityValue:
            return String::fromUTF8("visible");
        case VisibilityValue::HiddenVisibilityValue:
            return String::fromUTF8("hidden");
        default:
            return String::emptyString;
        }
    }
    case Opacity: {
        return String::fromFloat(numberValue());
    }
    case Overflow: {
        switch (overflowValue()) {
        case OverflowValue::VisibleOverflow:
            return String::fromUTF8("visible");
        case OverflowValue::HiddenOverflow:
            return String::fromUTF8("hidden");
        default:
            return String::emptyString;
        }
    }
    case ZIndex: {
        if (m_valueKind == CSSStyleValuePair::ValueKind::Auto)
            return String::fromUTF8("auto");
        else if (m_valueKind == CSSStyleValuePair::ValueKind::Int32)
            return String::fromUTF8(std::to_string(int32Value()).c_str());
        break;
    }
    case VerticalAlign: {
        switch (valueKind()) {
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
                // FIXED: NumericVAlignValue cannot be here. (only used in ComputedStyle)
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            default:
                return String::emptyString;
            }
        default:
            return lengthOrPercentageOrKeywordToString();
        }
    }
    case TransformOrigin:
        switch (m_valueKind) {
        case CSSStyleValuePair::ValueKind::ValueListKind: {
            String* str = String::emptyString;
            ValueList* vals = multiValue();
            for (unsigned int i = 0; i < vals->size(); i++) {
                CSSStyleValuePair& item = vals->atIndex(i);
                if (item.valueKind() == CSSStyleValuePair::ValueKind::SideValueKind) {
                    if (item.sideValue() == SideValue::LeftSideValue)
                        str = str->concat(String::fromUTF8("left"));
                    else if (item.sideValue() == SideValue::RightSideValue)
                        str = str->concat(String::fromUTF8("right"));
                    else if (item.sideValue() == SideValue::CenterSideValue)
                        str = str->concat(String::fromUTF8("center"));
                    else if (item.sideValue() == SideValue::TopSideValue)
                        str = str->concat(String::fromUTF8("top"));
                    else if (item.sideValue() == SideValue::BottomSideValue)
                        str = str->concat(String::fromUTF8("bottom"));
                } else {
                    str = str->concat(valueToString(item.valueKind(), item.value()));
                }

                if (i < vals->size() - 1) {
                    str = str->concat(String::spaceString);
                }
            }
            return str;
        }
        default:
            return String::emptyString;
        }
    case Transform: {
        if (m_valueKind == TransformFunctions) {
            CSSTransformFunctions* trans = transformValue();
            String* result = String::emptyString;
            for (unsigned i = 0; i < trans->size(); i++) {
                CSSTransformFunction item = trans->at(i);
                String* itemStr = item.functionName()->concat(String::fromUTF8("("));
                ValueList* values = item.values();
                for (unsigned int j = 0; j < values->size(); j++) {
                    CSSStyleValuePair& subitem = values->atIndex(j);
                    String* newstr = valueToString(subitem.valueKind(), subitem.value());
                    itemStr = itemStr->concat(newstr);
                    if (j != values->size() - 1)
                        itemStr = itemStr->concat(String::fromUTF8(", "));
                    else
                        itemStr = itemStr->concat(String::fromUTF8(") "));
                }
                result = result->concat(itemStr);
            }
            return result;
        } else {
            // initial or inherit or none
            return lengthOrPercentageOrKeywordToString();
        }
    }
    case UnicodeBidi: {
        switch (m_valueKind) {
        case UnicodeBidiValueKind: {
            switch (unicodeBidiValue()) {
            case NormalUnicodeBidiValue:
                return String::fromUTF8("normal");
            case EmbedUnicodeBidiValue:
                return String::fromUTF8("embed");
            }
        }
        default: {
            return String::emptyString;
        }
        }
    }
    default: {
        STARFISH_LOG_INFO("[CSSStyleValuePair::toString] Unsupported spec or something wrong");
        return String::emptyString;
    }
    }
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

void CSSStyleValuePair::setLengthValue(const char* value)
{
    updateValueLengthOrPercent(String::fromUTF8(value), true);
}

String* CSSStyleDeclaration::BackgroundRepeat()
{
    String* repeatX = BackgroundRepeatX();
    String* repeatY = BackgroundRepeatY();

    if (repeatX->equals("repeat") && repeatY->equals("repeat"))
        return String::fromUTF8("repeat");
    else if (repeatX->equals("repeat") && repeatY->equals("no-repeat"))
        return String::fromUTF8("repeat-x");
    else if (repeatX->equals("no-repeat") && repeatY->equals("repeat"))
        return String::fromUTF8("repeat-y");
    else if (repeatX->equals("no-repeat") && repeatY->equals("no-repeat"))
        return String::fromUTF8("no-repeat");
    else if (repeatX->equals("initial") && repeatY->equals("initial"))
        return String::initialString;
    else if (repeatX->equals("inherit") && repeatY->equals("inherit"))
        return String::inheritString;

    return String::emptyString;
}

void CSSStyleDeclaration::setBackgroundRepeat(String* value)
{
    if (value->length() == 0) {
        setBackgroundRepeatX(String::emptyString);
        setBackgroundRepeatY(String::emptyString);
        return;
    }

    std::vector<String*, gc_allocator<String*> > tokens;
    tokenizeCSSValue(&tokens, value);
    if (tokens.size() != 1)
        return;

    CSSStyleValuePair temp;
    if (temp.setValueCommon(&tokens) || temp.updateValueBackgroundRepeatUnit(&tokens)) {
        addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundRepeatX, temp);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundRepeatY, temp);
        return;
    }

    CSSStyleValuePair noRepeat(CSSStyleValuePair::ValueKind::BackgroundRepeatValueKind, BackgroundRepeatValue::NoRepeatRepeatValue);
    CSSStyleValuePair repeat(CSSStyleValuePair::ValueKind::BackgroundRepeatValueKind, BackgroundRepeatValue::RepeatRepeatValue);
    if (tokens[0]->equals("repeat-x")) {
        addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundRepeatX, repeat);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundRepeatY, noRepeat);
    } else if (tokens[0]->equals("repeat-y")) {
        addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundRepeatX, noRepeat);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundRepeatY, repeat);
    }
}

String* CSSStyleDeclaration::Background()
{
    String* result = String::emptyString;
    String* image = BackgroundImage();
    String* repeat = BackgroundRepeat();
    String* color = BackgroundColor();
    String* position = BackgroundPosition();
    String* size = BackgroundSize();

    if (image->length() != 0 && !image->equals(String::initialString))
        result = result->concat(image);
    if (position->length() != 0 && !position->equals(String::initialString)) {
        if (result->length())
            result = result->concat(String::spaceString);
        result = result->concat(position);
    }
    if (size->length() != 0 && !size->equals(String::initialString)) {
        if (result->length())
            result = result->concat(String::spaceString);
        if (position->length() == 0)
            result = result->concat(String::fromUTF8("0% 0%"));
        result = result->concat(String::fromUTF8(" / "));
        result = result->concat(size);
    }
    if (repeat->length() != 0 && !repeat->equals(String::initialString)) {
        if (result->length())
            result = result->concat(String::spaceString);
        result = result->concat(repeat);
    }
    if (color->length() != 0 && !color->equals(String::initialString)) {
        if (result->length())
            result = result->concat(String::spaceString);
        result = result->concat(color);
    }
    return result;
}

void CSSStyleDeclaration::setBackground(String* value)
{
    //  [<'background-color'> || <'background-image'> || <'background-repeat'> || <'background-position'> [/<bg-size>]?] | inherit
    if (value->length() == 0) {
        setBackgroundColor(String::emptyString);
        setBackgroundImage(String::emptyString);
        setBackgroundRepeat(String::emptyString);
        return;
    }
    std::vector<String*, gc_allocator<String*> > tokens;
    tokenizeCSSValue(&tokens, value);

    if (!CSSStyleValuePair::checkEssentialValue(&tokens) && !CSSStyleValuePair::checkInputErrorBackground(&tokens))
        return;

    size_t len = tokens.size();
    const char* token = tokens[0]->utf8Data();
    if (len == 1 && TOKEN_IS_STRING("inherit")) {
        setBackgroundColor(String::inheritString);
        setBackgroundImage(String::inheritString);
        setBackgroundRepeat(String::inheritString);
    } else if (len == 1 && TOKEN_IS_STRING("initial")) {
        setBackgroundColor(String::initialString);
        setBackgroundImage(String::initialString);
        setBackgroundRepeat(String::initialString);
    } else {
        String* colorStr = String::emptyString;
        String* imageStr = String::emptyString;
        String* repeatStr = String::emptyString;
        for (unsigned i = 0; i < tokens.size(); i++) {
            String* str = tokens[i];
            if (CSSPropertyParser::assureColor(str->utf8Data()))
                colorStr = colorStr->concat(str);
            else if (CSSPropertyParser::assureUrlOrNone(str->utf8Data()))
                imageStr = imageStr->concat(str);
            else
                repeatStr = repeatStr->concat(str);
        }
        if (colorStr->length() == 0)
            colorStr = colorStr->concat(String::initialString);
        if (imageStr->length() == 0)
            imageStr = imageStr->concat(String::initialString);
        if (repeatStr->length() == 0)
            repeatStr = repeatStr->concat(String::initialString);
        setBackgroundColor(colorStr);
        setBackgroundImage(imageStr);
        setBackgroundRepeat(repeatStr);
    }
}

void CSSStyleDeclaration::tokenizeCSSValue(std::vector<String*, gc_allocator<String*> >* tokens, String* src)
{
    tokens->clear();

    const char* data = src->utf8Data();
    size_t length = strlen(data);

    std::string str;
    bool inParenthesis = false;
    bool isWhiteSpaceState = false;
    for (size_t i = 0; i < length; i++) {
        if (data[i] == '(') {
            inParenthesis = true;
        } else if (data[i] == ')') {
        }

        if (isWhiteSpaceState && String::isSpaceOrNewline(data[i]))
            continue;

        isWhiteSpaceState = false;
        str += data[i];
        if (inParenthesis && String::isSpaceOrNewline(data[i])) {
            str[str.length()-1] = ' ';
            isWhiteSpaceState = true;
            continue;
        }

        if (!inParenthesis && String::isSpaceOrNewline(data[i])) {
            String* newToken = String::fromUTF8(str.data(), str.length() - 1)->toLower();
            if (!newToken->containsOnlyWhitespace())
                tokens->push_back(newToken);
            isWhiteSpaceState = true;
            str.clear();
        } else if ((inParenthesis && data[i] == ')') || i == length - 1) {
            if (str.length() > 3 && (str[0] == 'u' || str[0] == 'U') && (str[1] == 'r' || str[1] == 'R') && (str[2] == 'l' || str[2] == 'L'))
                tokens->push_back(String::fromUTF8("url")->concat(String::fromUTF8(str.data() + 3, str.length() - 3)));
            else if (str.length() != 0) {
                tokens->push_back(String::fromUTF8(str.data(), str.length())->toLower());
            }
            inParenthesis = false;
            isWhiteSpaceState = true;
            str.clear();
        }
    }
}

bool CSSStyleValuePair::checkEssentialValue(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // NOTE: check if string is the value which all property can have. (e.g. initial, inherit, "")
    if (tokens->size() != 1)
        return false;
    const char* value = tokens->at(0)->utf8Data();
    return CSSPropertyParser::assureEssential(value);
}

bool CSSStyleValuePair::checkInputErrorColor(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // <color>
    size_t size = tokens->size();
    if (size != 1)
        return false;

    const char* token = tokens->at(0)->utf8Data();
    if (!CSSPropertyParser::assureColor(token)) {
        return false;
    }

    // Valid format? : Both Chrome & Firefox produce color string that contains comma with single whitespace.
    String* newstr = tokens->at(0)->replaceAll(String::spaceString, String::emptyString);
    newstr = newstr->replaceAll(String::fromUTF8(","), String::fromUTF8(", "));
    tokens->clear();
    tokens->push_back(newstr);

    return true;
}

bool CSSStyleValuePair::checkInputErrorBackgroundColor(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkInputErrorColor(tokens);
}

bool CSSStyleValuePair::checkInputErrorBackgroundImage(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // none | <image>(=<uri>)
    if (tokens->size() == 1) {
        String* value = (*tokens)[0];
        if (STRING_VALUE_IS_NONE())
            return true;
        return CSSPropertyParser::assureUrl(value->utf8Data());
    }
    return false;
}

bool CSSStyleValuePair::checkInputErrorBackgroundRepeat(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() != 1)
        return false;
    const char* value = (*tokens)[0]->utf8Data();
    return VALUE_IS_STRING("repeat")
        || VALUE_IS_STRING("repeat-x")
        || VALUE_IS_STRING("repeat-y")
        || VALUE_IS_STRING("no-repeat")
        || VALUE_IS_STRING("initial");
}

bool CSSStyleValuePair::checkInputErrorBackground(std::vector<String*, gc_allocator<String*> >* tokens)
{
    //  [<'background-color'> || <'background-image'> || <'background-repeat'>] | inherit
    size_t len = tokens->size();
    if (len == 1) {
        if (checkInputErrorBackgroundColor(tokens)
            || checkInputErrorBackgroundImage(tokens)
            || checkInputErrorBackgroundRepeat(tokens))
            return true;
    } else if (len == 2) {
        std::vector<String*, gc_allocator<String*> > token0;
        std::vector<String*, gc_allocator<String*> > token1;

        token0.assign(tokens->begin(), tokens->end() - 1);
        token1.assign(tokens->begin() + 1, tokens->end());

        if ((checkInputErrorBackgroundColor(&token0) && checkInputErrorBackgroundImage(&token1))
            || (checkInputErrorBackgroundImage(&token0) && checkInputErrorBackgroundColor(&token1)))
            return true;
        else if ((checkInputErrorBackgroundColor(&token0) && checkInputErrorBackgroundRepeat(&token1))
            || (checkInputErrorBackgroundRepeat(&token0) && checkInputErrorBackgroundColor(&token1)))
            return true;
        else if ((checkInputErrorBackgroundImage(&token0) && checkInputErrorBackgroundRepeat(&token1))
            || (checkInputErrorBackgroundRepeat(&token0) && checkInputErrorBackgroundImage(&token1)))
            return true;
    } else if (len == 3) {
        std::vector<String*, gc_allocator<String*> > token0;
        std::vector<String*, gc_allocator<String*> > token1;
        std::vector<String*, gc_allocator<String*> > token2;

        token0.assign(tokens->begin(), tokens->end() - 2);
        token1.assign(tokens->begin() + 1, tokens->end() - 1);
        token2.assign(tokens->begin() + 2, tokens->end());

        if (checkInputErrorBackgroundColor(&token0) && checkInputErrorBackgroundImage(&token1) && checkInputErrorBackgroundRepeat(&token2))
            return true;
        else if (checkInputErrorBackgroundColor(&token0) && checkInputErrorBackgroundRepeat(&token1) && checkInputErrorBackgroundImage(&token2))
            return true;
        else if (checkInputErrorBackgroundRepeat(&token0) && checkInputErrorBackgroundColor(&token1) && checkInputErrorBackgroundImage(&token2))
            return true;
        else if (checkInputErrorBackgroundRepeat(&token0) && checkInputErrorBackgroundImage(&token1) && checkInputErrorBackgroundColor(&token2))
            return true;
        else if (checkInputErrorBackgroundImage(&token0) && checkInputErrorBackgroundColor(&token1) && checkInputErrorBackgroundRepeat(&token2))
            return true;
        else if (checkInputErrorBackgroundImage(&token0) && checkInputErrorBackgroundRepeat(&token1) && checkInputErrorBackgroundColor(&token2))
            return true;
    }

    return false;
}

void CSSStyleDeclaration::notifyNeedsStyleRecalc()
{
    if (m_element)
        m_element->notifyInlineStyleChanged();
}

ComputedStyle* StyleResolver::resolveDocumentStyle(Document* doc)
{
    ComputedStyle* ret = new ComputedStyle();
    ret->m_display = DisplayValue::BlockDisplayValue;
    ret->m_inheritedStyles.m_color = Color(0, 0, 0, 255);
    ret->m_inheritedStyles.m_textAlign = SideValue::NoneSideValue;
    ret->m_inheritedStyles.m_direction = DirectionValue::LtrDirectionValue;
    ret->loadResources(doc);
    return ret;
}

ComputedStyle* StyleResolver::resolveStyle(Element* element, ComputedStyle* parent)
{
    ComputedStyle* ret = new ComputedStyle(parent);

    auto apply = [](const URL& origin, std::vector<CSSStyleValuePair, gc_allocator<CSSStyleValuePair> >& cssValues, ComputedStyle* style, ComputedStyle* parentStyle)
    {
        for (unsigned k = 0; k < cssValues.size(); k++) {
            switch (cssValues[k].keyKind()) {
            case CSSStyleValuePair::KeyKind::Display:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->m_display = parentStyle->m_display;
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->m_display = DisplayValue::InlineDisplayValue;
                } else {
                    STARFISH_ASSERT(CSSStyleValuePair::ValueKind::DisplayValueKind == cssValues[k].valueKind());
                    style->m_display = cssValues[k].displayValue();
                }
                break;
            case CSSStyleValuePair::KeyKind::Position:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->m_position = parentStyle->m_position;
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->m_position = PositionValue::StaticPositionValue;
                } else {
                    STARFISH_ASSERT(CSSStyleValuePair::ValueKind::PositionValueKind == cssValues[k].valueKind());
                    style->m_position = cssValues[k].positionValue();
                }
                break;
            case CSSStyleValuePair::KeyKind::Width:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->m_width = parentStyle->m_width;
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->m_width = Length();
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Auto) {
                    style->m_width = Length();
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Length) {
                    style->m_width = cssValues[k].lengthValue().toLength();
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Percentage) {
                    style->m_width = Length(Length::Percent, cssValues[k].percentageValue());
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
                break;
            case CSSStyleValuePair::KeyKind::Height:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->m_height = parentStyle->m_height;
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->m_height = Length();
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Auto) {
                    style->m_height = Length();
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Length) {
                    style->m_height = cssValues[k].lengthValue().toLength();
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Percentage) {
                    style->m_height = Length(Length::Percent, cssValues[k].percentageValue());
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
                break;
            case CSSStyleValuePair::KeyKind::Color:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setColor(parentStyle->m_inheritedStyles.m_color);
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->setColor(Color(0, 0, 0, 255));
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::ColorValueKind) {
                    style->setColor(cssValues[k].colorValue());
                } else {
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::NamedColorValueKind);
                    if (cssValues[k].namedColorValue() == NamedColorValue::currentColor) {
                        style->m_inheritedStyles.m_color = parentStyle->m_inheritedStyles.m_color;
                    } else {
                        style->setColor(namedColorToColor(cssValues[k].namedColorValue()));
                    }
                }
                break;
            case CSSStyleValuePair::KeyKind::FontSize:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->m_inheritedStyles.m_fontSize = parentStyle->m_inheritedStyles.m_fontSize;
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->m_inheritedStyles.m_fontSize = parseAbsoluteFontSize(3);
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::FontSizeValueKind) {
                    if (cssValues[k].fontSizeValue() == FontSizeValue::XXSmallFontSizeValue) {
                        style->m_inheritedStyles.m_fontSize = parseAbsoluteFontSize(0);
                    } else if (cssValues[k].fontSizeValue() == FontSizeValue::XSmallFontSizeValue) {
                        style->m_inheritedStyles.m_fontSize = parseAbsoluteFontSize(1);
                    } else if (cssValues[k].fontSizeValue() == FontSizeValue::SmallFontSizeValue) {
                        style->m_inheritedStyles.m_fontSize = parseAbsoluteFontSize(2);
                    } else if (cssValues[k].fontSizeValue() == FontSizeValue::MediumFontSizeValue) {
                        style->m_inheritedStyles.m_fontSize = parseAbsoluteFontSize(3);
                    } else if (cssValues[k].fontSizeValue() == FontSizeValue::LargeFontSizeValue) {
                        style->m_inheritedStyles.m_fontSize = parseAbsoluteFontSize(4);
                    } else if (cssValues[k].fontSizeValue() == FontSizeValue::XLargeFontSizeValue) {
                        style->m_inheritedStyles.m_fontSize = parseAbsoluteFontSize(5);
                    } else if (cssValues[k].fontSizeValue() == FontSizeValue::XXLargeFontSizeValue) {
                        style->m_inheritedStyles.m_fontSize = parseAbsoluteFontSize(6);
                    } else if (cssValues[k].fontSizeValue() == FontSizeValue::LargerFontSizeValue) {
                        style->m_inheritedStyles.m_fontSize = Length(Length::Fixed, parentStyle->m_inheritedStyles.m_fontSize.fixed() * 1.2f);
                    } else if (cssValues[k].fontSizeValue() == FontSizeValue::SmallerFontSizeValue) {
                        style->m_inheritedStyles.m_fontSize = Length(Length::Fixed, parentStyle->m_inheritedStyles.m_fontSize.fixed() / 1.2f);
                    }
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Percentage) {
                    float parentComputedFontSize = parentStyle->fontSize().fixed();
                    style->m_inheritedStyles.m_fontSize = Length(Length::Fixed, cssValues[k].percentageValue() * parentComputedFontSize);
                } else {
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Length);
                    style->m_inheritedStyles.m_fontSize = cssValues[k].lengthValue().toLength();
                    style->m_inheritedStyles.m_fontSize.changeToFixedIfNeeded(parentStyle->fontSize(), parentStyle->font());
                }
                break;
            case CSSStyleValuePair::KeyKind::FontStyle:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->m_inheritedStyles.m_fontStyle = parentStyle->m_inheritedStyles.m_fontStyle;
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->m_inheritedStyles.m_fontStyle = FontStyleValue::NormalFontStyleValue;
                } else {
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::FontStyleValueKind);
                    style->m_inheritedStyles.m_fontStyle = cssValues[k].fontStyleValue();
                }
                break;
            case CSSStyleValuePair::KeyKind::FontWeight:
                // <normal> | bold | bolder | lighter | 100 | 200 | 300 | 400 | 500 | 600 | 700 | 800 | 900 | inherit // initial -> normal
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->m_inheritedStyles.m_fontWeight = parentStyle->m_inheritedStyles.m_fontWeight;
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->m_inheritedStyles.m_fontWeight = FontWeightValue::NormalFontWeightValue;
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::FontWeightValueKind) {
                    if (cssValues[k].fontWeightValue() == FontWeightValue::FourHundredsFontWeightValue) {
                        style->m_inheritedStyles.m_fontWeight = FontWeightValue::NormalFontWeightValue;
                    } else if (cssValues[k].fontWeightValue() == FontWeightValue::SevenHundredsFontWeightValue) {
                        style->m_inheritedStyles.m_fontWeight = FontWeightValue::BoldFontWeightValue;
                    } else if (cssValues[k].fontWeightValue() == FontWeightValue::BolderFontWeightValue) {
                        style->m_inheritedStyles.m_fontWeight = bolderWeight(parentStyle->m_inheritedStyles.m_fontWeight);
                    } else if (cssValues[k].fontWeightValue() == FontWeightValue::LighterFontWeightValue) {
                        style->m_inheritedStyles.m_fontWeight = lighterWeight(parentStyle->m_inheritedStyles.m_fontWeight);
                    } else {
                        style->m_inheritedStyles.m_fontWeight = cssValues[k].fontWeightValue();
                    }
                } else {
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::FontStyleValueKind);
                    style->m_inheritedStyles.m_fontStyle = cssValues[k].fontStyleValue();
                }
                break;
            case CSSStyleValuePair::KeyKind::VerticalAlign:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setVerticalAlign(parentStyle->verticalAlign());
                    if (style->isNumericVerticalAlign())
                        style->setVerticalAlignLength(parentStyle->verticalAlignLength());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->setVerticalAlign(ComputedStyle::initialVerticalAlign());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Length) {
                    style->setVerticalAlignLength(cssValues[k].lengthValue().toLength());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Percentage) {
                    style->setVerticalAlignLength(Length(Length::Percent, cssValues[k].percentageValue()));
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::VerticalAlignValueKind) {
                    STARFISH_ASSERT(cssValues[k].verticalAlignValue() != VerticalAlignValue::NumericVAlignValue);
                    style->setVerticalAlign(cssValues[k].verticalAlignValue());
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
                break;
            case CSSStyleValuePair::KeyKind::TextAlign:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    // NOTICE: Do not use getter of parent's textAlign here.
                    //         (to remove direction dependency)
                    style->setTextAlign(parentStyle->m_inheritedStyles.m_textAlign);
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    // Initial: none_value that acts as 'left' if 'direction' is 'ltr', 'right' if 'direction' is 'rtl'
                    style->setTextAlign(ComputedStyle::initialTextAlign());
                } else {
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::SideValueKind);
                    style->setTextAlign(cssValues[k].sideValue());
                }
                break;
            case CSSStyleValuePair::KeyKind::TextDecoration:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setTextDecoration(parentStyle->textDecoration());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->setTextDecoration(TextDecorationValue::NoneTextDecorationValue);
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::None) {
                    style->setTextDecoration(cssValues[k].textDecoration());
                } else {
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::TextDecorationValueKind);
                    style->setTextDecoration(cssValues[k].textDecoration());
                }
                break;
            case CSSStyleValuePair::KeyKind::Direction:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->m_inheritedStyles.m_direction = parentStyle->m_inheritedStyles.m_direction;
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->m_inheritedStyles.m_direction = DirectionValue::LtrDirectionValue;
                } else {
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::DirectionValueKind);
                    style->m_inheritedStyles.m_direction = cssValues[k].directionValue();
                }
                break;
            case CSSStyleValuePair::KeyKind::BackgroundColor:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setBackgroundColor(parentStyle->backgroundColor());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->setBackgroundColor(Color(0, 0, 0, 0));
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::ColorValueKind) {
                    style->setBackgroundColor(cssValues[k].colorValue());
                } else {
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::NamedColorValueKind);
                    if (cssValues[k].namedColorValue() == NamedColorValue::currentColor) {
                        // currentColor : represents the calculated value of the element's color property
                        // --> change to valid value when arrangeStyleValues()
                        style->setBackgroundColorToCurrentColor();
                    } else {
                        style->setBackgroundColor(namedColorToColor(cssValues[k].namedColorValue()));
                    }
                }
                break;
            case CSSStyleValuePair::KeyKind::BackgroundImage:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial
                    || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::None) {
                    style->setBackgroundImage(ComputedStyle::initialBgImage());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setBackgroundImage(parentStyle->backgroundImage());
                } else {
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::UrlValueKind);
                    style->setBackgroundImage(cssValues[k].urlValue(origin));
                }
                break;
            case CSSStyleValuePair::KeyKind::BackgroundPosition:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setBackgroundPositionValue(parentStyle->backgroundPosition());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->setBackgroundPositionValue(LengthPosition(Length(Length::Percent, 0.0f), Length(Length::Percent, 0.0f)));
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::ValueListKind) {
                    ValueList* list = cssValues[k].multiValue();
                    Length xAxis, yAxis;

                    xAxis = Length(Length::Percent, 0.5f);
                    yAxis = Length(Length::Percent, 0.5f);

                    for (unsigned int i = 0; i < list->size(); i++) {
                        CSSStyleValuePair& item = list->atIndex(i);
                        if (item.valueKind() == CSSStyleValuePair::ValueKind::SideValueKind) {
                            if (item.sideValue() == SideValue::LeftSideValue) {
                                xAxis = Length(Length::Percent, 0.0f);
                            } else if (item.sideValue() == SideValue::RightSideValue) {
                                xAxis = Length(Length::Percent, 1.0f);
                            } else if (item.sideValue() == SideValue::CenterSideValue) {

                            } else if (item.sideValue() == SideValue::TopSideValue) {
                                yAxis = Length(Length::Percent, 0.0f);
                            } else if (item.sideValue() == SideValue::BottomSideValue) {
                                yAxis = Length(Length::Percent, 1.0f);
                            }
                        } else {
                            if (i == 0)
                                xAxis = convertValueToLength(item.valueKind(), item.value());
                            else
                                yAxis = convertValueToLength(item.valueKind(), item.value());
                        }
                    }
                    style->setBackgroundPositionValue(LengthPosition(xAxis, yAxis));
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }

                break;
            case CSSStyleValuePair::KeyKind::BackgroundSize:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setBackgroundSizeType(parentStyle->bgSizeType());
                    style->setBackgroundSizeValue(parentStyle->bgSizeValue());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->setBackgroundSizeValue(LengthSize());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Cover) {
                    style->setBackgroundSizeType(BackgroundSizeType::Cover);
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Contain) {
                    style->setBackgroundSizeType(BackgroundSizeType::Contain);
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Auto) {
                    style->setBackgroundSizeValue(LengthSize());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueListKind) {
                    ValueList* list = cssValues[k].multiValue();
                    LengthSize result;
                    if (list->size() >= 1) {
                        result.m_width = convertValueToLength(list->atIndex(0).valueKind(), list->atIndex(0).value());
                    }
                    if (list->size() >= 2) {
                        result.m_height = convertValueToLength(list->atIndex(1).valueKind(), list->atIndex(1).value());
                    }
                    style->setBackgroundSizeValue(result);
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
                break;
            case CSSStyleValuePair::KeyKind::BackgroundRepeatX:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setBackgroundRepeatX(parentStyle->backgroundRepeatX());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->setBackgroundRepeatX(BackgroundRepeatValue::RepeatRepeatValue);
                } else {
                    STARFISH_ASSERT(CSSStyleValuePair::ValueKind::BackgroundRepeatValueKind == cssValues[k].valueKind());
                    style->setBackgroundRepeatX(cssValues[k].backgroundRepeatValue());
                }
                break;
            case CSSStyleValuePair::KeyKind::BackgroundRepeatY:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setBackgroundRepeatY(parentStyle->backgroundRepeatY());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->setBackgroundRepeatY(BackgroundRepeatValue::RepeatRepeatValue);
                } else {
                    STARFISH_ASSERT(CSSStyleValuePair::ValueKind::BackgroundRepeatValueKind == cssValues[k].valueKind());
                    style->setBackgroundRepeatY(cssValues[k].backgroundRepeatValue());
                }
                break;
            case CSSStyleValuePair::KeyKind::BorderImageSlice:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->setBorderImageSlices(ComputedStyle::initialBorderImageSlices());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    // TODO: Prevent parentStyle->surround() from creating object for this
                    style->setBorderImageSliceFromOther(parentStyle);
                } else {
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::ValueListKind);
                    Length top, right, bottom, left;
                    ValueList* l = cssValues[k].multiValue();
                    unsigned int size = l->size();
                    if (l->atIndex(size - 1).valueKind() == CSSStyleValuePair::ValueKind::StringValueKind) {
                        style->setBorderImageSliceFill(true);
                        size--;
                    }
                    top = convertValueToLength(l->atIndex(0).valueKind(), l->atIndex(0).value());
                    if (size > 1) {
                        right = convertValueToLength(l->atIndex(1).valueKind(), l->atIndex(1).value());
                    } else {
                        right = top;
                    }
                    if (size > 2) {
                        bottom = convertValueToLength(l->atIndex(2).valueKind(), l->atIndex(2).value());
                    } else {
                        bottom = top;
                    }
                    if (size > 3) {
                        left = convertValueToLength(l->atIndex(3).valueKind(), l->atIndex(3).value());
                    } else {
                        left = right;
                    }
                    style->setBorderImageSlices(LengthBox(top, right, bottom, left));
                }
                break;
            case CSSStyleValuePair::KeyKind::BorderImageSource:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setBorderImageSource(parentStyle->borderImageSource());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->setBorderImageSource(String::emptyString);
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::None) {
                    style->setBorderImageSource(String::emptyString);
                } else {
                    STARFISH_ASSERT(CSSStyleValuePair::ValueKind::UrlValueKind == cssValues[k].valueKind());
                    style->setBorderImageSource(cssValues[k].urlValue(origin));
                }
                break;
            case CSSStyleValuePair::KeyKind::BorderImageWidth:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setBorderImageWidths(parentStyle->borderImageWidths());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    BorderImageLengthBox box;
                    style->setBorderImageWidths(box);
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Number) {
                    BorderImageLength unit;
                    unit.setValue(cssValues[k].numberValue());
                    style->setBorderImageWidths(BorderImageLengthBox(unit, unit, unit, unit));
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Length) {
                    BorderImageLength unit;
                    unit.setValue(convertValueToLength(CSSStyleValuePair::ValueKind::Length, cssValues[k].value()));
                    style->setBorderImageWidths(BorderImageLengthBox(unit, unit, unit, unit));
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                    /* NOTE: Not allow ValueList in current spec
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::ValueListKind);
                    BorderImageLength top, right, bottom, left;
                    ValueList* l = cssValues[k].multiValue();
                    unsigned int size = l->size();
                    if (l->atIndex(0).valueKind() == CSSStyleValuePair::ValueKind::Number)
                        top.setValue(l->atIndex(0).numberValue());
                    else
                        top.setValue(convertValueToLength(l->atIndex(0).valueKind(), l->atIndex(0).value()));
                    if (size > 1) {
                        if (l->atIndex(1).valueKind() == CSSStyleValuePair::ValueKind::Number)
                            right.setValue(l->atIndex(1).numberValue());
                        else
                            right.setValue(convertValueToLength(l->atIndex(1).valueKind(), l->atIndex(1).value()));
                    } else {
                        right = top;
                    }
                    if (size > 2) {
                        if (l->atIndex(2).valueKind() == CSSStyleValuePair::ValueKind::Number)
                            bottom.setValue(l->atIndex(2).numberValue());
                        else
                            bottom.setValue(convertValueToLength(l->atIndex(2).valueKind(), l->atIndex(2).value()));
                    } else {
                        bottom = top;
                    }
                    if (size > 3) {
                        if (l->atIndex(3).valueKind() == CSSStyleValuePair::ValueKind::Number)
                            left.setValue(l->atIndex(3).numberValue());
                        else
                            left.setValue(convertValueToLength(l->atIndex(3).valueKind(), l->atIndex(3).value()));
                    } else {
                        left = right;
                    }
                    style->setBorderImageWidths(BorderImageLengthBox(top, right, bottom, left));
                    */
                }
                break;
            case CSSStyleValuePair::KeyKind::LineHeight:
                // <normal> | number | length | percentage | inherit
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setLineHeight(parentStyle->lineHeight());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial
                    || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Normal) {
                    // The compute value should be 'normal'.
                    // https://developer.mozilla.org/ko/docs/Web/CSS/line-height.
                    style->setLineHeight(Length(Length::Percent, -100));
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Number) {
                    // The computed value should be same as the specified value.
                    style->setLineHeight(Length(Length::InheritableNumber, cssValues[k].numberValue()));
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Length
                    || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Percentage) {
                    style->setLineHeight(convertValueToLength(cssValues[k].valueKind(), cssValues[k].value()));
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
                break;
#define ADD_RESOLVE_STYLE_POS(POS, pos) \
            case CSSStyleValuePair::KeyKind::POS: \
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) { \
                    style->set##POS(parentStyle->pos()); \
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial \
                    || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Auto) { \
                    style->set##POS(Length()); \
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Length) { \
                    style->set##POS(cssValues[k].lengthValue().toLength()); \
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Percentage) { \
                    style->set##POS(Length(Length::Percent, cssValues[k].percentageValue())); \
                } else { \
                    STARFISH_RELEASE_ASSERT_NOT_REACHED(); \
                } \
                break;
                ADD_RESOLVE_STYLE_POS(Top, top)
                ADD_RESOLVE_STYLE_POS(Right, right)
                ADD_RESOLVE_STYLE_POS(Bottom, bottom)
                ADD_RESOLVE_STYLE_POS(Left, left)
#undef ADD_RESOLVE_STYLE_POS
#define ADD_RESOLVE_STYLE_BORDER_STYLE(POS) \
            case CSSStyleValuePair::KeyKind::Border##POS##Style: \
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) { \
                    style->setBorder##POS##Style(parentStyle->border##POS##Style()); \
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) { \
                    style->setBorder##POS##Style(BorderStyleValue::NoneBorderStyleValue); \
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::BorderStyleValueKind) { \
                    style->setBorder##POS##Style(cssValues[k].borderStyleValue()); \
                } else { \
                    STARFISH_RELEASE_ASSERT_NOT_REACHED(); \
                } \
                break;
                ADD_RESOLVE_STYLE_BORDER_STYLE(Top)
                ADD_RESOLVE_STYLE_BORDER_STYLE(Right)
                ADD_RESOLVE_STYLE_BORDER_STYLE(Bottom)
                ADD_RESOLVE_STYLE_BORDER_STYLE(Left)
#undef ADD_RESOLVE_STYLE_BORDER_STYLE
#define ADD_RESOLVE_STYLE_BORDER_WIDTH(POS) \
            case CSSStyleValuePair::KeyKind::Border##POS##Width: \
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) { \
                    style->setBorder##POS##Width(parentStyle->border##POS##Width()); \
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) { \
                    style->setBorder##POS##Width(Length(Length::Fixed, 3)); \
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Length) { \
                    style->setBorder##POS##Width(cssValues[k].lengthValue().toLength()); \
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::BorderWidthValueKind) { \
                    if (cssValues[k].borderWidthValue() == BorderWidthValue::ThinBorderWidthValue) { \
                        style->setBorder##POS##Width(Length(Length::Fixed, 1)); \
                    } else if (cssValues[k].borderWidthValue() == BorderWidthValue::MediumBorderWidthValue) { \
                        style->setBorder##POS##Width(Length(Length::Fixed, 3)); \
                    } else if (cssValues[k].borderWidthValue() == BorderWidthValue::ThickBorderWidthValue) { \
                        style->setBorder##POS##Width(Length(Length::Fixed, 5)); \
                    } \
                } else { \
                    STARFISH_RELEASE_ASSERT_NOT_REACHED(); \
                } \
                break;
                ADD_RESOLVE_STYLE_BORDER_WIDTH(Top)
                ADD_RESOLVE_STYLE_BORDER_WIDTH(Right)
                ADD_RESOLVE_STYLE_BORDER_WIDTH(Bottom)
                ADD_RESOLVE_STYLE_BORDER_WIDTH(Left)
#undef ADD_RESOLVE_STYLE_BORDER_WIDTH
#define ADD_RESOLVE_STYLE_BORDER_COLOR(POS) \
            case CSSStyleValuePair::KeyKind::Border##POS##Color: \
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) { \
                    style->setBorder##POS##Color(parentStyle->border##POS##Color());     \
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) { \
                    style->clearBorder##POS##Color(); \
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::ColorValueKind) { \
                    style->setBorder##POS##Color(cssValues[k].colorValue()); \
                } else { \
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::NamedColorValueKind); \
                    if (cssValues[k].namedColorValue() == NamedColorValue::currentColor) { \
                        style->clearBorder##POS##Color();   \
                    } else { \
                        style->setBorder##POS##Color(namedColorToColor(cssValues[k].namedColorValue())); \
                    } \
                } \
                break;
                ADD_RESOLVE_STYLE_BORDER_COLOR(Top)
                ADD_RESOLVE_STYLE_BORDER_COLOR(Right)
                ADD_RESOLVE_STYLE_BORDER_COLOR(Bottom)
                ADD_RESOLVE_STYLE_BORDER_COLOR(Left)
#undef ADD_RESOLVE_STYLE_BORDER_COLOR
#define ADD_RESOLVE_STYLE_MARGIN(POS) \
            case CSSStyleValuePair::KeyKind::Margin##POS: \
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) { \
                    style->setMargin##POS(parentStyle->margin##POS()); \
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) { \
                    style->setMargin##POS(Length(Length::Fixed, 0)); \
                } else { \
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Auto \
                        || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Length \
                        || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Percentage); \
                    style->setMargin##POS(convertValueToLength(cssValues[k].valueKind(), cssValues[k].value())); \
                } \
                break;
                ADD_RESOLVE_STYLE_MARGIN(Top)
                ADD_RESOLVE_STYLE_MARGIN(Right)
                ADD_RESOLVE_STYLE_MARGIN(Bottom)
                ADD_RESOLVE_STYLE_MARGIN(Left)
#undef ADD_RESOLVE_STYLE_MARGIN
#define ADD_RESOLVE_STYLE_PADDING(POS) \
            case CSSStyleValuePair::KeyKind::Padding##POS: \
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) { \
                    style->setPadding##POS(parentStyle->padding##POS()); \
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) { \
                    style->setPadding##POS(ComputedStyle::initialPadding()); \
                } else { \
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Length \
                        || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Percentage); \
                    style->setPadding##POS(convertValueToLength(cssValues[k].valueKind(), cssValues[k].value())); \
                } \
                break;
                ADD_RESOLVE_STYLE_PADDING(Top)
                ADD_RESOLVE_STYLE_PADDING(Right)
                ADD_RESOLVE_STYLE_PADDING(Bottom)
                ADD_RESOLVE_STYLE_PADDING(Left)
#undef ADD_RESOLVE_STYLE_PADDING
            case CSSStyleValuePair::KeyKind::Opacity:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->m_opacity = parentStyle->m_opacity;
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->m_opacity = 1.0;
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Number) {
                    float beforeClip = cssValues[k].numberValue();
                    style->m_opacity = beforeClip < 0 ? 0 : (beforeClip > 1.0 ? 1.0: beforeClip);
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
                break;
            case CSSStyleValuePair::KeyKind::Overflow:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->m_overflow = parentStyle->overflow();
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->m_overflow = OverflowValue::VisibleOverflow;
                } else {
                    style->m_overflow = cssValues[k].overflowValue();
                }
                break;
            case CSSStyleValuePair::KeyKind::Visibility:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->m_inheritedStyles.m_visibility = parentStyle->visibility();
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->m_inheritedStyles.m_visibility = VisibilityValue::VisibleVisibilityValue;
                } else {
                    style->m_inheritedStyles.m_visibility = cssValues[k].visibility();
                }
                break;
            case CSSStyleValuePair::KeyKind::ZIndex:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->m_zIndex = parentStyle->m_zIndex;
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial
                    || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Auto) {
                    style->m_zIndex = 0;
                } else {
                    style->m_zIndex = cssValues[k].int32Value();
                    style->m_zIndexSpecifiedByUser = true;
                }
                break;
            case CSSStyleValuePair::KeyKind::Transform:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->m_transforms = parentStyle->m_transforms;
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial
                    || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::None) {
                } else {
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::TransformFunctions);
                    CSSTransformFunctions* funcs = cssValues[k].transformValue();
                    for (unsigned c = 0; c < funcs->size(); c++) {
                        CSSTransformFunction f = funcs->at(c);
                        int valueSize = f.values()->size();
                        float dValues[valueSize];
                        for (int i = 0; i < valueSize; i++) {
                            CSSStyleValuePair& item = f.values()->atIndex(i);
                            if (item.valueKind() == CSSStyleValuePair::ValueKind::Number) {
                                dValues[i] = item.numberValue();
                            } else if (item.valueKind() == CSSStyleValuePair::ValueKind::Angle) {
                                dValues[i] = item.angleValue().toDegreeValue();
                            }
                        }

                        switch (f.kind()) {
                        case CSSTransformFunction::Kind::Matrix:
                            style->setTransformMatrix(dValues[0], dValues[1], dValues[2], dValues[3], dValues[4], dValues[5]);
                            break;
                        case CSSTransformFunction::Kind::Translate:
                            {
                            Length a, b(Length::Fixed, 0);
                            a = convertValueToLength(f.values()->atIndex(0).valueKind(), f.values()->atIndex(0).value());
                            if (valueSize > 1)
                                b = convertValueToLength(f.values()->atIndex(1).valueKind(), f.values()->atIndex(1).value());
                            style->setTransformTranslate(a, b);
                            break;
                            }
                        case CSSTransformFunction::Kind::TranslateX:
                            {
                            Length a = convertValueToLength(f.values()->atIndex(0).valueKind(), f.values()->atIndex(0).value());
                            style->setTransformTranslate(a, Length(Length::Fixed, 0));
                            }
                            break;
                        case CSSTransformFunction::Kind::TranslateY:
                            {
                            Length a = convertValueToLength(f.values()->atIndex(0).valueKind(), f.values()->atIndex(0).value());
                            style->setTransformTranslate(Length(Length::Fixed, 0), a);
                            }
                            break;
                        case CSSTransformFunction::Kind::Scale:
                            if (valueSize == 1)
                                style->setTransformScale(dValues[0], dValues[0]);
                            else if (valueSize == 2)
                                style->setTransformScale(dValues[0], dValues[1]);
                            else
                                STARFISH_RELEASE_ASSERT_NOT_REACHED();
                            break;
                        case CSSTransformFunction::Kind::ScaleX:
                            style->setTransformScale(dValues[0], 1);
                            break;
                        case CSSTransformFunction::Kind::ScaleY:
                            style->setTransformScale(1, dValues[0]);
                            break;
                        case CSSTransformFunction::Kind::Rotate:
                            style->setTransformRotate(dValues[0]);
                            break;
                        case CSSTransformFunction::Kind::Skew:
                            if (valueSize == 2) {
                                style->setTransformSkew(dValues[0], dValues[1]);
                                break;
                            }
                        case CSSTransformFunction::Kind::SkewX:
                            style->setTransformSkew(dValues[0], 0);
                            break;
                        case CSSTransformFunction::Kind::SkewY:
                            style->setTransformSkew(0, dValues[0]);
                            break;
                        default:
                            STARFISH_RELEASE_ASSERT_NOT_REACHED();
                        }
                    }
                }
                break;
            case CSSStyleValuePair::KeyKind::TransformOrigin:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->m_transformOrigin = parentStyle->m_transformOrigin;
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->setTransformOriginValue(Length(Length::Percent, 0.5f), Length(Length::Percent, 0.5f));
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::ValueListKind) {
                    ValueList* list = cssValues[k].multiValue();
                    Length xAxis, yAxis;

                    xAxis = Length(Length::Percent, 0.5f);
                    yAxis = Length(Length::Percent, 0.5f);

                    for (unsigned int i = 0; i < list->size(); i++) {
                        CSSStyleValuePair& item = list->atIndex(i);
                        if (item.valueKind() == CSSStyleValuePair::ValueKind::SideValueKind) {
                            if (item.sideValue() == SideValue::LeftSideValue) {
                                xAxis = Length(Length::Percent, 0.0f);
                            } else if (item.sideValue() == SideValue::RightSideValue) {
                                xAxis = Length(Length::Percent, 1.0f);
                            } else if (item.sideValue() == SideValue::CenterSideValue) {

                            } else if (item.sideValue() == SideValue::TopSideValue) {
                                yAxis = Length(Length::Percent, 0.0f);
                            } else if (item.sideValue() == SideValue::BottomSideValue) {
                                yAxis = Length(Length::Percent, 1.0f);
                            }
                        } else {
                            if (i == 0)
                                xAxis = convertValueToLength(item.valueKind(), item.value());
                            else
                                yAxis = convertValueToLength(item.valueKind(), item.value());
                        }
                    }

                    style->setTransformOriginValue(xAxis, yAxis);
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }

                break;
            case CSSStyleValuePair::KeyKind::UnicodeBidi:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->m_unicodeBidi = parentStyle->m_unicodeBidi;
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->m_unicodeBidi = UnicodeBidiValue::NormalUnicodeBidiValue;
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::UnicodeBidiValueKind) {
                    style->setUnicodeBidi(cssValues[k].unicodeBidiValue());
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
                break;
            }
        }
    };

    if (element->isHTMLElement()) {
        if (element->asHTMLElement()->hasDirAttribute()) {
            String* str = element->asHTMLElement()->getAttribute(element->document()->window()->starFish()->staticStrings()->m_dir);
            str = str->toLower();
            if (str->equals("ltr")) {
                ret->m_inheritedStyles.m_direction = DirectionValue::LtrDirectionValue;
                ret->setUnicodeBidi(UnicodeBidiValue::EmbedUnicodeBidiValue);
            } else if (str->equals("rtl")) {
                ret->m_inheritedStyles.m_direction = DirectionValue::RtlDirectionValue;
                ret->setUnicodeBidi(UnicodeBidiValue::EmbedUnicodeBidiValue);
            } else {
                ret->m_inheritedStyles.m_direction = DirectionValue::LtrDirectionValue;
            }
        }
    }

    // first sheet is must user-agent style sheet!
    for (unsigned i = 0; i < m_sheets.size(); i++) {
        CSSStyleSheet* sheet = m_sheets[i];

        // * selector
        for (unsigned j = 0; j < sheet->rules().size(); j++) {
            if (sheet->rules()[j]->m_kind == CSSStyleRule::UniversalSelector && sheet->rules()[j]->m_pseudoClass == CSSStyleRule::PseudoClass::None) {
#ifdef STARFISH_TC_COVERAGE
                STARFISH_LOG_INFO("+++selector:universal-selector\n");
#endif
                auto cssValues = sheet->rules()[j]->styleDeclaration()->m_cssValues;
                apply(sheet->url(), cssValues, ret, parent);
            }
        }

        // type selector
        for (unsigned j = 0; j < sheet->rules().size(); j++) {
            if (sheet->rules()[j]->m_kind == CSSStyleRule::TypeSelector && sheet->rules()[j]->m_pseudoClass == CSSStyleRule::PseudoClass::None) {
#ifdef STARFISH_TC_COVERAGE
                STARFISH_LOG_INFO("+++selector:type-selector\n");
#endif
                if (sheet->rules()[j]->m_ruleText[0]->equalsWithoutCase(element->localName())) {
                    auto cssValues = sheet->rules()[j]->styleDeclaration()->m_cssValues;
                    apply(sheet->url(), cssValues, ret, parent);
                }
            }
        }

        // class selector
        for (unsigned j = 0; j < sheet->rules().size(); j++) {
            if (sheet->rules()[j]->m_kind == CSSStyleRule::ClassSelector && sheet->rules()[j]->m_pseudoClass == CSSStyleRule::PseudoClass::None) {
#ifdef STARFISH_TC_COVERAGE
                STARFISH_LOG_INFO("+++selector:class-selector\n");
#endif
                auto className = element->classNames();
                for (unsigned f = 0; f < className.size(); f++) {
                    if (className[f]->equals(sheet->rules()[j]->m_ruleText[0])) {
                        auto cssValues = sheet->rules()[j]->styleDeclaration()->m_cssValues;
                        apply(sheet->url(), cssValues, ret, parent);
                    }
                }
            }
        }

        // type.class selector
        for (unsigned j = 0; j < sheet->rules().size(); j++) {
            if (sheet->rules()[j]->m_kind == CSSStyleRule::TypeClassSelector && sheet->rules()[j]->m_pseudoClass == CSSStyleRule::PseudoClass::None) {
#ifdef STARFISH_TC_COVERAGE
                STARFISH_LOG_INFO("+++selector:class-selector\n");
#endif
                if (element->localName()->equalsWithoutCase(sheet->rules()[j]->m_ruleText[0])) {
                    auto className = element->classNames();
                    for (unsigned f = 0; f < className.size(); f++) {
                        if (className[f]->equals(sheet->rules()[j]->m_ruleText[1])) {
                            auto cssValues = sheet->rules()[j]->styleDeclaration()->m_cssValues;
                            apply(sheet->url(), cssValues, ret, parent);
                        }
                    }
                }
            }
        }

        // id selector
        for (unsigned j = 0; j < sheet->rules().size(); j++) {
            if (sheet->rules()[j]->m_kind == CSSStyleRule::IdSelector && sheet->rules()[j]->m_pseudoClass == CSSStyleRule::PseudoClass::None) {
#ifdef STARFISH_TC_COVERAGE
                STARFISH_LOG_INFO("+++selector:id-selector\n");
#endif
                if (element->id()->equals(sheet->rules()[j]->m_ruleText[0])) {
                    auto cssValues = sheet->rules()[j]->styleDeclaration()->m_cssValues;
                    apply(sheet->url(), cssValues, ret, parent);
                }
            }
        }

        // type#id selector
        for (unsigned j = 0; j < sheet->rules().size(); j++) {
            if (sheet->rules()[j]->m_kind == CSSStyleRule::TypeIdSelector && sheet->rules()[j]->m_pseudoClass == CSSStyleRule::PseudoClass::None) {
#ifdef STARFISH_TC_COVERAGE
                STARFISH_LOG_INFO("+++selector:id-selector\n");
#endif
                if (element->localName()->equalsWithoutCase(sheet->rules()[j]->m_ruleText[0])) {
                    if (element->id()->equals(sheet->rules()[j]->m_ruleText[1])) {
                        auto cssValues = sheet->rules()[j]->styleDeclaration()->m_cssValues;
                        apply(sheet->url(), cssValues, ret, parent);
                    }
                }
            }
        }
    }

    // first sheet is must user-agent style sheet!
    for (unsigned i = 0; i < m_sheets.size(); i++) {
        CSSStyleSheet* sheet = m_sheets[i];
        // *:active selector
        if ((element->state() & Node::NodeState::NodeStateActive)) {
            for (unsigned j = 0; j < sheet->rules().size(); j++) {
                if (sheet->rules()[j]->m_kind == CSSStyleRule::UniversalSelector && sheet->rules()[j]->m_pseudoClass == CSSStyleRule::PseudoClass::Active) {
#ifdef STARFISH_TC_COVERAGE
                    STARFISH_LOG_INFO("+++selector:pseudo-active-selector\n");
#endif
                    auto cssValues = sheet->rules()[j]->styleDeclaration()->m_cssValues;
                    apply(sheet->url(), cssValues, ret, parent);
                }
            }
        }

        // type:active selector
        if ((element->state() & Node::NodeState::NodeStateActive)) {
            for (unsigned j = 0; j < sheet->rules().size(); j++) {
                if (sheet->rules()[j]->m_kind == CSSStyleRule::TypeSelector && sheet->rules()[j]->m_pseudoClass == CSSStyleRule::PseudoClass::Active) {
#ifdef STARFISH_TC_COVERAGE
                    STARFISH_LOG_INFO("+++selector:pseudo-active-selector\n");
#endif
                    if (sheet->rules()[j]->m_ruleText[0]->equalsWithoutCase(element->localName())) {
                        auto cssValues = sheet->rules()[j]->styleDeclaration()->m_cssValues;
                        apply(sheet->url(), cssValues, ret, parent);
                    }
                }
            }
        }

        // class:active selector
        if ((element->state() & Node::NodeState::NodeStateActive)) {
            for (unsigned j = 0; j < sheet->rules().size(); j++) {
                if (sheet->rules()[j]->m_kind == CSSStyleRule::ClassSelector && sheet->rules()[j]->m_pseudoClass == CSSStyleRule::PseudoClass::Active) {
#ifdef STARFISH_TC_COVERAGE
                    STARFISH_LOG_INFO("+++selector:pseudo-active-selector\n");
#endif
                    auto className = element->classNames();
                    for (unsigned f = 0; f < className.size(); f++) {
                        if (className[f]->equals(sheet->rules()[j]->m_ruleText[0])) {
                            auto cssValues = sheet->rules()[j]->styleDeclaration()->m_cssValues;
                            apply(sheet->url(), cssValues, ret, parent);
                        }
                    }
                }
            }
        }

        // type.class:active selector
        for (unsigned j = 0; j < sheet->rules().size(); j++) {
            if (sheet->rules()[j]->m_kind == CSSStyleRule::TypeClassSelector && sheet->rules()[j]->m_pseudoClass == CSSStyleRule::PseudoClass::Active) {
#ifdef STARFISH_TC_COVERAGE
                STARFISH_LOG_INFO("+++selector:pseudo-active-selector\n");
#endif
                if (element->localName()->equalsWithoutCase(sheet->rules()[j]->m_ruleText[0])) {
                    auto className = element->classNames();
                    for (unsigned f = 0; f < className.size(); f++) {
                        if (className[f]->equals(sheet->rules()[j]->m_ruleText[1])) {
                            auto cssValues = sheet->rules()[j]->styleDeclaration()->m_cssValues;
                            apply(sheet->url(), cssValues, ret, parent);
                        }
                    }
                }
            }
        }

        // id:active selector
        if ((element->state() & Node::NodeState::NodeStateActive)) {
            for (unsigned j = 0; j < sheet->rules().size(); j++) {
                if (sheet->rules()[j]->m_kind == CSSStyleRule::IdSelector && sheet->rules()[j]->m_pseudoClass == CSSStyleRule::PseudoClass::Active) {
#ifdef STARFISH_TC_COVERAGE
                    STARFISH_LOG_INFO("+++selector:pseudo-active-selector\n");
#endif
                    if (element->id()->equals(sheet->rules()[j]->m_ruleText[0])) {
                        auto cssValues = sheet->rules()[j]->styleDeclaration()->m_cssValues;
                        apply(sheet->url(), cssValues, ret, parent);
                    }
                }
            }
        }

        // type#id:active selector
        for (unsigned j = 0; j < sheet->rules().size(); j++) {
            if (sheet->rules()[j]->m_kind == CSSStyleRule::TypeIdSelector && sheet->rules()[j]->m_pseudoClass == CSSStyleRule::PseudoClass::Active) {
#ifdef STARFISH_TC_COVERAGE
                STARFISH_LOG_INFO("+++selector:pseudo-active-selector\n");
#endif
                if (element->localName()->equalsWithoutCase(sheet->rules()[j]->m_ruleText[0])) {
                    if (element->id()->equals(sheet->rules()[j]->m_ruleText[1])) {
                        auto cssValues = sheet->rules()[j]->styleDeclaration()->m_cssValues;
                        apply(sheet->url(), cssValues, ret, parent);
                    }
                }
            }
        }
    }

    // inline style
    if (element->inlineStyleWithoutCreation()) {
        auto inlineCssValues = element->inlineStyleWithoutCreation()->m_cssValues;
        apply(element->document()->documentURI(), inlineCssValues, ret, parent);
    }

    ret->loadResources(element, element->style());
    ret->arrangeStyleValues(parent);
    return ret;
}

void resolveDOMStyleInner(StyleResolver* resolver, Element* element, ComputedStyle* parentStyle, bool inheritedStyleChanged = false)
{
    ComputedStyleDamage damage = ComputedStyleDamage::ComputedStyleDamageNone;

    if (element->needsStyleRecalc() || inheritedStyleChanged) {
        ComputedStyle* style = resolver->resolveStyle(element, parentStyle);

        if (!element->style()) {
            damage = (ComputedStyleDamage)(ComputedStyleDamage::ComputedStyleDamageInherited | ComputedStyleDamage::ComputedStyleDamageRebuildFrame);
        } else {
            damage = compareStyle(element->style(), style);
        }

        if (damage & ComputedStyleDamage::ComputedStyleDamageInherited) {
            inheritedStyleChanged = inheritedStyleChanged | true;
        }

        if (damage & ComputedStyleDamage::ComputedStyleDamageRebuildFrame) {
            element->setNeedsFrameTreeBuild();
        }

        if (damage & ComputedStyleDamage::ComputedStyleDamageLayout) {
            element->setNeedsLayout();
        }

        if (damage & ComputedStyleDamage::ComputedStyleDamagePainting) {
            element->setNeedsPainting();
        }

        if (damage & ComputedStyleDamage::ComputedStyleDamageComposite) {
            element->setNeedsComposite();
        }

        element->setStyle(style);
        element->clearNeedsStyleRecalc();
    }

    bool shouldWeStopTreeTraverseHere = element->style()->display() == DisplayValue::NoneDisplayValue;
    if (shouldWeStopTreeTraverseHere) {
        return;
    }

    if (inheritedStyleChanged | element->childNeedsStyleRecalc()) {
        ComputedStyle* childStyle = nullptr;
        Node* child = element->firstChild();
        while (child) {
            if (child->isElement()) {
                resolveDOMStyleInner(resolver, child->asElement(), element->style(), inheritedStyleChanged);
            } else {
                if (inheritedStyleChanged || child->needsStyleRecalc()) {
                    if (childStyle == nullptr) {
                        childStyle = new ComputedStyle(element->style());
                        childStyle->loadResources(element);
                        childStyle->arrangeStyleValues(element->style());
                    }

                    child->setStyle(childStyle);
                    child->clearNeedsStyleRecalc();
                }
            }
            child = child->nextSibling();
        }
        element->clearNeedsStyleRecalc();
    }
}

void StyleResolver::resolveDOMStyle(Document* document, bool force)
{
    Node* child = document->firstChild();
    while (child) {
        if (child->isElement()) {
            resolveDOMStyleInner(this, child->asElement(), document->style(), force);
        }
        child = child->nextSibling();
    }
}

void StyleResolver::addSheet(CSSStyleSheet* sheet)
{
    Node* origin = sheet->origin();
    bool originFounded = false;
    bool added = false;
    Traverse::findDescendant(&m_document, [&](Node* node) -> bool {
        if (!originFounded && node == origin) {
            originFounded = true;
        } else if (originFounded) {
            if (node->isElement()) {
                if (node->asElement()->isHTMLElement()) {
                    if (node->asElement()->asHTMLElement()->isHTMLStyleElement()) {
                        if (node->asElement()->asHTMLElement()->asHTMLStyleElement()->generatedSheet()) {
                            auto sheet2 = node->asElement()->asHTMLElement()->asHTMLStyleElement()->generatedSheet();
                            auto iter = std::find(m_sheets.begin(), m_sheets.end(), sheet2);
                            STARFISH_ASSERT(iter != m_sheets.end());
                            m_sheets.insert(iter, sheet);
                            added = true;
                            return true;
                        }
                    } else if (node->asElement()->asHTMLElement()->isHTMLLinkElement()) {
                        if (node->asElement()->asHTMLElement()->asHTMLLinkElement()->generatedSheet()) {
                            auto sheet2 = node->asElement()->asHTMLElement()->asHTMLLinkElement()->generatedSheet();
                            auto iter = std::find(m_sheets.begin(), m_sheets.end(), sheet2);
                            STARFISH_ASSERT(iter != m_sheets.end());
                            m_sheets.insert(iter, sheet);
                            added = true;
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    });
    if (!added)
        m_sheets.push_back(sheet);
}

bool CSSStyleValuePair::updateValueColor(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() != 1)
        return false;

    if (CSSPropertyParser::parseNonNamedColor(tokens->at(0), &(m_value.m_color))) {
        m_valueKind = CSSStyleValuePair::ValueKind::ColorValueKind;
    } else if (CSSPropertyParser::parseNamedColor(tokens->at(0), &(m_value.m_namedColor))) {
        m_valueKind = CSSStyleValuePair::ValueKind::NamedColorValueKind;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueBackgroundColor(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return updateValueColor(tokens);
}

#define UPDATE_VALUE_BORDER_COLOR(POS, ...) \
bool CSSStyleValuePair::updateValueBorder##POS##Color(std::vector<String*, gc_allocator<String*> >* tokens) \
{ \
    return updateValueColor(tokens); \
}
GEN_FOURSIDE(UPDATE_VALUE_BORDER_COLOR)
#undef UPDATE_VALUE_BORDER_COLOR

bool CSSStyleValuePair::updateValueBorderUnitStyle(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() != 1)
        return false;

    String* value = (*tokens)[0];
    m_valueKind = CSSStyleValuePair::ValueKind::BorderStyleValueKind;
    if (STRING_VALUE_IS_STRING("none")) {
        m_value.m_borderStyle = BorderStyleValue::NoneBorderStyleValue;
    } else if (STRING_VALUE_IS_STRING("solid")) {
        m_value.m_borderStyle = BorderStyleValue::SolidBorderStyleValue;
    } else {
        return false;
    }
    return true;
}

#define UPDATE_VALUE_BORDER_STYLE(POS, ...) \
bool CSSStyleValuePair::updateValueBorder##POS##Style(std::vector<String*, gc_allocator<String*> >* tokens) \
{ \
    return updateValueBorderUnitStyle(tokens); \
}
GEN_FOURSIDE(UPDATE_VALUE_BORDER_STYLE)
#undef UPDATE_VALUE_BORDER_STYLE

bool CSSStyleValuePair::updateValueDirection(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() != 1)
        return false;

    String* value = (*tokens)[0];
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

bool CSSStyleValuePair::updateValueDisplay(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() != 1)
        return false;

    String* value = (*tokens)[0];
    m_valueKind = CSSStyleValuePair::ValueKind::DisplayValueKind;
    if (STRING_VALUE_IS_STRING("block")) {
        m_value.m_display = DisplayValue::BlockDisplayValue;
    } else if (STRING_VALUE_IS_STRING("inline")) {
        m_value.m_display = DisplayValue::InlineDisplayValue;
    } else if (STRING_VALUE_IS_STRING("inline-block")) {
        m_value.m_display = DisplayValue::InlineBlockDisplayValue;
    } else if (STRING_VALUE_IS_STRING("none")) {
        m_value.m_display = DisplayValue::NoneDisplayValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueFontStyle(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() != 1)
        return false;

    String* value = (*tokens)[0];
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

bool CSSStyleValuePair::updateValueBackgroundRepeatUnit(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() != 1)
        return false;

    String* value = (*tokens)[0];
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

bool CSSStyleValuePair::updateValueBackgroundRepeatX(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return updateValueBackgroundRepeatUnit(tokens);
}

bool CSSStyleValuePair::updateValueBackgroundRepeatY(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return updateValueBackgroundRepeatUnit(tokens);
}

bool CSSStyleValuePair::updateValueUrlOrNone(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() != 1)
        return false;

    String* value = (*tokens)[0];
    if (STRING_VALUE_IS_NONE()) {
        m_valueKind = CSSStyleValuePair::ValueKind::None;
    } else if (CSSPropertyParser::parseUrl(value, &(m_value.m_stringValue))) {
        m_valueKind = CSSStyleValuePair::ValueKind::UrlValueKind;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueBackgroundImage(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return updateValueUrlOrNone(tokens);
}

bool CSSStyleValuePair::updateValueBorderImageSource(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return updateValueUrlOrNone(tokens);
}

bool CSSStyleValuePair::updateValueBorderUnitWidth(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() != 1)
        return false;

    String* value = (*tokens)[0];
    m_valueKind = CSSStyleValuePair::ValueKind::BorderWidthValueKind;
    if (STRING_VALUE_IS_STRING("thick")) {
        m_value.m_borderWidth = BorderWidthValue::ThickBorderWidthValue;
    } else if (STRING_VALUE_IS_STRING("thin")) {
        m_valueKind = CSSStyleValuePair::ValueKind::BorderWidthValueKind;
        m_value.m_borderWidth = BorderWidthValue::ThinBorderWidthValue;
    } else if (STRING_VALUE_IS_STRING("medium")) {
        m_valueKind = CSSStyleValuePair::ValueKind::BorderWidthValueKind;
        m_value.m_borderWidth = BorderWidthValue::MediumBorderWidthValue;
    } else if (CSSPropertyParser::parseLength(value->utf8Data(), false, &(m_value.m_length))) {
        m_valueKind = CSSStyleValuePair::ValueKind::Length;
    } else {
        return false;
    }
    return true;
}

#define UPDATE_VALUE_BORDER_WIDTH(POS, ...) \
bool CSSStyleValuePair::updateValueBorder##POS##Width(std::vector<String*, gc_allocator<String*> >* tokens) \
{ \
    return updateValueBorderUnitWidth(tokens); \
}
GEN_FOURSIDE(UPDATE_VALUE_BORDER_WIDTH)
#undef UPDATE_VALUE_BORDER_WIDTH

bool CSSStyleValuePair::updateValueLengthOrPercent(String* token, bool allowNegative)
{
    if (!CSSPropertyParser::parseLengthOrPercent(token->utf8Data(), allowNegative, this))
        return false;
    return true;
}

bool CSSStyleValuePair::updateValueLengthOrPercent(std::vector<String*, gc_allocator<String*> >* tokens, bool allowNegative)
{
    if (tokens->size() != 1)
        return false;
    return updateValueLengthOrPercent(tokens->at(0), allowNegative);
}

bool CSSStyleValuePair::updateValueLengthOrPercentOrAuto(String* token, bool allowNegative)
{
    if (token->equals("auto")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Auto;
        return true;
    }
    return updateValueLengthOrPercent(token, allowNegative);
}

bool CSSStyleValuePair::updateValueLengthOrPercentOrAuto(std::vector<String*, gc_allocator<String*> >* tokens, bool allowNegative)
{
    if (tokens->size() != 1)
        return false;
    return updateValueLengthOrPercentOrAuto(tokens->at(0), allowNegative);
}

bool CSSStyleValuePair::updateValueBorderImageWidth(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // [length | number]
    if (tokens->size() != 1)
        return false;

    const char* value = tokens->at(0)->utf8Data();
    if (CSSPropertyParser::parseNumber(value, false, &(m_value.m_floatValue))) {
        m_valueKind = CSSStyleValuePair::ValueKind::Number;
    } else if (CSSPropertyParser::parseLength(value, false, &(m_value.m_length))) {
        m_valueKind = CSSStyleValuePair::ValueKind::Length;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueBackgroundPosition(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // [ [ <percentage> | <length> | left | center | right ] [ <percentage> | <length> | top | center | bottom ]? ] | [ [ left | center | right ] || [ top | center | bottom ] ] | inherit
    if (tokens->size() != 1 && tokens->size() != 2)
        return false;

    m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
    ValueList* values = new ValueList();

    for (unsigned int i = 0; i < tokens->size(); i++) {
        String* value = tokens->at(i);
        if (STRING_VALUE_IS_STRING("left")) {
            values->append(CSSStyleValuePair::ValueKind::SideValueKind, SideValue::LeftSideValue);
        } else if (STRING_VALUE_IS_STRING("right")) {
            values->append(CSSStyleValuePair::ValueKind::SideValueKind, SideValue::RightSideValue);
        } else if (STRING_VALUE_IS_STRING("center")) {
            values->append(CSSStyleValuePair::ValueKind::SideValueKind, SideValue::CenterSideValue);
        } else if (STRING_VALUE_IS_STRING("top")) {
            values->append(CSSStyleValuePair::ValueKind::SideValueKind, SideValue::TopSideValue);
        } else if (STRING_VALUE_IS_STRING("bottom")) {
            values->append(CSSStyleValuePair::ValueKind::SideValueKind, SideValue::BottomSideValue);
        } else {
            CSSStyleValuePair ret;
            if (!ret.updateValueLengthOrPercent(value, true))
                return false;
            values->append(ret);
        }
    }

    if (tokens->size() == 1) {
        values->append(CSSStyleValuePair::ValueKind::SideValueKind, SideValue::CenterSideValue);
    }
    m_value.m_multiValue = values;

    return true;
}

bool CSSStyleValuePair::updateValueBackgroundSize(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // [length | percentage | auto]{1, 2} | cover | contain // initial value -> auto
    if (tokens->size() != 1 && tokens->size() != 2)
        return false;

    String* token = (*tokens)[0];
    if (tokens->size() == 1 && token->equals("cover")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Cover;
    } else if (tokens->size() == 1 && token->equals("contain")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Contain;
    } else {
        m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
        ValueList* values = new ValueList(ValueList::Separator::SpaceSeparator);
        m_value.m_multiValue = values;
        for (unsigned int i = 0; i < tokens->size(); i++) {
            token = (*tokens)[i];
            CSSStyleValuePair ret;
            if (!ret.updateValueLengthOrPercentOrAuto(token, false))
                return false;
            values->append(ret);
        }
    }
    return true;
}

bool CSSStyleValuePair::updateValueBorderImageSlice(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // number && fill?
    if (tokens->size() != 1 && tokens->size() != 2)
        return false;

    m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
    m_value.m_multiValue = new ValueList();
    bool isNum = false, isFill = false;
    float result = 0.f;
    for (unsigned int i = 0; i < tokens->size(); i++) {
        if (tokens->at(i)->equals("fill")) {
            if (!isFill)
                isFill = true;
            else
                return false;
            m_value.m_multiValue->append(CSSStyleValuePair::ValueKind::StringValueKind, { 0 });
        } else if (CSSPropertyParser::parseNumber(tokens->at(i)->utf8Data(), false, &result)) {
            isNum = true;
            m_value.m_multiValue->append(CSSStyleValuePair::ValueKind::Number, { (float)result });
        } else {
            return false;
        }
    }

    return isNum;
}

bool CSSStyleValuePair::updateValueFontSize(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // absolute-size | relative-size | length | percentage | inherit // initial value -> medium
    //        O      |       O       |   O    |    O       |    O
    if (tokens->size() != 1)
        return false;

    String* value = tokens->at(0);
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
        return updateValueLengthOrPercent(value, false);
    }
    return true;
}

bool CSSStyleValuePair::updateValueLineHeight(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // <normal> | number | length | percentage | inherit
    if (tokens->size() != 1)
        return false;

    String* value = tokens->at(0);
    float result = 0.f;
    if (STRING_VALUE_IS_STRING("normal")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Normal;
        return true;
    } else if (CSSPropertyParser::parseNumber(value->utf8Data(), false, &result)) {
        m_valueKind = CSSStyleValuePair::ValueKind::Number;
        m_value.m_floatValue = result;
        return true;
    } else {
        return updateValueLengthOrPercent(value, false);
    }
}

#define UPDATE_VALUE_PADDING(POS, ...) \
bool CSSStyleValuePair::updateValuePadding##POS(std::vector<String*, gc_allocator<String*> >* tokens) \
{ \
    return updateValueLengthOrPercent(tokens, false); \
}
GEN_FOURSIDE(UPDATE_VALUE_PADDING)
#undef UPDATE_VALUE_PADDING

#define UPDATE_VALUE_SIDE(POS, ...) \
bool CSSStyleValuePair::updateValue##POS(std::vector<String*, gc_allocator<String*> >* tokens) \
{ \
    return updateValueLengthOrPercentOrAuto(tokens, true); \
}
GEN_FOURSIDE(UPDATE_VALUE_SIDE)
#undef UPDATE_VALUE_SIDE

#define UPDATE_VALUE_MARGIN(POS, ...) \
bool CSSStyleValuePair::updateValueMargin##POS(std::vector<String*, gc_allocator<String*> >* tokens) \
{ \
    return updateValueLengthOrPercentOrAuto(tokens, true); \
}
GEN_FOURSIDE(UPDATE_VALUE_MARGIN)
#undef UPDATE_VALUE_MARGIN

bool CSSStyleValuePair::updateValueWidth(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return updateValueLengthOrPercentOrAuto(tokens, false);
}

bool CSSStyleValuePair::updateValueHeight(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return updateValueLengthOrPercentOrAuto(tokens, false);
}

bool CSSStyleValuePair::updateValueVerticalAlign(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() != 1)
        return false;

    String* value = tokens->at(0);
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
        return updateValueLengthOrPercent(value, true);
    }
    return true;
}

bool CSSStyleValuePair::updateValueTransformOrigin(std::vector<String*, gc_allocator<String*> >* tokens)
{
    //  [ left | center | right | top | bottom | <percentage> | <length> ]
    // |
    //  [ left | center | right | <percentage> | <length> ]
    //  [ top | center | bottom | <percentage> | <length> ] <length>?
    // |
    //  [ center | [ left | right ] ] && [ center | [ top | bottom ] ] <length>?

    if (tokens->size() != 1 && tokens->size() != 2) {
        return false;
    }

    m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
    ValueList* values = new ValueList();

    for (unsigned int i = 0; i < tokens->size(); i++) {
        String* value = tokens->at(i);
        if (STRING_VALUE_IS_STRING("left")) {
            values->append(CSSStyleValuePair::ValueKind::SideValueKind, SideValue::LeftSideValue);
        } else if (STRING_VALUE_IS_STRING("right")) {
            values->append(CSSStyleValuePair::ValueKind::SideValueKind, SideValue::RightSideValue);
        } else if (STRING_VALUE_IS_STRING("center")) {
            values->append(CSSStyleValuePair::ValueKind::SideValueKind, SideValue::CenterSideValue);
        } else if (STRING_VALUE_IS_STRING("top")) {
            values->append(CSSStyleValuePair::ValueKind::SideValueKind, SideValue::TopSideValue);
        } else if (STRING_VALUE_IS_STRING("bottom")) {
            values->append(CSSStyleValuePair::ValueKind::SideValueKind, SideValue::BottomSideValue);
        } else {
            CSSStyleValuePair ret;
            if (!ret.updateValueLengthOrPercent(value, true))
                return false;
            values->append(ret);
        }
    }
    if (tokens->size() == 1) {
        values->append(CSSStyleValuePair::ValueKind::SideValueKind, SideValue::CenterSideValue);
    }
    m_value.m_multiValue = values;
    return true;
}

bool CSSStyleValuePair::updateValueTransform(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() == 1 && tokens->at(0)->equals("none")) {
        m_valueKind = CSSStyleValuePair::ValueKind::None;
        return true;
    } else {
        m_valueKind = CSSStyleValuePair::ValueKind::TransformFunctions;
        CSSTransformFunction::Kind fkind;
        m_value.m_transforms = new CSSTransformFunctions();

        for (unsigned i = 0; i < tokens->size(); i++) {
            CSSPropertyParser* parser = new CSSPropertyParser((char*)tokens->at(i)->utf8Data());
            bool res = parser->consumeString() && parser->consumeIfNext('(');
            if (!res)
                return false;
            String* name = parser->parsedString();
            enum {
                Number, // <number>
                Angle, // <angle>
                TranslationValue // <translation-value>: percentage or length
            } unit = Number;
            int minArgCnt = 1, maxArgCnt = 1;
            if (name->equals("matrix")) {
                fkind = CSSTransformFunction::Kind::Matrix;
                minArgCnt = maxArgCnt = 6;
            } else if (name->equals("translate")) {
                fkind = CSSTransformFunction::Kind::Translate;
                maxArgCnt = 2;
                unit = TranslationValue;
            } else if (name->equals("translatex")) {
                fkind = CSSTransformFunction::Kind::TranslateX;
                unit = TranslationValue;
            } else if (name->equals("translatey")) {
                fkind = CSSTransformFunction::Kind::TranslateY;
                unit = TranslationValue;
            } else if (name->equals("translatez")) {
                continue;
            } else if (name->equals("scale")) {
                maxArgCnt = 2;
                fkind = CSSTransformFunction::Kind::Scale;
            } else if (name->equals("scalex")) {
                fkind = CSSTransformFunction::Kind::ScaleX;
            } else if (name->equals("scaley")) {
                fkind = CSSTransformFunction::Kind::ScaleY;
            } else if (name->equals("rotate")) {
                fkind = CSSTransformFunction::Kind::Rotate;
                unit = Angle;
            } else if (name->equals("skew")) {
                fkind = CSSTransformFunction::Kind::Skew;
                maxArgCnt = 2;
                unit = Angle;
            } else if (name->equals("skewx")) {
                fkind = CSSTransformFunction::Kind::SkewX;
                unit = Angle;
            } else if (name->equals("skewy")) {
                fkind = CSSTransformFunction::Kind::SkewY;
                unit = Angle;
            } else {
                return false;
            }

            ValueList* values = new ValueList(ValueList::Separator::CommaSeparator);
            int idx = -1;
            for (idx = 0; idx < maxArgCnt; idx++) {
                parser->consumeWhitespaces();
                if (!parser->consumeNumber())
                    return false;
                float num = parser->parsedNumber();

                if (unit == Number) {
                    values->append(CSSStyleValuePair::ValueKind::Number, { num});
                } else if (unit == Angle) {
                    String* str = String::emptyString;
                    if (parser->consumeString()) {
                        str = parser->parsedString();
                        if (!((str->length() == 0 && num == 0)
                            || str->equals("deg")
                            || str->equals("grad")
                            || str->equals("rad")
                            || str->equals("turn"))) {
                            return false;
                        }
                    } else if (num != 0) {
                        // After a zero length, the unit identifier is optional
                        return false;
                    }
                    ValueData data = { CSSAngle(str, num)};
                    values->append(CSSStyleValuePair::ValueKind::Angle, data);
                } else { // TranslationValue
                    if (parser->consumeString()) {
                        String* str = parser->parsedString();
                        if (str->equals("%")) {
                            ValueData data = { num / 100.f};
                            values->append(CSSStyleValuePair::ValueKind::Percentage, data);
                        } else {
                            if (!((str->length() == 0 && num == 0)
                                || CSSPropertyParser::isLengthUnit(str))) {
                                return false;
                            }
                            ValueData data = { CSSLength(str, num)};
                            values->append(CSSStyleValuePair::ValueKind::Length, data);
                        }
                    } else if (num != 0) {
                        // After a zero length, the unit identifier is optional
                        return false;
                    }
                }
                parser->consumeWhitespaces();
                if (!parser->consumeIfNext(','))
                    break;
            }
            if (!(parser->consumeIfNext(')') && parser->isEnd()) || idx + 1 < minArgCnt)
                return false;
            m_value.m_transforms->append(CSSTransformFunction(fkind, values));
        }
    }
    return true;
}

bool CSSStyleValuePair::updateValueOpacity(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() != 1)
        return false;

    float f;
    m_valueKind = CSSStyleValuePair::ValueKind::Number;
    if (CSSPropertyParser::parseNumber(tokens->at(0)->utf8Data(), true, &f)) {
        m_value.m_floatValue = f;
        return true;
    }
    return false;
}

bool CSSStyleValuePair::updateValueFontWeight(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() != 1)
        return false;

    String* value = tokens->at(0);
    m_valueKind = CSSStyleValuePair::ValueKind::FontWeightValueKind;

    // <normal> | bold | bolder | lighter | 100 | 200 | 300 | 400 | 500 | 600 | 700 | 800 | 900 | inherit // initial -> normal
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

bool CSSStyleValuePair::updateValueOverflow(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() != 1)
        return false;

    String* value = tokens->at(0);
    m_valueKind = CSSStyleValuePair::ValueKind::OverflowValueKind;

    if (STRING_VALUE_IS_STRING("visible")) {
        m_value.m_overflow = OverflowValue::VisibleOverflow;
    } else if (STRING_VALUE_IS_STRING("hidden")) {
        m_value.m_overflow = OverflowValue::HiddenOverflow;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValuePosition(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() != 1)
        return false;

    String* value = tokens->at(0);
    // <static> | relative | absolute | inherit
    m_valueKind = CSSStyleValuePair::ValueKind::PositionValueKind;

    if (STRING_VALUE_IS_STRING("static")) {
        m_value.m_position = PositionValue::StaticPositionValue;
    } else if (STRING_VALUE_IS_STRING("relative")) {
        m_value.m_position = PositionValue::RelativePositionValue;
    } else if (STRING_VALUE_IS_STRING("absolute")) {
        m_value.m_position = PositionValue::AbsolutePositionValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueTextDecoration(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() != 1)
        return false;

    String* value = tokens->at(0);
    // none | [ underline || line-through ] | inherit // Initial value -> none
    m_valueKind = CSSStyleValuePair::ValueKind::TextDecorationValueKind;

    if (STRING_VALUE_IS_NONE()) {
        m_valueKind = CSSStyleValuePair::ValueKind::None;
        m_value.m_textDecoration = TextDecorationValue::NoneTextDecorationValue;
    } else if (STRING_VALUE_IS_STRING("underline")) {
        m_value.m_textDecoration = TextDecorationValue::UnderLineTextDecorationValue;
    } else if (STRING_VALUE_IS_STRING("line-through")) {
        m_value.m_textDecoration = TextDecorationValue::LineThroughTextDecorationValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueTextAlign(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() != 1)
        return false;

    String* value = tokens->at(0);
    if (STRING_VALUE_IS_STRING("left")) {
        m_valueKind = CSSStyleValuePair::ValueKind::SideValueKind;
        m_value.m_side = SideValue::LeftSideValue;
    } else if (STRING_VALUE_IS_STRING("center")) {
        m_valueKind = CSSStyleValuePair::ValueKind::SideValueKind;
        m_value.m_side = SideValue::CenterSideValue;
    } else if (STRING_VALUE_IS_STRING("right")) {
        m_valueKind = CSSStyleValuePair::ValueKind::SideValueKind;
        m_value.m_side = SideValue::RightSideValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueUnicodeBidi(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() != 1)
        return false;

    String* value = (*tokens)[0];
    if (STRING_VALUE_IS_STRING("normal")) {
        m_value.m_unicodeBidi = UnicodeBidiValue::NormalUnicodeBidiValue;
        m_valueKind = CSSStyleValuePair::ValueKind::UnicodeBidiValueKind;
    } else if (STRING_VALUE_IS_STRING("embed")) {
        m_value.m_unicodeBidi = UnicodeBidiValue::EmbedUnicodeBidiValue;
        m_valueKind = CSSStyleValuePair::ValueKind::UnicodeBidiValueKind;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueVisibility(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() != 1)
        return false;

    m_valueKind = CSSStyleValuePair::ValueKind::VisibilityValueKind;
    String* value = (*tokens)[0];
    if (STRING_VALUE_IS_STRING("visible")) {
        m_value.m_visibility = VisibilityValue::VisibleVisibilityValue;
    } else if (STRING_VALUE_IS_STRING("hidden")) {
        m_value.m_visibility = VisibilityValue::HiddenVisibilityValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueZIndex(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() != 1)
        return false;

    const char* token = tokens->at(0)->utf8Data();
    int32_t val = 0;
    if (TOKEN_IS_STRING("auto")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Auto;
    } else if (CSSPropertyParser::parseInt32(token, true, &val)) {
        m_valueKind = CSSStyleValuePair::ValueKind::Int32;
        m_value.m_int32Value = val;
    } else {
        return false;
    }
    return true;
}


#ifdef STARFISH_ENABLE_TEST
void dump(Node* node, unsigned depth)
{
    if (!node->style()) {
        return;
    }

    for (unsigned i = 0; i < depth; i++) {
        printf("  ");
    }

    node->dumpStyle();
    printf("\n");

    Node* child = node->firstChild();
    while (child) {
        if (child->isElement()) {
            dump(child, depth + 1);
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
