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

#ifndef __StarFishStyle__
#define __StarFishStyle__

#include "util/String.h"
#include "style/Unit.h"
#include "style/Length.h"
#include "platform/canvas/font/Font.h"
#include "style/DefaultStyle.h"
#include "style/UnitHelper.h"
#include "dom/EventTarget.h"
#include "dom/DOMTokenList.h"

#include <sstream>

namespace StarFish {

class ComputedStyle;
class Element;
class Document;

// https://www.w3.org/TR/CSS21/syndata.html#value-def-length
class CSSLength {
public:
    enum Kind {
        PX,
        EM,
        EX,
        IN,
        CM,
        MM,
        PT,
        PC
    };

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
        if (unit->length() == 0 || unit->equals("px"))
            m_kind = PX;
        else if (unit->equals("em"))
            m_kind = EM;
        else if (unit->equals("ex"))
            m_kind = EX;
        else if (unit->equals("in"))
            m_kind = IN;
        else if (unit->equals("cm"))
            m_kind = CM;
        else if (unit->equals("mm"))
            m_kind = MM;
        else if (unit->equals("pt"))
            m_kind = PT;
        else if (unit->equals("pc"))
            m_kind = PC;

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
        // absolute length
        if (m_kind == PX)
            return Length(Length::Fixed, m_value);
        else if (m_kind == CM)
            return Length(Length::Fixed, convertFromCmToPx(m_value));
        else if (m_kind == MM)
            return Length(Length::Fixed, convertFromMmToPx(m_value));
        else if (m_kind == IN)
            return Length(Length::Fixed, convertFromInToPx(m_value));
        else if (m_kind == PC)
            return Length(Length::Fixed, convertFromPcToPx(m_value));
        else if (m_kind == PT)
            return Length(Length::Fixed, convertFromPtToPx(m_value));

