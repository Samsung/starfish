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

#ifndef __StarFishComputedStyle__
#define __StarFishComputedStyle__

#include "style/Style.h"
#include "style/StyleBackgroundData.h"
#include "style/StyleSurroundData.h"
#include "style/StyleTransformData.h"
#include "style/StyleTransformOrigin.h"
#include "style/DefaultStyle.h"

namespace StarFish {

class Frame;

enum ComputedStyleDamage {
    ComputedStyleDamageNone = 0,
    ComputedStyleDamageInherited = 1,
    ComputedStyleDamageRebuildFrame = 1 << 1,
    ComputedStyleDamageLayout = 1 << 2,
    ComputedStyleDamagePainting = 1 << 3,
    ComputedStyleDamageComposite = 1 << 4,
};

class ComputedStyle : public gc {
    friend class StyleResolver;
    friend void resolveDOMStyleInner(StyleResolver* resolver, Element* element, ComputedStyle* parentStyle, bool force);
    friend ComputedStyleDamage compareStyle(ComputedStyle* oldStyle, ComputedStyle* newStyle);

public:
    ComputedStyle(float mediumFontSize = DEFAULT_FONT_SIZE)
    {
        m_font = nullptr;

        m_inheritedStyles.m_color = Color(0, 0, 0, 255);
        m_inheritedStyles.m_fontSize = Length(Length::Fixed, mediumFontSize);
        m_inheritedStyles.m_fontWeight = FontWeightValue::NormalFontWeightValue;
        m_inheritedStyles.m_direction = DirectionValue::LtrDirectionValue;
        m_inheritedStyles.m_whiteSpace = WhiteSpaceValue::NormalWhiteSpaceValue;
        m_inheritedStyles.m_visibility = VisibilityValue::VisibleVisibilityValue;
        m_inheritedStyles.m_letterSpacing = Length(Length::Fixed, 0);
        // -100 is used to represent 'normal' value.
        m_inheritedStyles.m_lineHeight = Length(Length::Percent, -100);

        initNonInheritedStyles();
    }

    ComputedStyle(ComputedStyle* from)
    {
        m_font = nullptr;

        m_inheritedStyles = from->m_inheritedStyles;
        initNonInheritedStyles();
    }

    DisplayValue originalDisplay()
    {
        return m_originalDisplay;
    }

    DisplayValue display()
    {
        return m_display;
    }

    void setDisplay(DisplayValue v)
    {
        m_display = v;
    }

    PositionValue position()
    {
        return m_position;
    }

    Length width()
    {
        return m_width;
    }

    void setWidth(const Length& l)
    {
        m_width = l;
    }

    Length height()
    {
        return m_height;
    }

    void setHeight(const Length& l)
    {
        m_height = l;
    }

    void setColor(Color r)
    {
        m_inheritedStyles.m_color = r;
    }

    Color color()
    {
        return m_inheritedStyles.m_color;
    }

    VerticalAlignValue verticalAlign()
    {
        return m_verticalAlign;
    }

    void setVerticalAlign(VerticalAlignValue v)
    {
        m_verticalAlign = v;
    }

    Length verticalAlignLength()
    {
        return m_verticalAlignLength;
    }

    void setVerticalAlignLength(Length l)
    {
        setVerticalAlign(VerticalAlignValue::NumericVAlignValue);
        m_verticalAlignLength = l;
    }

    bool isNumericVerticalAlign()
    {
        return (verticalAlign() == VerticalAlignValue::NumericVAlignValue);
    }

    SideValue textAlign()
    {
        if (m_inheritedStyles.m_textAlign == SideValue::NoneSideValue) {
            if (m_inheritedStyles.m_direction == DirectionValue::RtlDirectionValue)
                return SideValue::RightSideValue;
            return SideValue::LeftSideValue;
        }
        return m_inheritedStyles.m_textAlign;
    }

    void setTextAlign(SideValue t)
    {
        m_inheritedStyles.m_textAlign = t;
    }

    TextDecorationValue textDecoration()
    {
        return m_textDecoration;
    }

    void setTextDecoration(TextDecorationValue decoration)
    {
        m_textDecoration = decoration;
    }

    DirectionValue direction()
    {
        return m_inheritedStyles.m_direction;
    }

    void setDirection(DirectionValue val)
    {
        m_inheritedStyles.m_direction = val;
    }

    WhiteSpaceValue whiteSpace()
    {
        return m_inheritedStyles.m_whiteSpace;
    }

