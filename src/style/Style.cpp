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

bool startsWith(const char* base, const char* str)
{
    return (strstr(base, str) - base) == 0;
}

bool endsWith(const char* base, const char* str)
{
    int blen = strlen(base);
    int slen = strlen(str);
    return (blen >= slen) && (0 == strcmp(base + blen - slen, str));
}

bool endsWithNumber(const char* str)
{
    const char* lastCh = (str + strlen(str) - 1);
    return *lastCh >= '0' && *lastCh <= '9';
}

#define TOKEN_IS_STRING(str) \
    (strcmp(token, str)) == 0

#define VALUE_IS_STRING(str) \
    (strcmp(value, str)) == 0

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

CSSLength parseCSSLength(const char* value)
{
    float f;
    if (endsWith(value, "px")) {
        sscanf(value, "%fpx", &f);
        return CSSLength(f);
    } else if (VALUE_IS_STRING("0")) {
        return CSSLength(0.0);
    } else if (endsWith(value, "em")) {
        sscanf(value, "%fem", &f);
        return CSSLength(CSSLength::Kind::EM, f);
    } else if (endsWith(value, "ex")) {
        sscanf(value, "%fex", &f);
        return CSSLength(CSSLength::Kind::EX, f);
    } else if (endsWith(value, "in")) {
        sscanf(value, "%fin", &f);
        return CSSLength(CSSLength::Kind::IN, f);
    } else if (endsWith(value, "cm")) {
        sscanf(value, "%fcm", &f);
        return CSSLength(CSSLength::Kind::CM, f);
    } else if (endsWith(value, "mm")) {
        sscanf(value, "%fmm", &f);
        return CSSLength(CSSLength::Kind::MM, f);
    } else if (endsWith(value, "pt")) {
        sscanf(value, "%fpt", &f);
        return CSSLength(CSSLength::Kind::PT, f);
    } else if (endsWith(value, "pc")) {
        sscanf(value, "%fpc", &f);
        return CSSLength(CSSLength::Kind::PC, f);
    } else {
        STARFISH_LOG_ERROR("unsupported css length value %s\n", value);
        return CSSLength(CSSLength::Kind::PX, 0);
    }
}
void parseLength(CSSStyleValuePair& ret, const char* value)
{
    ret.m_valueKind = CSSStyleValuePair::ValueKind::Length;
    ret.m_value.m_length = parseCSSLength(value);
}

void parsePercentageOrLength(CSSStyleValuePair& ret, const char* value)
{
    if (endsWith(value, "%")) {
        float f;
        sscanf(value, "%f%%", &f);
        ret.m_valueKind = CSSStyleValuePair::ValueKind::Percentage;
        f = f / 100.f;
        ret.m_value.m_floatValue = f;
    } else {
        parseLength(ret, value);
    }
}

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

const int fontSizeTableMax = 16;
const int fontSizeTableMin = 9;

FontWeightValue lighterWeight(FontWeightValue weight)
{
    if (weight == FontWeightValue::OneHundredFontWeightValue
        || weight == FontWeightValue::TwoHundredsFontWeightValue
        || weight == FontWeightValue::ThreeHundredsFontWeightValue
        || weight == FontWeightValue::NormalFontWeightValue
        || weight == FontWeightValue::FiveHundredsFontWeightValue) {
        return FontWeightValue::OneHundredFontWeightValue;
    } else if (weight == FontWeightValue::SixHundredsFontWeightValue
        || weight == FontWeightValue::BoldFontWeightValue) {
        return FontWeightValue::NormalFontWeightValue; // 400
    } else if (weight == FontWeightValue::EightHundredsFontWeightValue
        || weight == FontWeightValue::NineHundredsFontWeightValue) {
        return FontWeightValue::BoldFontWeightValue; // 700
    }

    ASSERT_NOT_REACHED();
    return FontWeightValue::NormalFontWeightValue;
}

FontWeightValue bolderWeight(FontWeightValue weight)
{
    if (weight == FontWeightValue::OneHundredFontWeightValue
        || weight == FontWeightValue::TwoHundredsFontWeightValue
        || weight == FontWeightValue::ThreeHundredsFontWeightValue) {
        return FontWeightValue::NormalFontWeightValue;
    } else if (weight == FontWeightValue::NormalFontWeightValue
        || weight == FontWeightValue::FiveHundredsFontWeightValue) {
        return FontWeightValue::BoldFontWeightValue; // 700
    } else if (weight == FontWeightValue::SixHundredsFontWeightValue
        || weight == FontWeightValue::BoldFontWeightValue
        || weight == FontWeightValue::EightHundredsFontWeightValue
        || weight == FontWeightValue::NineHundredsFontWeightValue) {
        return FontWeightValue::NineHundredsFontWeightValue;
    }
    return FontWeightValue::NormalFontWeightValue;
}

Length parseAbsoluteFontSize(int col)
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

