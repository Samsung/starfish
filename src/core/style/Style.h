/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishStyle__
#define __StarFishStyle__

#include "binding/DocumentHoldable.h"
#include "core/style/NamedColors.h"
#include "core/style/MediaQueryEvaluator.h"
#include "core/util/VectorWithInlineStorage.h"

namespace StarFish {

class CalcData;
class ComputedStyle;
class StyleRule;
class StyleRuleImport;
class StyleRuleBase;
class Document;
class Element;
class MediaQuerySet;
class MediaQueryEvaluator;
class Node;

class CSSTokenValue : public std::string {
public:
    CSSTokenValue()
        : std::string()
    {
    }

    CSSTokenValue(const char* str)
        : std::string(str)
    {
    }

    CSSTokenValue(const char* str, size_t len)
        : std::string(str, len)
    {
    }

    CSSTokenValue(std::string&& str)
        : std::string(std::move(str))
    {
    }

    bool startsWith(const char* str) const
    {
        if (std::string::find(str) == 0) {
            return true;
        }
        return false;
    }

    size_t indexOf(char ch) const
    {
        return std::string::find(ch);
    }

    bool equals(const char* str) const
    {
        size_t srcLen = strlen(str);
        if (length() != srcLen) {
            return false;
        }
        for (size_t i = 0; i < length(); i++) {
            char c = str[i];
            if (c != std::string::operator[](i)) {
                return false;
            }
        }
        return true;
    }

    CSSTokenValue substring(size_t pos, size_t len) const
    {
        return std::string::substr(pos, len);
    }

    void split(const char delim, std::vector<CSSTokenValue>& tokens) const
    {
        size_t prev_pos = 0, pos = 0;
        while ((pos = find(delim, pos)) != SIZE_MAX) {
            tokens.push_back(
                CSSTokenValue(&std::string::data()[prev_pos], pos - prev_pos));
            prev_pos = ++pos;
        }

        if (pos == SIZE_MAX)
            pos = length();

        tokens.push_back(CSSTokenValue(&std::string::data()[prev_pos],
                                       pos - prev_pos)); // Last word
    }

    char charAt(size_t i) const
    {
        return std::string::operator[](i);
    }

    CSSTokenValue trim() const
    {
        size_t first = 0;
        size_t last = 0;
        if (length()) {
            last = length() - 1;

            for (size_t i = 0; i < length(); i++) {
                if (!String::isSpaceOrNewline(charAt(i))) {
                    first = i;
                    break;
                }
            }

            do {
                if (!String::isSpaceOrNewline(charAt(last))) {
                    break;
                }
            } while (last--);
        } else {
            return *this;
        }

        if (first == 0 && ((last + 1) == length())) {
            return *this;
        }

        return substring(first, (last - first + 1));
    }
};
typedef VectorWithInlineStorage<4, CSSTokenValue, std::allocator<CSSTokenValue>>
    CSSTokenVector;

enum UnitType {
    UnknownType,
    Number,
    Percentage,
    // Length units
    Ems,
    Exs,
    Pixels,
    Centimeters,
    Millimeters,
    Inches,
    Points,
    Picas,
    ViewportWidth,
    ViewportHeight,
    ViewportMin,
    ViewportMax,
    Rems,
    Chs,
    UserUnits, // The SVG term for unitless lengths
    // Angle units
    Degrees,
    Radians,
    Gradians,
    Turns,
    // Time units
    Milliseconds,
    Seconds,
    Hertz,
    Kilohertz,
    // Resolution
    DotsPerPixel,
    DotsPerInch,
    DotsPerCentimeter,
    // Other units
    Fraction,
    Integer,
    Calc,
    CalcPercentageWithNumber,
    CalcPercentageWithLength,
    ValueID,
};

// https://www.w3.org/TR/CSS21/syndata.html#value-def-length
class CSSLength {
public:
    enum Kind { PX, EM, EX, IN, CM, MM, PT, PC, VW, VH, VMIN, VMAX, REM };

    CSSLength(float f)
    {
        m_kind = PX;
        m_value = f;
    }

    CSSLength(Kind kind, float f)
    {
        m_kind = kind;
        m_value = f;
    }

    CSSLength(String* unit, float f)
    {
        if (unit->length() == 0 || unit->equals("px")) {
            m_kind = PX;
        } else if (unit->equals("em")) {
            m_kind = EM;
        } else if (unit->equals("ex")) {
            m_kind = EX;
        } else if (unit->equals("in")) {
            m_kind = IN;
        } else if (unit->equals("cm")) {
            m_kind = CM;
        } else if (unit->equals("mm")) {
            m_kind = MM;
        } else if (unit->equals("pt")) {
            m_kind = PT;
        } else if (unit->equals("pc")) {
            m_kind = PC;
        } else if (unit->equals("vw")) {
            m_kind = VW;
        } else if (unit->equals("vh")) {
            m_kind = VH;
        } else if (unit->equals("vmin")) {
            m_kind = VMIN;
        } else if (unit->equals("vmax")) {
            m_kind = VMAX;
        } else if (unit->equals("rem")) {
            m_kind = REM;
        }

        m_value = f;
    }

    Kind kind() const
    {
        return m_kind;
    }

    float value() const
    {
        return m_value;
    }

