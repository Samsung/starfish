#include "StarFishConfig.h"

#include "Style.h"
#include "CSSParser.h"
#include "ComputedStyle.h"
#include "NamedColors.h"

#include "dom/Element.h"
#include "dom/Document.h"
#include "dom/DOMTokenList.h"

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

static String* mergeTokens(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() == 1)
        return (*tokens)[0];
    String* str = String::emptyString;
    for (unsigned i = 0; i < tokens->size(); i++) {
        str = str->concat(tokens->at(i));
    }
    return str;
}

#define VALUE_IS_STRING(str) \
    (strcmp(value, str)) == 0

#define VALUE_IS_INHERIT() \
    VALUE_IS_STRING("inherit")

#define VALUE_IS_INITIAL() \
    VALUE_IS_STRING("initial")

#define VALUE_IS_NONE() \
    VALUE_IS_STRING("none")

CSSLength parseCSSLength(const char* value)
{
    float f;
    if (endsWith(value, "px")) {
        sscanf(value, "%fpx", &f);
        return CSSLength(f);
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
    } else if (strcmp(value, "0")) {
        return CSSLength(0.0);
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}
void parseLength(CSSStyleValuePair& ret, const char* value)
{
    float f;
    if (endsWith(value, "px")) {
        sscanf(value, "%fpx", &f);
        ret.m_valueKind = CSSStyleValuePair::ValueKind::Length;
        ret.m_value.m_length = CSSLength(f);
    } else if (endsWith(value, "em")) {
        sscanf(value, "%fem", &f);
        ret.m_valueKind = CSSStyleValuePair::ValueKind::Length;
        ret.m_value.m_length = CSSLength(CSSLength::Kind::EM, f);
    } else if (endsWith(value, "ex")) {
        sscanf(value, "%fex", &f);
        ret.m_valueKind = CSSStyleValuePair::ValueKind::Length;
        ret.m_value.m_length = CSSLength(CSSLength::Kind::EX, f);
    } else if (endsWith(value, "in")) {
        sscanf(value, "%fin", &f);
        ret.m_valueKind = CSSStyleValuePair::ValueKind::Length;
        ret.m_value.m_length = CSSLength(CSSLength::Kind::IN, f);
    } else if (endsWith(value, "cm")) {
        sscanf(value, "%fcm", &f);
        ret.m_valueKind = CSSStyleValuePair::ValueKind::Length;
        ret.m_value.m_length = CSSLength(CSSLength::Kind::CM, f);
    } else if (endsWith(value, "mm")) {
        sscanf(value, "%fmm", &f);
        ret.m_valueKind = CSSStyleValuePair::ValueKind::Length;
        ret.m_value.m_length = CSSLength(CSSLength::Kind::MM, f);
    } else if (endsWith(value, "pt")) {
        sscanf(value, "%fpt", &f);
        ret.m_valueKind = CSSStyleValuePair::ValueKind::Length;
        ret.m_value.m_length = CSSLength(CSSLength::Kind::PT, f);
    } else if (endsWith(value, "pc")) {
        sscanf(value, "%fpc", &f);
        ret.m_valueKind = CSSStyleValuePair::ValueKind::Length;
        ret.m_value.m_length = CSSLength(CSSLength::Kind::PC, f);
    } else if (strcmp(value, "0") == 0) {
        ret.m_valueKind = CSSStyleValuePair::ValueKind::Length;
        ret.m_value.m_length = CSSLength(0.0);
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
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

    ASSERT_NOT_REACHED();
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

void CSSStyleValuePair::setValueColor(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else {
        m_valueKind = CSSStyleValuePair::ValueKind::StringValueKind;
        m_value.m_stringValue = mergeTokens(tokens);
    }
}

void CSSStyleValuePair::setValueDirection(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // <ltr> | rtl | inherit
    // TODO add initial
    m_keyKind = CSSStyleValuePair::KeyKind::Direction;
    m_valueKind = CSSStyleValuePair::ValueKind::DirectionValueKind;
    if (VALUE_IS_STRING("ltr")) {
        m_value.m_direction = DirectionValue::LtrDirectionValue;
    } else if (VALUE_IS_STRING("rtl")) {
        m_value.m_direction = DirectionValue::RtlDirectionValue;
    } else if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

void CSSStyleValuePair::setValueWidth(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // length | percentage | <auto> | inherit
    m_keyKind = CSSStyleValuePair::KeyKind::Width;
    m_valueKind = CSSStyleValuePair::ValueKind::Auto;

    if (VALUE_IS_STRING("auto")) {
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else {
        setValuePercentageOrLength(value);
    }
}

void CSSStyleValuePair::setValueHeight(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // length | percentage | <auto> | inherit
    m_keyKind = CSSStyleValuePair::KeyKind::Height;
    m_valueKind = CSSStyleValuePair::ValueKind::Auto;

    if (VALUE_IS_STRING("auto")) {
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else {
        setValuePercentageOrLength(value);
    }
}

void CSSStyleValuePair::setValueFontSize(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // TODO add initial
    // absolute-size | relative-size | length | percentage | inherit // initial value -> medium
    //        O      |       O       |   O    |    O       |    O
    m_keyKind = CSSStyleValuePair::KeyKind::FontSize;
    if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_STRING("xx-small")) {
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
    // TODO: LEESS
    const char* value = tokens->at(0)->utf8Data();

    // <normal> | bold | bolder | lighter | 100 | 200 | 300 | 400 | 500 | 600 | 700 | 800 | 900 | inherit // initial -> normal
    m_keyKind = CSSStyleValuePair::KeyKind::FontWeight;
    if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_STRING("normal")) {
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
    m_keyKind = CSSStyleValuePair::KeyKind::FontStyle;
    if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_STRING("normal")) {
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
    m_keyKind = CSSStyleValuePair::KeyKind::Display;
    m_valueKind = CSSStyleValuePair::ValueKind::DisplayValueKind;
    if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_STRING("block")) {
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
    // <static> | relative | absolute | fixed | inherit
    m_keyKind = CSSStyleValuePair::KeyKind::Position;
    m_valueKind = CSSStyleValuePair::ValueKind::PositionValueKind;

    if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_STRING("static")) {
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
    m_keyKind = CSSStyleValuePair::KeyKind::TextDecoration;
    m_valueKind = CSSStyleValuePair::ValueKind::TextDecorationKind;

    if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_NONE()) {
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

void CSSStyleValuePair::setValueBackgroundColor(std::vector<String*, gc_allocator<String*> >* tokens)
{
    setValueColor(tokens);
}

void CSSStyleValuePair::setValueBackgroundSize(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // [length | percentage | auto]{1, 2} | cover | contain // initial value -> auto
    m_keyKind = CSSStyleValuePair::KeyKind::BackgroundSize;

    String* token = (*tokens)[0];
    if (token->equals("cover")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Cover;
    } else if (token->equals("contain")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Contain;
    } else if (token->equals("initial")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (token->equals("inherit")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
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
                    values->append(kind, {.m_floatValue = parser->parsedFloatValue() });
                } else if (kind == CSSStyleValuePair::ValueKind::Length) {
                    CSSStyleValuePair::ValueData data = {.m_length = CSSLength(parser->parsedFloatValue()) };
                    values->append(kind, data);
                }
            }
        }
    }
}

void CSSStyleValuePair::setValueBackgroundImage(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // none | <image>
    m_keyKind = CSSStyleValuePair::KeyKind::BackgroundImage;
    if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_STRING("none")) {
        m_valueKind = CSSStyleValuePair::ValueKind::None;
    } else if (startsWith(value, "url(")) {
        m_valueKind = CSSStyleValuePair::ValueKind::UrlValueKind;
        m_value.m_stringValue = CSSPropertyParser::parseUrl(tokens, 0, tokens->size());
    }
}

void CSSStyleValuePair::setValueBackgroundRepeatX(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // repeat | no-repeat | initial | inherit // initial value -> repeat
    const char* value = tokens->at(0)->utf8Data();

    m_keyKind = CSSStyleValuePair::KeyKind::BackgroundRepeatX;
    m_valueKind = CSSStyleValuePair::ValueKind::BackgroundRepeatValueKind;

    if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_STRING("no-repeat")) {
        m_value.m_backgroundRepeatX = BackgroundRepeatValue::NoRepeatRepeatValue;
    } else if (VALUE_IS_STRING("repeat") || VALUE_IS_INITIAL()) {
        m_value.m_backgroundRepeatX = BackgroundRepeatValue::RepeatRepeatValue;
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

void CSSStyleValuePair::setValueBackgroundRepeatY(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // repeat | no-repeat | initial | inherit // initial value -> repeat
    const char* value = tokens->at(0)->utf8Data();

    m_keyKind = CSSStyleValuePair::KeyKind::BackgroundRepeatY;
    m_valueKind = CSSStyleValuePair::ValueKind::BackgroundRepeatValueKind;

    if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_STRING("no-repeat")) {
        m_value.m_backgroundRepeatY = BackgroundRepeatValue::NoRepeatRepeatValue;
    } else if (VALUE_IS_STRING("repeat") || VALUE_IS_INITIAL()) {
        m_value.m_backgroundRepeatY = BackgroundRepeatValue::RepeatRepeatValue;
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

void CSSStyleValuePair::setValuePercentageOrLength(const char* value)
{
    if (VALUE_IS_STRING("auto")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Auto;
    } else if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else {
        parsePercentageOrLength(*this, value);
    }
}

void CSSStyleValuePair::setValueLetterSpacing(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // normal | length | inherit
    const char* value = tokens->at(0)->utf8Data();
    m_keyKind = CSSStyleValuePair::KeyKind::LetterSpacing;
    if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_STRING("normal")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Normal;
    } else {
        parseLength(*this, value);
    }
}

void CSSStyleValuePair::setValueLineHeight(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // <normal> | number | length | percentage | inherit
    const char* value = tokens->at(0)->utf8Data();
    m_valueKind = CSSStyleValuePair::ValueKind::Normal;
    if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_STRING("inherit")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_STRING("normal")) {
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

void CSSStyleValuePair::setValuePaddingTop(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    setValuePercentageOrLength(value);
}

void CSSStyleValuePair::setValuePaddingRight(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    setValuePercentageOrLength(value);
}

void CSSStyleValuePair::setValuePaddingBottom(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    setValuePercentageOrLength(value);
}

void CSSStyleValuePair::setValuePaddingLeft(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    setValuePercentageOrLength(value);
}

void CSSStyleValuePair::setValueMarginTop(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    setValuePercentageOrLength(value);
}

void CSSStyleValuePair::setValueMarginRight(std::vector<String*, gc_allocator<String*> >* tokens)
{
    setValueMarginTop(tokens);
}

void CSSStyleValuePair::setValueMarginBottom(std::vector<String*, gc_allocator<String*> >* tokens)
{
    setValueMarginTop(tokens);
}

void CSSStyleValuePair::setValueMarginLeft(std::vector<String*, gc_allocator<String*> >* tokens)
{
    setValueMarginTop(tokens);
}

void CSSStyleValuePair::setValueTop(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // length | percentage | <auto> | inherit
    m_keyKind = CSSStyleValuePair::KeyKind::Top;
    m_valueKind = CSSStyleValuePair::ValueKind::Auto;

    if (VALUE_IS_STRING("auto")) {
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else {
        setValuePercentageOrLength(value);
    }
}

void CSSStyleValuePair::setValueBottom(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // length | percentage | <auto> | inherit
    m_keyKind = CSSStyleValuePair::KeyKind::Bottom;
    m_valueKind = CSSStyleValuePair::ValueKind::Auto;

    if (VALUE_IS_STRING("auto")) {
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else {
        setValuePercentageOrLength(value);
    }
}

void CSSStyleValuePair::setValueLeft(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // length | percentage | <auto> | inherit
    m_keyKind = CSSStyleValuePair::KeyKind::Left;
    m_valueKind = CSSStyleValuePair::ValueKind::Auto;
    if (VALUE_IS_STRING("auto")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Auto;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else {
        setValuePercentageOrLength(value);
    }
}

void CSSStyleValuePair::setValueRight(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // length | percentage | <auto> | inherit
    m_keyKind = CSSStyleValuePair::KeyKind::Right;
    m_valueKind = CSSStyleValuePair::ValueKind::Auto;

    if (VALUE_IS_STRING("auto")) {
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else {
        setValuePercentageOrLength(value);
    }
}

void CSSStyleValuePair::setValueBorderImageSlice(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // number | percentage {1, 4} && fill?
    m_keyKind = CSSStyleValuePair::KeyKind::BorderImageSlice;
    if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else {
        m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
        m_value.m_multiValue = new ValueList();
        for (unsigned int i = 0; i < tokens->size(); i++) {
            const char* currentToken = tokens->at(i)->toLower()->utf8Data();
            if (startsWith(currentToken, "fill")) {
                m_value.m_multiValue->append(CSSStyleValuePair::ValueKind::StringValueKind, { 0 });
            } else {
                char* pEnd;
                double d = strtod(currentToken, &pEnd);
                STARFISH_ASSERT(pEnd == currentToken + tokens->at(i)->length());
                m_value.m_multiValue->append(CSSStyleValuePair::ValueKind::Number, {.m_floatValue = (float)d });
            }
        }
    }
}

void CSSStyleValuePair::setValueBorderImageSource(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // none | <image>
    m_keyKind = CSSStyleValuePair::KeyKind::BorderImageSource;
    if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_STRING("none")) {
        m_valueKind = CSSStyleValuePair::ValueKind::None;
    } else if (startsWith(value, "url(")) {
        m_valueKind = CSSStyleValuePair::ValueKind::UrlValueKind;
        m_value.m_stringValue = CSSPropertyParser::parseUrl(tokens, 0, tokens->size());
    }
}

void CSSStyleValuePair::setValueBorderImageRepeat(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // <stretch> | repeat | round | space {1, 2}
    m_keyKind = CSSStyleValuePair::KeyKind::BorderImageRepeat;
    if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else {
        m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
        m_value.m_multiValue = new ValueList();

        // TODO: find better way to parse axis data
        // 1) parse X-axis data
        if (startsWith(value, "stretch")) {
            m_value.m_multiValue->append(BorderImageRepeatValueKind, {.m_borderImageRepeat = StretchValue });
        } else if (startsWith(value, "repeat")) {
            m_value.m_multiValue->append(BorderImageRepeatValueKind, {.m_borderImageRepeat = RepeatValue });
        } else if (startsWith(value, "round")) {
            m_value.m_multiValue->append(BorderImageRepeatValueKind, {.m_borderImageRepeat = RoundValue });
        } else if (startsWith(value, "space")) {
            m_value.m_multiValue->append(BorderImageRepeatValueKind, {.m_borderImageRepeat = SpaceValue });
        }
        // 2) parse Y-axis data
        if (endsWith(value, "stretch")) {
            m_value.m_multiValue->append(BorderImageRepeatValueKind, {.m_borderImageRepeat = StretchValue });
        } else if (endsWith(value, "repeat")) {
            m_value.m_multiValue->append(BorderImageRepeatValueKind, {.m_borderImageRepeat = RepeatValue });
        } else if (endsWith(value, "round")) {
            m_value.m_multiValue->append(BorderImageRepeatValueKind, {.m_borderImageRepeat = RoundValue });
        } else if (endsWith(value, "space")) {
            m_value.m_multiValue->append(BorderImageRepeatValueKind, {.m_borderImageRepeat = SpaceValue });
        }
    }
}

void CSSStyleValuePair::setValueBorderImageWidth(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // [length | number]
    m_keyKind = CSSStyleValuePair::KeyKind::BorderImageWidth;
    if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else {
        m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
        ValueList* values = new ValueList();
        for (unsigned int i = 0; i < tokens->size(); i++) {
            const char* currentToken = tokens->at(i)->utf8Data();
            if (endsWithNumber(currentToken)) {
                char* pEnd;
                double d = strtod(currentToken, &pEnd);
                STARFISH_ASSERT(pEnd == currentToken + tokens->at(i)->length());
                values->append(CSSStyleValuePair::ValueKind::Number, {.m_floatValue = (float)d });
            } else {
                CSSStyleValuePair::ValueData data = {.m_length = parseCSSLength(currentToken) };
                values->append(CSSStyleValuePair::ValueKind::Length, data);
            }
        }
        m_value.m_multiValue = values;
    }
}

void CSSStyleValuePair::setValueTransform(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL() || VALUE_IS_STRING("none")) {
        m_valueKind = CSSStyleValuePair::ValueKind::None;
    } else {
        m_valueKind = CSSStyleValuePair::ValueKind::TransformFunctions;
        CSSTransformFunction::Kind fkind;
        if (m_value.m_transforms == NULL)
            m_value.m_transforms = new CSSTransformFunctions();
        else
            m_value.m_transforms->clear();

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
                str = str->concat(tokens->at(i)->toLower());
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
            }

            ValueList* values = new ValueList(ValueList::Separator::CommaSeparator);
            for (int i = 0; i < expectedArgCnt; i++) {
                parser->consumeWhitespaces();
                if (unit == Number) {
                    parser->consumeNumber();
                    float num = parser->parsedNumber();
                    values->append(CSSStyleValuePair::ValueKind::Number, {.m_floatValue = num});
                } else if (unit == Angle) {
                    parser->consumeNumber();
                    parser->consumeString();
                    ValueData data = {.m_angle = CSSAngle(parser->parsedString(), parser->parsedNumber())};
                    values->append(CSSStyleValuePair::ValueKind::Angle, data);
                } else { // TranslationValue
                    parser->consumeNumber();
                    float num = parser->parsedNumber();
                    if (parser->consumeString()) {
                        String* str = parser->parsedString();
                        if (str->equals("%")) {
                            ValueData data = {.m_floatValue = num / 100.f};
                            values->append(CSSStyleValuePair::ValueKind::Percentage, data);
                        } else {
                            ValueData data = {.m_length = CSSLength(num)};
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
    m_keyKind = CSSStyleValuePair::KeyKind::TransformOrigin;

    if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else {
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
            } else if (endsWith(value, "%")) {
                float f;
                sscanf(value, "%f%%", &f);
                values->append(CSSStyleValuePair::ValueKind::Percentage, {.m_floatValue = (f / 100.f) });
            } else {
                CSSStyleValuePair::ValueData data = {.m_length = parseCSSLength(value) };
                values->append(CSSStyleValuePair::ValueKind::Length, data);
            }
        }

        m_value.m_multiValue = values;
    }
}

String* BorderString(String* width, String* style, String* color)
{
    String* space = String::fromUTF8(" ");
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

String* CSSStyleDeclaration::Border()
{
    String* width = BorderTopWidth();
    String* style = BorderTopStyle();
    String* color = BorderTopColor();
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

String* CSSStyleDeclaration::BorderStyle()
{
    String* t = BorderTopStyle();
    String* r = BorderLeftStyle();
    String* b = BorderBottomStyle();
    String* l = BorderLeftStyle();

    if (t->equals(r) && r->equals(b) && b->equals(l)) {
        return t;
    } else if (t->equals(b) && l->equals(r) && !t->equals(l)) {
        return t->concat(String::spaceString)->concat(l);
    } else if (l->equals(r) && !l->equals(r) && !t->equals(b)) {
        return t->concat(String::spaceString)->concat(l)->concat(String::spaceString)->concat(b);
    }
    return t->concat(String::spaceString)->concat(r)->concat(String::spaceString)->concat(b)->concat(String::spaceString)->concat(l);
}

String* CSSStyleDeclaration::BorderColor()
{
    String* t = BorderTopColor();
    String* r = BorderLeftColor();
    String* b = BorderBottomColor();
    String* l = BorderLeftColor();

    if (t->equals(r) && r->equals(b) && b->equals(l)) {
        return t;
    } else if (t->equals(b) && l->equals(r) && !t->equals(l)) {
        return t->concat(String::spaceString)->concat(l);
    } else if (l->equals(r) && !l->equals(r) && !t->equals(b)) {
        return t->concat(String::spaceString)->concat(l)->concat(String::spaceString)->concat(b);
    }
    return t->concat(String::spaceString)->concat(r)->concat(String::spaceString)->concat(b)->concat(String::spaceString)->concat(l);
}

String* CSSStyleDeclaration::BorderWidth()
{
    String* t = BorderTopWidth();
    String* r = BorderLeftWidth();
    String* b = BorderBottomWidth();
    String* l = BorderLeftWidth();

    if (t->equals(r) && r->equals(b) && b->equals(l)) {
        return t;
    } else if (t->equals(b) && l->equals(r) && !t->equals(l)) {
        return t->concat(String::spaceString)->concat(l);
    } else if (l->equals(r) && !l->equals(r) && !t->equals(b)) {
        return t->concat(String::spaceString)->concat(l)->concat(String::spaceString)->concat(b);
    }
    return t->concat(String::spaceString)->concat(r)->concat(String::spaceString)->concat(b)->concat(String::spaceString)->concat(l);
}

BorderShorthandValueType checkBorderValueType(const char* token)
{
    if (CSSPropertyParser::assureBorderWidth(token)) {
        return BorderShorthandValueType::BWidth;
    } else if (CSSPropertyParser::assureBorderStyle(token)) {
        return BorderShorthandValueType::BStyle;
    } else if (CSSPropertyParser::assureBorderColor(token)) {
        return BorderShorthandValueType::BColor;
    } else {
        return BorderShorthandValueType::BInvalid;
    }
}

void CSSStyleDeclaration::setBorderWidth(const char* value)
{
    std::vector<String*, gc_allocator<String*> > tokens;
    DOMTokenList::tokenize(&tokens, String::fromUTF8(value));
    if (tokens.size() == 1) {
        setBorderTopWidth(value);
        setBorderRightWidth(value);
        setBorderBottomWidth(value);
        setBorderLeftWidth(value);
    } else if (tokens.size() == 2) {
        setBorderTopWidth(tokens[0]->utf8Data());
        setBorderBottomWidth(tokens[0]->utf8Data());
        setBorderLeftWidth(tokens[1]->utf8Data());
        setBorderRightWidth(tokens[1]->utf8Data());
    } else if (tokens.size() == 3) {
        setBorderTopWidth(tokens[0]->utf8Data());
        setBorderLeftWidth(tokens[1]->utf8Data());
        setBorderRightWidth(tokens[1]->utf8Data());
        setBorderBottomWidth(tokens[2]->utf8Data());
    } else {
        setBorderTopWidth(tokens[0]->utf8Data());
        setBorderRightWidth(tokens[1]->utf8Data());
        setBorderBottomWidth(tokens[2]->utf8Data());
        setBorderLeftWidth(tokens[3]->utf8Data());
    }
}

void CSSStyleDeclaration::setBorderStyle(const char* value)
{
    std::vector<String*, gc_allocator<String*> > tokens;
    DOMTokenList::tokenize(&tokens, String::fromUTF8(value));
    if (tokens.size() == 1) {
        setBorderTopStyle(value);
        setBorderRightStyle(value);
        setBorderBottomStyle(value);
        setBorderLeftStyle(value);
    } else if (tokens.size() == 2) {
        setBorderTopStyle(tokens[0]->utf8Data());
        setBorderBottomStyle(tokens[0]->utf8Data());
        setBorderLeftStyle(tokens[1]->utf8Data());
        setBorderRightStyle(tokens[1]->utf8Data());
    } else if (tokens.size() == 3) {
        setBorderTopStyle(tokens[0]->utf8Data());
        setBorderLeftStyle(tokens[1]->utf8Data());
        setBorderRightStyle(tokens[1]->utf8Data());
        setBorderBottomStyle(tokens[2]->utf8Data());
    } else {
        setBorderTopStyle(tokens[0]->utf8Data());
        setBorderRightStyle(tokens[1]->utf8Data());
        setBorderBottomStyle(tokens[2]->utf8Data());
        setBorderLeftStyle(tokens[3]->utf8Data());
    }
}

void CSSStyleDeclaration::setBorderColor(const char* value)
{
    std::vector<String*, gc_allocator<String*> > tokens;
    DOMTokenList::tokenize(&tokens, String::fromUTF8(value));
    if (tokens.size() == 1) {
        setBorderTopColor(value);
        setBorderRightColor(value);
        setBorderBottomColor(value);
        setBorderLeftColor(value);
    } else if (tokens.size() == 2) {
        setBorderTopColor(tokens[0]->utf8Data());
        setBorderBottomColor(tokens[0]->utf8Data());
        setBorderLeftColor(tokens[1]->utf8Data());
        setBorderRightColor(tokens[1]->utf8Data());
    } else if (tokens.size() == 3) {
        setBorderTopColor(tokens[0]->utf8Data());
        setBorderLeftColor(tokens[1]->utf8Data());
        setBorderRightColor(tokens[1]->utf8Data());
        setBorderBottomColor(tokens[2]->utf8Data());
    } else {
        setBorderTopColor(tokens[0]->utf8Data());
        setBorderRightColor(tokens[1]->utf8Data());
        setBorderBottomColor(tokens[2]->utf8Data());
        setBorderLeftColor(tokens[3]->utf8Data());
    }
}

void CSSStyleDeclaration::setBorder(const char* value)
{
    std::vector<String*, gc_allocator<String*> > tokens;
    DOMTokenList::tokenize(&tokens, String::fromUTF8(value));
    if (checkInputErrorBorder(&tokens)) {
        if (tokens.size() == 1 && VALUE_IS_INHERIT()) {
            setBorderWidth("inherit");
            setBorderStyle("inherit");
            setBorderColor("inherit");
            return;
        }

        bool hasWidth = false, hasStyle = false, hasColor = false;
        for (unsigned i = 0; i < tokens.size(); i++) {
            switch (checkBorderValueType(tokens[i]->utf8Data())) {
            case BorderShorthandValueType::BWidth:
                setBorderWidth(tokens[i]->utf8Data());
                hasWidth = true;
                break;
            case BorderShorthandValueType::BStyle:
                setBorderStyle(tokens[i]->utf8Data());
                hasStyle = true;
                break;
            case BorderShorthandValueType::BColor:
                setBorderColor(tokens[i]->utf8Data());
                hasColor = true;
                break;
            default:
                break;
            }
        }
        if (!hasWidth)
            setBorderWidth("initial");
        if (!hasStyle)
            setBorderStyle("initial");
        if (!hasColor)
            setBorderColor("initial");
    }
}

void CSSStyleDeclaration::setBorderTop(const char* value)
{
    std::vector<String*, gc_allocator<String*> > tokens;
    DOMTokenList::tokenize(&tokens, String::fromUTF8(value));
    if (checkInputErrorBorderTop(&tokens)) {
        if (tokens.size() == 1 && VALUE_IS_INHERIT()) {
            setBorderTopWidth("inherit");
            setBorderTopStyle("inherit");
            setBorderTopColor("inherit");
            return;
        }

        bool hasWidth = false, hasStyle = false, hasColor = false;
        for (unsigned i = 0; i < tokens.size(); i++) {
            switch (checkBorderValueType(tokens[i]->utf8Data())) {
            case BorderShorthandValueType::BWidth:
                setBorderTopWidth(tokens[i]->utf8Data());
                hasWidth = true;
                break;
            case BorderShorthandValueType::BStyle:
                setBorderTopStyle(tokens[i]->utf8Data());
                hasStyle = true;
                break;
            case BorderShorthandValueType::BColor:
                setBorderTopColor(tokens[i]->utf8Data());
                hasColor = true;
                break;
            default:
                break;
            }
        }
        if (!hasWidth)
            setBorderTopWidth("initial");
        if (!hasStyle)
            setBorderTopStyle("initial");
        if (!hasColor)
            setBorderTopColor("initial");
    }
}

void CSSStyleDeclaration::setBorderRight(const char* value)
{
    std::vector<String*, gc_allocator<String*> > tokens;
    DOMTokenList::tokenize(&tokens, String::fromUTF8(value));
    if (checkInputErrorBorderRight(&tokens)) {
        if (tokens.size() == 1 && VALUE_IS_INHERIT()) {
            setBorderRightWidth("inherit");
            setBorderRightStyle("inherit");
            setBorderRightColor("inherit");
            return;
        }

        bool hasWidth = false, hasStyle = false, hasColor = false;
        for (unsigned i = 0; i < tokens.size(); i++) {
            switch (checkBorderValueType(tokens[i]->utf8Data())) {
            case BorderShorthandValueType::BWidth:
                setBorderRightWidth(tokens[i]->utf8Data());
                hasWidth = true;
                break;
            case BorderShorthandValueType::BStyle:
                setBorderRightStyle(tokens[i]->utf8Data());
                hasStyle = true;
                break;
            case BorderShorthandValueType::BColor:
                setBorderRightColor(tokens[i]->utf8Data());
                hasColor = true;
                break;
            default:
                break;
            }
        }
        if (!hasWidth)
            setBorderRightWidth("initial");
        if (!hasStyle)
            setBorderRightStyle("initial");
        if (!hasColor)
            setBorderRightColor("initial");
    }
}

void CSSStyleDeclaration::setBorderBottom(const char* value)
{
    std::vector<String*, gc_allocator<String*> > tokens;
    DOMTokenList::tokenize(&tokens, String::fromUTF8(value));
    if (checkInputErrorBorderBottom(&tokens)) {
        if (tokens.size() == 1 && VALUE_IS_INHERIT()) {
            setBorderBottomWidth("inherit");
            setBorderBottomStyle("inherit");
            setBorderBottomColor("inherit");
            return;
        }

        bool hasWidth = false, hasStyle = false, hasColor = false;
        for (unsigned i = 0; i < tokens.size(); i++) {
            switch (checkBorderValueType(tokens[i]->utf8Data())) {
            case BorderShorthandValueType::BWidth:
                setBorderBottomWidth(tokens[i]->utf8Data());
                hasWidth = true;
                break;
            case BorderShorthandValueType::BStyle:
                setBorderBottomStyle(tokens[i]->utf8Data());
                hasStyle = true;
                break;
            case BorderShorthandValueType::BColor:
                setBorderBottomColor(tokens[i]->utf8Data());
                hasColor = true;
                break;
            default:
                break;
            }
        }
        if (!hasWidth)
            setBorderBottomWidth("initial");
        if (!hasStyle)
            setBorderBottomStyle("initial");
        if (!hasColor)
            setBorderBottomColor("initial");
    }
}

void CSSStyleDeclaration::setBorderLeft(const char* value)
{
    std::vector<String*, gc_allocator<String*> > tokens;
    DOMTokenList::tokenize(&tokens, String::fromUTF8(value));
    if (checkInputErrorBorderLeft(&tokens)) {
        if (tokens.size() == 1 && VALUE_IS_INHERIT()) {
            setBorderLeftWidth("inherit");
            setBorderLeftStyle("inherit");
            setBorderLeftColor("inherit");
            return;
        }

        bool hasWidth = false, hasStyle = false, hasColor = false;
        for (unsigned i = 0; i < tokens.size(); i++) {
            switch (checkBorderValueType(tokens[i]->utf8Data())) {
            case BorderShorthandValueType::BWidth:
                setBorderLeftWidth(tokens[i]->utf8Data());
                hasWidth = true;
                break;
            case BorderShorthandValueType::BStyle:
                setBorderLeftStyle(tokens[i]->utf8Data());
                hasStyle = true;
                break;
            case BorderShorthandValueType::BColor:
                setBorderLeftColor(tokens[i]->utf8Data());
                hasColor = true;
                break;
            default:
                break;
            }
        }
        if (!hasWidth)
            setBorderLeftWidth("initial");
        if (!hasStyle)
            setBorderLeftStyle("initial");
        if (!hasColor)
            setBorderLeftColor("initial");
    }
}

void CSSStyleValuePair::setValueBorderTopColor(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // color | transparent | inherit
    if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_STRING("transparent")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Transparent;
    } else {
        setValueColor(tokens);
    }
}

void CSSStyleValuePair::setValueBorderRightColor(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // color | transparent | inherit
    if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_STRING("transparent")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Transparent;
    } else {
        setValueColor(tokens);
    }
}

void CSSStyleValuePair::setValueBorderBottomColor(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // color | transparent | inherit
    if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_STRING("transparent")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Transparent;
    } else {
        setValueColor(tokens);
    }
}

void CSSStyleValuePair::setValueBorderLeftColor(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // color | transparent | inherit
    if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_STRING("transparent")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Transparent;
    } else {
        setValueColor(tokens);
    }
}

void CSSStyleValuePair::setValueBorderTopStyle(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // border-style(<none> | solid) | inherit
    m_valueKind = CSSStyleValuePair::ValueKind::BorderNone;
    if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_STRING("solid")) {
        m_valueKind = CSSStyleValuePair::ValueKind::BorderSolid;
    }
}

void CSSStyleValuePair::setValueBorderRightStyle(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // border-style(<none> | solid) | inherit
    m_valueKind = CSSStyleValuePair::ValueKind::BorderNone;
    if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_STRING("solid")) {
        m_valueKind = CSSStyleValuePair::ValueKind::BorderSolid;
    }
}

void CSSStyleValuePair::setValueBorderBottomStyle(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // border-style(<none> | solid) | inherit
    m_valueKind = CSSStyleValuePair::ValueKind::BorderNone;
    if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_STRING("solid")) {
        m_valueKind = CSSStyleValuePair::ValueKind::BorderSolid;
    }
}

void CSSStyleValuePair::setValueBorderLeftStyle(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // border-style(<none> | solid) | inherit
    m_valueKind = CSSStyleValuePair::ValueKind::BorderNone;
    if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_STRING("solid")) {
        m_valueKind = CSSStyleValuePair::ValueKind::BorderSolid;
    }
}

void CSSStyleValuePair::setValueBorderTopWidth(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // border-width(thin | <medium> | thick | length) | inherit
    if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_STRING("thin")) {
        m_valueKind = CSSStyleValuePair::ValueKind::BorderThin;
    } else if (VALUE_IS_STRING("medium")) {
        m_valueKind = CSSStyleValuePair::ValueKind::BorderMedium;
    } else if (VALUE_IS_STRING("thick")) {
        m_valueKind = CSSStyleValuePair::ValueKind::BorderThick;
    } else {
        setValuePercentageOrLength(value);
    }
}

void CSSStyleValuePair::setValueBorderRightWidth(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // border-width(thin | <medium> | thick | length) | inherit
    if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_STRING("thin")) {
        m_valueKind = CSSStyleValuePair::ValueKind::BorderThin;
    } else if (VALUE_IS_STRING("medium")) {
        m_valueKind = CSSStyleValuePair::ValueKind::BorderMedium;
    } else if (VALUE_IS_STRING("thick")) {
        m_valueKind = CSSStyleValuePair::ValueKind::BorderThick;
    } else {
        setValuePercentageOrLength(value);
    }
}

void CSSStyleValuePair::setValueBorderBottomWidth(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // border-width(thin | <medium> | thick | length) | inherit
    if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_STRING("thin")) {
        m_valueKind = CSSStyleValuePair::ValueKind::BorderThin;
    } else if (VALUE_IS_STRING("medium")) {
        m_valueKind = CSSStyleValuePair::ValueKind::BorderMedium;
    } else if (VALUE_IS_STRING("thick")) {
        m_valueKind = CSSStyleValuePair::ValueKind::BorderThick;
    } else {
        setValuePercentageOrLength(value);
    }
}

void CSSStyleValuePair::setValueBorderLeftWidth(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    // border-width(thin | <medium> | thick | length) | inherit
    if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_STRING("thin")) {
        m_valueKind = CSSStyleValuePair::ValueKind::BorderThin;
    } else if (VALUE_IS_STRING("medium")) {
        m_valueKind = CSSStyleValuePair::ValueKind::BorderMedium;
    } else if (VALUE_IS_STRING("thick")) {
        m_valueKind = CSSStyleValuePair::ValueKind::BorderThick;
    } else {
        setValuePercentageOrLength(value);
    }
}