    void setLineHeight(Length length)
    {
        m_inheritedStyles.m_lineHeight = length;
    }

    void setBackgroundIfNeeded()
    {
        if (m_background == NULL) {
            m_background = new StyleBackgroundData();
        }
    }

    void setSurroundIfNeeded()
    {
        if (m_surround == nullptr) {
            m_surround = new StyleSurroundData();
        }
    }

    bool hasTransforms(Frame* frame);

    // DO NOT USE THIS FUNCTION
    StyleTransformDataGroup* uncheckedTransforms()
    {
        return m_transforms;
    }

    StyleTransformDataGroup* transforms(Frame* frame);

    SkMatrix transformsToMatrix(LayoutUnit containerWidth, LayoutUnit containerHeight, bool isTransformable);

    void setTransformIfNeeded()
    {
        if (m_transforms == NULL) {
            m_transforms = new StyleTransformDataGroup();
        }
    }

    void setTransformMatrix(double a, double b, double c, double d, double e, double f)
    {
        setTransformIfNeeded();
        StyleTransformData t(StyleTransformData::OperationType::Matrix);
        t.setMatrix(a, b, c, d, e, f);
        m_transforms->append(t);
    }

    void setTransformScale(double a, double b)
    {
        setTransformIfNeeded();
        StyleTransformData t(StyleTransformData::OperationType::Scale);
        t.setScale(a, b);
        m_transforms->append(t);
    }

    void setTransformRotate(double a)
    {
        setTransformIfNeeded();
        StyleTransformData t(StyleTransformData::OperationType::Rotate);
        t.setRotate(a);
        m_transforms->append(t);
    }

    void setTransformSkew(double a, double b)
    {
        setTransformIfNeeded();
        StyleTransformData t(StyleTransformData::OperationType::Skew);
        t.setSkew(a, b);
        m_transforms->append(t);
    }

    void setTransformTranslate(Length a, Length b)
    {
        setTransformIfNeeded();
        StyleTransformData t(StyleTransformData::OperationType::Translate);
        t.setTranslate(a, b);
        m_transforms->append(t);
    }

    void setTransformOriginIfNeeded()
    {
        if (m_transformOrigin == NULL)
            m_transformOrigin = new StyleTransformOrigin();
    }

    void setTransformOriginValue(Length x, Length y)
    {
        setTransformOriginIfNeeded();
        m_transformOrigin->setOriginValue(x, y);
    }

    void setBackgroundColor(Color color)
    {
        setBackgroundIfNeeded();
        m_background->setBgColor(color);
    }

    void setBackgroundColorToCurrentColor()
    {
        setBackgroundIfNeeded();
        m_background->setBgColorToCurrentColor();
    }

    void setBackgroundImage(String* img, unsigned int layer = 0)
    {
        setBackgroundIfNeeded();
        m_background->setBgImage(img, layer);
    }

    void setBackgroundImageResource(ImageResource* img, unsigned int layer = 0)
    {
        setBackgroundIfNeeded();
        m_background->setBgImageResource(img, layer);
    }

    void setBackgroundRepeatX(BackgroundRepeatValue repeat, unsigned int layer = 0)
    {
        setBackgroundIfNeeded();
        m_background->setRepeatX(repeat, layer);
    }

    void setBackgroundRepeatY(BackgroundRepeatValue repeat, unsigned int layer = 0)
    {
        setBackgroundIfNeeded();
        m_background->setRepeatY(repeat, layer);
    }

    void setBackgroundPositionX(Length value, unsigned int layer = 0)
    {
        setBackgroundIfNeeded();
        m_background->setPositionX(value, layer);
    }

    void setBackgroundPositionY(Length value, unsigned int layer = 0)
    {
        setBackgroundIfNeeded();
        m_background->setPositionY(value, layer);
    }

    void setBackgroundSizeType(BackgroundSizeType type, unsigned int layer = 0)
    {
        setBackgroundIfNeeded();
        m_background->setSizeType(type, layer);
    }

    void setBackgroundSizeValue(LengthSize size, unsigned int layer = 0)
    {
        setBackgroundIfNeeded();
        m_background->setSizeValue(size, layer);
    }

    unsigned int backgroundLayerSize()
    {
        if (m_background == NULL)
            return 0;
        return m_background->sizeOfLayers();
    }

    Color backgroundColor()
    {
        if (m_background == NULL)
            return Color();
        return m_background->bgColor();
    }

    String* backgroundImage(unsigned int layer = 0)
    {
        if (m_background == NULL)
            return String::emptyString;
        return m_background->bgImage(layer);
    }

