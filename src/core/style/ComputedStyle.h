/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#ifndef __StarfishComputedStyle__
#define __StarfishComputedStyle__

#include <SkMatrix.h>

#include "core/style/BorderRadiusData.h"
#include "core/style/BorderData.h"
#include "core/style/FlowRelativeBorderData.h"
#include "core/style/LengthData.h"
#include "core/style/FlowRelativeLengthData.h"
#include "core/style/ContentData.h"
#include "core/style/CounterBaseList.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/style/FlexBasisData.h"
#include "core/style/Style.h"
#include "core/style/StyleAnimationData.h"
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
#include "core/style/WillChangeData.h"

namespace Starfish {

class Frame;
class AnimationTimingFunction;
class StyleResolveContext;

enum ComputedStyleDamage {
    ComputedStyleDamageNone = 0,
    ComputedStyleDamageInherited = 1,
    ComputedStyleDamageRebuildFrame = 1 << 1,
    ComputedStyleDamageLayout = 1 << 2,
    ComputedStyleDamageEstablishesStackingContext = 1 << 3,
    ComputedStyleDamageComputeStackingContextProperties = 1 << 4,
    ComputedStyleDamagePainting = 1 << 5,
    ComputedStyleDamageComposite = 1 << 6
};

union FontFamilyData {
    size_t m_length;
    AtomicString m_familyName;

    FontFamilyData(size_t len)
        : m_length(len)
    {
    }

    FontFamilyData(const AtomicString& familyName)
        : m_familyName(familyName)
    {
    }
};

class FilterFunctions;

class RareComputedStyleData : public gc {
public:
    enum KeyKind : unsigned {
        Order,
        ZIndex,
        FlexGrow,
        FlexShrink,
        Opacity,
        Border,
        BorderBlockStart,
        BorderBlockEnd,
        BoxDecorationBreak,
        BoxShadow,
        Width,
        Height,
        Padding,
        PaddingInlineEnd,
        PaddingInlineStart,
        Margin,
        MarginBlockStart,
        MarginBlockEnd,
        MarginInlineEnd,
        MarginInlineStart,
        Offset,
        MinWidth,
        MaxWidth,
        MinHeight,
        MaxHeight,
        Filter,
        FlexBasis,
        VerticalAlignLength,
        Transforms,
        TransformOrigin,
        Transition,
        Animation,
        TextDecorationColor,
        TextDecorationStyle,
        TextUnderlinePosition,
        TextDecorationLine,
        Resize,
        Content,
        Quote,
        Outline,
        BorderRadius,
        PositionedMask,
        CachedPsuedoStyles,
        Background,
        ObjectSizing,
        X,
        Y,
        X1,
        Y1,
        X2,
        Y2,
        CX,
        CY,
        RX,
        RY,
        R,
        D,
        Clip,
        ClipPath,
        UserSelect,
        CaretColor,
        Hyphens,
        LineBreak,
        WordBreak,
        TextOverflow,
        CounterReset,
        CounterIncrement,
        Appearance,
        LineClamp,
        StopColor,
        StopOpacity,
        ColumnGap,
        CustomProperty,

        // Grid
        GridTemplateColumns,
        GridTemplateRows,
        GridRowStart,
        GridRowEnd,
        GridColumnStart,
        GridColumnEnd,
        GridRowGap,
        GridTemplateAreas,

        WillChange
    };

    union RareComputedStyleValue {
        int32_t m_int32Value;
        float m_floatValue;
        String* m_stringValue;
        Length m_length;
        BorderData* m_borderData;
        FlowRelativeBorderBlockData* m_flowRelativeBorderBlockData;
        BoxDecorationBreakValue m_boxDecorationBreak;
        LengthData* m_lengthData;
        FlowRelativeLengthBlockData* m_flowRelativeLengthBlockData;
        FlowRelativeLengthInlineData* m_flowRelativeLengthInlineData;
        FlexBasisData* m_flexBasis;
        StyleTransformDataGroup* m_transforms;
        StyleTransformOrigin* m_transformOrigin;
        StyleTransitionData* m_transition;
        StyleAnimationData* m_animation;
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
        GCVector<GridTrackSize>* m_gridTemplateUnits;
        Unit::Color m_color;
        HyphensValue m_hyphens;
        LineBreakValue m_lineBreak;
        WordBreakValue m_wordBreak;
        TextDecorationStyleValue m_textDecorationStyle;
        TextUnderlinePositionValue m_textUnderlinePosition;
        ResizeValue m_resize;
        QuoteValue m_quote;
        TextOverflowData* m_textOverflow;
        CounterBaseList* m_counterBaseList;
        WillChangeData* m_willChange;
        ValueList* m_textDecorationLine;
        FilterFunctions* m_filter;
        AppearanceValue m_appearance;
        StylePaintData* m_stopColor; // svg
        MutablePropertyValueList* m_mutablePropertyValueList;

        RareComputedStyleValue()
            : m_int32Value(0)
        {
        }

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

        RareComputedStyleValue(
            FlowRelativeBorderBlockData* flowRelativeBorderBlockData)
            : m_flowRelativeBorderBlockData(flowRelativeBorderBlockData)
        {
        }

        RareComputedStyleValue(BoxDecorationBreakValue v)
            : m_boxDecorationBreak(v)
        {
        }

        RareComputedStyleValue(LengthData* lengthData)
            : m_lengthData(lengthData)
        {
        }

        RareComputedStyleValue(
            FlowRelativeLengthBlockData* flowRelativeLengthBlockData)
            : m_flowRelativeLengthBlockData(flowRelativeLengthBlockData)
        {
        }

        RareComputedStyleValue(
            FlowRelativeLengthInlineData* flowRelativeLengthInlineData)
            : m_flowRelativeLengthInlineData(flowRelativeLengthInlineData)
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

        RareComputedStyleValue(StyleAnimationData* animation)
            : m_animation(animation)
        {
            STARFISH_ASSERT(animation != nullptr);
        }