        // font-relative length
        else if (m_kind == EM)
            return Length(Length::EmToBeFixed, m_value);
        else if (m_kind == EX)
            return Length(Length::ExToBeFixed, m_value);

        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    String* toString()
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
    enum Kind {
        DEG,
        GRAD,
        RAD,
        TURN
    };

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
        if (m_kind == DEG)
            return m_value;
        else if (m_kind == RAD)
            return convertFromRadToDeg(m_value);
        else if (m_kind == GRAD)
            return convertFromGradToDeg(m_value);
        else if (m_kind == TURN)
            return convertFromTurnToDeg(m_value);

        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    String* toString()
    {
        std::stringstream ss(std::stringstream::in | std::stringstream::out);
        ss << m_value;
        std::string stdStr = ss.str();
        if (m_kind == DEG)
            return String::fromUTF8(stdStr.append("deg").c_str());
        else if (m_kind == RAD)
            return String::fromUTF8(stdStr.append("rad").c_str());
        else if (m_kind == GRAD)
            return String::fromUTF8(stdStr.append("grad").c_str());
        else if (m_kind == TURN)
            return String::fromUTF8(stdStr.append("turn").c_str());

        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

protected:
    Kind m_kind;
    float m_value;
};

// inline | block | list-item | inline-block | table | inline-table | table-row-group | table-header-group | table-footer-group | table-row | table-column-group | table-column | table-cell | table-caption | none | inherit
enum DisplayValue {
    InlineDisplayValue, // initial value
    BlockDisplayValue,
    InlineBlockDisplayValue,
    NoneDisplayValue,
};

enum PositionValue {
    StaticPositionValue,
    RelativePositionValue,
    AbsolutePositionValue,
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
#define FOR_EACH_STYLE_ATTRIBUTE(F)                                \
    F(Color, color, "color")                                       \
    F(Direction, direction, "direction")                           \
    F(BackgroundColor, backgroundColor, "background-color")        \
    F(BackgroundImage, backgroundImage, "background-image")        \
    F(BackgroundPosition, backgroundPosition, "background-position") \
    F(BackgroundSize, backgroundSize, "background-size")           \
    F(LineHeight, lineHeight, "line-height")                       \
    F(PaddingTop, paddingTop, "padding-top")                       \
    F(PaddingRight, paddingRight, "padding-right")                 \
    F(PaddingBottom, paddingBottom, "padding-bottom")              \
    F(PaddingLeft, paddingLeft, "padding-left")                    \
    F(MarginTop, marginTop, "margin-top")                          \
    F(MarginRight, marginRight, "margin-right")                    \
    F(MarginBottom, marginBottom, "margin-bottom")                 \
    F(MarginLeft, marginLeft, "margin-left")                       \
    F(Top, top, "top")                                             \
    F(Bottom, bottom, "bottom")                                    \
    F(Left, left, "left")                                          \
    F(Right, right, "right")                                       \
    F(Width, width, "width")                                       \
    F(Height, height, "height")                                    \
    F(FontSize, fontSize, "font-size")                             \
    F(FontStyle, fontStyle, "font-style")                          \
    F(Position, position, "position")                              \
    F(TextDecoration, textDecoration, "text-decoration")           \
    F(Display, display, "display")                                 \
    F(BorderImageSlice, borderImageSlice, "border-image-slice")    \
    F(BorderImageSource, borderImageSource, "border-image-source") \
    F(BorderImageWidth, borderImageWidth, "border-image-width")    \
    F(BorderTopColor, borderTopColor, "border-top-color")          \
    F(BorderRightColor, borderRightColor, "border-right-color")    \
    F(BorderBottomColor, borderBottomColor, "border-bottom-color") \
    F(BorderLeftColor, borderLeftColor, "border-left-color")       \
    F(BorderTopStyle, borderTopStyle, "border-top-style")          \
    F(BorderRightStyle, borderRightStyle, "border-right-style")    \
    F(BorderBottomStyle, borderBottomStyle, "border-bottom-style") \
    F(BorderLeftStyle, borderLeftStyle, "border-left-style")       \
    F(BorderTopWidth, borderTopWidth, "border-top-width")          \
    F(BorderRightWidth, borderRightWidth, "border-right-width")    \
    F(BorderBottomWidth, borderBottomWidth, "border-bottom-width") \
    F(BorderLeftWidth, borderLeftWidth, "border-left-width")       \
    F(TextAlign, textAlign, "text-align")                          \
    F(Transform, transform, "transform")                           \
    F(TransformOrigin, transformOrigin, "transform-origin")        \
    F(Visibility, visibility, "visibility")                        \
    F(Overflow, overflow, "overflow")                              \
    F(ZIndex, zIndex, "z-index")                                   \
    F(VerticalAlign, verticalAlign, "vertical-align")              \
    F(BackgroundRepeatX, backgroundRepeatX, "background-repeat-x") \
    F(BackgroundRepeatY, backgroundRepeatY, "background-repeat-y") \
    F(Opacity, opacity, "opacity")                                 \
    F(FontWeight, fontWeight, "font-weight")                       \
    F(UnicodeBidi, unicodeBidi, "unicode-bidi")

#define FOR_EACH_STYLE_ATTRIBUTE_TOTAL(F)                          \
    FOR_EACH_STYLE_ATTRIBUTE(F)                                    \
    F(Border, border, "border")                                    \
    F(BorderTop, borderTop, "border-top")                          \
    F(BorderRight, borderRight, "border-right")                    \
    F(BorderBottom, borderBottom, "border-bottom")                 \
    F(BorderLeft, borderLeft, "border-left")                       \
    F(BorderStyle, borderStyle, "border-style")                    \
    F(BorderWidth, borderWidth, "border-width")                    \
    F(BorderColor, borderColor, "border-color")                    \
    F(Background, background, "background")                        \
    F(BackgroundRepeat, backgroundRepeat, "background-repeat")     \
    F(Margin, margin, "margin")                                    \
    F(Padding, padding, "padding")

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

class CSSTransformFunctions : public gc {
public:
    CSSTransformFunctions()
    {
    }

    ~CSSTransformFunctions()
    {
    }

    void append(CSSTransformFunction f)
    {
        m_transforms.push_back(f);
    }

    void clear()
    {
        m_transforms.clear();
    }

    CSSTransformFunction at(int i)
    {
        return m_transforms[i];
    }

    unsigned size()
    {
        return m_transforms.size();
    }

protected:
    std::vector<CSSTransformFunction, gc_allocator<CSSTransformFunction> > m_transforms;
};

class CSSStyleValuePair : public gc {
    friend class ValueList;

public:
    enum KeyKind {
#define ADD_CSS_KEYKIND(Name, name, cssname) \
        Name,
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
        Number, // real number values - https://www.w3.org/TR/CSS21/syndata.html#value-def-number
        Int32,
        Angle, //
        Normal,
        StringValueKind,
        ColorValueKind,
        UrlValueKind,