    ImageData* backgroundImageData(unsigned int layer = 0)
    {
        if (m_background == NULL)
            return NULL;
        return m_background->bgImageData(layer);
    }

    BackgroundRepeatValue backgroundRepeatX(unsigned int layer = 0)
    {
        if (m_background == NULL)
            return BackgroundRepeatValue::RepeatRepeatValue;
        return m_background->repeatX(layer);
    }

    BackgroundRepeatValue backgroundRepeatY(unsigned int layer = 0)
    {
        if (m_background == NULL)
            return BackgroundRepeatValue::RepeatRepeatValue;
        return m_background->repeatY(layer);
    }

    Length backgroundPositionX(unsigned int layer = 0)
    {
        if (m_background == NULL)
            return Length(Length::Percent, 0.0f);
        return m_background->positionX(layer);
    }

    Length backgroundPositionY(unsigned int layer = 0)
    {
        if (m_background == NULL)
            return Length(Length::Percent, 0.0f);
        return m_background->positionY(layer);
    }

    BackgroundSizeType bgSizeType(unsigned int layer = 0)
    {
        if (m_background == NULL)
            return BackgroundSizeType::SizeValue;
        return m_background->sizeType(layer);
    }

    LengthSize bgSizeValue(unsigned int layer = 0)
    {
        if (m_background == NULL)
            return LengthSize();

        STARFISH_ASSERT(m_background && m_background->sizeType() == BackgroundSizeType::SizeValue);
        return m_background->sizeValue(layer);
    }

    Font* font()
    {
        STARFISH_ASSERT(m_font);
        return m_font;
    }

    Length lineHeight()
    {
        // According to the CSS spec, the computed value is the absolute value for <length> and <percentage> & otherwise as specified.
        // However, our computed value is the absolute value.
        return m_inheritedStyles.m_lineHeight;
    }

    bool hasNormalLineHeight()
    {
        return m_inheritedStyles.m_lineHeight.isPercent() && m_inheritedStyles.m_lineHeight.percent() == -100;
    }

    float opacity()
    {
        return m_opacity;
    }

    int32_t zIndex()
    {
        return m_zIndex;
    }

    bool IsSpecifiedZIndex()
    {
        return m_zIndexSpecifiedByUser;
    }

    StyleBackgroundData* background()
    {
        return m_background;
    }

    bool hasBorderColor()
    {
        if (m_surround == nullptr) {
            return false;
        } else {
            return m_surround->border.hasBorderColor();
        }
    }

    bool hasBorderStyle()
    {
        if (m_surround == nullptr) {
            return false;
        } else {
            return m_surround->border.hasBorderStyle();
        }
    }

    bool hasBorderImageData()
    {
        if (m_surround == nullptr) {
            return false;
        } else {
            return m_surround->border.hasBorderImageData();
        }
    }

#define BORDER_COLOR(UPOS, LPOS, ...) \
    Color border##UPOS##Color() \
    { \
        if (m_surround == nullptr || !m_surround->border.LPOS().hasBorderColor()) { \
            return m_inheritedStyles.m_color; \
        } else { \
            return m_surround->border.LPOS().color(); \
        } \
    }
    GEN_FOURSIDE(BORDER_COLOR)
#undef BORDER_COLOR

#define BORDER_STYLE(UPOS, LPOS, ...) \
    BorderStyleValue border##UPOS##Style() \
    { \
        if (m_surround == nullptr) { \
            return initialBorderStyle(); \
        } else { \
            return m_surround->border.LPOS().style(); \
        } \
    }
    GEN_FOURSIDE(BORDER_STYLE)
#undef BORDER_STYLE

#define BORDER_WIDTH(UPOS, LPOS, ...) \
    Length border##UPOS##Width() \
    { \
        if (m_surround == nullptr) { \
            return initialBorderWidth(); \
        } else { \
            return m_surround->border.LPOS().width(); \
        } \
    }
    GEN_FOURSIDE(BORDER_WIDTH)
#undef BORDER_WIDTH

    StyleTransformOrigin* transformOrigin()
    {
        return m_transformOrigin;
    }

    bool hasTransformOrigin()
    {
        return m_transformOrigin != nullptr;
    }

    void clearBorderTopColor()
    {
        if (m_surround)
            surround()->border.top().clearColor();
    }

    void clearBorderRightColor()
    {
        if (m_surround)
            surround()->border.right().clearColor();
    }