void CSSStyleValuePair::setValueTextAlign(std::vector<String*, gc_allocator<String*> >* tokens)
{

    const char* value = tokens->at(0)->utf8Data();
    m_keyKind = CSSStyleValuePair::KeyKind::TextAlign;
    m_valueKind = CSSStyleValuePair::ValueKind::Inherit;

    if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_STRING("left")) {
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

void CSSStyleValuePair::setValueVisibility(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();

    m_keyKind = CSSStyleValuePair::KeyKind::Visibility;
    m_valueKind = CSSStyleValuePair::ValueKind::VisibilityKind;

    if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_STRING("visible")) {
        m_value.m_visibility = VisibilityValue::VisibleVisibilityValue;
    } else if (VALUE_IS_STRING("hidden")) {
        m_value.m_visibility = VisibilityValue::HiddenVisibilityValue;
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

void CSSStyleValuePair::setValueOpacity(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();

    m_keyKind = CSSStyleValuePair::KeyKind::Opacity;
    m_valueKind = CSSStyleValuePair::ValueKind::Number;

    if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else {
        sscanf(value, "%f%%", &m_value.m_floatValue);
        if (m_value.m_floatValue > 1.0)
            m_value.m_floatValue = 1.0;
    }
}

void CSSStyleValuePair::setValueOverflowX(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();

    m_keyKind = CSSStyleValuePair::KeyKind::OverflowX;
    m_valueKind = CSSStyleValuePair::ValueKind::OverflowValueKind;

    if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_STRING("auto") || VALUE_IS_STRING("scroll")) {
        // Not Supported!!
    } else if (VALUE_IS_STRING("visible")) {
        m_value.m_overflowX = OverflowValue::VisibleOverflow;
    } else if (VALUE_IS_STRING("hidden")) {
        m_value.m_overflowX = OverflowValue::HiddenOverflow;
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}
void CSSStyleValuePair::setValueZIndex(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    if (VALUE_IS_STRING("auto")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Auto;
    } else if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else {
        // TODO check string has right color string
        m_valueKind = CSSStyleValuePair::ValueKind::Number;
        sscanf(value, "%f%%", &m_value.m_floatValue);
    }
}

void CSSStyleValuePair::setValueVerticalAlign(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* value = tokens->at(0)->utf8Data();
    m_keyKind = CSSStyleValuePair::KeyKind::VerticalAlign;
    if (VALUE_IS_INITIAL()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (VALUE_IS_INHERIT()) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (VALUE_IS_STRING("baseline")) {
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
    switch (keyKind()) {
    case Color:
    case BackgroundColor: {
        if (m_valueKind == CSSStyleValuePair::ValueKind::Auto)
            return String::fromUTF8("auto");
        else if (m_valueKind == CSSStyleValuePair::ValueKind::Initial)
            return String::fromUTF8("initial");
        else if (m_valueKind == CSSStyleValuePair::ValueKind::Inherit)
            return String::fromUTF8("inherit");
        else if (m_valueKind == CSSStyleValuePair::ValueKind::StringValueKind)
            return stringValue();
        break;
    }
    case BackgroundImage:
        if (m_valueKind == CSSStyleValuePair::ValueKind::None) {
            return String::fromUTF8("none");
        } else if (m_valueKind == CSSStyleValuePair::ValueKind::Inherit) {
            return String::fromUTF8("inherit");
        } else if (m_valueKind == CSSStyleValuePair::ValueKind::Initial) {
            return String::fromUTF8("initial");
        } else if (m_valueKind == CSSStyleValuePair::ValueKind::UrlValueKind) {
            return urlValue();
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case BackgroundRepeatX:
        if (m_valueKind == CSSStyleValuePair::ValueKind::Inherit) {
            return String::fromUTF8("inherit");
        } else if (m_valueKind == CSSStyleValuePair::ValueKind::Initial) {
            return String::fromUTF8("initial");
        } else if (m_valueKind == CSSStyleValuePair::ValueKind::BackgroundRepeatValueKind) {
            if (backgroundRepeatXValue() == RepeatRepeatValue)
                return String::fromUTF8("repeat");
            else
                return String::fromUTF8("no-repeat");
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    case BackgroundRepeatY:
        if (m_valueKind == CSSStyleValuePair::ValueKind::Inherit) {
            return String::fromUTF8("inherit");
        } else if (m_valueKind == CSSStyleValuePair::ValueKind::Initial) {
            return String::fromUTF8("initial");
        } else if (m_valueKind == CSSStyleValuePair::ValueKind::BackgroundRepeatValueKind) {
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
        // TODO add initial
        switch (m_valueKind) {
        case CSSStyleValuePair::ValueKind::Cover:
            return String::fromUTF8("cover");
        case CSSStyleValuePair::ValueKind::Contain:
            return String::fromUTF8("contain");
        case CSSStyleValuePair::ValueKind::Initial:
            return String::fromUTF8("initial");
        case CSSStyleValuePair::ValueKind::Inherit:
            return String::fromUTF8("inherit");
        case CSSStyleValuePair::ValueKind::ValueListKind: {
            String* str = String::fromUTF8("");
            ValueList* vals = multiValue();
            for (unsigned int i = 0; i < vals->size(); i++) {
                str = str->concat(valueToString(vals->getValueKindAtIndex(i),
                    vals->getValueAtIndex(i)));
                if (i < vals->size() - 1) {
                    str = str->concat(String::fromUTF8(" "));
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
    case LetterSpacing:
        if (m_valueKind == CSSStyleValuePair::ValueKind::Normal) {
            return String::fromUTF8("normal");
        } else {
            return valueToString();
        }
        break;
    case LineHeight:
        if (m_valueKind == CSSStyleValuePair::ValueKind::Normal) {
            return String::fromUTF8("normal");
        } else {
            return valueToString();
        }
        break;
    case PaddingTop:
    case PaddingRight:
    case PaddingBottom:
    case PaddingLeft:
        return lengthOrPercentageOrKeywordToString();
    case MarginTop:
    case MarginRight:
    case MarginBottom:
    case MarginLeft:
        return lengthOrPercentageOrKeywordToString();
    case Top:
    case Bottom:
    case Left:
    case Right:
        return lengthOrPercentageOrKeywordToString();
    case Height:
    case Width:
        return lengthOrPercentageOrKeywordToString();

    case Position: {
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
        switch (valueKind()) {
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
            return lengthOrPercentageOrKeywordToString();
        }
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
            String* s = String::fromUTF8("");
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
                    s = s->concat(String::fromUTF8(" "));
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
            String* s = String::fromUTF8("");
            for (unsigned int i = 0; i < values->size(); i++) {
                String* newstr;
                if (values->getValueKindAtIndex(i) == CSSStyleValuePair::ValueKind::StringValueKind)
                    newstr = String::fromUTF8("fill");
                else
                    newstr = valueToString(values->getValueKindAtIndex(i),
                        values->getValueAtIndex(i));
                s = s->concat(newstr);
                if (i != values->size() - 1)
                    s = s->concat(String::fromUTF8(" "));
            }
            return s;
        }
        default:
            // initial or inherit
            return lengthOrPercentageOrKeywordToString();
        }
    }
    case BorderImageSource: {
        switch (valueKind()) {
        case CSSStyleValuePair::ValueKind::UrlValueKind: {
            String* str = String::fromUTF8("url(\"");
            if (urlValue())
                str = str->concat(urlValue());
            str = str->concat(String::fromUTF8("\")"));
            return str;
        }
        default:
            // initial or inherit or none
            return lengthOrPercentageOrKeywordToString();
        }
    }
    case BorderImageWidth: {
        switch (valueKind()) {
        case CSSStyleValuePair::ValueKind::ValueListKind: {
            ValueList* values = multiValue();
            String* s = String::fromUTF8("");
            for (unsigned int i = 0; i < values->size(); i++) {
                String* newstr = valueToString(values->getValueKindAtIndex(i),
                    values->getValueAtIndex(i));
                s = s->concat(newstr);
                if (i != values->size() - 1)
                    s = s->concat(String::fromUTF8(" "));
            }
            return s;
        }
        default:
            return lengthOrPercentageOrKeywordToString();
        }
    }
    case BorderTopColor:
    case BorderRightColor:
    case BorderBottomColor:
    case BorderLeftColor: {
        switch (valueKind()) {
        case CSSStyleValuePair::ValueKind::Initial:
            return String::fromUTF8("initial");
        case CSSStyleValuePair::ValueKind::Transparent:
            return String::fromUTF8("transparent");
        case CSSStyleValuePair::ValueKind::StringValueKind:
            return stringValue();
        case CSSStyleValuePair::ValueKind::Inherit:
            return String::fromUTF8("inherit");
        default:
            break;
        }
    }
    case BorderTopStyle:
    case BorderRightStyle:
    case BorderBottomStyle:
    case BorderLeftStyle: {
        switch (valueKind()) {
        case CSSStyleValuePair::ValueKind::Initial:
            return String::fromUTF8("initial");
        case CSSStyleValuePair::ValueKind::BorderNone:
            return String::fromUTF8("none");
        case CSSStyleValuePair::ValueKind::BorderSolid:
            return String::fromUTF8("solid");
        case CSSStyleValuePair::ValueKind::Inherit:
            return String::fromUTF8("inherit");
        default:
            break;
        }
    }
    case BorderTopWidth:
    case BorderBottomWidth:
    case BorderRightWidth:
    case BorderLeftWidth: {
        switch (valueKind()) {
        case CSSStyleValuePair::ValueKind::Initial:
            return String::fromUTF8("initial");
        case CSSStyleValuePair::ValueKind::BorderThin:
            return String::fromUTF8("thin");
        case CSSStyleValuePair::ValueKind::BorderMedium:
            return String::fromUTF8("medium");
        case CSSStyleValuePair::ValueKind::BorderThick:
            return String::fromUTF8("thick");
        case CSSStyleValuePair::ValueKind::Inherit:
            return String::fromUTF8("inherit");
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
        return String::fromUTF8(std::to_string(numberValue()).c_str());
    }
    case OverflowX: {
        switch (overflowValueX()) {
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
        else if (m_valueKind == CSSStyleValuePair::ValueKind::Initial)
            return String::fromUTF8("initial");
        else if (m_valueKind == CSSStyleValuePair::ValueKind::Inherit)
            return String::fromUTF8("inherit");
        else if (m_valueKind == CSSStyleValuePair::ValueKind::Number)
            return String::fromUTF8(std::to_string((int)numberValue()).c_str());
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
        case CSSStyleValuePair::ValueKind::Initial:
            return String::fromUTF8("initial");
        case CSSStyleValuePair::ValueKind::Inherit:
            return String::fromUTF8("inherit");
        case CSSStyleValuePair::ValueKind::ValueListKind: {
            String* str = String::fromUTF8("");
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
                    str = str->concat(String::fromUTF8(" "));
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
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
        return nullptr;
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

String* CSSStyleDeclaration::Margin()
{
    String* top = MarginTop();
    if (!top->equals(String::emptyString)) {
        String* right = MarginRight();
        if (!right->equals(String::emptyString)) {
            String* bottom = MarginBottom();
            if (!bottom->equals(String::emptyString)) {
                String* left = MarginLeft();
                if (!left->equals(String::emptyString)) {
                    return combineBoxString(top, right, bottom, left);
                }
            }
        }
    }
    return String::emptyString;
}

void CSSStyleDeclaration::setMargin(const char* value)
{
    std::vector<String*, gc_allocator<String*> > tokens;
    DOMTokenList::tokenize(&tokens, String::fromUTF8(value));
    if (checkInputErrorMargin(&tokens)) {
        unsigned len = tokens.size();
        if (len == 1) {
            tokens.push_back(tokens[0]);
            tokens.push_back(tokens[0]);
            tokens.push_back(tokens[0]);
        } else if (len == 2) {
            tokens.push_back(tokens[0]);
            tokens.push_back(tokens[1]);
        } else if (len == 3)
            tokens.push_back(tokens[1]);

        setMarginTop(tokens[0]->utf8Data());
        setMarginRight(tokens[1]->utf8Data());
        setMarginBottom(tokens[2]->utf8Data());
        setMarginLeft(tokens[3]->utf8Data());
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
        return String::fromUTF8("initial");
    else if (repeatX->equals("inherit") && repeatY->equals("inherit"))
        return String::fromUTF8("inherit");

    return String::emptyString;
}

void CSSStyleDeclaration::setBackgroundRepeat(const char* value)
{
    std::vector<String*, gc_allocator<String*> > tokens;
    DOMTokenList::tokenize(&tokens, String::fromUTF8(value));

    if (checkInputErrorBackgroundRepeat(&tokens)) {
        if (VALUE_IS_STRING("repeat")) {
            setBackgroundRepeatX("repeat");
            setBackgroundRepeatY("repeat");
        } else if (VALUE_IS_STRING("repeat-x")) {
            setBackgroundRepeatX("repeat");
            setBackgroundRepeatY("no-repeat");
        } else if (VALUE_IS_STRING("repeat-y")) {
            setBackgroundRepeatX("no-repeat");
            setBackgroundRepeatY("repeat");
        } else if (VALUE_IS_STRING("no-repeat")) {
            setBackgroundRepeatX("no-repeat");
            setBackgroundRepeatY("no-repeat");
        } else {
            setBackgroundRepeatX(value);
            setBackgroundRepeatY(value);
        }
    }
}

String* CSSStyleDeclaration::Background()
{
    String* image = BackgroundImage();
    String* repeat;
    String* repeatX = BackgroundRepeatX();
    String* repeatY = BackgroundRepeatY();
    String* color = BackgroundColor();

    if (repeatX->equals("repeat") && repeatY->equals("repeat"))
        repeat = String::fromUTF8("repeat");
    else if (repeatX->equals("repeat") && repeatY->equals("no-repeat"))
        repeat = String::fromUTF8("repeat-x");
    else if (repeatX->equals("no-repeat") && repeatY->equals("repeat"))
        repeat = String::fromUTF8("repeat-y");
    else if (repeatX->equals("no-repeat") && repeatY->equals("no-repeat"))
        repeat = String::fromUTF8("no-repeat");
    else if (repeatX->equals("initial") && repeatY->equals("initial"))
        repeat = String::fromUTF8("");
    else if (repeatX->equals("inherit") && repeatY->equals("inherit"))
        repeat = String::fromUTF8("");

    return image->concat(String::fromUTF8(" "))->concat(repeat)->concat(String::fromUTF8(" "))->concat(color);
}

void CSSStyleDeclaration::setBackground(const char* value)
{
    //  [<'background-color'> || <'background-image'> || <'background-repeat'>] | inherit
    std::vector<String*, gc_allocator<String*> > tokens;
    DOMTokenList::tokenize(&tokens, String::fromUTF8(value));

    {
        size_t len = tokens.size();
        if (len == 1) {
            if (checkInputErrorBackgroundColor(&tokens)) {
                setBackgroundColor(value);
                setBackgroundImage("initial");
                setBackgroundRepeat("initial");
            } else if (checkInputErrorBackgroundImage(&tokens)) {
                setBackgroundImage(value);
                setBackgroundRepeat("initial");
                setBackgroundColor("initial");
            } else if (checkInputErrorBackgroundRepeat(&tokens)) {
                setBackgroundRepeat(value);
                setBackgroundImage("initial");
                setBackgroundColor("initial");
            } else {
                setBackgroundRepeat(value);
                setBackgroundImage(value);
                setBackgroundColor(value);
            }
        } else if (len == 2) {
            std::vector<String*, gc_allocator<String*> > token0;
            std::vector<String*, gc_allocator<String*> > token1;

            token0.assign(tokens.begin(), tokens.end() - 1);
            token1.assign(tokens.begin() + 1, tokens.end());

            if (checkInputErrorBackgroundColor(&token0) && checkInputErrorBackgroundImage(&token1)) {
                setBackgroundColor(token0[0]->utf8Data());
                setBackgroundImage(token1[0]->utf8Data());
                setBackgroundRepeat("initial");
            } else if (checkInputErrorBackgroundImage(&token0) && checkInputErrorBackgroundColor(&token1)) {
                setBackgroundImage(token0[0]->utf8Data());
                setBackgroundColor(token1[0]->utf8Data());
                setBackgroundRepeat("initial");
            } else if (checkInputErrorBackgroundColor(&token0) && checkInputErrorBackgroundRepeat(&token1)) {
                setBackgroundColor(token0[0]->utf8Data());
                setBackgroundRepeat(token1[0]->utf8Data());
                setBackgroundImage("initial");
            } else if (checkInputErrorBackgroundRepeat(&token0) && checkInputErrorBackgroundColor(&token1)) {
                setBackgroundRepeat(token0[0]->utf8Data());
                setBackgroundColor(token1[0]->utf8Data());
                setBackgroundImage("initial");
            } else if (checkInputErrorBackgroundImage(&token0) && checkInputErrorBackgroundRepeat(&token1)) {
                setBackgroundImage(token0[0]->utf8Data());
                setBackgroundRepeat(token1[0]->utf8Data());
                setBackgroundColor("initial");
            } else if (checkInputErrorBackgroundRepeat(&token0) && checkInputErrorBackgroundImage(&token1)) {
                setBackgroundRepeat(token0[0]->utf8Data());
                setBackgroundImage(token1[0]->utf8Data());
                setBackgroundColor("initial");
            }
        } else if (len == 3) {
            std::vector<String*, gc_allocator<String*> > token0;
            std::vector<String*, gc_allocator<String*> > token1;
            std::vector<String*, gc_allocator<String*> > token2;

            token0.assign(tokens.begin(), tokens.end() - 2);
            token1.assign(tokens.begin() + 1, tokens.end() - 1);
            token2.assign(tokens.begin() + 2, tokens.end());

            if (checkInputErrorBackgroundColor(&token0) && checkInputErrorBackgroundImage(&token1) && checkInputErrorBackgroundRepeat(&token2)) {
                setBackgroundColor(token0[0]->utf8Data());
                setBackgroundImage(token1[0]->utf8Data());
                setBackgroundRepeat(token2[0]->utf8Data());
            } else if (checkInputErrorBackgroundColor(&token0) && checkInputErrorBackgroundRepeat(&token1) && checkInputErrorBackgroundImage(&token2)) {
                setBackgroundColor(token0[0]->utf8Data());
                setBackgroundRepeat(token1[0]->utf8Data());
                setBackgroundImage(token2[0]->utf8Data());
            } else if (checkInputErrorBackgroundRepeat(&token0) && checkInputErrorBackgroundColor(&token1) && checkInputErrorBackgroundImage(&token2)) {
                setBackgroundRepeat(token0[0]->utf8Data());
                setBackgroundColor(token1[0]->utf8Data());
                setBackgroundImage(token2[0]->utf8Data());
            } else if (checkInputErrorBackgroundRepeat(&token0) && checkInputErrorBackgroundImage(&token1) && checkInputErrorBackgroundColor(&token2)) {
                setBackgroundRepeat(token0[0]->utf8Data());
                setBackgroundImage(token1[0]->utf8Data());
                setBackgroundColor(token2[0]->utf8Data());
            } else if (checkInputErrorBackgroundImage(&token0) && checkInputErrorBackgroundColor(&token1) && checkInputErrorBackgroundRepeat(&token2)) {
                setBackgroundImage(token0[0]->utf8Data());
                setBackgroundColor(token1[0]->utf8Data());
                setBackgroundRepeat(token2[0]->utf8Data());
            } else if (checkInputErrorBackgroundImage(&token0) && checkInputErrorBackgroundRepeat(&token1) && checkInputErrorBackgroundColor(&token2)) {
                setBackgroundImage(token0[0]->utf8Data());
                setBackgroundRepeat(token1[0]->utf8Data());
                setBackgroundColor(token2[0]->utf8Data());
            }
        }
    }
}

String* CSSStyleDeclaration::Padding()
{
    String* top = PaddingTop();
    if (!top->equals(String::emptyString)) {
        String* right = PaddingRight();
        if (!right->equals(String::emptyString)) {
            String* bottom = PaddingBottom();
            if (!bottom->equals(String::emptyString)) {
                String* left = PaddingLeft();
                if (!left->equals(String::emptyString)) {
                    return combineBoxString(top, right, bottom, left);
                }
            }
        }
    }
    return String::emptyString;
}

void CSSStyleDeclaration::setPadding(const char* value)
{
    if (*value == '\0') {
        setPaddingTop(value);
        setPaddingRight(value);
        setPaddingBottom(value);
        setPaddingLeft(value);
        return;
    }
    std::vector<String*, gc_allocator<String*> > tokens;
    DOMTokenList::tokenize(&tokens, String::fromUTF8(value));
    unsigned len = tokens.size();
    if (len < 1 || len > 4)
        return;
    if (len != 1) {
        for (unsigned int i = 0; i < len; i++) {
            String* current = tokens.at(i)->toLower();
            if (current->equals("initial"))
                return;
            if (current->equals("inherit"))
                return;
            if (!CSSPropertyParser::assureLength(current->utf8Data(), false)
                && !CSSPropertyParser::assurePercent(current->utf8Data(), false))
                return;
        }
    }
    if (len == 1) {
        tokens.push_back(tokens[0]);
        tokens.push_back(tokens[0]);
        tokens.push_back(tokens[0]);
    } else if (len == 2) {
        tokens.push_back(tokens[0]);
        tokens.push_back(tokens[1]);
    } else if (len == 3)
        tokens.push_back(tokens[1]);

    setPaddingTop(tokens[0]->utf8Data());
    setPaddingRight(tokens[1]->utf8Data());
    setPaddingBottom(tokens[2]->utf8Data());
    setPaddingLeft(tokens[3]->utf8Data());
}

bool CSSStyleDeclaration::checkInputErrorColor(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // color | percentage | <auto> | inherit
    if (tokens->size() > 1) {
        String* str = String::emptyString;
        for (unsigned i = 0; i < tokens->size(); i++) {
            str = str->concat(tokens->at(i)->toLower());
        }
        (*tokens)[0] = str;
    }
    const char* token = tokens->at(0)->utf8Data();
    if (!(CSSPropertyParser::assureColor(token) || (strcmp(token, "initial") == 0) || (strcmp(token, "inherit") == 0))) {
        return false;
    }
    return true;
}

bool CSSStyleDeclaration::checkInputErrorBackgroundColor(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkInputErrorColor(tokens);
}

bool CSSStyleDeclaration::checkInputErrorBackgroundSize(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // [length | percentage | auto]{1, 2} | cover | contain // initial value -> auto
    if (tokens->size() == 1) {
        const char* token = (*tokens)[0]->utf8Data();
        if (strcmp(token, "cover") == 0 || strcmp(token, "contain") == 0 || strcmp(token, "initial") == 0 || strcmp(token, "inherit") == 0 || CSSPropertyParser::assureLength(token, false) || CSSPropertyParser::assurePercent(token, false)) {
            return true;
        }
    } else if (tokens->size() == 2) {
        for (unsigned int i = 0; i < tokens->size(); i++) {
            const char* token = (*tokens)[i]->utf8Data();
            if (!(CSSPropertyParser::assureLength(token, false) || CSSPropertyParser::assurePercent(token, false))) {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool CSSStyleDeclaration::checkInputErrorLetterSpacing(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() != 1)
        return false;
    (*tokens)[0] = (*tokens)[0]->toLower();
    const char* value = (*tokens)[0]->utf8Data();
    return (VALUE_IS_STRING("normal")
        || CSSPropertyParser::assureEssential(value)
        || CSSPropertyParser::assureLength(value, false));
}

bool CSSStyleDeclaration::checkInputErrorLineHeight(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() != 1)
        return false;
    (*tokens)[0] = (*tokens)[0]->toLower();
    const char* value = (*tokens)[0]->utf8Data();
    return (VALUE_IS_STRING("normal")
        || CSSPropertyParser::assureEssential(value)
        || CSSPropertyParser::assureLength(value, false)
        || CSSPropertyParser::assurePercent(value, false)
        || CSSPropertyParser::assureNumber(value, false));
}

bool CSSStyleDeclaration::checkInputErrorPaddingTop(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() != 1)
        return false;
    (*tokens)[0] = (*tokens)[0]->toLower();
    const char* value = (*tokens)[0]->utf8Data();
    return (CSSPropertyParser::assureEssential(value)
        || CSSPropertyParser::assureLength(value, false)
        || CSSPropertyParser::assurePercent(value, false));
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

bool CSSStyleDeclaration::checkInputErrorMarginTop(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkHavingOneTokenAndLengthOrPercentage(tokens, true);
}

bool CSSStyleDeclaration::checkInputErrorBackgroundImage(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() == 1) {
        (*tokens)[0] = (*tokens)[0]->toLower();
        const char* token = tokens->at(0)->utf8Data();
        size_t len = strlen(token);

        if (startsWith(token, "url(") && endsWith(token, ")")) {
            if ((token[4] == '\"' || token[4] == '\'') && (token[4] != token[len - 2])) {
                return false;
            }
            return true;
        } else if (CSSPropertyParser::assureEssential(token)
            || strcmp(token, "none") == 0 || strcmp(token, "") == 0) {
            return true;
        }
    } else if (tokens->size() == 2) {
        const char* token0 = tokens->at(0)->utf8Data();
        const char* token1 = tokens->at(1)->utf8Data();

        if (startsWith(token0, "url(") && endsWith(token1, ")")) {
            size_t len0 = strlen(token0);
            size_t len1 = strlen(token1);

            if (len0 > 4 && (token0[4] == '\"' || token0[4] == '\'') && (token0[4] != token0[len0 - 1])) {
                return false;
            } else if (len1 > 1 && (token1[0] == '\"' || token1[0] == '\'') && (token1[0] != token1[len1 - 2])) {
                return false;
            }
            return true;
        }
    } else if (tokens->size() == 3) {
        const char* token0 = tokens->at(0)->utf8Data();
        const char* token1 = tokens->at(1)->utf8Data();
        const char* token2 = tokens->at(2)->utf8Data();

        if ((startsWith(token0, "url(") && strlen(token0) == 4) && (endsWith(token2, ")") && strlen(token2) == 1)) {
            size_t len1 = strlen(token1);

            if ((token1[0] == '\"' || token1[0] == '\'') && (token1[0] != token1[len1 - 1])) {
                return false;
            }
            return true;
        }
    }

    return false;
}

bool CSSStyleDeclaration::checkInputErrorBackgroundRepeatX(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return true;
}
bool CSSStyleDeclaration::checkInputErrorBackgroundRepeatY(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return true;
}
bool CSSStyleDeclaration::checkInputErrorBackgroundRepeat(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() != 1)
        return false;
    (*tokens)[0] = (*tokens)[0]->toLower();
    const char* value = (*tokens)[0]->utf8Data();
    return VALUE_IS_STRING("repeat")
        || VALUE_IS_STRING("repeat-x")
        || VALUE_IS_STRING("repeat-y")
        || VALUE_IS_STRING("no-repeat")
        || CSSPropertyParser::assureEssential(value)
        || strcmp(value, "") == 0;
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
    // length | percentage | <auto> | inherit
    for (unsigned i = 0; i < tokens->size(); i++) {
        const char* token = tokens->at(i)->utf8Data();
        if (!(CSSPropertyParser::assureLength(token, true) || CSSPropertyParser::assurePercent(token, true) || (strcmp(token, "auto") == 0) || (strcmp(token, "initial") == 0) || (strcmp(token, "inherit") == 0))) {
            return false;
        }
    }
    return true;
}

bool CSSStyleDeclaration::checkHavingOneTokenAndLengthOrPercentage(std::vector<String*, gc_allocator<String*> >* tokens, bool allowNegative)
{
    // length | percentage | <auto> | inherit
    if (tokens->size() == 1) {
        (*tokens)[0] = (*tokens)[0]->toLower();
        const char* token = (*tokens)[0]->utf8Data();
        if (CSSPropertyParser::assureLength(token, allowNegative) || CSSPropertyParser::assurePercent(token, allowNegative) || (strcmp(token, "auto") == 0) || (strcmp(token, "initial") == 0) || (strcmp(token, "inherit") == 0)) {
            return true;
        }
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

bool CSSStyleDeclaration::checkInputErrorDirection(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // <ltr> | rtl | inherit
    if (tokens->size() == 1) {
        (*tokens)[0] = (*tokens)[0]->toLower();
        const char* token = (*tokens)[0]->utf8Data();
        if ((strcmp(token, "ltr") == 0) || (strcmp(token, "rtl") == 0) || (strcmp(token, "inherit") == 0)) {
            return true;
        }
    }
    return false;
}

bool CSSStyleDeclaration::checkInputErrorWidth(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkHavingOneTokenAndLengthOrPercentage(tokens, true);
}

bool CSSStyleDeclaration::checkInputErrorHeight(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkHavingOneTokenAndLengthOrPercentage(tokens, true);
}

bool CSSStyleDeclaration::checkInputErrorFontSize(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() == 1) {
        const char* token = (*tokens)[0]->toLower()->utf8Data();
        if (CSSPropertyParser::assureLength(token, false))
            return true;

        if ((strcmp(token, "xx-small") == 0) || (strcmp(token, "x-small") == 0) || (strcmp(token, "small") == 0) || (strcmp(token, "medium") == 0) || (strcmp(token, "large") == 0) || (strcmp(token, "x-large") == 0) || (strcmp(token, "xx-large") == 0) || (strcmp(token, "larger") == 0) || (strcmp(token, "smaller") == 0) || (strcmp(token, "inherit") == 0) || (strcmp(token, "init") == 0)) {
            return true;
        }
    }
    return false;
}

bool CSSStyleDeclaration::checkInputErrorFontStyle(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() == 1) {
        const char* token = (*tokens)[0]->toLower()->utf8Data();
        if ((strcmp(token, "normal") == 0) || (strcmp(token, "italic") == 0) || (strcmp(token, "oblique") == 0) || (strcmp(token, "inherit") == 0) || (strcmp(token, "init") == 0)) {
            return true;
        }
    }
    return false;
}

bool CSSStyleDeclaration::checkInputErrorFontWeight(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() == 1) {
        const char* token = (*tokens)[0]->toLower()->utf8Data();
        if ((strcmp(token, "normal") == 0) || (strcmp(token, "bold") == 0) || (strcmp(token, "bolder") == 0) || (strcmp(token, "lighter") == 0) || (strcmp(token, "100") == 0) || (strcmp(token, "200") == 0) || (strcmp(token, "300") == 0) || (strcmp(token, "400") == 0) || (strcmp(token, "500") == 0) || (strcmp(token, "600") == 0) || (strcmp(token, "700") == 0) || (strcmp(token, "800") == 0) || (strcmp(token, "900") == 0) || (strcmp(token, "inherit") == 0) || (strcmp(token, "initial") == 0)) {
            return true;
        } else if (CSSPropertyParser::assureInteger(token, false)) {
            int num = 0;
            sscanf(token, "%d", &num);

            if (((num % 100) == 0) && (((num / 100) >= 1) && ((num / 100) <= 9))) {
                char tmp[4] = {
                    0,
                };
                sprintf(tmp, "%d", num);
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
        const char* token = (*tokens)[0]->toLower()->utf8Data();
        if ((strcmp(token, "inline") == 0) || (strcmp(token, "block") == 0) || (strcmp(token, "inline-block") == 0) || (strcmp(token, "none") == 0) || (strcmp(token, "inherit") == 0) || (strcmp(token, "init") == 0)) {
            return true;
        }
    }
    return false;
}

bool CSSStyleDeclaration::checkInputErrorPosition(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() == 1) {
        const char* token = (*tokens)[0]->toLower()->utf8Data();
        if ((strcmp(token, "static") == 0) || (strcmp(token, "relative") == 0) || (strcmp(token, "absolute") == 0) || (strcmp(token, "fixed") == 0) || (strcmp(token, "inherit") == 0)) {
            return true;
        }
    }
    return false;
}

bool CSSStyleDeclaration::checkInputErrorTextDecoration(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() == 1) {
        const char* token = (*tokens)[0]->toLower()->utf8Data();
        if ((strcmp(token, "none") == 0) || (strcmp(token, "underline") == 0) || (strcmp(token, "overline") == 0) || (strcmp(token, "line-through") == 0) || (strcmp(token, "blink") == 0)) {
            return true;
        }
    }
    return false;
}

bool CSSStyleDeclaration::checkInputErrorBorderImageRepeat(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() == 1 || tokens->size() == 2) {
        for (unsigned int i = 0; i < tokens->size(); i++) {
            const char* token = (*tokens)[i]->toLower()->utf8Data();
            if ((strcmp(token, "stretch") == 0) || (strcmp(token, "repeat") == 0) || (strcmp(token, "round") == 0) || (strcmp(token, "space") == 0)) {
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
        const char* currentToken = tokens->at(i)->toLower()->utf8Data();
        if (CSSPropertyParser::assureNumber(currentToken, false)) {
            isNum = true;
        } else if (strcmp(currentToken, "fill") == 0) {
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

bool CSSStyleDeclaration::checkInputErrorBorderImageSource(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // none | <image>(=<uri>)
    if (tokens->size() == 1) {
        const char* value = (*tokens)[0]->toLower()->utf8Data();
        if (CSSPropertyParser::assureEssential(value)
            || strcmp(value, "none") == 0)
            return true;
    }
    // url
    return CSSPropertyParser::assureUrl(tokens, 0, tokens->size());
}

bool CSSStyleDeclaration::checkInputErrorBorderImageWidth(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // [ <length> | <number> ]
    if (tokens->size() != 1)
        return false;

    const char* currentToken = tokens->at(0)->toLower()->utf8Data();
    if (CSSPropertyParser::assureLength(currentToken, false) || CSSPropertyParser::assureNumber(currentToken, false))
        return true;

    return false;
}

bool CSSStyleDeclaration::checkInputErrorBorder(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // [border-width || border-style || border-color] | inherit | initial
    if (tokens->size() == 1 && CSSPropertyParser::assureEssential(tokens->at(0)->toLower()->utf8Data())) {
        return true;
    }

    bool hasWidth = false, hasStyle = false, hasColor = false;
    if (tokens->size() > 0 && tokens->size() <= 3) {
        for (unsigned i = 0; i < tokens->size(); i++) {
            switch (checkBorderValueType(tokens->at(i)->toLower()->utf8Data())) {
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

bool checkInputErrorBorderUnitColor(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // color | transparent | inherit
    if (tokens->size() == 1) {
        const char* token = tokens->at(0)->toLower()->utf8Data();
        if (CSSPropertyParser::assureColor(token) || (strcmp(token, "transparent") == 0) || (strcmp(token, "initial") == 0) || (strcmp(token, "inherit") == 0)) {
            return true;
        }
    }
    return false;
}

bool CSSStyleDeclaration::checkInputErrorBorderTopColor(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkInputErrorBorderUnitColor(tokens);
}

bool CSSStyleDeclaration::checkInputErrorBorderRightColor(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkInputErrorBorderUnitColor(tokens);
}

bool CSSStyleDeclaration::checkInputErrorBorderBottomColor(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkInputErrorBorderUnitColor(tokens);
}

bool CSSStyleDeclaration::checkInputErrorBorderLeftColor(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return checkInputErrorBorderUnitColor(tokens);
}

bool checkInputErrorBorderUnitStyle(std::vector<String*, gc_allocator<String*> >* tokens)
{
    // border-style(<none> | solid) | inherit
    if (tokens->size() == 1) {
        const char* token = tokens->at(0)->utf8Data();
        if ((strcmp(token, "none") == 0) || (strcmp(token, "solid") == 0) || (strcmp(token, "initial") == 0) || (strcmp(token, "inherit") == 0)) {
            return true;
        }
    }
    return false;
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
        if ((strcmp(token, "thin") == 0) || (strcmp(token, "medium") == 0) || (strcmp(token, "thick") == 0) || (strcmp(token, "inherit") == 0) || (strcmp(token, "initial") == 0) || CSSPropertyParser::assureLength(token, false)) {
            return true;
        }
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

bool CSSStyleDeclaration::checkInputErrorTextAlign(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() == 1) {
        const char* token = (*tokens)[0]->toLower()->utf8Data();
        if ((strcmp(token, "left") == 0) || (strcmp(token, "right") == 0) || (strcmp(token, "center") == 0) || (strcmp(token, "justify") == 0)) {
            return true;
        }
    }
    return false;
}

bool CSSStyleDeclaration::checkInputErrorVisibility(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() == 1) {
        const char* token = (*tokens)[0]->toLower()->utf8Data();
        if ((strcmp(token, "visible") == 0) || (strcmp(token, "hidden") == 0)) {
            return true;
        }
    }
    return false;
}

bool CSSStyleDeclaration::checkInputErrorOpacity(std::vector<String*, gc_allocator<String*> >* tokens)
{
    return true;
}

bool CSSStyleDeclaration::checkInputErrorOverflowX(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() == 1) {
        const char* token = (*tokens)[0]->toLower()->utf8Data();
        if ((strcmp(token, "visible") == 0) || (strcmp(token, "hidden") == 0)) {
            return true;
        }
    }
    return false;
}
bool CSSStyleDeclaration::checkInputErrorZIndex(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() == 1) {
        const char* token = tokens->at(0)->utf8Data();
        if ((strcmp(token, "auto") == 0) || (strcmp(token, "initial") == 0) || (strcmp(token, "inherit") == 0) || CSSPropertyParser::assureInteger(token, false)) {
            return true;
        }
    }
    return false;
}

bool CSSStyleDeclaration::checkInputErrorVerticalAlign(std::vector<String*, gc_allocator<String*> >* tokens)
{
    if (tokens->size() == 1) {
        const char* token = (*tokens)[0]->toLower()->utf8Data();
        if ((strcmp(token, "baseline") == 0) || (strcmp(token, "sub") == 0) || (strcmp(token, "super") == 0) || (strcmp(token, "top") == 0) || (strcmp(token, "text-top") == 0) || (strcmp(token, "middle") == 0) || (strcmp(token, "bottom") == 0) || (strcmp(token, "text-bottom") == 0)) {
            return true;
        } else if (CSSPropertyParser::assureLength(token, true) || CSSPropertyParser::assurePercent(token, true)) {
            return true;
        }
    }
    return false;
}

bool CSSStyleDeclaration::checkInputErrorTransform(std::vector<String*, gc_allocator<String*> >* tokens)
{
    const char* str0 = tokens->at(0)->utf8Data();
    if (tokens->size() == 1) {
        if (CSSPropertyParser::assureEssential(str0) || strcmp(str0, "none") == 0)
            return true;
    }

    String* str = String::emptyString;
    for (unsigned i = 0; i< tokens->size(); i++)
        str = str->concat(tokens->at(i)->toLower());
    tokens->clear();
    str->split(')', *tokens);
    for (unsigned i = 0; i < tokens->size(); i++) {
        str0 = tokens->at(i)->utf8Data();
        if (startsWith(str0, "matrix(")) {
            // NOTE: strlen("matrix(") => 7
            if (!CSSPropertyParser::assureNumberList(str0 + 7, true, 6, 6))
                return false;
        } else if (startsWith(str0, "translate(")) {
            if (!CSSPropertyParser::assureLengthOrPercentList(str0 + 11, true, 1, 2))
                return false;
        } else if (startsWith(str0, "translatex(")) {
            if (!CSSPropertyParser::assureLengthOrPercent(str0 + 11, true))
                return false;
        } else if (startsWith(str0, "translatey(")) {
            if (!CSSPropertyParser::assureLengthOrPercent(str0 + 11, true))
                return false;
        } else if (startsWith(str0, "translatez(")) {
            if (!CSSPropertyParser::assureLengthOrPercent(str0 + 11, true))
                return false;
        } else if (startsWith(str0, "scale(")) {
            if (!CSSPropertyParser::assureNumberList(str0 + 6, true, 1, 2))
                return false;
        } else if (startsWith(str0, "scalex(")) {
            if (!CSSPropertyParser::assureNumber(str0 + 7, true))
                return false;
        } else if (startsWith(str0, "scaley(")) {
            if (!CSSPropertyParser::assureNumber(str0 + 7, true))
                return false;
        } else if (startsWith(str0, "rotate(")) {
            if (!CSSPropertyParser::assureAngle(str0 + 7))
                return false;
        } else if (startsWith(str0, "skew(")) {
            if (!CSSPropertyParser::assureAngleList(str0 + 5, 1, 2))
                return false;
        } else if (startsWith(str0, "skewx(")) {
            if (!CSSPropertyParser::assureAngle(str0 + 6))
                return false;
        } else if (startsWith(str0, "skewy(")) {
            if (!CSSPropertyParser::assureAngle(str0 + 6))
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
            value = (*tokens)[i]->toLower()->utf8Data();
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

void CSSStyleDeclaration::notifyNeedsStyleRecalc()
{
    if (m_element)
        m_element->setNeedsStyleRecalc();
}

ComputedStyle* StyleResolver::resolveDocumentStyle(StarFish* sf)
{
    ComputedStyle* ret = new ComputedStyle();
    ret->m_display = DisplayValue::BlockDisplayValue;
    ret->m_inheritedStyles.m_color = Color(0, 0, 0, 255);
    // TODO implement ltr, rtl
    ret->m_inheritedStyles.m_textAlign = TextAlignValue::LeftTextAlignValue;
    ret->m_inheritedStyles.m_direction = DirectionValue::LtrDirectionValue;
    ret->loadResources(sf);
    return ret;
}

Color parseColor(String* str)
{
    const char* s = str->utf8Data();
    if (startsWith(s, "rgba")) {
        float r, g, b, a;
        sscanf(s, "rgba(%f,%f,%f,%f)", &r, &g, &b, &a);
        return Color(r, g, b, a * 255);
    } else if (startsWith(s, "rgb")) {
        float r, g, b;
        sscanf(s, "rgb(%f,%f,%f)", &r, &g, &b);
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

    auto apply = [](std::vector<CSSStyleValuePair, gc_allocator<CSSStyleValuePair> >& cssValues, ComputedStyle* style, ComputedStyle* parentStyle)
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
                    style->m_inheritedStyles.m_color = parseColor(cssValues[k].stringValue());
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
            case CSSStyleValuePair::KeyKind::LetterSpacing:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setLetterSpacing(parentStyle->letterSpacing());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial
                    || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Normal) {
                    style->setLetterSpacing(Length(Length::Fixed, 0));
                } else {
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Length);
                    style->setLetterSpacing(cssValues[k].lengthValue().toLength());
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
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial
                    || cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Transparent) {
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
                    style->setBackgroundImage(cssValues[k].urlValue());
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
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Transparent) {
                    style->setBorderTopColor(Color(0, 0, 0, 0));
                } else {
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::StringValueKind);
                    style->setBorderTopColor(parseColor(cssValues[k].stringValue()));
                }
                break;
            case CSSStyleValuePair::KeyKind::BorderRightColor:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setBorderRightColor(parentStyle->borderRightColor());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    // This is done after resolve style (arrangeStyleValues function in ComputedStyle Class)
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Transparent) {
                    style->setBorderRightColor(Color(0, 0, 0, 0));
                } else {
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::StringValueKind);
                    style->setBorderRightColor(parseColor(cssValues[k].stringValue()));
                }
                break;
            case CSSStyleValuePair::KeyKind::BorderBottomColor:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setBorderBottomColor(parentStyle->borderBottomColor());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    // This is done after resolve style (arrangeStyleValues function in ComputedStyle Class)
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Transparent) {
                    style->setBorderBottomColor(Color(0, 0, 0, 0));
                } else {
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::StringValueKind);
                    style->setBorderBottomColor(parseColor(cssValues[k].stringValue()));
                }
                break;
            case CSSStyleValuePair::KeyKind::BorderLeftColor:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->setBorderLeftColor(parentStyle->borderLeftColor());
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    // This is done after resolve style (arrangeStyleValues function in ComputedStyle Class)
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Transparent) {
                    style->setBorderLeftColor(Color(0, 0, 0, 0));
                } else {
                    STARFISH_ASSERT(cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::StringValueKind);
                    style->setBorderLeftColor(parseColor(cssValues[k].stringValue()));
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
                    style->setBorderImageSource(cssValues[k].urlValue());
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
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Number) {
                    // The computed value should be same as the specified value.
                    style->setLineHeight(Length(Length::EmToBeFixed, cssValues[k].numberValue()));
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
            case CSSStyleValuePair::KeyKind::OverflowX:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->m_overflowX = parentStyle->overflowX();
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->m_overflowX = OverflowValue::VisibleOverflow;
                } else {
                    style->m_overflowX = cssValues[k].overflowValueX();
                }
                break;
/*            case CSSStyleValuePair::KeyKind::OverflowY:
                if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
                    style->m_overflowY = parentStyle->m_overflowY;
                } else if (cssValues[k].valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    // TODO : should assign initial value
                } else {
                    style->m_overflowY = cssValues[k].overflowValueY();
                }
                break;
*/
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
                    style->m_zIndex = cssValues[k].numberValue();
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
            }
        }
    };

    // first sheet is must user-agent style sheet!
    for (unsigned i = 0; i < m_sheets.size(); i++) {
        CSSStyleSheet* sheet = m_sheets[i];

        // * selector
        for (unsigned j = 0; j < sheet->m_rules.size(); j++) {
            if (sheet->m_rules[j]->m_kind == CSSStyleRule::UniversalSelector && sheet->m_rules[j]->m_pseudoClass == CSSStyleRule::PseudoClass::None) {
                auto cssValues = sheet->m_rules[j]->styleDeclaration()->m_cssValues;
                apply(cssValues, ret, parent);
            }
        }

        // type selector
        for (unsigned j = 0; j < sheet->m_rules.size(); j++) {
            if (sheet->m_rules[j]->m_kind == CSSStyleRule::TypeSelector && sheet->m_rules[j]->m_pseudoClass == CSSStyleRule::PseudoClass::None) {
                if (sheet->m_rules[j]->m_ruleText[0]->equals(element->localName())) {
                    auto cssValues = sheet->m_rules[j]->styleDeclaration()->m_cssValues;
                    apply(cssValues, ret, parent);
                }
            }
        }

        // class selector
        for (unsigned j = 0; j < sheet->m_rules.size(); j++) {
            if (sheet->m_rules[j]->m_kind == CSSStyleRule::ClassSelector && sheet->m_rules[j]->m_pseudoClass == CSSStyleRule::PseudoClass::None) {
                auto className = element->classNames();
                for (unsigned f = 0; f < className.size(); f++) {
                    if (className[f]->equalsWithoutCase(sheet->m_rules[j]->m_ruleText[0])) {
                        auto cssValues = sheet->m_rules[j]->styleDeclaration()->m_cssValues;
                        apply(cssValues, ret, parent);
                    }
                }
            }
        }

        // type.class selector
        for (unsigned j = 0; j < sheet->m_rules.size(); j++) {
            if (sheet->m_rules[j]->m_kind == CSSStyleRule::TypeClassSelector && sheet->m_rules[j]->m_pseudoClass == CSSStyleRule::PseudoClass::None) {
                if (element->localName()->equals(sheet->m_rules[j]->m_ruleText[0])) {
                    auto className = element->classNames();
                    for (unsigned f = 0; f < className.size(); f++) {
                        if (className[f]->equalsWithoutCase(sheet->m_rules[j]->m_ruleText[1])) {
                            auto cssValues = sheet->m_rules[j]->styleDeclaration()->m_cssValues;
                            apply(cssValues, ret, parent);
                        }
                    }
                }
            }
        }

        // id selector
        for (unsigned j = 0; j < sheet->m_rules.size(); j++) {
            if (sheet->m_rules[j]->m_kind == CSSStyleRule::IdSelector && sheet->m_rules[j]->m_pseudoClass == CSSStyleRule::PseudoClass::None) {
                if (element->id()->equalsWithoutCase(sheet->m_rules[j]->m_ruleText[0])) {
                    auto cssValues = sheet->m_rules[j]->styleDeclaration()->m_cssValues;
                    apply(cssValues, ret, parent);
                }
            }
        }

        // type#id selector
        for (unsigned j = 0; j < sheet->m_rules.size(); j++) {
            if (sheet->m_rules[j]->m_kind == CSSStyleRule::TypeIdSelector && sheet->m_rules[j]->m_pseudoClass == CSSStyleRule::PseudoClass::None) {
                if (element->localName()->equals(sheet->m_rules[j]->m_ruleText[0])) {
                    if (element->id()->equalsWithoutCase(sheet->m_rules[j]->m_ruleText[1])) {
                        auto cssValues = sheet->m_rules[j]->styleDeclaration()->m_cssValues;
                        apply(cssValues, ret, parent);
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
            for (unsigned j = 0; j < sheet->m_rules.size(); j++) {
                if (sheet->m_rules[j]->m_kind == CSSStyleRule::UniversalSelector && sheet->m_rules[j]->m_pseudoClass == CSSStyleRule::PseudoClass::Active) {
                    auto cssValues = sheet->m_rules[j]->styleDeclaration()->m_cssValues;
                    apply(cssValues, ret, parent);
                }
            }
        }

        // type:active selector
        if ((element->state() & Node::NodeState::NodeStateActive)) {
            for (unsigned j = 0; j < sheet->m_rules.size(); j++) {
                if (sheet->m_rules[j]->m_kind == CSSStyleRule::TypeSelector && sheet->m_rules[j]->m_pseudoClass == CSSStyleRule::PseudoClass::Active) {
                    if (sheet->m_rules[j]->m_ruleText[0]->equals(element->localName())) {
                        auto cssValues = sheet->m_rules[j]->styleDeclaration()->m_cssValues;
                        apply(cssValues, ret, parent);
                    }
                }
            }
        }

        // class:active selector
        if ((element->state() & Node::NodeState::NodeStateActive)) {
            for (unsigned j = 0; j < sheet->m_rules.size(); j++) {
                if (sheet->m_rules[j]->m_kind == CSSStyleRule::ClassSelector && sheet->m_rules[j]->m_pseudoClass == CSSStyleRule::PseudoClass::Active) {
                    auto className = element->classNames();
                    for (unsigned f = 0; f < className.size(); f++) {
                        if (className[f]->equalsWithoutCase(sheet->m_rules[j]->m_ruleText[0])) {
                            auto cssValues = sheet->m_rules[j]->styleDeclaration()->m_cssValues;
                            apply(cssValues, ret, parent);
                        }
                    }
                }
            }
        }

        // type.class:active selector
        for (unsigned j = 0; j < sheet->m_rules.size(); j++) {
            if (sheet->m_rules[j]->m_kind == CSSStyleRule::TypeClassSelector && sheet->m_rules[j]->m_pseudoClass == CSSStyleRule::PseudoClass::Active) {
                if (element->localName()->equals(sheet->m_rules[j]->m_ruleText[0])) {
                    auto className = element->classNames();
                    for (unsigned f = 0; f < className.size(); f++) {
                        if (className[f]->equalsWithoutCase(sheet->m_rules[j]->m_ruleText[1])) {
                            auto cssValues = sheet->m_rules[j]->styleDeclaration()->m_cssValues;
                            apply(cssValues, ret, parent);
                        }
                    }
                }
            }
        }

        // id:active selector
        if ((element->state() & Node::NodeState::NodeStateActive)) {
            for (unsigned j = 0; j < sheet->m_rules.size(); j++) {
                if (sheet->m_rules[j]->m_kind == CSSStyleRule::IdSelector && sheet->m_rules[j]->m_pseudoClass == CSSStyleRule::PseudoClass::Active) {
                    if (element->id()->equalsWithoutCase(sheet->m_rules[j]->m_ruleText[0])) {
                        auto cssValues = sheet->m_rules[j]->styleDeclaration()->m_cssValues;
                        apply(cssValues, ret, parent);
                    }
                }
            }
        }

        // type#id:active selector
        for (unsigned j = 0; j < sheet->m_rules.size(); j++) {
            if (sheet->m_rules[j]->m_kind == CSSStyleRule::TypeIdSelector && sheet->m_rules[j]->m_pseudoClass == CSSStyleRule::PseudoClass::Active) {
                if (element->localName()->equals(sheet->m_rules[j]->m_ruleText[0])) {
                    if (element->id()->equalsWithoutCase(sheet->m_rules[j]->m_ruleText[1])) {
                        auto cssValues = sheet->m_rules[j]->styleDeclaration()->m_cssValues;
                        apply(cssValues, ret, parent);
                    }
                }
            }
        }
    }

    // inline style
    auto inline_cssValues = element->inlineStyle()->m_cssValues;
    if (inline_cssValues.size() > 0)
        apply(inline_cssValues, ret, parent);

    ret->loadResources(element->document()->window()->starFish());
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
                        childStyle->loadResources(element->document()->window()->starFish());
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

void dump(Node* node, unsigned depth)
{
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
}
