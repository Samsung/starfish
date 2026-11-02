/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishClipPathShape__
#define __StarfishClipPathShape__

#include "core/style/Length.h"
#include "core/style/Style.h"

namespace Starfish {

class ComputedStyle;
class Font;

// <basic-shape> || <geometry-box> value of clip-path.
// https://drafts.csswg.org/css-masking/#the-clip-path
// path(), shape(), xywh() and rect() are not supported.
class ClipPathShape : public gc {
public:
    // Length forbids placement new, which TightVector needs.
    struct LengthItem {
        LengthItem()
        {
        }
        LengthItem(const Length& length)
            : m_length(length)
        {
        }
        Length m_length;
    };

    enum class Type : uint8_t {
        BoxOnly,
        Inset,
        Circle,
        Ellipse,
        Polygon,
    };

    enum class Box : uint8_t {
        Unspecified,
        MarginBox,
        BorderBox,
        PaddingBox,
        ContentBox,
        FillBox,
        StrokeBox,
        ViewBox,
    };

    enum class Radius : uint8_t {
        Length,
        ClosestSide,
        FarthestSide,
        ClosestCorner,
        FarthestCorner,
    };

    // Returns nullptr when |tokens| is not a valid basic shape value.
    static ClipPathShape* parse(const CSSTokenVector& tokens);

    // The computed copy drops the specified text.
    ClipPathShape* clone() const;
    bool equals(const ClipPathShape* other) const;
    String* toString() const;

    // Font-relative lengths become px, as for other computed lengths.
    void resolveFontRelativeLengths(Length curFontSize, Length rootFontSize,
                                    Optional<Font*> font,
                                    LayoutUnit viewportWidth,
                                    LayoutUnit viewportHeight,
                                    ComputedStyle* style);

    Type type() const
    {
        return m_type;
    }

    Box box() const
    {
        return m_box;
    }

    bool isEvenOdd() const
    {
        return m_evenOdd;
    }

    // Inset: top, right, bottom, left.
    // Circle: radius, (unused), cx, cy. Ellipse: rx, ry, cx, cy.
    const Length& arg(size_t i) const
    {
        STARFISH_ASSERT(i < 4);
        return m_args[i];
    }

    Radius radiusX() const
    {
        return m_radiusX;
    }

    Radius radiusY() const
    {
        return m_radiusY;
    }

    // Inset round radii: top-left, top-right, bottom-right, bottom-left,
    // horizontal then vertical. Empty when there is no `round`.
    const GCTightVectorT<LengthItem>& insetRadii() const
    {
        return m_insetRadii;
    }

    // Polygon vertices as x, y pairs.
    const GCTightVectorT<LengthItem>& points() const
    {
        return m_points;
    }

private:
    struct SpecifiedTexts;

    ClipPathShape()
        : m_type(Type::BoxOnly)
        , m_box(Box::Unspecified)
        , m_radiusX(Radius::ClosestSide)
        , m_radiusY(Radius::ClosestSide)
        , m_evenOdd(false)
        , m_hasPosition(false)
        , m_specifiedText(nullptr)
        , m_parsingTexts(nullptr)
    {
    }

    String* serialize(const SpecifiedTexts* texts) const;

    bool parseFunction(const CSSTokenValue& token);
    bool parseInset(const CSSTokenValue& args);
    bool parseCircleOrEllipse(const CSSTokenValue& args, bool isCircle);
    bool parsePolygon(const CSSTokenValue& args);

    Type m_type;
    Box m_box;
    Radius m_radiusX;
    Radius m_radiusY;
    bool m_evenOdd;
    bool m_hasPosition;
    Length m_args[4];
    GCTightVectorT<LengthItem> m_insetRadii;
    GCTightVectorT<LengthItem> m_points;
    // Specified values serialize with their keywords and units as written.
    String* m_specifiedText;
    SpecifiedTexts* m_parsingTexts;
};
} // namespace Starfish

#endif