    void clearBorderBottomColor()
    {
        if (m_surround)
            surround()->border.bottom().clearColor();
    }

    void clearBorderLeftColor()
    {
        if (m_surround)
            surround()->border.left().clearColor();
    }

#define SET_BORDER_COLOR(UPOS, LPOS, ...) \
    void setBorder##UPOS##Color(Color color) \
    { \
        setSurroundIfNeeded(); \
        surround()->border.LPOS().setColor(color); \
    }
    GEN_FOURSIDE(SET_BORDER_COLOR)
#undef SET_BORDER_COLOR

#define SET_BORDER_STYLE(UPOS, LPOS, ...) \
    void setBorder##UPOS##Style(BorderStyleValue style) \
    { \
        setSurroundIfNeeded(); \
        surround()->border.LPOS().setStyle(style); \
    }
    GEN_FOURSIDE(SET_BORDER_STYLE)
#undef SET_BORDER_STYLE

#define SET_BORDER_WIDTH(UPOS, LPOS, ...) \
    void setBorder##UPOS##Width(Length width) \
    { \
        setSurroundIfNeeded(); \
        surround()->border.LPOS().setWidth(width); \
    }
    GEN_FOURSIDE(SET_BORDER_WIDTH)
#undef SET_BORDER_WIDTH

    String* borderImageSource()
    {
        if (m_surround)
            return surround()->border.image().url();
        return initialBorderImageSource();
    }

    LengthBox borderImageSlices()
    {
        if (m_surround)
            return surround()->border.image().slices();
        return initialBorderImageSlices();
    }

    bool borderImageSliceFill()
    {
        if (m_surround)
            return surround()->border.image().sliceFill();
        return initialBorderImageSliceFill();
    }

    BorderImageLengthBox borderImageWidths()
    {
        if (m_surround)
            return surround()->border.image().widths();
        return initialBorderImageWidths();
    }

    void setBorderImageSource(String* url)
    {
        setSurroundIfNeeded();
        surround()->border.image().setUrl(url);
    }

    void setBorderImageSlices(LengthBox slices)
    {
        setSurroundIfNeeded();
        surround()->border.image().setSlices(slices);
    }

    void setBorderImageSliceFill(bool fill)
    {
        setSurroundIfNeeded();
        surround()->border.image().setSliceFill(fill);
    }

    void setBorderImageWidths(BorderImageLengthBox value)
    {
        setSurroundIfNeeded();
        surround()->border.image().setWidths(value);
    }

    void setBorderImageResource(ImageResource* value)
    {
        setSurroundIfNeeded();
        surround()->border.image().setImageResource(value);
    }

    void setBorderImageSliceFromOther(ComputedStyle* other)
    {
        setBorderImageSlices(other->borderImageSlices());
        setBorderImageSliceFill(other->borderImageSliceFill());
    }

    StyleSurroundData* surround()
    {
        return m_surround;
    }

    OverflowValue overflow()
    {
        return m_overflow;
    }

#define SET_SIDE(UPOS, ...) \
    void set##UPOS(Length unit) \
    { \
        setSurroundIfNeeded(); \
        surround()->offset.set##UPOS(unit); \
    }
    GEN_FOURSIDE(SET_SIDE)
#undef SET_SIDE

#define SET_MARGIN(UPOS, ...) \
    void setMargin##UPOS(Length unit) \
    { \
        setSurroundIfNeeded(); \
        surround()->margin.set##UPOS(unit); \
    }
    GEN_FOURSIDE(SET_MARGIN)
#undef SET_MARGIN

#define SET_PADDING(UPOS, ...) \
    void setPadding##UPOS(Length unit) \
    { \
        setSurroundIfNeeded(); \
        surround()->padding.set##UPOS(unit); \
    }
    GEN_FOURSIDE(SET_PADDING)
#undef SET_PADDING

#define GET_SIDE(UPOS, LPOS, ...) \
    Length LPOS() \
    { \
        if (m_surround == nullptr) { \
            return Length(); \
        } else { \
            return m_surround->offset.LPOS(); \
        } \
    }
    GEN_FOURSIDE(GET_SIDE)
#undef GET_SIDE

#define GET_MARGIN(UPOS, LPOS, ...) \
    Length margin##UPOS() \
    { \
        if (m_surround == nullptr) { \
            return initialMargin(); \
        } else { \
            return m_surround->margin.LPOS(); \
        } \
    }
    GEN_FOURSIDE(GET_MARGIN)
#undef GET_MARGIN

#define GET_PADDING(UPOS, LPOS, ...) \
    Length padding##UPOS() \
    { \
        if (m_surround == nullptr) { \
            return initialPadding(); \
        } else { \
            return m_surround->padding.LPOS(); \
        } \
    }
    GEN_FOURSIDE(GET_PADDING)
#undef GET_PADDING

