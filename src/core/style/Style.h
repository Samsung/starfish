/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Alexey Proskuryakov <ap@webkit.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
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

#ifndef __StarfishStyle__
#define __StarfishStyle__

#include "StaticStrings.h"
#include "binding/DocumentHoldable.h"
#include "core/style/CSSTokenValue.h"
#include "core/style/CSSAngle.h"
#include "core/style/CSSLength.h"
#include "core/style/ImageValue.h"
#include "core/style/CSSTime.h"
#include "core/style/NamedColors.h"
#include "core/style/MediaQueryEvaluator.h"
#include "core/style/Length.h"
#include "core/style/GridTrackSize.h"
#include "core/style/GridAreaData.h"
#include "core/style/RectData.h"
#include "core/style/TextOverflowData.h"
#include "core/style/MutablePropertyValue.h"
#include "core/style/MutablePropertyValueList.h"
#include "core/modules/canvas/BlendMode.h"
#include "core/util/VectorWithInlineStorage.h"
#include "core/util/BloomFilter.h"

namespace Starfish {

class AncestorSelectorFilter;
class TimingFunction;
class CalcData;
class ComputedStyle;
class StyleRule;
class StyleRuleImport;
class StyleRuleBase;
class Document;
class Element;
class ShadowRoot;
class MediaQuerySet;
class MediaQueryEvaluator;
class Node;
class RuleSet;
class StyleRuleKeyframes;
class MutablePropertyValue;
class CSSCounterFunction;
class CSSGradientValue;

enum class StrokeLineCap;
enum class StrokeLineJoin;

typedef VectorWithInlineStorage<4, CSSTokenValue, std::allocator<CSSTokenValue>>
    CSSTokenVector;

enum UnitType ENSURE_ENUM_UNSIGNED {
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

// inline | block | list-item | inline-list-item | inline-block | table |
// inline-table | table-row-group | table-header-group | table-footer-group |
// table-row | table-column-group | table-column | table-cell | table-caption |
// flex | inline-flex | none | inherit
enum DisplayValue ENSURE_ENUM_UNSIGNED {
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
    BoxDisplayValue,       // Only for -webkit-box
    InlineBoxDisplayValue, // Only for -webkit-inline-box
    NoneDisplayValue,
};

enum PositionValue ENSURE_ENUM_UNSIGNED {
    StaticPositionValue,
    RelativePositionValue,
    AbsolutePositionValue,
    FixedPositionValue,
};

enum FloatValue ENSURE_ENUM_UNSIGNED {
    NoneFloatValue,
    LeftFloatValue,
    RightFloatValue,
};

enum ClearValue ENSURE_ENUM_UNSIGNED {
    NoneClearValue = 0,
    LeftClearValue = 1,
    RightClearValue = 1 << 1,
    BothClearValue = 1 | (1 << 1),
};

// Only for -webkit-box
enum BoxOrientValue ENSURE_ENUM_UNSIGNED {
    HorizontalBoxOrientValue,
    VerticalBoxOrientValue,
};

// flex
enum FlexDirectionValue ENSURE_ENUM_UNSIGNED {
    RowFlexDirectionValue,
    RowReverseFlexDirectionValue,
    ColumnFlexDirectionValue,
    ColumnReverseFlexDirectionValue,
};

enum FlexWrapValue ENSURE_ENUM_UNSIGNED {
    NoWrapFlexWrapValue,
    WrapFlexWrapValue,
    WrapReverseFlexWrapValue,
};

enum JustifyContentValue ENSURE_ENUM_UNSIGNED {
    NormalJustifyContentValue,

    // Positional alignment
    FlexStartJustifyContentValue,
    FlexEndJustifyContentValue,
    StartJustifyContentValue,
    CenterJustifyContentValue,
    EndJustifyContentValue,