Length convertValueToLength(CSSStyleValuePair::ValueKind kind, CSSStyleValuePair::ValueData data)
{
    if (kind == CSSStyleValuePair::ValueKind::Auto)
        return Length();
    else if (kind == CSSStyleValuePair::ValueKind::Length)
        return data.m_length.toLength();
    else if (kind == CSSStyleValuePair::ValueKind::Percentage)
        return Length(Length::Percent, data.m_floatValue);
    else
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

Length convertPercentOrNumberToLength(CSSStyleValuePair::ValueKind kind, CSSStyleValuePair::ValueData data)
{
    if (kind == CSSStyleValuePair::ValueKind::Percentage)
        return Length(Length::Percent, data.m_floatValue);
    else if (kind == CSSStyleValuePair::ValueKind::Number)
        return Length(Length::Fixed, data.m_floatValue);
    else
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

String* CSSStyleValuePair::keyName()
{
    switch (keyKind()) {
    case Display:
        return String::createASCIIString("display");
    case Position:
        return String::createASCIIString("position");
    case Width:
        return String::createASCIIString("width");
    case Height:
        return String::createASCIIString("height");
    case Color:
        return String::createASCIIString("color");
    case FontSize:
        return String::createASCIIString("font-size");
    case FontStyle:
        return String::createASCIIString("font-style");
    case FontWeight:
        return String::createASCIIString("font-weight");
    case VerticalAlign:
        return String::createASCIIString("vertical-align");
    case TextAlign:
        return String::createASCIIString("text-align");
    case TextDecoration:
        return String::createASCIIString("text-decoration");
    case TextOverflow:
        return String::createASCIIString("text-overflow");
    case Transform:
        return String::createASCIIString("transform");
    case TransformOrigin:
        return String::createASCIIString("transform-origin");
    case Direction:
        return String::createASCIIString("direction");
    case BackgroundColor:
        return String::createASCIIString("background-color");
    case BackgroundImage:
        return String::createASCIIString("background-image");
    case BackgroundSize:
        return String::createASCIIString("background-size");
    case BackgroundRepeatX:
        return String::createASCIIString("background-repeat-x");
    case BackgroundRepeatY:
        return String::createASCIIString("background-repeat-y");
    case Top:
        return String::createASCIIString("top");
    case Right:
        return String::createASCIIString("right");
    case Bottom:
        return String::createASCIIString("bottom");
    case Left:
        return String::createASCIIString("left");
    case LineHeight:
        return String::createASCIIString("line-height");
    case BorderTopColor:
        return String::createASCIIString("border-top-color");
    case BorderRightColor:
        return String::createASCIIString("border-right-color");
    case BorderBottomColor:
        return String::createASCIIString("border-bottom-color");
    case BorderLeftColor:
        return String::createASCIIString("border-left-color");
    case BorderImageRepeat:
        return String::createASCIIString("border-image-repeat");
    case BorderImageSlice:
        return String::createASCIIString("border-image-slice");
    case BorderImageSource:
        return String::createASCIIString("border-image-source");
    case BorderImageWidth:
        return String::createASCIIString("border-image-width");
    case BorderTopStyle:
        return String::createASCIIString("border-top-style");
    case BorderRightStyle:
        return String::createASCIIString("border-right-style");
    case BorderBottomStyle:
        return String::createASCIIString("border-bottom-style");
    case BorderLeftStyle:
        return String::createASCIIString("border-left-style");
    case BorderTopWidth:
        return String::createASCIIString("border-top-width");
    case BorderRightWidth:
        return String::createASCIIString("border-right-width");
    case BorderBottomWidth:
        return String::createASCIIString("border-bottom-width");
    case BorderLeftWidth:
        return String::createASCIIString("border-left-width");
    case MarginTop:
        return String::createASCIIString("margin-top");
    case MarginBottom:
        return String::createASCIIString("margin-bottom");
    case MarginRight:
        return String::createASCIIString("margin-right");
    case MarginLeft:
        return String::createASCIIString("margin-left");
    case PaddingTop:
        return String::createASCIIString("padding-top");
    case PaddingRight:
        return String::createASCIIString("padding-right");
    case PaddingBottom:
        return String::createASCIIString("padding-bottom");
    case PaddingLeft:
        return String::createASCIIString("padding-left");
    case Opacity:
        return String::createASCIIString("opacity");
    case Overflow:
        return String::createASCIIString("overflow");
    case Visibility:
        return String::createASCIIString("visibility");
    case ZIndex:
        return String::createASCIIString("z-index");
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
    } else if (VALUE_IS_AUTO()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Auto;
    } else {
        return false;
    }
    return true;
}

void CSSStyleValuePair::setValueColor(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // color | transparent | inherit
    m_valueKind = CSSStyleValuePair::ValueKind::StringValueKind;
    m_value.m_stringValue = tokens->at(0);
}

void CSSStyleValuePair::setValueBackgroundColor(std::vector<String*, gc_allocator<String*> >* tokens)
{
    setValueColor(tokens);
}

void CSSStyleValuePair::setValueBorderTopColor(std::vector<String*, gc_allocator<String*> >* tokens)
{
    setValueColor(tokens);
}

void CSSStyleValuePair::setValueBorderRightColor(std::vector<String*, gc_allocator<String*> >* tokens)
{
    setValueColor(tokens);
}

void CSSStyleValuePair::setValueBorderBottomColor(std::vector<String*, gc_allocator<String*> >* tokens)
{
    setValueColor(tokens);
}

void CSSStyleValuePair::setValueBorderLeftColor(std::vector<String*, gc_allocator<String*> >* tokens)
{
    setValueColor(tokens);
}

// <length> | <percentage> (| auto)
void CSSStyleValuePair::setValuePercentageOrLength(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    parsePercentageOrLength(*this, value);
}

void CSSStyleValuePair::setValuePercentageOrLength(const char* value)
{
    parsePercentageOrLength(*this, value);
}

void CSSStyleValuePair::setValuePaddingTop(std::vector<String*, gc_allocator<String*> >* tokens)
{
    setValuePercentageOrLength(tokens);
}

void CSSStyleValuePair::setValuePaddingRight(std::vector<String*, gc_allocator<String*> >* tokens)
{
    setValuePercentageOrLength(tokens);
}

void CSSStyleValuePair::setValuePaddingBottom(std::vector<String*, gc_allocator<String*> >* tokens)
{
    setValuePercentageOrLength(tokens);
}

void CSSStyleValuePair::setValuePaddingLeft(std::vector<String*, gc_allocator<String*> >* tokens)
{
    setValuePercentageOrLength(tokens);
}

void CSSStyleValuePair::setValueMarginTop(std::vector<String*, gc_allocator<String*> >* tokens)
{
    setValuePercentageOrLength(tokens);
}

void CSSStyleValuePair::setValueMarginRight(std::vector<String*, gc_allocator<String*> >* tokens)
{
    setValuePercentageOrLength(tokens);
}

void CSSStyleValuePair::setValueMarginBottom(std::vector<String*, gc_allocator<String*> >* tokens)
{
    setValuePercentageOrLength(tokens);
}

void CSSStyleValuePair::setValueMarginLeft(std::vector<String*, gc_allocator<String*> >* tokens)
{
    setValuePercentageOrLength(tokens);
}

void CSSStyleValuePair::setValueTop(std::vector<String*, gc_allocator<String*> >* tokens)
{
    setValuePercentageOrLength(tokens);
}

void CSSStyleValuePair::setValueBottom(std::vector<String*, gc_allocator<String*> >* tokens)
{
    setValuePercentageOrLength(tokens);
}

void CSSStyleValuePair::setValueLeft(std::vector<String*, gc_allocator<String*> >* tokens)
{
    setValuePercentageOrLength(tokens);
}

void CSSStyleValuePair::setValueRight(std::vector<String*, gc_allocator<String*> >* tokens)
{
    setValuePercentageOrLength(tokens);
}

void CSSStyleValuePair::setValueWidth(std::vector<String*, gc_allocator<String*> >* tokens)
{
    setValuePercentageOrLength(tokens);
}

void CSSStyleValuePair::setValueHeight(std::vector<String*, gc_allocator<String*> >* tokens)
{
    setValuePercentageOrLength(tokens);
}

void CSSStyleValuePair::setValueUrlOrNone(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // none | <image>
    const char* value = tokens->at(0)->utf8Data();
    if (VALUE_IS_STRING("none")) {
        m_valueKind = CSSStyleValuePair::ValueKind::None;
    } else if (startsWith(value, "url(")) {
        m_valueKind = CSSStyleValuePair::ValueKind::UrlValueKind;
        m_value.m_stringValue = CSSPropertyParser::parseUrl(tokens, 0, tokens->size());
    }
}

void CSSStyleValuePair::setValueBackgroundImage(std::vector<String*, gc_allocator<String*> >* tokens)
{
    setValueUrlOrNone(tokens);
}

void CSSStyleValuePair::setValueBorderImageSource(std::vector<String*, gc_allocator<String*> >* tokens)
{
    setValueUrlOrNone(tokens);
}

void CSSStyleValuePair::setValueDirection(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // <ltr> | rtl | inherit
    m_valueKind = CSSStyleValuePair::ValueKind::DirectionValueKind;
    if (VALUE_IS_STRING("ltr")) {
        m_value.m_direction = DirectionValue::LtrDirectionValue;
    } else if (VALUE_IS_STRING("rtl")) {
        m_value.m_direction = DirectionValue::RtlDirectionValue;
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

void CSSStyleValuePair::setValueFontSize(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // absolute-size | relative-size | length | percentage | inherit // initial value -> medium
    //        O      |       O       |   O    |    O       |    O
    if (VALUE_IS_STRING("xx-small")) {
        m_valueKind = CSSStyleValuePair::ValueKind::XXSmallFontSizeValueKind;
    } else if (VALUE_IS_STRING("x-small")) {
        m_valueKind = CSSStyleValuePair::ValueKind::XSmallFontSizeValueKind;
    } else if (VALUE_IS_STRING("small")) {
        m_valueKind = CSSStyleValuePair::ValueKind::SmallFontSizeValueKind;
    } else if (VALUE_IS_STRING("medium")) {
        m_valueKind = CSSStyleValuePair::ValueKind::MediumFontSizeValueKind;
    } else if (VALUE_IS_STRING("large")) {
        m_valueKind = CSSStyleValuePair::ValueKind::LargeFontSizeValueKind;
    } else if (VALUE_IS_STRING("x-large")) {
        m_valueKind = CSSStyleValuePair::ValueKind::XLargeFontSizeValueKind;
    } else if (VALUE_IS_STRING("xx-large")) {
        m_valueKind = CSSStyleValuePair::ValueKind::XXLargeFontSizeValueKind;
    } else if (VALUE_IS_STRING("larger")) {
        m_valueKind = CSSStyleValuePair::ValueKind::LargerFontSizeValueKind;
    } else if (VALUE_IS_STRING("smaller")) {
        m_valueKind = CSSStyleValuePair::ValueKind::SmallerFontSizeValueKind;
    } else {
        setValuePercentageOrLength(value);
    }
}

void CSSStyleValuePair::setValueFontWeight(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();

    // <normal> | bold | bolder | lighter | 100 | 200 | 300 | 400 | 500 | 600 | 700 | 800 | 900 | inherit // initial -> normal
    if (VALUE_IS_STRING("normal")) {
        m_valueKind = CSSStyleValuePair::ValueKind::FontWeightValueKind;
        m_value.m_fontWeight = FontWeightValue::NormalFontWeightValue;
    } else if (VALUE_IS_STRING("bold")) {
        m_valueKind = CSSStyleValuePair::ValueKind::FontWeightValueKind;
        m_value.m_fontWeight = FontWeightValue::BoldFontWeightValue;
    } else if (VALUE_IS_STRING("bolder")) {
        m_valueKind = CSSStyleValuePair::ValueKind::FontWeightValueKind;
        m_value.m_fontWeight = FontWeightValue::BolderFontWeightValue;
    } else if (VALUE_IS_STRING("lighter")) {
        m_valueKind = CSSStyleValuePair::ValueKind::FontWeightValueKind;
        m_value.m_fontWeight = FontWeightValue::LighterFontWeightValue;
    } else if (VALUE_IS_STRING("100")) {
        m_valueKind = CSSStyleValuePair::ValueKind::FontWeightValueKind;
        m_value.m_fontWeight = FontWeightValue::OneHundredFontWeightValue;
    } else if (VALUE_IS_STRING("200")) {
        m_valueKind = CSSStyleValuePair::ValueKind::FontWeightValueKind;
        m_value.m_fontWeight = FontWeightValue::TwoHundredsFontWeightValue;
    } else if (VALUE_IS_STRING("300")) {
        m_valueKind = CSSStyleValuePair::ValueKind::FontWeightValueKind;
        m_value.m_fontWeight = FontWeightValue::ThreeHundredsFontWeightValue;
    } else if (VALUE_IS_STRING("400")) {
        m_valueKind = CSSStyleValuePair::ValueKind::FontWeightValueKind;
        m_value.m_fontWeight = FontWeightValue::FourHundredsFontWeightValue;
    } else if (VALUE_IS_STRING("500")) {
        m_valueKind = CSSStyleValuePair::ValueKind::FontWeightValueKind;
        m_value.m_fontWeight = FontWeightValue::FiveHundredsFontWeightValue;
    } else if (VALUE_IS_STRING("600")) {
        m_valueKind = CSSStyleValuePair::ValueKind::FontWeightValueKind;
        m_value.m_fontWeight = FontWeightValue::SixHundredsFontWeightValue;
    } else if (VALUE_IS_STRING("700")) {
        m_valueKind = CSSStyleValuePair::ValueKind::FontWeightValueKind;
        m_value.m_fontWeight = FontWeightValue::SevenHundredsFontWeightValue;
    } else if (VALUE_IS_STRING("800")) {
        m_valueKind = CSSStyleValuePair::ValueKind::FontWeightValueKind;
        m_value.m_fontWeight = FontWeightValue::EightHundredsFontWeightValue;
    } else if (VALUE_IS_STRING("900")) {
        m_valueKind = CSSStyleValuePair::ValueKind::FontWeightValueKind;
        m_value.m_fontWeight = FontWeightValue::NineHundredsFontWeightValue;
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

void CSSStyleValuePair::setValueFontStyle(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // <normal> | italic | oblique | inherit
    if (VALUE_IS_STRING("normal")) {
        m_valueKind = CSSStyleValuePair::ValueKind::FontStyleValueKind;
        m_value.m_fontStyle = FontStyleValue::NormalFontStyleValue;
    } else if (VALUE_IS_STRING("italic")) {
        m_valueKind = CSSStyleValuePair::ValueKind::FontStyleValueKind;
        m_value.m_fontStyle = FontStyleValue::ItalicFontStyleValue;
    } else if (VALUE_IS_STRING("oblique")) {
        m_valueKind = CSSStyleValuePair::ValueKind::FontStyleValueKind;
        m_value.m_fontStyle = FontStyleValue::ObliqueFontStyleValue;
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

void CSSStyleValuePair::setValueDisplay(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // <inline> | block | list-item | inline-block | table | inline-table | table-row-group | table-header-group | table-footer-group | table-row | table-column-group | table-column | table-cell | table-caption | none | inherit
    m_valueKind = CSSStyleValuePair::ValueKind::DisplayValueKind;
    if (VALUE_IS_STRING("block")) {
        m_value.m_display = DisplayValue::BlockDisplayValue;
    } else if (VALUE_IS_STRING("inline")) {
        m_value.m_display = DisplayValue::InlineDisplayValue;
    } else if (VALUE_IS_STRING("inline-block")) {
        m_value.m_display = DisplayValue::InlineBlockDisplayValue;
    } else if (VALUE_IS_STRING("none")) {
        m_value.m_display = DisplayValue::NoneDisplayValue;
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

void CSSStyleValuePair::setValuePosition(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // <static> | relative | absolute | inherit
    m_valueKind = CSSStyleValuePair::ValueKind::PositionValueKind;

    if (VALUE_IS_STRING("static")) {
        m_value.m_position = PositionValue::StaticPositionValue;
    } else if (VALUE_IS_STRING("relative")) {
        m_value.m_position = PositionValue::RelativePositionValue;
    } else if (VALUE_IS_STRING("absolute")) {
        m_value.m_position = PositionValue::AbsolutePositionValue;
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

void CSSStyleValuePair::setValueTextDecoration(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // none | [ underline || overline || line-through || blink ] | inherit // Initial value -> none
    m_valueKind = CSSStyleValuePair::ValueKind::TextDecorationKind;

    if (VALUE_IS_NONE()) {
        m_valueKind = CSSStyleValuePair::ValueKind::None;
        m_value.m_textDecoration = TextDecorationValue::NoneTextDecorationValue;
    } else if (VALUE_IS_STRING("underline")) {
        m_value.m_textDecoration = TextDecorationValue::UnderLineTextDecorationValue;
    } else if (VALUE_IS_STRING("overline")) {
        m_value.m_textDecoration = TextDecorationValue::OverLineTextDecorationValue;
    } else if (VALUE_IS_STRING("line-through")) {
        m_value.m_textDecoration = TextDecorationValue::LineThroughTextDecorationValue;
    } else if (VALUE_IS_STRING("blink")) {
        m_value.m_textDecoration = TextDecorationValue::BlinkTextDecorationValue;
    }
}

void CSSStyleValuePair::setValueBackgroundSize(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // [length | percentage | auto]{1, 2} | cover | contain // initial value -> auto

    String* token = (*tokens)[0];
    if (token->equals("cover")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Cover;
    } else if (token->equals("contain")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Contain;
    } else {
        m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
        ValueList* values = new ValueList(ValueList::Separator::SpaceSeparator);
        m_value.m_multiValue = values;
        for (unsigned int i = 0; i < tokens->size(); i++) {
            token = (*tokens)[i];
            CSSPropertyParser* parser = new CSSPropertyParser((char*)token->utf8Data());
            // NOTE: CSS 2.1 does not support layering multiple background images(for comma-separated)
            CSSStyleValuePair::ValueKind kind;
            while (parser->findNextValueKind(' ', &kind)) {
                if (kind == CSSStyleValuePair::ValueKind::Auto) {
                    values->append(kind, { 0 });
                } else if (kind == CSSStyleValuePair::ValueKind::Percentage) {
                    values->append(kind, { parser->parsedFloatValue() });
                } else if (kind == CSSStyleValuePair::ValueKind::Length) {
                    CSSStyleValuePair::ValueData data = { CSSLength(parser->parsedFloatValue()) };
                    values->append(kind, data);
                }
            }
        }
    }
}

void CSSStyleValuePair::setValueBackgroundRepeatX(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // repeat | no-repeat | initial | inherit // initial value -> repeat
    const char* value = tokens->at(0)->utf8Data();
    m_valueKind = CSSStyleValuePair::ValueKind::BackgroundRepeatValueKind;

    if (VALUE_IS_STRING("no-repeat")) {
        m_value.m_backgroundRepeatX = BackgroundRepeatValue::NoRepeatRepeatValue;
    } else if (VALUE_IS_STRING("repeat")) {
        m_value.m_backgroundRepeatX = BackgroundRepeatValue::RepeatRepeatValue;
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

void CSSStyleValuePair::setValueBackgroundRepeatY(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // repeat | no-repeat | initial | inherit // initial value -> repeat
    const char* value = tokens->at(0)->utf8Data();
    m_valueKind = CSSStyleValuePair::ValueKind::BackgroundRepeatValueKind;

    if (VALUE_IS_STRING("no-repeat")) {
        m_value.m_backgroundRepeatY = BackgroundRepeatValue::NoRepeatRepeatValue;
    } else if (VALUE_IS_STRING("repeat")) {
        m_value.m_backgroundRepeatY = BackgroundRepeatValue::RepeatRepeatValue;
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

void CSSStyleValuePair::setValueLineHeight(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // <normal> | number | length | percentage | inherit
    const char* value = tokens->at(0)->utf8Data();
    m_valueKind = CSSStyleValuePair::ValueKind::Normal;
    if (VALUE_IS_STRING("normal")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Normal;
    } else {
        char* pEnd;
        double d = strtod(value, &pEnd);
        if (pEnd == value + strlen(value)) {
            m_valueKind = CSSStyleValuePair::ValueKind::Number;
            m_value.m_floatValue = d;
        } else {
            parsePercentageOrLength(*this, value);
        }
    }
}

void CSSStyleValuePair::setValueBorderImageSlice(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // number | percentage {1, 4} && fill?
    m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
    m_value.m_multiValue = new ValueList();
    for (unsigned int i = 0; i < tokens->size(); i++) {
        const char* currentToken = tokens->at(i)->utf8Data();
        if (startsWith(currentToken, "fill")) {
            m_value.m_multiValue->append(CSSStyleValuePair::ValueKind::StringValueKind, { 0 });
        } else {
            char* pEnd;
            double d = strtod(currentToken, &pEnd);
            STARFISH_ASSERT(pEnd == currentToken + tokens->at(i)->length());
            m_value.m_multiValue->append(CSSStyleValuePair::ValueKind::Number, { (float)d });
        }
    }
}

void CSSStyleValuePair::setValueBorderImageRepeat(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // <stretch> | repeat | round | space {1, 2}
    m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
    m_value.m_multiValue = new ValueList();

    // TODO: find better way to parse axis data
    // 1) parse X-axis data
    if (startsWith(value, "stretch")) {
        m_value.m_multiValue->append(BorderImageRepeatValueKind, { StretchValue });
    } else if (startsWith(value, "repeat")) {
        m_value.m_multiValue->append(BorderImageRepeatValueKind, { RepeatValue });
    } else if (startsWith(value, "round")) {
        m_value.m_multiValue->append(BorderImageRepeatValueKind, { RoundValue });
    } else if (startsWith(value, "space")) {
        m_value.m_multiValue->append(BorderImageRepeatValueKind, { SpaceValue });
    }
    // 2) parse Y-axis data
    if (endsWith(value, "stretch")) {
        m_value.m_multiValue->append(BorderImageRepeatValueKind, { StretchValue });
    } else if (endsWith(value, "repeat")) {
        m_value.m_multiValue->append(BorderImageRepeatValueKind, { RepeatValue });
    } else if (endsWith(value, "round")) {
        m_value.m_multiValue->append(BorderImageRepeatValueKind, { RoundValue });
    } else if (endsWith(value, "space")) {
        m_value.m_multiValue->append(BorderImageRepeatValueKind, { SpaceValue });
    }
}

void CSSStyleValuePair::setValueBorderImageWidth(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // [length | number]
    m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
    ValueList* values = new ValueList();
    for (unsigned int i = 0; i < tokens->size(); i++) {
        const char* currentToken = tokens->at(i)->utf8Data();
        if (endsWithNumber(currentToken)) {
            char* pEnd;
            double d = strtod(currentToken, &pEnd);
            STARFISH_ASSERT(pEnd == currentToken + tokens->at(i)->length());
            values->append(CSSStyleValuePair::ValueKind::Number, { (float)d });
        } else {
            CSSStyleValuePair::ValueData data = { parseCSSLength(currentToken) };
            values->append(CSSStyleValuePair::ValueKind::Length, data);
        }
    }
    m_value.m_multiValue = values;
}

void CSSStyleValuePair::setValueTransform(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    if (VALUE_IS_STRING("none")) {
        m_valueKind = CSSStyleValuePair::ValueKind::None;
    } else {
        m_valueKind = CSSStyleValuePair::ValueKind::TransformFunctions;
        CSSTransformFunction::Kind fkind;
        m_value.m_transforms = new CSSTransformFunctions();

        bool hasClosing = false;
        for (unsigned i = 0; i< tokens->size(); i++) {
            if (tokens->at(i)->indexOf(')') != SIZE_MAX) {
                hasClosing = true;
                break;
            }
        }
        if (hasClosing) {
            String* str = String::emptyString;
            for (unsigned i = 0; i< tokens->size(); i++) {
                str = str->concat(tokens->at(i));
            }
            tokens->clear();
            str->split(')', *tokens);
        }
        for (unsigned i = 0; i < tokens->size(); i++) {
            CSSPropertyParser* parser = new CSSPropertyParser((char*)tokens->at(i)->utf8Data());
            parser->consumeString() && parser->consumeIfNext('(');
            String* name = parser->parsedString();
            enum {
                Number, // <number>
                Angle, // <angle>
                TranslationValue // <translation-value>: percentage or length
            } unit = Number;
            int expectedArgCnt = 1;
            if (name->equals("matrix")) {
                fkind = CSSTransformFunction::Kind::Matrix;
                expectedArgCnt = 6;
            } else if (name->equals("translate")) {
                fkind = CSSTransformFunction::Kind::Translate;
                expectedArgCnt = 2;
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
                expectedArgCnt = 2;
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
                expectedArgCnt = 2;
                unit = Angle;
            } else if (name->equals("skewx")) {
                fkind = CSSTransformFunction::Kind::SkewX;
                unit = Angle;
            } else if (name->equals("skewy")) {
                fkind = CSSTransformFunction::Kind::SkewY;
                unit = Angle;
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }

            ValueList* values = new ValueList(ValueList::Separator::CommaSeparator);
            for (int i = 0; i < expectedArgCnt; i++) {
                parser->consumeWhitespaces();
                if (unit == Number) {
                    parser->consumeNumber();
                    float num = parser->parsedNumber();
                    values->append(CSSStyleValuePair::ValueKind::Number, { num});
                } else if (unit == Angle) {
                    parser->consumeNumber();
                    parser->consumeString();
                    ValueData data = { CSSAngle(parser->parsedString(), parser->parsedNumber())};
                    values->append(CSSStyleValuePair::ValueKind::Angle, data);
                } else { // TranslationValue
                    parser->consumeNumber();
                    float num = parser->parsedNumber();
                    if (parser->consumeString()) {
                        String* str = parser->parsedString();
                        if (str->equals("%")) {
                            ValueData data = { num / 100.f};
                            values->append(CSSStyleValuePair::ValueKind::Percentage, data);
                        } else {
                            ValueData data = { CSSLength(str, num)};
                            values->append(CSSStyleValuePair::ValueKind::Length, data);
                        }
                    }
                }
                parser->consumeWhitespaces();
                if (!parser->consumeIfNext(','))
                    break;
            }
            m_value.m_transforms->append(CSSTransformFunction(fkind, values));
        }
    }
}

void CSSStyleValuePair::setValueTransformOrigin(std::vector<String*, gc_allocator<String*> >* tokens)
{
    //  [ left | center | right | top | bottom | <percentage> | <length> ]
    // |
    //  [ left | center | right | <percentage> | <length> ]
    //  [ top | center | bottom | <percentage> | <length> ] <length>?
    // |
    //  [ center | [ left | right ] ] && [ center | [ top | bottom ] ] <length>?

    const char* value = tokens->at(0)->utf8Data();

    m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
    ValueList* values = new ValueList();

    for (unsigned int i = 0; i < tokens->size(); i++) {
        value = tokens->at(i)->utf8Data();
        if (VALUE_IS_STRING("left")) {
            values->append(CSSStyleValuePair::ValueKind::TransformOriginLeft, { 0 });
        } else if (VALUE_IS_STRING("right")) {
            values->append(CSSStyleValuePair::ValueKind::TransformOriginRight, { 0 });
        } else if (VALUE_IS_STRING("center")) {
            values->append(CSSStyleValuePair::ValueKind::TransformOriginCenter, { 0 });
        } else if (VALUE_IS_STRING("top")) {
            values->append(CSSStyleValuePair::ValueKind::TransformOriginTop, { 0 });
        } else if (VALUE_IS_STRING("bottom")) {
            values->append(CSSStyleValuePair::ValueKind::TransformOriginBottom, { 0 });
        } else {
            float result;
            String* unit = CSSPropertyParser::parseNumberAndUnit(value, &result);
            if (unit->equals("%")) {
                values->append(CSSStyleValuePair::ValueKind::Percentage, { (result / 100.f) });
            } else {
                ValueData data = { CSSLength(unit, result)};
                values->append(CSSStyleValuePair::ValueKind::Length, data);
            }
        }
    }

    m_value.m_multiValue = values;
}

String* BorderString(String* width, String* style, String* color)
{
    String* space = String::spaceString;
    String* sum = String::emptyString;

    if (!width->equals(String::emptyString)) {
        sum = width;
        if (!style->equals(String::emptyString)) {
            sum = sum->concat(space)->concat(style);
            if (!color->equals(String::emptyString)) {
                sum = sum->concat(space)->concat(color);
            }
        } else {
            if (!color->equals(String::emptyString)) {
                sum = sum->concat(space)->concat(color);
            }
        }
    } else {
        if (!style->equals(String::emptyString)) {
            sum = style;
            if (!color->equals(String::emptyString)) {
                sum = sum->concat(space)->concat(color);
            }
        } else {
            if (!color->equals(String::emptyString)) {
                sum = color;
            }
        }
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

BorderShorthandValueType checkBorderValueType(const char* token)
{
    if (CSSPropertyParser::assureBorderWidth(token)) {
        return BorderShorthandValueType::BWidth;
    } else if (CSSPropertyParser::assureBorderStyle(token)) {
        return BorderShorthandValueType::BStyle;
    } else if (CSSPropertyParser::assureColor(token)) {
        return BorderShorthandValueType::BColor;
    } else {
        return BorderShorthandValueType::BInvalid;
    }
}

void parseBorderWidthStyleColor(std::vector<String*, gc_allocator<String*> >* tokens, String** result)
{
    size_t len = tokens->size();
    String* value = tokens->at(0);
    if (len == 0) {
        result[0] = result[1] = result[2] = String::emptyString;
    } else if (len == 1 && STRING_VALUE_IS_INHERIT()) {
        result[0] = result[1] = result[2] = String::inheritString;
    } else if (len == 1 && STRING_VALUE_IS_INITIAL()) {
        result[0] = result[1] = result[2] = String::initialString;
    } else {
        for (unsigned i = 0; i < tokens->size(); i++) {
            String* str = tokens->at(i);
            switch (checkBorderValueType(str->utf8Data())) {
            case BorderShorthandValueType::BWidth:
                result[0] = str;
                break;
            case BorderShorthandValueType::BStyle:
                result[1] = str;
                break;
            case BorderShorthandValueType::BColor:
                result[2] = str;
                break;
            default:
                break;
            }
        }
    }
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
    if (checkInputErrorBorder(&tokens)) {
        String** result = new(GC) String*[3];
        result[0] = result[1] = result[2] = String::initialString;
        parseBorderWidthStyleColor(&tokens, result);
        setBorderWidth(result[0]);
        setBorderStyle(result[1]);
        setBorderColor(result[2]);
    }
}

void CSSStyleDeclaration::setBorderTop(String* value)
{
    if (value->length() == 0) {
        setBorderTopWidth(String::emptyString);
        setBorderTopStyle(String::emptyString);
        setBorderTopColor(String::emptyString);
        return;
    }
    std::vector<String*, gc_allocator<String*> > tokens;
    tokenizeCSSValue(&tokens, value);
    if (checkInputErrorBorderTop(&tokens)) {
        String** result = new(GC) String*[3];
        result[0] = result[1] = result[2] = String::initialString;
        parseBorderWidthStyleColor(&tokens, result);
        setBorderTopWidth(result[0]);
        setBorderTopStyle(result[1]);
        setBorderTopColor(result[2]);
    }
}

void CSSStyleDeclaration::setBorderRight(String* value)
{
    if (value->length() == 0) {
        setBorderRightWidth(String::emptyString);
        setBorderRightStyle(String::emptyString);
        setBorderRightColor(String::emptyString);
        return;
    }
    std::vector<String*, gc_allocator<String*> > tokens;
    tokenizeCSSValue(&tokens, value);
    if (checkInputErrorBorderRight(&tokens)) {
        String** result = new(GC) String*[3];
        result[0] = result[1] = result[2] = String::initialString;
        parseBorderWidthStyleColor(&tokens, result);
        setBorderRightWidth(result[0]);
        setBorderRightStyle(result[1]);
        setBorderRightColor(result[2]);
    }
}

void CSSStyleDeclaration::setBorderBottom(String* value)
{
    if (value->length() == 0) {
        setBorderBottomWidth(String::emptyString);
        setBorderBottomStyle(String::emptyString);
        setBorderBottomColor(String::emptyString);
        return;
    }
    std::vector<String*, gc_allocator<String*> > tokens;
    tokenizeCSSValue(&tokens, value);
    if (checkInputErrorBorderBottom(&tokens)) {
        String** result = new(GC) String*[3];
        result[0] = result[1] = result[2] = String::initialString;
        parseBorderWidthStyleColor(&tokens, result);
        setBorderBottomWidth(result[0]);
        setBorderBottomStyle(result[1]);
        setBorderBottomColor(result[2]);
    }
}

void CSSStyleDeclaration::setBorderLeft(String* value)
{
    if (value->length() == 0) {
        setBorderLeftWidth(String::emptyString);
        setBorderLeftStyle(String::emptyString);
        setBorderLeftColor(String::emptyString);
        return;
    }
    std::vector<String*, gc_allocator<String*> > tokens;
    tokenizeCSSValue(&tokens, value);
    if (checkInputErrorBorderLeft(&tokens)) {
        String** result = new(GC) String*[3];
        result[0] = result[1] = result[2] = String::initialString;
        parseBorderWidthStyleColor(&tokens, result);
        setBorderLeftWidth(result[0]);
        setBorderLeftStyle(result[1]);
        setBorderLeftColor(result[2]);
    }
}

void CSSStyleValuePair::setValueBorderUnitStyle(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // border-style(<none> | solid)
    m_valueKind = CSSStyleValuePair::ValueKind::BorderNone;
    if (VALUE_IS_STRING("solid")) {
        m_valueKind = CSSStyleValuePair::ValueKind::BorderSolid;
    }
}

void CSSStyleValuePair::setValueBorderTopStyle(std::vector<String*, gc_allocator<String*> >* tokens)
{
    setValueBorderUnitStyle(tokens);
}

void CSSStyleValuePair::setValueBorderRightStyle(std::vector<String*, gc_allocator<String*> >* tokens)
{
    setValueBorderUnitStyle(tokens);
}

void CSSStyleValuePair::setValueBorderBottomStyle(std::vector<String*, gc_allocator<String*> >* tokens)
{
    setValueBorderUnitStyle(tokens);
}

void CSSStyleValuePair::setValueBorderLeftStyle(std::vector<String*, gc_allocator<String*> >* tokens)
{
    setValueBorderUnitStyle(tokens);
}

void CSSStyleValuePair::setValueBorderUnitWidth(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // border-width(thin | <medium> | thick | length)
    if (VALUE_IS_STRING("thin")) {
        m_valueKind = CSSStyleValuePair::ValueKind::BorderThin;
    } else if (VALUE_IS_STRING("medium")) {
        m_valueKind = CSSStyleValuePair::ValueKind::BorderMedium;
    } else if (VALUE_IS_STRING("thick")) {
        m_valueKind = CSSStyleValuePair::ValueKind::BorderThick;
    } else {
        setValuePercentageOrLength(value);
    }
}

void CSSStyleValuePair::setValueBorderTopWidth(std::vector<String*, gc_allocator<String*> >* tokens)
{
    setValueBorderUnitWidth(tokens);
}

void CSSStyleValuePair::setValueBorderRightWidth(std::vector<String*, gc_allocator<String*> >* tokens)
{
    setValueBorderUnitWidth(tokens);
}

void CSSStyleValuePair::setValueBorderBottomWidth(std::vector<String*, gc_allocator<String*> >* tokens)
{
    setValueBorderUnitWidth(tokens);
}

void CSSStyleValuePair::setValueBorderLeftWidth(std::vector<String*, gc_allocator<String*> >* tokens)
{
    setValueBorderUnitWidth(tokens);
}

void CSSStyleValuePair::setValueTextAlign(std::vector<String*, gc_allocator<String*> >* tokens)
{

    const char* value = tokens->at(0)->utf8Data();
    m_valueKind = CSSStyleValuePair::ValueKind::Inherit;

    if (VALUE_IS_STRING("left")) {
        m_valueKind = CSSStyleValuePair::ValueKind::TextAlignValueKind;
        m_value.m_textAlign = TextAlignValue::LeftTextAlignValue;
    } else if (VALUE_IS_STRING("center")) {
        m_valueKind = CSSStyleValuePair::ValueKind::TextAlignValueKind;
        m_value.m_textAlign = TextAlignValue::CenterTextAlignValue;
    } else if (VALUE_IS_STRING("right")) {
        m_valueKind = CSSStyleValuePair::ValueKind::TextAlignValueKind;
        m_value.m_textAlign = TextAlignValue::RightTextAlignValue;
    } else if (VALUE_IS_STRING("justify")) {
        m_valueKind = CSSStyleValuePair::ValueKind::TextAlignValueKind;
        m_value.m_textAlign = TextAlignValue::JustifyTextAlignValue;
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

void CSSStyleValuePair::setValueUnicodeBidi(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();

    if (VALUE_IS_STRING("normal")) {
        m_value.m_unicodeBidi = UnicodeBidiValue::NormalUnicodeBidiValue;
        m_valueKind = CSSStyleValuePair::ValueKind::UnicodeBidiKind;
    } else if (VALUE_IS_STRING("embed")) {
        m_value.m_unicodeBidi = UnicodeBidiValue::EmbedUnicodeBidiValue;
        m_valueKind = CSSStyleValuePair::ValueKind::UnicodeBidiKind;
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

void CSSStyleValuePair::setValueVisibility(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();

    m_valueKind = CSSStyleValuePair::ValueKind::VisibilityKind;

    if (VALUE_IS_STRING("visible")) {
        m_value.m_visibility = VisibilityValue::VisibleVisibilityValue;
    } else if (VALUE_IS_STRING("hidden")) {
        m_value.m_visibility = VisibilityValue::HiddenVisibilityValue;
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

void CSSStyleValuePair::setValueOpacity(std::vector<String*, gc_allocator<String*> >* tokens)
{
    m_valueKind = CSSStyleValuePair::ValueKind::Number;
    float f = CSSPropertyParser::parseNumber(tokens->at(0)->utf8Data());
    m_value.m_floatValue = f; 
}

void CSSStyleValuePair::setValueOverflow(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();

    m_valueKind = CSSStyleValuePair::ValueKind::OverflowValueKind;

    if (VALUE_IS_STRING("visible")) {
        m_value.m_overflow = OverflowValue::VisibleOverflow;
    } else if (VALUE_IS_STRING("hidden")) {
        m_value.m_overflow = OverflowValue::HiddenOverflow;
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}
void CSSStyleValuePair::setValueZIndex(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    m_valueKind = CSSStyleValuePair::ValueKind::Int32;
    // TODO check string has right color string
    if (atof(value) - atoi(value)) {
        m_value.m_int32Value = 0;
    } else {
        sscanf(value, "%d%%", &m_value.m_int32Value);
    }
}

void CSSStyleValuePair::setValueVerticalAlign(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    if (VALUE_IS_STRING("baseline")) {
        m_valueKind = CSSStyleValuePair::ValueKind::VerticalAlignValueKind;
        m_value.m_verticalAlign = VerticalAlignValue::BaselineVAlignValue;
    } else if (VALUE_IS_STRING("sub")) {
        m_valueKind = CSSStyleValuePair::ValueKind::VerticalAlignValueKind;
        m_value.m_verticalAlign = VerticalAlignValue::SubVAlignValue;
    } else if (VALUE_IS_STRING("super")) {
        m_valueKind = CSSStyleValuePair::ValueKind::VerticalAlignValueKind;
        m_value.m_verticalAlign = VerticalAlignValue::SuperVAlignValue;
    } else if (VALUE_IS_STRING("top")) {
        m_valueKind = CSSStyleValuePair::ValueKind::VerticalAlignValueKind;
        m_value.m_verticalAlign = VerticalAlignValue::TopVAlignValue;
    } else if (VALUE_IS_STRING("text-top")) {
        m_valueKind = CSSStyleValuePair::ValueKind::VerticalAlignValueKind;
        m_value.m_verticalAlign = VerticalAlignValue::TextTopVAlignValue;
    } else if (VALUE_IS_STRING("middle")) {
        m_valueKind = CSSStyleValuePair::ValueKind::VerticalAlignValueKind;
        m_value.m_verticalAlign = VerticalAlignValue::MiddleVAlignValue;
    } else if (VALUE_IS_STRING("bottom")) {
        m_valueKind = CSSStyleValuePair::ValueKind::VerticalAlignValueKind;
        m_value.m_verticalAlign = VerticalAlignValue::BottomVAlignValue;
    } else if (VALUE_IS_STRING("text-bottom")) {
        m_valueKind = CSSStyleValuePair::ValueKind::VerticalAlignValueKind;
        m_value.m_verticalAlign = VerticalAlignValue::TextBottomVAlignValue;
    } else {
        setValuePercentageOrLength(value);
    }
}

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
        case CSSStyleValuePair::ValueKind::StringValueKind:
            return stringValue();
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
            if (backgroundRepeatYValue() == RepeatRepeatValue)
                return String::fromUTF8("repeat");
            else
                return String::fromUTF8("no-repeat");
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case BackgroundSize: {
        // [length | percentage | auto]{1, 2} | cover | contain // initial value -> auto
        switch (m_valueKind) {
        case CSSStyleValuePair::ValueKind::Cover:
            return String::fromUTF8("cover");
        case CSSStyleValuePair::ValueKind::Contain:
            return String::fromUTF8("contain");
        case CSSStyleValuePair::ValueKind::ValueListKind: {
            String* str = String::emptyString;
            ValueList* vals = multiValue();
            for (unsigned int i = 0; i < vals->size(); i++) {
                str = str->concat(valueToString(vals->getValueKindAtIndex(i),
                    vals->getValueAtIndex(i)));
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
        case CSSStyleValuePair::ValueKind::XXSmallFontSizeValueKind:
            return String::fromUTF8("xx-small");
        case CSSStyleValuePair::ValueKind::XSmallFontSizeValueKind:
            return String::fromUTF8("x-small");
        case CSSStyleValuePair::ValueKind::SmallFontSizeValueKind:
            return String::fromUTF8("small");
        case CSSStyleValuePair::ValueKind::MediumFontSizeValueKind:
            return String::fromUTF8("medium");
        case CSSStyleValuePair::ValueKind::LargeFontSizeValueKind:
            return String::fromUTF8("large");
        case CSSStyleValuePair::ValueKind::XLargeFontSizeValueKind:
            return String::fromUTF8("x-large");
        case CSSStyleValuePair::ValueKind::XXLargeFontSizeValueKind:
            return String::fromUTF8("xx-large");
        case CSSStyleValuePair::ValueKind::LargerFontSizeValueKind:
            return String::fromUTF8("larger");
        case CSSStyleValuePair::ValueKind::SmallerFontSizeValueKind:
            return String::fromUTF8("smaller");
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
    case BorderImageRepeat: {
        switch (valueKind()) {
        case CSSStyleValuePair::ValueKind::ValueListKind: {
            ValueList* values = multiValue();
            String* s = String::emptyString;
            for (unsigned int i = 0; i < values->size(); i++) {
                STARFISH_ASSERT(values->getValueKindAtIndex(i) == BorderImageRepeatValueKind);
                switch (values->getValueAtIndex(i).m_borderImageRepeat) {
                case BorderImageRepeatValue::StretchValue:
                    s = s->concat(String::fromUTF8("stretch"));
                    break;
                case BorderImageRepeatValue::RepeatValue:
                    s = s->concat(String::fromUTF8("repeat"));
                    break;
                case BorderImageRepeatValue::RoundValue:
                    s = s->concat(String::fromUTF8("round"));
                    break;
                case BorderImageRepeatValue::SpaceValue:
                    s = s->concat(String::fromUTF8("space"));
                    break;
                default:
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
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
    case BorderImageSlice: {
        switch (valueKind()) {
        case CSSStyleValuePair::ValueKind::ValueListKind: {
            ValueList* values = multiValue();
            String* s = String::emptyString;
            for (unsigned int i = 0; i < values->size(); i++) {
                String* newstr;
                if (values->getValueKindAtIndex(i) == CSSStyleValuePair::ValueKind::StringValueKind)
                    newstr = String::fromUTF8("fill");
                else
                    newstr = valueToString(values->getValueKindAtIndex(i),
                        values->getValueAtIndex(i));
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
                String* newstr = valueToString(values->getValueKindAtIndex(i),
                    values->getValueAtIndex(i));
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
        case CSSStyleValuePair::ValueKind::BorderNone:
            return String::fromUTF8("none");
        case CSSStyleValuePair::ValueKind::BorderSolid:
            return String::fromUTF8("solid");
        default:
            return String::emptyString;
        }
    }
    case BorderTopWidth:
    case BorderBottomWidth:
    case BorderRightWidth:
    case BorderLeftWidth: {
        switch (valueKind()) {
        case CSSStyleValuePair::ValueKind::BorderThin:
            return String::fromUTF8("thin");
        case CSSStyleValuePair::ValueKind::BorderMedium:
            return String::fromUTF8("medium");
        case CSSStyleValuePair::ValueKind::BorderThick:
            return String::fromUTF8("thick");
        default:
            return lengthOrPercentageOrKeywordToString();
        }
    }
    case TextAlign: {
        switch (textAlignValue()) {
        case TextAlignValue::NamelessTextAlignValue:
            return String::fromUTF8("left");
        case TextAlignValue::LeftTextAlignValue:
            return String::fromUTF8("left");
        case TextAlignValue::RightTextAlignValue:
            return String::fromUTF8("right");
        case TextAlignValue::CenterTextAlignValue:
            return String::fromUTF8("center");
        case TextAlignValue::JustifyTextAlignValue:
            return String::fromUTF8("justify");
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

                if (vals->getValueKindAtIndex(i) == CSSStyleValuePair::ValueKind::TransformOriginLeft)
                    str = str->concat(String::fromUTF8("left"));
                else if (vals->getValueKindAtIndex(i) == CSSStyleValuePair::ValueKind::TransformOriginRight)
                    str = str->concat(String::fromUTF8("right"));
                else if (vals->getValueKindAtIndex(i) == CSSStyleValuePair::ValueKind::TransformOriginTop)
                    str = str->concat(String::fromUTF8("top"));
                else if (vals->getValueKindAtIndex(i) == CSSStyleValuePair::ValueKind::TransformOriginBottom)
                    str = str->concat(String::fromUTF8("bottom"));
                else if (vals->getValueKindAtIndex(i) == CSSStyleValuePair::ValueKind::TransformOriginCenter)
                    str = str->concat(String::fromUTF8("center"));
                else
                    str = str->concat(valueToString(vals->getValueKindAtIndex(i), vals->getValueAtIndex(i)));

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
                    String* newstr = valueToString(values->getValueKindAtIndex(j),
                        values->getValueAtIndex(j));
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
    default: {
        STARFISH_LOG_INFO("[CSSStyleValuePair::toString] Unsupported spec or something wrong");
        return String::emptyString;
    }
    }
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

void CSSStyleValuePair::setLengthValue(const char* value)
{
    if (VALUE_IS_STRING("auto")) {
        setValueKind(CSSStyleValuePair::ValueKind::Auto);
    } else if (VALUE_IS_INHERIT()) {
        setValueKind(CSSStyleValuePair::ValueKind::Inherit);
    } else if (VALUE_IS_INITIAL()) {
        setValueKind(CSSStyleValuePair::ValueKind::Initial);
    } else {
        parsePercentageOrLength(*this, value);
    }
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
    std::vector<String*, gc_allocator<String*> > tokens;
    tokenizeCSSValue(&tokens, value);

    if (checkInputErrorBackgroundRepeat(&tokens)) {
        if (STRING_VALUE_IS_STRING("repeat")) {
            setBackgroundRepeatX(String::createASCIIString("repeat"));
            setBackgroundRepeatY(String::createASCIIString("repeat"));
        } else if (STRING_VALUE_IS_STRING("repeat-x")) {
            setBackgroundRepeatX(String::createASCIIString("repeat"));
            setBackgroundRepeatY(String::createASCIIString("no-repeat"));
        } else if (STRING_VALUE_IS_STRING("repeat-y")) {
            setBackgroundRepeatX(String::createASCIIString("no-repeat"));
            setBackgroundRepeatY(String::createASCIIString("repeat"));
        } else if (STRING_VALUE_IS_STRING("no-repeat")) {
            setBackgroundRepeatX(String::createASCIIString("no-repeat"));
            setBackgroundRepeatY(String::createASCIIString("no-repeat"));
        } else {
            setBackgroundRepeatX(value);
            setBackgroundRepeatY(value);
        }
    }
}

String* CSSStyleDeclaration::Background()
{
    String* result = String::emptyString;
    String* image = BackgroundImage();
    String* repeat = BackgroundRepeat();
    String* color = BackgroundColor();

    if (image->length() != 0 && !image->equals("initial"))
        result = result->concat(image);
    if (repeat->length() != 0 && !repeat->equals("initial") && !repeat->equals("inherit")) {
        if (result->length())
            result = result->concat(String::spaceString);
        result = result->concat(repeat);
    }
    if (color->length() != 0 && !color->equals("initial")) {
        if (result->length())
            result = result->concat(String::spaceString);
        result = result->concat(color);
    }
    return result;
}

void CSSStyleDeclaration::setBackground(String* value)
{
    //  [<'background-color'> || <'background-image'> || <'background-repeat'>] | inherit
    if (value->length() == 0) {
        setBackgroundColor(String::emptyString);
        setBackgroundImage(String::emptyString);
        setBackgroundRepeat(String::emptyString);
        return;
    }
    std::vector<String*, gc_allocator<String*> > tokens;
    tokenizeCSSValue(&tokens, value);

    if (!checkEssentialValue(&tokens) && !checkInputErrorBackground(&tokens))
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
            tokens->push_back(String::fromUTF8(str.data(), str.length() - 1)->toLower());
            isWhiteSpaceState = true;
            str.clear();
        } else if ((inParenthesis && data[i] == ')') || i == length - 1) {
            if (str.length() > 3 && (str[0] == 'u' || str[0] == 'U') && (str[1] == 'r' || str[1] == 'R') && (str[2] == 'l' || str[2] == 'L'))
                tokens->push_back(String::fromUTF8("url")->concat(String::fromUTF8(str.data() + 3, str.length() - 3)));
            else
                tokens->push_back(String::fromUTF8(str.data(), str.length())->toLower());
            inParenthesis = false;
            isWhiteSpaceState = true;
            str.clear();
        }
    }
}

bool CSSStyleDeclaration::checkEssentialValue(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // NOTE: check if string is the value which all property can have. (e.g. initial, inherit, "")
    if (tokens->size() != 1)
        return false;
    const char* value = tokens->at(0)->utf8Data();
    return CSSPropertyParser::assureEssential(value);
}

bool CSSStyleDeclaration::checkInputErrorColor(std::vector<String*, gc_allocator<String*> >* tokens)
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

bool CSSStyleDeclaration::checkInputErrorBackgroundColor(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkInputErrorColor(tokens);
}

bool CSSStyleDeclaration::checkInputErrorBorderTopColor(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkInputErrorColor(tokens);
}

bool CSSStyleDeclaration::checkInputErrorBorderRightColor(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkInputErrorColor(tokens);
}

bool CSSStyleDeclaration::checkInputErrorBorderBottomColor(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkInputErrorColor(tokens);
}

bool CSSStyleDeclaration::checkInputErrorBorderLeftColor(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkInputErrorColor(tokens);
}

bool CSSStyleDeclaration::checkInputErrorBorderColor(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // <color>{1,4}
    for (unsigned i = 0; i < tokens->size(); i++) {
        const char* token = tokens->at(i)->utf8Data();
        if (!CSSPropertyParser::assureColor(token))
            return false;
    }
    return true;
}

// <length> | <percentage>
bool CSSStyleDeclaration::checkInputErrorPaddingTop(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() != 1)
        return false;
    (*tokens)[0] = (*tokens)[0];
    const char* value = (*tokens)[0]->utf8Data();
    return (CSSPropertyParser::assureLengthOrPercent(value, false));
}

bool CSSStyleDeclaration::checkInputErrorPaddingRight(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkInputErrorPaddingTop(tokens);
}

bool CSSStyleDeclaration::checkInputErrorPaddingBottom(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkInputErrorPaddingTop(tokens);
}

bool CSSStyleDeclaration::checkInputErrorPaddingLeft(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkInputErrorPaddingTop(tokens);
}

bool CSSStyleDeclaration::checkInputErrorPadding(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // {length | percentage} {1,4}
    for (unsigned i = 0; i < tokens->size(); i++) {
        const char* token = tokens->at(i)->utf8Data();
        if (!CSSPropertyParser::assureLengthOrPercent(token, false))
            return false;
    }
    return true;
}

bool CSSStyleDeclaration::checkHavingOneTokenAndLengthOrPercentage(std::vector<String*, gc_allocator<String*> >* tokens, bool allowNegative)
{
    if (tokens->size() == 1) {
        const char* token = (*tokens)[0]->utf8Data();
        if (CSSPropertyParser::assureLengthOrPercentOrAuto(token, allowNegative))
            return true;
    }
    return false;
}

bool CSSStyleDeclaration::checkInputErrorTop(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkHavingOneTokenAndLengthOrPercentage(tokens, true);
}

bool CSSStyleDeclaration::checkInputErrorBottom(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkHavingOneTokenAndLengthOrPercentage(tokens, true);
}

bool CSSStyleDeclaration::checkInputErrorLeft(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkHavingOneTokenAndLengthOrPercentage(tokens, true);
}

bool CSSStyleDeclaration::checkInputErrorRight(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkHavingOneTokenAndLengthOrPercentage(tokens, true);
}

bool CSSStyleDeclaration::checkInputErrorWidth(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkHavingOneTokenAndLengthOrPercentage(tokens, false);
}

bool CSSStyleDeclaration::checkInputErrorHeight(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkHavingOneTokenAndLengthOrPercentage(tokens, false);
}

bool CSSStyleDeclaration::checkInputErrorMarginTop(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkHavingOneTokenAndLengthOrPercentage(tokens, true);
}

bool CSSStyleDeclaration::checkInputErrorMarginRight(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkHavingOneTokenAndLengthOrPercentage(tokens, true);
}

bool CSSStyleDeclaration::checkInputErrorMarginBottom(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkHavingOneTokenAndLengthOrPercentage(tokens, true);
}

bool CSSStyleDeclaration::checkInputErrorMarginLeft(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkHavingOneTokenAndLengthOrPercentage(tokens, true);
}

bool CSSStyleDeclaration::checkInputErrorMargin(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // {length | percentage | <auto>} {1,4}
    for (unsigned i = 0; i < tokens->size(); i++) {
        const char* token = tokens->at(i)->utf8Data();
        if (!CSSPropertyParser::assureLengthOrPercentOrAuto(token, true))
            return false;
    }
    return true;
}

bool CSSStyleDeclaration::checkInputErrorBorderImageSource(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // none | <image>(=<uri>)
    if (tokens->size() == 1) {
        const char* value = (*tokens)[0]->utf8Data();
        if (VALUE_IS_NONE())
            return true;
    }
    // url
    return CSSPropertyParser::assureUrl(tokens, 0, tokens->size());
}

bool CSSStyleDeclaration::checkInputErrorBackgroundSize(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // [length | percentage | auto]{1, 2} | cover | contain // initial value -> auto
    if (tokens->size() == 1) {
        const char* value = (*tokens)[0]->utf8Data();
        if (VALUE_IS_STRING("cover") || VALUE_IS_STRING("contain") || CSSPropertyParser::assureLengthOrPercent(value, false)) {
            return true;
        }
    } else if (tokens->size() == 2) {
        for (unsigned int i = 0; i < tokens->size(); i++) {
            const char* token = (*tokens)[i]->utf8Data();
            if (!(CSSPropertyParser::assureLengthOrPercent(token, false))) {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool CSSStyleDeclaration::checkInputErrorLineHeight(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() != 1)
        return false;
    (*tokens)[0] = (*tokens)[0];
    const char* value = (*tokens)[0]->utf8Data();
    return (VALUE_IS_STRING("normal")
        || CSSPropertyParser::assureLengthOrPercent(value, false)
        || CSSPropertyParser::assureNumber(value, false));
}

bool CSSStyleDeclaration::checkInputErrorBackgroundImage(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // none | <image>(=<uri>)
    if (tokens->size() == 1) {
        const char* value = (*tokens)[0]->utf8Data();
        if (VALUE_IS_NONE())
            return true;
        return CSSPropertyParser::assureUrl(tokens, 0, tokens->size());
    }
    return false;
}

bool CSSStyleDeclaration::checkInputErrorBackgroundRepeatX(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // TODO
    return true;
}
bool CSSStyleDeclaration::checkInputErrorBackgroundRepeatY(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // TODO
    return true;
}
bool CSSStyleDeclaration::checkInputErrorBackgroundRepeat(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() != 1)
        return false;
    const char* value = (*tokens)[0]->utf8Data();
    return VALUE_IS_STRING("repeat")
        || VALUE_IS_STRING("repeat-x")
        || VALUE_IS_STRING("repeat-y")
        || VALUE_IS_STRING("no-repeat");
}

bool CSSStyleDeclaration::checkInputErrorBackground(std::vector<String*, gc_allocator<String*> >* tokens)
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

bool CSSStyleDeclaration::checkInputErrorDirection(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // <ltr> | rtl | inherit
    if (tokens->size() == 1) {
        const char* value = (*tokens)[0]->utf8Data();
        if (VALUE_IS_STRING("ltr") || VALUE_IS_STRING("rtl")) {
            return true;
        }
    }
    return false;
}

bool CSSStyleDeclaration::checkInputErrorFontSize(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() == 1) {
        const char* token = (*tokens)[0]->utf8Data();
        if (CSSPropertyParser::assureLengthOrPercent(token, false))
            return true;

        if (TOKEN_IS_STRING("xx-small") || (TOKEN_IS_STRING("x-small")) || (TOKEN_IS_STRING("small")) || (TOKEN_IS_STRING("medium")) || (TOKEN_IS_STRING("large")) || (TOKEN_IS_STRING("x-large")) || (TOKEN_IS_STRING("xx-large")) || (TOKEN_IS_STRING("larger")) || (TOKEN_IS_STRING("smaller"))) {
            return true;
        }
    }
    return false;
}

bool CSSStyleDeclaration::checkInputErrorFontStyle(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() == 1) {
        const char* token = (*tokens)[0]->utf8Data();
        if ((TOKEN_IS_STRING("normal")) || (TOKEN_IS_STRING("italic")) || (TOKEN_IS_STRING("oblique"))) {
            return true;
        }
    }
    return false;
}

bool CSSStyleDeclaration::checkInputErrorFontWeight(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() == 1) {
        const char* token = (*tokens)[0]->utf8Data();
        if ((TOKEN_IS_STRING("normal")) || (TOKEN_IS_STRING("bold")) || (TOKEN_IS_STRING("bolder")) || (TOKEN_IS_STRING("lighter")) || (TOKEN_IS_STRING("100")) || (TOKEN_IS_STRING("200")) || (TOKEN_IS_STRING("300")) || (TOKEN_IS_STRING("400")) || (TOKEN_IS_STRING("500")) || (TOKEN_IS_STRING("600")) || (TOKEN_IS_STRING("700")) || (TOKEN_IS_STRING("800")) || (TOKEN_IS_STRING("900"))) {
            return true;
        } else if (CSSPropertyParser::assureInteger(token, false)) {
            int num = 0;
            sscanf(token, "%d", &num);

            if (((num % 100)) && (((num / 100) >= 1) && ((num / 100) <= 9))) {
                char tmp[4] = {
                    0,
                };
                snprintf(tmp, 4, "%d", num);
                (*tokens)[0] = String::fromUTF8(tmp);
                return true;
            }
        }
    }
    return false;
}

bool CSSStyleDeclaration::checkInputErrorDisplay(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() == 1) {
        const char* token = (*tokens)[0]->utf8Data();
        if ((TOKEN_IS_STRING("inline")) || (TOKEN_IS_STRING("block")) || (TOKEN_IS_STRING("inline-block")) || (TOKEN_IS_STRING("none"))) {
            return true;
        }
    }
    return false;
}

bool CSSStyleDeclaration::checkInputErrorPosition(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() == 1) {
        const char* token = (*tokens)[0]->utf8Data();
        if ((TOKEN_IS_STRING("static")) || (TOKEN_IS_STRING("relative")) || (TOKEN_IS_STRING("absolute"))) {
            return true;
        }
    }
    return false;
}

bool CSSStyleDeclaration::checkInputErrorTextDecoration(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() == 1) {
        const char* token = (*tokens)[0]->utf8Data();
        if ((TOKEN_IS_STRING("none")) || (TOKEN_IS_STRING("underline")) || (TOKEN_IS_STRING("overline")) || (TOKEN_IS_STRING("line-through")) || (TOKEN_IS_STRING("blink"))) {
            return true;
        }
    }
    return false;
}

bool CSSStyleDeclaration::checkInputErrorBorderImageRepeat(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() == 1 || tokens->size() == 2) {
        for (unsigned int i = 0; i < tokens->size(); i++) {
            const char* token = (*tokens)[i]->utf8Data();
            if ((TOKEN_IS_STRING("stretch")) || (TOKEN_IS_STRING("repeat")) || (TOKEN_IS_STRING("round")) || (TOKEN_IS_STRING("space"))) {
                continue;
            } else {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool CSSStyleDeclaration::checkInputErrorBorderImageSlice(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // number && fill?
    if (tokens->size() != 1 && tokens->size() != 2)
        return false;

    bool isNum = false, isFill = false;
    for (unsigned int i = 0; i < tokens->size(); i++) {
        const char* token = tokens->at(i)->utf8Data();
        if (CSSPropertyParser::assureNumber(token, false)) {
            isNum = true;
        } else if (TOKEN_IS_STRING("fill")) {
            if (!isFill) {
                isFill = true;
            } else {
                return false;
            }
        } else {
            return false;
        }
    }

    if (isNum) {
        return true;
    } else {
        return false;
    }
}

bool CSSStyleDeclaration::checkInputErrorBorderImageWidth(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // [ <length> | <number> ]
    if (tokens->size() != 1)
        return false;

    const char* currentToken = tokens->at(0)->utf8Data();
    if (CSSPropertyParser::assureLength(currentToken, false) || CSSPropertyParser::assureNumber(currentToken, false))
        return true;

    return false;
}

bool CSSStyleDeclaration::checkInputErrorBorder(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (checkEssentialValue(tokens))
        return true;

    // [border-width || border-style || border-color] | inherit | initial
    bool hasWidth = false, hasStyle = false, hasColor = false;
    if (tokens->size() > 0 && tokens->size() <= 3) {
        for (unsigned i = 0; i < tokens->size(); i++) {
            switch (checkBorderValueType(tokens->at(i)->utf8Data())) {
            case BorderShorthandValueType::BWidth:
                if (hasWidth)
                    return false;
                hasWidth = true;
                break;
            case BorderShorthandValueType::BStyle:
                if (hasStyle)
                    return false;
                hasStyle = true;
                break;
            case BorderShorthandValueType::BColor:
                if (hasColor)
                    return false;
                hasColor = true;
                break;
            case BorderShorthandValueType::BInvalid:
                return false;
            }
        }
        return true;
    }
    return false;
}

bool CSSStyleDeclaration::checkInputErrorBorderTop(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkInputErrorBorder(tokens);
}

bool CSSStyleDeclaration::checkInputErrorBorderRight(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkInputErrorBorder(tokens);
}

bool CSSStyleDeclaration::checkInputErrorBorderBottom(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkInputErrorBorder(tokens);
}

bool CSSStyleDeclaration::checkInputErrorBorderLeft(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkInputErrorBorder(tokens);
}

bool checkInputErrorBorderUnitStyle(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // border-style(<none> | solid)
    if (tokens->size() == 1) {
        const char* token = tokens->at(0)->utf8Data();
        return CSSPropertyParser::assureBorderStyle(token);
    }
    return false;
}

bool CSSStyleDeclaration::checkInputErrorBorderStyle(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // {none | solid} {1,4}
    for (unsigned i = 0; i < tokens->size(); i++) {
        const char* token = tokens->at(i)->utf8Data();
        if (!CSSPropertyParser::assureBorderStyle(token))
            return false;
    }
    return true;
}

bool CSSStyleDeclaration::checkInputErrorBorderTopStyle(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkInputErrorBorderUnitStyle(tokens);
}

bool CSSStyleDeclaration::checkInputErrorBorderRightStyle(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkInputErrorBorderUnitStyle(tokens);
}

bool CSSStyleDeclaration::checkInputErrorBorderBottomStyle(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkInputErrorBorderUnitStyle(tokens);
}

bool CSSStyleDeclaration::checkInputErrorBorderLeftStyle(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkInputErrorBorderUnitStyle(tokens);
}

bool checkInputErrorBorderUnitWidth(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // border-width(thin | <medium> | thick) | length | inherit
    if (tokens->size() == 1) {
        const char* token = tokens->at(0)->utf8Data();
        if (CSSPropertyParser::assureBorderWidth(token))
            return true;
    }
    return false;
}

bool CSSStyleDeclaration::checkInputErrorBorderTopWidth(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkInputErrorBorderUnitWidth(tokens);
}

bool CSSStyleDeclaration::checkInputErrorBorderRightWidth(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkInputErrorBorderUnitWidth(tokens);
}

bool CSSStyleDeclaration::checkInputErrorBorderBottomWidth(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkInputErrorBorderUnitWidth(tokens);
}

bool CSSStyleDeclaration::checkInputErrorBorderLeftWidth(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkInputErrorBorderUnitWidth(tokens);
}

bool CSSStyleDeclaration::checkInputErrorBorderWidth(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // <border-width>{1,4}
    for (unsigned i = 0; i < tokens->size(); i++) {
        const char* token = tokens->at(i)->utf8Data();
        if (!CSSPropertyParser::assureBorderWidth(token))
            return false;
    }
    return true;
}

bool CSSStyleDeclaration::checkInputErrorTextAlign(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() == 1) {
        const char* token = (*tokens)[0]->utf8Data();
        if ((TOKEN_IS_STRING("left")) || (TOKEN_IS_STRING("right")) || (TOKEN_IS_STRING("center")) || (TOKEN_IS_STRING("justify"))) {
            return true;
        }
    }
    return false;
}

bool CSSStyleDeclaration::checkInputErrorVisibility(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() == 1) {
        const char* token = (*tokens)[0]->utf8Data();
        if ((TOKEN_IS_STRING("visible")) || (TOKEN_IS_STRING("hidden"))) {
            return true;
        }
    }
    return false;
}

bool CSSStyleDeclaration::checkInputErrorOpacity(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() != 1)
        return false;
    return CSSPropertyParser::assureNumber(tokens->at(0)->utf8Data(), true);
}

bool CSSStyleDeclaration::checkInputErrorOverflow(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() == 1) {
        const char* token = (*tokens)[0]->utf8Data();
        if ((TOKEN_IS_STRING("visible")) || (TOKEN_IS_STRING("hidden"))) {
            return true;
        }
    }
    return false;
}
bool CSSStyleDeclaration::checkInputErrorZIndex(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() == 1) {
        const char* token = tokens->at(0)->utf8Data();
        if ((TOKEN_IS_STRING("auto")) || CSSPropertyParser::assureInteger(token, false)) {
            return true;
        }
    }
    return false;
}

bool CSSStyleDeclaration::checkInputErrorVerticalAlign(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() == 1) {
        const char* token = (*tokens)[0]->utf8Data();
        if ((TOKEN_IS_STRING("baseline")) || (TOKEN_IS_STRING("sub")) || (TOKEN_IS_STRING("super")) || (TOKEN_IS_STRING("top")) || (TOKEN_IS_STRING("text-top")) || (TOKEN_IS_STRING("middle")) || (TOKEN_IS_STRING("bottom")) || (TOKEN_IS_STRING("text-bottom"))) {
            return true;
        } else if (CSSPropertyParser::assureLengthOrPercent(token, true)) {
            return true;
        }
    }
    return false;
}

bool CSSStyleDeclaration::checkInputErrorTransform(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* token = tokens->at(0)->utf8Data();
    if (tokens->size() == 1) {
        if (TOKEN_IS_STRING("none"))
            return true;
    }

    String* str = String::emptyString;
    for (unsigned i = 0; i< tokens->size(); i++)
        str = str->concat(tokens->at(i));
    tokens->clear();
    str->split(')', *tokens);
    for (unsigned i = 0; i < tokens->size(); i++) {
        token = tokens->at(i)->utf8Data();
        if (startsWith(token, "matrix(")) {
            // NOTE: strlen("matrix(") => 7
            if (!CSSPropertyParser::assureNumberList(token + 7, true, 6, 6))
                return false;
        } else if (startsWith(token, "translate(")) {
            if (!CSSPropertyParser::assureLengthOrPercentList(token + 10, true, 1, 2))
                return false;
        } else if (startsWith(token, "translatex(")) {
            if (!CSSPropertyParser::assureLengthOrPercentList(token + 11, true, 1, 1))
                return false;
        } else if (startsWith(token, "translatey(")) {
            if (!CSSPropertyParser::assureLengthOrPercentList(token + 11, true, 1, 1))
                return false;
        } else if (startsWith(token, "translatez(")) {
            if (!CSSPropertyParser::assureLengthOrPercentList(token + 11, true, 1, 1))
                return false;
        } else if (startsWith(token, "scale(")) {
            if (!CSSPropertyParser::assureNumberList(token + 6, true, 1, 2))
                return false;
        } else if (startsWith(token, "scalex(")) {
            if (!CSSPropertyParser::assureNumberList(token + 7, true, 1, 1))
                return false;
        } else if (startsWith(token, "scaley(")) {
            if (!CSSPropertyParser::assureNumberList(token + 7, true, 1, 1))
                return false;
        } else if (startsWith(token, "rotate(")) {
            if (!CSSPropertyParser::assureAngleList(token + 7, 1, 1))
                return false;
        } else if (startsWith(token, "skew(")) {
            if (!CSSPropertyParser::assureAngleList(token + 5, 1, 2))
                return false;
        } else if (startsWith(token, "skewx(")) {
            if (!CSSPropertyParser::assureAngleList(token + 6, 1, 1))
                return false;
        } else if (startsWith(token, "skewy(")) {
            if (!CSSPropertyParser::assureAngleList(token + 6, 1, 1))
                return false;
        } else {
            return false;
        }
    }
    return true;
}

bool CSSStyleDeclaration::checkInputErrorTransformOrigin(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() == 1 || tokens->size() == 2) {
        const char* value;
        for (unsigned int i = 0; i < tokens->size(); i++) {
            value = (*tokens)[i]->utf8Data();
            if (VALUE_IS_STRING("left") || VALUE_IS_STRING("right") || VALUE_IS_STRING("top") || VALUE_IS_STRING("bottom") || VALUE_IS_STRING("center")) {
                return true;
            } else if (CSSPropertyParser::assureLength(value, true) || CSSPropertyParser::assurePercent(value, true)) {
                return true;
            } else {
                return false;
            }
        }
    }
    return false;
}

bool CSSStyleDeclaration::checkInputErrorUnicodeBidi(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() == 1) {
        const char* token = (*tokens)[0]->utf8Data();
        if ((TOKEN_IS_STRING("normal")) || (TOKEN_IS_STRING("embed"))) {
            return true;
        }
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
    ret->m_inheritedStyles.m_textAlign = TextAlignValue::NamelessTextAlignValue;
    ret->m_inheritedStyles.m_direction = DirectionValue::LtrDirectionValue;
    ret->loadResources(doc);
    return ret;
}

unsigned char parseColorFunctionPart(String* s)
{
    if (s->indexOf('%') == s->length() - 1) {
        float f;
        sscanf(s->utf8Data(), "%f%%", &f);
        if (f < 0)
            f = 0;
        if (f > 100)
            f = 100;
        return 255 * f / 100;
    }
    float f;
    sscanf(s->utf8Data(), "%f", &f);

    if (f < 0)
        f = 0;
    if (f > 255)
        f = 255;
    return f;
}

Color parseColor(String* str)
{
    const char* s = str->utf8Data();
    if (startsWith(s, "rgba")) {
        size_t s1 = str->indexOf('(');
        size_t s2 = str->indexOf(')');

        STARFISH_ASSERT(s1 != SIZE_MAX && s2 != SIZE_MAX && s1 < s2);
        String* sub = str->substring(s1 + 1, s2 - s1 - 1);
        String::Vector v;
        sub->split(',', v);
        STARFISH_ASSERT(v.size() == 4);

        unsigned char r = parseColorFunctionPart(v[0]);
        unsigned char g = parseColorFunctionPart(v[1]);
        unsigned char b = parseColorFunctionPart(v[2]);

        unsigned char a = 255;
        String* s = v[3];
        if (s->indexOf('%') == s->length() - 1) {
            float f;
            sscanf(s->utf8Data(), "%f%%", &f);
            if (f < 0)
                f = 0;
            if (f > 100)
                f = 100;
            a = 255 * f / 100;
        } else {
            float f;
            sscanf(s->utf8Data(), "%f", &f);

            if (f < 0)
                f = 0;
            if (f > 1)
                f = 1;
            a = f * 255;
        }
        return Color(r, g, b, a);
    } else if (startsWith(s, "rgb")) {

        size_t s1 = str->indexOf('(');
        size_t s2 = str->indexOf(')');

        STARFISH_ASSERT(s1 != SIZE_MAX && s2 != SIZE_MAX && s1 < s2);
        String* sub = str->substring(s1 + 1, s2 - s1 - 1);
        String::Vector v;
        sub->split(',', v);
        STARFISH_ASSERT(v.size() == 3);

        unsigned char r = parseColorFunctionPart(v[0]);
        unsigned char g = parseColorFunctionPart(v[1]);
        unsigned char b = parseColorFunctionPart(v[2]);
        return Color(r, g, b, 255);
    } else if (startsWith(s, "#") && (str->length() == 9)) {
        unsigned int r, g, b, a;
        sscanf(s, "#%02x%02x%02x%02x", &r, &g, &b, &a);
        return Color(r, g, b, a);
    } else if (startsWith(s, "#") && (str->length() == 7)) {
        unsigned int r, g, b;
        sscanf(s, "#%02x%02x%02x", &r, &g, &b);
        return Color(r, g, b, 255);
    } else if (startsWith(s, "#") && (str->length() == 4)) {
        unsigned int r, g, b;
        sscanf(s, "#%01x%01x%01x", &r, &g, &b);
        return Color(r * 17, g * 17, b * 17, 255);
    } else {
        if (strcmp("transparent", s) == 0) {
            return Color(0, 0, 0, 0);
        } else if (strcmp("currentcolor", s) == 0) {
            // Others think it as a black color
            return Color(0, 0, 0, 255);
        }
#define PARSE_COLOR(name, value)           \
    else if (strcmp(#name, s) == 0)        \
    {                                      \
        char r = (value & 0xff0000) >> 16; \
        char g = (value & 0xff00) >> 8;    \
        char b = (value & 0xff);           \
        char a = 255;                      \
        return Color(r, g, b, a);          \
    }

        NAMED_COLOR_FOR_EACH(PARSE_COLOR)

#undef PARSE_COLOR
        else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }
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
                    style->m_inheritedStyles.m_color = parentStyle->m_inheritedStyles.m_color;
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->m_inheritedStyles.m_color = parseColor(String::fromUTF8("black"));
                } else {
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::StringValueKind);
                    if (cssValues[k].stringValue()->equalsWithoutCase(String::fromUTF8("currentColor"))) {
                        style->m_inheritedStyles.m_color = parentStyle->m_inheritedStyles.m_color;
                    } else {
                        style->m_inheritedStyles.m_color = parseColor(cssValues[k].stringValue());
                    }
                }
                break;
            case CSSStyleValuePair::KeyKind::FontSize:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->m_inheritedStyles.m_fontSize = parentStyle->m_inheritedStyles.m_fontSize;
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->m_inheritedStyles.m_fontSize = parseAbsoluteFontSize(3);
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::XXSmallFontSizeValueKind) {
                    style->m_inheritedStyles.m_fontSize = parseAbsoluteFontSize(0);
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::XSmallFontSizeValueKind) {
                    style->m_inheritedStyles.m_fontSize = parseAbsoluteFontSize(1);
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::SmallFontSizeValueKind) {
                    style->m_inheritedStyles.m_fontSize = parseAbsoluteFontSize(2);
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::MediumFontSizeValueKind) {
                    style->m_inheritedStyles.m_fontSize = parseAbsoluteFontSize(3);
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::LargeFontSizeValueKind) {
                    style->m_inheritedStyles.m_fontSize = parseAbsoluteFontSize(4);
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::XLargeFontSizeValueKind) {
                    style->m_inheritedStyles.m_fontSize = parseAbsoluteFontSize(5);
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::XXLargeFontSizeValueKind) {
                    style->m_inheritedStyles.m_fontSize = parseAbsoluteFontSize(6);
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::LargerFontSizeValueKind) {
                    style->m_inheritedStyles.m_fontSize = Length(Length::Fixed, parentStyle->m_inheritedStyles.m_fontSize.fixed() * 1.2f);
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::SmallerFontSizeValueKind) {
                    style->m_inheritedStyles.m_fontSize = Length(Length::Fixed, parentStyle->m_inheritedStyles.m_fontSize.fixed() / 1.2f);
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
                // TODO: LEESS
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
            case CSSStyleValuePair::KeyKind::TextOverflow:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setTextOverflow(parentStyle->textOverflow());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->setTextOverflow(TextOverflowValue::ClipTextOverflowValue);
                } else {
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::TextOverflowValueKind);
                    style->setTextOverflow(cssValues[k].textOverflowValue());
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
                    // Initial: a nameless value that acts as 'left' if 'direction' is 'ltr', 'right' if 'direction' is 'rtl'
                    style->setTextAlign(ComputedStyle::initialTextAlign());
                } else {
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::TextAlignValueKind);
                    style->setTextAlign(cssValues[k].textAlignValue());
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
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::TextDecorationKind);
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
                } else {
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::StringValueKind);
                    style->setBackgroundColor(parseColor(cssValues[k].stringValue()));
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
            case CSSStyleValuePair::KeyKind::BackgroundSize:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setBackgroundSizeType(parentStyle->bgSizeType());
                    style->setBackgroundSizeValue(parentStyle->bgSizeValue());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    // initial value : auto
                    LengthSize* result = new LengthSize();
                    style->setBackgroundSizeValue(result);
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Cover) {
                    style->setBackgroundSizeType(BackgroundSizeType::Cover);
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Contain) {
                    style->setBackgroundSizeType(BackgroundSizeType::Contain);
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueListKind) {
                    ValueList* list = cssValues[k].multiValue();
                    LengthSize* result = new LengthSize();
                    if (list->size() >= 1) {
                        result->m_width = convertValueToLength(list->getValueKindAtIndex(0), list->getValueAtIndex(0));
                    }
                    if (list->size() >= 2) {
                        result->m_height = convertValueToLength(list->getValueKindAtIndex(1), list->getValueAtIndex(1));
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
                    style->setBackgroundRepeatX(cssValues[k].backgroundRepeatXValue());
                }
                break;
            case CSSStyleValuePair::KeyKind::BackgroundRepeatY:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setBackgroundRepeatY(parentStyle->backgroundRepeatY());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->setBackgroundRepeatY(BackgroundRepeatValue::RepeatRepeatValue);
                } else {
                    STARFISH_ASSERT(CSSStyleValuePair::ValueKind::BackgroundRepeatValueKind == cssValues[k].valueKind());
                    style->setBackgroundRepeatY(cssValues[k].backgroundRepeatYValue());
                }
                break;
            case CSSStyleValuePair::KeyKind::Top:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setTop(parentStyle->top());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial
                    || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Auto) {
                    style->setTop(Length());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Length) {
                    style->setTop(cssValues[k].lengthValue().toLength());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Percentage) {
                    style->setTop(Length(Length::Percent, cssValues[k].percentageValue()));
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
                break;
            case CSSStyleValuePair::KeyKind::Right:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setRight(parentStyle->right());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial
                    || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Auto) {
                    style->setRight(Length());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Length) {
                    style->setRight(cssValues[k].lengthValue().toLength());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Percentage) {
                    style->setRight(Length(Length::Percent, cssValues[k].percentageValue()));
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
                break;
            case CSSStyleValuePair::KeyKind::Bottom:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setBottom(parentStyle->bottom());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial
                    || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Auto) {
                    style->setBottom(Length());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Length) {
                    style->setBottom(cssValues[k].lengthValue().toLength());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Percentage) {
                    style->setBottom(Length(Length::Percent, cssValues[k].percentageValue()));
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
                break;
            case CSSStyleValuePair::KeyKind::Left:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setLeft(parentStyle->left());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial
                    || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Auto) {
                    style->setLeft(Length());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Length) {
                    style->setLeft(cssValues[k].lengthValue().toLength());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Percentage) {
                    style->setLeft(Length(Length::Percent, cssValues[k].percentageValue()));
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
                break;
            case CSSStyleValuePair::KeyKind::BorderTopColor:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setBorderTopColor(parentStyle->borderTopColor());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    // This is done after resolve style (arrangeStyleValues function in ComputedStyle Class)
                    style->clearBorderTopColor();
                } else {
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::StringValueKind);
                    if (cssValues[k].stringValue()->equalsWithoutCase(String::fromUTF8("currentColor"))) {
                        // This is done after resolve style (arrangeStyleValues function in ComputedStyle Class)
                        style->clearBorderTopColor();
                    } else {
                        style->setBorderTopColor(parseColor(cssValues[k].stringValue()));
                    }
                }
                break;
            case CSSStyleValuePair::KeyKind::BorderRightColor:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setBorderRightColor(parentStyle->borderRightColor());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    // This is done after resolve style (arrangeStyleValues function in ComputedStyle Class)
                    style->clearBorderRightColor();
                } else {
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::StringValueKind);
                    if (cssValues[k].stringValue()->equalsWithoutCase(String::fromUTF8("currentColor"))) {
                        // This is done after resolve style (arrangeStyleValues function in ComputedStyle Class)
                        style->clearBorderRightColor();
                    } else {
                        style->setBorderRightColor(parseColor(cssValues[k].stringValue()));
                    }
                }
                break;
            case CSSStyleValuePair::KeyKind::BorderBottomColor:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setBorderBottomColor(parentStyle->borderBottomColor());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    // This is done after resolve style (arrangeStyleValues function in ComputedStyle Class)
                    style->clearBorderBottomColor();
                } else {
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::StringValueKind);
                    if (cssValues[k].stringValue()->equalsWithoutCase(String::fromUTF8("currentColor"))) {
                        // This is done after resolve style (arrangeStyleValues function in ComputedStyle Class)
                        style->clearBorderBottomColor();
                    } else {
                        style->setBorderBottomColor(parseColor(cssValues[k].stringValue()));
                    }
                }
                break;
            case CSSStyleValuePair::KeyKind::BorderLeftColor:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setBorderLeftColor(parentStyle->borderLeftColor());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    // This is done after resolve style (arrangeStyleValues function in ComputedStyle Class)
                    style->clearBorderLeftColor();
                } else {
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::StringValueKind);
                    if (cssValues[k].stringValue()->equalsWithoutCase(String::fromUTF8("currentColor"))) {
                        // This is done after resolve style (arrangeStyleValues function in ComputedStyle Class)
                        style->clearBorderLeftColor();
                    } else {
                        style->setBorderLeftColor(parseColor(cssValues[k].stringValue()));
                    }
                }
                break;
            case CSSStyleValuePair::KeyKind::BorderImageRepeat:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->setBorderImageRepeatX(StretchValue);
                    style->setBorderImageRepeatY(StretchValue);
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    // TODO: Prevent parentStyle->surround() from creating object for this
                    style->setBorderImageRepeatFromOther(parentStyle);
                } else {
                    style->setBorderImageRepeatX(cssValues[k].multiValue()->getValueAtIndex(0).m_borderImageRepeat);
                    style->setBorderImageRepeatY(cssValues[k].multiValue()->getValueAtIndex(1).m_borderImageRepeat);
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
                    if (l->getValueKindAtIndex(size - 1) == CSSStyleValuePair::ValueKind::StringValueKind) {
                        style->setBorderImageSliceFill(true);
                        size--;
                    }
                    top = convertPercentOrNumberToLength(l->getValueKindAtIndex(0), l->getValueAtIndex(0));
                    if (size > 1) {
                        right = convertPercentOrNumberToLength(l->getValueKindAtIndex(1), l->getValueAtIndex(1));
                    } else {
                        right = top;
                    }
                    if (size > 2) {
                        bottom = convertPercentOrNumberToLength(l->getValueKindAtIndex(2), l->getValueAtIndex(2));
                    } else {
                        bottom = top;
                    }
                    if (size > 3) {
                        left = convertPercentOrNumberToLength(l->getValueKindAtIndex(3), l->getValueAtIndex(3));
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
                } else {
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::ValueListKind);
                    BorderImageLength top, right, bottom, left;
                    ValueList* l = cssValues[k].multiValue();
                    unsigned int size = l->size();
                    if (l->getValueKindAtIndex(0) == CSSStyleValuePair::ValueKind::Number)
                        top.setValue(l->getValueAtIndex(0).m_floatValue);
                    else
                        top.setValue(convertValueToLength(l->getValueKindAtIndex(0), l->getValueAtIndex(0)));
                    if (size > 1) {
                        if (l->getValueKindAtIndex(1) == CSSStyleValuePair::ValueKind::Number)
                            right.setValue(l->getValueAtIndex(1).m_floatValue);
                        else
                            right.setValue(convertValueToLength(l->getValueKindAtIndex(1), l->getValueAtIndex(1)));
                    } else {
                        right = top;
                    }
                    if (size > 2) {
                        if (l->getValueKindAtIndex(2) == CSSStyleValuePair::ValueKind::Number)
                            bottom.setValue(l->getValueAtIndex(2).m_floatValue);
                        else
                            bottom.setValue(convertValueToLength(l->getValueKindAtIndex(2), l->getValueAtIndex(2)));
                    } else {
                        bottom = top;
                    }
                    if (size > 3) {
                        if (l->getValueKindAtIndex(3) == CSSStyleValuePair::ValueKind::Number)
                            left.setValue(l->getValueAtIndex(3).m_floatValue);
                        else
                            left.setValue(convertValueToLength(l->getValueKindAtIndex(3), l->getValueAtIndex(3)));
                    } else {
                        left = right;
                    }
                    style->setBorderImageWidths(BorderImageLengthBox(top, right, bottom, left));
                }
                break;
            case CSSStyleValuePair::KeyKind::BorderTopStyle:
                // border-style(<none> | solid) | inherit
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setBorderTopStyle(parentStyle->borderTopStyle());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial
                    || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::BorderNone) {
                    style->setBorderTopStyle(BorderStyleValue::BNone);
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::BorderSolid) {
                    style->setBorderTopStyle(BorderStyleValue::BSolid);
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
                break;
            case CSSStyleValuePair::KeyKind::BorderRightStyle:
                // border-style(<none> | solid) | inherit
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setBorderRightStyle(parentStyle->borderRightStyle());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial
                    || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::BorderNone) {
                    style->setBorderRightStyle(BorderStyleValue::BNone);
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::BorderSolid) {
                    style->setBorderRightStyle(BorderStyleValue::BSolid);
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
                break;
            case CSSStyleValuePair::KeyKind::BorderBottomStyle:
                // border-style(<none> | solid) | inherit
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setBorderBottomStyle(parentStyle->borderBottomStyle());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial
                    || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::BorderNone) {
                    style->setBorderBottomStyle(BorderStyleValue::BNone);
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::BorderSolid) {
                    style->setBorderBottomStyle(BorderStyleValue::BSolid);
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
                break;
            case CSSStyleValuePair::KeyKind::BorderLeftStyle:
                // border-style(<none> | solid) | inherit
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setBorderLeftStyle(parentStyle->borderLeftStyle());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial
                    || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::BorderNone) {
                    style->setBorderLeftStyle(BorderStyleValue::BNone);
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::BorderSolid) {
                    style->setBorderLeftStyle(BorderStyleValue::BSolid);
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
                break;
            case CSSStyleValuePair::KeyKind::BorderTopWidth:
                // border-width(thin | <medium> | thick | length) | inherit
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setBorderTopWidth(parentStyle->borderTopWidth());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Length) {
                    style->setBorderTopWidth(cssValues[k].lengthValue().toLength());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::BorderThin) {
                    style->setBorderTopWidth(Length(Length::Fixed, 1));
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial
                    || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::BorderMedium) {
                    style->setBorderTopWidth(Length(Length::Fixed, 3));
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::BorderThick) {
                    style->setBorderTopWidth(Length(Length::Fixed, 5));
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
                break;
            case CSSStyleValuePair::KeyKind::BorderRightWidth:
                // border-width(thin | <medium> | thick | length) | inherit
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setBorderRightWidth(parentStyle->borderRightWidth());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Length) {
                    style->setBorderRightWidth(cssValues[k].lengthValue().toLength());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::BorderThin) {
                    style->setBorderRightWidth(Length(Length::Fixed, 1));
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial
                    || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::BorderMedium) {
                    style->setBorderRightWidth(Length(Length::Fixed, 3));
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::BorderThick) {
                    style->setBorderRightWidth(Length(Length::Fixed, 5));
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
                break;
            case CSSStyleValuePair::KeyKind::BorderBottomWidth:
                // border-width(thin | <medium> | thick | length) | inherit
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setBorderBottomWidth(parentStyle->borderBottomWidth());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Length) {
                    style->setBorderBottomWidth(cssValues[k].lengthValue().toLength());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::BorderThin) {
                    style->setBorderBottomWidth(Length(Length::Fixed, 1));
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial
                    || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::BorderMedium) {
                    style->setBorderBottomWidth(Length(Length::Fixed, 3));
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::BorderThick) {
                    style->setBorderBottomWidth(Length(Length::Fixed, 5));
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
                break;
            case CSSStyleValuePair::KeyKind::BorderLeftWidth:
                // border-width(thin | <medium> | thick | length) | inherit
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setBorderLeftWidth(parentStyle->borderLeftWidth());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Length) {
                    style->setBorderLeftWidth(cssValues[k].lengthValue().toLength());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::BorderThin) {
                    style->setBorderLeftWidth(Length(Length::Fixed, 1));
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial
                    || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::BorderMedium) {
                    style->setBorderLeftWidth(Length(Length::Fixed, 3));
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::BorderThick) {
                    style->setBorderLeftWidth(Length(Length::Fixed, 5));
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
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
            case CSSStyleValuePair::KeyKind::MarginTop:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setMarginTop(parentStyle->marginTop());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->setMarginTop(Length(Length::Fixed, 0));
                } else {
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Auto
                        || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Length
                        || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Percentage);
                    style->setMarginTop(convertValueToLength(cssValues[k].valueKind(), cssValues[k].value()));
                }
                break;
            case CSSStyleValuePair::KeyKind::MarginBottom:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Auto) {
                    style->setMarginBottom(Length(Length::Auto));
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setMarginBottom(parentStyle->marginBottom());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->setMarginBottom(ComputedStyle::initialMargin());
                } else {
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Auto
                        || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Length
                        || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Percentage);
                    style->setMarginBottom(convertValueToLength(cssValues[k].valueKind(), cssValues[k].value()));
                }
                break;
            case CSSStyleValuePair::KeyKind::MarginLeft:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setMarginLeft(parentStyle->marginLeft());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->setMarginLeft(Length(Length::Fixed, 0));
                } else {
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Auto
                        || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Length
                        || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Percentage);
                    style->setMarginLeft(convertValueToLength(cssValues[k].valueKind(), cssValues[k].value()));
                }
                break;
            case CSSStyleValuePair::KeyKind::MarginRight:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setMarginRight(parentStyle->marginRight());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->setMarginRight(Length(Length::Fixed, 0));
                } else {
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Auto
                        || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Length
                        || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Percentage);
                    style->setMarginRight(convertValueToLength(cssValues[k].valueKind(), cssValues[k].value()));
                }
                break;
            case CSSStyleValuePair::KeyKind::PaddingTop:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setPaddingTop(parentStyle->paddingTop());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->setPaddingTop(Length(Length::Fixed, 0));
                } else {
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Length
                        || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Percentage);
                    style->setPaddingTop(convertValueToLength(cssValues[k].valueKind(), cssValues[k].value()));
                }
                break;
            case CSSStyleValuePair::KeyKind::PaddingRight:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setPaddingRight(parentStyle->paddingRight());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->setPaddingRight(ComputedStyle::initialPadding());
                } else {
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Length
                        || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Percentage);
                    style->setPaddingRight(convertValueToLength(cssValues[k].valueKind(), cssValues[k].value()));
                }
                break;
            case CSSStyleValuePair::KeyKind::PaddingBottom:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setPaddingBottom(parentStyle->paddingBottom());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->setPaddingBottom(Length(Length::Fixed, 0));
                } else {
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Length
                        || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Percentage);
                    style->setPaddingBottom(convertValueToLength(cssValues[k].valueKind(), cssValues[k].value()));
                }
                break;
            case CSSStyleValuePair::KeyKind::PaddingLeft:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setPaddingLeft(parentStyle->paddingLeft());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->setPaddingLeft(Length(Length::Fixed, 0));
                } else {
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Length
                        || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Percentage);
                    style->setPaddingLeft(convertValueToLength(cssValues[k].valueKind(), cssValues[k].value()));
                }
                break;
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
                            if (f.values()->getValueKindAtIndex(i) == CSSStyleValuePair::ValueKind::Number) {
                                dValues[i] = f.values()->getValueAtIndex(i).m_floatValue;
                            } else if (f.values()->getValueKindAtIndex(i) == CSSStyleValuePair::ValueKind::Angle) {
                                dValues[i] = f.values()->getValueAtIndex(i).m_angle.toDegreeValue();
                            }
                        }

                        switch (f.kind()) {
                        case CSSTransformFunction::Kind::Matrix:
                            style->setTransformMatrix(dValues[0], dValues[1], dValues[2], dValues[3], dValues[4], dValues[5]);
                            break;
                        case CSSTransformFunction::Kind::Translate:
                            {
                            Length a, b(Length::Fixed, 0);
                            a = convertValueToLength(f.values()->getValueKindAtIndex(0), f.values()->getValueAtIndex(0));
                            if (valueSize > 1)
                                b = convertValueToLength(f.values()->getValueKindAtIndex(1), f.values()->getValueAtIndex(1));
                            style->setTransformTranslate(a, b);
                            break;
                            }
                        case CSSTransformFunction::Kind::TranslateX:
                            {
                            Length a = convertValueToLength(f.values()->getValueKindAtIndex(0), f.values()->getValueAtIndex(0));
                            style->setTransformTranslate(a, Length(Length::Fixed, 0));
                            }
                            break;
                        case CSSTransformFunction::Kind::TranslateY:
                            {
                            Length a = convertValueToLength(f.values()->getValueKindAtIndex(0), f.values()->getValueAtIndex(0));
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
                        if (list->getValueKindAtIndex(i) == CSSStyleValuePair::ValueKind::TransformOriginLeft) {
                            xAxis = Length(Length::Percent, 0.0f);
                        } else if (list->getValueKindAtIndex(i) == CSSStyleValuePair::ValueKind::TransformOriginRight) {
                            xAxis = Length(Length::Percent, 1.0f);
                        } else if (list->getValueKindAtIndex(i) == CSSStyleValuePair::ValueKind::TransformOriginCenter) {

                        } else if (list->getValueKindAtIndex(i) == CSSStyleValuePair::ValueKind::TransformOriginTop) {
                            yAxis = Length(Length::Percent, 0.0f);
                        } else if (list->getValueKindAtIndex(i) == CSSStyleValuePair::ValueKind::TransformOriginBottom) {
                            yAxis = Length(Length::Percent, 1.0f);
                        } else {
                            if (i == 0)
                                xAxis = convertValueToLength(list->getValueKindAtIndex(i), list->getValueAtIndex(i));
                            else
                                yAxis = convertValueToLength(list->getValueKindAtIndex(i), list->getValueAtIndex(i));
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
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::UnicodeBidiKind) {
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