        RareComputedStyleValue(ContentDataGroup* content)
            : m_content(content)
        {
            STARFISH_ASSERT(content != nullptr);
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

        RareComputedStyleValue(ResizeValue v)
            : m_resize(v)
        {
        }

        RareComputedStyleValue(QuoteValue v)
            : m_quote(v)
        {
        }

        RareComputedStyleValue(GCVector<GridTrackSize>* gridTemplate)
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

        RareComputedStyleValue(AppearanceValue v)
            : m_appearance(v)
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

        RareComputedStyleValue(WillChangeData* v)
            : m_willChange(v)
        {
        }

        RareComputedStyleValue(ValueList* v)
            : m_textDecorationLine(v)
        {
        }

        RareComputedStyleValue(FilterFunctions* v)
            : m_filter(v)
        {
        }

        RareComputedStyleValue(StylePaintData* v)
            : m_stopColor(v)
        {
        }

        RareComputedStyleValue(MutablePropertyValueList* v)
            : m_mutablePropertyValueList(v)
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

    RareComputedStyleData()
    {
    }

#define FIND_VALUE(Name)                        \
    auto it = m_styles.begin();                 \
    auto end = m_styles.end();                  \
    while (end != it) {                         \
        if ((*it).keyKind() == KeyKind::Name) { \
            break;                              \
        }                                       \
        it++;                                   \
    }

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

#define GETTER_VALUE(RETURN_TYPE, VALUE_NAME, name, Name, initVal)  \
    RETURN_TYPE* ensure##Name()                                     \
    {                                                               \
        FIND_VALUE(Name);                                           \
                                                                    \
        if (it == m_styles.end()) {                                 \
            RETURN_TYPE name = initVal;                             \
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

    GETTER_VALUE(int32_t, int32Value, order, Order, 0);
    GETTER_VALUE(int32_t, int32Value, zIndex, ZIndex, 0);
    GETTER_VALUE(float, floatValue, flexGrow, FlexGrow, 0);
    GETTER_VALUE(float, floatValue, flexShrink, FlexShrink, 0);
    GETTER_VALUE(float, floatValue, opacity, Opacity, 0);
    GETTER_VALUE(String*, stringValue, d, D, nullptr);
    GETTER_VALUE(String*, stringValue, gridTemplateAreas, GridTemplateAreas,
                 nullptr);
    GETTER_VALUE(int32_t, int32Value, lineClamp, LineClamp, 0);

    GETTER_VALUE(String*, stringValue, gridRowStart, GridRowStart, nullptr);
    GETTER_VALUE(String*, stringValue, gridRowEnd, GridRowEnd, nullptr);
    GETTER_VALUE(String*, stringValue, gridColumnStart, GridColumnStart,
                 nullptr);
    GETTER_VALUE(String*, stringValue, gridColumnEnd, GridColumnEnd, nullptr);

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

    GETTER_VALUE(Length, length, width, Width, 0);
    GETTER_VALUE(Length, length, height, Height, 0);
    GETTER_VALUE(Length, length, minWidth, MinWidth, 0);
    GETTER_VALUE(Length, length, maxWidth, MaxWidth, 0);
    GETTER_VALUE(Length, length, minHeight, MinHeight, 0);
    GETTER_VALUE(Length, length, maxHeight, MaxHeight, 0);
    GETTER_VALUE(Length, length, verticalAlignLength, VerticalAlignLength, 0);
    GETTER_VALUE(Length, length, x, X, 0);
    GETTER_VALUE(Length, length, y, Y, 0);
    GETTER_VALUE(Length, length, x1, X1, 0);
    GETTER_VALUE(Length, length, y1, Y1, 0);
    GETTER_VALUE(Length, length, x2, X2, 0);
    GETTER_VALUE(Length, length, y2, Y2, 0);
    GETTER_VALUE(Length, length, r, R, 0);
    GETTER_VALUE(Length, length, cx, CX, 0);
    GETTER_VALUE(Length, length, cy, CY, 0);
    GETTER_VALUE(Length, length, rx, RX, 0);
    GETTER_VALUE(Length, length, ry, RY, 0);
    GETTER_VALUE(StylePaintData*, stopColor, stopColor, StopColor, nullptr);
    GETTER_VALUE(float, floatValue, stopOpacity, StopOpacity, 1);
    GETTER_VALUE(Length, length, gridRowGap, GridRowGap, 0);
    GETTER_VALUE(UserSelectValue, userSelect, userSelect, UserSelect,
                 NoneUserSelectValue);
    GETTER_VALUE(LineBreakValue, lineBreak, lineBreak, LineBreak,
                 NormalLineBreakValue);
    GETTER_VALUE(AppearanceValue, appearance, appearance, Appearance,
                 AutoAppearanceValue);
    GETTER_VALUE(Unit::Color, color, textDecorationColor, TextDecorationColor,
                 Unit::Color());
    GETTER_VALUE(TextDecorationStyleValue, textDecorationStyle,
                 textDecorationStyle, TextDecorationStyle,
                 SolidTextDecorationStyleValue);
    GETTER_VALUE(ResizeValue, resize, resize, Resize, NoneResizeValue);
    GETTER_VALUE(QuoteValue, quote, quote, Quote, OpenQuoteValue);
    GETTER_VALUE(BoxDecorationBreakValue, boxDecorationBreak,
                 boxDecorationBreak, BoxDecorationBreak,
                 SliceBoxDecorationBreakValue);
    GETTER_VALUE(String*, stringValue, clipPath, ClipPath, nullptr);
    GETTER_VALUE(Length, length, columnGap, ColumnGap, 0);
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
    GETTER_PTR(FlowRelativeLengthBlockData, flowRelativeLengthBlockData,
               marginBlockStart, MarginBlockStart);
    GETTER_PTR(FlowRelativeLengthBlockData, flowRelativeLengthBlockData,
               marginBlockEnd, MarginBlockEnd);
    GETTER_PTR(FlowRelativeLengthInlineData, flowRelativeLengthInlineData,
               marginInlineEnd, MarginInlineEnd);
    GETTER_PTR(FlowRelativeLengthInlineData, flowRelativeLengthInlineData,
               marginInlineStart, MarginInlineStart);
    GETTER_PTR(LengthData, lengthData, padding, Padding);
    GETTER_PTR(FlowRelativeLengthInlineData, flowRelativeLengthInlineData,
               paddingInlineEnd, PaddingInlineEnd);
    GETTER_PTR(FlowRelativeLengthInlineData, flowRelativeLengthInlineData,
               paddingInlineStart, PaddingInlineStart);
    GETTER_PTR(BorderData, borderData, border, Border);
    GETTER_PTR(FlowRelativeBorderBlockData, flowRelativeBorderBlockData,
               borderBlockStart, BorderBlockStart);
    GETTER_PTR(FlowRelativeBorderBlockData, flowRelativeBorderBlockData,
               borderBlockEnd, BorderBlockEnd);
    GETTER_PTR(StyleTransformDataGroup, transforms, transforms, Transforms);
    GETTER_PTR(StyleTransformOrigin, transformOrigin, transformOrigin,
               TransformOrigin);
    GETTER_PTR(StyleTransitionData, transition, transition, Transition);
    GETTER_PTR(StyleAnimationData, animation, animation, Animation);
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
    GETTER_PTR(GCVector<GridTrackSize>, gridTemplateUnits, gridTemplateColumns,
               GridTemplateColumns);
    GETTER_PTR(GCVector<GridTrackSize>, gridTemplateUnits, gridTemplateRows,
               GridTemplateRows);
    GETTER_PTR(TextOverflowData, textOverflow, textOverflow, TextOverflow);
    GETTER_PTR(CounterBaseList, counterBaseList, counterReset, CounterReset);
    GETTER_PTR(CounterBaseList, counterBaseList, counterIncrement,
               CounterIncrement);
    GETTER_PTR(ValueList, textDecorationLine, textDecorationLine,
               TextDecorationLine);
    GETTER_PTR(MutablePropertyValueList, mutablePropertyValueList,
               customProperty, CustomProperty);

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

    WillChangeData* willChange()
    {
        FIND_VALUE(WillChange);
        if (it == m_styles.end()) {
            return nullptr;
        }
        return (*it).m_value.m_willChange;
    }

    void setWillChange(WillChangeData* v)
    {
        clearWillChange();
        if (v) {
            m_styles.emplace_back(KeyKind::WillChange, v);
        }
    }

    void setTextDecorationLine(ValueList* v)
    {
        clearTextDecorationLine();
        if (v) {
            m_styles.emplace_back(KeyKind::TextDecorationLine, v);
        }
    }

    FilterFunctions* filter()
    {
        FIND_VALUE(Filter);
        if (it == m_styles.end()) {
            return nullptr;
        }
        return (*it).m_value.m_filter;
    }

    void setFilter(FilterFunctions* v)
    {
        clearFilter();
        if (v) {
            m_styles.emplace_back(KeyKind::Filter, v);
        }
    }

    void setCustomProperty(MutablePropertyValueList* v)
    {
        FIND_VALUE(CustomProperty);
        if (it == m_styles.end()) {
            m_styles.emplace_back(KeyKind::CustomProperty, v);
        } else {
            it->m_value = v;
        }
    }

#undef GETTER_PTR

    CLEARER(Transforms);
    CLEARER(Content);
    CLEARER(CounterReset);
    CLEARER(CounterIncrement);
    CLEARER(WillChange);
    CLEARER(TextDecorationLine);
    CLEARER(Clip);
    CLEARER(Filter);

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
        WordBreakValue m_wordBreak : 3;
        TextUnderlinePositionValue m_textUnderlinePosition : 3;
        PointerEventsValue m_pointerEventsValue : 4;

        Length m_letterSpacing;
        Length m_textIndent;
        Length m_wordSpacing;
        Length m_horizontalBorderSpacing; // table
        Length m_verticalBorderSpacing;   // table

        StylePaintData* m_fill;   // svg
        float m_fillOpacity;      // svg
        StylePaintData* m_stroke; // svg
        float m_strokeOpacity;    // svg
        Length m_strokeWidth;     // svg

        ShadowDataList m_textShadowDataList;
        ListStyleData m_listStyleData;

        Unit::Color m_caretColor;

        InheritedStylesRareData()
        {
            m_letterSpacing = Length(Length::Fixed, 0);
            m_wordSpacing = Length(Length::Fixed, 0);
            m_textIndent = Length(Length::Fixed, 0);
            m_horizontalBorderSpacing = Length(Length::Fixed, 0);
            m_verticalBorderSpacing = Length(Length::Fixed, 0);

            m_fill = new StylePaintData(Unit::Color(0, 0, 0, 0xff));
            m_fillRule = FillRuleNonZero;
            m_fillOpacity = 1;
            m_strokeOpacity = 1;
            m_stroke = new StylePaintData(Unit::Color(0, 0, 0, 0));
            m_strokeWidth = Length(Length::Fixed, 1);

            m_textTransform = NoneTextTransformValue;
            m_caretColor = Unit::Color(0, 0, 0, 255);
            m_hyphens = HyphensValue::NoneHyphensValue;
            m_fontKerning = FontKerningValue::FontKerningAutoValue;
            m_imageRendering = ImageRenderingValue::ImageRenderingAutoValue;
            m_wordBreak = WordBreakValue::NormalWordBreakValue;
            m_textUnderlinePosition =
                TextUnderlinePositionValue::AutoTextUnderlinePositionValue;
            m_pointerEventsValue = PointerEventsValue::PointerEventsAutoValue;
        }

        void* operator new(size_t size);
    };

    ComputedStyle(uint32_t mediumFontSize)
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
        m_someNonInheritMemberExplicitlyInherited = false;
        m_originalDisplay = DisplayValue::InlineDisplayValue;

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
        m_someNonInheritMemberExplicitlyInherited = false;

        initNonInheritedStyles();
    }

    bool seenViewPortUnitInStyle()
    {
        return m_seenViewPortUnitInStyle;
    }

    bool someNonInheritMemberExplicitlyInherited()
    {
        return m_someNonInheritMemberExplicitlyInherited;
    }

    void markSomeNonInheritMemberExplicitlyInherited()
    {
        m_someNonInheritMemberExplicitlyInherited = true;
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

    bool isAbsolutePositioned()
    {
        return m_position == PositionValue::AbsolutePositionValue ||
               m_position == PositionValue::FixedPositionValue;
    }

    void setPosition(PositionValue p)
    {
        m_position = p;
    }

    void setClip(RectData* r)
    {
        if (r) {
            *m_rareComputedStyleData.ensureClip() = *r;
        } else {
            m_rareComputedStyleData.clearClip();
        }
    }

    void setClipPath(String* url)
    {
        *m_rareComputedStyleData.ensureClipPath() = url;
    }

    void setPaddingInlineEnd(FlowRelativeLengthInlineData length)
    {
        *m_rareComputedStyleData.ensurePaddingInlineEnd() = length;
    }

    void setPaddingInlineStart(FlowRelativeLengthInlineData length)
    {
        *m_rareComputedStyleData.ensurePaddingInlineStart() = length;
    }

    void setMarginBlockStart(FlowRelativeLengthBlockData length)
    {
        *m_rareComputedStyleData.ensureMarginBlockStart() = length;
    }

    void setMarginBlockEnd(FlowRelativeLengthBlockData length)
    {
        *m_rareComputedStyleData.ensureMarginBlockEnd() = length;
    }

    void setMarginInlineEnd(FlowRelativeLengthInlineData length)
    {
        *m_rareComputedStyleData.ensureMarginInlineEnd() = length;
    }

    void setMarginInlineStart(FlowRelativeLengthInlineData length)
    {
        *m_rareComputedStyleData.ensureMarginInlineStart() = length;
    }

    void setGridTemplateColumns(GCVector<GridTrackSize>* gridTemplate)
    {
        *m_rareComputedStyleData.ensureGridTemplateColumns() = *gridTemplate;
    }

    void setGridTemplateRows(GCVector<GridTrackSize>* gridTemplate)
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
        if (!hasRareComputeStyleData()) {
            return Length();
        }

        Nullable<Length> ret = m_rareComputedStyleData.width();
        if (ret.hasValue()) {
            return ret.getValue();
        }

        return Length();
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
        *m_rareComputedStyleData.ensureWidth() = l;
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
        if (!hasRareComputeStyleData()) {
            return Length();
        }

        Nullable<Length> ret = m_rareComputedStyleData.height();
        if (ret.hasValue()) {
            return ret.getValue();
        }

        return Length();
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
        *m_rareComputedStyleData.ensureHeight() = l;
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
            TextAlignValue::WebKitCenterTextAlignValue) {
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
        return Length(Length::Fixed, 0);
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
        return NoneTextTransformValue;
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

    void setTextShadow(const ShadowDataList& val)
    {
        if (textShadow() == nullptr && val.size() == 0) {
            return;
        }
        if (textShadow() == nullptr || val != *textShadow()) {
            ensureInheritedRareData()->m_textShadowDataList = val;
        }
    }

    ShadowDataList* textShadow()
    {
        if (m_inheritedStyles.m_rareData) {
            return &m_inheritedStyles.m_rareData->m_textShadowDataList;
        }
        return nullptr;
    }

    void addBoxShadow(ShadowData& shadow)
    {
        m_rareComputedStyleData.ensureBoxShadow()->push_back(shadow);
    }

    void setBoxShadow(const ShadowDataList& val)
    {
        if (boxShadow() == nullptr && val.size() == 0) {
            return;
        }
        if (boxShadow() == nullptr || val != *boxShadow()) {
            (*m_rareComputedStyleData.ensureBoxShadow()) = val;
        }
    }

    ShadowDataList* boxShadow()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return nullptr;
        }
        ShadowDataList* shadowDataList = m_rareComputedStyleData.boxShadow();
        if (shadowDataList) {
            return shadowDataList;
        }

        return nullptr;
    }