    // Distributed alignment
    SpaceBetweenJustifyContentValue,
    SpaceAroundJustifyContentValue,
    StretchJustifyContentValue,
};

enum AlignItemValue ENSURE_ENUM_UNSIGNED {
    FlexStartAlignItemValue,
    FlexEndAlignItemValue,
    StartAlignItemValue,
    CenterAlignItemValue,
    EndAlignItemValue,
    BaselineAlignItemValue,
    StretchAlignItemValue,
};

enum AlignContentValue ENSURE_ENUM_UNSIGNED {
    FlexStartAlignContentValue,
    FlexEndAlignContentValue,
    CenterAlignContentValue,
    SpaceBetweenAlignContentValue,
    SpaceAroundAlignContentValue,
    StretchAlignContentValue,
};

enum FlexBasisValue ENSURE_ENUM_UNSIGNED {
    AutoFlexBasisValue,
    ContentFlexBasisValue
};

enum VerticalAlignValue ENSURE_ENUM_UNSIGNED {
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

enum TextAlignValue ENSURE_ENUM_UNSIGNED {
    StartTextAlignValue,
    EndTextAlignValue,
    LeftTextAlignValue,
    RightTextAlignValue,
    CenterTextAlignValue,
    WebKitCenterTextAlignValue,
    InternalCenterTextAlignValue
};

// transform-origin, background-position
enum class SideValue ENSURE_ENUM_UNSIGNED {
    NoneSideValue,
    TopSideValue,
    RightSideValue,
    BottomSideValue,
    LeftSideValue,
    CenterSideValue,
    ValueSideValue,
};

enum DirectionValue ENSURE_ENUM_UNSIGNED {
    LtrDirectionValue,
    RtlDirectionValue,
};

// https://drafts.csswg.org/css-backgrounds-3/#typedef-bg-size
enum BackgroundSizeValue ENSURE_ENUM_UNSIGNED {
    CoverBackgroundSizeValue = 1,
    ContainBackgroundSizeValue,
    BackgroundSizeValueEnd = ContainBackgroundSizeValue,
};

// https://drafts.csswg.org/css-backgrounds-3/#typedef-repeat-style
enum RepeatStyleValue ENSURE_ENUM_UNSIGNED {
    RepeatRepeatValue,
    NoRepeatRepeatValue,
    // TODO: space, round
};

// Because padding-box is not supported in box-sizing property, so we make
// another enum.
enum BoxValue ENSURE_ENUM_UNSIGNED {
    BorderBoxBoxValue,
    PaddingBoxBoxValue,
    ContentBoxBoxValue,
};

enum BackgroundAttachmentValue ENSURE_ENUM_UNSIGNED {
    ScrollBackgroundAttachmentValue,
    FixedBackgroundAttachmentValue,
    LocalBackgroundAttachmentValue,
};

enum FontSizeValue ENSURE_ENUM_UNSIGNED {
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

enum WhiteSpaceValue ENSURE_ENUM_UNSIGNED {
    NoWrapWhiteSpaceValue = 1 << 0,  /* Ignore newline characters */
    PreWhiteSpaceValue = 1 << 1,     /* Preserve spaces */
    PreLineWhiteSpaceValue = 1 << 2, /* Wrap lines */
    NormalWhiteSpaceValue = PreLineWhiteSpaceValue | NoWrapWhiteSpaceValue,
    PreWrapWhiteSpaceValue = PreLineWhiteSpaceValue | PreWhiteSpaceValue,
};

enum OverflowValue ENSURE_ENUM_UNSIGNED {
    VisibleOverflow,
    HiddenOverflow,
    AutoOverflow,
    ScrollOverflow,
};

enum PointerEventsValue ENSURE_ENUM_UNSIGNED {
    PointerEventsNoneValue,
    PointerEventsAutoValue,
    PointerEventsVisiblePaintedValue,
    PointerEventsVisibleFillValue,
    PointerEventsVisibleStrokeValue,
    PointerEventsVisibleValue,
    PointerEventsPaintedValue,
    PointerEventsFillValue,
    PointerEventsStrokeValue,
    PointerEventsAllValue,
};

enum BorderImageRepeatValue ENSURE_ENUM_UNSIGNED {
    StretchValue,
    RepeatValue,
    RoundValue,
    SpaceValue,
};

enum BorderShorthandValueType ENSURE_ENUM_UNSIGNED {
    BWidth,
    BStyle,
    BColor,
    BInvalid,
};

enum BorderStyleValue ENSURE_ENUM_UNSIGNED {
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

enum BorderWidthValue ENSURE_ENUM_UNSIGNED {
    ThinBorderWidthValue,
    MediumBorderWidthValue,
    ThickBorderWidthValue,
};

enum BorderCollapseValue ENSURE_ENUM_UNSIGNED {
    SeparateBorderCollapseValue,
    CollapseBorderCollapseValue,
};

enum CaptionSideValue ENSURE_ENUM_UNSIGNED {
    TopCaptionSideValue,
    BottomCaptionSideValue,
};

enum TableLayoutValue ENSURE_ENUM_UNSIGNED {
    AutoTableLayoutValue,
    FixedTableLayoutValue,
};

enum EmptyCellsValue ENSURE_ENUM_UNSIGNED {
    ShowEmptyCellsValue,
    HideEmptyCellsValue,
};

enum TextDecorationLineValue ENSURE_ENUM_UNSIGNED {
    NoneTextDecorationLineValue,
    UnderlineTextDecorationLineValue,
    OverlineTextDecorationLineValue,
    LineThroughTextDecorationLineValue,
    BlinkTextDecorationLineValue,
};

enum TextDecorationStyleValue ENSURE_ENUM_UNSIGNED {
    SolidTextDecorationStyleValue,
    DoubleTextDecorationStyleValue,
    DottedTextDecorationStyleValue,
    DashedTextDecorationStyleValue,
    WavyTextDecorationStyleValue,
};

enum TextUnderlinePositionValue ENSURE_ENUM_UNSIGNED {
    AutoTextUnderlinePositionValue,
    UnderTextUnderlinePositionValue,
    LeftTextUnderlinePositionValue,
    RightTextUnderlinePositionValue,
};

enum FontStyleValue ENSURE_ENUM_UNSIGNED {
    NormalFontStyleValue,
    ItalicFontStyleValue,
    ObliqueFontStyleValue,
};

enum FontWeightValue ENSURE_ENUM_UNSIGNED {
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

enum WordWrapValue ENSURE_ENUM_UNSIGNED {
    NormalWordWrapValue,
    BreakWordWordWrapValue,
    //    BreakSpaceWordWrapValue,
};

enum VisibilityValue ENSURE_ENUM_UNSIGNED {
    VisibleVisibilityValue,
    CollapseVisibilityValue,
    HiddenVisibilityValue,
};

enum UnicodeBidiValue ENSURE_ENUM_UNSIGNED {
    NormalUnicodeBidiValue,
    EmbedUnicodeBidiValue,
    IsolateUnicodeBidiValue,
};

enum ObjectFitValue ENSURE_ENUM_UNSIGNED {
    FillObjectFitValue,
    ContainObjectFitValue,
    CoverObjectFitValue,
    NoneObjectFitValue,
    ScaledownObjectFitValue,
};

enum ImageRenderingValue ENSURE_ENUM_UNSIGNED {
    ImageRenderingAutoValue,
    ImageRenderingCrispEdgesValue,
    ImageRenderingPixelatedValue,
};

enum TimingFunctionValue ENSURE_ENUM_UNSIGNED {
    TimingFunctionEaseValue,
    TimingFunctionLinearValue,
    TimingFunctionEaseInValue,
    TimingFunctionEaseOutValue,
    TimingFunctionEaseInOutValue,
    TimingFunctionStepStartValue,
    TimingFunctionStepEndValue,
};

enum class AnimationDirectionValue ENSURE_ENUM_UNSIGNED {
    Normal,
    Reverse,
    Alternate,
    AlternateReverse,
};

enum class AnimationPlayStateValue ENSURE_ENUM_UNSIGNED {
    Running,
    Paused,
};

enum class AnimationFillModeValue ENSURE_ENUM_UNSIGNED {
    None,
    Forwards,
    Backwards,
    Both,
};

enum BoxSizingValue ENSURE_ENUM_UNSIGNED {
    ContentBoxBoxSizingValue,
    BorderBoxBoxSizingValue
};

enum QuoteValue ENSURE_ENUM_UNSIGNED {
    OpenQuoteValue,
    CloseQuoteValue,
    NoOpenQuoteValue,
    NoCloseQuoteValue,
};

enum FillRuleValue ENSURE_ENUM_UNSIGNED {
    FillRuleNonZero,
    FillRuleEvenOdd,
};

enum TextTransformValue ENSURE_ENUM_UNSIGNED {
    NoneTextTransformValue,
    CapitalizeTextTransformValue,
    UppercaseTextTransformValue,
    LowercaseTextTransformValue,
};

enum ListStylePositionValue ENSURE_ENUM_UNSIGNED {
    ListStylePositionOutside, // Default
    ListStylePositionInside,
};

enum UserSelectValue ENSURE_ENUM_UNSIGNED {
    NoneUserSelectValue,
    TextUserSelectValue,
    ContainUserSelectValue,
    AllUserSelectValue,
};

enum HyphensValue ENSURE_ENUM_UNSIGNED {
    NoneHyphensValue,
    ManualHyphensValue,
};

enum LineBreakValue ENSURE_ENUM_UNSIGNED {
    LooseLineBreakValue,
    NormalLineBreakValue,
    StrictLineBreakValue,
};

enum WordBreakValue ENSURE_ENUM_UNSIGNED {
    NormalWordBreakValue,
    BreakAllWordBreakValue,
    KeepAllWordBreakValue,
    BreakWordWordBreakValue,
};

enum AppearanceValue ENSURE_ENUM_UNSIGNED {
    AutoAppearanceValue,
    NoneAppearanceValue,
};

enum ResizeValue ENSURE_ENUM_UNSIGNED {
    NoneResizeValue,
    BothResizeValue,
    HorizontalResizeValue,
    VerticalResizeValue,
    BlockResizeValue,
    InlineResizeValue,
};

// These values are introduced in CSS basic box model
// (https://www.w3.org/TR/css-sizing-3/). But, it is undergoing changes and many
// parts are not consistent with other modules of CSS.
enum WidthHeightKeywordValue ENSURE_ENUM_UNSIGNED {
    AvailableValue,
    MaxContentValue,
    MinContentValue,
    FitContentValue,
};

enum BoxDecorationBreakValue ENSURE_ENUM_UNSIGNED {
    SliceBoxDecorationBreakValue,
    CloneBoxDecorationBreakValue,
};

enum class Separator {
    None,
    SpaceSeparator,
    CommaSeparator,
    CommaSeparatorAppendQuoteWhenMeetWhiteSpace,
    SlashSeparator
};

enum MaskTypeValue ENSURE_ENUM_UNSIGNED {
    LuminanceMaskTypeValue,
    AlphaMaskTypeValue,
};

class ValueList;
class ValuePair;
class FontFaceSrcData;
class CSSStyleDeclaration;
class CSSFilterFunction;

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
#define FOR_EACH_STYLE_ATTRIBUTE_BASIC(F)                                      \
    F(Color, color, "color")                                                   \
    F(Direction, direction, "direction")                                       \
    F(BackgroundColor, backgroundColor, "background-color")                    \
    F(BackgroundImage, backgroundImage, "background-image")                    \
    F(BackgroundSize, backgroundSize, "background-size")                       \
    F(BackgroundAttachment, backgroundAttachment, "background-attachment")     \
    F(BackgroundClip, backgroundClip, "background-clip")                       \
    F(BackgroundOrigin, backgroundOrigin, "background-origin")                 \
    F(BackgroundRepeatX, backgroundRepeatX, "background-repeat-x")             \
    F(BackgroundRepeatY, backgroundRepeatY, "background-repeat-y")             \
    F(BackgroundPositionX, backgroundPositionX, "background-position-x")       \
    F(BackgroundPositionY, backgroundPositionY, "background-position-y")       \
    F(BoxDecorationBreak, boxDecorationBreak, "box-decoration-break")          \
    F(ColumnGap, columnGap, "column-gap")                                      \
    F(RowGap, rowGap, "row-gap")                                               \
    F(CounterReset, counterReset, "counter-reset")                             \
    F(CounterIncrement, counterIncrement, "counter-increment")                 \
    F(LineHeight, lineHeight, "line-height")                                   \
    F(WhiteSpace, whiteSpace, "white-space")                                   \
    F(WordSpacing, wordSpacing, "word-spacing")                                \
    F(PaddingTop, paddingTop, "padding-top")                                   \
    F(PaddingRight, paddingRight, "padding-right")                             \
    F(PaddingBottom, paddingBottom, "padding-bottom")                          \
    F(PaddingLeft, paddingLeft, "padding-left")                                \
    F(PaddingBlockEnd, paddingPaddingEnd, "padding-block-end")                 \
    F(PaddingBlockStart, paddingPaddingStart, "padding-block-start")           \
    F(PaddingInlineEnd, paddingInlineEnd, "padding-inline-end")                \
    F(PaddingInlineStart, paddingInlineStart, "padding-inline-start")          \
    F(MarginTop, marginTop, "margin-top")                                      \
    F(MarginRight, marginRight, "margin-right")                                \
    F(MarginBottom, marginBottom, "margin-bottom")                             \
    F(MarginLeft, marginLeft, "margin-left")                                   \
    F(MarginBlockStart, marginBlockStart, "margin-block-start")                \
    F(MarginBlockEnd, marginBlockEnd, "margin-block-end")                      \
    F(MarginInlineEnd, marginInlineEnd, "margin-inline-end")                   \
    F(MarginInlineStart, marginInlineStart, "margin-inline-start")             \
    F(Top, top, "top")                                                         \
    F(Bottom, bottom, "bottom")                                                \
    F(Left, left, "left")                                                      \
    F(Right, right, "right")                                                   \
    F(Width, width, "width")                                                   \
    F(Height, height, "height")                                                \
    F(MaxWidth, maxWidth, "max-width")                                         \
    F(MinWidth, minWidth, "min-width")                                         \
    F(MaxHeight, maxHeight, "max-height")                                      \
    F(MinHeight, minHeight, "min-height")                                      \
    F(WordWrap, wordWrap, "word-wrap")                                         \
    F(OverflowWrap, overflowWrap, "overflow-wrap")                             \
    F(Position, position, "position")                                          \
    F(TextDecorationLine, textDecorationLine, "text-decoration-line")          \
    F(TextDecorationColor, textDecorationColor, "text-decoration-color")       \
    F(TextDecorationStyle, textDecorationStyle, "text-decoration-style")       \
    F(TextUnderlinePosition, textUnderlinePosition, "text-underline-position") \
    F(Display, display, "display")                                             \
    F(Float, float, "float")                                                   \
    F(Clear, clear, "clear")                                                   \
    F(BorderImageOutset, borderImageOutset, "border-image-outset")             \
    F(BorderImageRepeat, borderImageRepeat, "border-image-repeat")             \
    F(BorderImageSlice, borderImageSlice, "border-image-slice")                \
    F(BorderImageSource, borderImageSource, "border-image-source")             \
    F(BorderImageWidth, borderImageWidth, "border-image-width")                \
    F(BorderTopColor, borderTopColor, "border-top-color")                      \
    F(BorderRightColor, borderRightColor, "border-right-color")                \
    F(BorderBottomColor, borderBottomColor, "border-bottom-color")             \
    F(BorderLeftColor, borderLeftColor, "border-left-color")                   \
    F(BorderBlockStartColor, borderBlockStartColor,                            \
      "border-block-start-color")                                              \
    F(BorderBlockEndColor, borderBlockEndColor, "border-block-end-color")      \
    F(BorderInlineStartColor, borderInlineStartColor,                          \
      "border-inline-start-color")                                             \
    F(BorderInlineEndColor, borderInlineEndColor, "border-inline-end-color")   \
    F(BorderTopStyle, borderTopStyle, "border-top-style")                      \
    F(BorderRightStyle, borderRightStyle, "border-right-style")                \
    F(BorderBottomStyle, borderBottomStyle, "border-bottom-style")             \
    F(BorderLeftStyle, borderLeftStyle, "border-left-style")                   \
    F(BorderBlockStartStyle, borderBlockStartStyle,                            \
      "border-block-start-style")                                              \
    F(BorderBlockEndStyle, borderBlockEndStyle, "border-block-end-style")      \
    F(BorderInlineStartStyle, borderInlineStartStyle,                          \
      "border-inline-start-style")                                             \
    F(BorderInlineEndStyle, borderInlineEndStyle, "border-inline-end-style")   \
    F(BorderTopWidth, borderTopWidth, "border-top-width")                      \
    F(BorderRightWidth, borderRightWidth, "border-right-width")                \
    F(BorderBottomWidth, borderBottomWidth, "border-bottom-width")             \
    F(BorderLeftWidth, borderLeftWidth, "border-left-width")                   \
    F(BorderBlockStartWidth, borderBlockStartWidth,                            \
      "border-block-start-width")                                              \
    F(BorderBlockEndWidth, borderBlockEndWidth, "border-block-end-width")      \
    F(BorderInlineStartWidth, borderInlineStartWidth,                          \
      "border-inline-start-width")                                             \
    F(BorderInlineEndWidth, borderInlineEndWidth, "border-inline-end-width")   \
    F(BorderCollapse, borderCollapse, "border-collapse")                       \
    F(BorderSpacing, borderSpacing, "border-spacing")                          \
    F(BoxOrient, boxOrient, "box-orient")                                      \
    F(CaptionSide, CaptionSide, "caption-side")                                \
    F(EmptyCells, EmptyCells, "empty-cells")                                   \
    F(TextAlign, textAlign, "text-align")                                      \
    F(TextIndent, textIndent, "text-indent")                                   \
    F(TextShadow, textShadow, "text-shadow")                                   \
    F(TextTransform, textTransform, "text-transform")                          \
    F(Transform, transform, "transform")                                       \
    F(TransformOrigin, transformOrigin, "transform-origin")                    \
    F(Visibility, visibility, "visibility")                                    \
    F(ObjectFit, objectFit, "object-fit")                                      \
    F(ObjectPosition, objectPosition, "object-position")                       \
    F(OverflowX, overflowX, "overflow-x")                                      \
    F(OverflowY, overflowY, "overflow-y")                                      \
    F(ZIndex, zIndex, "z-index")                                               \
    F(VerticalAlign, verticalAlign, "vertical-align")                          \
    F(Opacity, opacity, "opacity")                                             \
    F(TableLayout, tableLayout, "table-layout")                                \
    F(UnicodeBidi, unicodeBidi, "unicode-bidi")                                \
    F(Content, content, "content")                                             \
    F(BoxShadow, boxShadow, "box-shadow")                                      \
    F(BoxSizing, boxSizing, "box-sizing")                                      \
    F(Fill, fill, "fill")                                                      \
    F(StrokeOpacity, strokeOpacity, "stroke-opacity")                          \
    F(FillOpacity, fillOpacity, "fill-opacity")                                \
    F(FillRule, fillRule, "fill-rule")                                         \
    F(Filter, filter, "filter")                                                \
    F(StopColor, stopColor, "stop-color")                                      \
    F(StopOpacity, stopOpacity, "stop-opacity")                                \
    F(Stroke, stroke, "stroke")                                                \
    F(StrokeWidth, strokeWidth, "stroke-width")                                \
    F(StrokeLineCap, strokeLineCap, "stroke-linecap")                          \
    F(StrokeLineJoin, strokeLineJoin, "stroke-linejoin")                       \
    F(StrokeMiterLimit, strokeMiterLimit, "stroke-miterlimit")                 \
    F(StrokeDasharray, strokeDasharray, "stroke-dasharray")                    \
    F(StrokeDashoffset, strokeDashoffset, "stroke-dashoffset")                 \
    F(X, x, "x")                                                               \
    F(Y, y, "y")                                                               \
    F(X1, x1, "x1")                                                            \
    F(Y1, y1, "y1")                                                            \
    F(X2, x2, "x2")                                                            \
    F(Y2, y2, "y2")                                                            \
    F(R, r, "r")                                                               \
    F(RX, rx, "rx")                                                            \
    F(RY, ry, "ry")                                                            \
    F(CX, cx, "cx")                                                            \
    F(CY, cy, "cy")                                                            \
    F(FX, fx, "fx")                                                            \
    F(FY, fy, "fy")                                                            \
    F(FR, fr, "fr")                                                            \
    F(FlexDirection, flexDirection, "flex-direction")                          \
    F(FlexWrap, flexWrap, "flex-wrap")                                         \
    F(Order, order, "order")                                                   \
    F(All, all, "all")                                                         \
    F(JustifyContent, justifyContent, "justify-content")                       \
    F(AlignItems, alignItems, "align-items")                                   \
    F(AlignSelf, alignSelf, "align-self")                                      \
    F(AlignContent, alignContent, "align-content")                             \
    F(JustifyItems, justifyItems, "justify-items")                             \
    F(JustifySelf, justifySelf, "justify-self")                                \
    F(FlexGrow, flexGrow, "flex-grow")                                         \
    F(FlexShrink, flexShrink, "flex-shrink")                                   \
    F(FlexBasis, flexBasis, "flex-basis")                                      \
    F(OutlineColor, outlineColor, "outline-color")                             \
    F(OutlineStyle, outlineStyle, "outline-style")                             \
    F(OutlineWidth, outlineWidth, "outline-width")                             \
    F(OutlineOffset, outlineOffset, "outline-offset")                          \
    F(BorderTopLeftRadius, borderTopLeftRadius, "border-top-left-radius")      \
    F(BorderTopRightRadius, borderTopRightRadius, "border-top-right-radius")   \
    F(BorderBottomRightRadius, borderBottomRightRadius,                        \
      "border-bottom-right-radius")                                            \
    F(BorderBottomLeftRadius, borderBottomLeftRadius,                          \
      "border-bottom-left-radius")                                             \
    F(Cursor, cursor, "cursor")                                                \
    F(MaskImage, maskImage, "mask-image")                                      \
    F(MaskSize, maskSize, "mask-size")                                         \
    F(MaskPositionX, maskPositionX, "mask-position-x")                         \
    F(MaskPositionY, maskPositionY, "mask-position-Y")                         \
    F(MaskRepeatX, maskRepeatX, "mask-repeat-x")                               \
    F(MaskRepeatY, maskRepeatY, "mask-repeat-Y")                               \
    F(MaskType, maskType, "mask-type")                                         \
    F(FontSize, fontSize, "font-size")                                         \
    F(FontWeight, fontWeight, "font-weight")                                   \
    F(FontStyle, fontStyle, "font-style")                                      \
    F(FontKerning, fontKerning, "font-kerning")                                \
    F(ListStylePosition, listStylePosition, "list-style-position")             \
    F(ListStyleImage, listStyleImage, "list-style-image")                      \
    F(ListStyleType, listStyleType, "list-style-type")                         \
    F(Clip, clip, "clip")                                                      \
    F(ClipPath, clipPath, "clip-path")                                         \
    F(LetterSpacing, letterSpacing, "letter-spacing")                          \
    F(UserSelect, userSelect, "user-select")                                   \
    F(GridTemplateColumns, gridTemplateColumns, "grid-template-columns")       \
    F(GridTemplateRows, gridTemplateRows, "grid-template-rows")                \
    F(GridRowStart, gridRowStart, "grid-row-start")                            \
    F(GridRowEnd, gridRowEnd, "grid-row-end")                                  \
    F(GridColumnStart, gridColumnStart, "grid-column-start")                   \
    F(GridColumnEnd, gridColumnEnd, "grid-column-end")                         \
    F(GridGap, gridGap, "grid-gap")                                            \
    F(GridRow, gridRow, "grid-row")                                            \
    F(GridColumn, gridColumn, "grid-column")                                   \
    F(GridTemplateAreas, gridTemplateAreas, "grid-template-areas")             \
    F(GridArea, gridArea, "grid-area")                                         \
    F(CaretColor, caretColor, "caret-color")                                   \
    F(ImageRendering, imageRendering, "image-rendering")                       \
    F(TextOverflow, textOverflow, "text-overflow")                             \
    F(Hyphens, hyphens, "hyphens")                                             \
    F(LineBreak, lineBreak, "line-break")                                      \
    F(WordBreak, wordBreak, "word-break")                                      \
    F(Appearance, appearance, "appearance")                                    \
    F(PointerEvents, pointerEvents, "pointer-events")                          \
    F(Resize, resize, "resize")                                                \
    F(WillChange, willChange, "will-change")                                   \
    F(LineClamp, lineClamp, "line-clamp")                                      \
    F(MixBlendMode, mixBlendMode, "mix-blend-mode")

// font related properties must be followed end of this
// define(FOR_EACH_STYLE_ATTRIBUTE)
// This order is used by CSSParser::parseFontFaceRule

#define FOR_EACH_STYLE_ATTRIBUTE_STICKY(F)                            \
    F(D, d, "d")                                                      \
    F(FontFamily, fontFamily, "font-family")                          \
    F(Src, src, "src")                                                \
    F(TransitionDelay, transitionDelay, "transition-delay")           \
    F(TransitionDuration, transitionDuration, "transition-duration")  \
    F(TransitionProperty, transitionProperty, "transition-property")  \
    F(TransitionTimingFunction, transitionTimingFunction,             \
      "transition-timing-function")                                   \
    F(AnimationName, animationName, "animation-name")                 \
    F(AnimationDuration, animationDuration, "animation-duration")     \
    F(AnimationTimingFunction, animationTimingFunction,               \
      "animation-timing-function")                                    \
    F(AnimationDelay, animationDelay, "animation-delay")              \
    F(AnimationIterationCount, animationIterationCount,               \
      "animation-iteration-count")                                    \
    F(AnimationDirection, animationDirection, "animation-direction")  \
    F(AnimationPlayState, animationPlayState, "animation-play-state") \
    F(AnimationFillMode, animationFillMode, "animation-fill-mode")

#define FOR_EACH_STYLE_ATTRIBUTE_SHORTHAND(F)                        \
    F(Border, border, "border")                                      \
    F(BorderTop, borderTop, "border-top")                            \
    F(BorderRight, borderRight, "border-right")                      \
    F(BorderBottom, borderBottom, "border-bottom")                   \
    F(BorderLeft, borderLeft, "border-left")                         \
    F(BorderImage, borderImage, "border-image")                      \
    F(BorderStyle, borderStyle, "border-style")                      \
    F(BorderWidth, borderWidth, "border-width")                      \
    F(BorderColor, borderColor, "border-color")                      \
    F(BorderRadius, borderRadius, "border-radius")                   \
    F(BorderBlockStart, borderBlockStart, "border-block-start")      \
    F(BorderBlockEnd, borderBlockEnd, "border-block-end")            \
    F(BorderInlineStart, borderInlineStart, "border-inline-start")   \
    F(BorderInlineEnd, BorderInlineEnd, "border-inline-end")         \
    F(Background, background, "background")                          \
    F(BackgroundRepeat, backgroundRepeat, "background-repeat")       \
    F(BackgroundPosition, backgroundPosition, "background-position") \
    F(Gap, gap, "gap")                                               \
    F(GridTemplate, gridTemplate, "grid-template")                   \
    F(Inset, inset, "inset")                                         \
    F(TextDecoration, textDecoration, "text-decoration")             \
    F(Margin, margin, "margin")                                      \
    F(MarginBlock, marginBlock, "margin-block")                      \
    F(MarginInline, marginInline, "margin-inline")                   \
    F(Padding, padding, "padding")                                   \
    F(PaddingBlock, paddingBlock, "padding-block")                   \
    F(PaddingInline, paddingInline, "padding-inline")                \
    F(Font, font, "font")                                            \
    F(Outline, outline, "outline")                                   \
    F(Overflow, overflow, "overflow")                                \
    F(Transition, transition, "transition")                          \
    F(Animation, animation, "animation")                             \
    F(FlexFlow, flexFlow, "flex-flow")                               \
    F(Flex, flex, "flex")                                            \
    F(ListStyle, listStyle, "list-style")                            \
    F(Mask, mask, "mask")                                            \
    F(MaskPosition, maskPosition, "mask-position")                   \
    F(MaskRepeat, maskRepeat, "mask-repeat")

#define FOR_EACH_STYLE_ATTRIBUTE_TOTAL(F) \
    FOR_EACH_STYLE_ATTRIBUTE_BASIC(F)     \
    FOR_EACH_STYLE_ATTRIBUTE_STICKY(F)    \
    FOR_EACH_STYLE_ATTRIBUTE_SHORTHAND(F)

#define GEN_FOURSIDE(F) \
    F(Top, top)         \
    F(Right, right)     \
    F(Bottom, bottom)   \
    F(Left, left)

class CSSTransformFunction : public gc {
public:
    enum Kind {
        None,
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