        DisplayValueKind,
        PositionValueKind,
        VerticalAlignValueKind,
        SideValueKind,
        DirectionValueKind,

        ValueListKind,

        // BackgroundSize
        Cover,
        Contain,

        BackgroundPositionValueKind,
        BackgroundRepeatValueKind,

        FontSizeValueKind,
        FontStyleValueKind,
        FontWeightValueKind,

        BorderStyleValueKind,
        BorderWidthValueKind,

        OverflowValueKind,
        TextDecorationValueKind,
        VisibilityValueKind,
        UnicodeBidiValueKind,

        // transform
        TransformFunctions,
    };

    CSSStyleValuePair()
        : m_value(0.0f)
    {
    }

    KeyKind keyKind()
    {
        return m_keyKind;
    }

    void setKeyKind(KeyKind kind)
    {
        m_keyKind = kind;
    }

    String* keyName();

    ValueKind valueKind()
    {
        return m_valueKind;
    }

    void setValueKind(ValueKind kind)
    {
        m_valueKind = kind;
    }

    bool setValueCommon(std::vector<String*, gc_allocator<String*> >* tokens);

    bool isAuto()
    {
        return valueKind() == Auto;
    }

    bool isInherit()
    {
        return valueKind() == Inherit;
    }

    DisplayValue displayValue()
    {
        STARFISH_ASSERT(m_valueKind == DisplayValueKind);
        return m_value.m_display;
    }

    PositionValue positionValue()
    {
        STARFISH_ASSERT(m_valueKind == PositionValueKind);
        return m_value.m_position;
    }

    VerticalAlignValue verticalAlignValue()
    {
        STARFISH_ASSERT(m_valueKind == VerticalAlignValueKind);
        return m_value.m_verticalAlign;
    }

    SideValue sideValue()
    {
        STARFISH_ASSERT(m_valueKind == SideValueKind);
        return m_value.m_side;
    }

    FontSizeValue fontSizeValue()
    {
        STARFISH_ASSERT(m_valueKind == FontSizeValueKind);
        return m_value.m_fontSize;
    }

    FontStyleValue fontStyleValue()
    {
        STARFISH_ASSERT(m_valueKind == FontStyleValueKind);
        return m_value.m_fontStyle;
    }

    FontWeightValue fontWeightValue()
    {
        STARFISH_ASSERT(m_valueKind == FontWeightValueKind);
        return m_value.m_fontWeight;
    }

    DirectionValue directionValue()
    {
        STARFISH_ASSERT(m_valueKind == DirectionValueKind);
        return m_value.m_direction;
    }

    UnicodeBidiValue unicodeBidiValue()
    {
        STARFISH_ASSERT(m_valueKind == UnicodeBidiValueKind);
        return m_value.m_unicodeBidi;
    }

    BorderStyleValue borderStyleValue()
    {
        STARFISH_ASSERT(m_valueKind == BorderStyleValueKind);
        return m_value.m_borderStyle;
    }

    BorderWidthValue borderWidthValue()
    {
        STARFISH_ASSERT(m_valueKind == BorderWidthValueKind);
        return m_value.m_borderWidth;
    }

    CSSLength lengthValue()
    {
        STARFISH_ASSERT(m_valueKind == Length);
        return m_value.m_length;
    }

    CSSAngle angleValue()
    {
        STARFISH_ASSERT(m_valueKind == Angle);
        return m_value.m_angle;
    }

    CSSTransformFunctions* transformValue()
    {
        STARFISH_ASSERT(m_valueKind == TransformFunctions);
        return m_value.m_transforms;
    }

    float numberValue()
    {
        STARFISH_ASSERT(m_valueKind == Number);
        return m_value.m_floatValue;
    }

    int32_t int32Value()
    {
        STARFISH_ASSERT(m_valueKind == Int32);
        return m_value.m_int32Value;
    }

    // 0~1
    float percentageValue()
    {
        STARFISH_ASSERT(m_valueKind == Percentage);
        return m_value.m_floatValue;
    }

    String* stringValue()
    {
        STARFISH_ASSERT(m_valueKind == StringValueKind);
        return m_value.m_stringValue;
    }