    BoxDecorationBreakValue boxDecorationBreak()
    {
        if (m_rareComputedStyleData.m_styles.size()) {
            Nullable<BoxDecorationBreakValue> v =
                m_rareComputedStyleData.boxDecorationBreak();
            if (v.hasValue()) {
                return v.getValue();
            }
        }
        return SliceBoxDecorationBreakValue;
    }

    void setBoxDecorationBreak(BoxDecorationBreakValue v)
    {
        *m_rareComputedStyleData.ensureBoxDecorationBreak() = v;
    }

    ValueList* textDecorationLine()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return nullptr;
        }
        return m_rareComputedStyleData.textDecorationLine();
    }

    ValueList* ensureTextDecorationLine()
    {
        return m_rareComputedStyleData.ensureTextDecorationLine();
    }

    void setTextDecorationLine(ValueList* decorationLines)
    {
        m_rareComputedStyleData.setTextDecorationLine(decorationLines);
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
        return TextUnderlinePositionValue::AutoTextUnderlinePositionValue;
    }

    void setTextUnderlinePosition(TextUnderlinePositionValue v)
    {
        ensureInheritedRareData()->m_textUnderlinePosition = v;
    }

    PointerEventsValue pointerEvents()
    {
        if (m_inheritedStyles.m_rareData) {
            return m_inheritedStyles.m_rareData->m_pointerEventsValue;
        }
        return PointerEventsValue::PointerEventsAutoValue;
    }

    void setPointerEvents(PointerEventsValue v)
    {
        ensureInheritedRareData()->m_pointerEventsValue = v;
    }

