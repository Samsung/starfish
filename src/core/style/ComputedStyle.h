/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#ifndef __StarFishComputedStyle__
#define __StarFishComputedStyle__

#include "core/style/BorderRadiusData.h"
#include "core/style/BorderData.h"
#include "core/style/LengthData.h"
#include "core/style/ContentData.h"
#include "core/style/CounterBaseList.h"
#include "core/style/FlexBasisData.h"
#include "core/style/DefaultStyle.h"
#include "core/style/Style.h"
#include "core/style/StyleBackgroundData.h"
#include "core/style/StyleTransformData.h"
#include "core/style/StyleTransformOrigin.h"
#include "core/style/StyleTransitionData.h"
#include "core/style/StylePaintData.h"
#include "core/style/ListStyleData.h"
#include "core/style/ShadowData.h"
#include "core/style/PositionedMaskData.h"
#include "core/style/OutlineData.h"
#include "core/style/ObjectSizingData.h"
#include "core/animation/Animation.h"

namespace StarFish {

class Frame;

enum ComputedStyleDamage {
    ComputedStyleDamageNone = 0,
    ComputedStyleDamageInherited = 1,
    ComputedStyleDamageRebuildFrame = 1 << 1,
    ComputedStyleDamageLayout = 1 << 2,
    ComputedStyleDamageComputeStackingContextProperties = 1 << 3,
    ComputedStyleDamagePainting = 1 << 4,
    ComputedStyleDamageComposite = 1 << 5,
};

union FontFamilyData {
    size_t m_length;
    String* m_familyName;

    FontFamilyData(size_t len)
        : m_length(len)
    {
    }

    FontFamilyData(String* familyName)
        : m_familyName(familyName)
    {
    }
};

class RareComputedStyleData : public gc {
    enum KeyKind {
        Order,
        ZIndex,
        FlexGrow,
        FlexShrink,
        Opacity,
        Border,
        BoxShadow,
        Padding,
        Margin,
        Offset,
        MinWidth,
        MaxWidth,
        MinHeight,
        MaxHeight,
        FlexBasis,
        VerticalAlignLength,
        Transforms,
        TransformOrigin,
        Transition,
        TextDecorationColor,
        TextDecorationStyle,
        TextUnderlinePosition,
        Content,
        Outline,
        BorderRadius,
        PositionedMask,
        CachedPsuedoStyles,
        Background,
        ObjectSizing,
        X,
        Y,
        CX,
        CY,
        RX,
        RY,
        R,
        D,
        Clip,
        UserSelect,
        CaretColor,
        Hyphens,
        LineBreak,
        WordBreak,
        TextOverflow,
        CounterReset,
        CounterIncrement,

        // Grid
        GridTemplateColumns,
        GridTemplateRows,
    };

    union RareComputedStyleValue {
        int32_t m_int32Value;
        float m_floatValue;
        String* m_stringValue;
        Length m_length;
        BorderData* m_borderData;
        LengthData* m_lengthData;
        FlexBasisData* m_flexBasis;
        StyleTransformDataGroup* m_transforms;
        StyleTransformOrigin* m_transformOrigin;
        StyleTransitionData* m_transition;
        ContentDataGroup* m_content;
        OutlineData* m_outline;
        BorderRadiusData* m_borderRadius;
        PositionedMaskData* m_positionedMask;
        GCVector<ComputedStyle*>* m_pseudoStyles;
        StyleBackgroundData* m_background;
        ObjectSizingData* m_objectSizing;
        ShadowDataList* m_boxShadowDataList;
        RectData* m_clip;
        UserSelectValue m_userSelect;
        GCVector<GridLength>* m_gridTemplateUnits;
        Unit::Color m_color;
        HyphensValue m_hyphens;
        LineBreakValue m_lineBreak;
        WordBreakValue m_wordBreak;
        TextDecorationStyleValue m_textDecorationStyle;
        TextUnderlinePositionValue m_textUnderlinePosition;
        TextOverflowData* m_textOverflow;
        CounterBaseList* m_counterBaseList;

        RareComputedStyleValue(int32_t int32Value)
            : m_int32Value(int32Value)
        {
        }

        RareComputedStyleValue(float floatValue)
            : m_floatValue(floatValue)
        {
        }

        RareComputedStyleValue(String* stringValue)
            : m_stringValue(stringValue)
        {
        }

        RareComputedStyleValue(Length length)
            : m_length(length)
        {
        }

        RareComputedStyleValue(BorderData* borderData)
            : m_borderData(borderData)
        {
        }

        RareComputedStyleValue(LengthData* lengthData)
            : m_lengthData(lengthData)
        {
        }

        RareComputedStyleValue(FlexBasisData* flexBasis)
            : m_flexBasis(flexBasis)
        {
        }

        RareComputedStyleValue(StyleTransformDataGroup* transforms)
            : m_transforms(transforms)
        {
        }

        RareComputedStyleValue(StyleTransformOrigin* origin)
            : m_transformOrigin(origin)
        {
        }

        RareComputedStyleValue(StyleTransitionData* transition)
            : m_transition(transition)
        {
        }

        RareComputedStyleValue(ContentDataGroup* content)
            : m_content(content)
        {
        }

        RareComputedStyleValue(OutlineData* outline)
            : m_outline(outline)
        {
        }

        RareComputedStyleValue(BorderRadiusData* borderRadius)
            : m_borderRadius(borderRadius)
        {
        }

        RareComputedStyleValue(PositionedMaskData* positionedMask)
            : m_positionedMask(positionedMask)
        {
        }

        RareComputedStyleValue(GCVector<ComputedStyle*>* pseudoStyles)
            : m_pseudoStyles(pseudoStyles)
        {
        }

        RareComputedStyleValue(StyleBackgroundData* background)
            : m_background(background)
        {
        }

        RareComputedStyleValue(ObjectSizingData* objSizing)
            : m_objectSizing(objSizing)
        {
        }

        RareComputedStyleValue(ShadowDataList* boxShadow)
            : m_boxShadowDataList(boxShadow)
        {
        }

        RareComputedStyleValue(RectData* rect)
            : m_clip(rect)
        {
        }

        RareComputedStyleValue(UserSelectValue v)
            : m_userSelect(v)
        {
        }

        RareComputedStyleValue(TextDecorationStyleValue v)
            : m_textDecorationStyle(v)
        {
        }

        RareComputedStyleValue(TextUnderlinePositionValue v)
            : m_textUnderlinePosition(v)
        {
        }

        RareComputedStyleValue(GCVector<GridLength>* gridTemplate)
            : m_gridTemplateUnits(gridTemplate)
        {
        }

        RareComputedStyleValue(Unit::Color c)
            : m_color(c)
        {
        }

        RareComputedStyleValue(HyphensValue v)
            : m_hyphens(v)
        {
        }

        RareComputedStyleValue(LineBreakValue v)
            : m_lineBreak(v)
        {
        }

        RareComputedStyleValue(WordBreakValue v)
            : m_wordBreak(v)
        {
        }

        RareComputedStyleValue(TextOverflowData* v)
            : m_textOverflow(v)
        {
        }

        RareComputedStyleValue(CounterBaseList* v)
            : m_counterBaseList(v)
        {
        }
    };

    struct RareComputedStyleValuePair {
        KeyKind m_keyKind;
        RareComputedStyleValue m_value;

        RareComputedStyleValuePair(KeyKind keyKind,
                                   RareComputedStyleValue value)
            : m_keyKind(keyKind)
            , m_value(value)
        {
        }