    String* urlValue(const URL& urlOfStyleSheet)
    {
        STARFISH_ASSERT(m_valueKind == UrlValueKind);
        return URL(urlOfStyleSheet.baseURI(), m_value.m_stringValue).urlString();
    }

    String* urlStringValue()
    {
        STARFISH_ASSERT(m_valueKind == UrlValueKind);
        return m_value.m_stringValue;
    }

    void setStringValue(String* value)
    {
        STARFISH_ASSERT(m_valueKind == StringValueKind);
        m_value.m_stringValue = value;
    }

    BackgroundRepeatValue backgroundRepeatXValue()
    {
        STARFISH_ASSERT(m_valueKind == BackgroundRepeatValueKind);
        return m_value.m_backgroundRepeatX;
    }

    BackgroundRepeatValue backgroundRepeatYValue()
    {
        STARFISH_ASSERT(m_valueKind == BackgroundRepeatValueKind);
        return m_value.m_backgroundRepeatY;
    }

    ValueList* multiValue()
    {
        STARFISH_ASSERT(m_valueKind == ValueListKind);
        return m_value.m_multiValue;
    }

    OverflowValue overflowValue()
    {
        STARFISH_ASSERT(m_valueKind == OverflowValueKind);
        return m_value.m_overflow;
    }

    VisibilityValue visibility()
    {
        STARFISH_ASSERT(m_valueKind == VisibilityValueKind);
        return m_value.m_visibility;
    }

    TextDecorationValue textDecoration()
    {
        return m_value.m_textDecoration;
    }

    ::StarFish::Color colorValue()
    {
        STARFISH_ASSERT(m_valueKind == ColorValueKind);
        return m_value.m_color;
    }

    union ValueData {
        float m_floatValue;
        int32_t m_int32Value;
        DisplayValue m_display;
        PositionValue m_position;
        VerticalAlignValue m_verticalAlign;
        FontSizeValue m_fontSize;
        FontStyleValue m_fontStyle;
        FontWeightValue m_fontWeight;
        SideValue m_side;
        DirectionValue m_direction;
        CSSLength m_length;
        CSSAngle m_angle;
        String* m_stringValue;
        BackgroundRepeatValue m_backgroundRepeatX;
        BackgroundRepeatValue m_backgroundRepeatY;
        BorderStyleValue m_borderStyle;
        BorderWidthValue m_borderWidth;
        ValueList* m_multiValue;
        OverflowValue m_overflow;
        VisibilityValue m_visibility;
        UnicodeBidiValue m_unicodeBidi;
        TextDecorationValue m_textDecoration;
        CSSTransformFunctions* m_transforms;
        ::StarFish::Color m_color;
        ValueData(int v) { m_floatValue = v; }
        ValueData(float v) { m_floatValue = v; }
        ValueData(DisplayValue v) { m_display = v; }
        ValueData(PositionValue v) { m_position = v; }
        ValueData(VerticalAlignValue v) { m_verticalAlign = v; }
        ValueData(FontSizeValue v) { m_fontSize = v; }
        ValueData(FontStyleValue v) { m_fontStyle = v; }
        ValueData(FontWeightValue v) { m_fontWeight = v; }
        ValueData(SideValue v) { m_side = v; }
        ValueData(DirectionValue v) { m_direction = v; }
        ValueData(CSSLength v) { m_length = v; }
        ValueData(CSSAngle v) { m_angle = v; }
        ValueData(String* v) { m_stringValue = v; }
        ValueData(BorderStyleValue v) { m_borderStyle = v; }
        ValueData(BorderWidthValue v) { m_borderWidth = v; }
        ValueData(ValueList* v) { m_multiValue = v; }
        ValueData(OverflowValue v) { m_overflow = v; }
        ValueData(VisibilityValue v) { m_visibility = v; }
        ValueData(UnicodeBidiValue v) { m_unicodeBidi = v; }
        ValueData(TextDecorationValue v) { m_textDecoration = v; }
        ValueData(CSSTransformFunctions* v) { m_transforms = v; }
        ValueData(::StarFish::Color v) { m_color = v; }
    };

    CSSStyleValuePair(ValueKind kind, ValueData value)
        : m_valueKind(kind)
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

    String* percentageToString(float f)
    {
        float v = f * 100.f;
        return String::fromFloat(v)->concat(String::createASCIIString("%"));
    }

