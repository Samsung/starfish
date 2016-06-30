#include "StarFishConfig.h"
#include "CSSStyleLookupTrie.h"

namespace StarFish {

CSSStyleKind lookupCSSStyle(const char* data, unsigned length)
{
    switch (length) {
    case 3:
        // Top
        if (memcmp(data, "top", 3) == 0) {
            return CSSStyleKind::Top;
        }
        break;
    case 4:
        // Left
        if (memcmp(data, "left", 4) == 0) {
            return CSSStyleKind::Left;
        }
        break;
    case 5:
        // Color
        // Width
        // Right
        switch (data[0]) {
        case 'c':
            if (memcmp(data, "color", 5) == 0) {
                return CSSStyleKind::Color;
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
        }
        break;
    case 6:
        // Bottom
        // Border
        // Height
        // Margin
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
        }
        break;
    case 7:
        // Display
        // Padding
        // Z-Index
        // Opacity
        switch (data[0]) {
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
        // Transform
        // Direction
        switch (data[0]) {
        case 'f':
            if (memcmp(data, "font-size", 9) == 0) {
                return CSSStyleKind::FontSize;
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
        }
        break;
    case 10:
        // Background
        // Border-Top
        // Font-Style
        // Text-Align
        // Margin-Top
        // Visibility
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "background", 10) == 0) {
                return CSSStyleKind::Background;
            }
            if (memcmp(data, "border-top", 10) == 0) {
                return CSSStyleKind::BorderTop;
            }
            break;
        case 'f':
            if (memcmp(data, "font-style", 10) == 0) {
                return CSSStyleKind::FontStyle;
            }
            break;
        case 't':
            if (memcmp(data, "text-align", 10) == 0) {
                return CSSStyleKind::TextAlign;
            }
            break;
        case 'm':
            if (memcmp(data, "margin-top", 10) == 0) {
                return CSSStyleKind::MarginTop;
            }
            break;
        case 'v':
            if (memcmp(data, "visibility", 10) == 0) {
                return CSSStyleKind::Visibility;
            }
            break;
        }
        break;
    case 11:
        // Border-Left
        // Font-Weight
        // Line-Height
        // Padding-Top
        // Margin-Left
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "border-left", 11) == 0) {
                return CSSStyleKind::BorderLeft;
            }
            break;
        case 'f':
            if (memcmp(data, "font-weight", 11) == 0) {
                return CSSStyleKind::FontWeight;
            }
            break;
        case 'l':
            if (memcmp(data, "line-height", 11) == 0) {
                return CSSStyleKind::LineHeight;
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
        case 'u':
            if (memcmp(data, "unicode-bidi", 12) == 0) {
                return CSSStyleKind::UnicodeBidi;
            }
            break;
        }
        break;
    case 13:
        // Padding-Right
        // Margin-Bottom
        // Border-Bottom
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
            break;
        }
        break;
    case 14:
        // Vertical-Align
        // Letter-Spacing
        // Padding-Bottom
        switch (data[0]) {
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
        }
        break;
    case 15:
        // Text-Decoration
        // Background-Size
        switch (data[0]) {
        case 't':
            if (memcmp(data, "text-decoration", 15) == 0) {
                return CSSStyleKind::TextDecoration;
            }
            break;
        case 'b':
            if (memcmp(data, "background-size", 15) == 0) {
                return CSSStyleKind::BackgroundSize;
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
        // Border-Image-Repeat
        // Border-Image-Source
        // Border-Bottom-Style
        // Border-Bottom-Width
        // Border-Bottom-Color
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "background-repeat-x", 19) == 0) {
                return CSSStyleKind::BackgroundRepeatX;
            }
            if (memcmp(data, "background-repeat-y", 19) == 0) {
                return CSSStyleKind::BackgroundRepeatY;
            }
            if (memcmp(data, "border-image-repeat", 19) == 0) {
                return CSSStyleKind::BorderImageRepeat;
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
        }
        break;
    }

    return CSSStyleKind::Unknown;
}

CSSStyleKind lookupCSSStyleCamelCase(const char* data, unsigned length)
{
    switch (length) {
    case 3:
        if (memcmp(data, "top", 3) == 0) {
            return CSSStyleKind::Top;
        }
        break;
    case 4:
        if (memcmp(data, "left", 4) == 0) {
            return CSSStyleKind::Left;
        }
        break;
    case 5:
        switch (data[0]) {
        case 'c':
            if (memcmp(data, "color", 5) == 0) {
                return CSSStyleKind::Color;
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
        }
        break;
    case 7:
        switch (data[0]) {
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
            break;
        }
        break;
    case 8:
        switch (data[0]) {
        case 'f':
            if (memcmp(data, "fontSize", 8) == 0) {
                return CSSStyleKind::FontSize;
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
        }
        break;
    case 9:
        switch (data[0]) {
        case 'b':
            if (memcmp(data, "borderTop", 9) == 0) {
                return CSSStyleKind::BorderTop;
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
            break;
        case 'm':
            if (memcmp(data, "marginTop", 9) == 0) {
                return CSSStyleKind::MarginTop;
            }
            break;
        }
        break;
    case 10:
        switch (data[0]) {
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
            break;
        case 'l':
            if (memcmp(data, "lineHeight", 10) == 0) {
                return CSSStyleKind::LineHeight;
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
        case 'u':
            if (memcmp(data, "unicodeBidi", 11) == 0) {
                return CSSStyleKind::UnicodeBidi;
            }
            break;
        }
        break;
    case 12:
        switch (data[0]) {
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
            break;
        }
        break;
    case 13:
        switch (data[0]) {
        case 'v':
            if (memcmp(data, "verticalAlign", 13) == 0) {
                return CSSStyleKind::VerticalAlign;
            }
            break;
        case 'p':
            if (memcmp(data, "paddingBottom", 13) == 0) {
                return CSSStyleKind::PaddingBottom;
            }
        }
        break;
    case 14:
        switch (data[0]) {
        case 't':
            if (memcmp(data, "textDecoration", 14) == 0) {
                return CSSStyleKind::TextDecoration;
            }
            break;
        case 'b':
            if (memcmp(data, "backgroundSize", 14) == 0) {
                return CSSStyleKind::BackgroundSize;
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
            break;
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
            if (memcmp(data, "borderImageRepeat", 17) == 0) {
                return CSSStyleKind::BorderImageRepeat;
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
    }

    return CSSStyleKind::Unknown;
}

}