    ResizeValue resize()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return ResizeValue::NoneResizeValue;
        }

        Nullable<ResizeValue> v = rareComputedStyleData()->resize();
        if (v.hasValue()) {
            return v.getValue();
        }

        return ResizeValue::NoneResizeValue;
    }

    void setResize(ResizeValue v)
    {
        *m_rareComputedStyleData.ensureResize() = v;
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

    bool hasFilter();
    bool hasAvailableFilter();

    bool hasTransforms(Frame* frame);
    bool hasComplexTransforms(Frame* frame);
    bool has3DTransforms(Frame* frame);

    StyleTransformDataGroup* transforms(Frame* frame = nullptr);

    static SkMatrix transformToMatrix(StyleTransformDataGroup* transform,
                                      LayoutUnit containerWidth,
                                      LayoutUnit containerHeight, Frame* f);
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

    void setTransformRotate(double a, Length x = Length(Length::Fixed, 0),
                            Length y = Length(Length::Fixed, 0))
    {
        StyleTransformData t(StyleTransformData::OperationType::Rotate);
        t.setRotate(a, x, y);
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
        m_rareComputedStyleData.ensureBackground()->setColor(color);
    }

    void setBackgroundColorToCurrentColor()
    {
        m_rareComputedStyleData.ensureBackground()->setColorToCurrentColor();
    }

    void setBackgroundImage(ImageValue* image, uint32_t layer)
    {
        m_rareComputedStyleData.ensureBackground()->setImage(image, layer);
    }

    void setBackgroundImageResource(ImageResource* img, uint32_t layer)
    {
        m_rareComputedStyleData.ensureBackground()->setImageResource(img,
                                                                     layer);
    }

    void setBackgroundRepeatX(RepeatStyleValue repeat, uint32_t layer)
    {
        m_rareComputedStyleData.ensureBackground()->setRepeatX(repeat, layer);
    }

    void setBackgroundRepeatY(RepeatStyleValue repeat, uint32_t layer)
    {
        m_rareComputedStyleData.ensureBackground()->setRepeatY(repeat, layer);
    }

    void setBackgroundPositionX(Length value, uint32_t layer)
    {
        m_rareComputedStyleData.ensureBackground()->setPositionX(value, layer);
    }

    void setBackgroundPositionY(Length value, uint32_t layer)
    {
        m_rareComputedStyleData.ensureBackground()->setPositionY(value, layer);
    }

    void setBackgroundSize(BackgroundSizeValue size, uint32_t layer)
    {
        m_rareComputedStyleData.ensureBackground()->setSize(size, layer);
    }

    void setBackgroundSize(const LengthSize& size, uint32_t layer)
    {
        m_rareComputedStyleData.ensureBackground()->setSize(size, layer);
    }

    void setBackgroundAttachment(BackgroundAttachmentValue attachment,
                                 uint32_t layer)
    {
        m_rareComputedStyleData.ensureBackground()->setAttachment(attachment,
                                                                  layer);
    }

    void setBackgroundClip(BoxValue clip, uint32_t layer)
    {
        m_rareComputedStyleData.ensureBackground()->setClip(clip, layer);
    }

    void setBackgroundOrigin(BoxValue origin, uint32_t layer)
    {
        m_rareComputedStyleData.ensureBackground()->setOrigin(origin, layer);
    }

    void resetBackgroundImages()
    {
        StyleBackgroundData* data = background();
        if (data) {
            data->shrinkImages(0);
        }
    }

    void resetBackgroundRepeatXs()
    {
        StyleBackgroundData* data = background();
        if (data) {
            data->shrinkRepeatXs(0);
        }
    }

    void resetBackgroundRepeatYs()
    {
        StyleBackgroundData* data = background();
        if (data) {
            data->shrinkRepeatYs(0);
        }
    }

    void resetBackgroundPositionXs()
    {
        StyleBackgroundData* data = background();
        if (data) {
            data->shrinkPositionXs(0);
        }
    }

    void resetBackgroundPositionYs()
    {
        StyleBackgroundData* data = background();
        if (data) {
            data->shrinkPositionYs(0);
        }
    }

    void resetBackgroundSizes()
    {
        StyleBackgroundData* data = background();
        if (data) {
            data->shrinkSizes(0);
        }
    }

    void resetBackgroundAttachments()
    {
        StyleBackgroundData* data = background();
        if (data) {
            data->shrinkAttachments(0);
        }
    }

    void resetBackgroundClips()
    {
        StyleBackgroundData* data = background();
        if (data) {
            data->shrinkClips(0);
        }
    }

    void resetBackgroundOrigins()
    {
        StyleBackgroundData* data = background();
        if (data) {
            data->shrinkOrigins(0);
        }
    }

    uint32_t backgroundLayerSize()
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
        return background->color();
    }

    NULLABLE ImageValue* backgroundImage(uint32_t layer)
    {
        StyleBackgroundData* background = this->background();
        if (background == nullptr) {
            return nullptr;
        }
        return background->image(layer);
    }

    NULLABLE NativeImageData* backgroundImageData(uint32_t layer)
    {
        StyleBackgroundData* background = this->background();
        if (background == nullptr) {
            return nullptr;
        }
        return background->imageData(layer);
    }

    RepeatStyleValue backgroundRepeatX(uint32_t layer)
    {
        StyleBackgroundData* background = this->background();
        if (background == nullptr) {
            return RepeatStyleValue::RepeatRepeatValue;
        }
        return background->repeatX(layer);
    }

    RepeatStyleValue backgroundRepeatY(uint32_t layer)
    {
        StyleBackgroundData* background = this->background();
        if (background == nullptr) {
            return RepeatStyleValue::RepeatRepeatValue;
        }
        return background->repeatY(layer);
    }

    Length backgroundPositionX(uint32_t layer)
    {
        StyleBackgroundData* background = this->background();
        if (background == nullptr) {
            return Length(Length::Percent, 0.0f);
        }
        return background->positionX(layer);
    }

    Length backgroundPositionY(uint32_t layer)
    {
        StyleBackgroundData* background = this->background();
        if (background == nullptr) {
            return Length(Length::Percent, 0.0f);
        }
        return background->positionY(layer);
    }

    bool backgroundSizeIsLength(uint32_t layer)
    {
        StyleBackgroundData* background = this->background();
        if (background == nullptr) {
            return true;
        }
        return background->sizeIsLength(layer);
    }

    BackgroundSizeValue backgroundSizeTypeValue(uint32_t layer)
    {
        StyleBackgroundData* background = this->background();
        if (background == nullptr) {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
            return BackgroundSizeValue::ContainBackgroundSizeValue;
        }
        return background->sizeTypeValue(layer);
    }

    LengthSize backgroundSizeLengthValue(uint32_t layer)
    {
        StyleBackgroundData* background = this->background();
        if (background == nullptr) {
            return LengthSize();
        }
        return background->sizeLengthValue(layer);
    }

    BackgroundAttachmentValue backgroundAttachment(uint32_t layer)
    {
        StyleBackgroundData* background = this->background();
        if (background == nullptr) {
            return BackgroundAttachmentValue::ScrollBackgroundAttachmentValue;
        }
        return background->attachment(layer);
    }

    BoxValue backgroundClip(uint32_t layer)
    {
        StyleBackgroundData* background = this->background();
        if (background == nullptr) {
            return BoxValue::BorderBoxBoxValue;
        }
        return background->clip(layer);
    }

    BoxValue backgroundOrigin(uint32_t layer)
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

    Length columnGap()
    {
        Nullable<Length> maybeLength = m_rareComputedStyleData.columnGap();
        if (maybeLength.hasValue()) {
            return maybeLength.getValue();
        }
        return Length();
    }

    void setColumnGap(const Length& length)
    {
        *m_rareComputedStyleData.ensureColumnGap() = length;
    }

    StylePaintData* stopColor()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return new StylePaintData(Unit::Color(0, 0, 0, 0xff));
        }

        Nullable<StylePaintData*> stopColor =
            m_rareComputedStyleData.stopColor();
        if (stopColor.hasValue()) {
            return stopColor.getValue();
        }

        return new StylePaintData(Unit::Color(0, 0, 0, 0xff));
    }

    void setStopColor(StylePaintData* v)
    {
        *m_rareComputedStyleData.ensureStopColor() = v;
    }

    float stopOpacity()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return 1;
        }

        Nullable<float> stopOpacity = m_rareComputedStyleData.stopOpacity();
        if (stopOpacity.hasValue()) {
            return stopOpacity.getValue();
        }

        return 1;
    }

    void setStopOpacity(float stopOpacity)
    {
        if (stopOpacity != 1) {
            *m_rareComputedStyleData.ensureStopOpacity() = stopOpacity;
        }
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

    void setGridTemplateAreas(String* areas)
    {
        *m_rareComputedStyleData.ensureGridTemplateAreas() = areas;
    }

    String* gridTemplateAreas()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return String::emptyString;
        }

        Nullable<String*> areas = m_rareComputedStyleData.gridTemplateAreas();
        if (areas.hasValue()) {
            return areas.getValue();
        }

        return String::emptyString;
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

    Length x1()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return Length();
        }

        Nullable<Length> x1 = m_rareComputedStyleData.x1();
        if (x1.hasValue()) {
            return x1.getValue();
        }

        return Length();
    }

    void setX1(Length x1)
    {
        *m_rareComputedStyleData.ensureX1() = x1;
    }

    Length y1()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return Length();
        }

        Nullable<Length> y1 = m_rareComputedStyleData.y1();
        if (y1.hasValue()) {
            return y1.getValue();
        }

        return Length();
    }

    void setY1(Length y1)
    {
        *m_rareComputedStyleData.ensureY1() = y1;
    }

    Length x2()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return Length();
        }

        Nullable<Length> x2 = m_rareComputedStyleData.x2();
        if (x2.hasValue()) {
            return x2.getValue();
        }

        return Length();
    }

    void setX2(Length x2)
    {
        *m_rareComputedStyleData.ensureX2() = x2;
    }

    Length y2()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return Length();
        }

        Nullable<Length> y2 = m_rareComputedStyleData.y2();
        if (y2.hasValue()) {
            return y2.getValue();
        }

        return Length();
    }

    void setY2(Length y2)
    {
        *m_rareComputedStyleData.ensureY2() = y2;
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
        return m_rareComputedStyleData.background();
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

    Nullable<BorderData*> nullableBorder()
    {
        return m_rareComputedStyleData.border();
    }

    FlowRelativeBorderBlockData borderBlockStart()
    {
        if (!hasRareComputeStyleData()) {
            return FlowRelativeBorderBlockData();
        }

        FlowRelativeBorderBlockData* data =
            m_rareComputedStyleData.borderBlockStart();
        if (data) {
            return *data;
        }

        return FlowRelativeBorderBlockData();
    }

    FlowRelativeBorderBlockData borderBlockEnd()
    {
        if (!hasRareComputeStyleData()) {
            return FlowRelativeBorderBlockData();
        }

        FlowRelativeBorderBlockData* data =
            m_rareComputedStyleData.borderBlockEnd();
        if (data) {
            return *data;
        }

        return FlowRelativeBorderBlockData();
    }

    StyleTransformOrigin* transformOrigin()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return new StyleTransformOrigin();
        }

        StyleTransformOrigin* transformOrigin =
            m_rareComputedStyleData.transformOrigin();
        if (transformOrigin) {
            return transformOrigin;
        }

        return new StyleTransformOrigin();
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

    void setBorderTopColor(Unit::Color color)
    {
        m_rareComputedStyleData.ensureBorder()->top().setColor(color);
        m_rareComputedStyleData.ensureBorderBlockStart()
            ->setCorrespondingTopIsSpecifiedLater(BorderValueKind::kColor,
                                                  true);
        m_rareComputedStyleData.ensureBorderBlockEnd()
            ->setCorrespondingTopIsSpecifiedLater(BorderValueKind::kColor,
                                                  true);
    }

    void setBorderRightColor(Unit::Color color)
    {
        m_rareComputedStyleData.ensureBorder()->right().setColor(color);
    }

    void setBorderBottomColor(Unit::Color color)
    {
        m_rareComputedStyleData.ensureBorder()->bottom().setColor(color);
        m_rareComputedStyleData.ensureBorderBlockStart()
            ->setCorrespondingBottomIsSpecifiedLater(BorderValueKind::kColor,
                                                     true);
        m_rareComputedStyleData.ensureBorderBlockEnd()
            ->setCorrespondingBottomIsSpecifiedLater(BorderValueKind::kColor,
                                                     true);
    }

    void setBorderLeftColor(Unit::Color color)
    {
        m_rareComputedStyleData.ensureBorder()->left().setColor(color);
    }

    void setBorderBlockStartColor(Unit::Color color)
    {
        FlowRelativeBorderBlockData* start =
            m_rareComputedStyleData.ensureBorderBlockStart();
        start->setColor(color);
        start->setCorrespondingTopIsSpecifiedLater(BorderValueKind::kColor,
                                                   false);
        start->setCorrespondingBottomIsSpecifiedLater(BorderValueKind::kColor,
                                                      false);
        start->setFromShorthand(false);
    }

    void setBorderBlockEndColor(Unit::Color color)
    {
        FlowRelativeBorderBlockData* end =
            m_rareComputedStyleData.ensureBorderBlockEnd();
        end->setColor(color);
        end->setCorrespondingTopIsSpecifiedLater(BorderValueKind::kColor,
                                                 false);
        end->setCorrespondingBottomIsSpecifiedLater(BorderValueKind::kColor,
                                                    false);
        end->setFromShorthand(false);
    }

#define CLEAR_BORDER_COLOR(UPOS, LPOS, ...)                          \
    void clearBorder##UPOS##Color()                                  \
    {                                                                \
        m_rareComputedStyleData.ensureBorder()->LPOS().clearColor(); \
    }
    GEN_FOURSIDE(CLEAR_BORDER_COLOR)
