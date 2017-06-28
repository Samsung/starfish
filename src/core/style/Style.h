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
#include "core/util/VectorWithInlineStorage.h"

namespace StarFish {

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
    enum Kind { PX, EM, EX, IN, CM, MM, PT, PC };

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
        }

        m_value = f;
    }

    Kind kind()
    {
        return m_kind;
    }

    float value()
    {
        return m_value;
    }

    Length toLength()
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
            return Length(Length::EmToBeFixed, m_value);
        } else if (m_kind == EX) {
            return Length(Length::ExToBeFixed, m_value);
        }

        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    String* toString() const
    {
        std::string stdStr = String::fromFloat(m_value)->utf8Data();
        if (m_kind == PX)
            return String::fromUTF8(stdStr.append("px").c_str());
        else if (m_kind == CM)
            return String::fromUTF8(stdStr.append("cm").c_str());
        else if (m_kind == MM)
            return String::fromUTF8(stdStr.append("mm").c_str());
        else if (m_kind == IN)
            return String::fromUTF8(stdStr.append("in").c_str());
        else if (m_kind == PC)
            return String::fromUTF8(stdStr.append("pc").c_str());
        else if (m_kind == PT)
            return String::fromUTF8(stdStr.append("pt").c_str());
        else if (m_kind == EM)
            return String::fromUTF8(stdStr.append("em").c_str());
        else if (m_kind == EX)
            return String::fromUTF8(stdStr.append("ex").c_str());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

protected:
    Kind m_kind;
    float m_value;
};

// https://www.w3.org/TR/css3-values/#angles
class CSSAngle {
public:
    enum Kind { DEG, GRAD, RAD, TURN };

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

    Kind kind()
    {
        return m_kind;
    }

    float value()
    {
        return m_value;
    }

    float toDegreeValue()
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

    String* toString()
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

class CSSTime {
public:
    enum Kind { S, MS };

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

    Kind kind()
    {
        return m_kind;
    }

    bool isZero()
    {
        return m_value == 0;
    }

    float value()
    {
        return m_value;
    }

    double toTimeValue()
    {
        if (m_kind == S) {
            return m_value * 1000; // to ms
        } else if (m_kind == MS) {
            return m_value;
        }

        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    String* toString()
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

// inline | block | list-item | inline-block | table | inline-table |
// table-row-group | table-header-group | table-footer-group | table-row |
// table-column-group | table-column | table-cell | table-caption | none |
// inherit
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
    NoneDisplayValue,
};

enum PositionValue {
    StaticPositionValue,
    RelativePositionValue,
    AbsolutePositionValue,
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

// text-align, transform-origin, background-position
enum SideValue {
    NoneSideValue, // Depends on direction
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

// Widget Engine will support only visible and hidden values.
enum OverflowValue {
    VisibleOverflow,
    HiddenOverflow,
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
    F(TextAlign, textAlign, "text-align")                                \
    F(Transform, transform, "transform")                                 \
    F(TransformOrigin, transformOrigin, "transform-origin")              \
    F(Visibility, visibility, "visibility")                              \
    F(Overflow, overflow, "overflow")                                    \
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
    F(TransitionDelay, transitionDelay, "transitionDelay")

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
    F(Transition, transition, "transition")

#define GEN_FOURSIDE(F) \
    F(Top, top)         \
    F(Right, right)     \
    F(Bottom, bottom)   \
    F(Left, left)

class CSSTransformFunction {
public:
    enum Kind {
        Matrix,
        Translate,
        TranslateX,
        TranslateY,
        Scale,
        ScaleX,
        ScaleY,
        Rotate,
        Skew,
        SkewX,
        SkewY
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
        case Translate:
            return String::fromUTF8("translate");
        case TranslateX:
            return String::fromUTF8("translateX");
        case TranslateY:
            return String::fromUTF8("translateY");
        case Scale:
            return String::fromUTF8("scale");
        case ScaleX:
            return String::fromUTF8("scaleX");
        case ScaleY:
            return String::fromUTF8("scaleY");
        case Rotate:
            return String::fromUTF8("rotate");
        case Skew:
            return String::fromUTF8("skew");
        case SkewX:
            return String::fromUTF8("skewX");
        case SkewY:
            return String::fromUTF8("skewY");
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

        DisplayValueKind,
        PositionValueKind,
        FloatValueKind,
        ClearValueKind,
        VerticalAlignValueKind,
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

        BorderStyleValueKind,
        BorderWidthValueKind,

        // table
        BorderCollapseValueKind,
        CaptionSideValueKind,
        TableLayoutValueKind,

        OverflowValueKind,
        TextDecorationValueKind,
        VisibilityValueKind,
        UnicodeBidiValueKind,

        // transform
        TransformFunctions,

        // transition
        TransitionPropertyValueKind,
        TransitionTimingFunctionValueKind,

        // content
        Attr
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

    String* urlValue(ResourceURL* urlOfStyleSheet);

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

    VisibilityValue visibility() const
    {
        STARFISH_ASSERT(m_valueKind == VisibilityValueKind);
        return m_value.m_visibility;
    }

    TextDecorationValue textDecoration() const
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

