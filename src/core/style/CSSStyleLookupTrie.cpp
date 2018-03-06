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

CSSStyleKind lookupCSSStyle(const char* data, unsigned length)
{
    switch (length) {
    case 1:
        if (memcmp(data, "x", 1) == 0) {
            return CSSStyleKind::X;
        } else if (memcmp(data, "y", 1) == 0) {
            return CSSStyleKind::Y;
        } else if (memcmp(data, "d", 1) == 0) {
            return CSSStyleKind::D;
        } else if (memcmp(data, "r", 1) == 0) {
            return CSSStyleKind::R;
        }
        break;
    case 2:
        if (memcmp(data, "cx", 2) == 0) {
            return CSSStyleKind::CX;
        } else if (memcmp(data, "cy", 2) == 0) {
            return CSSStyleKind::CY;
        } else if (memcmp(data, "rx", 2) == 0) {
            return CSSStyleKind::RX;
        } else if (memcmp(data, "ry", 2) == 0) {
            return CSSStyleKind::RY;
        }
        break;
    case 3:
        // Top
        // Src
        if (memcmp(data, "top", 3) == 0) {
            return CSSStyleKind::Top;
        }
        if (memcmp(data, "src", 3) == 0) {
            return CSSStyleKind::Src;
        }
        break;
    case 4:
        // Font
        // Flex
        // Left
        // Fill
        switch (data[0]) {
        case 'f':
            if (memcmp(data, "font", 4) == 0) {
                return CSSStyleKind::Font;
            }
            if (memcmp(data, "flex", 4) == 0) {
                return CSSStyleKind::Flex;
            }
            if (memcmp(data, "fill", 4) == 0) {
                return CSSStyleKind::Fill;
            }
            break;
        case 'l':
            if (memcmp(data, "left", 4) == 0) {
                return CSSStyleKind::Left;
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
                return CSSStyleKind::Color;
            }
            if (memcmp(data, "clear", 5) == 0) {
                return CSSStyleKind::Clear;
            }
            break;
        case 'f':
            if (memcmp(data, "float", 5) == 0) {
                return CSSStyleKind::Float;
            }
            break;
        case 'w':
            if (memcmp(data, "width", 5) == 0) {
                return CSSStyleKind::Width;
            }
            break;
        case 'r':
            if (memcmp(data, "right", 5) == 0) {
                return CSSStyleKind::Right;
            }
            break;
        case 'o':
            if (memcmp(data, "order", 5) == 0) {
                return CSSStyleKind::Order;
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
        switch (data[0]) {
        case 'b':
            switch (data[1]) {
            case 'o':
                if (memcmp(data, "bottom", 6) == 0) {
                    return CSSStyleKind::Bottom;
                }
                if (memcmp(data, "border", 6) == 0) {
                    return CSSStyleKind::Border;
                }
                break;
            }
            break;
        case 'c':
            if (memcmp(data, "cursor", 6) == 0) {
                return CSSStyleKind::Cursor;
            }
            break;
        case 'h':
            if (memcmp(data, "height", 6) == 0) {
                return CSSStyleKind::Height;
            }
            break;
        case 'm':
            if (memcmp(data, "margin", 6) == 0) {
                return CSSStyleKind::Margin;
            }
            break;
        case 's':
            if (memcmp(data, "stroke", 6) == 0) {
                return CSSStyleKind::Stroke;
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
        switch (data[0]) {
        case 'c':
            if (memcmp(data, "content", 7) == 0) {
                return CSSStyleKind::Content;
            }
            break;
        case 'd':
            if (memcmp(data, "display", 7) == 0) {
                return CSSStyleKind::Display;
            }
            break;
        case 'p':
            if (memcmp(data, "padding", 7) == 0) {
                return CSSStyleKind::Padding;
            }
            break;
        case 'z':
            if (memcmp(data, "z-index", 7) == 0) {
                return CSSStyleKind::ZIndex;
            }
            break;
        case 'o':
            if (memcmp(data, "opacity", 7) == 0) {
                return CSSStyleKind::Opacity;
            }
            if (memcmp(data, "outline", 7) == 0) {
                return CSSStyleKind::Outline;
            }
            break;
        }
        break;
    case 8:
        // Position
        // Overflow
        switch (data[0]) {
        case 'p':
            if (memcmp(data, "position", 8) == 0) {
                return CSSStyleKind::Position;
            }
            break;
        case 'o':
            if (memcmp(data, "overflow", 8) == 0) {
                return CSSStyleKind::Overflow;
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
        switch (data[0]) {
        case 'f':
            if (memcmp(data, "font-size", 9) == 0) {
                return CSSStyleKind::FontSize;
            }
            if (memcmp(data, "flex-wrap", 9) == 0) {
                return CSSStyleKind::FlexWrap;
            }
            if (memcmp(data, "flex-flow", 9) == 0) {
                return CSSStyleKind::FlexFlow;
            }
            if (memcmp(data, "flex-grow", 9) == 0) {
                return CSSStyleKind::FlexGrow;
            }
            if (memcmp(data, "fill-rule", 9) == 0) {
                return CSSStyleKind::FillRule;
            }
            break;
        case 't':
            if (memcmp(data, "transform", 9) == 0) {
                return CSSStyleKind::Transform;
            }
            break;
        case 'd':
            if (memcmp(data, "direction", 9) == 0) {
                return CSSStyleKind::Direction;
            }
            break;
        case 'm':
            if (memcmp(data, "max-width", 9) == 0) {
                return CSSStyleKind::MaxWidth;
            }
            if (memcmp(data, "min-width", 9) == 0) {
                return CSSStyleKind::MinWidth;
            }
            if (memcmp(data, "mask-size", 9) == 0) {
                return CSSStyleKind::MaskSize;
            }
            break;
        case 'w':
            if (memcmp(data, "word-wrap", 9) == 0) {
                return CSSStyleKind::WordWrap;
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
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "background", 10) == 0) {
                return CSSStyleKind::Background;
            }
            if (memcmp(data, "border-top", 10) == 0) {
                return CSSStyleKind::BorderTop;
            }
            if (memcmp(data, "box-shadow", 10) == 0) {
                return CSSStyleKind::BoxShadow;
            }
            if (memcmp(data, "box-sizing", 10) == 0) {
                return CSSStyleKind::BoxSizing;
            }
            break;
        case 'f':
            if (memcmp(data, "font-style", 10) == 0) {
                return CSSStyleKind::FontStyle;
            }
            if (memcmp(data, "flex-basis", 10) == 0) {
                return CSSStyleKind::FlexBasis;
            }
            break;
        case 't':
            if (memcmp(data, "text-align", 10) == 0) {
                return CSSStyleKind::TextAlign;
            }
            if (memcmp(data, "transition", 10) == 0) {
                return CSSStyleKind::Transition;
            }
            break;
        case 'm':
            if (memcmp(data, "margin-top", 10) == 0) {
                return CSSStyleKind::MarginTop;
            }
            if (memcmp(data, "mask-image", 10) == 0) {
                return CSSStyleKind::MaskImage;
            }
            if (memcmp(data, "max-height", 10) == 0) {
                return CSSStyleKind::MaxHeight;
            }
            if (memcmp(data, "min-height", 10) == 0) {
                return CSSStyleKind::MinHeight;
            }
            break;
        case 'o':
            if (memcmp(data, "object-fit", 10) == 0) {
                return CSSStyleKind::ObjectFit;
            }
            if (memcmp(data, "overflow-x", 10) == 0) {
                return CSSStyleKind::OverflowX;
            }
            if (memcmp(data, "overflow-y", 10) == 0) {
                return CSSStyleKind::OverflowY;
            }
            break;
        case 'v':
            if (memcmp(data, "visibility", 10) == 0) {
                return CSSStyleKind::Visibility;
            }
            break;
        case 'a':
            if (memcmp(data, "align-self", 10) == 0) {
                return CSSStyleKind::AlignSelf;
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
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "border-left", 11) == 0) {
                return CSSStyleKind::BorderLeft;
            }
            break;
        case 'e':
            if (memcmp(data, "empty-cells", 11) == 0) {
                return CSSStyleKind::EmptyCells;
            }
            break;
        case 'f':
            if (memcmp(data, "font-weight", 11) == 0) {
                return CSSStyleKind::FontWeight;
            }
            if (memcmp(data, "font-family", 11) == 0) {
                return CSSStyleKind::FontFamily;
            }
            if (memcmp(data, "flex-shrink", 11) == 0) {
                return CSSStyleKind::FlexShrink;
            }
            break;
        case 'l':
            if (memcmp(data, "line-height", 11) == 0) {
                return CSSStyleKind::LineHeight;
            }
            break;
        case 'w':
            if (memcmp(data, "white-space", 11) == 0) {
                return CSSStyleKind::WhiteSpace;
            }
            break;
        case 'p':
            if (memcmp(data, "padding-top", 11) == 0) {
                return CSSStyleKind::PaddingTop;
            }
            break;
        case 'm':
            if (memcmp(data, "margin-left", 11) == 0) {
                return CSSStyleKind::MarginLeft;
            }
            break;
        case 't':
            if (memcmp(data, "text-indent", 11) == 0) {
                return CSSStyleKind::TextIndent;
            }
            if (memcmp(data, "text-shadow", 11) == 0) {
                return CSSStyleKind::TextShadow;
            }
            break;
        case 'a':
            if (memcmp(data, "align-items", 11) == 0) {
                return CSSStyleKind::AlignItems;
            }
            break;
        }
        break;
    case 12:
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
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "border-style", 12) == 0) {
                return CSSStyleKind::BorderStyle;
            }
            if (memcmp(data, "border-width", 12) == 0) {
                return CSSStyleKind::BorderWidth;
            }
            if (memcmp(data, "border-color", 12) == 0) {
                return CSSStyleKind::BorderColor;
            }
            if (memcmp(data, "border-right", 12) == 0) {
                return CSSStyleKind::BorderRight;
            }
            break;
        case 'c':
            if (memcmp(data, "caption-side", 12) == 0) {
                return CSSStyleKind::CaptionSide;
            }
            break;
        case 'p':
            if (memcmp(data, "padding-left", 12) == 0) {
                return CSSStyleKind::PaddingLeft;
            }
            break;
        case 'm':
            if (memcmp(data, "margin-right", 12) == 0) {
                return CSSStyleKind::MarginRight;
            }
            break;
        case 't':
            if (memcmp(data, "table-layout", 12) == 0) {
                return CSSStyleKind::TableLayout;
            }
            break;
        case 'u':
            if (memcmp(data, "unicode-bidi", 12) == 0) {
                return CSSStyleKind::UnicodeBidi;
            }
            break;
        case 'f':
            if (memcmp(data, "fill-opacity", 12) == 0) {
                return CSSStyleKind::FillOpacity;
            }
            break;
        case 's':
            if (memcmp(data, "stroke-width", 12) == 0) {
                return CSSStyleKind::StrokeWidth;
            }
            break;
        case 'w':
            if (memcmp(data, "word-spacing", 12) == 0) {
                return CSSStyleKind::WordSpacing;
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
        // Outline-Width
        // Outline-Style
        // Outline-Color
        // Overflow-Wrap
        switch (data[0]) {
        case 'p':
            if (memcmp(data, "padding-right", 13) == 0) {
                return CSSStyleKind::PaddingRight;
            }
            break;
        case 'm':
            if (memcmp(data, "margin-bottom", 13) == 0) {
                return CSSStyleKind::MarginBottom;
            }
            break;
        case 'b':
            if (memcmp(data, "border-bottom", 13) == 0) {
                return CSSStyleKind::BorderBottom;
            }
            if (memcmp(data, "border-radius", 13) == 0) {
                return CSSStyleKind::BorderRadius;
            }
            break;
        case 'a':
            if (memcmp(data, "align-content", 13) == 0) {
                return CSSStyleKind::AlignContent;
            }
            break;
        case 'o':
            if (memcmp(data, "outline-width", 13) == 0) {
                return CSSStyleKind::OutlineWidth;
            }
            if (memcmp(data, "outline-style", 13) == 0) {
                return CSSStyleKind::OutlineStyle;
            }
            if (memcmp(data, "outline-color", 13) == 0) {
                return CSSStyleKind::OutlineColor;
            }
            if (memcmp(data, "overflow-wrap", 13) == 0) {
                return CSSStyleKind::OverflowWrap;
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
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "border-spacing", 14) == 0) {
                return CSSStyleKind::BorderSpacing;
            }
            break;
        case 'f':
            if (memcmp(data, "flex-direction", 14) == 0) {
                return CSSStyleKind::FlexDirection;
            }
            break;
        case 'v':
            if (memcmp(data, "vertical-align", 14) == 0) {
                return CSSStyleKind::VerticalAlign;
            }
            break;
        case 'p':
            if (memcmp(data, "padding-bottom", 14) == 0) {
                return CSSStyleKind::PaddingBottom;
            }
            break;
        case 'o':
            if (memcmp(data, "outline-offset", 14) == 0) {
                return CSSStyleKind::OutlineOffset;
            }
            break;
        case 't':
            if (memcmp(data, "text-transform", 14) == 0) {
                return CSSStyleKind::TextTransform;
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
        switch (data[0]) {
        case 'j':
            if (memcmp(data, "justify-content", 15) == 0) {
                return CSSStyleKind::JustifyContent;
            }
            break;
        case 't':
            if (memcmp(data, "text-decoration", 15) == 0) {
                return CSSStyleKind::TextDecoration;
            }
            break;
        case 'b':
            if (memcmp(data, "background-size", 15) == 0) {
                return CSSStyleKind::BackgroundSize;
            }
            if (memcmp(data, "background-clip", 15) == 0) {
                return CSSStyleKind::BackgroundClip;
            }
            if (memcmp(data, "border-collapse", 15) == 0) {
                return CSSStyleKind::BorderCollapse;
            }
            break;
        case 'o':
            if (memcmp(data, "object-position", 15) == 0) {
                return CSSStyleKind::ObjectPosition;
            }
        }
        break;
    case 16:
        // Transform-Origin
        // Background-Color
        // Background-Image
        // Border-Top-Color
        // Border-Top-Style
        // Border-Top-Width
        switch (data[0]) {
        case 't':
            if (memcmp(data, "transform-origin", 16) == 0) {
                return CSSStyleKind::TransformOrigin;
            }
            break;

        case 'b':
            if (memcmp(data, "background-color", 16) == 0) {
                return CSSStyleKind::BackgroundColor;
            }
            if (memcmp(data, "background-image", 16) == 0) {
                return CSSStyleKind::BackgroundImage;
            }
            if (memcmp(data, "border-top-color", 16) == 0) {
                return CSSStyleKind::BorderTopColor;
            }
            if (memcmp(data, "border-top-style", 16) == 0) {
                return CSSStyleKind::BorderTopStyle;
            }
            if (memcmp(data, "border-top-width", 16) == 0) {
                return CSSStyleKind::BorderTopWidth;
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
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "border-left-color", 17) == 0) {
                return CSSStyleKind::BorderLeftColor;
            }
            if (memcmp(data, "border-left-style", 17) == 0) {
                return CSSStyleKind::BorderLeftStyle;
            }
            if (memcmp(data, "border-left-width", 17) == 0) {
                return CSSStyleKind::BorderLeftWidth;
            }
            if (memcmp(data, "background-repeat", 17) == 0) {
                return CSSStyleKind::BackgroundRepeat;
            }
            if (memcmp(data, "background-origin", 17) == 0) {
                return CSSStyleKind::BackgroundOrigin;
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
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "border-image-slice", 18) == 0) {
                return CSSStyleKind::BorderImageSlice;
            }
            if (memcmp(data, "border-image-width", 18) == 0) {
                return CSSStyleKind::BorderImageWidth;
            }
            if (memcmp(data, "border-right-style", 18) == 0) {
                return CSSStyleKind::BorderRightStyle;
            }
            if (memcmp(data, "border-right-width", 18) == 0) {
                return CSSStyleKind::BorderRightWidth;
            }
            if (memcmp(data, "border-right-color", 18) == 0) {
                return CSSStyleKind::BorderRightColor;
            }
            break;
        }
        break;
    case 19:
        // Background-Repeat-X
        // Background-Repeat-Y
        // Border-Image-Source
        // Border-Bottom-Style
        // Border-Bottom-Width
        // Border-Bottom-Color
        // Background-Position
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "background-repeat-x", 19) == 0) {
                return CSSStyleKind::BackgroundRepeatX;
            }
            if (memcmp(data, "background-repeat-y", 19) == 0) {
                return CSSStyleKind::BackgroundRepeatY;
            }
            if (memcmp(data, "background-position", 19) == 0) {
                return CSSStyleKind::BackgroundPosition;
            }
            if (memcmp(data, "border-image-source", 19) == 0) {
                return CSSStyleKind::BorderImageSource;
            }
            if (memcmp(data, "border-bottom-style", 19) == 0) {
                return CSSStyleKind::BorderBottomStyle;
            }
            if (memcmp(data, "border-bottom-width", 19) == 0) {
                return CSSStyleKind::BorderBottomWidth;
            }
            if (memcmp(data, "border-bottom-color", 19) == 0) {
                return CSSStyleKind::BorderBottomColor;
            }
            break;
        case 't':
            if (memcmp(data, "transition-property", 19) == 0) {
                return CSSStyleKind::TransitionProperty;
            }
            if (memcmp(data, "transition-duration", 19) == 0) {
                return CSSStyleKind::TransitionDuration;
            }
            break;
        }
        break;
    case 21:
        // background-position-x
        // background-position-y
        // background-attachment
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "background-position-x", 21) == 0) {
                return CSSStyleKind::BackgroundPositionX;
            }
            if (memcmp(data, "background-position-y", 21) == 0) {
                return CSSStyleKind::BackgroundPositionY;
            }
            if (memcmp(data, "background-attachment", 21) == 0) {
                return CSSStyleKind::BackgroundAttachment;
            }
            break;
        }
        break;
    case 22:
        // border-top-left-radius
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "border-top-left-radius", 22) == 0) {
                return CSSStyleKind::BorderTopLeftRadius;
            }
            break;
        }
        break;
    case 23:
        // border-top-right-radius
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "border-top-right-radius", 23) == 0) {
                return CSSStyleKind::BorderTopRightRadius;
            }
            break;
        }
        break;
    case 25:
        // border-bottom-left-radius
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "border-bottom-left-radius", 25) == 0) {
                return CSSStyleKind::BorderBottomLeftRadius;
            }
            break;
        }
        break;
    case 26:
        // border-bottom-right-radius
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "border-bottom-right-radius", 26) == 0) {
                return CSSStyleKind::BorderBottomRightRadius;
            }
            break;
        }
        break;
    }

    // https://www.w3.org/TR/css-variables-1/#defining-variables
    if (length >= 2 && data[0] == '-' && data[1] == '-')
        return CSSStyleKind::CustomProperty;

    return CSSStyleKind::Unknown;
}