        KeyKind keyKind()
        {
            return m_keyKind;
        }
    };

public:
    RareComputedStyleData()
    {
    }

#define FIND_VALUE(Name)                                               \
    auto it = std::find_if(m_styles.begin(), m_styles.end(),           \
                           [](RareComputedStyleValuePair pair) {       \
                               return pair.keyKind() == KeyKind::Name; \
                           });

#define CLEARER(Name)               \
    void clear##Name()              \
    {                               \
        FIND_VALUE(Name);           \
                                    \
        if (it == m_styles.end()) { \
            return;                 \
        }                           \
        m_styles.erase(it);         \
    }

#define GETTER_VALUE(RETURN_TYPE, VALUE_NAME, name, Name)           \
    RETURN_TYPE* ensure##Name()                                     \
    {                                                               \
        FIND_VALUE(Name);                                           \
                                                                    \
        if (it == m_styles.end()) {                                 \
            RETURN_TYPE name;                                       \
            m_styles.emplace_back(KeyKind::Name, name);             \
            return &m_styles.back().m_value.m_##VALUE_NAME;         \
        }                                                           \
                                                                    \
        return &(*it).m_value.m_##VALUE_NAME;                       \
    }                                                               \
                                                                    \
    Nullable<RETURN_TYPE> name()                                    \
    {                                                               \
        FIND_VALUE(Name);                                           \
                                                                    \
        if (it == m_styles.end()) {                                 \
            return Nullable<RETURN_TYPE>();                         \
        }                                                           \
                                                                    \
        return Nullable<RETURN_TYPE>((*it).m_value.m_##VALUE_NAME); \
    }

    GETTER_VALUE(int32_t, int32Value, order, Order);
    GETTER_VALUE(int32_t, int32Value, zIndex, ZIndex);
    GETTER_VALUE(float, floatValue, flexGrow, FlexGrow);
    GETTER_VALUE(float, floatValue, flexShrink, FlexShrink);
    GETTER_VALUE(float, floatValue, opacity, Opacity);
    GETTER_VALUE(String*, stringValue, d, D);

    LengthData* ensureOffset()
    {
        FIND_VALUE(Offset);

        if (it == m_styles.end()) {
            LengthData* offset = new LengthData(Length());
            m_styles.emplace_back(KeyKind::Offset, offset);
            return m_styles.back().m_value.m_lengthData;
        }

        return (*it).m_value.m_lengthData;
    }

    LengthData* offset()
    {
        FIND_VALUE(Offset);

        if (it == m_styles.end()) {
            return nullptr;
        }

        return (*it).m_value.m_lengthData;
    }

    GETTER_VALUE(Length, length, minWidth, MinWidth);
    GETTER_VALUE(Length, length, maxWidth, MaxWidth);
    GETTER_VALUE(Length, length, minHeight, MinHeight);
    GETTER_VALUE(Length, length, maxHeight, MaxHeight);
    GETTER_VALUE(Length, length, verticalAlignLength, VerticalAlignLength);
    GETTER_VALUE(Length, length, x, X);
    GETTER_VALUE(Length, length, y, Y);
    GETTER_VALUE(Length, length, r, R);
    GETTER_VALUE(Length, length, cx, CX);
    GETTER_VALUE(Length, length, cy, CY);
    GETTER_VALUE(Length, length, rx, RX);
    GETTER_VALUE(Length, length, ry, RY);
    GETTER_VALUE(UserSelectValue, userSelect, userSelect, UserSelect);
    GETTER_VALUE(LineBreakValue, lineBreak, lineBreak, LineBreak);
    GETTER_VALUE(Unit::Color, color, textDecorationColor, TextDecorationColor);
    GETTER_VALUE(TextDecorationStyleValue, textDecorationStyle,
                 textDecorationStyle, TextDecorationStyle);

#undef GETTER_VALUE

#define GETTER_PTR(RETURN_TYPE, VALUE_NAME, name, Name) \
    RETURN_TYPE* ensure##Name()                         \
    {                                                   \
        FIND_VALUE(Name);                               \
                                                        \
        if (it == m_styles.end()) {                     \
            RETURN_TYPE* name = new RETURN_TYPE();      \
            m_styles.emplace_back(KeyKind::Name, name); \
            return name;                                \
        }                                               \
                                                        \
        return (*it).m_value.m_##VALUE_NAME;            \
    }                                                   \
                                                        \
    RETURN_TYPE* name()                                 \
    {                                                   \
        FIND_VALUE(Name);                               \
                                                        \
        if (it == m_styles.end()) {                     \
            return nullptr;                             \
        }                                               \
                                                        \
        return (*it).m_value.m_##VALUE_NAME;            \
    }

    GETTER_PTR(FlexBasisData, flexBasis, flexBasis, FlexBasis);
    GETTER_PTR(LengthData, lengthData, margin, Margin);
    GETTER_PTR(LengthData, lengthData, padding, Padding);
    GETTER_PTR(BorderData, borderData, border, Border);
    GETTER_PTR(StyleTransformDataGroup, transforms, transforms, Transforms);
    GETTER_PTR(StyleTransformOrigin, transformOrigin, transformOrigin,
               TransformOrigin);
    GETTER_PTR(StyleTransitionData, transition, transition, Transition);
    GETTER_PTR(ContentDataGroup, content, content, Content);
    GETTER_PTR(OutlineData, outline, outline, Outline);
    GETTER_PTR(BorderRadiusData, borderRadius, borderRadius, BorderRadius);
    GETTER_PTR(PositionedMaskData, positionedMask, positionedMask,
               PositionedMask);
    GETTER_PTR(GCVector<ComputedStyle*>, pseudoStyles, cachedPsuedoStyles,
               CachedPsuedoStyles);
    GETTER_PTR(StyleBackgroundData, background, background, Background);
    GETTER_PTR(ObjectSizingData, objectSizing, objectSizing, ObjectSizing);
    GETTER_PTR(ShadowDataList, boxShadowDataList, boxShadow, BoxShadow);
    GETTER_PTR(RectData, clip, clip, Clip);
    GETTER_PTR(GCVector<GridLength>, gridTemplateUnits, gridTemplateColumns,
               GridTemplateColumns);
    GETTER_PTR(GCVector<GridLength>, gridTemplateUnits, gridTemplateRows,
               GridTemplateRows);
    GETTER_PTR(TextOverflowData, textOverflow, textOverflow, TextOverflow);
    GETTER_PTR(CounterBaseList, counterBaseList, counterReset, CounterReset);
    GETTER_PTR(CounterBaseList, counterBaseList, counterIncrement,
               CounterIncrement);

    void setCounterReset(CounterBaseList* v)
    {
        clearCounterReset();
        if (v) {
            m_styles.emplace_back(KeyKind::CounterReset, v);
        }
    }

    void setCounterIncrement(CounterBaseList* v)
    {
        clearCounterIncrement();
        if (v) {
            m_styles.emplace_back(KeyKind::CounterIncrement, v);
        }
    }

#undef GETTER_PTR

    CLEARER(Transforms);
    CLEARER(Content);
    CLEARER(CounterReset);
    CLEARER(CounterIncrement);

#undef FIND_VALUE
#undef CLEARER

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    GCVector<RareComputedStyleValuePair> m_styles;
};

class ComputedStyle : public gc {
    friend class StyleResolver;
    friend class Frame;
    friend class Length;
    friend void resolveDOMStyleInner(StyleResolver* resolver, Element* element,
                                     ComputedStyle* parentStyle, bool force);
    friend ComputedStyleDamage compareStyle(ComputedStyle* oldStyle,
                                            ComputedStyle* newStyle,
                                            bool* damagedKeys);

    struct InheritedStylesRareData {
        FillRuleValue m_fillRule : 1; // svg
        TextTransformValue m_textTransform : 2;
        HyphensValue m_hyphens : 1;
        FontKerningValue m_fontKerning : 2;
        ImageRenderingValue m_imageRendering : 2;

        Length m_letterSpacing;
        Length m_textIndent;
        Length m_wordSpacing;
        Length m_horizontalBorderSpacing; // table
        Length m_verticalBorderSpacing;   // table

        StylePaintData m_fill;   // svg
        float m_fillOpacity;     // svg
        StylePaintData m_stroke; // svg
        Length m_strokeWidth;    // svg

        ShadowDataList m_textShadowDataList;
        ListStyleData m_listStyleData;

        Unit::Color m_caretColor;
        WordBreakValue m_wordBreak : 3;
        TextUnderlinePositionValue m_textUnderlinePosition : 3;

        InheritedStylesRareData()
        {
            m_letterSpacing = Length(Length::Fixed, 0);
            m_wordSpacing = Length(Length::Fixed, 0);
            m_textIndent = Length(Length::Fixed, 0);
            m_horizontalBorderSpacing = Length(Length::Fixed, 0);
            m_verticalBorderSpacing = Length(Length::Fixed, 0);

            m_fill = Unit::Color(0, 0, 0, 0xff);
            m_fillRule = FillRuleNonZero;
            m_fillOpacity = 1;
            m_stroke = Unit::Color(0, 0, 0, 0);
            m_strokeWidth = Length(Length::Fixed, 1);

            m_textTransform = NoneTextTransformValue;
            m_caretColor = Unit::Color(0, 0, 0, 255);
            m_hyphens = HyphensValue::NoneHyphensValue;
            m_fontKerning = FontKerningValue::FontKerningAutoValue;
            m_imageRendering = ImageRenderingValue::ImageRenderingAutoValue;
            m_wordBreak = WordBreakValue::NormalWordBreakValue;
            m_textUnderlinePosition =
                TextUnderlinePositionValue::AutoTextUnderlinePositionValue;
        }

        void* operator new(size_t size);
    };

    ComputedStyle(float mediumFontSize = DEFAULT_FONT_SIZE)
    {
        m_font = nullptr;

        m_inheritedStyles.m_color = Unit::Color(0, 0, 0, 255);
        m_inheritedStyles.m_fontSize = Length(Length::Fixed, mediumFontSize);
        m_inheritedStyles.m_fontWeight = FontWeightValue::NormalFontWeightValue;
        m_inheritedStyles.m_wordWrap = WordWrapValue::NormalWordWrapValue;
        m_inheritedStyles.m_textAlign = TextAlignValue::StartTextAlignValue;
        m_inheritedStyles.m_direction = DirectionValue::LtrDirectionValue;
        m_inheritedStyles.m_whiteSpace = WhiteSpaceValue::NormalWhiteSpaceValue;
        m_inheritedStyles.m_visibility =
            VisibilityValue::VisibleVisibilityValue;
        m_inheritedStyles.m_borderCollapse =
            BorderCollapseValue::SeparateBorderCollapseValue;
        m_inheritedStyles.m_captionSide = CaptionSideValue::TopCaptionSideValue;
        m_inheritedStyles.m_emptyCells = EmptyCellsValue::ShowEmptyCellsValue;
        m_inheritedStyles.m_rareData = nullptr;
        m_inheritedStyles.m_isRareDataAllocated = false;
        m_inheritedStyles.m_fontFamilyDatas = nullptr;
        // -100 is used to represent 'normal' value.
        m_inheritedStyles.m_lineHeight = Length(Length::Percent, -100);
        m_seenViewPortUnitInStyle = false;
        m_seenPseudoElementFirstLine = false;
        m_seenPseudoElementFirstLetter = false;
        m_seenPseudoElementBefore = false;
        m_seenPseudoElementAfter = false;
        m_gotInheritedColor = false;
        m_usedInAnimator = false;

        initNonInheritedStyles();
    }

public:
    ComputedStyle(ComputedStyle* from)
    {
        inheritStylesFrom(from);
    }