    String* attrValue() const
    {
        STARFISH_ASSERT(m_valueKind == Attr);
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
        TransitionPropertyValue m_transitionProperty;
        TransitionTimingFunctionValue m_transitionTimingFunction;
        CSSTime m_time;
        ValueData(int v)
        {
            m_floatValue = v;
        }
        ValueData(float v)
        {
            m_floatValue = v;
        }
        ValueData(DisplayValue v)
        {
            m_display = v;
        }
        ValueData(PositionValue v)
        {
            m_position = v;
        }
        ValueData(VerticalAlignValue v)
        {
            m_verticalAlign = v;
        }
        ValueData(FontSizeValue v)
        {
            m_fontSize = v;
        }
        ValueData(FontStyleValue v)
        {
            m_fontStyle = v;
        }
        ValueData(FontWeightValue v)
        {
            m_fontWeight = v;
        }
        ValueData(SideValue v)
        {
            m_side = v;
        }
        ValueData(DirectionValue v)
        {
            m_direction = v;
        }
        ValueData(WhiteSpaceValue v)
        {
            m_whiteSpace = v;
        }
        ValueData(CSSLength v)
        {
            m_length = v;
        }
        ValueData(CSSAngle v)
        {
            m_angle = v;
        }
        ValueData(String* v)
        {
            m_stringValue = v;
        }
        ValueData(BackgroundRepeatValue v)
        {
            m_backgroundRepeat = v;
        }
        ValueData(BorderStyleValue v)
        {
            m_borderStyle = v;
        }
        ValueData(BorderWidthValue v)
        {
            m_borderWidth = v;
        }
        ValueData(ValueList* v)
        {
            m_multiValue = v;
        }
        ValueData(OverflowValue v)
        {
            m_overflow = v;
        }
        ValueData(VisibilityValue v)
        {
            m_visibility = v;
        }
        ValueData(UnicodeBidiValue v)
        {
            m_unicodeBidi = v;
        }
        ValueData(TextDecorationValue v)
        {
            m_textDecoration = v;
        }
        ValueData(CSSTransformFunctions* v)
        {
            m_transforms = v;
        }
        ValueData(Unit::Color v)
        {
            m_color = v;
        }
        ValueData(NamedColor::NamedColorValue v)
        {
            m_namedColor = v;
        }
        ValueData(BorderCollapseValue v)
        {
            m_borderCollapse = v;
        }
        ValueData(CaptionSideValue v)
        {
            m_captionSide = v;
        }
        ValueData(TableLayoutValue v)
        {
            m_tableLayout = v;
        }
        ValueData(TransitionPropertyValue v)
        {
            m_transitionProperty = v;
        }
        ValueData(CSSTime v)
        {
            m_time = v;
        }
        ValueData(TransitionTimingFunctionValue v)
        {
            m_transitionTimingFunction = v;
        }
    };

    CSSStyleValuePair(ValueKind kind, ValueData value)
        : m_valueKind(kind)
        , m_flagImportant(false)
        , m_value(value)
    {
    }

    void setValue(const ValueData& value)
    {
        m_value = value;
    }

    const ValueData& value()
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

    bool updateValueLengthOrPercent(const CSSTokenVector& tokens,
                                    bool allowNegative);
    bool updateValueLengthOrPercent(const CSSTokenValue& token,
                                    bool allowNegative);
    bool updateValueLengthOrPercentOrAuto(const CSSTokenVector& tokens,
                                          bool allowNegative);
    bool updateValueLengthOrPercentOrAutoOrNone(const CSSTokenVector& tokens,
                                                bool allowNegative);
    bool updateValueLengthOrPercentOrAuto(const CSSTokenValue& token,
                                          bool allowNegative);
    bool updateValueLengthOrPercentOrAutoOrNone(const CSSTokenValue& token,
                                                bool allowNegative);

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
    bool updateValueUnitLineHeight(const CSSTokenValue& token);
    bool updateValueUnitTransitionProperty(const CSSTokenValue& value);
    bool updateValueUnitTransitionTimingFunction(const CSSTokenValue& value);
    bool updateValueUnitTransitionTime(const CSSTokenValue& value);

protected:
    KeyKind m_keyKind : 8;
    ValueKind m_valueKind : 8;
    bool m_flagImportant : 1;
    ValueData m_value;
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
    void push_front(CSSSelector* s)
    {
        GCVector<CSSSelector*>::insert(begin(), s);
    }

    String* selectorText();
    static String* selectorText(CSSSelectorList* list, unsigned idx,
                                String* rightSide);
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

    AtomicString& selectorText()
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

class CSSStyleSheet;
class StyleResolver : public DocumentHoldable {
public:
    enum PseudoElementType {
        PseudoElementNone = 0,
        PseudoElementFirstLine = 1,
        PseudoElementFirstLetter = 1 << 1,
        PseudoElementBefore = 1 << 2,
        PseudoElementAfter = 1 << 3,
        PseudoElementFirstLineInherited = 1 << 4
    };

    enum Match {
        SelectorMatches,          // The selector matches the element
        SelectorFailsLocally,     // The selector fails for the element.
        SelectorFailsAllSiblings, // The selector fails for the element and any
                                  // sibling of the element
        SelectorFailsCompletely   // The selector fails for the element or
                                  // ancestor of the element
    };

    struct MatchResult {
        MatchResult()
            : pseudoType(PseudoElementType::PseudoElementNone)
        {
        }

        PseudoElementType pseudoType;
    };

    StyleResolver(Document* document);
    void addSheet(CSSStyleSheet* sheet);
    void removeSheet(CSSStyleSheet* sheet)
    {
        STARFISH_ASSERT(std::find(m_sheets.begin(), m_sheets.end(), sheet) !=
                        m_sheets.end());
        m_sheets.erase(std::find(m_sheets.begin(), m_sheets.end(), sheet));
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

    bool usesFirstLineRule()
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

    const MediaQueryEvaluator& mediaQueryEvaluator();

protected:
    void apply(Element* element, GCVector<CSSStyleValuePair>& cssValues,
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
};
}

#endif
