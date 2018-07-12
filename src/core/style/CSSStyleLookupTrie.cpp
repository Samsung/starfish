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

#include "StarFishConfig.h"
#include "CSSStyleLookupTrie.h"

namespace StarFish {

CSSStyleValuePair::KeyKind lookupCSSStyle(const char* data, unsigned length)
{
    switch (length) {
    case 1:
        if (memcmp(data, "x", 1) == 0) {
            return CSSStyleValuePair::KeyKind::X;
        } else if (memcmp(data, "y", 1) == 0) {
            return CSSStyleValuePair::KeyKind::Y;
        } else if (memcmp(data, "d", 1) == 0) {
            return CSSStyleValuePair::KeyKind::D;
        } else if (memcmp(data, "r", 1) == 0) {
            return CSSStyleValuePair::KeyKind::R;
        }
        break;
    case 2:
        if (memcmp(data, "cx", 2) == 0) {
            return CSSStyleValuePair::KeyKind::CX;
        } else if (memcmp(data, "cy", 2) == 0) {
            return CSSStyleValuePair::KeyKind::CY;
        } else if (memcmp(data, "rx", 2) == 0) {
            return CSSStyleValuePair::KeyKind::RX;
        } else if (memcmp(data, "ry", 2) == 0) {
            return CSSStyleValuePair::KeyKind::RY;
        }
        break;
    case 3:
        // All
        // Top
        // Src
        if (memcmp(data, "all", 3) == 0) {
            return CSSStyleValuePair::KeyKind::All;
        }
        if (memcmp(data, "top", 3) == 0) {
            return CSSStyleValuePair::KeyKind::Top;
        }
        if (memcmp(data, "src", 3) == 0) {
            return CSSStyleValuePair::KeyKind::Src;
        }
        break;
    case 4:
        // Font
        // Flex
        // Left
        // Fill
        // Clip
        switch (data[0]) {
        case 'f':
            if (memcmp(data, "font", 4) == 0) {
                return CSSStyleValuePair::KeyKind::Font;
            }
            if (memcmp(data, "flex", 4) == 0) {
                return CSSStyleValuePair::KeyKind::Flex;
            }
            if (memcmp(data, "fill", 4) == 0) {
                return CSSStyleValuePair::KeyKind::Fill;
            }
            break;
        case 'l':
            if (memcmp(data, "left", 4) == 0) {
                return CSSStyleValuePair::KeyKind::Left;
            }
            break;
        case 'c':
            if (memcmp(data, "clip", 4) == 0) {
                return CSSStyleValuePair::KeyKind::Clip;
            }
            break;
        }
        break;
    case 5:
        // Color
        // Clear
        // Float
        // Width
        // Right
        // Order
        switch (data[0]) {
        case 'c':
            if (memcmp(data, "color", 5) == 0) {
                return CSSStyleValuePair::KeyKind::Color;
            }
            if (memcmp(data, "clear", 5) == 0) {
                return CSSStyleValuePair::KeyKind::Clear;
            }
            break;
        case 'f':
            if (memcmp(data, "float", 5) == 0) {
                return CSSStyleValuePair::KeyKind::Float;
            }
            break;
        case 'w':
            if (memcmp(data, "width", 5) == 0) {
                return CSSStyleValuePair::KeyKind::Width;
            }
            break;
        case 'r':
            if (memcmp(data, "right", 5) == 0) {
                return CSSStyleValuePair::KeyKind::Right;
            }
            break;
        case 'o':
            if (memcmp(data, "order", 5) == 0) {
                return CSSStyleValuePair::KeyKind::Order;
            }
            break;
        }
        break;
    case 6:
        // Bottom
        // Border
        // Height
        // Margin
        // Stroke
        // Cursor
        // resize
        switch (data[0]) {
        case 'b':
            switch (data[1]) {
            case 'o':
                if (memcmp(data, "bottom", 6) == 0) {
                    return CSSStyleValuePair::KeyKind::Bottom;
                }
                if (memcmp(data, "border", 6) == 0) {
                    return CSSStyleValuePair::KeyKind::Border;
                }
                break;
            }
            break;
        case 'c':
            if (memcmp(data, "cursor", 6) == 0) {
                return CSSStyleValuePair::KeyKind::Cursor;
            }
            break;
        case 'f':
            if (memcmp(data, "filter", 6) == 0) {
                return CSSStyleValuePair::KeyKind::Filter;
            }
            break;
        case 'h':
            if (memcmp(data, "height", 6) == 0) {
                return CSSStyleValuePair::KeyKind::Height;
            }
            break;
        case 'm':
            if (memcmp(data, "margin", 6) == 0) {
                return CSSStyleValuePair::KeyKind::Margin;
            }
            break;
        case 'r':
            if (memcmp(data, "resize", 6) == 0) {
                return CSSStyleValuePair::KeyKind::Resize;
            }
            break;
        case 's':
            if (memcmp(data, "stroke", 6) == 0) {
                return CSSStyleValuePair::KeyKind::Stroke;
            }
            break;
        }
        break;
    case 7:
        // Content
        // Display
        // Padding
        // Z-Index
        // Opacity
        // Outline
        // Hyphens
        switch (data[0]) {
        case 'c':
            if (memcmp(data, "content", 7) == 0) {
                return CSSStyleValuePair::KeyKind::Content;
            }
            break;
        case 'd':
            if (memcmp(data, "display", 7) == 0) {
                return CSSStyleValuePair::KeyKind::Display;
            }
            break;
        case 'h':
            if (memcmp(data, "hyphens", 7) == 0) {
                return CSSStyleValuePair::KeyKind::Hyphens;
            }
            break;
        case 'p':
            if (memcmp(data, "padding", 7) == 0) {
                return CSSStyleValuePair::KeyKind::Padding;
            }
            break;
        case 'z':
            if (memcmp(data, "z-index", 7) == 0) {
                return CSSStyleValuePair::KeyKind::ZIndex;
            }
            break;
        case 'o':
            if (memcmp(data, "opacity", 7) == 0) {
                return CSSStyleValuePair::KeyKind::Opacity;
            }
            if (memcmp(data, "outline", 7) == 0) {
                return CSSStyleValuePair::KeyKind::Outline;
            }
            break;
        }
        break;
    case 8:
        // Position
        // Overflow
        // Grid-Gap
        // Grid-Row
        switch (data[0]) {
        case 'p':
            if (memcmp(data, "position", 8) == 0) {
                return CSSStyleValuePair::KeyKind::Position;
            }
            break;
        case 'o':
            if (memcmp(data, "overflow", 8) == 0) {
                return CSSStyleValuePair::KeyKind::Overflow;
            }
            break;
        case 'g':
            if (memcmp(data, "grid-gap", 8) == 0) {
                return CSSStyleValuePair::KeyKind::GridGap;
            }
            if (memcmp(data, "grid-row", 8) == 0) {
                return CSSStyleValuePair::KeyKind::GridRow;
            }
            break;
        }
        break;
    case 9:
        // Font-Size
        // Flex-Wrap
        // Flex-Flow
        // Flex-Grow
        // Transform
        // Direction
        // Max-Width
        // Min-Width
        // Fill-Rule
        // Word-Wrap
        // Mask-Size
        // Grid-Area
        switch (data[0]) {
        case 'f':
            if (memcmp(data, "font-size", 9) == 0) {
                return CSSStyleValuePair::KeyKind::FontSize;
            }
            if (memcmp(data, "flex-wrap", 9) == 0) {
                return CSSStyleValuePair::KeyKind::FlexWrap;
            }
            if (memcmp(data, "flex-flow", 9) == 0) {
                return CSSStyleValuePair::KeyKind::FlexFlow;
            }
            if (memcmp(data, "flex-grow", 9) == 0) {
                return CSSStyleValuePair::KeyKind::FlexGrow;
            }
            if (memcmp(data, "fill-rule", 9) == 0) {
                return CSSStyleValuePair::KeyKind::FillRule;
            }
            break;
        case 't':
            if (memcmp(data, "transform", 9) == 0) {
                return CSSStyleValuePair::KeyKind::Transform;
            }
            break;
        case 'd':
            if (memcmp(data, "direction", 9) == 0) {
                return CSSStyleValuePair::KeyKind::Direction;
            }
            break;
        case 'm':
            if (memcmp(data, "max-width", 9) == 0) {
                return CSSStyleValuePair::KeyKind::MaxWidth;
            }
            if (memcmp(data, "min-width", 9) == 0) {
                return CSSStyleValuePair::KeyKind::MinWidth;
            }
            if (memcmp(data, "mask-size", 9) == 0) {
                return CSSStyleValuePair::KeyKind::MaskSize;
            }
            break;
        case 'w':
            if (memcmp(data, "word-wrap", 9) == 0) {
                return CSSStyleValuePair::KeyKind::WordWrap;
            }
            break;
        case 'g':
            if (memcmp(data, "grid-area", 9) == 0) {
                return CSSStyleValuePair::KeyKind::GridArea;
            }
            break;
        }
        break;
    case 10:
        // Background
        // Border-Top
        // Box-Shadow
        // Box-Sizing
        // Font-Style
        // Flex-Basis
        // List-Style
        // Text-Align
        // Transition
        // Margin-Top
        // Mask-Image
        // Max-Height
        // Min-Height
        // Object-fit
        // Overflow-X
        // Overflow-Y
        // Visibility
        // Align-Self
        // Line-Break
        // Word-Break
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "background", 10) == 0) {
                return CSSStyleValuePair::KeyKind::Background;
            }
            if (memcmp(data, "border-top", 10) == 0) {
                return CSSStyleValuePair::KeyKind::BorderTop;
            }
            if (memcmp(data, "box-shadow", 10) == 0) {
                return CSSStyleValuePair::KeyKind::BoxShadow;
            }
            if (memcmp(data, "box-sizing", 10) == 0) {
                return CSSStyleValuePair::KeyKind::BoxSizing;
            }
            break;
        case 'f':
            if (memcmp(data, "font-style", 10) == 0) {
                return CSSStyleValuePair::KeyKind::FontStyle;
            }
            if (memcmp(data, "flex-basis", 10) == 0) {
                return CSSStyleValuePair::KeyKind::FlexBasis;
            }
            break;
        case 'l':
            if (memcmp(data, "list-style", 10) == 0) {
                return CSSStyleValuePair::KeyKind::ListStyle;
            }
            if (memcmp(data, "line-break", 10) == 0) {
                return CSSStyleValuePair::KeyKind::LineBreak;
            }
            break;
        case 't':
            if (memcmp(data, "text-align", 10) == 0) {
                return CSSStyleValuePair::KeyKind::TextAlign;
            }
            if (memcmp(data, "transition", 10) == 0) {
                return CSSStyleValuePair::KeyKind::Transition;
            }
            break;
        case 'm':
            if (memcmp(data, "margin-top", 10) == 0) {
                return CSSStyleValuePair::KeyKind::MarginTop;
            }
            if (memcmp(data, "mask-image", 10) == 0) {
                return CSSStyleValuePair::KeyKind::MaskImage;
            }
            if (memcmp(data, "max-height", 10) == 0) {
                return CSSStyleValuePair::KeyKind::MaxHeight;
            }
            if (memcmp(data, "min-height", 10) == 0) {
                return CSSStyleValuePair::KeyKind::MinHeight;
            }
            break;
        case 'o':
            if (memcmp(data, "object-fit", 10) == 0) {
                return CSSStyleValuePair::KeyKind::ObjectFit;
            }
            if (memcmp(data, "overflow-x", 10) == 0) {
                return CSSStyleValuePair::KeyKind::OverflowX;
            }
            if (memcmp(data, "overflow-y", 10) == 0) {
                return CSSStyleValuePair::KeyKind::OverflowY;
            }
            break;
        case 'v':
            if (memcmp(data, "visibility", 10) == 0) {
                return CSSStyleValuePair::KeyKind::Visibility;
            }
            break;
        case 'a':
            if (memcmp(data, "align-self", 10) == 0) {
                return CSSStyleValuePair::KeyKind::AlignSelf;
            }
            break;
        case 'w':
            if (memcmp(data, "word-break", 10) == 0) {
                return CSSStyleValuePair::KeyKind::WordBreak;
            }
            break;
        }
        break;
    case 11:
        // Border-Left
        // Font-Weight
        // Font-Family
        // Flex-Shrink
        // Line-Height
        // White-Space
        // Padding-Top
        // Margin-Left
        // Text-Indent
        // Text-Shadow
        // Empty-Cells
        // Align-Items
        // User-Select
        // Caret-Color
        // Will-Change
        // Grid-Column
        switch (data[0]) {
        case 'a':
            if (memcmp(data, "align-items", 11) == 0) {
                return CSSStyleValuePair::KeyKind::AlignItems;
            }
            break;
        case 'b':
            if (memcmp(data, "border-left", 11) == 0) {
                return CSSStyleValuePair::KeyKind::BorderLeft;
            }
            break;
        case 'c':
            if (memcmp(data, "caret-color", 11) == 0) {
                return CSSStyleValuePair::KeyKind::CaretColor;
            }
            break;
        case 'e':
            if (memcmp(data, "empty-cells", 11) == 0) {
                return CSSStyleValuePair::KeyKind::EmptyCells;
            }
            break;
        case 'f':
            if (memcmp(data, "font-weight", 11) == 0) {
                return CSSStyleValuePair::KeyKind::FontWeight;
            }
            if (memcmp(data, "font-family", 11) == 0) {
                return CSSStyleValuePair::KeyKind::FontFamily;
            }
            if (memcmp(data, "flex-shrink", 11) == 0) {
                return CSSStyleValuePair::KeyKind::FlexShrink;
            }
            break;
        case 'l':
            if (memcmp(data, "line-height", 11) == 0) {
                return CSSStyleValuePair::KeyKind::LineHeight;
            }
            break;
        case 'm':
            if (memcmp(data, "margin-left", 11) == 0) {
                return CSSStyleValuePair::KeyKind::MarginLeft;
            }
            break;
        case 'p':
            if (memcmp(data, "padding-top", 11) == 0) {
                return CSSStyleValuePair::KeyKind::PaddingTop;
            }
            break;
        case 't':
            if (memcmp(data, "text-indent", 11) == 0) {
                return CSSStyleValuePair::KeyKind::TextIndent;
            }
            if (memcmp(data, "text-shadow", 11) == 0) {
                return CSSStyleValuePair::KeyKind::TextShadow;
            }
            break;
        case 'u':
            if (memcmp(data, "user-select", 11) == 0) {
                return CSSStyleValuePair::KeyKind::UserSelect;
            }
            break;
        case 'w':
            if (memcmp(data, "will-change", 11) == 0) {
                return CSSStyleValuePair::KeyKind::WillChange;
            }
            if (memcmp(data, "white-space", 11) == 0) {
                return CSSStyleValuePair::KeyKind::WhiteSpace;
            }
            break;
        case 'g':
            if (memcmp(data, "grid-column", 11) == 0) {
                return CSSStyleValuePair::KeyKind::GridColumn;
            }
            break;
        }
        break;
    case 12:
        // Border-Image
        // Border-Style
        // Border-Width
        // Border-Color
        // Border-Right
        // Padding-Left
        // Margin-Right
        // unicode-bidi
        // caption-side
        // Fill-Opacity
        // Stroke-Width
        // Word-Spacing
        // font-kerning
        // Grid-Row-End
        // Grid-Row-Gap
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "border-image", 12) == 0) {
                return CSSStyleValuePair::KeyKind::BorderImage;
            }
            if (memcmp(data, "border-style", 12) == 0) {
                return CSSStyleValuePair::KeyKind::BorderStyle;
            }
            if (memcmp(data, "border-width", 12) == 0) {
                return CSSStyleValuePair::KeyKind::BorderWidth;
            }
            if (memcmp(data, "border-color", 12) == 0) {
                return CSSStyleValuePair::KeyKind::BorderColor;
            }
            if (memcmp(data, "border-right", 12) == 0) {
                return CSSStyleValuePair::KeyKind::BorderRight;
            }
            break;
        case 'c':
            if (memcmp(data, "caption-side", 12) == 0) {
                return CSSStyleValuePair::KeyKind::CaptionSide;
            }
            break;
        case 'p':
            if (memcmp(data, "padding-left", 12) == 0) {
                return CSSStyleValuePair::KeyKind::PaddingLeft;
            }
            break;
        case 'm':
            if (memcmp(data, "margin-right", 12) == 0) {
                return CSSStyleValuePair::KeyKind::MarginRight;
            }
            break;
        case 't':
            if (memcmp(data, "table-layout", 12) == 0) {
                return CSSStyleValuePair::KeyKind::TableLayout;
            }
            break;
        case 'u':
            if (memcmp(data, "unicode-bidi", 12) == 0) {
                return CSSStyleValuePair::KeyKind::UnicodeBidi;
            }
            break;
        case 'f':
            if (memcmp(data, "fill-opacity", 12) == 0) {
                return CSSStyleValuePair::KeyKind::FillOpacity;
            }
            if (memcmp(data, "font-kerning", 12) == 0) {
                return CSSStyleValuePair::KeyKind::FontKerning;
            }
            break;
        case 's':
            if (memcmp(data, "stroke-width", 12) == 0) {
                return CSSStyleValuePair::KeyKind::StrokeWidth;
            }
            break;
        case 'w':
            if (memcmp(data, "word-spacing", 12) == 0) {
                return CSSStyleValuePair::KeyKind::WordSpacing;
            }
            break;
        case 'g':
            if (memcmp(data, "grid-row-end", 12) == 0) {
                return CSSStyleValuePair::KeyKind::GridRowEnd;
            }
            if (memcmp(data, "grid-row-gap", 12) == 0) {
                return CSSStyleValuePair::KeyKind::GridRowGap;
            }
            break;
        }
        break;
    case 13:
        // Border-Radius
        // Padding-Right
        // Margin-Bottom
        // Border-Bottom
        // Align-Content
        // Counter-Reset
        // Outline-Width
        // Outline-Style
        // Outline-Color
        // Overflow-Wrap
        // text-overflow
        switch (data[0]) {
        case 'p':
            if (memcmp(data, "padding-right", 13) == 0) {
                return CSSStyleValuePair::KeyKind::PaddingRight;
            }
            break;
        case 'm':
            if (memcmp(data, "margin-bottom", 13) == 0) {
                return CSSStyleValuePair::KeyKind::MarginBottom;
            }
            break;
        case 'b':
            if (memcmp(data, "border-bottom", 13) == 0) {
                return CSSStyleValuePair::KeyKind::BorderBottom;
            }
            if (memcmp(data, "border-radius", 13) == 0) {
                return CSSStyleValuePair::KeyKind::BorderRadius;
            }
            break;
        case 'a':
            if (memcmp(data, "align-content", 13) == 0) {
                return CSSStyleValuePair::KeyKind::AlignContent;
            }
            break;
        case 'c':
            if (memcmp(data, "counter-reset", 13) == 0) {
                return CSSStyleValuePair::KeyKind::CounterReset;
            }
            break;
        case 't':
            if (memcmp(data, "text-overflow", 13) == 0) {
                return CSSStyleValuePair::KeyKind::TextOverflow;
            }
            break;
        case 'o':
            if (memcmp(data, "outline-width", 13) == 0) {
                return CSSStyleValuePair::KeyKind::OutlineWidth;
            }
            if (memcmp(data, "outline-style", 13) == 0) {
                return CSSStyleValuePair::KeyKind::OutlineStyle;
            }
            if (memcmp(data, "outline-color", 13) == 0) {
                return CSSStyleValuePair::KeyKind::OutlineColor;
            }
            if (memcmp(data, "overflow-wrap", 13) == 0) {
                return CSSStyleValuePair::KeyKind::OverflowWrap;
            }
            break;
        }
        break;
    case 14:
        // Vertical-Align
        // Padding-Bottom
        // Border-Spacing
        // Flex-Direction
        // Outline-Offset
        // text-transform
        // letter-spacing
        // Grid-Row-Start
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "border-spacing", 14) == 0) {
                return CSSStyleValuePair::KeyKind::BorderSpacing;
            }
            break;
        case 'f':
            if (memcmp(data, "flex-direction", 14) == 0) {
                return CSSStyleValuePair::KeyKind::FlexDirection;
            }
            break;
        case 'v':
            if (memcmp(data, "vertical-align", 14) == 0) {
                return CSSStyleValuePair::KeyKind::VerticalAlign;
            }
            break;
        case 'p':
            if (memcmp(data, "padding-bottom", 14) == 0) {
                return CSSStyleValuePair::KeyKind::PaddingBottom;
            }
            if (memcmp(data, "pointer-events", 14) == 0) {
                return CSSStyleValuePair::KeyKind::PointerEvents;
            }
            break;
        case 'o':
            if (memcmp(data, "outline-offset", 14) == 0) {
                return CSSStyleValuePair::KeyKind::OutlineOffset;
            }
            break;
        case 't':
            if (memcmp(data, "text-transform", 14) == 0) {
                return CSSStyleValuePair::KeyKind::TextTransform;
            }
            break;
        case 'l':
            if (memcmp(data, "letter-spacing", 14) == 0) {
                return CSSStyleValuePair::KeyKind::LetterSpacing;
            }
            break;
        case 'g':
            if (memcmp(data, "grid-row-start", 14) == 0) {
                return CSSStyleValuePair::KeyKind::GridRowStart;
            }
            break;
        }
        break;
    case 15:
        // Text-Decoration
        // Background-Size
        // Border-Collapse
        // Justify-Content
        // Background-Clip
        // Object-position
        // List-Style-Type
        // Grid-Column-End
        // Grid-Column-Gap
        switch (data[0]) {
        case 'j':
            if (memcmp(data, "justify-content", 15) == 0) {
                return CSSStyleValuePair::KeyKind::JustifyContent;
            }
            break;
        case 't':
            if (memcmp(data, "text-decoration", 15) == 0) {
                return CSSStyleValuePair::KeyKind::TextDecoration;
            }
            break;
        case 'b':
            if (memcmp(data, "background-size", 15) == 0) {
                return CSSStyleValuePair::KeyKind::BackgroundSize;
            }
            if (memcmp(data, "background-clip", 15) == 0) {
                return CSSStyleValuePair::KeyKind::BackgroundClip;
            }
            if (memcmp(data, "border-collapse", 15) == 0) {
                return CSSStyleValuePair::KeyKind::BorderCollapse;
            }
            break;
        case 'l':
            if (memcmp(data, "list-style-type", 15) == 0) {
                return CSSStyleValuePair::KeyKind::ListStyleType;
            }
            break;
        case 'o':
            if (memcmp(data, "object-position", 15) == 0) {
                return CSSStyleValuePair::KeyKind::ObjectPosition;
            }
            break;
        case 'i':
            if (memcmp(data, "image-rendering", 15) == 0) {
                return CSSStyleValuePair::KeyKind::ImageRendering;
            }
            break;
        case 'g':
            if (memcmp(data, "grid-column-end", 15) == 0) {
                return CSSStyleValuePair::KeyKind::GridColumnEnd;
            }
            if (memcmp(data, "grid-column-gap", 15) == 0) {
                return CSSStyleValuePair::KeyKind::GridColumnGap;
            }
            break;
        }
        break;
    case 16:
        // Transform-Origin
        // Background-Color
        // Background-Image
        // Border-Top-Color
        // Border-Top-Style
        // Border-Top-Width
        // List-Style-Image
        // transition-delay
        switch (data[0]) {
        case 't':
            if (memcmp(data, "transform-origin", 16) == 0) {
                return CSSStyleValuePair::KeyKind::TransformOrigin;
            }
            if (memcmp(data, "transition-delay", 16) == 0) {
                return CSSStyleValuePair::KeyKind::TransitionDelay;
            }
            break;

        case 'b':
            if (memcmp(data, "background-color", 16) == 0) {
                return CSSStyleValuePair::KeyKind::BackgroundColor;
            }
            if (memcmp(data, "background-image", 16) == 0) {
                return CSSStyleValuePair::KeyKind::BackgroundImage;
            }
            if (memcmp(data, "border-top-color", 16) == 0) {
                return CSSStyleValuePair::KeyKind::BorderTopColor;
            }
            if (memcmp(data, "border-top-style", 16) == 0) {
                return CSSStyleValuePair::KeyKind::BorderTopStyle;
            }
            if (memcmp(data, "border-top-width", 16) == 0) {
                return CSSStyleValuePair::KeyKind::BorderTopWidth;
            }
            break;
        case 'l':
            if (memcmp(data, "list-style-image", 16) == 0) {
                return CSSStyleValuePair::KeyKind::ListStyleImage;
            }
            break;
        }
        break;
    case 17:
        // Border-Left-Color
        // Border-Left-Style
        // Border-Left-Width
        // Background-Repeat
        // Background-Origin
        // Counter-Increment
        // Grid-Column-Start
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "border-left-color", 17) == 0) {
                return CSSStyleValuePair::KeyKind::BorderLeftColor;
            }
            if (memcmp(data, "border-left-style", 17) == 0) {
                return CSSStyleValuePair::KeyKind::BorderLeftStyle;
            }
            if (memcmp(data, "border-left-width", 17) == 0) {
                return CSSStyleValuePair::KeyKind::BorderLeftWidth;
            }
            if (memcmp(data, "background-repeat", 17) == 0) {
                return CSSStyleValuePair::KeyKind::BackgroundRepeat;
            }
            if (memcmp(data, "background-origin", 17) == 0) {
                return CSSStyleValuePair::KeyKind::BackgroundOrigin;
            }
            break;
        case 'c':
            if (memcmp(data, "counter-increment", 17) == 0) {
                return CSSStyleValuePair::KeyKind::CounterIncrement;
            }
            break;
        case 'g':
            if (memcmp(data, "grid-column-start", 17) == 0) {
                return CSSStyleValuePair::KeyKind::GridColumnStart;
            }
            break;
        }
        break;
    case 18:
        // Border-Image-Slice
        // Border-Image-Width
        // Border-Right-Style
        // Border-Right-Width
        // Border-Right-Color
        // Grid-Template-Rows
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "border-image-slice", 18) == 0) {
                return CSSStyleValuePair::KeyKind::BorderImageSlice;
            }
            if (memcmp(data, "border-image-width", 18) == 0) {
                return CSSStyleValuePair::KeyKind::BorderImageWidth;
            }
            if (memcmp(data, "border-right-style", 18) == 0) {
                return CSSStyleValuePair::KeyKind::BorderRightStyle;
            }
            if (memcmp(data, "border-right-width", 18) == 0) {
                return CSSStyleValuePair::KeyKind::BorderRightWidth;
            }
            if (memcmp(data, "border-right-color", 18) == 0) {
                return CSSStyleValuePair::KeyKind::BorderRightColor;
            }
            break;
        case 'g':
            if (memcmp(data, "grid-template-rows", 18) == 0) {
                return CSSStyleValuePair::KeyKind::GridTemplateRows;
            }
            break;
        }
        break;
    case 19:
        // Background-Repeat-X
        // Background-Repeat-Y
        // Border-Image-Outset
        // Border-Image-Repeat
        // Border-Image-Source
        // Border-Bottom-Style
        // Border-Bottom-Width
        // Border-Bottom-Color
        // Background-Position
        // List-Style-Position
        // Grid-Template-Areas
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "background-repeat-x", 19) == 0) {
                return CSSStyleValuePair::KeyKind::BackgroundRepeatX;
            }
            if (memcmp(data, "background-repeat-y", 19) == 0) {
                return CSSStyleValuePair::KeyKind::BackgroundRepeatY;
            }
            if (memcmp(data, "background-position", 19) == 0) {
                return CSSStyleValuePair::KeyKind::BackgroundPosition;
            }
            if (memcmp(data, "border-image-outset", 19) == 0) {
                return CSSStyleValuePair::KeyKind::BorderImageOutset;
            }
            if (memcmp(data, "border-image-repeat", 19) == 0) {
                return CSSStyleValuePair::KeyKind::BorderImageRepeat;
            }
            if (memcmp(data, "border-image-source", 19) == 0) {
                return CSSStyleValuePair::KeyKind::BorderImageSource;
            }
            if (memcmp(data, "border-bottom-style", 19) == 0) {
                return CSSStyleValuePair::KeyKind::BorderBottomStyle;
            }
            if (memcmp(data, "border-bottom-width", 19) == 0) {
                return CSSStyleValuePair::KeyKind::BorderBottomWidth;
            }
            if (memcmp(data, "border-bottom-color", 19) == 0) {
                return CSSStyleValuePair::KeyKind::BorderBottomColor;
            }
            break;
        case 'l':
            if (memcmp(data, "list-style-position", 19) == 0) {
                return CSSStyleValuePair::KeyKind::ListStylePosition;
            }
            break;
        case 't':
            if (memcmp(data, "transition-property", 19) == 0) {
                return CSSStyleValuePair::KeyKind::TransitionProperty;
            }
            if (memcmp(data, "transition-duration", 19) == 0) {
                return CSSStyleValuePair::KeyKind::TransitionDuration;
            }
            break;
        case 'g':
            if (memcmp(data, "grid-template-areas", 19) == 0) {
                return CSSStyleValuePair::KeyKind::GridTemplateAreas;
            }
            break;
        }
        break;

    case 20:
        // box-decoration-break
        // text-decoration-line
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "box-decoration-break", 20) == 0) {
                return CSSStyleValuePair::KeyKind::BoxDecorationBreak;
            }
            break;
        case 't':
            if (memcmp(data, "text-decoration-line", 20) == 0) {
                return CSSStyleValuePair::KeyKind::TextDecorationLine;
            }
            break;
        }
        break;
    case 21:
        // background-position-x
        // background-position-y
        // background-attachment
        // grid-template-columns
        // text-decoration-color
        // text-decoration-style
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "background-position-x", 21) == 0) {
                return CSSStyleValuePair::KeyKind::BackgroundPositionX;
            }
            if (memcmp(data, "background-position-y", 21) == 0) {
                return CSSStyleValuePair::KeyKind::BackgroundPositionY;
            }
            if (memcmp(data, "background-attachment", 21) == 0) {
                return CSSStyleValuePair::KeyKind::BackgroundAttachment;
            }
            break;
        case 'g':
            if (memcmp(data, "grid-template-columns", 21) == 0) {
                return CSSStyleValuePair::KeyKind::GridTemplateColumns;
            }
            break;
        case 't':
            if (memcmp(data, "text-decoration-color", 21) == 0) {
                return CSSStyleValuePair::KeyKind::TextDecorationColor;
            }
            if (memcmp(data, "text-decoration-style", 21) == 0) {
                return CSSStyleValuePair::KeyKind::TextDecorationStyle;
            }
            break;
        }
        break;
    case 22:
        // border-top-left-radius
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "border-top-left-radius", 22) == 0) {
                return CSSStyleValuePair::KeyKind::BorderTopLeftRadius;
            }
            break;
        }
        break;
    case 23:
        // border-top-right-radius
        // text-underline-position
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "border-top-right-radius", 23) == 0) {
                return CSSStyleValuePair::KeyKind::BorderTopRightRadius;
            }
            break;
        case 't':
            if (memcmp(data, "text-underline-position", 23) == 0) {
                return CSSStyleValuePair::KeyKind::TextUnderlinePosition;
            }
            break;
        }
        break;
    case 25:
        // border-bottom-left-radius
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "border-bottom-left-radius", 25) == 0) {
                return CSSStyleValuePair::KeyKind::BorderBottomLeftRadius;
            }
            break;
        }
        break;
    case 26:
        // border-bottom-right-radius
        // transition-timing-function
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "border-bottom-right-radius", 26) == 0) {
                return CSSStyleValuePair::KeyKind::BorderBottomRightRadius;
            }
            break;
        case 't':
            if (memcmp(data, "transition-timing-function", 26) == 0) {
                return CSSStyleValuePair::KeyKind::TransitionTimingFunction;
            }
            break;
        }
        break;
    }

    // https://www.w3.org/TR/css-variables-1/#defining-variables
    if (length >= 2 && data[0] == '-' && data[1] == '-')
        return CSSStyleValuePair::KeyKind::CustomProperty;

    return CSSStyleValuePair::KeyKind::Unknown;
}