CSSStyleKind lookupCSSStyleCamelCase(const char* data, unsigned length)
{
    switch (length) {
    case 3:
        if (memcmp(data, "top", 3) == 0) {
            return CSSStyleKind::Top;
        }
        if (memcmp(data, "src", 3) == 0) {
            return CSSStyleKind::Src;
        }
        break;
    case 4:
        switch (data[0]) {
        case 'f':
            if (memcmp(data, "font", 4) == 0) {
                return CSSStyleKind::Font;
            }
            if (memcmp(data, "flex", 4) == 0) {
                return CSSStyleKind::Flex;
            }
            if (memcmp(data, "fill", 4) == 0) {
                return CSSStyleKind::Fill;
            }
            break;
        case 'l':
            if (memcmp(data, "left", 4) == 0) {
                return CSSStyleKind::Left;
            }
            break;
        }
        break;
    case 5:
        switch (data[0]) {
        case 'c':
            if (memcmp(data, "color", 5) == 0) {
                return CSSStyleKind::Color;
            }
            if (memcmp(data, "clear", 5) == 0) {
                return CSSStyleKind::Clear;
            }
            break;
        case 'f':
            if (memcmp(data, "float", 5) == 0) {
                return CSSStyleKind::Float;
            }
            break;
        case 'w':
            if (memcmp(data, "width", 5) == 0) {
                return CSSStyleKind::Width;
            }
            break;
        case 'r':
            if (memcmp(data, "right", 5) == 0) {
                return CSSStyleKind::Right;
            }
            break;
        case 'o':
            if (memcmp(data, "order", 5) == 0) {
                return CSSStyleKind::Order;
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
                    return CSSStyleKind::Bottom;
                }
                if (memcmp(data, "border", 6) == 0) {
                    return CSSStyleKind::Border;
                }
                break;
            }
            break;
        case 'h':
            if (memcmp(data, "height", 6) == 0) {
                return CSSStyleKind::Height;
            }
            break;
        case 'm':
            if (memcmp(data, "margin", 6) == 0) {
                return CSSStyleKind::Margin;
            }
            break;
        case 'z':
            if (memcmp(data, "zIndex", 6) == 0) {
                return CSSStyleKind::ZIndex;
            }
            break;
        case 's':
            if (memcmp(data, "stroke", 6) == 0) {
                return CSSStyleKind::Stroke;
            }
            break;
        }
        break;
    case 7:
        switch (data[0]) {
        case 'c':
            if (memcmp(data, "content", 7) == 0) {
                return CSSStyleKind::Content;
            }
            break;
        case 'd':
            if (memcmp(data, "display", 7) == 0) {
                return CSSStyleKind::Display;
            }
            break;
        case 'p':
            if (memcmp(data, "padding", 7) == 0) {
                return CSSStyleKind::Padding;
            }
            break;
        case 'o':
            if (memcmp(data, "opacity", 7) == 0) {
                return CSSStyleKind::Opacity;
            }
            if (memcmp(data, "outline", 7) == 0) {
                return CSSStyleKind::Outline;
            }
            break;
        }
        break;
    case 8:
        switch (data[0]) {
        case 'c':
            if (memcmp(data, "cssFloat", 8) == 0) {
                return CSSStyleKind::Float;
            }
            break;
        case 'f':
            if (memcmp(data, "fontSize", 8) == 0) {
                return CSSStyleKind::FontSize;
            }
            if (memcmp(data, "flexWrap", 8) == 0) {
                return CSSStyleKind::FlexWrap;
            }
            if (memcmp(data, "flexFlow", 8) == 0) {
                return CSSStyleKind::FlexFlow;
            }
            if (memcmp(data, "flexGrow", 8) == 0) {
                return CSSStyleKind::FlexGrow;
            }
            if (memcmp(data, "fillRule", 8) == 0) {
                return CSSStyleKind::FillRule;
            }
            break;
        case 'm':
            if (memcmp(data, "maxWidth", 8) == 0) {
                return CSSStyleKind::MaxWidth;
            }
            if (memcmp(data, "minWidth", 8) == 0) {
                return CSSStyleKind::MinWidth;
            }
            if (memcmp(data, "masksize", 8) == 0) {
                return CSSStyleKind::MaskSize;
            }
            break;
        case 'p':
            if (memcmp(data, "position", 8) == 0) {
                return CSSStyleKind::Position;
            }
            break;
        case 'o':
            if (memcmp(data, "overflow", 8) == 0) {
                return CSSStyleKind::Overflow;
            }
            break;
        case 'w':
            if (memcmp(data, "wordWrap", 8) == 0) {
                return CSSStyleKind::WordWrap;
            }
            break;
        }
        break;
    case 9:
        switch (data[0]) {
        case 'a':
            if (memcmp(data, "alignSelf", 9) == 0) {
                return CSSStyleKind::AlignSelf;
            }
            break;
        case 'b':
            if (memcmp(data, "borderTop", 9) == 0) {
                return CSSStyleKind::BorderTop;
            }
            if (memcmp(data, "boxSizing", 9) == 0) {
                return CSSStyleKind::BoxSizing;
            }
            break;
        case 't':
            if (memcmp(data, "transform", 9) == 0) {
                return CSSStyleKind::Transform;
            }
            if (memcmp(data, "textAlign", 9) == 0) {
                return CSSStyleKind::TextAlign;
            }
            break;
        case 'd':
            if (memcmp(data, "direction", 9) == 0) {
                return CSSStyleKind::Direction;
            }
            break;
        case 'f':
            if (memcmp(data, "fontStyle", 9) == 0) {
                return CSSStyleKind::FontStyle;
            }
            if (memcmp(data, "flexBasis", 9) == 0) {
                return CSSStyleKind::FlexBasis;
            }
            break;
        case 'm':
            if (memcmp(data, "marginTop", 9) == 0) {
                return CSSStyleKind::MarginTop;
            }
            if (memcmp(data, "maskImage", 9) == 0) {
                return CSSStyleKind::MaskImage;
            }
            if (memcmp(data, "maxHeight", 9) == 0) {
                return CSSStyleKind::MaxHeight;
            }
            if (memcmp(data, "minHeight", 9) == 0) {
                return CSSStyleKind::MinHeight;
            }
            break;
        case 'o':
            if (memcmp(data, "objectFit", 9) == 0) {
                return CSSStyleKind::ObjectFit;
            }
            if (memcmp(data, "overflowX", 9) == 0) {
                return CSSStyleKind::OverflowX;
            }
            if (memcmp(data, "overflowY", 9) == 0) {
                return CSSStyleKind::OverflowY;
            }
            break;
        }
        break;
    case 10:
        switch (data[0]) {
        case 'a':
            if (memcmp(data, "alignItems", 10) == 0) {
                return CSSStyleKind::AlignItems;
            }
            break;
        case 'b':
            if (memcmp(data, "background", 10) == 0) {
                return CSSStyleKind::Background;
            }
            if (memcmp(data, "borderLeft", 10) == 0) {
                return CSSStyleKind::BorderLeft;
            }
            break;
        case 'v':
            if (memcmp(data, "visibility", 10) == 0) {
                return CSSStyleKind::Visibility;
            }
            break;
        case 'f':
            if (memcmp(data, "fontWeight", 10) == 0) {
                return CSSStyleKind::FontWeight;
            }
            if (memcmp(data, "fontFamily", 10) == 0) {
                return CSSStyleKind::FontFamily;
            }
            if (memcmp(data, "flexShrink", 10) == 0) {
                return CSSStyleKind::FlexShrink;
            }
            break;
        case 'l':
            if (memcmp(data, "lineHeight", 10) == 0) {
                return CSSStyleKind::LineHeight;
            }
            break;
        case 'w':
            if (memcmp(data, "whiteSpace", 10) == 0) {
                return CSSStyleKind::WhiteSpace;
            }
            break;
        case 'p':
            if (memcmp(data, "paddingTop", 10) == 0) {
                return CSSStyleKind::PaddingTop;
            }
            break;
        case 'm':
            if (memcmp(data, "marginLeft", 10) == 0) {
                return CSSStyleKind::MarginLeft;
            }
            break;
        case 'e':
            if (memcmp(data, "emptyCells", 10) == 0) {
                return CSSStyleKind::EmptyCells;
            }
            break;
        case 't':
            if (memcmp(data, "transition", 10) == 0) {
                return CSSStyleKind::Transition;
            }
            if (memcmp(data, "textIndent", 10) == 0) {
                return CSSStyleKind::TextIndent;
            }
            break;
        }
        break;
    case 11:
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "borderStyle", 11) == 0) {
                return CSSStyleKind::BorderStyle;
            }
            if (memcmp(data, "borderWidth", 11) == 0) {
                return CSSStyleKind::BorderWidth;
            }
            if (memcmp(data, "borderColor", 11) == 0) {
                return CSSStyleKind::BorderColor;
            }
            if (memcmp(data, "borderRight", 11) == 0) {
                return CSSStyleKind::BorderRight;
            }
            break;
        case 'c':
            if (memcmp(data, "captionSide", 11) == 0) {
                return CSSStyleKind::CaptionSide;
            }
            break;
        case 'p':
            if (memcmp(data, "paddingLeft", 11) == 0) {
                return CSSStyleKind::PaddingLeft;
            }
            break;
        case 'm':
            if (memcmp(data, "marginRight", 11) == 0) {
                return CSSStyleKind::MarginRight;
            }
            break;
        case 't':
            if (memcmp(data, "tableLayout", 11) == 0) {
                return CSSStyleKind::TableLayout;
            }
            break;
        case 'u':
            if (memcmp(data, "unicodeBidi", 11) == 0) {
                return CSSStyleKind::UnicodeBidi;
            }
            break;
        case 'f':
            if (memcmp(data, "fillOpacity", 11) == 0) {
                return CSSStyleKind::FillOpacity;
            }
            break;
        case 's':
            if (memcmp(data, "strokeWidth", 11) == 0) {
                return CSSStyleKind::StrokeWidth;
            }
            break;
        case 'w':
            if (memcmp(data, "wordSpacing", 11) == 0) {
                return CSSStyleKind::WordSpacing;
            }
            break;
        }
        break;
    case 12:
        switch (data[0]) {
        case 'a':
            if (memcmp(data, "alignContent", 12) == 0) {
                return CSSStyleKind::AlignContent;
            }
            break;
        case 'p':
            if (memcmp(data, "paddingRight", 12) == 0) {
                return CSSStyleKind::PaddingRight;
            }
            break;
        case 'm':
            if (memcmp(data, "marginBottom", 12) == 0) {
                return CSSStyleKind::MarginBottom;
            }
            break;
        case 'b':
            if (memcmp(data, "borderBottom", 12) == 0) {
                return CSSStyleKind::BorderBottom;
            }
            if (memcmp(data, "borderRadius", 12) == 0) {
                return CSSStyleKind::BorderRadius;
            }
            break;
        case 'o':
            if (memcmp(data, "outlineColor", 12) == 0) {
                return CSSStyleKind::OutlineColor;
            }
            if (memcmp(data, "outlineWidth", 12) == 0) {
                return CSSStyleKind::OutlineWidth;
            }
            if (memcmp(data, "outlineStyle", 12) == 0) {
                return CSSStyleKind::OutlineStyle;
            }
            if (memcmp(data, "overflowWrap", 12) == 0) {
                return CSSStyleKind::OverflowWrap;
            }
            break;
        }
        break;
    case 13:
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "borderSpacing", 13) == 0) {
                return CSSStyleKind::BorderSpacing;
            }
            break;
        case 'f':
            if (memcmp(data, "flexDirection", 13) == 0) {
                return CSSStyleKind::FlexDirection;
            }
            break;
        case 'v':
            if (memcmp(data, "verticalAlign", 13) == 0) {
                return CSSStyleKind::VerticalAlign;
            }
            break;
        case 'p':
            if (memcmp(data, "paddingBottom", 13) == 0) {
                return CSSStyleKind::PaddingBottom;
            }
            break;
        case 'o':
            if (memcmp(data, "outlineOffset", 13) == 0) {
                return CSSStyleKind::OutlineOffset;
            }
            break;
        case 't':
            if (memcmp(data, "textTransform", 13) == 0) {
                return CSSStyleKind::TextTransform;
            }
            break;
        }
        break;
    case 14:
        switch (data[0]) {
        case 'j':
            if (memcmp(data, "justifyContent", 14) == 0) {
                return CSSStyleKind::JustifyContent;
            }
            break;
        case 't':
            if (memcmp(data, "textDecoration", 14) == 0) {
                return CSSStyleKind::TextDecoration;
            }
            break;
        case 'b':
            if (memcmp(data, "backgroundSize", 14) == 0) {
                return CSSStyleKind::BackgroundSize;
            }
            if (memcmp(data, "backgroundClip", 14) == 0) {
                return CSSStyleKind::BackgroundClip;
            }
            if (memcmp(data, "borderTopColor", 14) == 0) {
                return CSSStyleKind::BorderTopColor;
            }
            if (memcmp(data, "borderTopStyle", 14) == 0) {
                return CSSStyleKind::BorderTopStyle;
            }
            if (memcmp(data, "borderTopWidth", 14) == 0) {
                return CSSStyleKind::BorderTopWidth;
            }
            if (memcmp(data, "borderCollapse", 14) == 0) {
                return CSSStyleKind::BorderCollapse;
            }
            break;
        case 'o':
            if (memcmp(data, "objectPosition", 14) == 0) {
                return CSSStyleKind::ObjectPosition;
            }
        }
        break;
    case 15:
        switch (data[0]) {
        case 't':
            if (memcmp(data, "transformOrigin", 15) == 0) {
                return CSSStyleKind::TransformOrigin;
            }
            break;
        case 'b':
            if (memcmp(data, "backgroundColor", 15) == 0) {
                return CSSStyleKind::BackgroundColor;
            }
            if (memcmp(data, "backgroundImage", 15) == 0) {
                return CSSStyleKind::BackgroundImage;
            }
            if (memcmp(data, "borderLeftColor", 15) == 0) {
                return CSSStyleKind::BorderLeftColor;
            }
            if (memcmp(data, "borderLeftStyle", 15) == 0) {
                return CSSStyleKind::BorderLeftStyle;
            }
            if (memcmp(data, "borderLeftWidth", 15) == 0) {
                return CSSStyleKind::BorderLeftWidth;
            }
        }
        break;
    case 16:
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "backgroundRepeat", 16) == 0) {
                return CSSStyleKind::BackgroundRepeat;
            }
            if (memcmp(data, "backgroundOrigin", 16) == 0) {
                return CSSStyleKind::BackgroundOrigin;
            }
            if (memcmp(data, "borderImageSlice", 16) == 0) {
                return CSSStyleKind::BorderImageSlice;
            }
            if (memcmp(data, "borderImageWidth", 16) == 0) {
                return CSSStyleKind::BorderImageWidth;
            }
            if (memcmp(data, "borderRightStyle", 16) == 0) {
                return CSSStyleKind::BorderRightStyle;
            }
            if (memcmp(data, "borderRightWidth", 16) == 0) {
                return CSSStyleKind::BorderRightWidth;
            }
            if (memcmp(data, "borderRightColor", 16) == 0) {
                return CSSStyleKind::BorderRightColor;
            }
            break;
        }
        break;
    case 17:
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "backgroundRepeatX", 17) == 0) {
                return CSSStyleKind::BackgroundRepeatX;
            }
            if (memcmp(data, "backgroundRepeatY", 17) == 0) {
                return CSSStyleKind::BackgroundRepeatY;
            }
            if (memcmp(data, "borderImageSource", 17) == 0) {
                return CSSStyleKind::BorderImageSource;
            }
            if (memcmp(data, "borderBottomStyle", 17) == 0) {
                return CSSStyleKind::BorderBottomStyle;
            }
            if (memcmp(data, "borderBottomWidth", 17) == 0) {
                return CSSStyleKind::BorderBottomWidth;
            }
            if (memcmp(data, "borderBottomColor", 17) == 0) {
                return CSSStyleKind::BorderBottomColor;
            }
            break;
        }
        break;
    case 18:
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "backgroundPosition", 18) == 0) {
                return CSSStyleKind::BackgroundPosition;
            }
            break;
        }
        break;
    case 't':
        if (memcmp(data, "transitionProperty", 18) == 0) {
            return CSSStyleKind::TransitionProperty;
        }
        if (memcmp(data, "transitionDuration", 18) == 0) {
            return CSSStyleKind::TransitionDuration;
        }
        break;
    case 19:
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "backgroundPositionX", 19) == 0) {
                return CSSStyleKind::BackgroundPositionX;
            }
            if (memcmp(data, "backgroundPositionY", 19) == 0) {
                return CSSStyleKind::BackgroundPositionY;
            }
            if (memcmp(data, "borderTopLeftRadius", 19) == 0) {
                return CSSStyleKind::BorderTopLeftRadius;
            }
            break;
        }
        break;
    case 20:
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "borderTopRightRadius", 20) == 0) {
                return CSSStyleKind::BorderTopRightRadius;
            }
            if (memcmp(data, "backgroundAttachment", 20) == 0) {
                return CSSStyleKind::BackgroundAttachment;
            }
            break;
        }
        break;
    case 22:
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "borderBottomLeftRadius", 22) == 0) {
                return CSSStyleKind::BorderBottomLeftRadius;
            }
            break;
        }
        break;
    case 23:
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "borderBottomRightRadius", 23) == 0) {
                return CSSStyleKind::BorderBottomRightRadius;
            }
            break;
        }
        break;
    }

    return CSSStyleKind::Unknown;
}

