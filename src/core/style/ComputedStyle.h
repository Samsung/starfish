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

#ifndef __StarFishComputedStyle__
#define __StarFishComputedStyle__

#include "core/style/ContentData.h"
#include "core/style/FlexBasisData.h"
#include "core/style/DefaultStyle.h"
#include "core/style/Style.h"
#include "core/style/StyleBackgroundData.h"
#include "core/style/StyleSurroundData.h"
#include "core/style/StyleTransformData.h"
#include "core/style/StyleTransformOrigin.h"
#include "core/style/StyleTransitionData.h"

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

class RareComputedStyleData : public gc {
public:
    RareComputedStyleData()
        : m_transforms(nullptr)
        , m_transformOrigin(nullptr)
        , m_transition(nullptr)
    {
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    Length m_minWidth;
    Length m_maxWidth;
    Length m_minHeight;
    Length m_maxHeight;

    StyleTransformDataGroup* m_transforms;
    StyleTransformOrigin* m_transformOrigin;
    StyleTransitionData* m_transition;

    ContentDataGroup m_content;
    GCVector<ComputedStyle*> m_cachedPseudoStyles;
};

class ComputedStyle : public gc {
    friend class StyleResolver;
    friend void resolveDOMStyleInner(StyleResolver* resolver, Element* element,
                                     ComputedStyle* parentStyle, bool force);
    friend ComputedStyleDamage compareStyle(ComputedStyle* oldStyle,
                                            ComputedStyle* newStyle);

public:
    ComputedStyle(float mediumFontSize = DEFAULT_FONT_SIZE)
    {
        m_font = nullptr;

        m_inheritedStyles.m_color = Unit::Color(0, 0, 0, 255);
        m_inheritedStyles.m_fontSize = Length(Length::Fixed, mediumFontSize);
        m_inheritedStyles.m_fontWeight = FontWeightValue::NormalFontWeightValue;
        m_inheritedStyles.m_textAlign = TextAlignValue::StartTextAlignValue;
        m_inheritedStyles.m_direction = DirectionValue::LtrDirectionValue;
        m_inheritedStyles.m_whiteSpace = WhiteSpaceValue::NormalWhiteSpaceValue;
        m_inheritedStyles.m_visibility =
            VisibilityValue::VisibleVisibilityValue;
        m_inheritedStyles.m_borderCollapse =
            BorderCollapseValue::SeparateBorderCollapseValue;
        m_inheritedStyles.m_captionSide = CaptionSideValue::TopCaptionSideValue;
        m_inheritedStyles.m_emptyCells = EmptyCellsValue::ShowEmptyCellsValue;

        m_inheritedStyles.m_letterSpacing = Length(Length::Fixed, 0);
        // -100 is used to represent 'normal' value.
        m_inheritedStyles.m_lineHeight = Length(Length::Percent, -100);
        m_inheritedStyles.m_textIndent = Length(Length::Fixed, 0);
        m_inheritedStyles.m_horizontalBorderSpacing = Length(Length::Fixed, 0);
        m_inheritedStyles.m_verticalBorderSpacing = Length(Length::Fixed, 0);
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

    void setPosition(PositionValue p)
    {
        m_position = p;
    }

    FloatValue floating()
    {
        return m_float;
    }

    ClearValue clear()
    {
        return m_clear;
    }

    Length width()
    {
        return m_width;
    }

    Length maxWidth()
    {
        if (!hasRareComputeStyleData()) {
            return Length();
        }
        return m_rareComputedStyleData->m_maxWidth;
    }

    Length minWidth()
    {
        if (!hasRareComputeStyleData()) {
            return Length();
        }
        return m_rareComputedStyleData->m_minWidth;
    }

    void setWidth(const Length& l)
    {
        m_width = l;
    }

    void setMaxWidth(const Length& l)
    {
        setRareComputedStyleDataIfNeeded();
        m_rareComputedStyleData->m_maxWidth = l;
    }

    void setMinWidth(const Length& l)
    {
        setRareComputedStyleDataIfNeeded();
        m_rareComputedStyleData->m_minWidth = l;
    }

    Length height()
    {
        return m_height;
    }

    Length maxHeight()
    {
        if (!hasRareComputeStyleData()) {
            return Length();
        }
        return m_rareComputedStyleData->m_maxHeight;
    }

    Length minHeight()
    {
        if (!hasRareComputeStyleData()) {
            return Length();
        }
        return m_rareComputedStyleData->m_minHeight;
    }

    void setHeight(const Length& l)
    {
        m_height = l;
    }

    void setMaxHeight(const Length& l)
    {
        setRareComputedStyleDataIfNeeded();
        m_rareComputedStyleData->m_maxHeight = l;
    }

    void setMinHeight(const Length& l)
    {
        setRareComputedStyleDataIfNeeded();
        m_rareComputedStyleData->m_minHeight = l;
    }

    void setColor(Unit::Color r)
    {
        m_inheritedStyles.m_color = r;
    }

    Unit::Color color()
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

    TextAlignValue textAlign()
    {
        return m_inheritedStyles.m_textAlign;
    }

    void setTextAlign(TextAlignValue t)
    {
        m_inheritedStyles.m_textAlign = t;
    }

    Length textIndent()
    {
        return m_inheritedStyles.m_textIndent;
    }

    void setTextIndent(Length val)
    {
        m_inheritedStyles.m_textIndent = val;
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

    void setWhiteSpace(WhiteSpaceValue val)
    {
        m_inheritedStyles.m_whiteSpace = val;
    }

    void setLineHeight(Length length)
    {
        m_inheritedStyles.m_lineHeight = length;
    }

    void setBackgroundIfNeeded()
    {
        if (m_background == nullptr) {
            m_background = new StyleBackgroundData();
        }
    }

    void setSurroundIfNeeded()
    {
        if (m_surround == nullptr) {
            m_surround = new StyleSurroundData();
        }
    }

    bool hasTransition()
    {
        if (hasRareComputeStyleData() &&
            m_rareComputedStyleData->m_transition) {
            return true;
        }
        return false;
    }

    void setTransitionIfNeeded()
    {
        setRareComputedStyleDataIfNeeded();
        if (m_rareComputedStyleData->m_transition == nullptr) {
            m_rareComputedStyleData->m_transition = new StyleTransitionData();
        }
    }

    bool hasTransforms()
    {
        if (hasRareComputeStyleData() &&
            m_rareComputedStyleData->m_transforms) {
            return true;
        }
        return false;
    }

    bool hasTransforms(Frame* frame);
    bool hasComplexTransforms(Frame* frame);

    StyleTransformDataGroup* transforms(Frame* frame = nullptr);

    SkMatrix transformsToMatrix(LayoutUnit containerWidth,
                                LayoutUnit containerHeight,
                                LayoutUnit viewportWidth,
                                LayoutUnit viewportHeight,
                                bool isTransformable);

    void setTransformIfNeeded()
    {
        setRareComputedStyleDataIfNeeded();
        if (m_rareComputedStyleData->m_transforms == nullptr) {
            m_rareComputedStyleData->m_transforms =
                new StyleTransformDataGroup();
        }
    }

    void setTransform(StyleTransformDataGroup* transform)
    {
        setTransformIfNeeded();
        m_rareComputedStyleData->m_transforms = transform;
    }

    void setTransformMatrix(double a, double b, double c, double d, double e,
                            double f)
    {
        setTransformIfNeeded();
        StyleTransformData t(StyleTransformData::OperationType::Matrix);
        t.setMatrix(a, b, c, d, e, f);
        m_rareComputedStyleData->m_transforms->append(t);
    }

    void setTransformScale(double a, double b)
    {
        setTransformIfNeeded();
        StyleTransformData t(StyleTransformData::OperationType::Scale);
        t.setScale(a, b);
        m_rareComputedStyleData->m_transforms->append(t);
    }

    void setTransformRotate(double a)
    {
        setTransformIfNeeded();
        StyleTransformData t(StyleTransformData::OperationType::Rotate);
        t.setRotate(a);
        m_rareComputedStyleData->m_transforms->append(t);
    }

    void setTransformSkew(double a, double b)
    {
        setTransformIfNeeded();
        StyleTransformData t(StyleTransformData::OperationType::Skew);
        t.setSkew(a, b);
        m_rareComputedStyleData->m_transforms->append(t);
    }

    void setTransformTranslate(Length a, Length b)
    {
        setTransformIfNeeded();
        StyleTransformData t(StyleTransformData::OperationType::Translate);
        t.setTranslate(a, b);
        m_rareComputedStyleData->m_transforms->append(t);
    }

    void setTransformOriginIfNeeded()
    {
        setRareComputedStyleDataIfNeeded();
        if (m_rareComputedStyleData->m_transformOrigin == nullptr) {
            m_rareComputedStyleData->m_transformOrigin =
                new StyleTransformOrigin();
        }
    }

    void setTransformOrigin(StyleTransformOrigin* transformOrigin)
    {
        setTransformOriginIfNeeded();
        m_rareComputedStyleData->m_transformOrigin = transformOrigin;
    }

    void setTransformOriginValue(Length x, Length y, Length z)
    {
        setTransformOriginIfNeeded();
        m_rareComputedStyleData->m_transformOrigin->setOriginValue(x, y, z);
    }

    void setBackgroundColor(Unit::Color color)
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

    void setBackgroundRepeatX(BackgroundRepeatValue repeat,
                              unsigned int layer = 0)
    {
        setBackgroundIfNeeded();
        m_background->setRepeatX(repeat, layer);
    }

    void setBackgroundRepeatY(BackgroundRepeatValue repeat,
                              unsigned int layer = 0)
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
        if (m_background == nullptr) {
            return 0;
        }
        return m_background->sizeOfLayers();
    }

    Unit::Color backgroundColor()
    {
        if (m_background == nullptr) {
            return Unit::Color();
        }
        return m_background->bgColor();
    }

    String* backgroundImage(unsigned int layer = 0)
    {
        if (m_background == nullptr) {
            return String::emptyString;
        }
        return m_background->bgImage(layer);
    }

    ImageData* backgroundImageData(unsigned int layer = 0)
    {
        if (m_background == nullptr) {
            return nullptr;
        }
        return m_background->bgImageData(layer);
    }

    BackgroundRepeatValue backgroundRepeatX(unsigned int layer = 0)
    {
        if (m_background == nullptr) {
            return BackgroundRepeatValue::RepeatRepeatValue;
        }
        return m_background->repeatX(layer);
    }

    BackgroundRepeatValue backgroundRepeatY(unsigned int layer = 0)
    {
        if (m_background == nullptr) {
            return BackgroundRepeatValue::RepeatRepeatValue;
        }
        return m_background->repeatY(layer);
    }

    Length backgroundPositionX(unsigned int layer = 0)
    {
        if (m_background == nullptr) {
            return Length(Length::Percent, 0.0f);
        }
        return m_background->positionX(layer);
    }

    Length backgroundPositionY(unsigned int layer = 0)
    {
        if (m_background == nullptr) {
            return Length(Length::Percent, 0.0f);
        }
        return m_background->positionY(layer);
    }

    BackgroundSizeType bgSizeType(unsigned int layer = 0)
    {
        if (m_background == nullptr) {
            return BackgroundSizeType::SizeValue;
        }
        return m_background->sizeType(layer);
    }

    LengthSize bgSizeValue(unsigned int layer = 0)
    {
        if (m_background == nullptr) {
            return LengthSize();
        }

        STARFISH_ASSERT(m_background &&
                        m_background->sizeType() ==
                            BackgroundSizeType::SizeValue);
        return m_background->sizeValue(layer);
    }

    Font* font()
    {
        STARFISH_ASSERT(m_font);
        return m_font;
    }

    Length lineHeight()
    {
        // According to the CSS spec, the computed value is the absolute value
        // for <length> and <percentage> & otherwise as specified.
        // However, our computed value is the absolute value.
        return m_inheritedStyles.m_lineHeight;
    }

    bool hasNormalLineHeight()
    {
        return m_inheritedStyles.m_lineHeight.isPercent() &&
               m_inheritedStyles.m_lineHeight.percent() == -100;
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

#define BORDER_COLOR(UPOS, LPOS, ...)                      \
    Unit::Color border##UPOS##Color()                      \
    {                                                      \
        if (m_surround == nullptr ||                       \
            !m_surround->border.LPOS().hasBorderColor()) { \
            return m_inheritedStyles.m_color;              \
        } else {                                           \
            return m_surround->border.LPOS().color();      \
        }                                                  \
    }
    GEN_FOURSIDE(BORDER_COLOR)
#undef BORDER_COLOR

#define BORDER_STYLE(UPOS, LPOS, ...)                 \
    BorderStyleValue border##UPOS##Style()            \
    {                                                 \
        if (m_surround == nullptr) {                  \
            return initialBorderStyle();              \
        } else {                                      \
            return m_surround->border.LPOS().style(); \
        }                                             \
    }
    GEN_FOURSIDE(BORDER_STYLE)
#undef BORDER_STYLE

#define BORDER_WIDTH(UPOS, LPOS, ...)                 \
    Length border##UPOS##Width()                      \
    {                                                 \
        if (m_surround == nullptr) {                  \
            return initialBorderWidth();              \
        } else {                                      \
            return m_surround->border.LPOS().width(); \
        }                                             \
    }
    GEN_FOURSIDE(BORDER_WIDTH)
#undef BORDER_WIDTH

    StyleTransformOrigin* transformOrigin()
    {
        STARFISH_ASSERT(hasRareComputeStyleData());
        return m_rareComputedStyleData->m_transformOrigin;
    }

    bool hasTransformOrigin()
    {
        if (hasRareComputeStyleData() &&
            m_rareComputedStyleData->m_transformOrigin) {
            return true;
        }
        return false;
    }

    void clearBorderTopColor()
    {
        if (m_surround) {
            surround()->border.top().clearColor();
        }
    }

    void clearBorderRightColor()
    {
        if (m_surround) {
            surround()->border.right().clearColor();
        }
    }

    void clearBorderBottomColor()
    {
        if (m_surround) {
            surround()->border.bottom().clearColor();
        }
    }

    void clearBorderLeftColor()
    {
        if (m_surround) {
            surround()->border.left().clearColor();
        }
    }

#define SET_BORDER_COLOR(UPOS, LPOS, ...)          \
    void setBorder##UPOS##Color(Unit::Color color) \
    {                                              \
        setSurroundIfNeeded();                     \
        surround()->border.LPOS().setColor(color); \
    }
    GEN_FOURSIDE(SET_BORDER_COLOR)
#undef SET_BORDER_COLOR

#define SET_BORDER_STYLE(UPOS, LPOS, ...)               \
    void setBorder##UPOS##Style(BorderStyleValue style) \
    {                                                   \
        setSurroundIfNeeded();                          \
        surround()->border.LPOS().setStyle(style);      \
    }
    GEN_FOURSIDE(SET_BORDER_STYLE)
#undef SET_BORDER_STYLE

#define SET_BORDER_WIDTH(UPOS, LPOS, ...)          \
    void setBorder##UPOS##Width(Length width)      \
    {                                              \
        setSurroundIfNeeded();                     \
        surround()->border.LPOS().setWidth(width); \
    }
    GEN_FOURSIDE(SET_BORDER_WIDTH)
#undef SET_BORDER_WIDTH

    String* borderImageSource()
    {
        if (m_surround) {
            return surround()->border.image().url();
        }
        return initialBorderImageSource();
    }

    LengthBox borderImageSlices()
    {
        if (m_surround) {
            return surround()->border.image().slices();
        }
        return initialBorderImageSlices();
    }

    bool borderImageSliceFill()
    {
        if (m_surround) {
            return surround()->border.image().sliceFill();
        }
        return initialBorderImageSliceFill();
    }

    BorderImageLengthBox borderImageWidths()
    {
        if (m_surround) {
            return surround()->border.image().widths();
        }
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

    OverflowValue overflowX()
    {
        return m_overflowX;
    }

    OverflowValue overflowY()
    {
        return m_overflowY;
    }

    TransitionPropertyValue transitionProperty()
    {
        if (!hasTransition()) {
            return TransitionPropertyValue::TransitionPropertyAllValue;
        }
        return m_rareComputedStyleData->m_transition->transitionProperty();
    }

    void setTransitionProperty(TransitionPropertyValue property)
    {
        setTransitionIfNeeded();
        m_rareComputedStyleData->m_transition->setTransitionProperty(property);
    }

    CSSTime transitionDuration()
    {
        if (!hasTransition()) {
            return CSSTime(0);
        }
        return m_rareComputedStyleData->m_transition->transitionDuration();
    }

    void setTransitionDuration(CSSTime duration)
    {
        setTransitionIfNeeded();
        m_rareComputedStyleData->m_transition->setTransitionDuration(duration);
    }

#define SET_SIDE(UPOS, ...)                 \
    void set##UPOS(Length unit)             \
    {                                       \
        setSurroundIfNeeded();              \
        surround()->offset.set##UPOS(unit); \
    }
    GEN_FOURSIDE(SET_SIDE)
#undef SET_SIDE

#define SET_MARGIN(UPOS, ...)               \
    void setMargin##UPOS(Length unit)       \
    {                                       \
        setSurroundIfNeeded();              \
        surround()->margin.set##UPOS(unit); \
    }
    GEN_FOURSIDE(SET_MARGIN)
#undef SET_MARGIN

#define SET_PADDING(UPOS, ...)               \
    void setPadding##UPOS(Length unit)       \
    {                                        \
        setSurroundIfNeeded();               \
        surround()->padding.set##UPOS(unit); \
    }
    GEN_FOURSIDE(SET_PADDING)
#undef SET_PADDING

#define GET_SIDE(UPOS, LPOS, ...)             \
    Length LPOS()                             \
    {                                         \
        if (m_surround == nullptr) {          \
            return Length();                  \
        } else {                              \
            return m_surround->offset.LPOS(); \
        }                                     \
    }
    GEN_FOURSIDE(GET_SIDE)
#undef GET_SIDE

#define GET_MARGIN(UPOS, LPOS, ...)           \
    Length margin##UPOS()                     \
    {                                         \
        if (m_surround == nullptr) {          \
            return initialMargin();           \
        } else {                              \
            return m_surround->margin.LPOS(); \
        }                                     \
    }
    GEN_FOURSIDE(GET_MARGIN)
#undef GET_MARGIN

#define GET_PADDING(UPOS, LPOS, ...)           \
    Length padding##UPOS()                     \
    {                                          \
        if (m_surround == nullptr) {           \
            return initialPadding();           \
        } else {                               \
            return m_surround->padding.LPOS(); \
        }                                      \
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

    static VerticalAlignValue initialVerticalAlign()
    {
        return VerticalAlignValue::BaselineVAlignValue;
    }
    static Length initialPadding()
    {
        return Length(Length::Fixed, 0);
    }
    static Length initialMargin()
    {
        return Length(Length::Fixed, 0);
    }
    static Length initialBorderWidth()
    {
        return Length(Length::Fixed, 0);
    }
    static BorderStyleValue initialBorderStyle()
    {
        return BorderStyleValue::NoneBorderStyleValue;
    }
    static String* initialBgImage()
    {
        return String::emptyString;
    }
    static String* initialBorderImageSource()
    {
        return String::emptyString;
    }
    static BorderImageLengthBox initialBorderImageWidths()
    {
        return BorderImageLengthBox(1.0);
    }
    static LengthBox initialBorderImageSlices()
    {
        return LengthBox(Length(Length::Fixed, 0), Length(Length::Fixed, 0),
                         Length(Length::Fixed, 0), Length(Length::Fixed, 0));
    }
    static bool initialBorderImageSliceFill()
    {
        return false;
    }
    static bool isDisplayTableValueType(DisplayValue display)
    {
        return display == DisplayValue::TableDisplayValue ||
               display == DisplayValue::InlineTableDisplayValue ||
               display == DisplayValue::TableRowGroupDisplayValue ||
               display == DisplayValue::TableHeaderGroupDisplayValue ||
               display == DisplayValue::TableFooterGroupDisplayValue ||
               display == DisplayValue::TableRowDisplayValue ||
               display == DisplayValue::TableColumnGroupDisplayValue ||
               display == DisplayValue::TableColumnDisplayValue ||
               display == DisplayValue::TableCellDisplayValue ||
               display == DisplayValue::TableCaptionDisplayValue;
    }

    void loadResources(
        Node* consumer,
        ComputedStyle* prevComputedStyleValueForReferenceLoadedResources =
            nullptr);
    void arrangeStyleValues(ComputedStyle* parentStyle,
                            ComputedStyle* rootStyle, Node* current = nullptr);
    void blockify(Node* current, bool force);

    void clearTransforms()
    {
        if (hasRareComputeStyleData()) {
            m_rareComputedStyleData->m_transforms = nullptr;
        }
    }

    void setUnicodeBidi(UnicodeBidiValue value)
    {
        m_unicodeBidi = value;
    }

    UnicodeBidiValue unicodeBidi()
    {
        return m_unicodeBidi;
    }

    void setBoxSizing(BoxSizingValue value)
    {
        m_boxSizing = value;
    }

    BoxSizingValue boxSizing()
    {
        return m_boxSizing;
    }

    Length horizontalBorderSpacing()
    {
        return m_inheritedStyles.m_horizontalBorderSpacing;
    }

    Length verticalBorderSpacing()
    {
        return m_inheritedStyles.m_verticalBorderSpacing;
    }

    BorderCollapseValue borderCollapse()
    {
        return m_inheritedStyles.m_borderCollapse;
    }

    CaptionSideValue captionSide()
    {
        return m_inheritedStyles.m_captionSide;
    }

    EmptyCellsValue emptyCells()
    {
        return m_inheritedStyles.m_emptyCells;
    }

    void setTableLayout(TableLayoutValue value)
    {
        m_tableLayout = value;
    }

    TableLayoutValue tableLayout()
    {
        return m_tableLayout;
    }

    void setFlexDirection(FlexDirectionValue value)
    {
        m_flexDirection = value;
    }

    FlexDirectionValue flexDirection()
    {
        return m_flexDirection;
    }

    void setFlexWrap(FlexWrapValue value)
    {
        m_flexWrap = value;
    }

    FlexWrapValue flexWrap()
    {
        return m_flexWrap;
    }

    void setOrder(int32_t order)
    {
        m_order = order;
    }

    int32_t order()
    {
        return m_order;
    }

    void setJustifyContent(JustifyContentValue value)
    {
        m_justifyContent = value;
    }

    JustifyContentValue justifyContent()
    {
        return m_justifyContent;
    }

    void setAlignItems(AlignItemValue value)
    {
        m_alignItems = value;
    }

    AlignItemValue alignItems()
    {
        return m_alignItems;
    }

    void setAlignSelf(AlignItemValue value)
    {
        m_alignSelf = value;
    }

    AlignItemValue alignSelf()
    {
        return m_alignSelf;
    }

    void setAlignContent(AlignContentValue value)
    {
        m_alignContent = value;
    }

    AlignContentValue alignContent()
    {
        return m_alignContent;
    }

    void setFlexGrow(float value)
    {
        m_flexGrow = value;
    }

    float flexGrow()
    {
        return m_flexGrow;
    }

    void setFlexShrink(float value)
    {
        m_flexShrink = value;
    }

    float flexShrink()
    {
        return m_flexShrink;
    }

    void setFlexBasis(FlexBasisData value)
    {
        m_flexBasis = value;
    }

    FlexBasisData flexBasis()
    {
        return m_flexBasis;
    }

    bool hasContent()
    {
        if (hasRareComputeStyleData() &&
            m_rareComputedStyleData->m_content.size()) {
            return true;
        }
        return false;
    }

    ContentDataGroup& content()
    {
        STARFISH_ASSERT(hasRareComputeStyleData());
        return m_rareComputedStyleData->m_content;
    }

    void clearContent()
    {
        if (hasRareComputeStyleData()) {
            m_rareComputedStyleData->m_content.clear();
        }
    }

    void setContentText(String* text)
    {
        ContentData content(ContentData::ContentType::Text);
        content.setText(text);
        setRareComputedStyleDataIfNeeded();
        m_rareComputedStyleData->m_content.push_back(content);
    }

    void setContentImage(String* image)
    {
        ContentData content(ContentData::ContentType::Image);
        content.setImage(image);
        setRareComputedStyleDataIfNeeded();
        m_rareComputedStyleData->m_content.push_back(content);
    }

    void setPseudoType(StyleResolver::PseudoElementType id)
    {
        m_pseudoId = id;
    }

    StyleResolver::PseudoElementType pseudoType()
    {
        return m_pseudoId;
    }

    void setCombinatorMatchingResult(
        StyleResolver::CombinatorMatchingResult result)
    {
        m_combinatorMatchingResult = result;
    }

    StyleResolver::CombinatorMatchingResult combinatorMatchingResult() const
    {
        return m_combinatorMatchingResult;
    }

    GCVector<ComputedStyle*>& cachedPseudoStyles()
    {
        setRareComputedStyleDataIfNeeded();
        return m_rareComputedStyleData->m_cachedPseudoStyles;
    }
    ComputedStyle* cachedPseudoStyle(StyleResolver::PseudoElementType pid);
    ComputedStyle* addCachedPseudoStyle(ComputedStyle* pseudoStyle);
    void removeCachedPseudoStyle(StyleResolver::PseudoElementType pid);

    bool hasRareComputeStyleData()
    {
        return m_rareComputedStyleData != nullptr;
    }

    RareComputedStyleData* rareComputedStyleData()
    {
        return m_rareComputedStyleData;
    }

    void setRareComputedStyleDataIfNeeded()
    {
        if (!m_rareComputedStyleData) {
            m_rareComputedStyleData = new RareComputedStyleData();
        }
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    void initNonInheritedStyles()
    {
        m_display = DisplayValue::InlineDisplayValue;
        m_float = FloatValue::NoneFloatValue;
        m_clear = ClearValue::NoneClearValue;
        m_opacity = 1;
        m_zIndex = 0;
        m_zIndexSpecifiedByUser = false;
        m_background = nullptr;
        m_surround = nullptr;
        m_order = 0;
        m_flexGrow = 0;
        m_flexShrink = 1;
        m_flexBasis = FlexBasisData(true);
        m_overflowX = OverflowValue::VisibleOverflow;
        m_overflowY = OverflowValue::VisibleOverflow;
        m_textDecoration = TextDecorationValue::NoneTextDecorationValue;
        m_verticalAlign = initialVerticalAlign();
        m_unicodeBidi = UnicodeBidiValue::NormalUnicodeBidiValue;
        m_boxSizing = BoxSizingValue::ContentBoxBoxSizingValue;
        m_tableLayout = TableLayoutValue::AutoTableLayoutValue;
        m_flexDirection = FlexDirectionValue::RowFlexDirectionValue;
        m_flexWrap = FlexWrapValue::NoWrapFlexWrapValue;
        m_justifyContent = JustifyContentValue::FlexStartJustifyContentValue;
        m_alignItems = AlignItemValue::StretchAlignItemValue;
        m_alignSelf = AlignItemValue::StretchAlignItemValue;
        m_alignSelfSpecifiedByUser = false;
        m_alignContent = AlignContentValue::StretchAlignContentValue;
        m_pseudoId = StyleResolver::PseudoElementType::PseudoElementNone;
        m_combinatorMatchingResult =
            StyleResolver::CombinatorMatchingResult::CombinatorFails;
    }

    // NOTICE
    // if you add new property, you MUST implement comparing style for new
    // property in [compareStyle function]

    struct InheritedStyles {
        FontStyleValue m_fontStyle : 2;
        FontWeightValue m_fontWeight : 4;
        TextAlignValue m_textAlign : 3;
        DirectionValue m_direction : 1;
        WhiteSpaceValue m_whiteSpace : 3;
        VisibilityValue m_visibility : 1;
        BorderCollapseValue m_borderCollapse : 1; // table
        CaptionSideValue m_captionSide : 1;       // table
        EmptyCellsValue m_emptyCells : 1;         // table

        Unit::Color m_color;
        Length m_fontSize;
        Length m_letterSpacing;
        Length m_lineHeight;
        Length m_textIndent;
        Length m_horizontalBorderSpacing; // table
        Length m_verticalBorderSpacing;   // table
    } m_inheritedStyles;

    FloatValue m_float : 2;
    ClearValue m_clear : 2;
    DisplayValue m_display : 4;
    DisplayValue m_originalDisplay : 4;
    PositionValue m_position : 2;
    VerticalAlignValue m_verticalAlign : 4;
    OverflowValue m_overflowX : 2;
    OverflowValue m_overflowY : 2;
    TextDecorationValue m_textDecoration : 3;
    UnicodeBidiValue m_unicodeBidi : 2;
    BoxSizingValue m_boxSizing : 1;
    TableLayoutValue m_tableLayout : 1; // table
    FlexDirectionValue m_flexDirection : 2;
    FlexWrapValue m_flexWrap : 2;
    JustifyContentValue m_justifyContent : 3;
    AlignItemValue m_alignItems : 3;
    bool m_alignSelfSpecifiedByUser;
    AlignItemValue m_alignSelf : 3;
    AlignContentValue m_alignContent : 3;
    StyleResolver::PseudoElementType m_pseudoId : 6;
    StyleResolver::CombinatorMatchingResult m_combinatorMatchingResult : 1;

    Length m_width;
    Length m_height;
    Length m_verticalAlignLength;

    float m_opacity;
    int32_t m_zIndex;
    bool m_zIndexSpecifiedByUser;
    Font* m_font;
    StyleBackgroundData* m_background;
    StyleSurroundData* m_surround;
    int32_t m_order;
    float m_flexGrow;
    float m_flexShrink;
    FlexBasisData m_flexBasis;

    RareComputedStyleData* m_rareComputedStyleData;
};

ComputedStyleDamage compareStyle(ComputedStyle* oldStyle,
                                 ComputedStyle* newStyle);

void applyTransition(Element* element, ComputedStyle* oldStyle,
                     ComputedStyle* newStyle);
}

#endif