    void inheritStylesFrom(ComputedStyle* from)
    {
        m_font = nullptr;

        m_inheritedStyles = from->m_inheritedStyles;
        m_inheritedStyles.m_isRareDataAllocated = false;
        m_seenViewPortUnitInStyle = false;
        m_seenPseudoElementFirstLine = false;
        m_seenPseudoElementFirstLetter = false;
        m_seenPseudoElementBefore = false;
        m_seenPseudoElementAfter = false;
        m_gotInheritedColor = false;

        initNonInheritedStyles();
    }

    bool seenViewPortUnitInStyle()
    {
        return m_seenViewPortUnitInStyle;
    }

    bool usedInAnimator()
    {
        return m_usedInAnimator;
    }

    void markUsedInAnimator()
    {
        m_usedInAnimator = true;
    }

    DisplayValue originalDisplay()
    {
        return m_originalDisplay;
    }

    DisplayValue display()
    {
        return m_display;
    }

    bool hasBlockLikeDisplay()
    {
        switch (display()) {
        case BlockDisplayValue:
        case ListItemDisplayValue:
        case InlineListItemDisplayValue:
        case InlineBlockDisplayValue:
        case TableDisplayValue:
        case InlineTableDisplayValue:
        case TableCellDisplayValue:
        case FlexDisplayValue:
        case InlineFlexDisplayValue:
        case GridDisplayValue:
        case InlineGridDisplayValue:
            return true;
        default:
            return false;
        }
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

    void setClip(RectData* r)
    {
        *m_rareComputedStyleData.ensureClip() = *r;
    }

    void setGridTemplateColumns(GCVector<GridLength>* gridTemplate)
    {
        *m_rareComputedStyleData.ensureGridTemplateColumns() = *gridTemplate;
    }

    void setGridTemplateRows(GCVector<GridLength>* gridTemplate)
    {
        *m_rareComputedStyleData.ensureGridTemplateRows() = *gridTemplate;
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

        Nullable<Length> ret = m_rareComputedStyleData.maxWidth();
        if (ret.hasValue()) {
            return ret.getValue();
        }

        return Length();
    }

    Length minWidth()
    {
        if (!hasRareComputeStyleData()) {
            return Length();
        }

        Nullable<Length> ret = m_rareComputedStyleData.minWidth();
        if (ret.hasValue()) {
            return ret.getValue();
        }

        return Length();
    }

    void setWidth(const Length& l)
    {
        m_width = l;
    }

    void setMaxWidth(const Length& l)
    {
        *m_rareComputedStyleData.ensureMaxWidth() = l;
    }

    void setMinWidth(const Length& l)
    {
        *m_rareComputedStyleData.ensureMinWidth() = l;
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

        Nullable<Length> ret = m_rareComputedStyleData.maxHeight();
        if (ret.hasValue()) {
            return ret.getValue();
        }

        return Length();
    }

    Length minHeight()
    {
        if (!hasRareComputeStyleData()) {
            return Length();
        }

        Nullable<Length> ret = m_rareComputedStyleData.minHeight();
        if (ret.hasValue()) {
            return ret.getValue();
        }

        return Length();
    }

    void setHeight(const Length& l)
    {
        m_height = l;
    }

    void setMaxHeight(const Length& l)
    {
        *m_rareComputedStyleData.ensureMaxHeight() = l;
    }

    void setMinHeight(const Length& l)
    {
        *m_rareComputedStyleData.ensureMinHeight() = l;
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
        if (!m_rareComputedStyleData.m_styles.size()) {
            return Length();
        }

        Nullable<Length> verticalAlign =
            m_rareComputedStyleData.verticalAlignLength();
        if (verticalAlign.hasValue()) {
            return verticalAlign.getValue();
        }

        return Length();
    }

    void setVerticalAlignLength(Length l)
    {
        setVerticalAlign(VerticalAlignValue::NumericVAlignValue);
        *m_rareComputedStyleData.ensureVerticalAlignLength() = l;
    }

    bool isNumericVerticalAlign()
    {
        return (verticalAlign() == VerticalAlignValue::NumericVAlignValue);
    }

    TextAlignValue orignalTextAlign()
    {
        return m_inheritedStyles.m_textAlign;
    }

    TextAlignValue textAlign()
    {
        if (m_inheritedStyles.m_textAlign ==
            TextAlignValue::StarFishCenterTextAlignValue) {
            return TextAlignValue::CenterTextAlignValue;
        }
        return m_inheritedStyles.m_textAlign;
    }

    void setTextAlign(TextAlignValue t)
    {
        m_inheritedStyles.m_textAlign = t;
    }

    Length textIndent()
    {
        if (m_inheritedStyles.m_rareData) {
            return m_inheritedStyles.m_rareData->m_textIndent;
        }
        return InheritedStylesRareData().m_textIndent;
    }

    void setTextIndent(Length val)
    {
        if (!val.isFixed() || val != textIndent()) {
            ensureInheritedRareData()->m_textIndent = val;
        }
    }

    TextTransformValue textTransform()
    {
        if (m_inheritedStyles.m_rareData) {
            return m_inheritedStyles.m_rareData->m_textTransform;
        }
        return InheritedStylesRareData().m_textTransform;
    }

    void setTextTransform(TextTransformValue val)
    {
        if (val != textTransform()) {
            ensureInheritedRareData()->m_textTransform = val;
        }
    }

    void addTextShadow(ShadowData& shadow)
    {
        ensureInheritedRareData()->m_textShadowDataList.push_back(shadow);
    }

    void setTextShadow(ShadowDataList val)
    {
        if (val != textShadow()) {
            ensureInheritedRareData()->m_textShadowDataList = val;
        }
    }

    ShadowDataList textShadow()
    {
        if (m_inheritedStyles.m_rareData) {
            return m_inheritedStyles.m_rareData->m_textShadowDataList;
        }
        return InheritedStylesRareData().m_textShadowDataList;
    }

    void addBoxShadow(ShadowData& shadow)
    {
        m_rareComputedStyleData.ensureBoxShadow()->push_back(shadow);
    }

    void setBoxShadow(ShadowDataList val)
    {
        if (val != boxShadow()) {
            (*m_rareComputedStyleData.ensureBoxShadow()) = val;
        }
    }

    ShadowDataList boxShadow()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return ShadowDataList();
        }
        ShadowDataList* shadowDataList = m_rareComputedStyleData.boxShadow();
        if (shadowDataList) {
            return *shadowDataList;
        }

        return ShadowDataList();
    }

    TextDecorationLineValue textDecoration()
    {
        // TODO: shorthand not supported yet
        return textDecorationLine();
    }

    void setTextDecoration(TextDecorationLineValue decoration)
    {
        // TODO: shorthand not supported yet
        setTextDecorationLine(decoration);
    }

    TextDecorationLineValue textDecorationLine()
    {
        return m_textDecorationLine;
    }

    void setTextDecorationLine(TextDecorationLineValue decoration)
    {
        m_textDecorationLine = decoration;
    }

    Unit::Color textDecorationColor()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return Unit::Color(0, 0, 0, 255);
        }

        Nullable<Unit::Color> c =
            rareComputedStyleData()->textDecorationColor();
        if (c.hasValue()) {
            return c.getValue();
        }