#undef CLEAR_BORDER_COLOR

    void clearBorderBlockStartColor()
    {
        FlowRelativeBorderBlockData* start =
            m_rareComputedStyleData.ensureBorderBlockStart();
        start->borderValue().clearColor();
        start->setCorrespondingTopIsSpecifiedLater(BorderValueKind::kColor,
                                                   false);
        start->setCorrespondingBottomIsSpecifiedLater(BorderValueKind::kColor,
                                                      false);
        start->setFromShorthand(false);
    }

    void clearBorderBlockEndColor()
    {
        FlowRelativeBorderBlockData* end =
            m_rareComputedStyleData.ensureBorderBlockEnd();
        end->borderValue().clearColor();
        end->setCorrespondingTopIsSpecifiedLater(BorderValueKind::kColor,
                                                 false);
        end->setCorrespondingBottomIsSpecifiedLater(BorderValueKind::kColor,
                                                    false);
        end->setFromShorthand(false);
    }

    void setBorderTopStyle(BorderStyleValue style)
    {
        m_rareComputedStyleData.ensureBorder()->top().setStyle(style);
        m_rareComputedStyleData.ensureBorderBlockStart()
            ->setCorrespondingTopIsSpecifiedLater(BorderValueKind::kStyle,
                                                  true);
        m_rareComputedStyleData.ensureBorderBlockEnd()
            ->setCorrespondingTopIsSpecifiedLater(BorderValueKind::kStyle,
                                                  true);
    }

    void setBorderRightStyle(BorderStyleValue style)
    {
        m_rareComputedStyleData.ensureBorder()->right().setStyle(style);
    }

    void setBorderBottomStyle(BorderStyleValue style)
    {
        m_rareComputedStyleData.ensureBorder()->bottom().setStyle(style);
        m_rareComputedStyleData.ensureBorderBlockStart()
            ->setCorrespondingBottomIsSpecifiedLater(BorderValueKind::kStyle,
                                                     true);
        m_rareComputedStyleData.ensureBorderBlockEnd()
            ->setCorrespondingBottomIsSpecifiedLater(BorderValueKind::kStyle,
                                                     true);
    }

    void setBorderLeftStyle(BorderStyleValue style)
    {
        m_rareComputedStyleData.ensureBorder()->left().setStyle(style);
    }

    void setBorderBlockStartStyle(BorderStyleValue style)
    {
        m_rareComputedStyleData.ensureBorderBlockStart()->setStyle(style);
        m_rareComputedStyleData.ensureBorderBlockStart()
            ->setCorrespondingTopIsSpecifiedLater(BorderValueKind::kStyle,
                                                  false);
        m_rareComputedStyleData.ensureBorderBlockStart()
            ->setCorrespondingBottomIsSpecifiedLater(BorderValueKind::kStyle,
                                                     false);
    }

    void setBorderBlockEndStyle(BorderStyleValue style)
    {
        FlowRelativeBorderBlockData* end =
            m_rareComputedStyleData.ensureBorderBlockEnd();
        end->setStyle(style);
        end->setCorrespondingTopIsSpecifiedLater(BorderValueKind::kStyle,
                                                 false);
        end->setCorrespondingBottomIsSpecifiedLater(BorderValueKind::kStyle,
                                                    false);
    }

    void setBorderTopWidth(Length width)
    {
        m_rareComputedStyleData.ensureBorder()->top().setWidth(width);
        m_rareComputedStyleData.ensureBorderBlockStart()
            ->setCorrespondingTopIsSpecifiedLater(BorderValueKind::kWidth,
                                                  true);
        m_rareComputedStyleData.ensureBorderBlockEnd()
            ->setCorrespondingTopIsSpecifiedLater(BorderValueKind::kWidth,
                                                  true);
    }

    void setBorderRightWidth(Length width)
    {
        m_rareComputedStyleData.ensureBorder()->right().setWidth(width);
    }

    void setBorderBottomWidth(Length width)
    {
        m_rareComputedStyleData.ensureBorder()->bottom().setWidth(width);
        m_rareComputedStyleData.ensureBorderBlockStart()
            ->setCorrespondingBottomIsSpecifiedLater(BorderValueKind::kWidth,
                                                     true);
        m_rareComputedStyleData.ensureBorderBlockEnd()
            ->setCorrespondingBottomIsSpecifiedLater(BorderValueKind::kWidth,
                                                     true);
    }

    void setBorderLeftWidth(Length width)
    {
        m_rareComputedStyleData.ensureBorder()->left().setWidth(width);
    }

    void setBorderBlockStartWidth(Length width)
    {
        m_rareComputedStyleData.ensureBorderBlockStart()->setWidth(width);
        m_rareComputedStyleData.ensureBorderBlockStart()
            ->setCorrespondingTopIsSpecifiedLater(BorderValueKind::kWidth,
                                                  false);
        m_rareComputedStyleData.ensureBorderBlockStart()
            ->setCorrespondingBottomIsSpecifiedLater(BorderValueKind::kWidth,
                                                     false);
    }

    void setBorderBlockEndWidth(Length width)
    {
        FlowRelativeBorderBlockData* end =
            m_rareComputedStyleData.ensureBorderBlockEnd();
        end->setWidth(width);
        end->setCorrespondingTopIsSpecifiedLater(BorderValueKind::kWidth,
                                                 false);
        end->setCorrespondingBottomIsSpecifiedLater(BorderValueKind::kWidth,
                                                    false);
    }

    void setBorderBlockStartFromShorthand(bool value)
    {
        m_rareComputedStyleData.ensureBorderBlockStart()->setFromShorthand(
            value);
    }

    void setBorderBlockEndFromShorthand(bool value)
    {
        m_rareComputedStyleData.ensureBorderBlockEnd()->setFromShorthand(value);
    }

    void setBorderImageSource(String* url)
    {
        m_rareComputedStyleData.ensureBorder()->image().setUrl(url);
    }

    void setBorderImageSource(CSSGradientValue* gradient)
    {
        STARFISH_UNIMPLEMENTED();
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

    void setBorderImageOutsets(BorderImageLengthBox value)
    {
        m_rareComputedStyleData.ensureBorder()->image().setOutsets(value);
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

    StyleTransitionData* transition()
    {
        return m_rareComputedStyleData.transition();
    }

    size_t transitionLayerSize()
    {
        StyleTransitionData* t = transition();
        if (t) {
            return t->size();
        }
        return 0;
    }

    CSSStyleValuePair::KeyKind transitionProperty(size_t layer = 0)
    {
        StyleTransitionData* t = transition();
        if (t) {
            return t->property(layer);
        }
        return CSSStyleValuePair::KeyKind::All;
    }

    CSSTime transitionDuration(size_t layer = 0)
    {
        StyleTransitionData* t = transition();
        if (t) {
            return t->duration(layer);
        }
        return CSSTime(0);
    }

    CSSTime transitionDelay(size_t layer = 0)
    {
        StyleTransitionData* t = transition();
        if (t) {
            return t->delay(layer);
        }
        return CSSTime(0);
    }

    TimingFunction* transitionTimingFunction(size_t layer = 0)
    {
        StyleTransitionData* t = transition();
        if (t) {
            return t->timingFunction(layer);
        }
        return StyleTransitionData::defaultTimingFunction();
    }

    CSSTime animationDuration(size_t layer = 0)
    {
        StyleAnimationData* a = animation();
        if (a != nullptr) {
            return a->duration(layer);
        }
        return CSSTime(0);
    }

    TimingFunction* animationTimingFunction(size_t layer = 0)
    {
        StyleAnimationData* a = animation();
        if (a != nullptr) {
            return a->timingFunction(layer);
        }
        return AnimationKeyframe::defaultTimingFunction();
    }

    CSSTime animationDelay(size_t layer = 0)
    {
        StyleAnimationData* a = animation();
        if (a != nullptr) {
            return a->delay(layer);
        }
        return CSSTime(0);
    }

    float animationIterationCount(size_t layer = 0)
    {
        StyleAnimationData* a = animation();
        if (a != nullptr) {
            return a->iterationCount(layer);
        }
        return 1;
    }

    AnimationDirectionValue animationDirect(size_t layer = 0)
    {
        StyleAnimationData* a = animation();
        if (a != nullptr) {
            return a->direction(layer);
        }
        return AnimationDirectionValue::AnimationDirectionNormalValue;
    }

    AnimationPlayStateValue animationPlayState(size_t layer = 0)
    {
        StyleAnimationData* a = animation();
        if (a != nullptr) {
            return a->playState(layer);
        }
        return AnimationPlayStateValue::AnimationPlayStateRunningValue;
    }

    AnimationFillModeValue animationFillMode(size_t layer = 0)
    {
        StyleAnimationData* a = animation();
        if (a != nullptr) {
            return a->fillMode(layer);
        }
        return AnimationFillModeValue::AnimationFillModeNoneValue;
    }

    void setTransitionProperty(CSSStyleValuePair::KeyKind property,
                               size_t layer = 0)
    {
        m_rareComputedStyleData.ensureTransition()->setProperty(property,
                                                                layer);
    }

    void setTransitionDuration(CSSTime duration, size_t layer = 0)
    {
        m_rareComputedStyleData.ensureTransition()->setDuration(duration,
                                                                layer);
    }

    void setTransitionDelay(CSSTime duration, size_t layer = 0)
    {
        m_rareComputedStyleData.ensureTransition()->setDelay(duration, layer);
    }

    void setTransitionTimingFunction(TimingFunction* v, size_t layer = 0)
    {
        STARFISH_ASSERT(v != nullptr);
        m_rareComputedStyleData.ensureTransition()->setTimingFunction(v, layer);
    }

    void setTransitionTimingFunction(TimingFunctionValue v, size_t layer = 0)
    {
        m_rareComputedStyleData.ensureTransition()->setTimingFunction(
            knownTimingFunction(v), layer);
    }

    StyleAnimationData* animation()
    {
        return m_rareComputedStyleData.animation();
    }

    size_t animationNameSize()
    {
        StyleAnimationData* a = animation();
        if (a != nullptr) {
            return a->animationNameSize();
        }
        return 0;
    }

    String* animationName(size_t index = 0)
    {
        StyleAnimationData* a = animation();
        if (a != nullptr) {
            return a->animationName(index);
        }
        return String::emptyString;
    }

    void setAnimationName(String* name, size_t index)
    {
        STARFISH_ASSERT(name != nullptr);
        m_rareComputedStyleData.ensureAnimation()->setAnimationName(name,
                                                                    index);
    }

    void setAnimationDuration(CSSTime duration, size_t index)
    {
        m_rareComputedStyleData.ensureAnimation()->setDuration(duration, index);
    }

    void setAnimationTimingFunction(TimingFunctionValue v, size_t index)
    {
        m_rareComputedStyleData.ensureAnimation()->setTimingFunction(
            knownTimingFunction(v), index);
    }

    void setAnimationTimingFunction(TimingFunction* f, size_t index)
    {
        STARFISH_ASSERT(f != nullptr);
        m_rareComputedStyleData.ensureAnimation()->setTimingFunction(f, index);
    }

    void setAnimationDelay(CSSTime delay, size_t index)
    {
        m_rareComputedStyleData.ensureAnimation()->setDelay(delay, index);
    }

    void setAnimationIterationCount(float value, size_t index)
    {
        m_rareComputedStyleData.ensureAnimation()->setIterationCount(value,
                                                                     index);
    }

    void setAnimationDirection(AnimationDirectionValue value, size_t index)
    {
        m_rareComputedStyleData.ensureAnimation()->setDirection(value, index);
    }

    void setAnimationPlayState(AnimationPlayStateValue value, size_t index)
    {
        m_rareComputedStyleData.ensureAnimation()->setPlayState(value, index);
    }

    void setAnimationFillMode(AnimationFillModeValue value, size_t index)
    {
        m_rareComputedStyleData.ensureAnimation()->setFillMode(value, index);
    }

    void resetTransitionProperties()
    {
        StyleTransitionData* t = transition();
        if (t) {
            t->shrinkProperties(0);
        }
    }

    void resetTransitionDurations()
    {
        StyleTransitionData* t = transition();
        if (t) {
            t->shrinkDurations(0);
        }
    }

    void resetTransitionDelays()
    {
        StyleTransitionData* t = transition();
        if (t) {
            t->shrinkDelays(0);
        }
    }

    void resetTransitionTimingFunctions()
    {
        StyleTransitionData* t = transition();
        if (t) {
            t->shrinkTimingFunctions(0);
        }
    }

    void resetAnimationNames()
    {
        StyleAnimationData* a = animation();
        if (a != nullptr) {
            a->clearAnimationNames();
        }
    }

    void resetAnimationDurations()
    {
        StyleAnimationData* a = animation();
        if (a != nullptr) {
            a->clearDurations();
        }
    }

    void resetAnimationTimingFunctions()
    {
        StyleAnimationData* a = animation();
        if (a != nullptr) {
            a->clearTimingFunctions();
        }
    }

    void resetAnimationDelays()
    {
        StyleAnimationData* a = animation();
        if (a != nullptr) {
            a->clearDelays();
        }
    }

    void resetAnimationIterationCount()
    {
        StyleAnimationData* a = animation();
        if (a != nullptr) {
            a->clearIterationCounts();
        }
    }

    void resetAnimationDirection()
    {
        StyleAnimationData* a = animation();
        if (a != nullptr) {
            a->clearDirections();
        }
    }

    void resetAnimationPlayState()
    {
        StyleAnimationData* a = animation();
        if (a != nullptr) {
            a->clearPlayStates();
        }
    }

    void resetAnimationFillMode()
    {
        StyleAnimationData* a = animation();
        if (a != nullptr) {
            a->clearFillMode();
        }
    }

    void setMarginTop(const Length& unit)
    {
        m_rareComputedStyleData.ensureMargin()->setTop(unit);

        m_rareComputedStyleData.ensureMarginBlockStart()
            ->markCorrespondingTopIsSet();
        m_rareComputedStyleData.ensureMarginBlockEnd()
            ->markCorrespondingTopIsSet();
    }

    void setMarginRight(const Length& unit)
    {
        m_rareComputedStyleData.ensureMargin()->setRight(unit);

        m_rareComputedStyleData.ensureMarginInlineStart()
            ->markCorrespondingRightIsSet();
        m_rareComputedStyleData.ensureMarginInlineEnd()
            ->markCorrespondingRightIsSet();
    }

    void setMarginBottom(const Length& unit)
    {
        m_rareComputedStyleData.ensureMargin()->setBottom(unit);

        m_rareComputedStyleData.ensureMarginBlockStart()
            ->markCorrespondingBottomIsSet();
        m_rareComputedStyleData.ensureMarginBlockEnd()
            ->markCorrespondingBottomIsSet();
    }

    void setMarginLeft(const Length& unit)
    {
        m_rareComputedStyleData.ensureMargin()->setLeft(unit);

        m_rareComputedStyleData.ensureMarginInlineStart()
            ->markCorrespondingLeftIsSet();
        m_rareComputedStyleData.ensureMarginInlineEnd()
            ->markCorrespondingLeftIsSet();
    }

    void setPaddingTop(const Length& unit)
    {
        m_rareComputedStyleData.ensurePadding()->setTop(unit);
    }

    void setPaddingRight(const Length& unit)
    {
        m_rareComputedStyleData.ensurePadding()->setRight(unit);

        m_rareComputedStyleData.ensurePaddingInlineStart()
            ->markCorrespondingRightIsSet();
        m_rareComputedStyleData.ensurePaddingInlineEnd()
            ->markCorrespondingRightIsSet();
    }

    void setPaddingBottom(const Length& unit)
    {
        m_rareComputedStyleData.ensurePadding()->setBottom(unit);
    }

    void setPaddingLeft(const Length& unit)
    {
        m_rareComputedStyleData.ensurePadding()->setLeft(unit);

        m_rareComputedStyleData.ensurePaddingInlineStart()
            ->markCorrespondingLeftIsSet();
        m_rareComputedStyleData.ensurePaddingInlineEnd()
            ->markCorrespondingLeftIsSet();
    }

#define SET_SIDE(UPOS, ...)                                      \
    void set##UPOS(const Length& unit)                           \
    {                                                            \
        m_rareComputedStyleData.ensureOffset()->set##UPOS(unit); \
    }
    GEN_FOURSIDE(SET_SIDE)
#undef SET_SIDE

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

    Nullable<LengthData*> nullableMargin()
    {
        return Nullable<LengthData*>(m_rareComputedStyleData.margin());
    }

    LengthData margin()
    {
        LengthData* d = m_rareComputedStyleData.margin();
        if (d) {
            return *d;
        }
        return LengthData();
    }

    Nullable<LengthData*> nullablePadding()
    {
        return Nullable<LengthData*>(m_rareComputedStyleData.padding());
    }

    LengthData padding()
    {
        LengthData* padding = m_rareComputedStyleData.padding();
        if (padding) {
            return *padding;
        }

        return LengthData();
    }

    // <margin, border, padding>
    std::tuple<Nullable<LengthData*>, Nullable<BorderData*>,
               Nullable<LengthData*>>
    marginBorderPadding()
    {
        Nullable<LengthData*> margin;
        Nullable<BorderData*> border;
        Nullable<LengthData*> padding;

        int count = 0;
        auto it = m_rareComputedStyleData.m_styles.begin();
        auto end = m_rareComputedStyleData.m_styles.end();
        while (end != it) {
            auto kk = (*it).keyKind();
            switch (kk) {
            case RareComputedStyleData::Margin:
                margin = it->m_value.m_lengthData;
                count++;
                if (count == 3) {
                    return std::make_tuple(margin, border, padding);
                }
                break;
            case RareComputedStyleData::Border:
                border = it->m_value.m_borderData;
                count++;
                if (count == 3) {
                    return std::make_tuple(margin, border, padding);
                }
                break;
            case RareComputedStyleData::Padding:
                padding = it->m_value.m_lengthData;
                count++;
                if (count == 3) {
                    return std::make_tuple(margin, border, padding);
                }
                break;
            default:
                break;
            }
            it++;
        }

        return std::make_tuple(margin, border, padding);
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

    void setVisibility(VisibilityValue v)
    {
        m_inheritedStyles.m_visibility = v;
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
        return FontKerningValue::FontKerningAutoValue;
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
        return ImageRenderingValue::ImageRenderingAutoValue;
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
        return Length(Length::Fixed, 0);
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

        auto br = m_rareComputedStyleData.borderRadius();
        if (br && !(br->m_topLeftHorizontal.hasZeroValueAnyway() &&
                    br->m_topLeftVertical.hasZeroValueAnyway() &&
                    br->m_topRightHorizontal.hasZeroValueAnyway() &&
                    br->m_topRightVertical.hasZeroValueAnyway() &&
                    br->m_bottomLeftHorizontal.hasZeroValueAnyway() &&
                    br->m_bottomLeftVertical.hasZeroValueAnyway() &&
                    br->m_bottomRightHorizontal.hasZeroValueAnyway() &&
                    br->m_bottomRightVertical.hasZeroValueAnyway())) {
            return true;
        } else {
            return false;
        }
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
    void loadMaskImage(
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
        return Length(Length::Fixed, 0);
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
        return Length(Length::Fixed, 0);
    }

    void setVerticalBorderSpacing(Length v)
    {
        if (!v.isFixed() || v != verticalBorderSpacing())
            ensureInheritedRareData()->m_verticalBorderSpacing = v;
    }

    StylePaintData* fill()
    {
        if (m_inheritedStyles.m_rareData) {
            return m_inheritedStyles.m_rareData->m_fill;
        }
        return new StylePaintData(Unit::Color(0, 0, 0, 0xff));
    }

    void setFill(StylePaintData* v)
    {
        ensureInheritedRareData()->m_fill = v;
    }

    FillRuleValue fillRule()
    {
        if (m_inheritedStyles.m_rareData) {
            return m_inheritedStyles.m_rareData->m_fillRule;
        }
        return FillRuleNonZero;
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
        return 1;
    }

    void setFillOpacity(float v)
    {
        if (v != fillOpacity())
            ensureInheritedRareData()->m_fillOpacity = v;
    }

    float strokeOpacity()
    {
        if (m_inheritedStyles.m_rareData) {
            return m_inheritedStyles.m_rareData->m_strokeOpacity;
        }
        return 1;
    }

    void setStrokeOpacity(float v)
    {
        if (v != fillOpacity())
            ensureInheritedRareData()->m_strokeOpacity = v;
    }

    StylePaintData* stroke()
    {
        if (m_inheritedStyles.m_rareData) {
            return m_inheritedStyles.m_rareData->m_stroke;
        }
        return new StylePaintData(Unit::Color(0, 0, 0, 0));
    }

    void setStroke(StylePaintData* v)
    {
        ensureInheritedRareData()->m_stroke = v;
    }

    Length strokeWidth()
    {
        if (m_inheritedStyles.m_rareData) {
            return m_inheritedStyles.m_rareData->m_strokeWidth;
        }
        return Length(Length::Fixed, 1);
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

    void setBoxOrient(BoxOrientValue value)
    {
        m_boxOrient = value;
    }

    BoxOrientValue boxOrient()
    {
        return m_boxOrient;
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

    String* clipPath()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return String::emptyString;
        }

        Nullable<String*> clipPathValue = m_rareComputedStyleData.clipPath();
        if (clipPathValue.hasValue()) {
            return clipPathValue.getValue();
        }
        return String::emptyString;
    }

    FlowRelativeLengthInlineData paddingInlineEnd()
    {
        if (!hasRareComputeStyleData()) {
            return FlowRelativeLengthInlineData();
        }

        FlowRelativeLengthInlineData* data =
            m_rareComputedStyleData.paddingInlineEnd();
        if (data) {
            return *data;
        }

        return FlowRelativeLengthInlineData();
    }

    FlowRelativeLengthInlineData paddingInlineStart()
    {
        if (!hasRareComputeStyleData()) {
            return FlowRelativeLengthInlineData();
        }

        FlowRelativeLengthInlineData* data =
            m_rareComputedStyleData.paddingInlineStart();
        if (data) {
            return *data;
        }

        return FlowRelativeLengthInlineData();
    }

    FlowRelativeLengthBlockData marginBlockStart()
    {
        if (!hasRareComputeStyleData()) {
            return FlowRelativeLengthBlockData();
        }

        FlowRelativeLengthBlockData* data =
            m_rareComputedStyleData.marginBlockStart();
        if (data) {
            return *data;
        }

        return FlowRelativeLengthBlockData();
    }

    FlowRelativeLengthBlockData marginBlockEnd()
    {
        if (!hasRareComputeStyleData()) {
            return FlowRelativeLengthBlockData();
        }

        FlowRelativeLengthBlockData* data =
            m_rareComputedStyleData.marginBlockEnd();
        if (data) {
            return *data;
        }

        return FlowRelativeLengthBlockData();
    }

    FlowRelativeLengthInlineData marginInlineEnd()
    {
        if (!hasRareComputeStyleData()) {
            return FlowRelativeLengthInlineData();
        }

        FlowRelativeLengthInlineData* data =
            m_rareComputedStyleData.marginInlineEnd();
        if (data) {
            return *data;
        }

        return FlowRelativeLengthInlineData();
    }

    FlowRelativeLengthInlineData marginInlineStart()
    {
        if (!hasRareComputeStyleData()) {
            return FlowRelativeLengthInlineData();
        }

        FlowRelativeLengthInlineData* data =
            m_rareComputedStyleData.marginInlineStart();
        if (data) {
            return *data;
        }

        return FlowRelativeLengthInlineData();
    }

    PositionedMaskData* mask()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return nullptr;
        }
        return m_rareComputedStyleData.positionedMask();
    }

    uint32_t maskLayerSize()
    {
        PositionedMaskData* positionedMaskData = this->mask();
        if (positionedMaskData == nullptr) {
            return 0;
        }
        return positionedMaskData->sizeOfLayers();
    }

    bool hasZeroClipRect()
    {
        auto c = clip();
        if (c && c->left().numberData() == 0 && c->top().numberData() == 0 &&
            c->right().numberData() == 0 && c->bottom().numberData() == 0) {
            return true;
        }

        return false;
    }

    GCVector<GridTrackSize>* gridTemplateColumns()
    {
        return m_rareComputedStyleData.gridTemplateColumns();
    }

    GCVector<GridTrackSize>* gridTemplateRows()
    {
        return m_rareComputedStyleData.gridTemplateRows();
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

    void setGridRowStart(String* v)
    {
        *m_rareComputedStyleData.ensureGridRowStart() = v;
    }

    void setGridRowEnd(String* v)
    {
        *m_rareComputedStyleData.ensureGridRowEnd() = v;
    }

    void setGridColumnStart(String* v)
    {
        *m_rareComputedStyleData.ensureGridColumnStart() = v;
    }

    void setGridColumnEnd(String* v)
    {
        *m_rareComputedStyleData.ensureGridColumnEnd() = v;
    }

    void setGridRowGap(Length l)
    {
        *m_rareComputedStyleData.ensureGridRowGap() = l;
    }

    Length gridRowGap()
    {
        Nullable<Length> gap = m_rareComputedStyleData.gridRowGap();
        if (gap.hasValue()) {
            return gap.getValue();
        }

        return Length();
    }

    String* gridRowStart()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return String::emptyString;
        }

        Nullable<String*> area = m_rareComputedStyleData.gridRowStart();
        if (area.hasValue()) {
            return area.getValue();
        }

        return String::emptyString;
    }

    String* gridRowEnd()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return String::emptyString;
        }

        Nullable<String*> area = m_rareComputedStyleData.gridRowEnd();
        if (area.hasValue()) {
            return area.getValue();
        }

        return String::emptyString;
    }

    String* gridColumnStart()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return String::emptyString;
        }

        Nullable<String*> area = m_rareComputedStyleData.gridColumnStart();
        if (area.hasValue()) {
            return area.getValue();
        }

        return String::emptyString;
    }

    String* gridColumnEnd()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return String::emptyString;
        }

        Nullable<String*> area = m_rareComputedStyleData.gridColumnEnd();
        if (area.hasValue()) {
            return area.getValue();
        }

        return String::emptyString;
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

    void setContentCounter(CounterContentData* c)
    {
        ContentData content(ContentData::ContentType::Counter);
        content.setCounter(c);
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

    void setContentQuote(QuoteValue v)
    {
        ContentData content(ContentData::ContentType::Quote);
        content.setQuote(v);
        m_rareComputedStyleData.ensureContent()->push_back(content);
    }

    bool hasQuote()
    {
        ComputedStyle* before =
            cachedPseudoStyle(PseudoElementType::PseudoElementBefore);
        ComputedStyle* after =
            cachedPseudoStyle(PseudoElementType::PseudoElementAfter);

        if (before != nullptr && hasQuote(before)) {
            return true;
        } else if (after != nullptr && hasQuote(after)) {
            return true;
        }
        return false;
    }

    bool hasQuote(ComputedStyle* style)
    {
        if (!style->m_rareComputedStyleData.m_styles.size()) {
            return false;
        }
        ContentDataGroup* content = style->rareComputedStyleData()->content();
        if (content != nullptr && content->size() == 1 &&
            content->back().isQuote()) {
            return true;
        }
        return false;
    }

    WillChangeData* willChange()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return nullptr;
        }
        return m_rareComputedStyleData.willChange();
    }

    void setWillChange(WillChangeData* v)
    {
        if (!v && !m_rareComputedStyleData.m_styles.size()) {
            return;
        }
        m_rareComputedStyleData.setWillChange(v);
    }

    void setPseudoType(PseudoElementType id)
    {
        m_pseudoId = id;
    }

    PseudoElementType pseudoType()
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

    void setStyleDamageSourceNodeStateMap(int result)
    {
        m_styleDamageSourceNodeStateMap =
            (m_styleDamageSourceNodeStateMap | result);
    }

    int styleDamageSourceNodeStateMap() const
    {
        return m_styleDamageSourceNodeStateMap;
    }

    void setStyleDamageSourceNodeStateDOMTreeMap(int result)
    {
        m_styleDamageSourceNodeStateDOMTreeMap =
            (m_styleDamageSourceNodeStateDOMTreeMap | result);
    }

    int styleDamageSourceNodeStateDOMTreeMap() const
    {
        return m_styleDamageSourceNodeStateDOMTreeMap;
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

    ComputedStyle* pseudoStyle(
        Element* containerElement, PseudoElementType pid,
        ComputedStyle* stickyInheritFrom = nullptr,
        ComputedStyle* oldPseudoStyleIfHas = nullptr,
        Nullable<StyleResolveContext*> ctx = Nullable<StyleResolveContext*>());

    bool seenPseudoElement(PseudoElementType pseudoId)
    {
        if (pseudoId == PseudoElementType::PseudoElementBefore) {
            return seenPseudoElementBefore();
        } else if (pseudoId == PseudoElementType::PseudoElementAfter) {
            return seenPseudoElementAfter();
        } else if (pseudoId == PseudoElementType::PseudoElementFirstLetter) {
            return seenPseudoElementFirstLetter();
        } else if (pseudoId == PseudoElementType::PseudoElementFirstLine) {
            return seenPseudoElementFirstLine();
        } else if (pseudoId ==
                   PseudoElementType::PseudoElementFirstLineInherited) {
            return seenPseudoElementFirstLine();
        } else {
            STARFISH_ASSERT_NOT_REACHED();
            return false;
        }
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

    ImageValue* maskImage(uint32_t layer)
    {
        PositionedMaskData* positionedMaskData = mask();
        if (positionedMaskData == nullptr) {
            return nullptr;
        }
        return positionedMaskData->image(layer);
    }

    void setMaskImage(ImageValue* image, uint32_t layer)
    {
        rareComputedStyleData()->ensurePositionedMask()->setImage(image, layer);
    }

    void setMaskImageResource(ImageResource* image, uint32_t layer)
    {
        rareComputedStyleData()->ensurePositionedMask()->setImageResource(
            image, layer);
    }

    void setMaskSize(BackgroundSizeValue size, unsigned int layer = 0)
    {
        rareComputedStyleData()->ensurePositionedMask()->setSize(size, layer);
    }

    void setMaskSize(LengthSize size, unsigned int layer = 0)
    {
        rareComputedStyleData()->ensurePositionedMask()->setSize(size, layer);
    }

    void resetMaskSizes()
    {
        PositionedMaskData* positionedMaskData = mask();
        if (positionedMaskData) {
            positionedMaskData->shrinkSizes(0);
        }
    }

    bool maskSizeIsLength(unsigned int layer = 0)
    {
        PositionedMaskData* positionedMask =
            m_rareComputedStyleData.positionedMask();
        if (positionedMask) {
            return positionedMask->maskSizeIsLength(layer);
        }
        return true;
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

    BackgroundSizeValue maskSizeTypeValue(unsigned int layer = 0)
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return BackgroundSizeValue::ContainBackgroundSizeValue;
        }

        PositionedMaskData* positionedMask =
            m_rareComputedStyleData.positionedMask();
        if (positionedMask) {
            return positionedMask->maskSizeTypeValue(layer);
        }

        return BackgroundSizeValue::ContainBackgroundSizeValue;
    }

    Length maskPositionX(uint32_t layer = 0)
    {
        PositionedMaskData* positionedMaskData = mask();
        if (positionedMaskData == nullptr) {
            return Length();
        }
        return positionedMaskData->positionX(layer);
    }

    void setMaskPositionX(Length value, uint32_t layer = 0)
    {
        rareComputedStyleData()->ensurePositionedMask()->setPositionX(value,
                                                                      layer);
    }

    void resetMaskPositionXs()
    {
        PositionedMaskData* positionedMaskData = mask();
        if (positionedMaskData) {
            positionedMaskData->shrinkPositionXs(0);
        }
    }

    Length maskPositionY(uint32_t layer = 0)
    {
        PositionedMaskData* positionedMaskData = mask();
        if (positionedMaskData == nullptr) {
            return Length();
        }
        return positionedMaskData->positionY(layer);
    }

    void setMaskPositionY(Length value, uint32_t layer = 0)
    {
        rareComputedStyleData()->ensurePositionedMask()->setPositionY(value,
                                                                      layer);
    }

    void resetMaskPositionYs()
    {
        PositionedMaskData* positionedMaskData = mask();
        if (positionedMaskData) {
            positionedMaskData->shrinkPositionYs(0);
        }
    }

    RepeatStyleValue maskRepeatX(uint32_t layer = 0)
    {
        PositionedMaskData* positionedMaskData = mask();
        if (positionedMaskData == nullptr) {
            return RepeatRepeatValue;
        }
        return positionedMaskData->repeatX(layer);
    }

    void setMaskRepeatX(RepeatStyleValue value, uint32_t layer = 0)
    {
        rareComputedStyleData()->ensurePositionedMask()->setRepeatX(value,
                                                                    layer);
    }

    void resetMaskRepeatXs()
    {
        PositionedMaskData* positionedMaskData = mask();
        if (positionedMaskData) {
            positionedMaskData->shrinkRepeatXs(0);
        }
    }

    RepeatStyleValue maskRepeatY(uint32_t layer = 0)
    {
        PositionedMaskData* positionedMaskData = mask();
        if (positionedMaskData == nullptr) {
            return RepeatRepeatValue;
        }
        return positionedMaskData->repeatY(layer);
    }

    void setMaskRepeatY(RepeatStyleValue value, uint32_t layer = 0)
    {
        rareComputedStyleData()->ensurePositionedMask()->setRepeatY(value,
                                                                    layer);
    }

    void resetMaskRepeatYs()
    {
        PositionedMaskData* positionedMaskData = mask();
        if (positionedMaskData) {
            positionedMaskData->shrinkRepeatYs(0);
        }
    }

    void resetMaskImage()
    {
        PositionedMaskData* data = mask();
        if (data) {
            data->shrinkImages(0);
        }
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
        // https://drafts.csswg.org/css-ui-3/#propdef-caret-color
        return m_inheritedStyles.m_color;
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
        return HyphensValue::NoneHyphensValue;
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

    void setLineClamp(int32_t v)
    {
        *m_rareComputedStyleData.ensureLineClamp() = v;
    }

    int32_t lineClamp()
    {
        return *m_rareComputedStyleData.ensureLineClamp();
    }

    WordBreakValue wordBreak()
    {
        if (m_inheritedStyles.m_rareData) {
            return m_inheritedStyles.m_rareData->m_wordBreak;
        }
        return WordBreakValue::NormalWordBreakValue;
    }

    void setWordBreak(WordBreakValue v)
    {
        ensureInheritedRareData()->m_wordBreak = v;
    }

    AppearanceValue appearance()
    {
        if (!m_rareComputedStyleData.m_styles.size()) {
            return AppearanceValue::AutoAppearanceValue;
        }

        Nullable<AppearanceValue> v = rareComputedStyleData()->appearance();
        if (v.hasValue()) {
            return v.getValue();
        }

        return AppearanceValue::AutoAppearanceValue;
    }

    void setAppearance(AppearanceValue v)
    {
        *m_rareComputedStyleData.ensureAppearance() = v;
    }

    FilterFunctions* filter()
    {
        return m_rareComputedStyleData.filter();
    }

    void setFilter(FilterFunctions* v)
    {
        if (!v && !m_rareComputedStyleData.m_styles.size()) {
            return;
        }
        m_rareComputedStyleData.setFilter(v);
    }

    bool hasCustomProperty()
    {
        return !!customProperty();
    }

    Nullable<MutablePropertyValueList*> customProperty()
    {
        return m_rareComputedStyleData.customProperty();
    }

    static TimingFunction* knownTimingFunction(TimingFunctionValue v);

    void* operator new(size_t size);
    void* operator new(size_t /* size */, void* p)
    {
        return p;
    }
    void* operator new[](size_t size) = delete;

protected:
    ComputedStyle* cachedPseudoStyle(PseudoElementType pid);
    ComputedStyle* addCachedPseudoStyle(ComputedStyle* pseudoStyle);
    static ComputedStyle* pseudoStyleForElementInternal(
        Node* node, PseudoElementType pseudoId, ComputedStyle* parentStyle,
        ComputedStyle* oldPseudoStyleIfHas, Nullable<StyleResolveContext*> ctx);
    void removeCachedPseudoStyle(PseudoElementType pid);

    void applyFlowRelativeBlockProperties();
    void applyFlowRelativeInlineProperties();

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
        m_verticalAlign = VerticalAlignValue::BaselineVAlignValue;
        m_unicodeBidi = UnicodeBidiValue::NormalUnicodeBidiValue;
        m_boxSizing = BoxSizingValue::ContentBoxBoxSizingValue;
        m_boxOrient = BoxOrientValue::HorizontalBoxOrientValue;
        m_tableLayout = TableLayoutValue::AutoTableLayoutValue;
        m_flexDirection = FlexDirectionValue::RowFlexDirectionValue;
        m_flexWrap = FlexWrapValue::NoWrapFlexWrapValue;
        m_justifyContent = JustifyContentValue::FlexStartJustifyContentValue;
        m_alignItems = AlignItemValue::StretchAlignItemValue;
        m_alignSelf = AlignItemValue::StretchAlignItemValue;
        m_alignSelfSpecifiedByUser = false;
        m_alignContent = AlignContentValue::StretchAlignContentValue;
        m_pseudoId = PseudoElementType::PseudoElementNone;
        m_styleDamageSource = StyleResolver::StyleDamageSource::NoDamage;
        m_styleDamageSourceNodeStateMap = 0;
        m_styleDamageSourceNodeStateDOMTreeMap = 0;
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
    bool m_gotInheritedColor : 1;
    bool m_someNonInheritMemberExplicitlyInherited : 1;
    FloatValue m_float : 2;
    ClearValue m_clear : 2;
    DisplayValue m_display : 5;
    DisplayValue m_originalDisplay : 5;
    PositionValue m_position : 2;
    VerticalAlignValue m_verticalAlign : 4;
    OverflowValue m_overflowX : 2;
    OverflowValue m_overflowY : 2;
    UnicodeBidiValue m_unicodeBidi : 2;
    BoxSizingValue m_boxSizing : 1;
    BoxOrientValue m_boxOrient : 1;
    TableLayoutValue m_tableLayout : 1; // table
    FlexDirectionValue m_flexDirection : 2;
    FlexWrapValue m_flexWrap : 2;
    JustifyContentValue m_justifyContent : 3;
    AlignItemValue m_alignItems : 3;
    bool m_alignSelfSpecifiedByUser : 1;
    AlignItemValue m_alignSelf : 3;
    AlignContentValue m_alignContent : 3;
    PseudoElementType m_pseudoId : 3;
    StyleResolver::StyleDamageSource m_styleDamageSource : 6;
    int m_styleDamageSourceNodeStateMap : 5;
    int m_styleDamageSourceNodeStateDOMTreeMap : 5;
    bool m_zIndexSpecifiedByUser : 1;

    Font* m_font;

    RareComputedStyleData m_rareComputedStyleData;
};

struct KeyframeAnimationOptions;

ComputedStyleDamage compareStyle(ComputedStyle* oldStyle,
                                 ComputedStyle* newStyle, bool* damagedKeys);

void computeTransition(Element* element, NULLABLE ComputedStyle* oldStyle,
                       NULLABLE Frame* oldFrame, ComputedStyle* style,
                       ComputedStyleDamage& damage,
                       bool (&damagedKeys)[CSSStyleValuePair::KeyKindSize]);

void computeAnimation(StyleResolver& resolver, Element* element,
                      NULLABLE ComputedStyle* oldStyle,
                      NULLABLE Frame* oldFrame, ComputedStyle* style,
                      ComputedStyleDamage& damage,
                      bool (&damagedKeys)[CSSStyleValuePair::KeyKindSize]);
void computeCSSAnimationKeyframes(const StyleResolver& resolver,
                                  Element* element, ComputedStyle* style);
void computeWebAnimationKeyframes(const StyleResolver& resolver,
                                  Element* element, ComputedStyle* style,
                                  std::vector<StyleRuleBase*>& keyframes);
} // namespace Starfish

#endif