CSSStyleValuePair::KeyKind lookupCSSStyleCamelCase(const char* data,
                                                   unsigned length)
{
    switch (length) {
    case 3:
        if (memcmp(data, "all", 3) == 0) {
            return CSSStyleValuePair::KeyKind::All;
        }
        if (memcmp(data, "top", 3) == 0) {
            return CSSStyleValuePair::KeyKind::Top;
        }
        if (memcmp(data, "src", 3) == 0) {
            return CSSStyleValuePair::KeyKind::Src;
        }
        break;
    case 4:
        switch (data[0]) {
        case 'f':
            if (memcmp(data, "font", 4) == 0) {
                return CSSStyleValuePair::KeyKind::Font;
            }
            if (memcmp(data, "flex", 4) == 0) {
                return CSSStyleValuePair::KeyKind::Flex;
            }
            if (memcmp(data, "fill", 4) == 0) {
                return CSSStyleValuePair::KeyKind::Fill;
            }
            break;
        case 'l':
            if (memcmp(data, "left", 4) == 0) {
                return CSSStyleValuePair::KeyKind::Left;
            }
            break;
        case 'c':
            if (memcmp(data, "clip", 4) == 0) {
                return CSSStyleValuePair::KeyKind::Clip;
            }
            break;
        }
        break;
    case 5:
        switch (data[0]) {
        case 'c':
            if (memcmp(data, "color", 5) == 0) {
                return CSSStyleValuePair::KeyKind::Color;
            }
            if (memcmp(data, "clear", 5) == 0) {
                return CSSStyleValuePair::KeyKind::Clear;
            }
            break;
        case 'f':
            if (memcmp(data, "float", 5) == 0) {
                return CSSStyleValuePair::KeyKind::Float;
            }
            break;
        case 'w':
            if (memcmp(data, "width", 5) == 0) {
                return CSSStyleValuePair::KeyKind::Width;
            }
            break;
        case 'r':
            if (memcmp(data, "right", 5) == 0) {
                return CSSStyleValuePair::KeyKind::Right;
            }
            break;
        case 'o':
            if (memcmp(data, "order", 5) == 0) {
                return CSSStyleValuePair::KeyKind::Order;
            }
            break;
        }
        break;
    case 6:
        switch (data[0]) {
        case 'b':
            switch (data[1]) {
            case 'o':
                if (memcmp(data, "bottom", 6) == 0) {
                    return CSSStyleValuePair::KeyKind::Bottom;
                }
                if (memcmp(data, "border", 6) == 0) {
                    return CSSStyleValuePair::KeyKind::Border;
                }
                break;
            }
            break;
        case 'f':
            if (memcmp(data, "filter", 6) == 0) {
                return CSSStyleValuePair::KeyKind::Filter;
            }
            break;
        case 'h':
            if (memcmp(data, "height", 6) == 0) {
                return CSSStyleValuePair::KeyKind::Height;
            }
            break;
        case 'm':
            if (memcmp(data, "margin", 6) == 0) {
                return CSSStyleValuePair::KeyKind::Margin;
            }
            break;
        case 'z':
            if (memcmp(data, "zIndex", 6) == 0) {
                return CSSStyleValuePair::KeyKind::ZIndex;
            }
            break;
        case 'r':
            if (memcmp(data, "resize", 6) == 0) {
                return CSSStyleValuePair::KeyKind::Resize;
            }
            break;
        case 's':
            if (memcmp(data, "stroke", 6) == 0) {
                return CSSStyleValuePair::KeyKind::Stroke;
            }
            break;
        }
        break;
    case 7:
        switch (data[0]) {
        case 'c':
            if (memcmp(data, "content", 7) == 0) {
                return CSSStyleValuePair::KeyKind::Content;
            }
            break;
        case 'd':
            if (memcmp(data, "display", 7) == 0) {
                return CSSStyleValuePair::KeyKind::Display;
            }
            break;
        case 'h':
            if (memcmp(data, "hyphens", 7) == 0) {
                return CSSStyleValuePair::KeyKind::Hyphens;
            }
            break;
        case 'p':
            if (memcmp(data, "padding", 7) == 0) {
                return CSSStyleValuePair::KeyKind::Padding;
            }
            break;
        case 'o':
            if (memcmp(data, "opacity", 7) == 0) {
                return CSSStyleValuePair::KeyKind::Opacity;
            }
            if (memcmp(data, "outline", 7) == 0) {
                return CSSStyleValuePair::KeyKind::Outline;
            }
            break;
        case 'g':
            if (memcmp(data, "gridGap", 7) == 0) {
                return CSSStyleValuePair::KeyKind::GridGap;
            }
            if (memcmp(data, "gridRow", 7) == 0) {
                return CSSStyleValuePair::KeyKind::GridRow;
            }
            break;
        }
        break;
    case 8:
        switch (data[0]) {
        case 'c':
            if (memcmp(data, "cssFloat", 8) == 0) {
                return CSSStyleValuePair::KeyKind::Float;
            }
            break;
        case 'f':
            if (memcmp(data, "fontSize", 8) == 0) {
                return CSSStyleValuePair::KeyKind::FontSize;
            }
            if (memcmp(data, "flexWrap", 8) == 0) {
                return CSSStyleValuePair::KeyKind::FlexWrap;
            }
            if (memcmp(data, "flexFlow", 8) == 0) {
                return CSSStyleValuePair::KeyKind::FlexFlow;
            }
            if (memcmp(data, "flexGrow", 8) == 0) {
                return CSSStyleValuePair::KeyKind::FlexGrow;
            }
            if (memcmp(data, "fillRule", 8) == 0) {
                return CSSStyleValuePair::KeyKind::FillRule;
            }
            break;
        case 'm':
            if (memcmp(data, "maxWidth", 8) == 0) {
                return CSSStyleValuePair::KeyKind::MaxWidth;
            }
            if (memcmp(data, "minWidth", 8) == 0) {
                return CSSStyleValuePair::KeyKind::MinWidth;
            }
            if (memcmp(data, "masksize", 8) == 0) {
                return CSSStyleValuePair::KeyKind::MaskSize;
            }
            break;
        case 'p':
            if (memcmp(data, "position", 8) == 0) {
                return CSSStyleValuePair::KeyKind::Position;
            }
            break;
        case 'o':
            if (memcmp(data, "overflow", 8) == 0) {
                return CSSStyleValuePair::KeyKind::Overflow;
            }
            break;
        case 'w':
            if (memcmp(data, "wordWrap", 8) == 0) {
                return CSSStyleValuePair::KeyKind::WordWrap;
            }
            break;
        case 'g':
            if (memcmp(data, "gridArea", 8) == 0) {
                return CSSStyleValuePair::KeyKind::GridArea;
            }
            break;
        }
        break;
    case 9:
        switch (data[0]) {
        case 'a':
            if (memcmp(data, "alignSelf", 9) == 0) {
                return CSSStyleValuePair::KeyKind::AlignSelf;
            }
            break;
        case 'b':
            if (memcmp(data, "borderTop", 9) == 0) {
                return CSSStyleValuePair::KeyKind::BorderTop;
            }
            if (memcmp(data, "boxSizing", 9) == 0) {
                return CSSStyleValuePair::KeyKind::BoxSizing;
            }
            if (memcmp(data, "boxShadow", 9) == 0) {
                return CSSStyleValuePair::KeyKind::BoxShadow;
            }
            break;
        case 't':
            if (memcmp(data, "transform", 9) == 0) {
                return CSSStyleValuePair::KeyKind::Transform;
            }
            if (memcmp(data, "textAlign", 9) == 0) {
                return CSSStyleValuePair::KeyKind::TextAlign;
            }
            break;
        case 'd':
            if (memcmp(data, "direction", 9) == 0) {
                return CSSStyleValuePair::KeyKind::Direction;
            }
            break;
        case 'f':
            if (memcmp(data, "fontStyle", 9) == 0) {
                return CSSStyleValuePair::KeyKind::FontStyle;
            }
            if (memcmp(data, "flexBasis", 9) == 0) {
                return CSSStyleValuePair::KeyKind::FlexBasis;
            }
            break;
        case 'l':
            if (memcmp(data, "listStyle", 9) == 0) {
                return CSSStyleValuePair::KeyKind::ListStyle;
            }
            if (memcmp(data, "lineBreak", 9) == 0) {
                return CSSStyleValuePair::KeyKind::LineBreak;
            }
            break;
        case 'm':
            if (memcmp(data, "marginTop", 9) == 0) {
                return CSSStyleValuePair::KeyKind::MarginTop;
            }
            if (memcmp(data, "maskImage", 9) == 0) {
                return CSSStyleValuePair::KeyKind::MaskImage;
            }
            if (memcmp(data, "maxHeight", 9) == 0) {
                return CSSStyleValuePair::KeyKind::MaxHeight;
            }
            if (memcmp(data, "minHeight", 9) == 0) {
                return CSSStyleValuePair::KeyKind::MinHeight;
            }
            break;
        case 'o':
            if (memcmp(data, "objectFit", 9) == 0) {
                return CSSStyleValuePair::KeyKind::ObjectFit;
            }
            if (memcmp(data, "overflowX", 9) == 0) {
                return CSSStyleValuePair::KeyKind::OverflowX;
            }
            if (memcmp(data, "overflowY", 9) == 0) {
                return CSSStyleValuePair::KeyKind::OverflowY;
            }
            break;
        case 'w':
            if (memcmp(data, "wordBreak", 9) == 0) {
                return CSSStyleValuePair::KeyKind::WordBreak;
            }
            break;
        }
        break;
    case 10:
        switch (data[0]) {
        case 'a':
            if (memcmp(data, "alignItems", 10) == 0) {
                return CSSStyleValuePair::KeyKind::AlignItems;
            }
            break;
        case 'b':
            if (memcmp(data, "background", 10) == 0) {
                return CSSStyleValuePair::KeyKind::Background;
            }
            if (memcmp(data, "borderLeft", 10) == 0) {
                return CSSStyleValuePair::KeyKind::BorderLeft;
            }
            break;
        case 'c':
            if (memcmp(data, "caretColor", 10) == 0) {
                return CSSStyleValuePair::KeyKind::CaretColor;
            }
            break;
        case 'e':
            if (memcmp(data, "emptyCells", 10) == 0) {
                return CSSStyleValuePair::KeyKind::EmptyCells;
            }
            break;
        case 'f':
            if (memcmp(data, "fontWeight", 10) == 0) {
                return CSSStyleValuePair::KeyKind::FontWeight;
            }
            if (memcmp(data, "fontFamily", 10) == 0) {
                return CSSStyleValuePair::KeyKind::FontFamily;
            }
            if (memcmp(data, "flexShrink", 10) == 0) {
                return CSSStyleValuePair::KeyKind::FlexShrink;
            }
            break;
        case 'g':
            if (memcmp(data, "gridRowEnd", 10) == 0) {
                return CSSStyleValuePair::KeyKind::GridRowEnd;
            }
            if (memcmp(data, "gridRowGap", 10) == 0) {
                return CSSStyleValuePair::KeyKind::GridRowGap;
            }
            if (memcmp(data, "gridColumn", 10) == 0) {
                return CSSStyleValuePair::KeyKind::GridColumn;
            }
            break;
        case 'l':
            if (memcmp(data, "lineHeight", 10) == 0) {
                return CSSStyleValuePair::KeyKind::LineHeight;
            }
            break;
        case 'm':
            if (memcmp(data, "marginLeft", 10) == 0) {
                return CSSStyleValuePair::KeyKind::MarginLeft;
            }
            break;
        case 'p':
            if (memcmp(data, "paddingTop", 10) == 0) {
                return CSSStyleValuePair::KeyKind::PaddingTop;
            }
            break;
        case 't':
            if (memcmp(data, "transition", 10) == 0) {
                return CSSStyleValuePair::KeyKind::Transition;
            }
            if (memcmp(data, "textIndent", 10) == 0) {
                return CSSStyleValuePair::KeyKind::TextIndent;
            }
            if (memcmp(data, "textShadow", 10) == 0) {
                return CSSStyleValuePair::KeyKind::TextShadow;
            }
            break;
        case 'u':
            if (memcmp(data, "userSelect", 10) == 0) {
                return CSSStyleValuePair::KeyKind::UserSelect;
            }
            break;
        case 'v':
            if (memcmp(data, "visibility", 10) == 0) {
                return CSSStyleValuePair::KeyKind::Visibility;
            }
            break;
        case 'w':
            if (memcmp(data, "willChange", 10) == 0) {
                return CSSStyleValuePair::KeyKind::WillChange;
            }
            if (memcmp(data, "whiteSpace", 10) == 0) {
                return CSSStyleValuePair::KeyKind::WhiteSpace;
            }
            break;
        }
        break;
    case 11:
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "borderImage", 11) == 0) {
                return CSSStyleValuePair::KeyKind::BorderImage;
            }
            if (memcmp(data, "borderStyle", 11) == 0) {
                return CSSStyleValuePair::KeyKind::BorderStyle;
            }
            if (memcmp(data, "borderWidth", 11) == 0) {
                return CSSStyleValuePair::KeyKind::BorderWidth;
            }
            if (memcmp(data, "borderColor", 11) == 0) {
                return CSSStyleValuePair::KeyKind::BorderColor;
            }
            if (memcmp(data, "borderRight", 11) == 0) {
                return CSSStyleValuePair::KeyKind::BorderRight;
            }
            break;
        case 'c':
            if (memcmp(data, "captionSide", 11) == 0) {
                return CSSStyleValuePair::KeyKind::CaptionSide;
            }
            break;
        case 'p':
            if (memcmp(data, "paddingLeft", 11) == 0) {
                return CSSStyleValuePair::KeyKind::PaddingLeft;
            }
            break;
        case 'm':
            if (memcmp(data, "marginRight", 11) == 0) {
                return CSSStyleValuePair::KeyKind::MarginRight;
            }
            break;
        case 't':
            if (memcmp(data, "tableLayout", 11) == 0) {
                return CSSStyleValuePair::KeyKind::TableLayout;
            }
            break;
        case 'u':
            if (memcmp(data, "unicodeBidi", 11) == 0) {
                return CSSStyleValuePair::KeyKind::UnicodeBidi;
            }
            break;
        case 'f':
            if (memcmp(data, "fillOpacity", 11) == 0) {
                return CSSStyleValuePair::KeyKind::FillOpacity;
            }
            if (memcmp(data, "fontKerning", 11) == 0) {
                return CSSStyleValuePair::KeyKind::FontKerning;
            }
            break;
        case 's':
            if (memcmp(data, "strokeWidth", 11) == 0) {
                return CSSStyleValuePair::KeyKind::StrokeWidth;
            }
            break;
        case 'w':
            if (memcmp(data, "wordSpacing", 11) == 0) {
                return CSSStyleValuePair::KeyKind::WordSpacing;
            }
            break;
        }
        break;
    case 12:
        switch (data[0]) {
        case 'a':
            if (memcmp(data, "alignContent", 12) == 0) {
                return CSSStyleValuePair::KeyKind::AlignContent;
            }
            break;
        case 'p':
            if (memcmp(data, "paddingRight", 12) == 0) {
                return CSSStyleValuePair::KeyKind::PaddingRight;
            }
            break;
        case 'm':
            if (memcmp(data, "marginBottom", 12) == 0) {
                return CSSStyleValuePair::KeyKind::MarginBottom;
            }
            break;
        case 'b':
            if (memcmp(data, "borderBottom", 12) == 0) {
                return CSSStyleValuePair::KeyKind::BorderBottom;
            }
            if (memcmp(data, "borderRadius", 12) == 0) {
                return CSSStyleValuePair::KeyKind::BorderRadius;
            }
            break;
        case 'c':
            if (memcmp(data, "counterReset", 12) == 0) {
                return CSSStyleValuePair::KeyKind::CounterReset;
            }
            break;
        case 'o':
            if (memcmp(data, "outlineColor", 12) == 0) {
                return CSSStyleValuePair::KeyKind::OutlineColor;
            }
            if (memcmp(data, "outlineWidth", 12) == 0) {
                return CSSStyleValuePair::KeyKind::OutlineWidth;
            }
            if (memcmp(data, "outlineStyle", 12) == 0) {
                return CSSStyleValuePair::KeyKind::OutlineStyle;
            }
            if (memcmp(data, "overflowWrap", 12) == 0) {
                return CSSStyleValuePair::KeyKind::OverflowWrap;
            }
            break;
        case 't':
            if (memcmp(data, "textOverflow", 12) == 0) {
                return CSSStyleValuePair::KeyKind::TextOverflow;
            }
            break;
        case 'g':
            if (memcmp(data, "gridRowStart", 12) == 0) {
                return CSSStyleValuePair::KeyKind::GridRowStart;
            }
            break;
        }
        break;
    case 13:
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "borderSpacing", 13) == 0) {
                return CSSStyleValuePair::KeyKind::BorderSpacing;
            }
            break;
        case 'f':
            if (memcmp(data, "flexDirection", 13) == 0) {
                return CSSStyleValuePair::KeyKind::FlexDirection;
            }
            break;
        case 'v':
            if (memcmp(data, "verticalAlign", 13) == 0) {
                return CSSStyleValuePair::KeyKind::VerticalAlign;
            }
            break;
        case 'p':
            if (memcmp(data, "paddingBottom", 13) == 0) {
                return CSSStyleValuePair::KeyKind::PaddingBottom;
            }
            if (memcmp(data, "pointerEvents", 13) == 0) {
                return CSSStyleValuePair::KeyKind::PointerEvents;
            }
            break;
        case 'o':
            if (memcmp(data, "outlineOffset", 13) == 0) {
                return CSSStyleValuePair::KeyKind::OutlineOffset;
            }
            break;
        case 't':
            if (memcmp(data, "textTransform", 13) == 0) {
                return CSSStyleValuePair::KeyKind::TextTransform;
            }
            break;
        case 'l':
            if (memcmp(data, "listStyleType", 13) == 0) {
                return CSSStyleValuePair::KeyKind::ListStyleType;
            }
            if (memcmp(data, "letterSpacing", 13) == 0) {
                return CSSStyleValuePair::KeyKind::LetterSpacing;
            }
            break;
        case 'g':
            if (memcmp(data, "gridColumnEnd", 13) == 0) {
                return CSSStyleValuePair::KeyKind::GridColumnEnd;
            }
            if (memcmp(data, "gridColumnGap", 13) == 0) {
                return CSSStyleValuePair::KeyKind::GridColumnGap;
            }
            break;
        }
        break;
    case 14:
        switch (data[0]) {
        case 'j':
            if (memcmp(data, "justifyContent", 14) == 0) {
                return CSSStyleValuePair::KeyKind::JustifyContent;
            }
            break;
        case 'l':
            if (memcmp(data, "listStyleImage", 14) == 0) {
                return CSSStyleValuePair::KeyKind::ListStyleImage;
            }
            break;
        case 't':
            if (memcmp(data, "textDecoration", 14) == 0) {
                return CSSStyleValuePair::KeyKind::TextDecoration;
            }
            break;
        case 'b':
            if (memcmp(data, "backgroundSize", 14) == 0) {
                return CSSStyleValuePair::KeyKind::BackgroundSize;
            }
            if (memcmp(data, "backgroundClip", 14) == 0) {
                return CSSStyleValuePair::KeyKind::BackgroundClip;
            }
            if (memcmp(data, "borderTopColor", 14) == 0) {
                return CSSStyleValuePair::KeyKind::BorderTopColor;
            }
            if (memcmp(data, "borderTopStyle", 14) == 0) {
                return CSSStyleValuePair::KeyKind::BorderTopStyle;
            }
            if (memcmp(data, "borderTopWidth", 14) == 0) {
                return CSSStyleValuePair::KeyKind::BorderTopWidth;
            }
            if (memcmp(data, "borderCollapse", 14) == 0) {
                return CSSStyleValuePair::KeyKind::BorderCollapse;
            }
            break;
        case 'o':
            if (memcmp(data, "objectPosition", 14) == 0) {
                return CSSStyleValuePair::KeyKind::ObjectPosition;
            }
            break;
        case 'i':
            if (memcmp(data, "imageRendering", 14) == 0) {
                return CSSStyleValuePair::KeyKind::ImageRendering;
            }
            break;
        }
        break;
    case 15:
        switch (data[0]) {
        case 't':
            if (memcmp(data, "transformOrigin", 15) == 0) {
                return CSSStyleValuePair::KeyKind::TransformOrigin;
            }
            if (memcmp(data, "transitionDelay", 15) == 0) {
                return CSSStyleValuePair::KeyKind::TransitionDelay;
            }
            break;
        case 'b':
            if (memcmp(data, "backgroundColor", 15) == 0) {
                return CSSStyleValuePair::KeyKind::BackgroundColor;
            }
            if (memcmp(data, "backgroundImage", 15) == 0) {
                return CSSStyleValuePair::KeyKind::BackgroundImage;
            }
            if (memcmp(data, "borderLeftColor", 15) == 0) {
                return CSSStyleValuePair::KeyKind::BorderLeftColor;
            }
            if (memcmp(data, "borderLeftStyle", 15) == 0) {
                return CSSStyleValuePair::KeyKind::BorderLeftStyle;
            }
            if (memcmp(data, "borderLeftWidth", 15) == 0) {
                return CSSStyleValuePair::KeyKind::BorderLeftWidth;
            }
            break;
        case 'g':
            if (memcmp(data, "gridColumnStart", 15) == 0) {
                return CSSStyleValuePair::KeyKind::GridColumnStart;
            }
            break;
        }
        break;
    case 16:
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "backgroundRepeat", 16) == 0) {
                return CSSStyleValuePair::KeyKind::BackgroundRepeat;
            }
            if (memcmp(data, "backgroundOrigin", 16) == 0) {
                return CSSStyleValuePair::KeyKind::BackgroundOrigin;
            }
            if (memcmp(data, "borderImageSlice", 16) == 0) {
                return CSSStyleValuePair::KeyKind::BorderImageSlice;
            }
            if (memcmp(data, "borderImageWidth", 16) == 0) {
                return CSSStyleValuePair::KeyKind::BorderImageWidth;
            }
            if (memcmp(data, "borderRightStyle", 16) == 0) {
                return CSSStyleValuePair::KeyKind::BorderRightStyle;
            }
            if (memcmp(data, "borderRightWidth", 16) == 0) {
                return CSSStyleValuePair::KeyKind::BorderRightWidth;
            }
            if (memcmp(data, "borderRightColor", 16) == 0) {
                return CSSStyleValuePair::KeyKind::BorderRightColor;
            }
            break;
        case 'c':
            if (memcmp(data, "counterIncrement", 16) == 0) {
                return CSSStyleValuePair::KeyKind::CounterIncrement;
            }
            break;
        case 'g':
            if (memcmp(data, "gridTemplateRows", 16) == 0) {
                return CSSStyleValuePair::KeyKind::GridTemplateRows;
            }
            break;
        }
        break;
    case 17:
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "backgroundRepeatX", 17) == 0) {
                return CSSStyleValuePair::KeyKind::BackgroundRepeatX;
            }
            if (memcmp(data, "backgroundRepeatY", 17) == 0) {
                return CSSStyleValuePair::KeyKind::BackgroundRepeatY;
            }
            if (memcmp(data, "borderImageOutset", 17) == 0) {
                return CSSStyleValuePair::KeyKind::BorderImageOutset;
            }
            if (memcmp(data, "borderImageRepeat", 17) == 0) {
                return CSSStyleValuePair::KeyKind::BorderImageRepeat;
            }
            if (memcmp(data, "borderImageSource", 17) == 0) {
                return CSSStyleValuePair::KeyKind::BorderImageSource;
            }
            if (memcmp(data, "borderBottomStyle", 17) == 0) {
                return CSSStyleValuePair::KeyKind::BorderBottomStyle;
            }
            if (memcmp(data, "borderBottomWidth", 17) == 0) {
                return CSSStyleValuePair::KeyKind::BorderBottomWidth;
            }
            if (memcmp(data, "borderBottomColor", 17) == 0) {
                return CSSStyleValuePair::KeyKind::BorderBottomColor;
            }
            break;
        case 'l':
            if (memcmp(data, "listStylePosition", 17) == 0) {
                return CSSStyleValuePair::KeyKind::ListStylePosition;
            }
            break;
        case 'g':
            if (memcmp(data, "gridTemplateAreas", 17) == 0) {
                return CSSStyleValuePair::KeyKind::GridTemplateAreas;
            }
            break;
        }
        break;
    case 18:
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "backgroundPosition", 18) == 0) {
                return CSSStyleValuePair::KeyKind::BackgroundPosition;
            }
            if (memcmp(data, "boxDecorationBreak", 18) == 0) {
                return CSSStyleValuePair::KeyKind::BoxDecorationBreak;
            }
            break;
        case 't':
            if (memcmp(data, "transitionProperty", 18) == 0) {
                return CSSStyleValuePair::KeyKind::TransitionProperty;
            }
            if (memcmp(data, "transitionDuration", 18) == 0) {
                return CSSStyleValuePair::KeyKind::TransitionDuration;
            }
            if (memcmp(data, "textDecorationLine", 18) == 0) {
                return CSSStyleValuePair::KeyKind::TextDecorationLine;
            }
            break;
        }
        break;
    case 19:
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "backgroundPositionX", 19) == 0) {
                return CSSStyleValuePair::KeyKind::BackgroundPositionX;
            }
            if (memcmp(data, "backgroundPositionY", 19) == 0) {
                return CSSStyleValuePair::KeyKind::BackgroundPositionY;
            }
            if (memcmp(data, "borderTopLeftRadius", 19) == 0) {
                return CSSStyleValuePair::KeyKind::BorderTopLeftRadius;
            }
            break;
        case 't':
            if (memcmp(data, "textDecorationColor", 19) == 0) {
                return CSSStyleValuePair::KeyKind::TextDecorationColor;
            }
            if (memcmp(data, "textDecorationStyle", 19) == 0) {
                return CSSStyleValuePair::KeyKind::TextDecorationStyle;
            }
            break;
        case 'g':
            if (memcmp(data, "gridTemplateColumns", 19) == 0) {
                return CSSStyleValuePair::KeyKind::GridTemplateColumns;
            }
        }
        break;
    case 20:
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "borderTopRightRadius", 20) == 0) {
                return CSSStyleValuePair::KeyKind::BorderTopRightRadius;
            }
            if (memcmp(data, "backgroundAttachment", 20) == 0) {
                return CSSStyleValuePair::KeyKind::BackgroundAttachment;
            }
            break;
        }
        break;
    case 21:
        switch (data[0]) {
        case 't':
            if (memcmp(data, "textUnderlinePosition", 21) == 0) {
                return CSSStyleValuePair::KeyKind::TextUnderlinePosition;
            }
            break;
        }
        break;
    case 22:
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "borderBottomLeftRadius", 22) == 0) {
                return CSSStyleValuePair::KeyKind::BorderBottomLeftRadius;
            }
            break;
        }
        break;
    case 23:
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "borderBottomRightRadius", 23) == 0) {
                return CSSStyleValuePair::KeyKind::BorderBottomRightRadius;
            }
            break;
        }
        break;
    case 24:
        switch (data[0]) {
        case 't':
            if (memcmp(data, "transitionTimingFunction", 24) == 0) {
                return CSSStyleValuePair::KeyKind::TransitionTimingFunction;
            }
            break;
        }
        break;
    }

    return CSSStyleValuePair::KeyKind::Unknown;
}