        return Unit::Color(0, 0, 0, 255);
    }

    void setTextDecorationColor(Unit::Color c)
    {
        *m_rareComputedStyleData.ensureTextDecorationColor() = c;
    }

    TextDecorationStyleValue textDecorationStyle()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return TextDecorationStyleValue::SolidTextDecorationStyleValue;
        }

        Nullable<TextDecorationStyleValue> c =
            rareComputedStyleData()->textDecorationStyle();
        if (c.hasValue()) {
            return c.getValue();
        }

        return TextDecorationStyleValue::SolidTextDecorationStyleValue;
    }

    void setTextDecorationStyle(TextDecorationStyleValue v)
    {
        *m_rareComputedStyleData.ensureTextDecorationStyle() = v;
    }

    TextUnderlinePositionValue textUnderlinePosition()
    {
        if (m_inheritedStyles.m_rareData) {
            return m_inheritedStyles.m_rareData->m_textUnderlinePosition;
        }
        return InheritedStylesRareData().m_textUnderlinePosition;
    }

    void setTextUnderlinePosition(TextUnderlinePositionValue v)
    {
        ensureInheritedRareData()->m_textUnderlinePosition = v;
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
        if (!length.isFixed() || length != lineHeight()) {
            m_inheritedStyles.m_lineHeight = length;
        }
    }

    Length wordSpacing()
    {
        if (m_inheritedStyles.m_rareData) {
            return m_inheritedStyles.m_rareData->m_wordSpacing;
        }
        return Length(Length::Fixed, 0);
    }

    void setWordSpacing(Length val)
    {
        if (!val.isFixed() || val != wordSpacing()) {
            ensureInheritedRareData()->m_wordSpacing = val;
        }
    }

    bool hasTransforms()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return false;
        }

        return m_rareComputedStyleData.transforms() != nullptr &&
               m_rareComputedStyleData.transforms()->size();
    }

    bool hasTransforms(Frame* frame);
    bool hasComplexTransforms(Frame* frame);
    bool has3DTransforms(Frame* frame);

    StyleTransformDataGroup* transforms(Frame* frame = nullptr);

    SkMatrix transformsToMatrix(LayoutUnit containerWidth,
                                LayoutUnit containerHeight, Frame* f,
                                bool isTransformable);

    void clearTransform()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return;
        }

        m_rareComputedStyleData.clearTransforms();
    }

    void setTransform(StyleTransformDataGroup* transform)
    {
        STARFISH_ASSERT(transform);
        m_rareComputedStyleData.ensureTransforms()->reset(transform);
    }

    void setTransformMatrix(double a, double b, double c, double d, double e,
                            double f)
    {
        StyleTransformData t(StyleTransformData::OperationType::Matrix);
        t.setMatrix(a, b, c, d, e, f);
        m_rareComputedStyleData.ensureTransforms()->append(t);
    }

    void setTransformScale(double a, double b)
    {
        StyleTransformData t(StyleTransformData::OperationType::Scale);
        t.setScale(a, b);
        m_rareComputedStyleData.ensureTransforms()->append(t);
    }

    void setTransformRotate(double a)
    {
        StyleTransformData t(StyleTransformData::OperationType::Rotate);
        t.setRotate(a);
        m_rareComputedStyleData.ensureTransforms()->append(t);
    }

    void setTransformSkew(double a, double b)
    {
        StyleTransformData t(StyleTransformData::OperationType::Skew);
        t.setSkew(a, b);
        m_rareComputedStyleData.ensureTransforms()->append(t);
    }

    void setTransformTranslate(Length a, Length b)
    {
        StyleTransformData t(StyleTransformData::OperationType::Translate);
        t.setTranslate(a, b);
        m_rareComputedStyleData.ensureTransforms()->append(t);
    }

    void setTransformOrigin(StyleTransformOrigin* transformOrigin)
    {
        m_rareComputedStyleData.ensureTransformOrigin()->setOrigin(
            transformOrigin);
    }

    void setTransformOriginValue(Length x, Length y, Length z)
    {
        m_rareComputedStyleData.ensureTransformOrigin()->setOriginValue(x, y,
                                                                        z);
    }

    void setBackgroundColor(Unit::Color color)
    {
        m_rareComputedStyleData.ensureBackground()->setBgColor(color);
    }

    void setBackgroundColorToCurrentColor()
    {
        m_rareComputedStyleData.ensureBackground()->setBgColorToCurrentColor();
    }

    void setBackgroundImage(String* img, unsigned int layer = 0)
    {
        m_rareComputedStyleData.ensureBackground()->setBgImage(img, layer);
    }

    void setBackgroundImageResource(ImageResource* img, unsigned int layer = 0)
    {
        m_rareComputedStyleData.ensureBackground()->setBgImageResource(img,
                                                                       layer);
    }

    void setBackgroundRepeatX(BackgroundRepeatValue repeat,
                              unsigned int layer = 0)
    {
        m_rareComputedStyleData.ensureBackground()->setRepeatX(repeat, layer);
    }

    void setBackgroundRepeatY(BackgroundRepeatValue repeat,
                              unsigned int layer = 0)
    {
        m_rareComputedStyleData.ensureBackground()->setRepeatY(repeat, layer);
    }

    void setBackgroundPositionX(Length value, unsigned int layer = 0)
    {
        m_rareComputedStyleData.ensureBackground()->setPositionX(value, layer);
    }

    void setBackgroundPositionY(Length value, unsigned int layer = 0)
    {
        m_rareComputedStyleData.ensureBackground()->setPositionY(value, layer);
    }

    void setBackgroundSize(BackgroundSizeValue size, unsigned int layer = 0)
    {
        m_rareComputedStyleData.ensureBackground()->setSize(size, layer);
    }

    void setBackgroundSize(LengthSize size, unsigned int layer = 0)
    {
        m_rareComputedStyleData.ensureBackground()->setSize(size, layer);
    }

    void setBackgroundAttachment(BackgroundAttachmentValue attachment,
                                 unsigned int layer = 0)
    {
        m_rareComputedStyleData.ensureBackground()->setAttachment(attachment,
                                                                  layer);
    }

    void setBackgroundClip(BoxValue clip, unsigned int layer = 0)
    {
        m_rareComputedStyleData.ensureBackground()->setClip(clip, layer);
    }

    void setBackgroundOrigin(BoxValue origin, unsigned int layer = 0)
    {
        m_rareComputedStyleData.ensureBackground()->setOrigin(origin, layer);
    }

    unsigned int backgroundLayerSize()
    {
        StyleBackgroundData* background = this->background();
        if (background == nullptr) {
            return 0;
        }
        return background->sizeOfLayers();
    }

    Unit::Color backgroundColor()
    {
        StyleBackgroundData* background = this->background();
        if (background == nullptr) {
            return Unit::Color();
        }
        return background->bgColor();
    }

    String* backgroundImage(unsigned int layer = 0)
    {
        StyleBackgroundData* background = this->background();
        if (background == nullptr) {
            return String::emptyString;
        }
        return background->bgImage(layer);
    }

    NativeImageData* backgroundImageData(unsigned int layer = 0)
    {
        StyleBackgroundData* background = this->background();
        if (background == nullptr) {
            return nullptr;
        }
        return background->bgImageData(layer);
    }

    BackgroundRepeatValue backgroundRepeatX(unsigned int layer = 0)
    {
        StyleBackgroundData* background = this->background();
        if (background == nullptr) {
            return BackgroundRepeatValue::RepeatRepeatValue;
        }
        return background->repeatX(layer);
    }

    BackgroundRepeatValue backgroundRepeatY(unsigned int layer = 0)
    {
        StyleBackgroundData* background = this->background();
        if (background == nullptr) {
            return BackgroundRepeatValue::RepeatRepeatValue;
        }
        return background->repeatY(layer);
    }

    Length backgroundPositionX(unsigned int layer = 0)
    {
        StyleBackgroundData* background = this->background();
        if (background == nullptr) {
            return Length(Length::Percent, 0.0f);
        }
        return background->positionX(layer);
    }

    Length backgroundPositionY(unsigned int layer = 0)
    {
        StyleBackgroundData* background = this->background();
        if (background == nullptr) {
            return Length(Length::Percent, 0.0f);
        }
        return background->positionY(layer);
    }

    bool backgroundSizeIsLength(unsigned int layer = 0)
    {
        StyleBackgroundData* background = this->background();
        if (background == nullptr) {
            return true;
        }
        return background->sizeIsLength(layer);
    }

    BackgroundSizeValue backgroundSizeTypeValue(unsigned int layer = 0)
    {
        StyleBackgroundData* background = this->background();
        if (background == nullptr) {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
            return BackgroundSizeValue::ContainBackgroundSizeValue;
        }
        return background->sizeTypeValue(layer);
    }

    LengthSize backgroundSizeLengthValue(unsigned int layer = 0)
    {
        StyleBackgroundData* background = this->background();
        if (background == nullptr) {
            return LengthSize();
        }
        return background->sizeLengthValue(layer);
    }

    BackgroundAttachmentValue backgroundAttachment(unsigned int layer = 0)
    {
        StyleBackgroundData* background = this->background();
        if (background == nullptr) {
            return BackgroundAttachmentValue::ScrollBackgroundAttachmentValue;
        }
        return background->attachment(layer);
    }

    BoxValue backgroundClip(unsigned int layer = 0)
    {
        StyleBackgroundData* background = this->background();
        if (background == nullptr) {
            return BoxValue::BorderBoxBoxValue;
        }
        return background->clip(layer);
    }

    BoxValue backgroundOrigin(unsigned int layer = 0)
    {
        StyleBackgroundData* background = this->background();
        if (background == nullptr) {
            return BoxValue::PaddingBoxBoxValue;
        }
        return background->origin(layer);
    }

    void setFont(Font* font)
    {
        m_font = font;
    }

    Font* font()
    {
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
        return lineHeight().isPercent() && lineHeight().percent() == -100;
    }

    float opacity()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return 1;
        }

        Nullable<float> opacity = m_rareComputedStyleData.opacity();
        if (opacity.hasValue()) {
            return opacity.getValue();
        }

        return 1;
    }

    void setOpacity(float opacity)
    {
        *m_rareComputedStyleData.ensureOpacity() = opacity;
    }

    int32_t zIndex()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return 0;
        }

        Nullable<int32_t> zIndex = m_rareComputedStyleData.zIndex();
        if (zIndex.hasValue()) {
            return zIndex.getValue();
        }

        return 0;
    }

    void setZIndex(int32_t zIndex)
    {
        *m_rareComputedStyleData.ensureZIndex() = zIndex;
    }

    bool isSpecifiedZIndex()
    {
        return m_zIndexSpecifiedByUser;
    }

    String* d()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return String::emptyString;
        }

        Nullable<String*> d = m_rareComputedStyleData.d();
        if (d.hasValue()) {
            return d.getValue();
        }

        return String::emptyString;
    }

    void setD(String* d)
    {
        *m_rareComputedStyleData.ensureD() = d;
    }

    Length x()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return Length();
        }

        Nullable<Length> x = m_rareComputedStyleData.x();
        if (x.hasValue()) {
            return x.getValue();
        }

        return Length();
    }

    void setX(Length x)
    {
        *m_rareComputedStyleData.ensureX() = x;
    }

    Length y()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return Length();
        }

        Nullable<Length> y = m_rareComputedStyleData.y();
        if (y.hasValue()) {
            return y.getValue();
        }

        return Length();
    }

    void setY(Length y)
    {
        *m_rareComputedStyleData.ensureY() = y;
    }

    Length r()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return Length();
        }

        Nullable<Length> r = m_rareComputedStyleData.r();
        if (r.hasValue()) {
            return r.getValue();
        }

        return Length();
    }

    void setR(Length r)
    {
        *m_rareComputedStyleData.ensureR() = r;
    }

    Length cx()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return Length();
        }

        Nullable<Length> x = m_rareComputedStyleData.cx();
        if (x.hasValue()) {
            return x.getValue();
        }

        return Length();
    }

    void setCX(Length x)
    {
        *m_rareComputedStyleData.ensureCX() = x;
    }

    Length cy()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return Length();
        }

        Nullable<Length> y = m_rareComputedStyleData.cy();
        if (y.hasValue()) {
            return y.getValue();
        }

        return Length();
    }

    void setCY(Length y)
    {
        *m_rareComputedStyleData.ensureCY() = y;
    }

    Length rx()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return Length();
        }

        Nullable<Length> x = m_rareComputedStyleData.rx();
        if (x.hasValue()) {
            return x.getValue();
        }

        return Length();
    }

    void setRX(Length x)
    {
        *m_rareComputedStyleData.ensureRX() = x;
    }

    Length ry()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return Length();
        }

        Nullable<Length> y = m_rareComputedStyleData.ry();
        if (y.hasValue()) {
            return y.getValue();
        }

        return Length();
    }

    void setRY(Length y)
    {
        *m_rareComputedStyleData.ensureRY() = y;
    }

    StyleBackgroundData* background()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return nullptr;
        }

        StyleBackgroundData* background = m_rareComputedStyleData.background();
        if (background) {
            return background;
        }

        return nullptr;
    }

    bool isFourSideBorderStyleValueSolid();

    BorderData border()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            BorderData b = BorderData();
            b.makeZeroWidth();
            return b;
        }

        BorderData* border = m_rareComputedStyleData.border();
        if (border) {
            return *border;
        }

        BorderData b = BorderData();
        b.makeZeroWidth();
        return b;
    }

    StyleTransformOrigin* transformOrigin()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return nullptr;
        }

        StyleTransformOrigin* transformOrigin =
            m_rareComputedStyleData.transformOrigin();
        if (transformOrigin) {
            return transformOrigin;
        }

        return nullptr;
    }

    bool hasTransformOrigin()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return false;
        }

        return m_rareComputedStyleData.transformOrigin() != nullptr;
    }

    bool hasObjectSizing()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return false;
        }

        return m_rareComputedStyleData.objectSizing() != nullptr;
    }