    String* lengthOrPercentageOrKeywordToString()
    {
        if (valueKind() == CSSStyleValuePair::ValueKind::Auto)
            return String::fromUTF8("auto");
        else if (valueKind() == CSSStyleValuePair::ValueKind::None)
            return String::fromUTF8("none");
        else if (valueKind() == CSSStyleValuePair::ValueKind::Length)
            return lengthValue().toString();
        else if (valueKind() == CSSStyleValuePair::ValueKind::Percentage)
            return percentageToString(percentageValue());
        else
            return String::emptyString;
    }

    String* numberToString(float f)
    {
        return String::fromFloat(f);
    }

    String* valueToString()
    {
        return valueToString(valueKind(), m_value);
    }

    String* valueToString(CSSStyleValuePair::ValueKind kind, CSSStyleValuePair::ValueData data)
    {
        if (kind == CSSStyleValuePair::ValueKind::Auto)
            return String::fromUTF8("auto");
        else if (kind == CSSStyleValuePair::ValueKind::Inherit)
            return String::inheritString;
        else if (kind == CSSStyleValuePair::ValueKind::Initial)
            return String::initialString;
        else if (kind == CSSStyleValuePair::ValueKind::Length)
            return data.m_length.toString();
        else if (kind == CSSStyleValuePair::ValueKind::Percentage)
            return percentageToString(data.m_floatValue);
        else if (kind == CSSStyleValuePair::ValueKind::Number)
            return numberToString(data.m_floatValue);
        else if (kind == CSSStyleValuePair::ValueKind::Angle)
            return data.m_angle.toString();
        else if (kind == CSSStyleValuePair::ValueKind::StringValueKind)
            return data.m_stringValue;
        else
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    String* toString();

    void setLengthValue(const char* value);

    void setValue(KeyKind kKind, const char* value)
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
            setLengthValue(value);
            break;
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    void setValuePercentageOrLength(const char* value);
    void setValuePercentageOrLength(std::vector<String*, gc_allocator<String*> >* tokens);
    void setValueUrlOrNone(std::vector<String*, gc_allocator<String*> >* tokens);
    void setValueBorderUnitStyle(std::vector<String*, gc_allocator<String*> >* tokens);
    void setValueBorderUnitWidth(std::vector<String*, gc_allocator<String*> >* tokens);

#define SET_VALUE(name, ...) \
    void setValue##name(std::vector<String*, gc_allocator<String*> >* tokens);

    FOR_EACH_STYLE_ATTRIBUTE(SET_VALUE)
#undef SET_VALUE

protected:
    KeyKind m_keyKind;
    ValueKind m_valueKind;
    ValueData m_value;
};

class ValueList : public gc {
public:
    enum Separator {
        SpaceSeparator,
        CommaSeparator,
        SlashSeparator
    };

    ValueList()
        : m_separator(SpaceSeparator)
    {
    }

    ValueList(Separator sep)
        : m_separator(sep)
    {
    }

    void append(CSSStyleValuePair::ValueKind kind, CSSStyleValuePair::ValueData value)
    {
        m_values.push_back(CSSStyleValuePair(kind, value));
    }

    CSSStyleValuePair& atIndex(int idx)
    {
        return m_values[idx];
    }

    unsigned int size()
    {
        return m_values.size();
    }

protected:
    Separator m_separator;
    std::vector<CSSStyleValuePair, gc_allocator<CSSStyleValuePair> > m_values;
};

class CSSStyleDeclaration : public ScriptWrappable {
    friend class StyleResolver;

public:
    CSSStyleDeclaration(Document* document, Element* element = NULL)
        : ScriptWrappable(this)
        , m_document(document)
    {
        m_element = element;
    }

    void addValuePair(CSSStyleValuePair p)
    {
        for (size_t i = 0; i < m_cssValues.size(); i++) {
            CSSStyleValuePair v = m_cssValues[i];
            if (v.keyKind() == p.keyKind()) {
                m_cssValues[i] = p;
                return;
            }
        }
        m_cssValues.push_back(p);
    }

    String* getPropertyValue(String* key)
    {
        for (size_t i = 0; i < m_cssValues.size(); i++) {
            CSSStyleValuePair v = m_cssValues[i];
            if (v.keyName()->equals(key)) {
                return v.toString();
            }
        }
        return String::emptyString;
    }

    void clear()
    {
        m_cssValues.clear();
    }

    Document* document()
    {
        return m_document;
    }