bool lookupCSSMediaQueryConstraints(const char* data, unsigned length)
{
    switch (length) {
    case 4:
        switch (data[0]) {
        case 'g':
            if (memcmp(data, "grid", 4) == 0) {
                return true;
            }
            break;
        case 's':
            if (memcmp(data, "scan", 4) == 0) {
                return true;
            }
            break;
        }
        break;
    case 5:
        switch (data[0]) {
        case 'c':
            if (memcmp(data, "color", 5) == 0) {
                return true;
            }
            break;
        case 'w':
            if (memcmp(data, "width", 5) == 0) {
                return true;
            }
            break;
        }
        break;
    case 6:
        switch (data[0]) {
        case 'h':
            if (memcmp(data, "height", 6) == 0) {
                return true;
            }
            break;
        }
        break;
    case 9:
        switch (data[0]) {
        case 'm':
            if (memcmp(data, "max-color", 9) == 0) {
                return true;
            }
            if (memcmp(data, "max-width", 9) == 0) {
                return true;
            }
            if (memcmp(data, "min-color", 9) == 0) {
                return true;
            }
            if (memcmp(data, "min-width", 9) == 0) {
                return true;
            }
            break;
        }
        break;
    case 10:
        switch (data[0]) {
        case 'm':
            if (memcmp(data, "max-height", 10) == 0) {
                return true;
            }
            if (memcmp(data, "min-height", 10) == 0) {
                return true;
            }
            if (memcmp(data, "monochrome", 10) == 0) {
                return true;
            }
            break;
        case 'r':
            if (memcmp(data, "resolution", 10) == 0) {
                return true;
            }
            break;
        }
        break;
    case 11:
        switch (data[0]) {
        case 'c':
            if (memcmp(data, "color-index", 11) == 0) {
                return true;
            }
            break;
        case 'o':
            if (memcmp(data, "orientation", 11) == 0) {
                return true;
            }
            break;
        }
        break;
    case 12:
        switch (data[0]) {
        case 'a':
            if (memcmp(data, "aspect-ratio", 12) == 0) {
                return true;
            }
            break;
        case 'd':
            if (memcmp(data, "device-width", 12) == 0) {
                return true;
            }
            break;
        }
        break;
    case 13:
        switch (data[0]) {
        case 'd':
            if (memcmp(data, "device-height", 13) == 0) {
                return true;
            }
            break;
        }
        break;
    case 14:
        switch (data[0]) {
        case 'm':
            if (memcmp(data, "max-monochrome", 14) == 0) {
                return true;
            }
            if (memcmp(data, "max-resolution", 14) == 0) {
                return true;
            }
            if (memcmp(data, "min-monochrome", 14) == 0) {
                return true;
            }
            if (memcmp(data, "min-resolution", 14) == 0) {
                return true;
            }
            break;
        }
        break;
    case 15:
        switch (data[0]) {
        case 'm':
            if (memcmp(data, "max-color-index", 15) == 0) {
                return true;
            }
            if (memcmp(data, "min-color-index", 15) == 0) {
                return true;
            }
            break;
        }
        break;
    case 16:
        switch (data[0]) {
        case 'm':
            if (memcmp(data, "max-aspect-ratio", 16) == 0) {
                return true;
            }
            if (memcmp(data, "max-device-width", 16) == 0) {
                return true;
            }
            if (memcmp(data, "min-aspect-ratio", 16) == 0) {
                return true;
            }
            if (memcmp(data, "min-device-width", 16) == 0) {
                return true;
            }
            break;
        }
        break;
    case 17:
        switch (data[0]) {
        case 'm':
            if (memcmp(data, "max-device-height", 17) == 0) {
                return true;
            }
            if (memcmp(data, "min-device-height", 17) == 0) {
                return true;
            }
            break;
        }
        break;
    case 19:
        switch (data[0]) {
        case 'd':
            if (memcmp(data, "device-aspect-ratio", 19) == 0) {
                return true;
            }
            break;
        }
        break;
    case 23:
        switch (data[0]) {
        case 'm':
            if (memcmp(data, "max-device-aspect-ratio", 23) == 0) {
                return true;
            }
            if (memcmp(data, "min-device-aspect-ratio", 23) == 0) {
                return true;
            }
            break;
        }
        break;
    }
    return false;
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