    Length toLength() const
    {
        if (m_kind == PX) { // absolute length
            return Length(Length::Fixed, m_value);
        } else if (m_kind == CM) {
            return Length(Length::Fixed, convertFromCmToPx(m_value));
        } else if (m_kind == MM) {
            return Length(Length::Fixed, convertFromMmToPx(m_value));
        } else if (m_kind == IN) {
            return Length(Length::Fixed, convertFromInToPx(m_value));
        } else if (m_kind == PC) {
            return Length(Length::Fixed, convertFromPcToPx(m_value));
        } else if (m_kind == PT) {
            return Length(Length::Fixed, convertFromPtToPx(m_value));
        } else if (m_kind == EM) { // font-relative length
            return Length(Length::Em, m_value);
        } else if (m_kind == EX) { // font-relative length
            return Length(Length::Ex, m_value);
        } else if (m_kind == VW) {
            return Length(Length::Vw, m_value);
        } else if (m_kind == VH) {
            return Length(Length::Vh, m_value);
        } else if (m_kind == VMIN) {
            return Length(Length::Vmin, m_value);
        } else if (m_kind == VMAX) {
            return Length(Length::Vmax, m_value);
        } else if (m_kind == REM) { // font-relative length
            return Length(Length::Rem, m_value);
        }

        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    String* toString() const
    {
        UTF8StringDataNonGCStd stdStr =
            String::fromFloat(m_value)->toUTF8NonGCString();
        if (m_kind == PX) {
            return String::fromUTF8(stdStr.append("px").c_str());
        } else if (m_kind == CM) {
            return String::fromUTF8(stdStr.append("cm").c_str());
        } else if (m_kind == MM) {
            return String::fromUTF8(stdStr.append("mm").c_str());
        } else if (m_kind == IN) {
            return String::fromUTF8(stdStr.append("in").c_str());
        } else if (m_kind == PC) {
            return String::fromUTF8(stdStr.append("pc").c_str());
        } else if (m_kind == PT) {
            return String::fromUTF8(stdStr.append("pt").c_str());
        } else if (m_kind == EM) {
            return String::fromUTF8(stdStr.append("em").c_str());
        } else if (m_kind == EX) {
            return String::fromUTF8(stdStr.append("ex").c_str());
        } else if (m_kind == VW) {
            return String::fromUTF8(stdStr.append("vw").c_str());
        } else if (m_kind == VH) {
            return String::fromUTF8(stdStr.append("vh").c_str());
        } else if (m_kind == VMIN) {
            return String::fromUTF8(stdStr.append("vmin").c_str());
        } else if (m_kind == VMAX) {
            return String::fromUTF8(stdStr.append("vmax").c_str());
        } else if (m_kind == REM) {
            return String::fromUTF8(stdStr.append("rem").c_str());
        }
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

protected:
    Kind m_kind;
    float m_value;
};

inline CSSLength operator*(const CSSLength& a, const float b)
{
    return CSSLength(a.value() * b);
}

inline CSSLength operator*(const float a, const CSSLength& b)
{
    return CSSLength(a * b.value());
}

// https://www.w3.org/TR/css3-values/#angles
class CSSAngle {
public:
    enum Kind { DEG, GRAD, RAD, TURN };

    CSSAngle()
    {
        m_kind = DEG;
        m_value = 0;
    }

    CSSAngle(float f)
    {
        m_kind = DEG;
        m_value = f;
    }

    CSSAngle(Kind kind, float f)
    {
        m_kind = kind;
        m_value = f;
    }

    CSSAngle(String* str, float f)
    {
        if (str->length() == 0 || str->equals("deg")) {
            m_kind = DEG;
        } else if (str->equals("grad")) {
            m_kind = GRAD;
        } else if (str->equals("rad")) {
            m_kind = RAD;
        } else if (str->equals("turn")) {
            m_kind = TURN;
        }
        m_value = f;
    }

    Kind kind() const
    {
        return m_kind;
    }

    float value() const
    {
        return m_value;
    }

    float toDegreeValue() const
    {
        if (m_kind == DEG) {
            return m_value;
        } else if (m_kind == RAD) {
            return convertFromRadToDeg(m_value);
        } else if (m_kind == GRAD) {
            return convertFromGradToDeg(m_value);
        } else if (m_kind == TURN) {
            return convertFromTurnToDeg(m_value);
        }

        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    String* toString() const
    {
        std::stringstream ss(std::stringstream::in | std::stringstream::out);
        ss << m_value;
        std::string stdStr = ss.str();
        if (m_kind == DEG) {
            return String::fromUTF8(stdStr.append("deg").c_str());
        } else if (m_kind == RAD) {
            return String::fromUTF8(stdStr.append("rad").c_str());
        } else if (m_kind == GRAD) {
            return String::fromUTF8(stdStr.append("grad").c_str());
        } else if (m_kind == TURN) {
            return String::fromUTF8(stdStr.append("turn").c_str());
        }

        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

protected:
    Kind m_kind;
    float m_value;
};

inline CSSAngle operator+(const CSSAngle& a, const CSSAngle& b)
{
    return CSSAngle(a.toDegreeValue() + b.toDegreeValue());
}

inline CSSAngle& operator+=(CSSAngle& a, const CSSAngle& b)
{
    a = a + b;
    return a;
}

inline CSSAngle operator-(const CSSAngle& a, const CSSAngle& b)
{
    return CSSAngle(a.toDegreeValue() - b.toDegreeValue());
}

inline CSSAngle operator*(const float a, const CSSAngle& b)
{
    return CSSAngle(a * b.toDegreeValue());
}

inline CSSAngle operator*(const CSSAngle& a, const float b)
{
    return CSSAngle(a.toDegreeValue() * b);
}

inline CSSAngle& operator*=(CSSAngle& a, float b)
{
    a = a * b;
    return a;
}

inline CSSAngle operator/(const CSSAngle& a, const float b)
{
    return CSSAngle(a.toDegreeValue() / b);
}

inline CSSAngle& operator/=(CSSAngle& a, float b)
{
    a = a / b;
    return a;
}

class CSSTime {
public:
    enum Kind { S, MS };

    CSSTime()
    {
        m_kind = MS;
        m_value = 0;
    }

    CSSTime(double time)
    {
        m_kind = MS;
        m_value = time;
    }

    CSSTime(Kind kind, double time)
    {
        m_kind = kind;
        m_value = time;
    }

    Kind kind() const
    {
        return m_kind;
    }

    bool isZero() const
    {
        return m_value == 0;
    }

    float value() const
    {
        return m_value;
    }

    double toTimeValue() const
    {
        if (m_kind == S) {
            return m_value * 1000; // to ms
        } else if (m_kind == MS) {
            return m_value;
        }

        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    String* toString() const
    {
        std::stringstream ss(std::stringstream::in | std::stringstream::out);
        ss << m_value;
        std::string stdStr = ss.str();
        if (m_kind == S) {
            return String::fromUTF8(stdStr.append("s").c_str());
        } else if (m_kind == MS) {
            return String::fromUTF8(stdStr.append("ms").c_str());
        }

        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    bool operator==(const CSSTime& t) const
    {
        if (m_kind != t.m_kind) {
            return false;
        }

        if (m_value != t.m_value) {
            return false;
        }
        return true;
    }

    bool operator!=(const CSSTime& t) const
    {
        return !this->operator==(t);
    }

protected:
    Kind m_kind;
    double m_value; // ms
};

inline CSSTime operator+(const CSSTime& a, const CSSTime& b)
{
    return CSSTime(a.toTimeValue() + b.toTimeValue());
}

inline CSSTime& operator+=(CSSTime& a, const CSSTime& b)
{
    a = a + b;
    return a;
}

inline CSSTime operator-(const CSSTime& a, const CSSTime& b)
{
    return CSSTime(a.toTimeValue() - b.toTimeValue());
}

inline CSSTime operator*(const float a, const CSSTime& b)
{
    return CSSTime(a * b.toTimeValue());
}

inline CSSTime operator*(const CSSTime& a, const float b)
{
    return CSSTime(a.toTimeValue() * b);
}

inline CSSTime& operator*=(CSSTime& a, float b)
{
    a = a * b;
    return a;
}

inline CSSTime operator/(const CSSTime& a, const float b)
{
    return CSSTime(a.toTimeValue() / b);
}

inline CSSTime& operator/=(CSSTime& a, float b)
{
    a = a / b;
    return a;
}

// inline | block | list-item | inline-block | table | inline-table |
// table-row-group | table-header-group | table-footer-group | table-row |
// table-column-group | table-column | table-cell | table-caption | flex |
// inline-flex | none | inherit
enum DisplayValue {
    InlineDisplayValue, // initial value
    BlockDisplayValue,
    InlineBlockDisplayValue,
    TableDisplayValue,
    InlineTableDisplayValue,
    TableRowGroupDisplayValue,
    TableHeaderGroupDisplayValue,
    TableFooterGroupDisplayValue,
    TableRowDisplayValue,
    TableColumnGroupDisplayValue,
    TableColumnDisplayValue,
    TableCellDisplayValue,
    TableCaptionDisplayValue,
    FlexDisplayValue,
    InlineFlexDisplayValue,
    NoneDisplayValue,
};

enum PositionValue {
    StaticPositionValue,
    RelativePositionValue,
    AbsolutePositionValue,
    FixedPositionValue,
};

enum FloatValue {
    NoneFloatValue,
    LeftFloatValue,
    RightFloatValue,
};

enum ClearValue {
    NoneClearValue,
    LeftClearValue,
    RightClearValue,
    BothClearValue
};

// flex
enum FlexDirectionValue {
    RowFlexDirectionValue,
    RowReverseFlexDirectionValue,
    ColumnFlexDirectionValue,
    ColumnReverseFlexDirectionValue,
};

enum FlexWrapValue {
    NoWrapFlexWrapValue,
    WrapFlexWrapValue,
    WrapReverseFlexWrapValue,
};

enum JustifyContentValue {
    FlexStartJustifyContentValue,
    FlexEndJustifyContentValue,
    CenterJustifyContentValue,
    SpaceBetweenJustifyContentValue,
    SpaceAroundJustifyContentValue,
};

enum AlignItemValue {
    FlexStartAlignItemValue,
    FlexEndAlignItemValue,
    CenterAlignItemValue,
    BaselineAlignItemValue,
    StretchAlignItemValue
};

enum AlignContentValue {
    FlexStartAlignContentValue,
    FlexEndAlignContentValue,
    CenterAlignContentValue,
    SpaceBetweenAlignContentValue,
    SpaceAroundAlignContentValue,
    StretchAlignContentValue,
};

enum FlexBasisValue { ContentFlexBasisValue };

enum VerticalAlignValue {
    BaselineVAlignValue,
    SubVAlignValue,
    SuperVAlignValue,
    TopVAlignValue,
    TextTopVAlignValue,
    MiddleVAlignValue,
    BottomVAlignValue,
    TextBottomVAlignValue,
    NumericVAlignValue,
};

enum TextAlignValue {
    StartTextAlignValue,
    EndTextAlignValue,
    LeftTextAlignValue,
    RightTextAlignValue,
    CenterTextAlignValue
};

// transform-origin, background-position
enum SideValue {
    TopSideValue,
    RightSideValue,
    BottomSideValue,
    LeftSideValue,
    CenterSideValue,
    ValueSideValue,
};

enum DirectionValue {
    LtrDirectionValue,
    RtlDirectionValue,
};

enum BackgroundSizeType {
    Cover,
    Contain,
    SizeValue,
    SizeNone,
};

enum BackgroundRepeatValue {
    RepeatRepeatValue,
    NoRepeatRepeatValue,
};

enum FontSizeValue {
    XXSmallFontSizeValue,
    XSmallFontSizeValue,
    SmallFontSizeValue,
    MediumFontSizeValue,
    LargeFontSizeValue,
    XLargeFontSizeValue,
    XXLargeFontSizeValue,
    XXXLargeFontSizeValue, // This is a non-CSS value used a legacy font size.
    LargerFontSizeValue,
    SmallerFontSizeValue,
};

enum WhiteSpaceValue {
    NoWrapWhiteSpaceValue = 1 << 0,  /* Ignore newline characters */
    PreWhiteSpaceValue = 1 << 1,     /* Preserve spaces */
    PreLineWhiteSpaceValue = 1 << 2, /* Wrap lines */
    NormalWhiteSpaceValue = PreLineWhiteSpaceValue | NoWrapWhiteSpaceValue,
    PreWrapWhiteSpaceValue = PreLineWhiteSpaceValue | PreWhiteSpaceValue,
};

enum OverflowValue {
    VisibleOverflow,
    HiddenOverflow,
    AutoOverflow,
    ScrollOverflow,
};

enum BorderImageRepeatValue {
    StretchValue,
    RepeatValue,
    RoundValue,
    SpaceValue,
};

enum BorderShorthandValueType {
    BWidth,
    BStyle,
    BColor,
    BInvalid,
};

enum BorderStyleValue {
    NoneBorderStyleValue,
    SolidBorderStyleValue,
    InsetBorderStyleValue,
    OutsetBorderStyleValue,
};

enum BorderWidthValue {
    ThinBorderWidthValue,
    MediumBorderWidthValue,
    ThickBorderWidthValue,
};

enum BorderCollapseValue {
    SeparateBorderCollapseValue,
    CollapseBorderCollapseValue,
};

enum CaptionSideValue {
    TopCaptionSideValue,
    BottomCaptionSideValue,
};

enum TableLayoutValue {
    AutoTableLayoutValue,
    FixedTableLayoutValue,
};

enum EmptyCellsValue {
    ShowEmptyCellsValue,
    HideEmptyCellsValue,
};

enum TextDecorationValue {
    NoneTextDecorationValue,
    UnderLineTextDecorationValue,
    OverLineTextDecorationValue,
    LineThroughTextDecorationValue,
    BlinkTextDecorationValue,
};

enum FontStyleValue {
    NormalFontStyleValue,
    ItalicFontStyleValue,
    ObliqueFontStyleValue,
};

enum FontWeightValue {
    NormalFontWeightValue,
    BoldFontWeightValue,
    BolderFontWeightValue,
    LighterFontWeightValue,
    OneHundredFontWeightValue,
    TwoHundredsFontWeightValue,
    ThreeHundredsFontWeightValue,
    FourHundredsFontWeightValue,
    FiveHundredsFontWeightValue,
    SixHundredsFontWeightValue,
    SevenHundredsFontWeightValue,
    EightHundredsFontWeightValue,
    NineHundredsFontWeightValue,
};

enum WordWrapValue {
    NormalWordWrapValue,
    BreakWordWordWrapValue,
    //    BreakSpaceWordWrapValue,
};

enum VisibilityValue {
    VisibleVisibilityValue,
    HiddenVisibilityValue,
};

enum UnicodeBidiValue {
    NormalUnicodeBidiValue,
    EmbedUnicodeBidiValue,
    IsolateUnicodeBidiValue,
};

enum TransitionPropertyValue {
    TransitionPropertyAllValue,
    TransitionPropertyBackgroundColorValue,
    TransitionPropertyBackgroundPositionValue,
    TransitionPropertyBorderBottomColorValue,
    TransitionPropertyBorderBottomWidthValue,
    TransitionPropertyBorderLeftColorValue,
    TransitionPropertyBorderLeftWidthValue,
    TransitionPropertyBorderRightColorValue,
    TransitionPropertyBorderRightWidthValue,
    TransitionPropertyBorderSpacingValue,
    TransitionPropertyBorderTopColorValue,
    TransitionPropertyBorderTopWidthValue,
    TransitionPropertyBottomValue,
    TransitionPropertyClipValue,
    TransitionPropertyColorValue,
    TransitionPropertyFontSizeValue,
    TransitionPropertyFontWeightValue,
    TransitionPropertyHeightValue,
    TransitionPropertyLeftValue,
    TransitionPropertyLetterSpacingValue,
    TransitionPropertyLineHeightValue,
    TransitionPropertyMarginBottomValue,
    TransitionPropertyMarginLeftValue,
    TransitionPropertyMarginRightValue,
    TransitionPropertyMarginTopValue,
    TransitionPropertyMaxHeightValue,
    TransitionPropertyMaxWidthValue,
    TransitionPropertyMinHeightValue,
    TransitionPropertyMinWidthValue,
    TransitionPropertyOpacityValue,
    TransitionPropertyOutlineColorValue,
    TransitionPropertyOutlineWidthValue,
    TransitionPropertyPaddingBottomValue,
    TransitionPropertyPaddingLeftValue,
    TransitionPropertyPaddingRightValue,
    TransitionPropertyPaddingTopValue,
    TransitionPropertyRightValue,
    TransitionPropertyTextIndentValue,
    TransitionPropertyTextDhadowValue,
    TransitionPropertyTopValue,
    TransitionPropertyVerticalAlignValue,
    TransitionPropertyVisibilityValue,
    TransitionPropertyWidthValue,
    TransitionPropertyWordSpacingValue,
    TransitionPropertyZIndexValue
};

enum TransitionTimingFunctionValue {
    TransitionTimingFunctionEaseValue,
    TransitionTimingFunctionLinearValue,
    TransitionTimingFunctionEaseInValue,
    TransitionTimingFunctionEaseOutValue,
    TransitionTimingFunctionEaseInOutValue,
    TransitionTimingFunctionStepStartValue,
    TransitionTimingFunctionStepEndValue,
    TransitionTimingFunctionStepsValue,
    TransitionTimingFunctionCubicBezierValue
};

enum BoxSizingValue { ContentBoxBoxSizingValue, BorderBoxBoxSizingValue };

enum FillRuleValue {
    FillRuleNonZero,
    FillRuleEvenOdd,
};

class ValueList;
class CSSStyleDeclaration;

// https://www.w3.org/TR/CSS2/visufx.html
// https://www.w3.org/TR/CSS2/text.html
// https://www.w3.org/TR/CSS21/visuren.html
// https://www.w3.org/TR/CSS21/visudet.html
// https://www.w3.org/TR/CSS21/colors.html
// https://www.w3.org/TR/CSS21/fonts.html
// https://www.w3.org/TR/CSS21/text.html
// https://www.w3.org/TR/CSS21/box.html
// https://www.w3.org/TR/css3-transforms
// https://www.w3.org/TR/css3-background
// https://www.w3.org/TR/css3-color
//
// The following are for internal use only
// * border-horizontal-spacing
// * border-vertical-spacing
#define FOR_EACH_STYLE_ATTRIBUTE(F)                                      \
    F(Color, color, "color")                                             \
    F(Direction, direction, "direction")                                 \
    F(BackgroundColor, backgroundColor, "background-color")              \
    F(BackgroundImage, backgroundImage, "background-image")              \
    F(BackgroundSize, backgroundSize, "background-size")                 \
    F(LineHeight, lineHeight, "line-height")                             \
    F(WhiteSpace, whiteSpace, "white-space")                             \
    F(PaddingTop, paddingTop, "padding-top")                             \
    F(PaddingRight, paddingRight, "padding-right")                       \
    F(PaddingBottom, paddingBottom, "padding-bottom")                    \
    F(PaddingLeft, paddingLeft, "padding-left")                          \
    F(MarginTop, marginTop, "margin-top")                                \
    F(MarginRight, marginRight, "margin-right")                          \
    F(MarginBottom, marginBottom, "margin-bottom")                       \
    F(MarginLeft, marginLeft, "margin-left")                             \
    F(Top, top, "top")                                                   \
    F(Bottom, bottom, "bottom")                                          \
    F(Left, left, "left")                                                \
    F(Right, right, "right")                                             \
    F(Width, width, "width")                                             \
    F(MaxWidth, maxWidth, "max-width")                                   \
    F(MinWidth, minWidth, "min-width")                                   \
    F(Height, height, "height")                                          \
    F(MaxHeight, maxHeight, "max-height")                                \
    F(MinHeight, minHeight, "min-height")                                \
    F(FontSize, fontSize, "font-size")                                   \
    F(FontStyle, fontStyle, "font-style")                                \
    F(WordWrap, wordWrap, "word-wrap")                                   \
    F(OverflowWrap, overflowWrap, "overflow-wrap")                       \
    F(Position, position, "position")                                    \
    F(TextDecoration, textDecoration, "text-decoration")                 \
    F(Display, display, "display")                                       \
    F(Float, float, "float")                                             \
    F(Clear, clear, "clear")                                             \
    F(BorderImageSlice, borderImageSlice, "border-image-slice")          \
    F(BorderImageSource, borderImageSource, "border-image-source")       \
    F(BorderImageWidth, borderImageWidth, "border-image-width")          \
    F(BorderTopColor, borderTopColor, "border-top-color")                \
    F(BorderRightColor, borderRightColor, "border-right-color")          \
    F(BorderBottomColor, borderBottomColor, "border-bottom-color")       \
    F(BorderLeftColor, borderLeftColor, "border-left-color")             \
    F(BorderTopStyle, borderTopStyle, "border-top-style")                \
    F(BorderRightStyle, borderRightStyle, "border-right-style")          \
    F(BorderBottomStyle, borderBottomStyle, "border-bottom-style")       \
    F(BorderLeftStyle, borderLeftStyle, "border-left-style")             \
    F(BorderTopWidth, borderTopWidth, "border-top-width")                \
    F(BorderRightWidth, borderRightWidth, "border-right-width")          \
    F(BorderBottomWidth, borderBottomWidth, "border-bottom-width")       \
    F(BorderLeftWidth, borderLeftWidth, "border-left-width")             \
    F(BorderCollapse, borderCollapse, "border-collapse")                 \
    F(BorderSpacing, borderSpacing, "border-spacing")                    \
    F(CaptionSide, CaptionSide, "caption-side")                          \
    F(EmptyCells, EmptyCells, "empty-cells")                             \
    F(TextAlign, textAlign, "text-align")                                \
    F(TextIndent, textIndent, "text-indent")                             \
    F(Transform, transform, "transform")                                 \
    F(TransformOrigin, transformOrigin, "transform-origin")              \
    F(Visibility, visibility, "visibility")                              \
    F(OverflowX, overflowX, "overflow-x")                                \
    F(OverflowY, overflowY, "overflow-y")                                \
    F(ZIndex, zIndex, "z-index")                                         \
    F(VerticalAlign, verticalAlign, "vertical-align")                    \
    F(BackgroundRepeatX, backgroundRepeatX, "background-repeat-x")       \
    F(BackgroundRepeatY, backgroundRepeatY, "background-repeat-y")       \
    F(BackgroundPositionX, backgroundPositionX, "background-position-x") \
    F(BackgroundPositionY, backgroundPositionY, "background-position-y") \
    F(Opacity, opacity, "opacity")                                       \
    F(FontWeight, fontWeight, "font-weight")                             \
    F(TableLayout, tableLayout, "table-layout")                          \
    F(UnicodeBidi, unicodeBidi, "unicode-bidi")                          \
    F(Content, content, "content")                                       \
    F(TransitionProperty, transitionProperty, "transitionProperty")      \
    F(TransitionDuration, transitionDuration, "transitionDuration")      \
    F(TransitionTimingFunction, transitionTimingFunction,                \
      "transitionTimingFunction")                                        \
    F(TransitionDelay, transitionDelay, "transitionDelay")               \
    F(BoxSizing, boxSizing, "box-sizing")                                \
    F(Fill, fill, "fill")                                                \
    F(FillOpacity, fillOpacity, "fill-opacity")                          \
    F(FillRule, fillRule, "fill-rule")                                   \
    F(Stroke, stroke, "stroke")                                          \
    F(StrokeWidth, strokeWidth, "stroke-width")                          \
    F(FlexDirection, flexDirection, "flex-direction")                    \
    F(FlexWrap, flexWrap, "flex-wrap")                                   \
    F(Order, order, "order")                                             \
    F(JustifyContent, justifyContent, "justify-content")                 \
    F(AlignItems, alignItems, "align-items")                             \
    F(AlignSelf, alignSelf, "align-self")                                \
    F(AlignContent, alignContent, "align-content")                       \
    F(FlexGrow, flexGrow, "flex-grow")                                   \
    F(FlexShrink, flexShrink, "flex-shrink")                             \
    F(FlexBasis, flexBasis, "flex-basis")                                \
    F(OutlineColor, outlineColor, "outline-color")                       \
    F(OutlineStyle, outlineStyle, "outline-style")                       \
    F(OutlineWidth, outlineWidth, "outline-width")                       \
    F(OutlineOffset, outlineOffset, "outline-offset")                    \
    F(Cursor, cursor, "cursor")

#define FOR_EACH_STYLE_ATTRIBUTE_TOTAL(F)                            \
    FOR_EACH_STYLE_ATTRIBUTE(F)                                      \
    F(Border, border, "border")                                      \
    F(BorderTop, borderTop, "border-top")                            \
    F(BorderRight, borderRight, "border-right")                      \
    F(BorderBottom, borderBottom, "border-bottom")                   \
    F(BorderLeft, borderLeft, "border-left")                         \
    F(BorderStyle, borderStyle, "border-style")                      \
    F(BorderWidth, borderWidth, "border-width")                      \
    F(BorderColor, borderColor, "border-color")                      \
    F(Background, background, "background")                          \
    F(BackgroundRepeat, backgroundRepeat, "background-repeat")       \
    F(BackgroundPosition, backgroundPosition, "background-position") \
    F(Margin, margin, "margin")                                      \
    F(Padding, padding, "padding")                                   \
    F(Font, font, "font")                                            \
    F(Outline, outline, "outline")                                   \
    F(Overflow, overflow, "overflow")                                \
    F(Transition, transition, "transition")                          \
    F(FlexFlow, flexFlow, "flex-flow")                               \
    F(Flex, flex, "flex")

#define GEN_FOURSIDE(F) \
    F(Top, top)         \
    F(Right, right)     \
    F(Bottom, bottom)   \
    F(Left, left)

class CSSTransformFunction {
public:
    enum Kind {
        Matrix,
        Matrix3D,
        Translate,
        Translate3D,
        TranslateX,
        TranslateY,
        TranslateZ,
        Scale,
        Scale3D,
        ScaleX,
        ScaleY,
        ScaleZ,
        Rotate,
        Rotate3D,
        Skew,
        SkewX,
        SkewY,
        Perspective
    };

    CSSTransformFunction(Kind kind, ValueList* values)
    {
        m_kind = kind;
        m_values = values;
    }

    CSSTransformFunction(Kind kind)
    {
        m_kind = kind;
        m_values = nullptr;
    }

    Kind kind()
    {
        return m_kind;
    }

    ValueList* values()
    {
        return m_values;
    }

    String* functionName()
    {
        switch (m_kind) {
        case Matrix:
            return String::fromUTF8("matrix");
        case Matrix3D:
            return String::fromUTF8("matrix3d");
        case Translate:
            return String::fromUTF8("translate");
        case Translate3D:
            return String::fromUTF8("translate3d");
        case TranslateX:
            return String::fromUTF8("translateX");
        case TranslateY:
            return String::fromUTF8("translateY");
        case TranslateZ:
            return String::fromUTF8("translateZ");
        case Scale:
            return String::fromUTF8("scale");
        case Scale3D:
            return String::fromUTF8("scale3d");
        case ScaleX:
            return String::fromUTF8("scaleX");
        case ScaleY:
            return String::fromUTF8("scaleY");
        case ScaleZ:
            return String::fromUTF8("scaleZ");
        case Rotate:
            return String::fromUTF8("rotate");
        case Rotate3D:
            return String::fromUTF8("rotate3D");
        case Skew:
            return String::fromUTF8("skew");
        case SkewX:
            return String::fromUTF8("skewX");
        case SkewY:
            return String::fromUTF8("skewY");
        case Perspective:
            return String::fromUTF8("perspective");
        }
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

protected:
    Kind m_kind;
    ValueList* m_values;
};

class CSSTransformFunctions : public GCVector<CSSTransformFunction>, public gc {
public:
    String* toString();
};

class CSSStyleValuePair : public gc {
    friend class ValueList;

public:
    enum KeyKind {
        Empty,
#define ADD_CSS_KEYKIND(Name, name, cssname) Name,
        FOR_EACH_STYLE_ATTRIBUTE(ADD_CSS_KEYKIND)
#undef ADD_CSS_KEYKIND
    };

    enum ValueKind {
        Initial,
        Inherit,
        Length,
        Percentage,
        Auto,
        None,
        Number, // real number values -
                // https://www.w3.org/TR/CSS21/syndata.html#value-def-number
        Int32,
        Angle, //
        Time,
        Normal,
        StringValueKind,
        ColorValueKind,
        NamedColorValueKind,
        UrlValueKind,

        CalcValueKind,

        DisplayValueKind,
        PositionValueKind,
        FloatValueKind,
        ClearValueKind,
        VerticalAlignValueKind,
        TextAlignValueKind,
        SideValueKind,
        DirectionValueKind,
        WhiteSpaceValueKind,

        ValueListKind,

        // BackgroundSize
        Cover,
        Contain,

        BackgroundRepeatValueKind,

        FontSizeValueKind,
        FontStyleValueKind,
        FontWeightValueKind,
        WordWrapValueKind,

        BorderStyleValueKind,
        BorderWidthValueKind,

        // table
        BorderCollapseValueKind,
        CaptionSideValueKind,
        TableLayoutValueKind,
        EmptyCellsValueKind,

        OverflowValueKind,
        TextDecorationValueKind,
        VisibilityValueKind,
        UnicodeBidiValueKind,
        BoxSizingValueKind,

        // flex
        FlexDirectionValueKind,
        FlexWrapValueKind,
        JustifyContentValueKind,
        AlignItemValueKind,
        AlignContentValueKind,
        FlexBasisValueKind,

        // transform
        TransformFunctions,

        // transition
        TransitionPropertyValueKind,
        TransitionTimingFunctionValueKind,

        // content
        Attr,

        // svg
        FillRuleValueKind
    };

    CSSStyleValuePair()
        : m_keyKind(KeyKind::Empty)
        , m_valueKind(ValueKind::None)
        , m_flagImportant(false)
        , m_value(0.0f)
    {
    }

    KeyKind keyKind() const
    {
        return m_keyKind;
    }

    void setKeyKind(KeyKind kind)
    {
        m_keyKind = kind;
    }

    String* keyName() const;

    ValueKind valueKind() const
    {
        return m_valueKind;
    }

    void setValueKind(ValueKind kind)
    {
        m_valueKind = kind;
    }

    bool flagImportant() const
    {
        return m_flagImportant;
    }

    void setFlagImportant(bool isImportant)
    {
        m_flagImportant = isImportant;
    }

    bool updateValueCommon(const CSSTokenVector& tokens);

    bool isAuto()
    {
        return valueKind() == Auto;
    }

    bool isInherit()
    {
        return valueKind() == Inherit;
    }

    DisplayValue displayValue() const
    {
        STARFISH_ASSERT(m_valueKind == DisplayValueKind);
        return m_value.m_display;
    }

    PositionValue positionValue() const
    {
        STARFISH_ASSERT(m_valueKind == PositionValueKind);
        return m_value.m_position;
    }

    FloatValue floatValue() const
    {
        STARFISH_ASSERT(m_valueKind == FloatValueKind);
        return m_value.m_float;
    }

    ClearValue clearValue() const
    {
        STARFISH_ASSERT(m_valueKind == ClearValueKind);
        return m_value.m_clear;
    }

    VerticalAlignValue verticalAlignValue() const
    {
        STARFISH_ASSERT(m_valueKind == VerticalAlignValueKind);
        return m_value.m_verticalAlign;
    }

    TextAlignValue textAlignValue() const
    {
        STARFISH_ASSERT(m_valueKind == TextAlignValueKind);
        return m_value.m_textAlign;
    }

    SideValue sideValue() const
    {
        STARFISH_ASSERT(m_valueKind == SideValueKind);
        return m_value.m_side;
    }

    FontSizeValue fontSizeValue() const
    {
        STARFISH_ASSERT(m_valueKind == FontSizeValueKind);
        return m_value.m_fontSize;
    }

    FontStyleValue fontStyleValue() const
    {
        STARFISH_ASSERT(m_valueKind == FontStyleValueKind);
        return m_value.m_fontStyle;
    }

    FontWeightValue fontWeightValue() const
    {
        STARFISH_ASSERT(m_valueKind == FontWeightValueKind);
        return m_value.m_fontWeight;
    }

    WordWrapValue wordWrapValue() const
    {
        STARFISH_ASSERT(m_valueKind == WordWrapValueKind);
        return m_value.m_wordWrap;
    }

    DirectionValue directionValue() const
    {
        STARFISH_ASSERT(m_valueKind == DirectionValueKind);
        return m_value.m_direction;
    }

    WhiteSpaceValue whiteSpaceValue() const
    {
        STARFISH_ASSERT(m_valueKind == WhiteSpaceValueKind);
        return m_value.m_whiteSpace;
    }

    UnicodeBidiValue unicodeBidiValue() const
    {
        STARFISH_ASSERT(m_valueKind == UnicodeBidiValueKind);
        return m_value.m_unicodeBidi;
    }

    BorderStyleValue borderStyleValue() const
    {
        STARFISH_ASSERT(m_valueKind == BorderStyleValueKind);
        return m_value.m_borderStyle;
    }

    BorderWidthValue borderWidthValue() const
    {
        STARFISH_ASSERT(m_valueKind == BorderWidthValueKind);
        return m_value.m_borderWidth;
    }

    CSSLength lengthValue() const
    {
        STARFISH_ASSERT(m_valueKind == Length);
        return m_value.m_length;
    }

    CSSAngle angleValue() const
    {
        STARFISH_ASSERT(m_valueKind == Angle);
        return m_value.m_angle;
    }

    CSSTime timeValue() const
    {
        STARFISH_ASSERT(m_valueKind == Time);
        return m_value.m_time;
    }

    CSSTransformFunctions* transformValue() const
    {
        STARFISH_ASSERT(m_valueKind == TransformFunctions);
        return m_value.m_transforms;
    }

    float numberValue() const
    {
        STARFISH_ASSERT(m_valueKind == Number);
        return m_value.m_floatValue;
    }

    int32_t int32Value() const
    {
        STARFISH_ASSERT(m_valueKind == Int32);
        return m_value.m_int32Value;
    }

    // 0~1
    float percentageValue() const
    {
        STARFISH_ASSERT(m_valueKind == Percentage);
        return m_value.m_floatValue;
    }

    String* stringValue() const
    {
        STARFISH_ASSERT(m_valueKind == StringValueKind);
        return m_value.m_stringValue;
    }

    String* urlValue(ResourceURL* urlOfStyleSheet) const;

    String* urlStringValue() const
    {
        STARFISH_ASSERT(m_valueKind == UrlValueKind);
        return m_value.m_stringValue;
    }

    void setStringValue(String* value)
    {
        STARFISH_ASSERT(m_valueKind == StringValueKind);
        m_value.m_stringValue = value;
    }

    BackgroundRepeatValue backgroundRepeatValue() const
    {
        STARFISH_ASSERT(m_valueKind == BackgroundRepeatValueKind);
        return m_value.m_backgroundRepeat;
    }

    ValueList* multiValue() const
    {
        STARFISH_ASSERT(m_valueKind == ValueListKind);
        return m_value.m_multiValue;
    }

    OverflowValue overflowValue() const
    {
        STARFISH_ASSERT(m_valueKind == OverflowValueKind);
        return m_value.m_overflow;
    }

    VisibilityValue visibilityValue() const
    {
        STARFISH_ASSERT(m_valueKind == VisibilityValueKind);
        return m_value.m_visibility;
    }

    TextDecorationValue textDecorationValue() const
    {
        return m_value.m_textDecoration;
    }

    Unit::Color colorValue() const
    {
        STARFISH_ASSERT(m_valueKind == ColorValueKind);
        return m_value.m_color;
    }

    NamedColor::NamedColorValue namedColorValue() const
    {
        STARFISH_ASSERT(m_valueKind == NamedColorValueKind);
        return m_value.m_namedColor;
    }

    BorderCollapseValue borderCollapseValue() const
    {
        STARFISH_ASSERT(m_valueKind == BorderCollapseValueKind);
        return m_value.m_borderCollapse;
    }

    CaptionSideValue captionSideValue() const
    {
        STARFISH_ASSERT(m_valueKind == CaptionSideValueKind);
        return m_value.m_captionSide;
    }

    EmptyCellsValue emptyCellsValue() const
    {
        STARFISH_ASSERT(m_valueKind == EmptyCellsValueKind);
        return m_value.m_emptyCells;
    }

    TableLayoutValue tableLayoutValue() const
    {
        STARFISH_ASSERT(m_valueKind == TableLayoutValueKind);
        return m_value.m_tableLayout;
    }

    TransitionPropertyValue transitionPropertyValue() const
    {
        STARFISH_ASSERT(m_valueKind == TransitionPropertyValueKind);
        return m_value.m_transitionProperty;
    }

    TransitionTimingFunctionValue transitionTimingFunctionValue() const
    {
        STARFISH_ASSERT(m_valueKind == TransitionTimingFunctionValueKind);
        return m_value.m_transitionTimingFunction;
    }

    BoxSizingValue boxSizingValue() const
    {
        STARFISH_ASSERT(m_valueKind == BoxSizingValueKind);
        return m_value.m_boxSizing;
    }

    String* attrValue() const
    {
        STARFISH_ASSERT(m_valueKind == Attr);
        return m_value.m_stringValue;
    }

    FlexDirectionValue flexDirectionValue() const
    {
        STARFISH_ASSERT(m_valueKind == FlexDirectionValueKind);
        return m_value.m_flexDirection;
    }

    FlexWrapValue flexWrapValue() const
    {
        STARFISH_ASSERT(m_valueKind == FlexWrapValueKind);
        return m_value.m_flexWrap;
    }

    JustifyContentValue justifyContentValue() const
    {
        STARFISH_ASSERT(m_valueKind == JustifyContentValueKind);
        return m_value.m_justifyContent;
    }

    AlignItemValue alignItemValue() const
    {
        STARFISH_ASSERT(m_valueKind == AlignItemValueKind);
        return m_value.m_alignItem;
    }

    AlignContentValue alignContentValue() const
    {
        STARFISH_ASSERT(m_valueKind == AlignContentValueKind);
        return m_value.m_alignContent;
    }

    FlexBasisValue flexBasisValue() const
    {
        STARFISH_ASSERT(m_valueKind == FlexBasisValueKind);
        return m_value.m_flexBasis;
    }

    FillRuleValue fillRuleValue() const
    {
        STARFISH_ASSERT(m_valueKind == FillRuleValueKind);
        return m_value.m_fillRule;
    }

    CalcData* calcValue() const
    {
        STARFISH_ASSERT(m_valueKind == CalcValueKind);
        return m_value.m_calc;
    }

    union ValueData {
        float m_floatValue;
        int32_t m_int32Value;
        DisplayValue m_display;
        PositionValue m_position;
        FloatValue m_float;
        ClearValue m_clear;
        VerticalAlignValue m_verticalAlign;
        FontSizeValue m_fontSize;
        FontStyleValue m_fontStyle;
        FontWeightValue m_fontWeight;
        WordWrapValue m_wordWrap;
        TextAlignValue m_textAlign;
        SideValue m_side;
        DirectionValue m_direction;
        WhiteSpaceValue m_whiteSpace;
        CSSLength m_length;
        CSSAngle m_angle;
        String* m_stringValue;
        BackgroundRepeatValue m_backgroundRepeat;
        BorderStyleValue m_borderStyle;
        BorderWidthValue m_borderWidth;
        ValueList* m_multiValue;
        OverflowValue m_overflow;
        VisibilityValue m_visibility;
        UnicodeBidiValue m_unicodeBidi;
        TextDecorationValue m_textDecoration;
        CSSTransformFunctions* m_transforms;
        Unit::Color m_color;
        NamedColor::NamedColorValue m_namedColor;
        BorderCollapseValue m_borderCollapse;
        CaptionSideValue m_captionSide;
        TableLayoutValue m_tableLayout;
        EmptyCellsValue m_emptyCells;
        TransitionPropertyValue m_transitionProperty;
        TransitionTimingFunctionValue m_transitionTimingFunction;
        BoxSizingValue m_boxSizing;
        CSSTime m_time;
        FlexDirectionValue m_flexDirection;
        FlexWrapValue m_flexWrap;
        JustifyContentValue m_justifyContent;
        AlignItemValue m_alignItem;
        AlignContentValue m_alignContent;
        FlexBasisValue m_flexBasis;
        FillRuleValue m_fillRule;
        CalcData* m_calc;

        ValueData(int v)
            : m_floatValue(v)
        {
        }
        ValueData(float v)
            : m_floatValue(v)
        {
        }
        ValueData(DisplayValue v)
            : m_display(v)
        {
        }
        ValueData(PositionValue v)
            : m_position(v)
        {
        }
        ValueData(FloatValue v)
            : m_float(v)
        {
        }
        ValueData(ClearValue v)
            : m_clear(v)
        {
        }
        ValueData(VerticalAlignValue v)
            : m_verticalAlign(v)
        {
        }
        ValueData(FontSizeValue v)
            : m_fontSize(v)
        {
        }
        ValueData(FontStyleValue v)
            : m_fontStyle(v)
        {
        }
        ValueData(FontWeightValue v)
            : m_fontWeight(v)
        {
        }
        ValueData(WordWrapValue v)
            : m_wordWrap(v)
        {
        }
        ValueData(TextAlignValue v)
            : m_textAlign(v)
        {
        }
        ValueData(SideValue v)
            : m_side(v)
        {
        }
        ValueData(DirectionValue v)
            : m_direction(v)
        {
        }
        ValueData(WhiteSpaceValue v)
            : m_whiteSpace(v)
        {
        }
        ValueData(CSSLength v)
            : m_length(v)
        {
        }
        ValueData(CSSAngle v)
            : m_angle(v)
        {
        }
        ValueData(String* v)
            : m_stringValue(v)
        {
        }
        ValueData(BackgroundRepeatValue v)
            : m_backgroundRepeat(v)
        {
        }
        ValueData(BorderStyleValue v)
            : m_borderStyle(v)
        {
        }
        ValueData(BorderWidthValue v)
            : m_borderWidth(v)
        {
        }
        ValueData(ValueList* v)
            : m_multiValue(v)
        {
        }
        ValueData(OverflowValue v)
            : m_overflow(v)
        {
        }
        ValueData(VisibilityValue v)
            : m_visibility(v)
        {
        }
        ValueData(UnicodeBidiValue v)
            : m_unicodeBidi(v)
        {
        }
        ValueData(TextDecorationValue v)
            : m_textDecoration(v)
        {
        }
        ValueData(CSSTransformFunctions* v)
            : m_transforms(v)
        {
        }
        ValueData(Unit::Color v)
            : m_color(v)
        {
        }
        ValueData(NamedColor::NamedColorValue v)
            : m_namedColor(v)
        {
        }
        ValueData(BorderCollapseValue v)
            : m_borderCollapse(v)
        {
        }
        ValueData(CaptionSideValue v)
            : m_captionSide(v)
        {
        }
        ValueData(EmptyCellsValue v)
            : m_emptyCells(v)
        {
        }
        ValueData(TableLayoutValue v)
            : m_tableLayout(v)
        {
        }
        ValueData(TransitionPropertyValue v)
            : m_transitionProperty(v)
        {
        }
        ValueData(CSSTime v)
            : m_time(v)
        {
        }
        ValueData(TransitionTimingFunctionValue v)
            : m_transitionTimingFunction(v)
        {
        }
        ValueData(BoxSizingValue v)
            : m_boxSizing(v)
        {
        }
        ValueData(FlexDirectionValue v)
            : m_flexDirection(v)
        {
        }
        ValueData(FlexWrapValue v)
            : m_flexWrap(v)
        {
        }
        ValueData(JustifyContentValue v)
            : m_justifyContent(v)
        {
        }
        ValueData(AlignItemValue v)
            : m_alignItem(v)
        {
        }
        ValueData(AlignContentValue v)
            : m_alignContent(v)
        {
        }
        ValueData(FlexBasisValue v)
            : m_flexBasis(v)
        {
        }
        ValueData(FillRuleValue v)
            : m_fillRule(v)
        {
        }
        ValueData(CalcData* v)
            : m_calc(v)
        {
        }
    };

    CSSStyleValuePair(ValueKind kind, ValueData value)
        : m_valueKind(kind)
        , m_flagImportant(false)
        , m_value(value)
    {
    }

    void* pointerValue()
    {
        switch (m_valueKind) {
        case UrlValueKind:
        case StringValueKind:
            return m_value.m_stringValue;
        case ValueListKind:
            return m_value.m_multiValue;
        case TransformFunctions:
            return m_value.m_transforms;
        default:
            return nullptr;
        }
    }

    void setValue(const ValueData& value)
    {
        m_value = value;
    }

    const ValueData& value() const
    {
        return m_value;
    }

    String* toString() const;

    void setLengthValue(const char* value)
    {
        setLengthValue(value, strlen(value));
    }
    void setLengthValue(const char* value, size_t len);

    void setLengthValue(CSSLength val)
    {
        m_valueKind = CSSStyleValuePair::ValueKind::Length;
        m_value.m_length = val;
    }

    void setPercentageValue(float val)
    {
        m_valueKind = CSSStyleValuePair::ValueKind::Percentage;
        m_value.m_floatValue = val;
    }

    void setValueList(ValueList* val)
    {
        m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
        m_value.m_multiValue = val;
    }

    void setValue(KeyKind kKind, const char* value, size_t len)
    {
        switch (kKind) {
        case Color: {
            setValueKind(StringValueKind);
            setStringValue(String::fromUTF8(value));
            break;
        }
        case MarginTop:
        case MarginRight:
        case MarginBottom:
        case MarginLeft:
            setLengthValue(value, len);
            break;
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

#define NEW_SET_VALUE_DECL(name, ...) \
    bool updateValue##name(const CSSTokenVector& tokens);
    FOR_EACH_STYLE_ATTRIBUTE(NEW_SET_VALUE_DECL)
#undef NEW_SET_VALUE_DECL

    bool updateValueNumber(const CSSTokenVector& tokens);
    bool updateValueNumber(const CSSTokenValue& token);
    enum LengthOption {
        AllowNegative = 1 << 0,
        AllowPercent = 1 << 1,
        AllowAuto = 1 << 2,
        AllowLengthWithoutUnit = 1 << 3,
        AllowNone = 1 << 4
    };
    bool updateValueLength(const CSSTokenVector& tokens, uint8_t option);
    bool updateValueUnitLength(const CSSTokenValue& token, uint8_t option);
    bool updateValueUnitLengthOrCalc(const CSSTokenValue& token,
                                     uint8_t option);
    bool updateValueBackgroundImage(const CSSTokenVector& tokens,
                                    bool allowComma);
    bool updateValueBackgroundSize(const CSSTokenVector& tokens,
                                   bool allowComma);
    bool updateValueUnitBackgroundRepeat(const CSSTokenValue& token);
    bool updateValueUnitBackgroundPositionX(const CSSTokenValue& token);
    bool updateValueUnitBackgroundPositionY(const CSSTokenValue& token);
    bool updateValueUnitBorderStyle(const CSSTokenValue& token);
    bool updateValueUnitBorderWidth(const CSSTokenValue& token);
    bool updateValueUnitBorderColor(const CSSTokenValue& token);
    bool updateValueUnitColor(const CSSTokenValue& token);
    bool updateValueUnitUrlOrNone(const CSSTokenValue& token);
    bool updateValueUnitMargin(const CSSTokenValue& token);
    bool updateValueUnitPadding(const CSSTokenValue& token);
    bool updateValueUnitFontSize(const CSSTokenValue& token);
    bool updateValueUnitFontStyle(const CSSTokenValue& token);
    bool updateValueUnitFontWeight(const CSSTokenValue& token);
    bool updateValueUnitWordWrap(const CSSTokenValue& token);
    bool updateValueUnitLineHeight(const CSSTokenValue& token);
    bool updateValueUnitTransitionProperty(const CSSTokenValue& value);
    bool updateValueUnitTransitionTimingFunction(const CSSTokenValue& value);
    bool updateValueUnitTransitionTime(const CSSTokenValue& value);
    bool updateValueUnitOverflowX(const CSSTokenValue& value);
    bool updateValueUnitOverflowY(const CSSTokenValue& value);
    bool updateValueUnitFlexDirection(const CSSTokenValue& value);
    bool updateValueUnitFlexWrap(const CSSTokenValue& value);
    bool updateValueUnitAlignItem(const CSSTokenValue& value);
    bool updateValueUnitFlexGrow(const CSSTokenValue& value);
    bool updateValueUnitFlexShrink(const CSSTokenValue& value);
    bool updateValueUnitFlexBasis(const CSSTokenValue& value);

    bool updateValueTransform(const CSSTokenVector& tokens, bool canIgnoreUnit);

protected:
    KeyKind m_keyKind : 8;
    ValueKind m_valueKind : 8;
    bool m_flagImportant : 1;
    ValueData m_value;
};

class CSSStyleValuePairVectorHolder : public gc {
public:
    void push_back(CSSStyleValuePair p)
    {
        for (size_t i = 0; i < m_data.size(); i++) {
            CSSStyleValuePair v = m_data[i];
            if (v.keyKind() == p.keyKind()) {
                m_data[i] = p;
                rootPointer(p);
                return;
            }
        }
        m_data.push_back(p);
        rootPointer(p);
    }

    const GCAtomicVector<CSSStyleValuePair>& data()
    {
        return m_data;
    }

protected:
    void rootPointer(CSSStyleValuePair v)
    {
        auto p = v.pointerValue();
        if (p) {
            m_pointerRooter.insert(p);
        }
    }
    GCAtomicVector<CSSStyleValuePair> m_data;
    GCUnorderedSet<void*> m_pointerRooter;
};

class ValueList : public GCVector<CSSStyleValuePair>, public gc {
public:
    enum Separator { None, SpaceSeparator, CommaSeparator, SlashSeparator };

    ValueList()
        : GCVector<CSSStyleValuePair>()
        , m_separator(None)
    {
    }

    ValueList(Separator sep)
        : GCVector<CSSStyleValuePair>()
        , m_separator(sep)
    {
    }

    String* separatorString()
    {
        if (m_separator == None) {
            return String::emptyString;
        } else if (m_separator == SpaceSeparator) {
            return String::spaceString;
        } else if (m_separator == CommaSeparator) {
            return String::fromUTF8(", ");
        } else {
            return String::fromUTF8("/ ");
        }
    }

protected:
    Separator m_separator;
};

class CSSSelector;
class CSSAttributeSelector;
class CSSPseudoSelector;

class CSSSelectorList : public GCVector<CSSSelector*>, public gc {
public:
    CSSSelectorList()
        : m_specificity(0)
    {
    }

    void push_front(CSSSelector* s)
    {
        GCVector<CSSSelector*>::insert(begin(), s);
    }

    unsigned specificity();
    String* selectorText();
    static String* selectorText(CSSSelectorList* list, unsigned idx,
                                String* rightSide);

private:
    unsigned m_specificity;
};

class CSSSelector : public gc {
public:
    enum Type {
        UnKnown,
        Universal,
        Tag,
        Id,
        Class,
        PseudoElement,
        PseudoClass,
        AttributeExact,   // Example: E[foo="bar"]
        AttributeSet,     // Example: E[foo]
        AttributeHyphen,  // Example: E[foo|="bar"]
        AttributeList,    // Example: E[foo~="bar"]
        AttributeContain, // css3: E[foo*="bar"]
        AttributeBegin,   // css3: E[foo^="bar"]
        AttributeEnd,     // css3: E[foo$="bar"]
        FirstAttributeSelectorMatch = AttributeExact,
    };

    enum RelationType {
        None,
        SubSelector,     // No combinator
        Descendant,      // "Space" combinator
        Child,           // > combinator
        AdjacentSibling, // + combinator
        GeneralSibling   // ~ combinator
    };

    enum PseudoType {
        PseudoNone,
        PseudoEmpty,
        PseudoRoot,
        PseudoFirstChild,
        PseudoFirstOfType,
        PseudoLastChild,
        PseudoLastOfType,
        PseudoOnlyChild,
        PseudoOnlyOfType,
        PseudoFirstLine,
        PseudoFirstLetter,
        PseudoNthChild,
        PseudoNthOfType,
        PseudoNthLastChild,
        PseudoNthLastOfType,
        PseudoLink,
        PseudoHover,
        PseudoFocus,
        PseudoActive,
        PseudoEnabled,
        PseudoDisabled,
        PseudoTarget,
        PseudoBefore,
        PseudoAfter,
        PseudoLang,
        PseudoNot,
        PseudoSelection
    };

    enum AttributeMatchType {
        CaseInsensitive,
        CaseSensitive,
    };

    CSSSelector(Type type, RelationType relation, AtomicString text)
        : m_type(type)
        , m_relation(relation)
        , m_pseudotype(PseudoNone)
        , m_attributeMatch(CaseInsensitive)
        , m_relationIsAffectedByPseudoContent(false)
        , m_selectorText(text)
    {
    }

    bool isAttributeSelector() const
    {
        return m_type >= FirstAttributeSelectorMatch;
    }

    CSSAttributeSelector* asCSSAttributeSelector()
    {
        STARFISH_ASSERT(isAttributeSelector());
        return (CSSAttributeSelector*)this;
    }

    bool isPseudoSelector() const
    {
        return m_type == PseudoClass || m_type == PseudoElement;
    }

    CSSPseudoSelector* asCSSPseudoSelector()
    {
        STARFISH_ASSERT(isPseudoSelector());
        return (CSSPseudoSelector*)this;
    }

    Type type() const
    {
        return m_type;
    }

    RelationType relation() const
    {
        return m_relation;
    }

    void updateRelation(RelationType rel)
    {
        m_relation = rel;
    }

    bool relationIsAffectedByPseudoContent()
    {
        return m_relationIsAffectedByPseudoContent;
    }

    void setRelationIsAffectedByPseudoContent()
    {
        m_relationIsAffectedByPseudoContent = true;
    }

    const AtomicString& selectorText() const
    {
        return m_selectorText;
    }

    bool isSimple(CSSSelectorList* selectorList);

    // http://www.w3.org/TR/css3-selectors/#specificity
    unsigned specificityForOneSelector() const;

    bool isLastInTagHistory() const
    {
        return relation() == RelationType::None;
    }

protected:
    Type m_type : 4;
    RelationType m_relation : 3;
    PseudoType m_pseudotype : 5;
    AttributeMatchType m_attributeMatch : 1;
    bool m_relationIsAffectedByPseudoContent : 1;

    AtomicString m_selectorText;
};

static_assert(sizeof(CSSSelector) <= sizeof(size_t) * 2,
              "keep sizeof CSSSelector small");

class CSSAttributeSelector : public CSSSelector {
public:
    CSSAttributeSelector(CSSSelector::Type type, const QualifiedName& attr,
                         String* value,
                         CSSSelector::AttributeMatchType matchType,
                         CSSSelector::RelationType relType)
        : CSSSelector(type, relType, AtomicString())
        , m_value(value)
        , m_attribute(attr)
    {
        m_attributeMatch = matchType;
    }

    AttributeMatchType attributeMatch() const
    {
        return m_attributeMatch;
    }

    const QualifiedName& attribute() const
    {
        STARFISH_ASSERT(isAttributeSelector());
        return m_attribute;
    }

    String* value() const
    {
        return m_value;
    }

protected:
    String* m_value;
    QualifiedName m_attribute;
};

class CSSPseudoSelector : public CSSSelector {
public:
    CSSPseudoSelector(CSSSelector::Type type, CSSSelector::RelationType relType)
        : CSSSelector(type, relType, AtomicString())
        , m_argument(String::emptyString)
    {
        m_pseudotype = PseudoNone;
    }

    PseudoType pseudoType() const
    {
        return m_pseudotype;
    }

    CSSSelectorList& pseudoSelectorList()
    {
        return m_pseudoSelectorList;
    }

    void setPseudoSelectorList(CSSSelector* selector)
    {
        STARFISH_ASSERT(m_type != Tag);
        m_pseudoSelectorList.push_back(selector);
    }

    String* argument()
    {
        return m_argument;
    }

    void setArgument(String* value)
    {
        STARFISH_ASSERT(m_type != Tag);
        m_argument = value;
    }

    int nthAValue()
    {
        return m_nth.m_a;
    }

    int nthBValue()
    {
        return m_nth.m_b;
    }

    void setNth(int a, int b)
    {
        STARFISH_ASSERT(m_type != Tag);
        m_nth.m_a = a;
        m_nth.m_b = b;
    }

    bool matchNth(int count);

    PseudoType parsePseudoType(StarFish* sf, AtomicString name,
                               bool hasArguments);
    void updatePseudoType(StarFish* sf, AtomicString name, bool hasArguments);

protected:
    CSSSelectorList m_pseudoSelectorList;
    String* m_argument;
    struct {
        int m_a; // Used for :nth-*
        int m_b; // Used for :nth-*
    } m_nth;
};

using Declarations = GCVector<std::pair<CSSStyleDeclaration*, ResourceURL*>>;

template <unsigned int InlineStorageSize>
using MatchedStyleRules = VectorWithInlineStorage<
    InlineStorageSize, std::pair<StyleRule*, ResourceURL*>,
    std::allocator<std::pair<StyleRule*, ResourceURL*>>>;

class CSSStyleSheet;
class StyleResolver : public DocumentHoldable, public gc {
public:
    enum PseudoElementType {
        PseudoElementNone = 0,
        PseudoElementFirstLine = 1,
        PseudoElementFirstLetter = 1 << 1,
        PseudoElementBefore = 1 << 2,
        PseudoElementAfter = 1 << 3,
        PseudoElementFirstLineInherited = 1 << 4,
        PseudoElementFormOnly = 1 << 5,
    };

    enum Match {
        SelectorMatches,          // The selector matches the element
        SelectorFailsLocally,     // The selector fails for the element.
        SelectorFailsAllSiblings, // The selector fails for the element and any
                                  // sibling of the element
        SelectorFailsCompletely   // The selector fails for the element or
                                  // ancestor of the element
    };

    enum CombinatorMatchingResult {
        CombinatorFails,
        CombinatorMatchesPartially,
    };

    struct MatchResult {
        MatchResult()
            : pseudoType(PseudoElementNone)
            , combinatorResult(CombinatorFails)
        {
        }

        PseudoElementType pseudoType;
        CombinatorMatchingResult combinatorResult;
    };

    StyleResolver(Document* document);
    void addSheet(CSSStyleSheet* sheet);
    void removeSheet(CSSStyleSheet* sheet)
    {
        auto iter = std::find(m_sheets.begin(), m_sheets.end(), sheet);
        STARFISH_ASSERT(iter != m_sheets.end());
        m_sheets.erase(iter);
    }

    GCVector<CSSStyleSheet*>& sheets()
    {
        return m_sheets;
    }

    CSSStyleSheet* styleSheetWithStyleRules();
    void removeAllRules()
    {
        if (!m_styleSheetWithAllRules) {
            return;
        }
        m_styleSheetWithAllRules = nullptr;
    }

    bool usesFirstLineRule() const
    {
        return m_usesFirstLineRule;
    }

    void resolveDOMStyle(Document* document, bool force = false);

#ifdef STARFISH_ENABLE_TEST
    void dumpDOMStyle(Document* document);
#endif
    ComputedStyle* resolveDocumentStyle(Document* doc);
    ComputedStyle* resolveStyle(Element* node, ComputedStyle* parent);

    void matchAllRules(
        Element* element, ComputedStyle* ret, ComputedStyle* parent,
        PseudoElementType pseudoType = PseudoElementType::PseudoElementNone);
    Match matchSelector(Element* element, AtomicString elementName,
                        AtomicString elementId,
                        const GCVector<AtomicString>& elementClasses,
                        const CSSSelectorList& selectorList, unsigned idx,
                        MatchResult& result, bool isQueryingSelector = false);
    void collectMatchingRulesFromUASheet(
        std::pair<StyleRule*, ResourceURL*>* rules, unsigned ruleCount,
        Element* element, AtomicString elementName, AtomicString elementId,
        const GCVector<AtomicString>& elementClasses,
        MatchedStyleRules<6>& authorRules);
    void collectMatchingRulesFromAuthorSheet(
        std::pair<StyleRule*, ResourceURL*>* rules, unsigned ruleCount,
        Element* element, AtomicString elementName, AtomicString elementId,
        const GCVector<AtomicString>& elementClasses,
        MatchedStyleRules<16>& authorRules, ComputedStyle* ret,
        PseudoElementType pseudoElementType);

    const MediaQueryEvaluator& mediaQueryEvaluator();

    MediaQueryResultList& viewportDependentMediaQueryResults()
    {
        return m_viewportDependentMediaQueryResults;
    }

    MediaQueryResultList& deviceDependentMediaQueryResults()
    {
        return m_deviceDependentMediaQueryResults;
    }

    bool mediaQueryAffectedByViewportChange();
    bool mediaQueryAffectedByDeviceChange();

protected:
    void apply(Element* element,
               const GCAtomicVector<CSSStyleValuePair>& cssValues,
               ResourceURL* origin, ComputedStyle* style,
               ComputedStyle* parentStyle, bool isImportant = false);

    Match matchForRelation(Element* element, AtomicString elementName,
                           AtomicString elementId,
                           const GCVector<AtomicString>& elementClasses,
                           const CSSSelectorList& selectorList,
                           CSSSelector::RelationType relation, unsigned idx,
                           MatchResult& result);

    ALWAYS_INLINE bool checkOne(Element* element, AtomicString elementName,
                                AtomicString elementId,
                                const GCVector<AtomicString>& elementClasses,
                                CSSSelector* selector, MatchResult& result,
                                bool isQueryingSelector = false);
    bool checkPseudoClass(Element* element, CSSPseudoSelector* selector,
                          MatchResult& result);
    bool checkPseudoElement(Element* element, CSSPseudoSelector* selector,
                            MatchResult& result);
    bool anyAttributeMatches(Element* element, CSSSelector::Type type,
                             CSSAttributeSelector* selector,
                             MatchResult& result);
    bool tryAddSheet(Node* node, CSSStyleSheet* sheet);
    bool traverseAndTryAddSheet(Node* node, CSSStyleSheet* sheet,
                                bool& originFound);

    float m_mediumFontSize;
    GCVector<CSSStyleSheet*> m_sheets;
    CSSStyleSheet* m_styleSheetWithAllRules;
    bool m_usesFirstLineRule;
    MediaQueryEvaluator* m_mediaQueryEvaluator;
    MediaQueryResultList m_viewportDependentMediaQueryResults;
    MediaQueryResultList m_deviceDependentMediaQueryResults;
};
}

#endif