    CSSStyleDeclaration* clone(Document* document, Element* element)
    {
        CSSStyleDeclaration* newStyle = new CSSStyleDeclaration(document, element);
        newStyle->m_cssValues = m_cssValues;

        return newStyle;
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }


    String* generateCSSText();


    void notifyNeedsStyleRecalc();

    void tokenizeCSSValue(std::vector<String*, gc_allocator<String*> >* tokens, String* src);

    bool checkEssentialValue(std::vector<String*, gc_allocator<String*> >* tokens);

#define CHECK_INPUT_ERROR(name, ...) \
    bool checkInputError##name(std::vector<String*, gc_allocator<String*> >* tokens);

    FOR_EACH_STYLE_ATTRIBUTE(CHECK_INPUT_ERROR)
#undef CHECK_INPUT_ERROR

#define FOR_EACH_STYLE_ATTRIBUTE_FOURSIDE_SHORTHAND(F)                  \
    F(Margin, MarginTop, MarginBottom, MarginLeft, MarginRight)         \
    F(Padding, PaddingTop, PaddingBottom, PaddingLeft, PaddingRight)    \
    F(BorderStyle, BorderTopStyle, BorderBottomStyle, BorderLeftStyle, BorderRightStyle)    \
    F(BorderWidth, BorderTopWidth, BorderBottomWidth, BorderLeftWidth, BorderRightWidth)    \
    F(BorderColor, BorderTopColor, BorderBottomColor, BorderLeftColor, BorderRightColor)

#define CHECK_INPUT_ERROR_FOURSIDE(name, nameTop, nameBottom, nameLeft, nameRight) \
    bool checkInputError##name(std::vector<String*, gc_allocator<String*> >* tokens);

    FOR_EACH_STYLE_ATTRIBUTE_FOURSIDE_SHORTHAND(CHECK_INPUT_ERROR_FOURSIDE)
#undef CHECK_INPUT_ERROR_FOURSIDE

    bool checkInputErrorBackground(std::vector<String*, gc_allocator<String*> >* tokens);
    bool checkInputErrorBackgroundRepeat(std::vector<String*, gc_allocator<String*> >* tokens);
    bool checkInputErrorBorder(std::vector<String*, gc_allocator<String*> >* tokens);
    bool checkInputErrorBorderTop(std::vector<String*, gc_allocator<String*> >* tokens);
    bool checkInputErrorBorderRight(std::vector<String*, gc_allocator<String*> >* tokens);
    bool checkInputErrorBorderBottom(std::vector<String*, gc_allocator<String*> >* tokens);
    bool checkInputErrorBorderLeft(std::vector<String*, gc_allocator<String*> >* tokens);
    bool checkHavingOneTokenAndLengthOrPercentage(std::vector<String*, gc_allocator<String*> >* tokens, bool allowNegative);
#define ATTRIBUTE_GETTER(name, ...)                                              \
    String* name()                                                               \
    {                                                                            \
        for (unsigned i = 0; i < m_cssValues.size(); i++) {                      \
            if (m_cssValues.at(i).keyKind() == CSSStyleValuePair::KeyKind::name) \
                return m_cssValues.at(i).toString();                             \
        }                                                                        \
        return String::emptyString;                                              \
    }

    FOR_EACH_STYLE_ATTRIBUTE(ATTRIBUTE_GETTER)
#undef ATTRIBUTE_GETTER

#define ATTRIBUTE_SETTER(name, ...)                                                    \
    void set##name(String* value)                                                      \
    {                                                                                  \
        if (value->length() == 0) {                                                    \
            for (unsigned i = 0; i < m_cssValues.size(); i++) {                        \
                if (m_cssValues.at(i).keyKind() == CSSStyleValuePair::KeyKind::name) { \
                    m_cssValues.erase(m_cssValues.begin() + i);                        \
                }                                                                      \
            }                                                                          \
            notifyNeedsStyleRecalc();                                                  \
            return;                                                                    \
        }                                                                              \
        std::vector<String*, gc_allocator<String*> > tokens;                           \
        tokenizeCSSValue(&tokens, value);                                              \
        if (checkEssentialValue(&tokens) || checkInputError##name(&tokens)) {                                          \
            for (unsigned i = 0; i < m_cssValues.size(); i++) {                        \
                if (m_cssValues.at(i).keyKind() == CSSStyleValuePair::KeyKind::name) { \
                    if (!m_cssValues.at(i).setValueCommon(&tokens))                    \
                        m_cssValues.at(i).setValue##name(&tokens);                     \
                    notifyNeedsStyleRecalc();                                          \
                    return;                                                            \
                }                                                                      \
            }                                                                          \
            CSSStyleValuePair ret;                                                     \
            ret.setKeyKind(CSSStyleValuePair::KeyKind::name);                          \
            if (!ret.setValueCommon(&tokens))                                          \
                ret.setValue##name(&tokens);                                           \
            notifyNeedsStyleRecalc();                                                  \
            m_cssValues.push_back(ret);                                                \
        }                                                                              \
    }