#define SET_BORDER_COLOR(UPOS, LPOS, ...)                               \
    void setBorder##UPOS##Color(Unit::Color color)                      \
    {                                                                   \
        m_rareComputedStyleData.ensureBorder()->LPOS().setColor(color); \
    }
    GEN_FOURSIDE(SET_BORDER_COLOR)
#undef SET_BORDER_COLOR

#define CLEAR_BORDER_COLOR(UPOS, LPOS, ...)                          \
    void clearBorder##UPOS##Color()                                  \
    {                                                                \
        m_rareComputedStyleData.ensureBorder()->LPOS().clearColor(); \
    }
    GEN_FOURSIDE(CLEAR_BORDER_COLOR)
#undef CLEAR_BORDER_COLOR

#define SET_BORDER_STYLE(UPOS, LPOS, ...)                               \
    void setBorder##UPOS##Style(BorderStyleValue style)                 \
    {                                                                   \
        m_rareComputedStyleData.ensureBorder()->LPOS().setStyle(style); \
    }
    GEN_FOURSIDE(SET_BORDER_STYLE)
#undef SET_BORDER_STYLE

#define SET_BORDER_WIDTH(UPOS, LPOS, ...)                               \
    void setBorder##UPOS##Width(Length width)                           \
    {                                                                   \
        m_rareComputedStyleData.ensureBorder()->LPOS().setWidth(width); \
    }
    GEN_FOURSIDE(SET_BORDER_WIDTH)
#undef SET_BORDER_WIDTH

    void setBorderImageSource(String* url)
    {
        m_rareComputedStyleData.ensureBorder()->image().setUrl(url);
    }

    void setBorderImageSlices(BorderImageLengthBox slices)
    {
        m_rareComputedStyleData.ensureBorder()->image().setSlices(slices);
    }

    void setBorderImageSliceFill(bool fill)
    {
        m_rareComputedStyleData.ensureBorder()->image().setSliceFill(fill);
    }

    void setBorderImageWidths(BorderImageLengthBox value)
    {
        m_rareComputedStyleData.ensureBorder()->image().setWidths(value);
    }

    void setBorderImageResource(ImageResource* value)
    {
        m_rareComputedStyleData.ensureBorder()->image().setImageResource(value);
    }

    void setBorderImageRepeatX(const BorderImageRepeatValue v)
    {
        m_rareComputedStyleData.ensureBorder()->image().setRepeatX(v);
    }

    void setBorderImageRepeatY(const BorderImageRepeatValue v)
    {
        m_rareComputedStyleData.ensureBorder()->image().setRepeatY(v);
    }

    void setBorderImageSliceFromOther(ComputedStyle* other)
    {
        BorderData oBorder = other->border();
        BorderData* border = m_rareComputedStyleData.ensureBorder();
        border->image().setSlices(oBorder.image().slices());
        border->image().setSliceFill(oBorder.image().sliceFill());
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
        if (!m_rareComputedStyleData.m_styles.size()) {
            return TransitionPropertyValue::TransitionPropertyAllValue;
        }

        StyleTransitionData* transition = m_rareComputedStyleData.transition();
        if (transition) {
            return transition->property();
        }

        return TransitionPropertyValue::TransitionPropertyAllValue;
    }

    void setTransitionProperty(TransitionPropertyValue property)
    {
        m_rareComputedStyleData.ensureTransition()->setProperty(property);
    }

    CSSTime transitionDuration()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return CSSTime(0);
        }

        StyleTransitionData* transition = m_rareComputedStyleData.transition();
        if (transition) {
            return transition->duration();
        }

        return CSSTime(0);
    }

    void setTransitionDuration(CSSTime duration)
    {
        m_rareComputedStyleData.ensureTransition()->setDuration(duration);
    }

    CSSTime transitionDelay()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return CSSTime(0);
        }

        StyleTransitionData* transition = m_rareComputedStyleData.transition();
        if (transition) {
            return transition->delay();
        }

        return CSSTime(0);
    }

    void setTransitionDelay(CSSTime duration)
    {
        m_rareComputedStyleData.ensureTransition()->setDelay(duration);
    }

    TransitionTimingFunctionValue transitionTimingFunction()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return TransitionTimingFunctionValue::
                TransitionTimingFunctionEaseValue;
        }

        StyleTransitionData* transition = m_rareComputedStyleData.transition();
        if (transition) {
            return transition->timingFunction();
        }

        return TransitionTimingFunctionValue::TransitionTimingFunctionEaseValue;
    }

    void setTransitionTimingFunction(TransitionTimingFunctionValue f)
    {
        m_rareComputedStyleData.ensureTransition()->setTimingFunction(f);
    }

#define SET_SIDE(UPOS, ...)                                      \
    void set##UPOS(Length unit)                                  \
    {                                                            \
        m_rareComputedStyleData.ensureOffset()->set##UPOS(unit); \
    }
    GEN_FOURSIDE(SET_SIDE)
#undef SET_SIDE

#define SET_MARGIN(UPOS, ...)                                    \
    void setMargin##UPOS(Length unit)                            \
    {                                                            \
        m_rareComputedStyleData.ensureMargin()->set##UPOS(unit); \
    }
    GEN_FOURSIDE(SET_MARGIN)