    bool operator==(const CSSTransformFunction& src);
    bool operator!=(const CSSTransformFunction& src)
    {
        return !operator==(src);
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
        case None:
            return String::emptyString;
        }
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

protected:
    Kind m_kind;
    ValueList* m_values;
};

class CSSTransformFunctions : public GCVector<CSSTransformFunction> {
public:
    void toTransformDataGroup(Element* element, ComputedStyle* style);
    String* toString();

    bool equals(CSSTransformFunctions* src)
    {
        if (size() != src->size()) {
            return false;
        }

        for (size_t i = 0; i < size(); i++) {
            if (at(i) != src->at(i)) {
                return false;
            }
        }

        return true;
    }
};

class CSSStyleValuePair : public gc {
    friend class ValueList;
    friend class ValuePair;

public:
    enum KeyKind ENSURE_ENUM_UNSIGNED {
        Unknown,
#define ADD_CSS_KEYKIND(Name, ...) Name,
        FOR_EACH_STYLE_ATTRIBUTE_TOTAL(ADD_CSS_KEYKIND)
#undef ADD_CSS_KEYKIND
            CustomProperty,
        KeyKindSize,
    };
    // font related properties must be followed end of this enum(KeyKind)
    // This order is used by CSSParser::parseFontFaceRule

    enum ValueKind ENSURE_ENUM_UNSIGNED {
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
        Angle,
        Time,
        Normal,
        StringValueKind,
        AtomicStringValueKind,
        KeywordValueKind,
        ColorValueKind,
        NamedColorValueKind,
        UrlValueKind,
        PathFunctionValueKind,
        CSSPropertyNameValueKind,
        FilterFunctionValueKind,

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
        RepeatStyleValueKind,
        BackgroundAttachmentValueKind,
        BoxValueKind,

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
        TextDecorationStyleValueKind,
        TextUnderlinePositionValueKind,
        ResizeValueKind,
        VisibilityValueKind,
        UnicodeBidiValueKind,
        BoxSizingValueKind,
        BoxDecorationBreakValueKind,

