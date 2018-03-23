/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Alexey Proskuryakov <ap@webkit.org>
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

#ifndef __StarFishStyle__
#define __StarFishStyle__

#include "StaticStrings.h"
#include "binding/DocumentHoldable.h"
#include "core/style/NamedColors.h"
#include "core/style/MediaQueryEvaluator.h"
#include "core/style/Length.h"
#include "core/style/GridLength.h"
#include "core/style/RectData.h"
#include "core/style/TextOverflowData.h"
#include "core/util/VectorWithInlineStorage.h"
#include "core/util/BloomFilter.h"

namespace StarFish {

class AncestorSelectorFilter;
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
class RuleSet;
class MutablePropertyValue;
class CSSCounterFunction;

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
    enum Kind { PX, EM, EX, IN, CM, MM, PT, PC, VW, VH, VMIN, VMAX, REM, CH };

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
        } else if (unit->equals("ch")) {
            m_kind = CH;
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
        } else if (m_kind == CH) { // font-relative length
            return Length(Length::Ch, m_value);
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
        } else if (m_kind == CH) {
            return String::fromUTF8(stdStr.append("ch").c_str());
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

    CSSTime(String* str, float f)
    {
        if (str->length() == 0 || str->equals("s")) {
            m_kind = S;
        } else if (str->equals("ms")) {
            m_kind = MS;
        }
        m_value = f;
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

// inline | block | list-item | inline-list-item | inline-block | table |
// inline-table | table-row-group | table-header-group | table-footer-group |
// table-row | table-column-group | table-column | table-cell | table-caption |
// flex | inline-flex | none | inherit
enum DisplayValue {
    InlineDisplayValue, // initial value
    BlockDisplayValue,
    ListItemDisplayValue,
    InlineListItemDisplayValue,
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
    GridDisplayValue,
    InlineGridDisplayValue,
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
    NoneClearValue = 0,
    LeftClearValue = 1,
    RightClearValue = 1 << 1,
    BothClearValue = 1 | (1 << 1),
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
    StretchAlignItemValue,
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
    CenterTextAlignValue,
    StarFishCenterTextAlignValue,
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

enum BackgroundSizeValue {
    CoverBackgroundSizeValue = 1,
    ContainBackgroundSizeValue,
    BackgroundSizeValueEnd = ContainBackgroundSizeValue,
};

enum BackgroundRepeatValue {
    RepeatRepeatValue,
    NoRepeatRepeatValue,
};

enum MaskSizeValue {
    CoverMaskSizeValue,
    ContainMaskSizeValue,
};

// Because padding-box is not supported in box-sizing property, so we make
// another enum.
enum BoxValue {
    BorderBoxBoxValue,
    PaddingBoxBoxValue,
    ContentBoxBoxValue,
};

enum BackgroundAttachmentValue {
    ScrollBackgroundAttachmentValue,
    FixedBackgroundAttachmentValue,
    LocalBackgroundAttachmentValue,
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
    HiddenBorderStyleValue,
    SolidBorderStyleValue,
    DashedBorderStyleValue,
    DottedBorderStyleValue,
    DoubleBorderStyleValue,
    InsetBorderStyleValue,
    OutsetBorderStyleValue,
    GrooveBorderStyleValue,
    RidgeBorderStyleValue,
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

enum TextDecorationLineValue {
    NoneTextDecorationLineValue,
    UnderlineTextDecorationLineValue,
    OverlineTextDecorationLineValue,
    LineThroughTextDecorationLineValue,
    BlinkTextDecorationLineValue,
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
    CollapseVisibilityValue,
    HiddenVisibilityValue,
};

enum UnicodeBidiValue {
    NormalUnicodeBidiValue,
    EmbedUnicodeBidiValue,
    IsolateUnicodeBidiValue,
};

enum ObjectFitValue {
    FillObjectFitValue,
    ContainObjectFitValue,
    CoverObjectFitValue,
    NoneObjectFitValue,
    ScaledownObjectFitValue,
};

enum ImageRenderingValue {
    ImageRenderingAutoValue,
    ImageRenderingCrispEdgesValue,
    ImageRenderingPixelatedValue,
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
    TransitionPropertyTextShadowValue,
    TransitionPropertyTransformValue,
    TransitionPropertyTransformOriginValue,
    TransitionPropertyTopValue,
    TransitionPropertyVerticalAlignValue,
    TransitionPropertyVisibilityValue,
    TransitionPropertyWidthValue,
    TransitionPropertyWordSpacingValue,
    TransitionPropertyZIndexValue,
};

String* transitionPropertyValueToString(TransitionPropertyValue val);

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

enum TextTransformValue {
    NoneTextTransformValue,
    CapitalizeTextTransformValue,
    UppercaseTextTransformValue,
    LowercaseTextTransformValue,
};

enum ListStylePositionValue {
    ListStylePositionOutside, // Default
    ListStylePositionInside,
};

enum UserSelectValue {
    NoneUserSelectValue,
    TextUserSelectValue,
    ContainUserSelectValue,
    AllUserSelectValue,
};

enum HyphensValue {
    NoneHyphensValue,
    ManualHyphensValue,
};

class ValueList;
class ValuePair;
class FontFaceSrcData;
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
//
// The order of followings are used to tell which properties should be
// calculated after layout.
// * padding-[top|bottom|left|right]
// * margin-[top|bottom|left|right]
// * top|bottom|left|right
// * width|height
// * border--[top|bottom|left|right]-width
#define FOR_EACH_STYLE_ATTRIBUTE_BASIC(F)                                    \
    F(Color, color, "color")                                                 \
    F(Direction, direction, "direction")                                     \
    F(BackgroundColor, backgroundColor, "background-color")                  \
    F(BackgroundImage, backgroundImage, "background-image")                  \
    F(BackgroundSize, backgroundSize, "background-size")                     \
    F(BackgroundAttachment, backgroundAttachment, "background-attachment")   \
    F(BackgroundClip, backgroundClip, "background-clip")                     \
    F(BackgroundOrigin, backgroundOrigin, "background-origin")               \
    F(BackgroundRepeatX, backgroundRepeatX, "background-repeat-x")           \
    F(BackgroundRepeatY, backgroundRepeatY, "background-repeat-y")           \
    F(BackgroundPositionX, backgroundPositionX, "background-position-x")     \
    F(BackgroundPositionY, backgroundPositionY, "background-position-y")     \
    F(CounterReset, counterReset, "counter-reset")                           \
    F(CounterIncrement, counterIncrement, "counter-increment")               \
    F(LineHeight, lineHeight, "line-height")                                 \
    F(WhiteSpace, whiteSpace, "white-space")                                 \
    F(WordSpacing, wordSpacing, "word-spacing")                              \
    F(PaddingTop, paddingTop, "padding-top")                                 \
    F(PaddingRight, paddingRight, "padding-right")                           \
    F(PaddingBottom, paddingBottom, "padding-bottom")                        \
    F(PaddingLeft, paddingLeft, "padding-left")                              \
    F(MarginTop, marginTop, "margin-top")                                    \
    F(MarginRight, marginRight, "margin-right")                              \
    F(MarginBottom, marginBottom, "margin-bottom")                           \
    F(MarginLeft, marginLeft, "margin-left")                                 \
    F(Top, top, "top")                                                       \
    F(Bottom, bottom, "bottom")                                              \
    F(Left, left, "left")                                                    \
    F(Right, right, "right")                                                 \
    F(Width, width, "width")                                                 \
    F(Height, height, "height")                                              \
    F(MaxWidth, maxWidth, "max-width")                                       \
    F(MinWidth, minWidth, "min-width")                                       \
    F(MaxHeight, maxHeight, "max-height")                                    \
    F(MinHeight, minHeight, "min-height")                                    \
    F(WordWrap, wordWrap, "word-wrap")                                       \
    F(OverflowWrap, overflowWrap, "overflow-wrap")                           \
    F(Position, position, "position")                                        \
    F(TextDecoration, textDecoration, "text-decoration")                     \
    F(TextDecorationLine, textDecorationLine, "text-decoration-line")        \
    F(TextDecorationColor, textDecorationColor, "text-decoration-color")     \
    F(Display, display, "display")                                           \
    F(Float, float, "float")                                                 \
    F(Clear, clear, "clear")                                                 \
    F(BorderImageSlice, borderImageSlice, "border-image-slice")              \
    F(BorderImageSource, borderImageSource, "border-image-source")           \
    F(BorderImageRepeat, borderImageRepeat, "border-image-repeat")           \
    F(BorderImageWidth, borderImageWidth, "border-image-width")              \
    F(BorderTopColor, borderTopColor, "border-top-color")                    \
    F(BorderRightColor, borderRightColor, "border-right-color")              \
    F(BorderBottomColor, borderBottomColor, "border-bottom-color")           \
    F(BorderLeftColor, borderLeftColor, "border-left-color")                 \
    F(BorderTopStyle, borderTopStyle, "border-top-style")                    \
    F(BorderRightStyle, borderRightStyle, "border-right-style")              \
    F(BorderBottomStyle, borderBottomStyle, "border-bottom-style")           \
    F(BorderLeftStyle, borderLeftStyle, "border-left-style")                 \
    F(BorderTopWidth, borderTopWidth, "border-top-width")                    \
    F(BorderRightWidth, borderRightWidth, "border-right-width")              \
    F(BorderBottomWidth, borderBottomWidth, "border-bottom-width")           \
    F(BorderLeftWidth, borderLeftWidth, "border-left-width")                 \
    F(BorderCollapse, borderCollapse, "border-collapse")                     \
    F(BorderSpacing, borderSpacing, "border-spacing")                        \
    F(CaptionSide, CaptionSide, "caption-side")                              \
    F(EmptyCells, EmptyCells, "empty-cells")                                 \
    F(TextAlign, textAlign, "text-align")                                    \
    F(TextIndent, textIndent, "text-indent")                                 \
    F(TextShadow, textShadow, "text-shadow")                                 \
    F(TextTransform, textTransform, "text-transform")                        \
    F(Transform, transform, "transform")                                     \
    F(TransformOrigin, transformOrigin, "transform-origin")                  \
    F(Visibility, visibility, "visibility")                                  \
    F(ObjectFit, objectFit, "object-fit")                                    \
    F(ObjectPosition, objectPosition, "object-position")                     \
    F(OverflowX, overflowX, "overflow-x")                                    \
    F(OverflowY, overflowY, "overflow-y")                                    \
    F(ZIndex, zIndex, "z-index")                                             \
    F(VerticalAlign, verticalAlign, "vertical-align")                        \
    F(Opacity, opacity, "opacity")                                           \
    F(TableLayout, tableLayout, "table-layout")                              \
    F(UnicodeBidi, unicodeBidi, "unicode-bidi")                              \
    F(Content, content, "content")                                           \
    F(TransitionProperty, transitionProperty, "transition-property")         \
    F(TransitionDuration, transitionDuration, "transition-duration")         \
    F(TransitionTimingFunction, transitionTimingFunction,                    \
      "transition-timing-function")                                          \
    F(TransitionDelay, transitionDelay, "transition-delay")                  \
    F(BoxShadow, boxShadow, "box-shadow")                                    \
    F(BoxSizing, boxSizing, "box-sizing")                                    \
    F(Fill, fill, "fill")                                                    \
    F(FillOpacity, fillOpacity, "fill-opacity")                              \
    F(FillRule, fillRule, "fill-rule")                                       \
    F(Stroke, stroke, "stroke")                                              \
    F(StrokeWidth, strokeWidth, "stroke-width")                              \
    F(X, x, "x")                                                             \
    F(Y, y, "y")                                                             \
    F(R, r, "r")                                                             \
    F(RX, rx, "rx")                                                          \
    F(RY, ry, "ry")                                                          \
    F(CX, cx, "cx")                                                          \
    F(CY, cy, "cy")                                                          \
    F(FlexDirection, flexDirection, "flex-direction")                        \
    F(FlexWrap, flexWrap, "flex-wrap")                                       \
    F(Order, order, "order")                                                 \
    F(JustifyContent, justifyContent, "justify-content")                     \
    F(AlignItems, alignItems, "align-items")                                 \
    F(AlignSelf, alignSelf, "align-self")                                    \
    F(AlignContent, alignContent, "align-content")                           \
    F(FlexGrow, flexGrow, "flex-grow")                                       \
    F(FlexShrink, flexShrink, "flex-shrink")                                 \
    F(FlexBasis, flexBasis, "flex-basis")                                    \
    F(OutlineColor, outlineColor, "outline-color")                           \
    F(OutlineStyle, outlineStyle, "outline-style")                           \
    F(OutlineWidth, outlineWidth, "outline-width")                           \
    F(OutlineOffset, outlineOffset, "outline-offset")                        \
    F(BorderTopLeftRadius, borderTopLeftRadius, "border-top-left-radius")    \
    F(BorderTopRightRadius, borderTopRightRadius, "border-top-right-radius") \
    F(BorderBottomRightRadius, borderBottomRightRadius,                      \
      "border-bottom-right-radius")                                          \
    F(BorderBottomLeftRadius, borderBottomLeftRadius,                        \
      "border-bottom-left-radius")                                           \
    F(Cursor, cursor, "cursor")                                              \
    F(MaskImage, maskImage, "mask-image")                                    \
    F(MaskSize, maskSize, "mask-size")                                       \
    F(FontSize, fontSize, "font-size")                                       \
    F(FontWeight, fontWeight, "font-weight")                                 \
    F(FontStyle, fontStyle, "font-style")                                    \
    F(ListStylePosition, listStylePosition, "list-style-position")           \
    F(ListStyleImage, listStyleImage, "list-style-image")                    \
    F(ListStyleType, listStyleType, "list-style-type")                       \
    F(Clip, clip, "clip")                                                    \
    F(LetterSpacing, letterSpacing, "letter-spacing")                        \
    F(UserSelect, userSelect, "user-select")                                 \
    F(GridTemplateColumns, gridTemplateColumns, "grid-template-columns")     \
    F(GridTemplateRows, gridTemplateRows, "grid-template-rows")              \
    F(CaretColor, caretColor, "caret-color")                                 \
    F(FontKerning, fontKerning, "font-kerning")                              \
    F(ImageRendering, imageRendering, "image-rendering")                     \
    F(TextOverflow, textOverflow, "text-overflow")                           \
    F(Hyphens, hyphens, "hyphens")

// font related properties must be followed end of this
// define(FOR_EACH_STYLE_ATTRIBUTE)
// This order is used by CSSParser::parseFontFaceRule

// sticky properties
#define FOR_EACH_STYLE_ATTRIBUTE(F)          \
    FOR_EACH_STYLE_ATTRIBUTE_BASIC(F)        \
    F(D, d, "d")                             \
    F(FontFamily, fontFamily, "font-family") \
    F(Src, src, "src")

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
    F(BorderRadius, borderRadius, "border-radius")                   \
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
    F(Flex, flex, "flex")                                            \
    F(ListStyle, listStyle, "list-style")

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

class CSSTransformFunctions : public GCVector<CSSTransformFunction> {
public:
    String* toString();
};

class CSSStyleValuePair : public gc {
    friend class ValueList;
    friend class ValuePair;

public:
    enum KeyKind {
        Empty,
#define ADD_CSS_KEYKIND(Name, name, cssname) Name,
        FOR_EACH_STYLE_ATTRIBUTE(ADD_CSS_KEYKIND)
#undef ADD_CSS_KEYKIND
            FontKeyKindStart = FontSize,
        FontKeyKindEnd = FontFamily,
        VarValue,
        KeyKindSize,
    };
    // font related properties must be followed end of this enum(KeyKind)
    // This order is used by CSSParser::parseFontFaceRule

    enum ValueKind {
        Initial,
        Inherit,
        Unset,
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
        AtomicStringValueKind,
        KeywordValueKind,
        ColorValueKind,
        NamedColorValueKind,
        UrlValueKind,

        CalcValueKind,

        FontFaceSrcDataValueKind,

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
        ValuePairKind,

        // Background
        BackgroundSizeValueKind,
        BackgroundRepeatValueKind,
        BackgroundAttachmentValueKind,
        BoxValueKind,

        MaskSizeValueKind,

        FontSizeValueKind,
        FontStyleValueKind,
        FontWeightValueKind,
        FontKerningValueKind,
        WordWrapValueKind,

        BorderStyleValueKind,
        BorderWidthValueKind,
        BorderImageRepeatValueKind,

        // table
        BorderCollapseValueKind,
        CaptionSideValueKind,
        TableLayoutValueKind,
        EmptyCellsValueKind,

        OverflowValueKind,
        TextDecorationLineValueKind,
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
        FillRuleValueKind,

        // text-transform
        TextTransformValueKind,

        // object-fit
        ObjectFitValueKind,
        ObjectPositionValueKind,

        ListStylePositionValueKind,
        CounterFunctionValueKind,

        // rect for clip
        RectValueKind,

        // user-select
        UserSelectValueKind,

        // grid
        GridTemplateUnits,

        // img
        ImageRenderingValueKind,

        // text-overflow
        TextOverflowValueKind,

        HyphensValueKind,
        VarFunctionValueKind
    };

    CSSStyleValuePair()
        : m_keyKind(KeyKind::Empty)
        , m_temporaryKeyKind(KeyKind::Empty)
        , m_valueKind(ValueKind::None)
        , m_flagImportant(false)
        , m_value(0.0f)
    {
    }

    CSSStyleValuePair(const CSSStyleValuePair& o)
        : m_keyKind(o.m_keyKind)
        , m_temporaryKeyKind(o.m_temporaryKeyKind)
        , m_valueKind(o.m_valueKind)
        , m_flagImportant(o.m_flagImportant)
        , m_value(o.m_value)
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

    KeyKind temporaryKeyKind() const
    {
        return m_temporaryKeyKind;
    }

    void setTemporaryKeyKind(KeyKind kind)
    {
        m_temporaryKeyKind = kind;
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

    bool isSideValueKind() const
    {
        return m_valueKind == SideValueKind;
    }

    bool flagImportant() const
    {
        return m_flagImportant;
    }

    void setFlagImportant(bool isImportant)
    {
        m_flagImportant = isImportant;
    }

    bool updateVarValue(const char*, const CSSTokenVector&);

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

    FontKerningValue fontKerningValue() const
    {
        STARFISH_ASSERT(m_valueKind == FontKerningValueKind);
        return m_value.m_fontKerning;
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

    ObjectFitValue objectFitValue() const
    {
        STARFISH_ASSERT(m_valueKind == ObjectFitValueKind);
        return m_value.m_objectFit;
    }

    UnicodeBidiValue unicodeBidiValue() const
    {
        STARFISH_ASSERT(m_valueKind == UnicodeBidiValueKind);
        return m_value.m_unicodeBidi;
    }

    BorderImageRepeatValue borderImageRepeatValue() const
    {
        STARFISH_ASSERT(m_valueKind == BorderImageRepeatValueKind);
        return m_value.m_borderImageRepeat;
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

    ImageRenderingValue imageRenderingValue() const
    {
        STARFISH_ASSERT(m_valueKind == ImageRenderingValueKind);
        return m_value.m_imageRendering;
    }

    CSSLength cssLengthValue() const
    {
        STARFISH_ASSERT(m_valueKind == Length);
        return m_value.m_length;
    }

    ::StarFish::Length lengthValue() const
    {
        if (m_valueKind == Length) {
            return m_value.m_length.toLength();
        } else {
            STARFISH_ASSERT(m_valueKind == Calc);
            return ::StarFish::Length(calcValue());
        }
    }

    ::StarFish::Length toLengthValue() const
    {
        if (m_valueKind == Length) {
            return m_value.m_length.toLength();
        } else if (m_valueKind == Percentage) {
            return ::StarFish::Length(::StarFish::Length::Percent,
                                      percentageValue());
        } else {
            STARFISH_ASSERT(m_valueKind == Calc);
            return ::StarFish::Length(calcValue());
        }
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

    const AtomicString& atomicStringValue() const
    {
        STARFISH_ASSERT(m_valueKind == AtomicStringValueKind);
        return m_value.m_atomicStringValue;
    }

    void setAtomicStringValue(AtomicString& v)
    {
        m_valueKind = AtomicStringValueKind;
        m_value.m_atomicStringValue = v;
    }

    String* keywordValue() const
    {
        STARFISH_ASSERT(m_valueKind == KeywordValueKind);
        return m_value.m_stringValue;
    }

    void setKeywordValue(String* v)
    {
        m_valueKind = KeywordValueKind;
        m_value.m_stringValue = v;
    }

    BackgroundSizeValue backgroundSizeValue() const
    {
        STARFISH_ASSERT(m_valueKind == BackgroundSizeValueKind);
        return m_value.m_backgroundSize;
    }

    BackgroundRepeatValue backgroundRepeatValue() const
    {
        STARFISH_ASSERT(m_valueKind == BackgroundRepeatValueKind);
        return m_value.m_backgroundRepeat;
    }

    BackgroundAttachmentValue backgroundAttachmentValue() const
    {
        STARFISH_ASSERT(m_valueKind == BackgroundAttachmentValueKind);
        return m_value.m_backgroundAttachment;
    }

    ValueList* multiValue() const
    {
        STARFISH_ASSERT(m_valueKind == ValueListKind);
        return m_value.m_multiValue;
    }

    ValuePair* pairValue() const
    {
        STARFISH_ASSERT(m_valueKind == ValuePairKind);
        return m_value.m_pairValue;
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

    TextDecorationLineValue textDecorationValue() const
    {
        STARFISH_ASSERT(m_valueKind == TextDecorationLineValueKind);
        return m_value.m_textDecoration;
    }

    TextDecorationLineValue textDecorationLineValue() const
    {
        STARFISH_ASSERT(m_valueKind == TextDecorationLineValueKind);
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

    BoxValue boxValue() const
    {
        STARFISH_ASSERT(m_valueKind == BoxValueKind);
        return m_value.m_box;
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

    TextTransformValue textTransformValue() const
    {
        STARFISH_ASSERT(m_valueKind == TextTransformValueKind);
        return m_value.m_textTransform;
    }

    FontFaceSrcData* fontFaceSrcDataValue() const
    {
        STARFISH_ASSERT(m_valueKind == FontFaceSrcDataValueKind);
        return m_value.m_fontFaceSrcData;
    }

    MaskSizeValue maskSizeValue() const
    {
        STARFISH_ASSERT(m_valueKind == MaskSizeValueKind);
        return m_value.m_maskSize;
    }

    ListStylePositionValue listStylePositionValue() const
    {
        STARFISH_ASSERT(m_valueKind == ListStylePositionValueKind);
        return m_value.m_listStylePosition;
    }

    RectData* clip() const
    {
        STARFISH_ASSERT(m_valueKind == RectValueKind);
        return m_value.m_rect;
    }

    UserSelectValue userSelectValue() const
    {
        STARFISH_ASSERT(m_valueKind == UserSelectValueKind);
        return m_value.m_userSelect;
    }

    HyphensValue hyphensValue() const
    {
        STARFISH_ASSERT(m_valueKind == HyphensValueKind);
        return m_value.m_hyphens;
    }

    GCVector<GridLength>* gridTemplateUnits() const
    {
        STARFISH_ASSERT(m_valueKind == GridTemplateUnits);
        return m_value.m_gridTemplateUnits;
    }

    CSSCounterFunction* counterFunctionValue() const
    {
        STARFISH_ASSERT(m_valueKind == CounterFunctionValueKind);
        return m_value.m_counterFunctionValue;
    }

    TextOverflowData textOverflowValue() const
    {
        STARFISH_ASSERT(m_valueKind == TextOverflowValueKind);
        return *m_value.m_textOverflowData;
    }

    String* varFunctionValue() const
    {
        STARFISH_ASSERT(m_valueKind == VarFunctionValueKind);
        return m_value.m_stringValue;
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
        FontKerningValue m_fontKerning;
        WordWrapValue m_wordWrap;
        TextAlignValue m_textAlign;
        SideValue m_side;
        DirectionValue m_direction;
        WhiteSpaceValue m_whiteSpace;
        CSSLength m_length;
        CSSAngle m_angle;
        String* m_stringValue;
        AtomicString m_atomicStringValue;
        BackgroundSizeValue m_backgroundSize;
        BoxValue m_box;
        BackgroundRepeatValue m_backgroundRepeat;
        BackgroundAttachmentValue m_backgroundAttachment;
        BorderImageRepeatValue m_borderImageRepeat;
        BorderStyleValue m_borderStyle;
        BorderWidthValue m_borderWidth;
        ImageRenderingValue m_imageRendering;
        ValueList* m_multiValue;
        ValuePair* m_pairValue;
        FontFaceSrcData* m_fontFaceSrcData;
        OverflowValue m_overflow;
        VisibilityValue m_visibility;
        UnicodeBidiValue m_unicodeBidi;
        TextDecorationLineValue m_textDecoration;
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
        TextTransformValue m_textTransform;
        MaskSizeValue m_maskSize;
        ObjectFitValue m_objectFit;
        ListStylePositionValue m_listStylePosition;
        UserSelectValue m_userSelect;
        HyphensValue m_hyphens;
        RectData* m_rect;
        GCVector<GridLength>* m_gridTemplateUnits;
        CSSCounterFunction* m_counterFunctionValue;
        TextOverflowData* m_textOverflowData;

        ValueData(int v)
            : m_int32Value(v)
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
        ValueData(FontKerningValue v)
            : m_fontKerning(v)
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
        ValueData(AtomicString& v)
            : m_atomicStringValue(v)
        {
        }
        ValueData(BackgroundSizeValue v)
            : m_backgroundSize(v)
        {
        }
        ValueData(BoxValue v)
            : m_box(v)
        {
        }
        ValueData(BackgroundRepeatValue v)
            : m_backgroundRepeat(v)
        {
        }
        ValueData(BackgroundAttachmentValue v)
            : m_backgroundAttachment(v)
        {
        }
        ValueData(BorderImageRepeatValue v)
            : m_borderImageRepeat(v)
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
        ValueData(ValuePair* v)
            : m_pairValue(v)
        {
        }
        ValueData(FontFaceSrcData* v)
            : m_fontFaceSrcData(v)
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
        ValueData(ImageRenderingValue v)
            : m_imageRendering(v)
        {
        }
        ValueData(TextDecorationLineValue v)
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
        ValueData(TextTransformValue v)
            : m_textTransform(v)
        {
        }
        ValueData(MaskSizeValue v)
            : m_maskSize(v)
        {
        }
        ValueData(ObjectFitValue v)
            : m_objectFit(v)
        {
        }

        ValueData(ListStylePositionValue v)
            : m_listStylePosition(v)
        {
        }

        ValueData(UserSelectValue v)
            : m_userSelect(v)
        {
        }

        ValueData(HyphensValue v)
            : m_hyphens(v)
        {
        }

        ValueData(RectData* v)
            : m_rect(v)
        {
        }

        ValueData(GCVector<GridLength>* v)
            : m_gridTemplateUnits(v)
        {
        }

        ValueData(CSSCounterFunction* v)
            : m_counterFunctionValue(v)
        {
        }

        ValueData(TextOverflowData* v)
            : m_textOverflowData(v)
        {
        }
    };

    CSSStyleValuePair(ValueKind kind, ValueData value)
        : m_keyKind(KeyKind::Empty)
        , m_temporaryKeyKind(KeyKind::Empty)
        , m_valueKind(kind)
        , m_flagImportant(false)
        , m_value(value)
    {
    }

    void* pointerValue()
    {
        switch (m_valueKind) {
        case UrlValueKind:
        case StringValueKind:
        case KeywordValueKind:
        case Attr:
        case VarFunctionValueKind:
            return m_value.m_stringValue;
        case ValueListKind:
            return m_value.m_multiValue;
        case TransformFunctions:
            return m_value.m_transforms;
        case CalcValueKind:
            return m_value.m_calc;
        case FontFaceSrcDataValueKind:
            return m_value.m_fontFaceSrcData;
        case ValuePairKind:
            return m_value.m_pairValue;
        case RectValueKind:
            return m_value.m_rect;
        case GridTemplateUnits:
            return m_value.m_gridTemplateUnits;
        case CounterFunctionValueKind:
            return m_value.m_counterFunctionValue;
        case TextOverflowValueKind:
            return m_value.m_textOverflowData;
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

    void setInt32Value(int32_t val)
    {
        m_valueKind = CSSStyleValuePair::ValueKind::Int32;
        m_value.m_int32Value = val;
    }

    void setNumberValue(float val)
    {
        m_valueKind = CSSStyleValuePair::ValueKind::Number;
        m_value.m_floatValue = val;
    }

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

    void setCalcValue(CalcData* val)
    {
        m_valueKind = CSSStyleValuePair::ValueKind::CalcValueKind;
        m_value.m_calc = val;
    }

    void setAngleValue(CSSAngle val)
    {
        m_valueKind = CSSStyleValuePair::ValueKind::Angle;
        m_value.m_angle = val;
    }

    void setTimeValue(CSSTime val)
    {
        m_valueKind = CSSStyleValuePair::ValueKind::Time;
        m_value.m_time = val;
    }

    void setColorValue(Unit::Color val)
    {
        m_valueKind = CSSStyleValuePair::ValueKind::ColorValueKind;
        m_value.m_color = val;
    }

    void setNamedColorValue(NamedColor::NamedColorValue val)
    {
        m_valueKind = CSSStyleValuePair::ValueKind::NamedColorValueKind;
        m_value.m_namedColor = val;
    }

    void setUrlValue(String* val)
    {
        m_valueKind = CSSStyleValuePair::ValueKind::UrlValueKind;
        m_value.m_stringValue = val;
    }

    void setBackgroundSizeValue(BackgroundSizeValue val)
    {
        m_valueKind = CSSStyleValuePair::ValueKind::BackgroundSizeValueKind;
        m_value.m_backgroundSize = val;
    }

    void setBackgroundRepeatValue(BackgroundRepeatValue val)
    {
        m_valueKind = CSSStyleValuePair::ValueKind::BackgroundRepeatValueKind;
        m_value.m_backgroundRepeat = val;
    }

    void setBackgroundAttachmentValue(BackgroundAttachmentValue val)
    {
        m_valueKind =
            CSSStyleValuePair::ValueKind::BackgroundAttachmentValueKind;
        m_value.m_backgroundAttachment = val;
    }

    void setBorderImageRepeatValue(BorderImageRepeatValue val)
    {
        m_valueKind = CSSStyleValuePair::ValueKind::BorderImageRepeatValueKind;
        m_value.m_borderImageRepeat = val;
    }

    void setBorderStyleValue(BorderStyleValue val)
    {
        m_valueKind = CSSStyleValuePair::ValueKind::BorderStyleValueKind;
        m_value.m_borderStyle = val;
    }

    void setBoxValue(BoxValue value)
    {
        m_valueKind = CSSStyleValuePair::ValueKind::BoxValueKind;
        m_value.m_box = value;
    }

    void setValueList(ValueList* val)
    {
        m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
        m_value.m_multiValue = val;
    }

    void setValuePair(ValuePair* val)
    {
        m_valueKind = CSSStyleValuePair::ValueKind::ValuePairKind;
        m_value.m_pairValue = val;
    }

    void setFontFaceSrcData(FontFaceSrcData* val)
    {
        m_valueKind = CSSStyleValuePair::ValueKind::FontFaceSrcDataValueKind;
        m_value.m_fontFaceSrcData = val;
    }

    void setClipData(RectData* val)
    {
        m_valueKind = CSSStyleValuePair::ValueKind::RectValueKind;
        m_value.m_rect = val;
    }

    void setGridTemplateUnits(GCVector<GridLength>* val)
    {
        m_valueKind = CSSStyleValuePair::ValueKind::GridTemplateUnits;
        m_value.m_gridTemplateUnits = val;
    }

    void setCounterFunctionValue(CSSCounterFunction* v)
    {
        m_valueKind = CSSStyleValuePair::CounterFunctionValueKind;
        m_value.m_counterFunctionValue = v;
    }

    void setVarFunctionValue(String* v)
    {
        m_valueKind = VarFunctionValueKind;
        m_value.m_stringValue = v;
    }

#define NEW_SET_VALUE_DECL(name, ...) \
    bool updateValue##name(Document* document, const CSSTokenVector& tokens);
    FOR_EACH_STYLE_ATTRIBUTE_BASIC(NEW_SET_VALUE_DECL)
#undef NEW_SET_VALUE_DECL
    bool updateValueFontFamily(const CSSTokenVector& tokens);
    bool updateValueSrc(const CSSTokenVector& tokens);

    bool updateValueNumber(const CSSTokenVector& tokens, uint8_t option);
    bool updateValueUnitNumber(const CSSTokenValue& token, uint8_t option);
    enum CalcParserOption { LengthParser = 0, AngleParser = 1, TimeParser = 2 };
    bool updateValueUnitCalc(const CSSTokenValue& token, uint8_t parserOption,
                             uint8_t lengthOption);
    bool updateValueLength(const CSSTokenVector& tokens, uint8_t option);
    bool updateValueUnitLength(const CSSTokenValue& token, uint8_t option);
    bool updateValueUnitLengthOrCalc(const CSSTokenValue& token,
                                     uint8_t option);
    bool updateValueTime(const CSSTokenVector& tokens, uint8_t option);
    bool updateValueUnitTime(const CSSTokenValue& token, uint8_t option);
    bool updateValueUnitTimeOrCalc(const CSSTokenValue& token, uint8_t option);
    bool updateValueAngle(const CSSTokenVector& tokens, uint8_t option);
    bool updateValueUnitAngle(const CSSTokenValue& token, uint8_t option);
    bool updateValueUnitAngleOrCalc(const CSSTokenValue& token, uint8_t option);
    bool updateValueUnitBox(const CSSTokenValue& token);
    bool updateValueBackgroundImage(const CSSTokenVector& tokens,
                                    bool allowComma);
    bool updateValueBackgroundSize(const CSSTokenVector& tokens,
                                   bool allowComma);
    bool updateValueBackgroundAttachment(const CSSTokenVector& tokens,
                                         bool allowComma);
    bool updateValueBox(const CSSTokenVector& tokens, bool allowComma);
    bool updateValueUnitBackgroundRepeat(const CSSTokenValue& token);
    bool updateValueUnitBackgroundPositionX(const CSSTokenValue& token);
    bool updateValueUnitBackgroundPositionY(const CSSTokenValue& token);
    bool updateValueUnitBackgroundAttachment(const CSSTokenValue& token);
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
    bool updateValueUnitOverflowX(const CSSTokenValue& value);
    bool updateValueUnitOverflowY(const CSSTokenValue& value);
    bool updateValueUnitFlexDirection(const CSSTokenValue& value);
    bool updateValueUnitFlexWrap(const CSSTokenValue& value);
    bool updateValueUnitAlignItem(const CSSTokenValue& value);
    bool updateValueUnitFlexGrow(const CSSTokenValue& value);
    bool updateValueUnitFlexShrink(const CSSTokenValue& value);
    bool updateValueUnitFlexBasis(const CSSTokenValue& value);
    bool updateValueUnitWordSpacing(const CSSTokenValue& value);

    bool updateValueTransform(const CSSTokenVector& tokens, bool canIgnoreUnit);
    bool updateValueObjectPosition(const CSSTokenVector& tokens,
                                   CSSStyleValuePair& xPair,
                                   CSSStyleValuePair& yPair);
    bool updateValueShadow(const CSSTokenVector& tokens, bool boxShadow);

protected:
    KeyKind m_keyKind : 8;
    KeyKind m_temporaryKeyKind : 8;
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

    GCAtomicVector<CSSStyleValuePair>& mutableData()
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

class ValuePair : public gc {
public:
    ValuePair(const CSSStyleValuePair& first, const CSSStyleValuePair& second)
        : m_first(first)
        , m_second(second)
    {
    }

    void* operator new(size_t size)
    {
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word obj_bitmap[GC_BITMAP_SIZE(ValuePair)] = { 0 };
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(ValuePair, m_first));
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(ValuePair, m_second));
            descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(ValuePair));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new[](size_t size) = delete;

    const CSSStyleValuePair& first()
    {
        return m_first;
    }

    const CSSStyleValuePair& second()
    {
        return m_second;
    }

    String* toString()
    {
        StringBuilder builder;
        builder.appendString(m_first.toString());
        builder.appendChar(' ');
        builder.appendString(m_second.toString());

        return builder.finalize();
    }

private:
    CSSStyleValuePair m_first;
    CSSStyleValuePair m_second;
};

class ValueList : protected GCAtomicVector<CSSStyleValuePair> {
public:
    enum Separator {
        None,
        SpaceSeparator,
        CommaSeparator,
        CommaSeparatorAppendQuoteWhenMeetWhiteSpace,
        SlashSeparator
    };

    ValueList()
        : GCAtomicVector<CSSStyleValuePair>()
        , m_separator(None)
    {
    }

    ValueList(Separator sep)
        : GCAtomicVector<CSSStyleValuePair>()
        , m_separator(sep)
    {
    }

    void push_back(CSSStyleValuePair p)
    {
        GCAtomicVector<CSSStyleValuePair>::push_back(p);
        rootPointer(p);
    }

    void pushBack(CSSStyleValuePair p)
    {
        GCAtomicVector<CSSStyleValuePair>::pushBack(p);
        rootPointer(p);
    }

    template <class... Args>
    void emplace_back(Args&&... args)
    {
        pushBack(CSSStyleValuePair(args...));
        rootPointer(back());
    }

    size_t size() const
    {
        return GCAtomicVector<CSSStyleValuePair>::size();
    }

    CSSStyleValuePair& at(const size_t& idx)
    {
        return GCAtomicVector<CSSStyleValuePair>::at(idx);
    }

    const CSSStyleValuePair& at(const size_t& idx) const
    {
        return GCAtomicVector<CSSStyleValuePair>::at(idx);
    }

    CSSStyleValuePair& operator[](const size_t& idx)
    {
        return at(idx);
    }

    const CSSStyleValuePair& operator[](const size_t& idx) const
    {
        return at(idx);
    }

    static void* operator new(size_t size)
    {
        return GC_MALLOC(size);
    }

    String* toString()
    {
        StringBuilder builder;
        size_t len = size();
        for (size_t i = 0; i < len; i++) {
            String* src = at(i).toString();
            if (m_separator == CommaSeparatorAppendQuoteWhenMeetWhiteSpace &&
                src->containsWhitespace()) {
                builder.appendChar('"');
                builder.appendString(src);
                builder.appendChar('"');
            } else {
                builder.appendString(src);
            }
            if (i != len - 1) {
                builder.appendString(separatorString());
            }
        }
        return builder.finalize();
    }

protected:
    String* separatorString()
    {
        if (m_separator == None) {
            return String::emptyString;
        } else if (m_separator == SpaceSeparator) {
            return String::spaceString;
        } else if (m_separator == CommaSeparator) {
            return String::createASCIIString(", ");
        } else if (m_separator == CommaSeparatorAppendQuoteWhenMeetWhiteSpace) {
            return String::createASCIIString(", ");
        } else {
            return String::createASCIIString("/ ");
        }
    }

    void rootPointer(CSSStyleValuePair v)
    {
        auto p = v.pointerValue();
        if (p) {
            m_pointerRooter.insert(p);
        }
    }

    Separator m_separator;
    GCUnorderedSet<void*> m_pointerRooter;
};

class CSSSelector;
class CSSAttributeSelector;
class CSSPseudoSelector;

class CSSSelectorList : public GCVector<CSSSelector*> {
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
#define ADD_PSEUDO_TYPE(name, nameLower, selectorName) Pseudo##name,
        STARFISH_ENUM_PSEUDO_SELECTORS(ADD_PSEUDO_TYPE)
#undef ADD_PSEUDO_TYPE
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

    bool isContentPseudoElement() const
    {
        return m_type == PseudoElement &&
               (pseudotype() == PseudoAfter || pseudotype() == PseudoBefore);
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

    PseudoType pseudotype() const
    {
        return m_pseudotype;
    }

    AttributeMatchType attributeMatch() const
    {
        return m_attributeMatch;
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
    PseudoType m_pseudotype : 6;
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

    PseudoType parsePseudoType(StarFish* sf, AtomicString pseudoName,
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
class StyleResolveContext {
public:
    StyleResolveContext(Document* document);
    ~StyleResolveContext();
    void pushIntoComputedStylePool(ComputedStyle* b);

    bool hasItemInComputedStylePool()
    {
        return m_computedStylePool.size();
    }

    void* takeFromComputedStylePool()
    {
        void* ret = m_computedStylePool.back();
#ifndef NDEBUG
        m_dbg.erase(m_dbg.find((ComputedStyle*)ret));
#endif
        m_computedStylePool.pop_back();
        STARFISH_ASSERT(ret);
        return ret;
    }

    void* allocateComputedStyle();

    Document* m_document;
    std::unique_ptr<AncestorSelectorFilter> m_ancestorSelectorFilter;
    GCVector<ComputedStyle*> m_computedStylePool;
#ifndef NDEBUG
    std::set<ComputedStyle*> m_dbg;
#endif
};

class StyleResolver : public DocumentHoldable, public gc {
    friend void computeCSSCombinatorSelectorCache(
        StyleResolver* resolver, Element* element,
        const GCVector<std::pair<StyleRule*, ResourceURL*>>::iterator& begin,
        const GCVector<std::pair<StyleRule*, ResourceURL*>>::iterator& end);

public:
    enum PseudoElementType {
        PseudoElementNone,
        PseudoElementFirstLine,
        PseudoElementFirstLetter,
        PseudoElementBefore,
        PseudoElementAfter,
        PseudoElementFirstLineInherited,
        PseudoElementFormOnly,
        PseudoElementCounter,
    };

    enum Match {
        SelectorMatches,          // The selector matches the element
        SelectorFailsLocally,     // The selector fails for the element.
        SelectorFailsAllSiblings, // The selector fails for the element and any
                                  // sibling of the element
        SelectorFailsCompletely   // The selector fails for the element or
                                  // ancestor of the element
    };

    // MUST uses same bit with Node::StyleChangeReason
    enum StyleDamageSource {
        NoDamage = 0,
        StyleDamageFromID = 1,
        StyleDamageFromClass = 1 << 1,
        StyleDamageFromAttribute = 1 << 2,
        StyleDamageFromElementState = 1 << 3,
        StyleDamageFromDOMTree = 1 << 4,
        StyleDamageFromAll = StyleDamageFromID | StyleDamageFromClass |
                             StyleDamageFromAttribute |
                             StyleDamageFromElementState |
                             StyleDamageFromDOMTree
    };

    struct MatchResult {
        MatchResult()
            : pseudoType(PseudoElementNone)
            , styleDamageFrom(NoDamage)
            , seenCombinator(false)
        {
        }

        PseudoElementType pseudoType;
        StyleDamageSource styleDamageFrom;
        bool seenCombinator;
    };

    StyleResolver(Document* document);
    void addToRuleSet(std::pair<StyleRule*, ResourceURL*> rule);
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

    RuleSet* ruleSet()
    {
        return m_ruleSet;
    }

    void removeAllRules();

    bool usesFirstLineRule() const
    {
        return m_usesFirstLineRule;
    }

    void resolveDOMStyle(Document* document, bool force = false);

#ifdef STARFISH_ENABLE_TEST
    void dumpDOMStyle(Document* document);
#endif
    ComputedStyle* resolveDocumentStyle(Document* doc);
    ComputedStyle* resolveStyle(StyleResolveContext& ctx, Element* node,
                                ComputedStyle* parent);

    void matchAllRules(
        StyleResolveContext& ctx, Element* element, ComputedStyle* ret,
        ComputedStyle* parent,
        PseudoElementType pseudoType = PseudoElementType::PseudoElementNone);
    Match matchSelector(Element* element, AtomicString elementName,
                        AtomicString elementId,
                        const GCVector<AtomicString>& elementClasses,
                        const CSSSelectorList& selectorList, unsigned idx,
                        MatchResult& result, bool isQueryingSelector = false);
    void collectMatchingRulesFromAuthorSheet(
        StyleResolveContext& ctx,
        const GCUnorderedMultiMap<
            AtomicString, std::pair<StyleRule*, ResourceURL*>>::iterator& begin,
        const GCUnorderedMultiMap<
            AtomicString, std::pair<StyleRule*, ResourceURL*>>::iterator& end,
        CSSSelector::Type type, Element* element, AtomicString elementName,
        AtomicString elementId, const GCVector<AtomicString>& elementClasses,
        MatchedStyleRules<32>& authorRules, ComputedStyle* ret,
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

    bool mayHaveAttrSelectorWithName(AtomicString localName)
    {
        auto iter = std::find(m_ruleSetAttrFilter.begin(),
                              m_ruleSetAttrFilter.end(), localName);
        return iter != m_ruleSetAttrFilter.end();
    }

protected:
    void resolveChildrenStyle(StyleResolveContext& ctx, StyleResolver* resolver,
                              Node* element, ComputedStyle* elementStyle,
                              bool inheritedStyleChanged = false);

    void apply(Element* element, GCAtomicVector<CSSStyleValuePair>& cssValues,
               GCVector<MutablePropertyValue>& cssCustomValues,
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
    RuleSet* m_ruleSet;
    GCAtomicVector<AtomicString> m_ruleSetAttrFilter;
};
}

#endif