UnitType lookupUnitType(const char* data, unsigned length)
{
    switch (length) {
    case 1:
        switch (data[0]) {
        case 's':
            if (memcmp(data, "s", 1) == 0) {
                return UnitType::Seconds;
            }
        }
        break;
    case 2:
        switch (data[0]) {
        case 'c':
            if (memcmp(data, "ch", 2) == 0) {
                return UnitType::Chs;
            }
            if (memcmp(data, "cm", 2) == 0) {
                return UnitType::Centimeters;
            }
            break;
        case 'e':
            if (memcmp(data, "em", 2) == 0) {
                return UnitType::Ems;
            }
            if (memcmp(data, "ex", 2) == 0) {
                return UnitType::Exs;
            }
            break;
        case 'f':
            if (memcmp(data, "fr", 2) == 0) {
                return UnitType::Fraction;
            }
            break;
        case 'h':
            if (memcmp(data, "hz", 2) == 0) {
                return UnitType::Hertz;
            }
            break;
        case 'i':
            if (memcmp(data, "in", 2) == 0) {
                return UnitType::Inches;
            }
            break;
        case 'm':
            if (memcmp(data, "mm", 2) == 0) {
                return UnitType::Millimeters;
            }
            if (memcmp(data, "ms", 2) == 0) {
                return UnitType::Milliseconds;
            }
            break;
        case 'p':
            if (memcmp(data, "pc", 2) == 0) {
                return UnitType::Picas;
            }
            if (memcmp(data, "pt", 2) == 0) {
                return UnitType::Points;
            }
            if (memcmp(data, "px", 2) == 0) {
                return UnitType::Pixels;
            }
            break;
        case 'v':
            if (memcmp(data, "vh", 2) == 0) {
                return UnitType::ViewportHeight;
            }
            if (memcmp(data, "vw", 2) == 0) {
                return UnitType::ViewportWidth;
            }
            break;
        }
        break;
    case 3:
        switch (data[0]) {
        case 'd':
            if (memcmp(data, "deg", 3) == 0) {
                return UnitType::Degrees;
            }
            if (memcmp(data, "dpi", 3) == 0) {
                return UnitType::DotsPerInch;
            }
            break;
        case 'k':
            if (memcmp(data, "khz", 3) == 0) {
                return UnitType::Kilohertz;
            }
            break;
        case 'r':
            if (memcmp(data, "rad", 3) == 0) {
                return UnitType::Radians;
            }
            if (memcmp(data, "rem", 3) == 0) {
                return UnitType::Rems;
            }
            break;
        }
        break;
    case 4:
        switch (data[0]) {
        case 'd':
            if (memcmp(data, "dpcm", 4) == 0) {
                return UnitType::DotsPerCentimeter;
            }
            if (memcmp(data, "dppx", 4) == 0) {
                return UnitType::DotsPerPixel;
            }
            break;
        case 'g':
            if (memcmp(data, "grad", 4) == 0) {
                return UnitType::Gradians;
            }
            break;
        case 't':
            if (memcmp(data, "turn", 4) == 0) {
                return UnitType::Turns;
            }
            break;
        case 'v':
            if (memcmp(data, "vmax", 4) == 0) {
                return UnitType::ViewportMax;
            }
            if (memcmp(data, "vmin", 4) == 0) {
                return UnitType::ViewportMin;
            }
            break;
        }
        break;
    }
    return UnitType::UnknownType;
}
}