        // Only for -webkit-box-orient
        BoxOrientValueKind,

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
        TimingFunctionValueKind,
        TimingFunctionPointerKind,

        // animation
        AnimationDirectionValueKind,
        AnimationPlayStateValueKind,
        AnimationFillModeValueKind,

        // content
        Attr,
        QuoteValueKind,

        // svg
        FillRuleValueKind,

        // text-transform
        TextTransformValueKind,

        // object-fit
        ObjectFitValueKind,

        ListStylePositionValueKind,
        CounterFunctionValueKind,

        // rect for clip
        RectValueKind,

        // user-select
        UserSelectValueKind,

        // grid
        GridTemplateUnits,
        GridTemplateAreasValueKind,

        // img
        ImageRenderingValueKind,

        // text-overflow
        TextOverflowValueKind,

        HyphensValueKind,
        LineBreakValueKind,
        WordBreakValueKind,
        AppearanceValueKind,
        VarFunctionValueKind,

        // gradient
        GradientValueKind,

        // keyword value for width and height
        WidthHeightKeywordValueKind,

        // pointer-events
        PointerEventsValueKind,

        // mask
        MaskTypeValueKind,

        // stroke-linecap
        StrokeLineCapValueKind,

        // stroke-linejoin
        StrokeLineJoinValueKind,

        // mix-blend-mode
        BlendModeValueKind,

        // AnimateMotion
        AnimateMotionValueKind
    };

    enum class TransformUnit {
        Number,           // <number>
        Angle,            // <angle>
        TranslationValue, // <translation-value>: percentage or length
        Length            // <length>: length
    };

    CSSStyleValuePair()
        : m_keyKind(KeyKind::Unknown)
        , m_valueKind(ValueKind::None)
        , m_temporaryValueKind(ValueKind::None)
        , m_flagImportant(false)
        , m_value(0.0f)
    {
    }

    CSSStyleValuePair(const CSSStyleValuePair& o)
        : m_keyKind(o.m_keyKind)
        , m_valueKind(o.m_valueKind)
        , m_temporaryValueKind(o.m_temporaryValueKind)
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

    String* keyName() const;

    ValueKind valueKind() const
    {
        return m_valueKind;
    }

    void setValueKind(ValueKind kind)
    {
        m_valueKind = kind;
    }

    ValueKind temporaryValueKind() const
    {
        return m_temporaryValueKind;
    }