#undef SET_MARGIN

#define SET_PADDING(UPOS, ...)                                    \
    void setPadding##UPOS(Length unit)                            \
    {                                                             \
        m_rareComputedStyleData.ensurePadding()->set##UPOS(unit); \
    }
    GEN_FOURSIDE(SET_PADDING)
#undef SET_PADDING

    LengthData offset()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return LengthData(Length());
        }

        LengthData* offset = m_rareComputedStyleData.offset();
        if (offset) {
            return *offset;
        }

        return LengthData(Length());
    }

#define GET_SIDE(UPOS, LPOS, ...)           \
    Length LPOS()                           \
    {                                       \
        LengthData offset = this->offset(); \
        return offset.LPOS();               \
    }
    GEN_FOURSIDE(GET_SIDE)
#undef GET_SIDE

    LengthData margin()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return LengthData();
        }

        LengthData* margin = m_rareComputedStyleData.margin();
        if (margin) {
            return *margin;
        }

        return LengthData();
    }

    LengthData padding()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return LengthData();
        }

        LengthData* padding = m_rareComputedStyleData.padding();
        if (padding) {
            return *padding;
        }

        return LengthData();
    }

    Length fontSize()
    {
        return m_inheritedStyles.m_fontSize;
    }

    void setFontSize(Length l)
    {
        m_inheritedStyles.m_fontSize = l;
    }

    float fixedFontSize()
    {
        return m_inheritedStyles.m_fontSize.fixed();
    }

    void setFontFamily(FontFamilyData* datas)
    {
        m_inheritedStyles.m_fontFamilyDatas = datas;
    }

    FontFamilyData* fontFamily()
    {
        return m_inheritedStyles.m_fontFamilyDatas;
    }

    void setLetterSpacing(Length len)
    {
        if (!len.isFixed() || letterSpacing() != len)
            ensureInheritedRareData()->m_letterSpacing = len;
    }

    VisibilityValue visibility()
    {
        // only table elements needs `collapse` value.
        return m_inheritedStyles.m_visibility ==
                       VisibilityValue::VisibleVisibilityValue
                   ? VisibilityValue::VisibleVisibilityValue
                   : VisibilityValue::HiddenVisibilityValue;
    }

    VisibilityValue originalVisibility()
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

    WordWrapValue wordWrap()
    {
        return m_inheritedStyles.m_wordWrap;
    }

    FontKerningValue fontKerning()
    {
        if (m_inheritedStyles.m_rareData) {
            return m_inheritedStyles.m_rareData->m_fontKerning;
        }
        return InheritedStylesRareData().m_fontKerning;
    }

    void setFontKerning(FontKerningValue v)
    {
        if (v != fontKerning())
            ensureInheritedRareData()->m_fontKerning = v;
    }

    ImageRenderingValue imageRendering()
    {
        if (m_inheritedStyles.m_rareData) {
            return m_inheritedStyles.m_rareData->m_imageRendering;
        }
        return InheritedStylesRareData().m_imageRendering;
    }

    void setImageRendering(ImageRenderingValue v)
    {
        if (v != imageRendering())
            ensureInheritedRareData()->m_imageRendering = v;
    }

    Length letterSpacing()
    {
        if (m_inheritedStyles.m_rareData) {
            return m_inheritedStyles.m_rareData->m_letterSpacing;
        }
        return InheritedStylesRareData().m_letterSpacing;
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

    void loadFont(Node* consumer, bool respectLetterSpacing = false);
    bool hasBorderRadius()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return false;
        }

        return m_rareComputedStyleData.borderRadius() != nullptr;
    }

    BorderRadiusData borderRadius()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return BorderRadiusData();
        }

        BorderRadiusData* borderRadius = m_rareComputedStyleData.borderRadius();
        if (borderRadius) {
            return *borderRadius;
        }

        return BorderRadiusData();
    }

    ObjectSizingData objectSizing()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return ObjectSizingData();
        }

        ObjectSizingData* objectSizing = m_rareComputedStyleData.objectSizing();
        if (objectSizing) {
            return *objectSizing;
        }
        return ObjectSizingData();
    }

    ObjectFitValue objectFit()
    {
        return objectSizing().objectFit();
    }

    void setObjectFit(const ObjectFitValue& v)
    {
        auto s = rareComputedStyleData()->ensureObjectSizing();
        s->setObjectFit(v);
    }

    Length objectPositionX()
    {
        return objectSizing().offsetX();
    }

    Length objectPositionY()
    {
        return objectSizing().offsetY();
    }

    void setObjectPosition(const Length& x, const Length& y)
    {
        auto s = rareComputedStyleData()->ensureObjectSizing();
        s->setObjectPosition(x, y);
    }

    void setBorderTopLeftRadius(const Length& v, const Length& v2)
    {
        auto s = rareComputedStyleData()->ensureBorderRadius();
        s->m_topLeftHorizontal = v;
        s->m_topLeftVertical = v2;
    }

    void setBorderTopRightRadius(const Length& v, const Length& v2)
    {
        auto s = rareComputedStyleData()->ensureBorderRadius();
        s->m_topRightHorizontal = v;
        s->m_topRightVertical = v2;
    }

    void setBorderBottomRightRadius(const Length& v, const Length& v2)
    {
        auto s = rareComputedStyleData()->ensureBorderRadius();
        s->m_bottomRightHorizontal = v;
        s->m_bottomRightVertical = v2;
    }

    void setBorderBottomLeftRadius(const Length& v, const Length& v2)
    {
        auto s = rareComputedStyleData()->ensureBorderRadius();
        s->m_bottomLeftHorizontal = v;
        s->m_bottomLeftVertical = v2;
    }

    void loadBackgroundImage(
        Node* consumer,
        ComputedStyle* prevComputedStyleValueForReferenceLoadedResources =
            nullptr);
    void loadBorderImage(
        Node* consumer,
        ComputedStyle* prevComputedStyleValueForReferenceLoadedResources =
            nullptr);
    void loadListStyleImage(
        Node* consumer,
        ComputedStyle* prevComputedStyleValueForReferenceLoadedResources =
            nullptr);
    void loadResources(
        Node* consumer,
        ComputedStyle* prevComputedStyleValueForReferenceLoadedResources =
            nullptr);
    void arrangeStyleValues(ComputedStyle* parentStyle, Node* current);
    void changeFontPercentToFixedIfNeeded(Length parentFontSize,
                                          Length rootFontSize, Font* font,
                                          Node* current);
    void blockify(Node* current, bool force);

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
        if (m_inheritedStyles.m_rareData) {
            return m_inheritedStyles.m_rareData->m_horizontalBorderSpacing;
        }
        return InheritedStylesRareData().m_horizontalBorderSpacing;
    }

    void setHorizontalBorderSpacing(Length v)
    {
        if (!v.isFixed() || v != horizontalBorderSpacing())
            ensureInheritedRareData()->m_horizontalBorderSpacing = v;
    }

    Length verticalBorderSpacing()
    {
        if (m_inheritedStyles.m_rareData) {
            return m_inheritedStyles.m_rareData->m_verticalBorderSpacing;
        }
        return InheritedStylesRareData().m_verticalBorderSpacing;
    }

    void setVerticalBorderSpacing(Length v)
    {
        if (!v.isFixed() || v != verticalBorderSpacing())
            ensureInheritedRareData()->m_verticalBorderSpacing = v;
    }

    StylePaintData fill()
    {
        if (m_inheritedStyles.m_rareData) {
            return m_inheritedStyles.m_rareData->m_fill;
        }
        return InheritedStylesRareData().m_fill;
    }

    void setFill(StylePaintData v)
    {
        if (v != fill())
            ensureInheritedRareData()->m_fill = v;
    }

    FillRuleValue fillRule()
    {
        if (m_inheritedStyles.m_rareData) {
            return m_inheritedStyles.m_rareData->m_fillRule;
        }
        return InheritedStylesRareData().m_fillRule;
    }

    void setFillRule(FillRuleValue v)
    {
        if (v != fillRule())
            ensureInheritedRareData()->m_fillRule = v;
    }

    float fillOpacity()
    {
        if (m_inheritedStyles.m_rareData) {
            return m_inheritedStyles.m_rareData->m_fillOpacity;
        }
        return InheritedStylesRareData().m_fillOpacity;
    }

    void setFillOpacity(float v)
    {
        if (v != fillOpacity())
            ensureInheritedRareData()->m_fillOpacity = v;
    }

    StylePaintData stroke()
    {
        if (m_inheritedStyles.m_rareData) {
            return m_inheritedStyles.m_rareData->m_stroke;
        }
        return InheritedStylesRareData().m_stroke;
    }

    void setStroke(StylePaintData v)
    {
        if (v != stroke())
            ensureInheritedRareData()->m_stroke = v;
    }

    Length strokeWidth()
    {
        if (m_inheritedStyles.m_rareData) {
            return m_inheritedStyles.m_rareData->m_strokeWidth;
        }
        return InheritedStylesRareData().m_strokeWidth;
    }

    void setStrokeWidth(Length v)
    {
        if (!v.isFixed() || v != strokeWidth())
            ensureInheritedRareData()->m_strokeWidth = v;
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
        *m_rareComputedStyleData.ensureOrder() = order;
    }

    int32_t order()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return 0;
        }

        Nullable<int32_t> order = m_rareComputedStyleData.order();
        if (order.hasValue()) {
            return order.getValue();
        }

        return 0;
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
        *m_rareComputedStyleData.ensureFlexGrow() = value;
    }

    float flexGrow()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return 0;
        }

        Nullable<float> flexGrow = m_rareComputedStyleData.flexGrow();
        if (flexGrow.hasValue()) {
            return flexGrow.getValue();
        }

        return 0;
    }

    void setFlexShrink(float value)
    {
        *m_rareComputedStyleData.ensureFlexShrink() = value;
    }

    float flexShrink()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return 1;
        }

        Nullable<float> flexShrink = m_rareComputedStyleData.flexShrink();
        if (flexShrink.hasValue()) {
            return flexShrink.getValue();
        }

        return 1;
    }

    void setFlexBasis(FlexBasisData value)
    {
        *m_rareComputedStyleData.ensureFlexBasis() = value;
    }

    FlexBasisData flexBasis()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return FlexBasisData(false);
        }

        FlexBasisData* flexBasis = m_rareComputedStyleData.flexBasis();
        if (flexBasis) {
            return *flexBasis;
        }

        return FlexBasisData(false);
    }

    ContentDataGroup* content()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return nullptr;
        }

        ContentDataGroup* content = m_rareComputedStyleData.content();
        if (content) {
            return content;
        }

        return nullptr;
    }

    RectData* clip()
    {
        RectData* rect = m_rareComputedStyleData.clip();
        return rect;
    }

    GCVector<GridLength>* gridTemplateColumns()
    {
        GCVector<GridLength>* gridTemplate =
            m_rareComputedStyleData.gridTemplateColumns();
        return gridTemplate;
    }

    GCVector<GridLength>* gridTemplateRows()
    {
        GCVector<GridLength>* gridTemplate =
            m_rareComputedStyleData.gridTemplateRows();
        return gridTemplate;
    }

    TextOverflowData textOverflow()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return TextOverflowData();
        }

        TextOverflowData* o = m_rareComputedStyleData.textOverflow();
        if (o) {
            return *o;
        }

        return TextOverflowData();
    }

    void setTextOverflow(TextOverflowData v)
    {
        *m_rareComputedStyleData.ensureTextOverflow() = v;
    }

    void clearContent()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return;
        }

        m_rareComputedStyleData.clearContent();
    }

    void setContentText(String* text)
    {
        ContentData content(ContentData::ContentType::Text);
        content.setText(text);
        m_rareComputedStyleData.ensureContent()->push_back(content);
    }

    void setContentImage(String* image)
    {
        ContentData content(ContentData::ContentType::Image);
        content.setImage(image);
        m_rareComputedStyleData.ensureContent()->push_back(content);
    }

    void setContentCounter(const AtomicString& id, const CounterStyle* v)
    {
        ContentData content(ContentData::ContentType::Counter);
        content.setCounter(id, v);
        m_rareComputedStyleData.ensureContent()->push_back(content);
    }

    void setContentCounters(const AtomicString& id, String* sp,
                            const CounterStyle* v)
    {
        ContentData content(ContentData::ContentType::Counter);
        content.setCounters(id, sp, v);
        m_rareComputedStyleData.ensureContent()->push_back(content);
    }

    CounterBaseList* counterReset()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return nullptr;
        }
        return m_rareComputedStyleData.counterReset();
    }

    CounterBaseList* counterIncrement()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return nullptr;
        }
        return m_rareComputedStyleData.counterIncrement();
    }

    void setCounterReset(CounterBaseList* v)
    {
        if (!v && !m_rareComputedStyleData.m_styles.size()) {
            return;
        }
        m_rareComputedStyleData.setCounterReset(v);
    }

    void setCounterResetItem(const AtomicString& id, int32_t v)
    {
        m_rareComputedStyleData.ensureCounterReset()->emplace_back(id, v);
    }

    void setCounterIncrement(CounterBaseList* v)
    {
        if (!v && !m_rareComputedStyleData.m_styles.size()) {
            return;
        }
        m_rareComputedStyleData.setCounterIncrement(v);
    }

    void setCounterIncrementItem(const AtomicString& id, int32_t v)
    {
        m_rareComputedStyleData.ensureCounterIncrement()->emplace_back(id, v);
    }

    void setPseudoType(StyleResolver::PseudoElementType id)
    {
        m_pseudoId = id;
    }

    StyleResolver::PseudoElementType pseudoType()
    {
        return m_pseudoId;
    }

    void setStyleDamageSource(StyleResolver::StyleDamageSource result)
    {
        m_styleDamageSource =
            (StyleResolver::StyleDamageSource)(m_styleDamageSource | result);
    }

    StyleResolver::StyleDamageSource styleDamageSource() const
    {
        return m_styleDamageSource;
    }

    GCVector<ComputedStyle*>* cachedPseudoStyles()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return nullptr;
        }

        GCVector<ComputedStyle*>* pseudoStyles =
            m_rareComputedStyleData.cachedPsuedoStyles();
        if (pseudoStyles) {
            return pseudoStyles;
        }

        return nullptr;
    }

    ComputedStyle* pseudoStyle(Element* containerElement,
                               StyleResolver::PseudoElementType pid,
                               ComputedStyle* stickyInheritFrom = nullptr);
    bool seenPseudoElement(StyleResolver::PseudoElementType pseudoId)
    {
        if (pseudoId == StyleResolver::PseudoElementType::PseudoElementBefore) {
            if (seenPseudoElementBefore()) {
                return true;
            }
        } else if (pseudoId ==
                   StyleResolver::PseudoElementType::PseudoElementAfter) {
            if (seenPseudoElementAfter()) {
                return true;
            }
        } else if (pseudoId ==
                   StyleResolver::PseudoElementType::PseudoElementFirstLetter) {
            if (seenPseudoElementFirstLetter()) {
                return true;
            }
        } else if (pseudoId ==
                   StyleResolver::PseudoElementType::PseudoElementFirstLine) {
            if (seenPseudoElementFirstLine()) {
                return true;
            }
        } else if (pseudoId == StyleResolver::PseudoElementType::
                                   PseudoElementFirstLineInherited) {
            if (seenPseudoElementFirstLine()) {
                return true;
            }
        } else {
            STARFISH_ASSERT_NOT_REACHED();
        }
        return false;
    }

    bool hasRareComputeStyleData() const
    {
        return m_rareComputedStyleData.m_styles.size();
    }

    RareComputedStyleData* rareComputedStyleData()
    {
        return &m_rareComputedStyleData;
    }

    BorderStyleValue outlineStyle()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return BorderStyleValue::NoneBorderStyleValue;
        }

        OutlineData* outline = m_rareComputedStyleData.outline();
        if (outline) {
            return outline->border().style();
        }

        return BorderStyleValue::NoneBorderStyleValue;
    }

    void setOutlineStyle(BorderStyleValue v)
    {
        rareComputedStyleData()->ensureOutline()->border().setStyle(v);
    }

    Length outlineWidth()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return Length(Length::Fixed, 2);
        }

        OutlineData* outline = m_rareComputedStyleData.outline();
        if (outline) {
            return outline->border().width();
        }

        return Length(Length::Fixed, 2);
    }

    void setOutlineWidth(Length v)
    {
        rareComputedStyleData()->ensureOutline()->border().setWidth(v);
    }

    Unit::Color outlineColor()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return color();
        }

        OutlineData* outline = m_rareComputedStyleData.outline();
        if (outline) {
            return outline->border().color();
        }
        return color();
    }

    void setOutlineColor(Unit::Color v)
    {
        rareComputedStyleData()->ensureOutline()->border().setColor(v);
    }

    Length outlineOffset()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return Length(Length::Fixed, 0);
        }

        OutlineData* outline = m_rareComputedStyleData.outline();
        if (outline) {
            return outline->offset();
        }

        return Length(Length::Fixed, 0);
    }

    void setOutlineOffset(Length v)
    {
        rareComputedStyleData()->ensureOutline()->setOffset(v);
    }

    String* maskImage()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return String::emptyString;
        }

        PositionedMaskData* positionedMask =
            m_rareComputedStyleData.positionedMask();
        if (positionedMask) {
            return positionedMask->image();
        }

        return String::emptyString;
    }

    void setMaskImage(String* url)
    {
        rareComputedStyleData()->ensurePositionedMask()->setImage(url);
    }

    void setMaskSize(MaskSizeValue size, unsigned int layer = 0)
    {
        rareComputedStyleData()->ensurePositionedMask()->setSize(size, layer);
    }

    void setMaskSize(LengthSize size, unsigned int layer = 0)
    {
        rareComputedStyleData()->ensurePositionedMask()->setSize(size, layer);
    }

    LengthSize maskSizeLengthValue(unsigned int layer = 0)
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return LengthSize();
        }

        PositionedMaskData* positionedMask =
            m_rareComputedStyleData.positionedMask();
        if (positionedMask) {
            return positionedMask->maskSizeLengthValue(layer);
        }

        return LengthSize();
    }

    MaskSizeValue maskSizeTypeValue(unsigned int layer = 0)
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return MaskSizeValue::ContainMaskSizeValue;
        }

        PositionedMaskData* positionedMask =
            m_rareComputedStyleData.positionedMask();
        if (positionedMask) {
            return positionedMask->maskSizeTypeValue(layer);
        }

        return MaskSizeValue::ContainMaskSizeValue;
    }

    size_t maskSizeLayerLength()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return 0;
        }

        PositionedMaskData* positionedMask =
            m_rareComputedStyleData.positionedMask();
        if (positionedMask) {
            return positionedMask->size();
        }

        return 0;
    }

    const ListStyleData listStyleData()
    {
        if (m_inheritedStyles.m_rareData) {
            return m_inheritedStyles.m_rareData->m_listStyleData;
        }
        return ListStyleData();
    }

    bool hasVisibleListCounter()
    {
        return listStyleData().typeData()->system() != CounterStyle::NoneSystem;
    }

    String* listStyleType()
    {
        return listStyleData().type();
    }

    void setListStyleType(String* v)
    {
        ensureInheritedRareData()->m_listStyleData.setType(v);
    }

    void setListStyleType(const CounterStyle* v)
    {
        ensureInheritedRareData()->m_listStyleData.setType(v);
    }

    String* listStyleImage()
    {
        return listStyleData().image();
    }

    void setListStyleImage(String* v)
    {
        ensureInheritedRareData()->m_listStyleData.setImage(v);
    }

    void setListStyleImage(ImageResource* v)
    {
        ensureInheritedRareData()->m_listStyleData.setImageResource(v);
    }

    ListStylePositionValue listStylePosition()
    {
        if (m_inheritedStyles.m_rareData) {
            return m_inheritedStyles.m_rareData->m_listStyleData.position();
        }
        return ListStyleData().position();
    }

    void setListStylePosition(ListStylePositionValue v)
    {
        ensureInheritedRareData()->m_listStyleData.setPosition(v);
    }

    bool seenPseudoElementFirstLine() const
    {
        return m_seenPseudoElementFirstLine;
    }

    bool seenPseudoElementFirstLetter() const
    {
        return m_seenPseudoElementFirstLetter;
    }

    bool seenPseudoElementBefore() const
    {
        return m_seenPseudoElementBefore;
    }

    bool seenPseudoElementAfter() const
    {
        return m_seenPseudoElementAfter;
    }

    UserSelectValue userSelect()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return UserSelectValue::NoneUserSelectValue;
        }

        Nullable<UserSelectValue> us = rareComputedStyleData()->userSelect();
        if (us.hasValue()) {
            return us.getValue();
        }

        return UserSelectValue::NoneUserSelectValue;
    }

    void setUserSelect(UserSelectValue us)
    {
        *m_rareComputedStyleData.ensureUserSelect() = us;
    }

    Unit::Color caretColor()
    {
        if (m_inheritedStyles.m_rareData) {
            return m_inheritedStyles.m_rareData->m_caretColor;
        }
        return InheritedStylesRareData().m_caretColor;
    }

    void setCaretColor(Unit::Color c)
    {
        ensureInheritedRareData()->m_caretColor = c;
    }

    HyphensValue hyphens()
    {
        if (m_inheritedStyles.m_rareData) {
            return m_inheritedStyles.m_rareData->m_hyphens;
        }
        return InheritedStylesRareData().m_hyphens;
    }

    void setHyphens(HyphensValue v)
    {
        ensureInheritedRareData()->m_hyphens = v;
    }

    LineBreakValue lineBreak()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return LineBreakValue::NormalLineBreakValue;
        }

        Nullable<LineBreakValue> v = rareComputedStyleData()->lineBreak();
        if (v.hasValue()) {
            return v.getValue();
        }

        return LineBreakValue::NormalLineBreakValue;
    }

    void setLineBreak(LineBreakValue v)
    {
        *m_rareComputedStyleData.ensureLineBreak() = v;
    }

    WordBreakValue wordBreak()
    {
        if (m_inheritedStyles.m_rareData) {
            return m_inheritedStyles.m_rareData->m_wordBreak;
        }
        return InheritedStylesRareData().m_wordBreak;
    }

    void setWordBreak(WordBreakValue v)
    {
        ensureInheritedRareData()->m_wordBreak = v;
    }

    void* operator new(size_t size);
    void* operator new(size_t /* size */, void* p)
    {
        return p;
    }
    void* operator new[](size_t size) = delete;