    FOR_EACH_STYLE_ATTRIBUTE(ATTRIBUTE_SETTER)
#undef ATTRIBUTE_SETTER

#define ATTRIBUTE_GETTER_FOURSIDE(name, nameTop, nameBottom, nameLeft, nameRight)                                            \
    String* name()                                                               \
    {                                                                            \
        String* top = nameTop();                                                 \
        if (!top->equals(String::emptyString)) {                                 \
            String* right = nameRight();                                         \
            if (!right->equals(String::emptyString)) {                           \
                String* bottom = nameBottom();                                   \
                if (!bottom->equals(String::emptyString)) {                      \
                    String* left = nameLeft();                                   \
                    if (!left->equals(String::emptyString)) {                    \
                        return combineBoxString(top, right, bottom, left);       \
                    }                                                            \
                }                                                                \
            }                                                                    \
        }                                                                        \
        return String::emptyString;                                              \
    }

    FOR_EACH_STYLE_ATTRIBUTE_FOURSIDE_SHORTHAND(ATTRIBUTE_GETTER_FOURSIDE)
#undef ATTRIBUTE_GETTER_FOURSIDE

#define ATTRIBUTE_SETTER_FOURSIDE(name, nameTop, nameBottom, nameLeft, nameRight)                                            \
    void set##name(String* value)                                                      \
    {                                                                                  \
        if (value->length() == 0) {                                                    \
            set##nameTop(String::emptyString);                                         \
            set##nameBottom(String::emptyString);                                      \
            set##nameLeft(String::emptyString);                                        \
            set##nameRight(String::emptyString);                                       \
            return;                                                                    \
        }                                                                              \
        std::vector<String*, gc_allocator<String*> > tokens;                           \
        tokenizeCSSValue(&tokens, value);                                              \
        if (checkEssentialValue(&tokens) || checkInputError##name(&tokens)) {                                          \
            size_t len = tokens.size();                                                \
            if (len == 1) {                                                            \
                set##nameTop(tokens[0]);                                               \
                set##nameBottom(tokens[0]);                                            \
                set##nameLeft(tokens[0]);                                              \
                set##nameRight(tokens[0]);                                             \
            } else if (len == 2) {                                                     \
                set##nameTop(tokens[0]);                                               \
                set##nameBottom(tokens[0]);                                            \
                set##nameLeft(tokens[1]);                                              \
                set##nameRight(tokens[1]);                                             \
            } else if (len == 3) {                                                     \
                set##nameTop(tokens[0]);                                               \
                set##nameLeft(tokens[1]);                                              \
                set##nameRight(tokens[1]);                                             \
                set##nameBottom(tokens[2]);                                            \
            } else if (len == 4) {                                                     \
                set##nameTop(tokens[0]);                                               \
                set##nameRight(tokens[1]);                                             \
                set##nameBottom(tokens[2]);                                            \
                set##nameLeft(tokens[3]);                                              \
            }                                                                          \
        }                                                                              \
    }

    FOR_EACH_STYLE_ATTRIBUTE_FOURSIDE_SHORTHAND(ATTRIBUTE_SETTER_FOURSIDE)
#undef ATTRIBUTE_SETTER_FOURSIDE

    String* Border();
    String* BorderTop();
    String* BorderRight();
    String* BorderBottom();
    String* BorderLeft();
    void setBorder(String* value);
    void setBorderTop(String* value);
    void setBorderRight(String* value);
    void setBorderBottom(String* value);
    void setBorderLeft(String* value);

    String* BackgroundRepeat();
    void setBackgroundRepeat(String* value);

    String* Background();
    void setBackground(String* value);