    void setTemporaryValueKind(ValueKind kind)
    {
        m_temporaryValueKind = kind;
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

    bool updateValueVarReferences(const CSSTokenVector& tokens);

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

    QuoteValue quoteValue() const
    {
        STARFISH_ASSERT(m_valueKind == QuoteValueKind);
        return m_value.m_quote;
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

    ::Starfish::Length lengthValue() const
    {
        if (m_valueKind == Length) {
            return m_value.m_length.toLength();
        } else {
            STARFISH_ASSERT(m_valueKind == CalcValueKind);
            return ::Starfish::Length(calcValue());
        }
    }

    ::Starfish::Length toLengthValue() const
    {
        if (m_valueKind == Length) {
            return m_value.m_length.toLength();
        } else if (m_valueKind == Percentage) {
            return ::Starfish::Length(::Starfish::Length::Percent,
                                      percentageValue());
        } else {
            STARFISH_ASSERT(m_valueKind == CalcValueKind);
            return ::Starfish::Length(calcValue());
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
        m_valueKind = StringValueKind;
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

    RepeatStyleValue repeatStyleValue() const
    {
        STARFISH_ASSERT(m_valueKind == RepeatStyleValueKind);
        return m_value.m_repeatStyle;
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
        // TODO: shorthand not supported yet
        return textDecorationLineValue();
    }

    TextDecorationLineValue textDecorationLineValue() const
    {
        STARFISH_ASSERT(m_valueKind == TextDecorationLineValueKind);
        return m_value.m_textDecorationLine;
    }

    TextDecorationStyleValue textDecorationStyleValue() const
    {
        STARFISH_ASSERT(m_valueKind == TextDecorationStyleValueKind);
        return m_value.m_textDecorationStyle;
    }

    TextUnderlinePositionValue textUnderlinePositionValue() const
    {
        STARFISH_ASSERT(m_valueKind == TextUnderlinePositionValueKind);
        return m_value.m_textUnderlinePosition;
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

    KeyKind cssPropertyNameValue() const
    {
        STARFISH_ASSERT(m_valueKind == CSSPropertyNameValueKind);
        return m_value.m_cssPropertyNameValue;
    }

    TimingFunctionValue timingFunctionValue() const
    {
        STARFISH_ASSERT(m_valueKind == TimingFunctionValueKind);
        return m_value.m_timingFunctionValue;
    }

    AnimationDirectionValue animationDirectionValue() const
    {
        STARFISH_ASSERT(m_valueKind == AnimationDirectionValueKind);
        return m_value.m_animationDirectionValue;
    }

    AnimationPlayStateValue animationPlayStateValue() const
    {
        STARFISH_ASSERT(m_valueKind == AnimationPlayStateValueKind);
        return m_value.m_animationPlayStateValue;
    }

    AnimationFillModeValue animationFillModeValue() const
    {
        STARFISH_ASSERT(m_valueKind == AnimationFillModeValueKind);
        return m_value.m_animationFillModeValue;
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

    BoxOrientValue boxOrientValue() const
    {
        STARFISH_ASSERT(m_valueKind == BoxOrientValueKind);
        return m_value.m_boxOrient;
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

    String* clipPath() const
    {
        STARFISH_ASSERT(m_valueKind == StringValueKind);
        return m_value.m_stringValue;
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

    LineBreakValue lineBreakValue() const
    {
        STARFISH_ASSERT(m_valueKind == LineBreakValueKind);
        return m_value.m_lineBreak;
    }

    WordBreakValue wordBreakValue() const
    {
        STARFISH_ASSERT(m_valueKind == WordBreakValueKind);
        return m_value.m_wordBreak;
    }

    AppearanceValue appearanceValue() const
    {
        STARFISH_ASSERT(m_valueKind == AppearanceValueKind);
        return m_value.m_appearance;
    }

    ResizeValue resizeValue() const
    {
        STARFISH_ASSERT(m_valueKind == ResizeValueKind);
        return m_value.m_resize;
    }

    GCVector<GridTrackSize*>* gridTemplateUnits() const
    {
        STARFISH_ASSERT(m_valueKind == GridTemplateUnits);
        return m_value.m_gridTemplateUnits;
    }

    NamedGridAreaDataMap* gridTemplateAreas() const
    {
        STARFISH_ASSERT(m_valueKind == GridTemplateAreasValueKind);
        return m_value.m_gridTemplateAreas;
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

    CSSGradientValue* gradientValue() const
    {
        STARFISH_ASSERT(m_valueKind == GradientValueKind);
        return m_value.m_gradientValue;
    }

    WidthHeightKeywordValue widthHeightKeywordValue() const
    {
        STARFISH_ASSERT(m_valueKind == WidthHeightKeywordValueKind);
        return m_value.m_widthHeightKeywordValue;
    }

    PointerEventsValue pointerEventsValue() const
    {
        STARFISH_ASSERT(m_valueKind == PointerEventsValueKind);
        return m_value.m_pointerEventsValue;
    }

    BlendMode blendModeValue() const
    {
        STARFISH_ASSERT(m_valueKind == BlendModeValueKind);
        return m_value.m_blendMode;
    }

    BoxDecorationBreakValue boxDecorationBreakValue() const
    {
        STARFISH_ASSERT(m_valueKind == BoxDecorationBreakValueKind);
        return m_value.m_boxDecorationBreakValue;
    }

    String* pathFunctionValue() const
    {
        STARFISH_ASSERT(m_valueKind == PathFunctionValueKind);
        return m_value.m_stringValue;
    }

    TimingFunction* timingFunctionPointerValue() const
    {
        STARFISH_ASSERT(m_valueKind == TimingFunctionPointerKind);
        return m_value.m_timingFunction;
    }

    CSSFilterFunction* filterFunctionValue() const
    {
        STARFISH_ASSERT(m_valueKind == FilterFunctionValueKind);
        return m_value.m_filterFunction;
    }

    MaskTypeValue maskTypeValue() const
    {
        STARFISH_ASSERT(m_valueKind == MaskTypeValueKind);
        return m_value.m_maskType;
    }

    ::Starfish::StrokeLineCap strokeLineCap() const
    {
        STARFISH_ASSERT(m_valueKind == StrokeLineCapValueKind);
        return m_value.m_strokeLineCap;
    }

    ::Starfish::StrokeLineJoin strokeLineJoin() const
    {
        STARFISH_ASSERT(m_valueKind == StrokeLineJoinValueKind);
        return m_value.m_strokeLineJoin;
    }

    GCAtomicVector<Unit::FloatPoint>* animateMotion() const
    {
        STARFISH_ASSERT(m_valueKind == AnimateMotionValueKind);
        return m_value.m_animateMotion;
    }

    bool valueEquals(const CSSStyleValuePair& src) const;
    bool operator==(const CSSStyleValuePair& src) const;
    bool operator!=(const CSSStyleValuePair& src) const
    {
        return !operator==(src);
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
        RepeatStyleValue m_repeatStyle;
        BackgroundAttachmentValue m_backgroundAttachment;
        QuoteValue m_quote;
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
        TextDecorationLineValue m_textDecorationLine;
        TextDecorationStyleValue m_textDecorationStyle;
        TextUnderlinePositionValue m_textUnderlinePosition;
        ResizeValue m_resize;
        CSSTransformFunctions* m_transforms;
        Unit::Color m_color;
        NamedColor::NamedColorValue m_namedColor;
        BorderCollapseValue m_borderCollapse;
        CaptionSideValue m_captionSide;
        TableLayoutValue m_tableLayout;
        EmptyCellsValue m_emptyCells;
        KeyKind m_cssPropertyNameValue;
        TimingFunctionValue m_timingFunctionValue;
        AnimationDirectionValue m_animationDirectionValue;
        AnimationPlayStateValue m_animationPlayStateValue;
        AnimationFillModeValue m_animationFillModeValue;
        BoxSizingValue m_boxSizing;
        CSSTime m_time;
        BoxOrientValue m_boxOrient;
        FlexDirectionValue m_flexDirection;
        FlexWrapValue m_flexWrap;
        JustifyContentValue m_justifyContent;
        AlignItemValue m_alignItem;
        AlignContentValue m_alignContent;
        FlexBasisValue m_flexBasis;
        FillRuleValue m_fillRule;
        CalcData* m_calc;
        TextTransformValue m_textTransform;
        ObjectFitValue m_objectFit;
        ListStylePositionValue m_listStylePosition;
        UserSelectValue m_userSelect;
        HyphensValue m_hyphens;
        LineBreakValue m_lineBreak;
        WordBreakValue m_wordBreak;
        AppearanceValue m_appearance;
        RectData* m_rect;
        GCVector<GridTrackSize*>* m_gridTemplateUnits;
        NamedGridAreaDataMap* m_gridTemplateAreas;
        CSSCounterFunction* m_counterFunctionValue;
        TextOverflowData* m_textOverflowData;
        CSSGradientValue* m_gradientValue;
        WidthHeightKeywordValue m_widthHeightKeywordValue;
        PointerEventsValue m_pointerEventsValue;
        BlendMode m_blendMode;
        BoxDecorationBreakValue m_boxDecorationBreakValue;
        TimingFunction* m_timingFunction;
        CSSFilterFunction* m_filterFunction;
        MaskTypeValue m_maskType;
        ::Starfish::StrokeLineCap m_strokeLineCap;
        ::Starfish::StrokeLineJoin m_strokeLineJoin;
        GCAtomicVector<Unit::FloatPoint>* m_animateMotion;

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
        ValueData(RepeatStyleValue v)
            : m_repeatStyle(v)
        {
        }
        ValueData(BackgroundAttachmentValue v)
            : m_backgroundAttachment(v)
        {
        }
        ValueData(QuoteValue v)
            : m_quote(v)
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
            : m_textDecorationLine(v)
        {
        }
        ValueData(TextDecorationStyleValue v)
            : m_textDecorationStyle(v)
        {
        }
        ValueData(TextUnderlinePositionValue v)
            : m_textUnderlinePosition(v)
        {
        }
        ValueData(ResizeValue v)
            : m_resize(v)
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
        ValueData(KeyKind v)
            : m_cssPropertyNameValue(v)
        {
        }
        ValueData(CSSTime v)
            : m_time(v)
        {
        }
        ValueData(TimingFunctionValue v)
            : m_timingFunctionValue(v)
        {
        }
        ValueData(AnimationDirectionValue v)
            : m_animationDirectionValue(v)
        {
        }
        ValueData(AnimationPlayStateValue v)
            : m_animationPlayStateValue(v)
        {
        }
        ValueData(AnimationFillModeValue v)
            : m_animationFillModeValue(v)
        {
        }
        ValueData(BoxSizingValue v)
            : m_boxSizing(v)
        {
        }
        ValueData(BoxOrientValue v)
            : m_boxOrient(v)
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

        ValueData(LineBreakValue v)
            : m_lineBreak(v)
        {
        }

        ValueData(WordBreakValue v)
            : m_wordBreak(v)
        {
        }

        ValueData(AppearanceValue v)
            : m_appearance(v)
        {
        }

        ValueData(RectData* v)
            : m_rect(v)
        {
        }

        ValueData(GCVector<GridTrackSize*>* v)
            : m_gridTemplateUnits(v)
        {
        }

        ValueData(NamedGridAreaDataMap* v)
            : m_gridTemplateAreas(v)
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

        ValueData(CSSGradientValue* v)
            : m_gradientValue(v)
        {
        }

        ValueData(WidthHeightKeywordValue v)
            : m_widthHeightKeywordValue(v)
        {
        }

        ValueData(PointerEventsValue v)
            : m_pointerEventsValue(v)
        {
        }

        ValueData(BlendMode v)
            : m_blendMode(v)
        {
        }

        ValueData(BoxDecorationBreakValue v)
            : m_boxDecorationBreakValue(v)
        {
        }

        ValueData(TimingFunction* v)
            : m_timingFunction(v)
        {
            STARFISH_ASSERT(v != nullptr);
        }

        ValueData(CSSFilterFunction* v)
            : m_filterFunction(v)
        {
        }

        ValueData(MaskTypeValue v)
            : m_maskType(v)
        {
        }

        ValueData(GCAtomicVector<Unit::FloatPoint>* v)
            : m_animateMotion(v)
        {
        }
    };

    CSSStyleValuePair(ValueKind kind, ValueData value)
        : m_keyKind(KeyKind::Unknown)
        , m_valueKind(kind)
        , m_temporaryValueKind(ValueKind::None)
        , m_flagImportant(false)
        , m_value(value)
    {
    }

    void* toPointerValueIfPossible() const;
    void rootPointerValue(GCVector<void*>& rooter) const;
    void unrootPointerValue(GCVector<void*>& rooter) const;

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

    void setTextDecorationLineValue(TextDecorationLineValue v)
    {
        m_valueKind = CSSStyleValuePair::ValueKind::TextDecorationLineValueKind;
        m_value.m_textDecorationLine = v;
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

    void setRepeatStyleValue(RepeatStyleValue val)
    {
        m_valueKind = CSSStyleValuePair::ValueKind::RepeatStyleValueKind;
        m_value.m_repeatStyle = val;
    }

    void setBackgroundAttachmentValue(BackgroundAttachmentValue val)
    {
        m_valueKind =
            CSSStyleValuePair::ValueKind::BackgroundAttachmentValueKind;
        m_value.m_backgroundAttachment = val;
    }

    void setQuoteValue(QuoteValue val)
    {
        m_valueKind = CSSStyleValuePair::ValueKind::QuoteValueKind;
        m_value.m_quote = val;
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

    void setGridTemplateUnits(GCVector<GridTrackSize*>* val)
    {
        m_valueKind = CSSStyleValuePair::ValueKind::GridTemplateUnits;
        m_value.m_gridTemplateUnits = val;
    }

    void setGridTemplateAreas(NamedGridAreaDataMap* val)
    {
        m_valueKind = CSSStyleValuePair::ValueKind::GridTemplateAreasValueKind;
        m_value.m_gridTemplateAreas = val;
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

    void setGradientValue(CSSGradientValue* val)
    {
        m_valueKind = CSSStyleValuePair::GradientValueKind;
        m_value.m_gradientValue = val;
    }

    void setBoxDecorationBreakValue(BoxDecorationBreakValue v)
    {
        m_valueKind = BoxDecorationBreakValueKind;
        m_value.m_boxDecorationBreakValue = v;
    }

    void setPathFunctionValue(String* v)
    {
        m_valueKind = PathFunctionValueKind;
        m_value.m_stringValue = v;
    }

    void setCSSPropertyNameValue(KeyKind v)
    {
        m_valueKind = CSSPropertyNameValueKind;
        m_value.m_cssPropertyNameValue = v;
    }

    void setTimingFunctionValue(TimingFunctionValue v)
    {
        m_valueKind = TimingFunctionValueKind;
        m_value.m_timingFunctionValue = v;
    }

    void setTimingFunctionPointerValue(TimingFunction* v)
    {
        STARFISH_ASSERT(v != nullptr);
        m_valueKind = TimingFunctionPointerKind;
        m_value.m_timingFunction = v;
    }

    void setAnimationDirectionValue(AnimationDirectionValue v)
    {
        m_valueKind = AnimationDirectionValueKind;
        m_value.m_animationDirectionValue = v;
    }

    void setAnimationPlayStateValue(AnimationPlayStateValue v)
    {
        m_valueKind = AnimationPlayStateValueKind;
        m_value.m_animationPlayStateValue = v;
    }

    void setAnimationFillModeValue(AnimationFillModeValue v)
    {
        m_valueKind = AnimationFillModeValueKind;
        m_value.m_animationFillModeValue = v;
    }

    void setFilterFunctionValue(CSSFilterFunction* v)
    {
        m_valueKind = FilterFunctionValueKind;
        m_value.m_filterFunction = v;
    }

    void setTransformFunctionsValue(CSSTransformFunctions* transforms)
    {
        m_valueKind = TransformFunctions;
        m_value.m_transforms = transforms;
    }

    void setMaskTypeValue(MaskTypeValue maskType)
    {
        m_valueKind = MaskTypeValueKind;
        m_value.m_maskType = maskType;
    }

    void setStrokeLineCapValue(::Starfish::StrokeLineCap c)
    {
        m_valueKind = StrokeLineCapValueKind;
        m_value.m_strokeLineCap = c;
    }

    void setStrokeLineJoinValue(::Starfish::StrokeLineJoin c)
    {
        m_valueKind = StrokeLineJoinValueKind;
        m_value.m_strokeLineJoin = c;
    }

    void setAnimateMotionValue(GCAtomicVector<Unit::FloatPoint>* c)
    {
        m_valueKind = AnimateMotionValueKind;
        m_value.m_animateMotion = c;
    }

    bool updateValueForAttributeBasic(Document* document,
                                      CSSStyleValuePair::KeyKind keyKind,
                                      const CSSTokenVector& tokens);
#define NEW_SET_VALUE_DECL(name, ...) \
    bool updateValue##name(Document* document, const CSSTokenVector& tokens);
    FOR_EACH_STYLE_ATTRIBUTE_BASIC(NEW_SET_VALUE_DECL)
#undef NEW_SET_VALUE_DECL
    bool updateValueFontFamily(const CSSTokenVector& tokens);
    bool updateValueSrc(const CSSTokenVector& tokens);

    bool updateValueNumber(const CSSTokenVector& tokens, uint8_t option);
    bool updateValueUnitNumber(const CSSTokenValue& token, uint8_t option);
    enum CalcParserOption {
        LengthParser = 0,
        AngleParser = 1,
        TimeParser = 2,
        LineheightParser = 3
    };
    bool updateValueUnitCalc(const CSSTokenValue& token, uint8_t parserOption,
                             uint8_t lengthOption);
    bool updateValueLength(const CSSTokenVector& tokens, uint8_t option);
    bool updateValueUnitLength(const CSSTokenValue& token, uint8_t option);
    bool updateValueUnitLengthOrCalc(const CSSTokenValue& token,
                                     uint8_t option);
    bool updateValueWidthHeightKeyword(const CSSTokenVector& tokens);
    bool updateValueUnitWidthHeightKeyword(const CSSTokenValue& value);
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
    bool updateValueBorderRadius(const CSSTokenVector& tokens);
    bool updateValueBox(const CSSTokenVector& tokens, bool allowComma);
    bool updateValueUnitRepeatStyle(const CSSTokenValue& token);
    bool updateValueUnitPositionX(const CSSTokenValue& token);
    bool updateValueUnitPositionY(const CSSTokenValue& token);
    bool updateValueUnitBackgroundAttachment(const CSSTokenValue& token);
    bool updateValueUnitBorderStyle(const CSSTokenValue& token);
    bool updateValueUnitBorderWidth(const CSSTokenValue& token);
    bool updateValueUnitBorderColor(const CSSTokenValue& token);
    bool updateValueUnitBorderImageSource(const CSSTokenValue& token);
    bool updateValueUnitBorderImageSlice(const CSSTokenVector& tokens);
    bool updateValueUnitBorderImageWidth(const CSSTokenVector& tokens);
    bool updateValueUnitBorderImageOutset(const CSSTokenVector& tokens);
    bool updateValueUnitBorderImageRepeat(const CSSTokenVector& tokens);
    bool updateValueUnitColor(const CSSTokenValue& token);
    bool updateValueUnitUrlOrNone(const CSSTokenValue& token);
    bool updateValueUnitGradient(const CSSTokenValue& value);
    bool updateValueUnitMargin(const CSSTokenValue& token);
    bool updateValueUnitPadding(const CSSTokenValue& token);
    bool updateValueUnitInset(const CSSTokenValue& token);
    bool updateValueUnitFilterFunction(const CSSTokenValue& token);
    bool updateValueUnitFontSize(const CSSTokenValue& token);
    bool updateValueUnitFontStyle(const CSSTokenValue& token);
    bool updateValueUnitFontWeight(const CSSTokenValue& token);
    bool updateValueUnitWordWrap(const CSSTokenValue& token);
    bool updateValueUnitLineHeight(const CSSTokenValue& token);
    bool updateValueUnitListStyleImage(const CSSTokenValue& value);
    bool updateValueUnitListStylePosition(const CSSTokenValue& value);
    bool updateValueUnitListStyleType(Document* document,
                                      const CSSTokenValue& value);
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
    bool updateValueUnitImageValue(const CSSTokenValue& value);
    bool updateValueUnitGap(const CSSTokenValue& token);

    bool updateValueTransform(const CSSTokenVector& tokens, bool canIgnoreUnit,
                              Separator sep = Separator::CommaSeparator);
    bool updateValueTransformFunction(const CSSTokenValue& transformValue,
                                      CSSTransformFunction::Kind fkind,
                                      bool canIgnoreUnit, ValueList* values);
    bool addTransformValueToList(const CSSTokenVector& transformValueTokens,
                                 CSSTokenVector& transformValueList);
    bool updateValueTransformOrigin(const CSSTokenVector& tokens,
                                    bool canIgnoreUnit);
    bool updateValueObjectPosition(const CSSTokenVector& tokens,
                                   CSSStyleValuePair& xPair,
                                   CSSStyleValuePair& yPair);
    bool updateValueShadow(const CSSTokenVector& tokens, bool boxShadow);
    bool updateValueLayerTransitionProperty(const CSSTokenVector& tokens);
    bool updateValueLayerTransitionDuration(const CSSTokenVector& tokens);
    bool updateValueLayerTransitionTimingFunction(const CSSTokenVector& tokens);
    bool updateValueLayerTransitionDelay(const CSSTokenVector& tokens);

    bool updateValueLayerAnimationName(const CSSTokenVector& tokens);
    bool updateValueLayerAnimationDuration(const CSSTokenVector& tokens);
    bool updateValueLayerAnimationTimingFunction(const CSSTokenVector& tokens);
    bool updateValueLayerAnimationDelay(const CSSTokenVector& tokens);
    bool updateValueLayerAnimationIterationCount(const CSSTokenVector& tokens);
    bool updateValueLayerAnimationDirection(const CSSTokenVector& tokens);
    bool updateValueLayerAnimationPlayState(const CSSTokenVector& tokens);
    bool updateValueLayerAnimationFillMode(const CSSTokenVector& tokens);

    bool updateValueUnitAnimationName(const CSSTokenValue& value);
    bool updateValueUnitAnimationTimingFunction(const CSSTokenValue& value);
    bool updateValueUnitAnimationIterationCount(const CSSTokenValue& value);
    bool updateValueUnitAnimationDirection(const CSSTokenValue& value);
    bool updateValueUnitAnimationPlayState(const CSSTokenValue& value);
    bool updateValueUnitAnimationFillMode(const CSSTokenValue& value);

    bool updateValueMaskImage(const CSSTokenVector& tokens, bool allowComma);
    bool updateValueMaskType(const CSSTokenVector& tokens, bool allowComma);

    bool updateValueUnitFourSidedShorthandProperty(
        CSSStyleValuePair::KeyKind keyKind, const CSSTokenValue& token);

protected:
    KeyKind m_keyKind : 8;
    ValueKind m_valueKind : 8;
    ValueKind m_temporaryValueKind : 8;
    bool m_flagImportant : 1;
    ValueData m_value;

    bool updateTransformUnit(CSSTransformFunction::Kind fkind,
                             TransformUnit units[16], int& minArgCnt,
                             int& maxArgCnt);
};

class CSSStyleValuePairVectorHolder : public gc {
public:
    void push_back(const CSSStyleValuePair& p)
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
    void rootPointer(const CSSStyleValuePair& v)
    {
        v.rootPointerValue(m_pointerRooter);
    }
    GCAtomicVector<CSSStyleValuePair> m_data;
    GCVector<void*> m_pointerRooter;
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
        return GC_MALLOC_ATOMIC(size);
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

    bool equals(ValuePair* src)
    {
        return m_first == src->m_first && m_second == src->m_second;
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

class ValueList : public GCAtomicVector<CSSStyleValuePair> {
public:
    ValueList()
        : GCAtomicVector<CSSStyleValuePair>()
        , m_separator(Separator::None)
    {
    }

    ValueList(Separator sep)
        : GCAtomicVector<CSSStyleValuePair>()
        , m_separator(sep)
    {
    }

    ValueList(const ValueList& src)
        : GCAtomicVector<CSSStyleValuePair>(src)
        , m_separator(src.m_separator)
        , m_pointerRooter(src.m_pointerRooter)
    {
    }

    ValueList(ValueList&& src)
        : GCAtomicVector<CSSStyleValuePair>(src)
        , m_separator(src.m_separator)
        , m_pointerRooter(std::move(src.m_pointerRooter))
    {
    }

    void push_back(const CSSStyleValuePair& p)
    {
        GCAtomicVector<CSSStyleValuePair>::push_back(p);
        rootPointer(p);
    }

    void pushBack(const CSSStyleValuePair& p)
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

    bool equalsTextDecorationLine(ValueList* list)
    {
        if (size() != list->size()) {
            return false;
        }

        for (size_t i = 0; i < size(); i++) {
            if (at(i).textDecorationLineValue() !=
                list->at(i).textDecorationLineValue()) {
                return false;
            }
        }

        return true;
    }

    bool equals(ValueList* list)
    {
        if (size() != list->size()) {
            return false;
        }

        for (size_t i = 0; i < size(); i++) {
            if (at(i) != list->at(i)) {
                return false;
            }
        }

        return true;
    }

    String* toString()
    {
        StringBuilder builder;
        size_t len = size();
        for (size_t i = 0; i < len; i++) {
            String* src = at(i).toString();
            if (m_separator ==
                    Separator::CommaSeparatorAppendQuoteWhenMeetWhiteSpace &&
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

    Separator separator()
    {
        return m_separator;
    }

protected:
    String* separatorString()
    {
        if (m_separator == Separator::None) {
            return String::emptyString;
        } else if (m_separator == Separator::SpaceSeparator) {
            return String::spaceString;
        } else if (m_separator == Separator::CommaSeparator) {
            return String::createASCIIString(", ");
        } else if (m_separator ==
                   Separator::CommaSeparatorAppendQuoteWhenMeetWhiteSpace) {
            return String::createASCIIString(", ");
        } else {
            return String::createASCIIString("/ ");
        }
    }

    void rootPointer(CSSStyleValuePair v)
    {
        v.rootPointerValue(m_pointerRooter);
    }

    Separator m_separator;
    GCVector<void*> m_pointerRooter;
};

class CSSSelector;
class CSSAttributeSelector;
class CSSNamespacedTagSelector;
class CSSPseudoSelector;

struct CSSSelectorListItem {
    enum RelationType ENSURE_ENUM_UNSIGNED {
        None,
        SubSelector,     // No combinator
        Descendant,      // "Space" combinator
        Child,           // > combinator
        AdjacentSibling, // + combinator
        GeneralSibling   // ~ combinator
    };

    bool m_relationIsAffectedByPseudoContent : 1;
    RelationType m_relation : 3;
    CSSSelector* m_selector;

    explicit CSSSelectorListItem(CSSSelector* selector)
        : m_relationIsAffectedByPseudoContent(false)
        , m_relation(RelationType::SubSelector)
        , m_selector(selector)
    {
    }
};

class CSSSelectorList : public GCVector<CSSSelectorListItem> {
public:
    CSSSelectorList()
        : m_specificity(0)
    {
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
    enum Type ENSURE_ENUM_UNSIGNED {
        UnKnown,
        Universal,
        Tag,
        Id,
        Class,
        PseudoElement,
        PseudoClass,
        // A type/universal selector qualified by an explicit or default
        // namespace (`svg|rect`, `*|rect`, `|rect`, or a bare `div`/`*`
        // rewritten under an in-scope default namespace -- see
        // CSSNamespacedTagSelector below). Must stay strictly before
        // AttributeExact: hasImmutableData()/isAttributeSelector() below key
        // off this enum's ordering, and this type is neither.
        NamespacedTag,
        AttributeExact,   // Example: E[foo="bar"]
        AttributeSet,     // Example: E[foo]
        AttributeHyphen,  // Example: E[foo|="bar"]
        AttributeList,    // Example: E[foo~="bar"]
        AttributeContain, // css3: E[foo*="bar"]
        AttributeBegin,   // css3: E[foo^="bar"]
        AttributeEnd,     // css3: E[foo$="bar"]
        FirstAttributeSelectorMatch = AttributeExact,
    };

    enum PseudoType ENSURE_ENUM_UNSIGNED {
        PseudoNone,
#define ADD_PSEUDO_TYPE(name, nameLower, selectorName) Pseudo##name,
        STARFISH_ENUM_PSEUDO_SELECTORS(ADD_PSEUDO_TYPE)
#undef ADD_PSEUDO_TYPE
            PseudoTotalCount,
    };

    enum AttributeMatchType ENSURE_ENUM_UNSIGNED {
        CaseInsensitive,
        CaseSensitive,
    };

    CSSSelector(Type type, AtomicString text,
                PseudoType pseudoType = PseudoNone,
                AttributeMatchType attributeMatch = CaseInsensitive)
        : m_type(type)
        , m_pseudotype(pseudoType)
        , m_attributeMatch(attributeMatch)
        , m_selectorText(text)
    {
    }

    bool hasImmutableData() const
    {
        return m_type < PseudoElement;
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

    bool isNamespacedTagSelector() const
    {
        return m_type == NamespacedTag;
    }

    CSSNamespacedTagSelector* asCSSNamespacedTagSelector()
    {
        STARFISH_ASSERT(isNamespacedTagSelector());
        return (CSSNamespacedTagSelector*)this;
    }

    const CSSNamespacedTagSelector* asCSSNamespacedTagSelector() const
    {
        STARFISH_ASSERT(isNamespacedTagSelector());
        return (const CSSNamespacedTagSelector*)this;
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

    PseudoType pseudotype() const
    {
        return m_pseudotype;
    }

    AttributeMatchType attributeMatch() const
    {
        return m_attributeMatch;
    }

    const AtomicString& selectorText() const
    {
        return m_selectorText;
    }

    bool isSimple(CSSSelectorList* selectorList);

    // http://www.w3.org/TR/css3-selectors/#specificity
    unsigned specificityForOneSelector() const;
    bool isPseudoClassHostFamilySelector();
    bool isSlottedSelector();

protected:
    Type m_type : 4;
    PseudoType m_pseudotype : 7;
    AttributeMatchType m_attributeMatch : 1;
    AtomicString m_selectorText;
};

#if !defined(COMPILER_MSVC)
static_assert(sizeof(CSSSelector) <= sizeof(size_t) * 2,
              "keep sizeof CSSSelector small");
#endif

class CSSAttributeSelector : public CSSSelector {
public:
    CSSAttributeSelector(CSSSelector::Type type, const QualifiedName& attr,
                         String* value,
                         CSSSelector::AttributeMatchType matchType)
        : CSSSelector(type, AtomicString())
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

// A type/universal selector qualified by a namespace (explicit prefix,
// explicit `*` any-namespace, explicit `|` no-namespace, or an in-scope
// default namespace applied at parse time -- see
// CSSParser::prependTypeSelectorIfNeeded()/determineNamespace()).
// `m_selectorText` (inherited) is kept as the bare local name -- NOT
// `prefix|localName` -- so existing local-name-keyed code (rule bucketing,
// the ancestor Bloom filter, SelectorQuery's tag fast path) keeps working
// unmodified; only matching/serialization/specificity need to know about the
// namespace, and consult `m_name` for it.
//
// Namespace URI convention (matches determineNamespace()'s return value):
// literal "*" = any namespace, empty = no namespace, anything else = that
// resolved URI. Local name "*" = any tag (the namespaced-universal form,
// e.g. `svg|*`).
class CSSNamespacedTagSelector : public CSSSelector {
public:
    CSSNamespacedTagSelector(const QualifiedName& name)
        : CSSSelector(CSSSelector::Type::NamespacedTag, name.localNameAtomic())
        , m_name(name)
    {
    }

    const QualifiedName& qualifiedName() const
    {
        return m_name;
    }

protected:
    QualifiedName m_name;
};

class CSSPseudoSelector : public CSSSelector {
public:
    CSSPseudoSelector(CSSSelector::Type type)
        : CSSSelector(type, AtomicString())
        , m_argument(String::emptyString)
    {
        m_pseudotype = PseudoNone;
        m_nth = { 0, 0 };
    }

    PseudoType pseudoType() const
    {
        return m_pseudotype;
    }

    // Each element is one complex-selector branch of the pseudo's argument:
    // comma-separated branches for :is()/:where()/:not(), and exactly one
    // branch (a single compound selector) for :host(). See
    // CSSParser::getPseudoSelector() for how each pseudo populates this.
    GCVector<CSSSelectorList*>& selectorArguments()
    {
        return m_selectorArguments;
    }

    void addSelectorArgument(CSSSelectorList* argument)
    {
        STARFISH_ASSERT(m_type != Tag);
        m_selectorArguments.push_back(argument);
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

    PseudoType parsePseudoType(Starfish* starfish, AtomicString pseudoName,
                               bool hasArguments);
    void updatePseudoType(Starfish* starfish, AtomicString name,
                          bool hasArguments);

protected:
    GCVector<CSSSelectorList*> m_selectorArguments;
    String* m_argument;
    struct {
        int m_a; // Used for :nth-*
        int m_b; // Used for :nth-*
    } m_nth;
};

using Declarations = GCVector<std::pair<CSSStyleDeclaration*, ResourceURL*>>;

template <unsigned int InlineStorageSize = 32>
using MatchedStyleRules = VectorWithInlineStorage<
    InlineStorageSize, std::pair<StyleRule*, ResourceURL*>,
    std::allocator<std::pair<StyleRule*, ResourceURL*>>>;

class CSSStyleSheet;
class StyleResolver;

class StyleResolveContext {
public:
    StyleResolveContext(
        Node* node,
        Optional<GCVector<ComputedStyle*>*> computedStylePool = nullptr);
    StyleResolveContext(StyleResolver* sr, StyleResolveContext& origin);
    ~StyleResolveContext();
    void pushIntoComputedStylePool(ComputedStyle* b);

    bool hasItemInComputedStylePool()
    {
        return m_computedStylePool->size();
    }

    void* takeFromComputedStylePool()
    {
        void* ret = m_computedStylePool->back();
        m_computedStylePool->pop_back();
        STARFISH_ASSERT(ret);
        return ret;
    }

    void* allocateComputedStyle();

    StyleResolver* m_styleResolver;
    std::unique_ptr<AncestorSelectorFilter> m_ancestorSelectorFilter;
    GCVector<ComputedStyle*>* m_computedStylePool;
};

enum PseudoElementType ENSURE_ENUM_UNSIGNED {
    PseudoElementNone,
    PseudoElementFirstLine,
    PseudoElementFirstLetter,
    PseudoElementBefore,
    PseudoElementAfter,
    PseudoElementFirstLineInherited,
    PseudoElementFormOnly,
    PseudoElementCounter,
    PseudoElementGeneralTypeStart = PseudoElementFirstLine,
    PseudoElementGeneralTypeEnd = PseudoElementAfter,
    PseudoElementMappedTypeStart = PseudoElementBefore,
    PseudoElementMappedTypeEnd = PseudoElementAfter,
};

class StyleResolver : public DocumentHoldable, public gc {
    friend void computeCSSCombinatorSelectorCache(
        StyleResolver* resolver, Element* element,
        const GCVector<std::pair<StyleRule*, ResourceURL*>>::iterator& begin,
        const GCVector<std::pair<StyleRule*, ResourceURL*>>::iterator& end);
    friend class BrowsingContext; // for updating m_mediumFontSize
public:
    enum Match ENSURE_ENUM_UNSIGNED {
        SelectorMatches,          // The selector matches the element
        SelectorFailsLocally,     // The selector fails for the element.
        SelectorFailsAllSiblings, // The selector fails for the element and any
                                  // sibling of the element
        SelectorFailsCompletely   // The selector fails for the element or
                                  // ancestor of the element
    };

    // MUST uses same bit with Node::StyleChangeReason
    enum StyleDamageSource ENSURE_ENUM_UNSIGNED {
        NoDamage = 0,
        StyleDamageFromID = 1,
        StyleDamageFromClass = 1 << 1,
        StyleDamageFromAttribute = 1 << 2,
        StyleDamageFromElementState = 1 << 3,
        StyleDamageFromDOMTree = 1 << 4,
        StyleDamageFromElementStateDOMTree = 1 << 5,
        StyleDamageFromAll =
            StyleDamageFromID | StyleDamageFromClass |
            StyleDamageFromAttribute | StyleDamageFromElementState |
            StyleDamageFromDOMTree | StyleDamageFromElementStateDOMTree
    };

    struct MatchResult {
        MatchResult(Optional<Node*> scopeRoot)
            : pseudoType(PseudoElementNone)
            , styleDamageFrom(NoDamage)
            , styleDamageSourceNodeStateMap(0)
            , styleDamageSourceNodeStateDOMTreeMap(0)
            , seenCombinator(false)
            , scope(scopeRoot)
        {
        }

        PseudoElementType pseudoType;
        StyleDamageSource styleDamageFrom;
        int styleDamageSourceNodeStateMap;
        int styleDamageSourceNodeStateDOMTreeMap;
        bool seenCombinator;
        Optional<Node*> scope;
    };

    StyleResolver(Document* document, ShadowRoot* ownerShadowRoot = nullptr);

    // True when this resolver was created for a ShadowRoot rather than for the
    // document.  Used to gate :host promotion and to identify the origin host.
    bool isShadowResolver() const
    {
        return m_ownerShadowRoot.hasValue();
    }
    Element* ownerHost() const;

    void setNeedsRecalcRuleSet()
    {
        m_needsRecalcRuleSet = true;
    }
    void recalcRuleSetIfNeeds();
    void addSheet(CSSStyleSheet* sheet);
    void removeSheet(CSSStyleSheet* sheet);

    // Constructable style sheets adopted via
    // DocumentOrShadowRoot.adoptedStyleSheets. Kept apart from m_sheets (which
    // holds <style>/<link> sheets in tree order) and folded into the rule set
    // during recalc; per CSSOM they cascade after the tree's own sheets.
    void setAdoptedSheets(const GCVector<CSSStyleSheet*>& sheets);

    // A :host rule promoted from a shadow resolver carries the host element as
    // its scope so the document resolver can restrict matching to that element.
    struct HostScopedRule {
        StyleRule* rule;
        ResourceURL* url;
        Element* host; // ShadowRoot::host() of the originating shadow tree
    };
    void addHostScopedRule(std::pair<StyleRule*, ResourceURL*> rule,
                           Element* host);

    // ::slotted() is the mirror image of :host(): the rule also lives in a
    // shadow resolver, but the element it matches is a slotted light-DOM
    // node that the OUTER resolver styles. Promoted the same way, keyed by
    // the same origin host, so matching can find "the rules from the shadow
    // tree this element is slotted into".
    struct SlottedScopedRule {
        StyleRule* rule;
        ResourceURL* url;
        Element* host; // ShadowRoot::host() of the originating shadow tree
    };
    void addSlottedScopedRule(std::pair<StyleRule*, ResourceURL*> rule,
                              Element* host);

    GCVector<CSSStyleSheet*>& sheets()
    {
        return m_sheets;
    }

    RuleSet* ruleSet() const
    {
        return m_ruleSet;
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
    ComputedStyle* resolveStyle(StyleResolveContext& ctx, Element* node,
                                ComputedStyle* parent);

    void matchAllRules(
        StyleResolveContext& ctx, Element* element, ComputedStyle* ret,
        ComputedStyle* parent,
        PseudoElementType pseudoType = PseudoElementType::PseudoElementNone);
    Match matchSelector(Element* element, AtomicString elementName,
                        AtomicString elementId,
                        const GCAtomicTightVector<AtomicString>& elementClasses,
                        const CSSSelectorList& selectorList, unsigned idx,
                        MatchResult& result, bool isQueryingSelector = false);
    void collectMatchingRulesFromAuthorSheet(
        StyleResolveContext& ctx,
        const GCVector<std::pair<StyleRule*, ResourceURL*>>::iterator& begin,
        const GCVector<std::pair<StyleRule*, ResourceURL*>>::iterator& end,
        CSSSelector::Type type, Element* element, AtomicString elementName,
        AtomicString elementId,
        const GCAtomicTightVector<AtomicString>& elementClasses,
        MatchedStyleRules<>& authorRules, ComputedStyle* ret,
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
    void resolveChildrenStyle(StyleResolveContext& ctx, Node* element,
                              ComputedStyle* elementStyle,
                              bool inheritedStyleChanged = false);

    static CSSTokenValue resolveVarReferencedValue(
        Node* node, OptionalUTF8String utf8String,
        Optional<const MutablePropertyValueList*> cssCustomValues);

    CSSTokenValue resolveVarReferencedValue(
        Node* node, const CSSStyleValuePair& cssValuePair);

    void clearCssCustomValues();
    Optional<const MutablePropertyValueList*> cssCustomValues();
    CSSStyleDeclaration* resolveVarValue(Element* element,
                                         const CSSStyleValuePair& cssValuePair,
                                         CSSStyleValuePair::KeyKind keyKind,
                                         bool isImportant);

protected:
    void recalcWebFonts();
    void addToRuleSet(CSSStyleSheet* sheet);
    void addToRuleSet(std::pair<StyleRule*, ResourceURL*> rule);
    void registerAttrFilterFromSelectorArguments(
        CSSPseudoSelector* pseudoSelector);
    void addToKeyframesRule(StyleRuleKeyframes* rule);
    void removeAllRules();
    size_t nextRuleSetOrder();
    void resetNextRuleSetOrder();

    void apply(Element* element,
               const GCAtomicVector<CSSStyleValuePair>& cssValues,
               ResourceURL* origin, ComputedStyle* style,
               ComputedStyle* parentStyle, bool isImportant = false);

    void applyProperty(Element* element, const CSSStyleValuePair& cssValues,
                       ResourceURL* origin, ComputedStyle* style,
                       ComputedStyle* parentStyle, bool isImportant = false);

    void applyAllProperty(Element* element,
                          CSSStyleValuePair::ValueKind valueKind,
                          ResourceURL* origin, ComputedStyle*& style,
                          ComputedStyle* parentStyle, bool isImportant = false);

    Match matchForRelation(
        Element* element, AtomicString elementName, AtomicString elementId,
        const GCAtomicTightVector<AtomicString>& elementClasses,
        const CSSSelectorList& selectorList,
        CSSSelectorListItem::RelationType relation, unsigned idx,
        MatchResult& result);

    ALWAYS_INLINE bool checkOne(
        Element* element, AtomicString elementName, AtomicString elementId,
        const GCAtomicTightVector<AtomicString>& elementClasses,
        CSSSelector* selector, MatchResult& result,
        bool isQueryingSelector = false);
    bool checkPseudoClass(Element* element, CSSPseudoSelector* selector,
                          MatchResult& result, bool isQueryingSelector = false);
    bool checkPseudoElement(Element* element, CSSPseudoSelector* selector,
                            MatchResult& result);
    bool anyAttributeMatches(Element* element, CSSSelector::Type type,
                             CSSAttributeSelector* selector,
                             MatchResult& result);
    bool tryAddSheet(Node* node, CSSStyleSheet* sheet);
    bool traverseAndTryAddSheet(Node* node, CSSStyleSheet* sheet,
                                bool& originFound);

    bool m_usesFirstLineRule;
    bool m_needsRecalcRuleSet;
    bool m_hasSimplePseudoClassHostSelector;
    bool m_hasSlottedSelector;
    uint32_t m_mediumFontSize;
    Optional<ShadowRoot*> m_ownerShadowRoot;
    GCVector<CSSStyleSheet*> m_sheets;
    GCVector<CSSStyleSheet*> m_adoptedSheets;
    // :host rules promoted from shadow resolvers, stored with the origin host
    // so the document resolver can scope matching to the correct element.
    GCVector<HostScopedRule> m_hostScopedRules;
    // ::slotted() rules promoted from shadow resolvers; matched against
    // slotted light-DOM elements by the outer resolver (see matchAllRules).
    GCVector<SlottedScopedRule> m_slottedScopedRules;
    GCVector<std::pair<CSSStyleDeclaration*, ResourceURL*>> m_webFonts;
    MediaQueryEvaluator* m_mediaQueryEvaluator;
    MediaQueryResultList m_viewportDependentMediaQueryResults;
    MediaQueryResultList m_deviceDependentMediaQueryResults;
    RuleSet* m_ruleSet;
    size_t m_nextRuleSetOrder;
    GCAtomicVector<AtomicString> m_ruleSetAttrFilter;
    Optional<MutablePropertyValueList*> m_cssCustomValues;
};

} // namespace Starfish

#endif