protected:
    ComputedStyle* cachedPseudoStyle(StyleResolver::PseudoElementType pid);
    ComputedStyle* addCachedPseudoStyle(ComputedStyle* pseudoStyle);
    void removeCachedPseudoStyle(StyleResolver::PseudoElementType pid);

    InheritedStylesRareData* ensureInheritedRareData()
    {
        if (m_inheritedStyles.m_isRareDataAllocated) {
            return m_inheritedStyles.m_rareData;
        }
        InheritedStylesRareData* newData = new InheritedStylesRareData();
        if (m_inheritedStyles.m_rareData)
            *newData = *m_inheritedStyles.m_rareData;

        m_inheritedStyles.m_rareData = newData;
        m_inheritedStyles.m_isRareDataAllocated = true;

        return m_inheritedStyles.m_rareData;
    }

    void initNonInheritedStyles()
    {
        m_display = DisplayValue::InlineDisplayValue;
        m_position = PositionValue::StaticPositionValue;
        m_float = FloatValue::NoneFloatValue;
        m_clear = ClearValue::NoneClearValue;
        m_zIndexSpecifiedByUser = false;
        m_overflowX = OverflowValue::VisibleOverflow;
        m_overflowY = OverflowValue::VisibleOverflow;
        m_textDecorationLine =
            TextDecorationLineValue::NoneTextDecorationLineValue;
        m_verticalAlign = VerticalAlignValue::BaselineVAlignValue;
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
        m_styleDamageSource = StyleResolver::StyleDamageSource::NoDamage;
    }

    // NOTICE
    // if you add new property, you MUST implement comparing style for new
    // property in [compareStyle function]
    struct InheritedStyles {
        FontStyleValue m_fontStyle : 2;
        FontWeightValue m_fontWeight : 4;
        WordWrapValue m_wordWrap : 1;
        TextAlignValue m_textAlign : 3;
        DirectionValue m_direction : 1;
        WhiteSpaceValue m_whiteSpace : 3;
        VisibilityValue m_visibility : 2;
        BorderCollapseValue m_borderCollapse : 1; // table
        CaptionSideValue m_captionSide : 1;       // table
        EmptyCellsValue m_emptyCells : 1;         // table
        bool m_isRareDataAllocated : 1;

        FontFamilyData* m_fontFamilyDatas; // [size_t, String, String...]
        Unit::Color m_color;
        Length m_fontSize;
        Length m_lineHeight;
        InheritedStylesRareData* m_rareData;
    } m_inheritedStyles;

    bool m_seenViewPortUnitInStyle : 1;
    bool m_seenPseudoElementFirstLine : 1;
    bool m_seenPseudoElementFirstLetter : 1;
    bool m_seenPseudoElementBefore : 1;
    bool m_seenPseudoElementAfter : 1;
    bool m_seenPseudoElementFirstLineInherited : 1;
    bool m_gotInheritedColor : 1;
    bool m_usedInAnimator : 1;
    FloatValue m_float : 2;
    ClearValue m_clear : 2;
    DisplayValue m_display : 5;
    DisplayValue m_originalDisplay : 5;
    PositionValue m_position : 2;
    VerticalAlignValue m_verticalAlign : 4;
    OverflowValue m_overflowX : 2;
    OverflowValue m_overflowY : 2;
    TextDecorationLineValue m_textDecorationLine : 3;

    UnicodeBidiValue m_unicodeBidi : 2;
    BoxSizingValue m_boxSizing : 1;
    TableLayoutValue m_tableLayout : 1; // table
    FlexDirectionValue m_flexDirection : 2;
    FlexWrapValue m_flexWrap : 2;
    JustifyContentValue m_justifyContent : 3;
    AlignItemValue m_alignItems : 3;
    bool m_alignSelfSpecifiedByUser : 1;
    AlignItemValue m_alignSelf : 3;
    AlignContentValue m_alignContent : 3;
    StyleResolver::PseudoElementType m_pseudoId : 6;
    StyleResolver::StyleDamageSource m_styleDamageSource : 5;
    bool m_zIndexSpecifiedByUser : 1;

    Length m_width;
    Length m_height;

    Font* m_font;

    RareComputedStyleData m_rareComputedStyleData;
};

ComputedStyleDamage compareStyle(ComputedStyle* oldStyle,
                                 ComputedStyle* newStyle, bool* damagedKeys);

void applyTransition(Element* element, ComputedStyle* oldStyle,
                     ComputedStyle* newStyle, const bool* damagedKeys);
}

#endif