    static String* combineBoxString(String* t, String* r, String* b, String* l)
    {
        String* space = String::spaceString;
        if (!r->equals(l))
            return t->concat(space)->concat(r)->concat(space)->concat(b)->concat(space)->concat(l);
        else if (!t->equals(b))
            return t->concat(space)->concat(r)->concat(space)->concat(b);
        else if (!t->equals(r))
            return t->concat(space)->concat(r);
        else
            return t;
    }

    unsigned long length() const
    {
        return m_cssValues.size();
    }

    String* item(unsigned long index)
    {
        if (index < m_cssValues.size())
            return m_cssValues.at(index).keyName();
        return String::emptyString;
    }

protected:
    std::vector<CSSStyleValuePair, gc_allocator<CSSStyleValuePair> > m_cssValues;
    Document* m_document;
    Element* m_element;
};

class CSSStyleRule : public ScriptWrappable {
    friend class StyleResolver;

public:
    enum Kind {
        UniversalSelector,
        TypeSelector,
        ClassSelector,
        TypeClassSelector,
        IdSelector,
        TypeIdSelector,
    };

    enum PseudoClass {
        None,
        Active,
        Hover,
    };

    CSSStyleRule(Kind kind, String** ruleText, size_t ruleTextLength, PseudoClass pc, Document* document, CSSStyleDeclaration* decl)
        : ScriptWrappable(this)
        , m_document(document)
    {
        init(kind, ruleText, ruleTextLength, pc, document, decl);
    }

    CSSStyleRule(Kind kind, String* ruleText, PseudoClass pc, Document* document)
        : ScriptWrappable(this)
        , m_document(document)
    {
        String** rt =  new(GC) String*[1];
        rt[0] = ruleText;
        init(kind, rt, 1, pc, document, new CSSStyleDeclaration(document));
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }


    CSSStyleDeclaration* styleDeclaration()
    {
        return m_styleDeclaration;
    }

    Document* document()
    {
        return m_document;
    }

protected:
    void init(Kind kind, String** ruleText, size_t ruleTextLength, PseudoClass pc, Document* document, CSSStyleDeclaration* decl)
    {
        m_kind = kind;
        m_ruleText = ruleText;
        m_ruleTextLength = ruleTextLength;
        m_pseudoClass = pc;
        m_styleDeclaration = decl;
    }

    Kind m_kind;
    PseudoClass m_pseudoClass;
    String** m_ruleText;
    size_t m_ruleTextLength;
    CSSStyleDeclaration* m_styleDeclaration;
    Document* m_document;
};

class CSSStyleSheet : public gc {
public:
    CSSStyleSheet(Node* origin, String* str)
    {
        m_sourceString = str;
        m_origin = origin;
    }

    void addRule(CSSStyleRule* rule)
    {
        m_rules.push_back(rule);
    }

    URL url();
    Node* origin()
    {
        return m_origin;
    }

    void parseSheetIfneeds();

    std::vector<CSSStyleRule*, gc_allocator<CSSStyleRule*> >& rules()
    {
        STARFISH_ASSERT(m_sourceString == String::emptyString);
        return m_rules;
    }
protected:
    // m_stringString != String::emptyString means we need to parse style sheet before access style rules.
    String* m_sourceString;

    std::vector<CSSStyleRule*, gc_allocator<CSSStyleRule*> > m_rules;
    Node* m_origin;
};

class StyleResolver {
public:
    StyleResolver(Document& document)
        : m_document(document)
    {
    }
    void addSheet(CSSStyleSheet* sheet);
    void removeSheet(CSSStyleSheet* sheet)
    {
        STARFISH_ASSERT(std::find(m_sheets.begin(), m_sheets.end(), sheet) != m_sheets.end());
        m_sheets.erase(std::find(m_sheets.begin(), m_sheets.end(), sheet));
    }

    std::vector<CSSStyleSheet*, gc_allocator<CSSStyleSheet*> >& sheets()
    {
        return m_sheets;
    }

    void resolveDOMStyle(Document* document, bool force = false);
#ifdef STARFISH_ENABLE_TEST
    void dumpDOMStyle(Document* document);
#endif
    ComputedStyle* resolveDocumentStyle(Document* doc);
    ComputedStyle* resolveStyle(Element* node, ComputedStyle* parent);

protected:
    Document& m_document;
    std::vector<CSSStyleSheet*, gc_allocator<CSSStyleSheet*> > m_sheets;
};
}

#endif