    Length fontSize()
    {
        return m_inheritedStyles.m_fontSize;
    }

    void setLetterSpacing(Length len)
    {
        m_inheritedStyles.m_letterSpacing = len;
    }

    VisibilityValue visibility()
    {
        return m_inheritedStyles.m_visibility;
    }

    FontStyleValue fontStyle()
    {
        return m_inheritedStyles.m_fontStyle;
    }

    FontWeightValue fontWeight()
    {
        return m_inheritedStyles.m_fontWeight;
    }

    Length letterSpacing()
    {
        return m_inheritedStyles.m_letterSpacing;
    }

    static VerticalAlignValue initialVerticalAlign() { return VerticalAlignValue::BaselineVAlignValue; }
    static SideValue initialTextAlign() { return SideValue::NoneSideValue; }
    static Length initialPadding() { return Length(Length::Fixed, 0); }
    static Length initialMargin() { return Length(Length::Fixed, 0); }
    static Length initialBorderWidth() { return Length(Length::Fixed, 0); }
    static BorderStyleValue initialBorderStyle() { return BorderStyleValue::NoneBorderStyleValue; }
    static String* initialBgImage() { return String::emptyString; }
    static String* initialBorderImageSource() { return String::emptyString; }
    static BorderImageLengthBox initialBorderImageWidths() { return BorderImageLengthBox(1.0); }
    static LengthBox initialBorderImageSlices() { return LengthBox(Length(Length::Fixed, 0), Length(Length::Fixed, 0), Length(Length::Fixed, 0), Length(Length::Fixed, 0)); }
    static bool initialBorderImageSliceFill() { return false; }

    void loadResources(Node* consumer, ComputedStyle* prevComputedStyleValueForReferenceLoadedResources = nullptr);
    void arrangeStyleValues(ComputedStyle* parentStyle, Node* current = nullptr);

    void clearTransforms()
    {
        m_transforms = nullptr;
    }

    void setUnicodeBidi(UnicodeBidiValue value)
    {
        m_unicodeBidi = value;
    }

    UnicodeBidiValue unicodeBidi()
    {
        return m_unicodeBidi;
    }

protected:
    void initNonInheritedStyles()
    {
        m_display = DisplayValue::InlineDisplayValue;
        m_opacity = 1;
        m_zIndex = 0;
        m_zIndexSpecifiedByUser = false;
        m_background = nullptr;
        m_surround = nullptr;
        m_overflow = OverflowValue::VisibleOverflow;
        m_textDecoration = TextDecorationValue::NoneTextDecorationValue;
        m_verticalAlign = initialVerticalAlign();
        m_transforms = nullptr;
        m_transformOrigin = nullptr;
        m_unicodeBidi = UnicodeBidiValue::NormalUnicodeBidiValue;
    }

    // NOTICE
    // if you add new property, you MUST implement comparing style for new property in [compareStyle function]

    struct InheritedStyles {
        Color m_color;
        Length m_fontSize;
        Length m_letterSpacing;
        Length m_lineHeight;
        FontStyleValue m_fontStyle : 2;
        FontWeightValue m_fontWeight : 4;
        SideValue m_textAlign : 3;
        DirectionValue m_direction : 2;
        WhiteSpaceValue m_whiteSpace : 1;
        VisibilityValue m_visibility : 1;
    } m_inheritedStyles;

    DisplayValue m_display : 3;
    DisplayValue m_originalDisplay : 3;
    PositionValue m_position : 2;
    VerticalAlignValue m_verticalAlign : 4;
    OverflowValue m_overflow : 1;
    TextDecorationValue m_textDecoration : 3;
    UnicodeBidiValue m_unicodeBidi : 2;
    bool m_zIndexSpecifiedByUser : 1;

    Length m_width;
    Length m_height;
    Length m_verticalAlignLength;

    float m_opacity;
    int32_t m_zIndex;
    Font* m_font;
    StyleBackgroundData* m_background;
    StyleSurroundData* m_surround;
    StyleTransformDataGroup* m_transforms;
    StyleTransformOrigin* m_transformOrigin;
};

ComputedStyleDamage compareStyle(ComputedStyle* oldStyle, ComputedStyle* newStyle);
}

#endif
