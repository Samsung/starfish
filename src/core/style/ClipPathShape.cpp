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

#include "StarfishConfig.h"
#include "core/style/ClipPathShape.h"
#include "core/style/CSSParser.h"
#include "core/style/Style.h"

namespace Starfish {

static void splitTopLevel(const CSSTokenValue& src, char separator,
                          std::vector<CSSTokenValue>& out)
{
    CSSTokenValue cur;
    int depth = 0;
    for (char ch : src) {
        if (ch == '(') {
            depth++;
        } else if (ch == ')') {
            depth--;
        }
        bool isSeparator =
            separator == ' ' ? String::isSpaceOrNewline(ch) : ch == separator;
        if (isSeparator && depth == 0) {
            if (separator != ' ' || cur.length()) {
                out.push_back(cur.trim());
            }
            cur.clear();
            continue;
        }
        cur.push_back(ch);
    }
    if (separator != ' ' || cur.length()) {
        out.push_back(cur.trim());
    }
}

// Source text of each component, kept only while parsing to build the
// specified-value serialization.
struct ClipPathShape::SpecifiedTexts {
    std::string m_args[4];
    std::string m_radii[8];
    std::vector<std::string> m_points;
};

static std::string componentText(const CSSTokenValue& token)
{
    // A unitless zero serializes as a length.
    if (token == "0") {
        return "0px";
    }
    return token;
}

static bool parseLengthPercentage(const CSSTokenValue& token,
                                  bool allowNegative, Length& result)
{
    uint32_t options = CSSPropertyParser::AllowPercent;
    if (allowNegative) {
        options |= CSSPropertyParser::AllowNegative;
    }
    CSSStyleValuePair pair;
    if (!CSSPropertyParser::parseLength(token.data(), options, &pair)) {
        return false;
    }
    if (pair.valueKind() == CSSStyleValuePair::ValueKind::Percentage) {
        result = Length(Length::Percent, pair.percentageValue());
    } else {
        result = pair.lengthValue();
    }
    return true;
}

static bool parseBox(const CSSTokenValue& token, ClipPathShape::Box& box)
{
    static const struct {
        const char* name;
        ClipPathShape::Box box;
    } kBoxes[] = {
        { "margin-box", ClipPathShape::Box::MarginBox },
        { "border-box", ClipPathShape::Box::BorderBox },
        { "padding-box", ClipPathShape::Box::PaddingBox },
        { "content-box", ClipPathShape::Box::ContentBox },
        { "fill-box", ClipPathShape::Box::FillBox },
        { "stroke-box", ClipPathShape::Box::StrokeBox },
        { "view-box", ClipPathShape::Box::ViewBox },
    };
    for (const auto& entry : kBoxes) {
        if (token == entry.name) {
            box = entry.box;
            return true;
        }
    }
    return false;
}

static bool parseRadiusKeyword(const CSSTokenValue& token,
                               ClipPathShape::Radius& radius)
{
    if (token == "closest-side") {
        radius = ClipPathShape::Radius::ClosestSide;
    } else if (token == "farthest-side") {
        radius = ClipPathShape::Radius::FarthestSide;
    } else if (token == "closest-corner") {
        radius = ClipPathShape::Radius::ClosestCorner;
    } else if (token == "farthest-corner") {
        radius = ClipPathShape::Radius::FarthestCorner;
    } else {
        return false;
    }
    return true;
}

static bool parseShapeRadius(const CSSTokenValue& token,
                             ClipPathShape::Radius& keyword, Length& length)
{
    if (parseRadiusKeyword(token, keyword)) {
        return true;
    }
    keyword = ClipPathShape::Radius::Length;
    return parseLengthPercentage(token, false, length);
}

enum class PositionAxis {
    Either,
    Horizontal,
    Vertical,
};

static bool parsePositionComponent(const CSSTokenValue& token, Length& length,
                                   PositionAxis& axis, bool& isKeyword)
{
    isKeyword = true;
    if (token == "center") {
        length = Length(Length::Percent, 0.5f);
        axis = PositionAxis::Either;
    } else if (token == "left") {
        length = Length(Length::Percent, 0);
        axis = PositionAxis::Horizontal;
    } else if (token == "right") {
        length = Length(Length::Percent, 1);
        axis = PositionAxis::Horizontal;
    } else if (token == "top") {
        length = Length(Length::Percent, 0);
        axis = PositionAxis::Vertical;
    } else if (token == "bottom") {
        length = Length(Length::Percent, 1);
        axis = PositionAxis::Vertical;
    } else {
        isKeyword = false;
        axis = PositionAxis::Either;
        return parseLengthPercentage(token, true, length);
    }
    return true;
}

// Only the 1- and 2-value <position> forms are supported; the 3- and 4-value
// forms compute to calc() offsets.
// https://drafts.csswg.org/css-values-4/#position
static bool parsePosition(const std::vector<CSSTokenValue>& parts, size_t from,
                          Length& x, Length& y, std::string& xText,
                          std::string& yText)
{
    size_t count = parts.size() - from;
    Length first, second;
    PositionAxis firstAxis, secondAxis;
    bool firstIsKeyword, secondIsKeyword;
    if (count == 1) {
        if (!parsePositionComponent(parts[from], first, firstAxis,
                                    firstIsKeyword)) {
            return false;
        }
        if (firstAxis == PositionAxis::Vertical) {
            x = Length(Length::Percent, 0.5f);
            y = first;
            xText = "center";
            yText = componentText(parts[from]);
        } else {
            x = first;
            y = Length(Length::Percent, 0.5f);
            xText = componentText(parts[from]);
            yText = "center";
        }
        return true;
    }
    if (count != 2 ||
        !parsePositionComponent(parts[from], first, firstAxis,
                                firstIsKeyword) ||
        !parsePositionComponent(parts[from + 1], second, secondAxis,
                                secondIsKeyword)) {
        return false;
    }
    bool swap = firstAxis == PositionAxis::Vertical ||
                secondAxis == PositionAxis::Horizontal;
    if (swap) {
        // Swapped order is only allowed between two keywords.
        if (!firstIsKeyword || !secondIsKeyword ||
            firstAxis == PositionAxis::Horizontal ||
            secondAxis == PositionAxis::Vertical) {
            return false;
        }
        x = second;
        y = first;
        xText = componentText(parts[from + 1]);
        yText = componentText(parts[from]);
    } else {
        x = first;
        y = second;
        xText = componentText(parts[from]);
        yText = componentText(parts[from + 1]);
    }
    return true;
}

ClipPathShape* ClipPathShape::parse(const CSSTokenVector& tokens)
{
    if (!tokens.size() || tokens.size() > 2) {
        return nullptr;
    }
    ClipPathShape* shape = new ClipPathShape();
    SpecifiedTexts texts;
    shape->m_parsingTexts = &texts;
    bool hasShape = false;
    bool hasBox = false;
    for (size_t i = 0; i < tokens.size(); i++) {
        const CSSTokenValue& token = tokens[i];
        Box box;
        if (parseBox(token, box)) {
            if (hasBox) {
                return nullptr;
            }
            hasBox = true;
            shape->m_box = box;
        } else {
            if (hasShape || !shape->parseFunction(token)) {
                return nullptr;
            }
            hasShape = true;
        }
    }
    shape->m_parsingTexts = nullptr;
    shape->m_specifiedText = shape->serialize(&texts);
    return shape;
}

bool ClipPathShape::parseFunction(const CSSTokenValue& token)
{
    static const struct {
        const char* name;
        Type type;
    } kFunctions[] = {
        { "inset", Type::Inset },
        { "circle", Type::Circle },
        { "ellipse", Type::Ellipse },
        { "polygon", Type::Polygon },
    };
    for (const auto& entry : kFunctions) {
        Optional<CSSTokenValue> args =
            CSSPropertyParser::parseFunctionBlock(token.data(), entry.name);
        if (!args.hasValue()) {
            continue;
        }
        m_type = entry.type;
        switch (entry.type) {
        case Type::Inset:
            return parseInset(args.getValue());
        case Type::Circle:
            return parseCircleOrEllipse(args.getValue(), true);
        case Type::Ellipse:
            return parseCircleOrEllipse(args.getValue(), false);
        case Type::Polygon:
            return parsePolygon(args.getValue());
        case Type::BoxOnly:
            break;
        }
    }
    return false;
}

// inset( <length-percentage>{1,4} [ round <'border-radius'> ]? )
bool ClipPathShape::parseInset(const CSSTokenValue& args)
{
    std::vector<CSSTokenValue> parts;
    splitTopLevel(args, ' ', parts);
    size_t i = 0;
    Length offsets[4];
    std::string offsetTexts[4];
    size_t offsetCount = 0;
    for (; i < parts.size() && parts[i] != "round"; i++) {
        if (offsetCount == 4 ||
            !parseLengthPercentage(parts[i], true, offsets[offsetCount])) {
            return false;
        }
        offsetTexts[offsetCount] = componentText(parts[i]);
        offsetCount++;
    }
    if (!offsetCount) {
        return false;
    }
    // Expand like the margin shorthand.
    m_args[0] = offsets[0];
    m_args[1] = offsetCount > 1 ? offsets[1] : m_args[0];
    m_args[2] = offsetCount > 2 ? offsets[2] : m_args[0];
    m_args[3] = offsetCount > 3 ? offsets[3] : m_args[1];
    static const size_t kSideSource[4][4] = {
        { 0, 0, 0, 0 }, { 0, 1, 0, 1 }, { 0, 1, 2, 1 }, { 0, 1, 2, 3 }
    };
    for (size_t side = 0; side < 4; side++) {
        m_parsingTexts->m_args[side] =
            offsetTexts[kSideSource[offsetCount - 1][side]];
    }

    if (i == parts.size()) {
        return true;
    }

    // `round` followed by the border-radius syntax.
    Length radii[2][4];
    std::string radiiTexts[2][4];
    size_t counts[2] = { 0, 0 };
    size_t axis = 0;
    for (i++; i < parts.size(); i++) {
        if (parts[i] == "/") {
            if (axis == 1 || !counts[0]) {
                return false;
            }
            axis = 1;
            continue;
        }
        if (counts[axis] == 4 ||
            !parseLengthPercentage(parts[i], false,
                                   radii[axis][counts[axis]])) {
            return false;
        }
        radiiTexts[axis][counts[axis]] = componentText(parts[i]);
        counts[axis]++;
    }
    if (!counts[0] || (axis == 1 && !counts[1])) {
        return false;
    }
    if (!counts[1]) {
        for (size_t j = 0; j < counts[0]; j++) {
            radii[1][j] = radii[0][j];
            radiiTexts[1][j] = radiiTexts[0][j];
        }
        counts[1] = counts[0];
    }
    for (size_t a = 0; a < 2; a++) {
        Length* r = radii[a];
        size_t n = counts[a];
        Length expanded[4] = { r[0], n > 1 ? r[1] : r[0], n > 2 ? r[2] : r[0],
                               n > 3 ? r[3] : (n > 1 ? r[1] : r[0]) };
        for (const auto& length : expanded) {
            m_insetRadii.push_back(length);
        }
        // Same corner expansion as the border-radius shorthand.
        static const size_t kCornerSource[4][4] = {
            { 0, 0, 0, 0 }, { 0, 1, 0, 1 }, { 0, 1, 2, 1 }, { 0, 1, 2, 3 }
        };
        for (size_t corner = 0; corner < 4; corner++) {
            m_parsingTexts->m_radii[a * 4 + corner] =
                radiiTexts[a][kCornerSource[n - 1][corner]];
        }
    }
    return true;
}

// circle( <shape-radius>? [ at <position> ]? )
// ellipse( [ <shape-radius>{2} ]? [ at <position> ]? )
bool ClipPathShape::parseCircleOrEllipse(const CSSTokenValue& args,
                                         bool isCircle)
{
    std::vector<CSSTokenValue> parts;
    splitTopLevel(args, ' ', parts);
    size_t at = 0;
    while (at < parts.size() && parts[at] != "at") {
        at++;
    }
    size_t radiusCount = at;
    if (isCircle) {
        if (radiusCount > 1 ||
            (radiusCount == 1 &&
             !parseShapeRadius(parts[0], m_radiusX, m_args[0]))) {
            return false;
        }
        m_radiusY = m_radiusX;
    } else {
        if (radiusCount != 0 && radiusCount != 2) {
            return false;
        }
        if (radiusCount == 2 &&
            (!parseShapeRadius(parts[0], m_radiusX, m_args[0]) ||
             !parseShapeRadius(parts[1], m_radiusY, m_args[1]))) {
            return false;
        }
    }

    for (size_t r = 0; r < radiusCount; r++) {
        m_parsingTexts->m_args[r] = componentText(parts[r]);
    }

    m_args[2] = Length(Length::Percent, 0.5f);
    m_args[3] = Length(Length::Percent, 0.5f);
    if (at == parts.size()) {
        return true;
    }
    m_hasPosition = true;
    return parsePosition(parts, at + 1, m_args[2], m_args[3],
                         m_parsingTexts->m_args[2], m_parsingTexts->m_args[3]);
}

// polygon( <'fill-rule'>? , [ <length-percentage> <length-percentage> ]# )
bool ClipPathShape::parsePolygon(const CSSTokenValue& args)
{
    std::vector<CSSTokenValue> vertices;
    splitTopLevel(args, ',', vertices);
    size_t i = 0;
    if (vertices.size() &&
        (vertices[0] == "nonzero" || vertices[0] == "evenodd")) {
        m_evenOdd = vertices[0] == "evenodd";
        i = 1;
    }
    if (i == vertices.size()) {
        return false;
    }
    for (; i < vertices.size(); i++) {
        std::vector<CSSTokenValue> xy;
        splitTopLevel(vertices[i], ' ', xy);
        Length x, y;
        if (xy.size() != 2 || !parseLengthPercentage(xy[0], true, x) ||
            !parseLengthPercentage(xy[1], true, y)) {
            return false;
        }
        m_points.push_back(x);
        m_points.push_back(y);
        m_parsingTexts->m_points.push_back(componentText(xy[0]));
        m_parsingTexts->m_points.push_back(componentText(xy[1]));
    }
    return true;
}

ClipPathShape* ClipPathShape::clone() const
{
    ClipPathShape* copy = new ClipPathShape();
    copy->m_type = m_type;
    copy->m_box = m_box;
    copy->m_radiusX = m_radiusX;
    copy->m_radiusY = m_radiusY;
    copy->m_evenOdd = m_evenOdd;
    copy->m_hasPosition = m_hasPosition;
    for (size_t i = 0; i < 4; i++) {
        copy->m_args[i] = m_args[i];
    }
    copy->m_insetRadii = m_insetRadii;
    copy->m_points = m_points;
    return copy;
}

static bool lengthsEqual(const GCTightVectorT<ClipPathShape::LengthItem>& a,
                         const GCTightVectorT<ClipPathShape::LengthItem>& b)
{
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t i = 0; i < a.size(); i++) {
        if (!(a[i].m_length == b[i].m_length)) {
            return false;
        }
    }
    return true;
}

bool ClipPathShape::equals(const ClipPathShape* other) const
{
    if (m_type != other->m_type || m_box != other->m_box ||
        m_radiusX != other->m_radiusX || m_radiusY != other->m_radiusY ||
        m_evenOdd != other->m_evenOdd ||
        m_hasPosition != other->m_hasPosition) {
        return false;
    }
    for (size_t i = 0; i < 4; i++) {
        if (!(m_args[i] == other->m_args[i])) {
            return false;
        }
    }
    return lengthsEqual(m_insetRadii, other->m_insetRadii) &&
           lengthsEqual(m_points, other->m_points);
}

void ClipPathShape::resolveFontRelativeLengths(
    Length curFontSize, Length rootFontSize, Optional<Font*> font,
    LayoutUnit viewportWidth, LayoutUnit viewportHeight, ComputedStyle* style)
{
    auto resolve = [&](Length& length) {
        if (!length.isComputed()) {
            length.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                         viewportWidth, viewportHeight, style);
        }
    };
    for (auto& length : m_args) {
        resolve(length);
    }
    for (size_t i = 0; i < m_insetRadii.size(); i++) {
        resolve(m_insetRadii[i].m_length);
    }
    for (size_t i = 0; i < m_points.size(); i++) {
        resolve(m_points[i].m_length);
    }
}

static void appendLength(StringBuilder& builder, const Length& length)
{
    const char* unit = "px";
    float value = 0;
    switch (length.type()) {
    case Length::Fixed:
        value = length.fixed();
        break;
    case Length::Percent:
        value = length.percent() * 100;
        unit = "%";
        break;
    case Length::Em:
        value = length.numberData();
        unit = "em";
        break;
    case Length::Rem:
        value = length.numberData();
        unit = "rem";
        break;
    case Length::Ex:
        value = length.numberData();
        unit = "ex";
        break;
    case Length::Ch:
        value = length.numberData();
        unit = "ch";
        break;
    case Length::Vw:
        value = length.numberData();
        unit = "vw";
        break;
    case Length::Vh:
        value = length.numberData();
        unit = "vh";
        break;
    case Length::Vmin:
        value = length.numberData();
        unit = "vmin";
        break;
    case Length::Vmax:
        value = length.numberData();
        unit = "vmax";
        break;
    default:
        STARFISH_ASSERT_NOT_REACHED();
        break;
    }
    builder.appendString(String::fromFloat(value));
    builder.appendString(unit, strlen(unit));
}

// |text| is the specified source text, or null for a computed value.
static void appendItem(StringBuilder& builder, const Length& length,
                       const std::string* text)
{
    if (text) {
        builder.appendString(text->data(), text->length());
    } else {
        appendLength(builder, length);
    }
}

static void appendRadius(StringBuilder& builder, ClipPathShape::Radius radius,
                         const Length& length, const std::string* text)
{
    switch (radius) {
    case ClipPathShape::Radius::Length:
        appendItem(builder, length, text);
        break;
    case ClipPathShape::Radius::ClosestSide:
        builder.appendString("closest-side");
        break;
    case ClipPathShape::Radius::FarthestSide:
        builder.appendString("farthest-side");
        break;
    case ClipPathShape::Radius::ClosestCorner:
        builder.appendString("closest-corner");
        break;
    case ClipPathShape::Radius::FarthestCorner:
        builder.appendString("farthest-corner");
        break;
    }
}

// Appends the shortest 1-4 value form of |values| (top, right, bottom, left
// order, as for the margin shorthand).
static void appendSides(StringBuilder& builder, const Length* values,
                        const std::string* texts)
{
    size_t count = 4;
    if (values[3] == values[1]) {
        count = 3;
        if (values[2] == values[0]) {
            count = 2;
            if (values[1] == values[0]) {
                count = 1;
            }
        }
    }
    for (size_t i = 0; i < count; i++) {
        if (i) {
            builder.appendChar(' ');
        }
        appendItem(builder, values[i], texts ? &texts[i] : nullptr);
    }
}

static const char* boxName(ClipPathShape::Box box)
{
    switch (box) {
    case ClipPathShape::Box::MarginBox:
        return "margin-box";
    case ClipPathShape::Box::BorderBox:
        return "border-box";
    case ClipPathShape::Box::PaddingBox:
        return "padding-box";
    case ClipPathShape::Box::ContentBox:
        return "content-box";
    case ClipPathShape::Box::FillBox:
        return "fill-box";
    case ClipPathShape::Box::StrokeBox:
        return "stroke-box";
    case ClipPathShape::Box::ViewBox:
        return "view-box";
    case ClipPathShape::Box::Unspecified:
        break;
    }
    return nullptr;
}

String* ClipPathShape::toString() const
{
    if (m_specifiedText) {
        return m_specifiedText;
    }
    return serialize(nullptr);
}

String* ClipPathShape::serialize(const SpecifiedTexts* texts) const
{
    StringBuilder builder;
    switch (m_type) {
    case Type::BoxOnly:
        break;
    case Type::Inset: {
        builder.appendString("inset(");
        appendSides(builder, m_args, texts ? texts->m_args : nullptr);
        if (m_insetRadii.size()) {
            Length horizontal[4], vertical[4];
            bool isZero = true;
            for (size_t i = 0; i < 4; i++) {
                horizontal[i] = m_insetRadii[i].m_length;
                vertical[i] = m_insetRadii[i + 4].m_length;
                isZero =
                    isZero && horizontal[i].isZero() && vertical[i].isZero();
            }
            if (!isZero) {
                builder.appendString(" round ");
                appendSides(builder, horizontal,
                            texts ? texts->m_radii : nullptr);
                bool sameAxes = true;
                for (size_t i = 0; i < 4; i++) {
                    sameAxes = sameAxes && horizontal[i] == vertical[i];
                }
                if (!sameAxes) {
                    builder.appendString(" / ");
                    appendSides(builder, vertical,
                                texts ? texts->m_radii + 4 : nullptr);
                }
            }
        }
        builder.appendChar(')');
        break;
    }
    case Type::Circle:
    case Type::Ellipse: {
        bool isCircle = m_type == Type::Circle;
        if (isCircle) {
            builder.appendString("circle(");
        } else {
            builder.appendString("ellipse(");
        }
        bool hasRadius = m_radiusX != Radius::ClosestSide ||
                         m_radiusY != Radius::ClosestSide;
        if (hasRadius) {
            appendRadius(builder, m_radiusX, m_args[0],
                         texts ? &texts->m_args[0] : nullptr);
            if (!isCircle) {
                builder.appendChar(' ');
                appendRadius(builder, m_radiusY, m_args[1],
                             texts ? &texts->m_args[1] : nullptr);
            }
        }
        if (m_hasPosition) {
            if (hasRadius) {
                builder.appendChar(' ');
            }
            builder.appendString("at ");
            appendItem(builder, m_args[2], texts ? &texts->m_args[2] : nullptr);
            builder.appendChar(' ');
            appendItem(builder, m_args[3], texts ? &texts->m_args[3] : nullptr);
        }
        builder.appendChar(')');
        break;
    }
    case Type::Polygon:
        builder.appendString("polygon(");
        if (m_evenOdd) {
            builder.appendString("evenodd, ");
        }
        for (size_t i = 0; i < m_points.size(); i += 2) {
            if (i) {
                builder.appendString(", ");
            }
            appendItem(builder, m_points[i].m_length,
                       texts ? &texts->m_points[i] : nullptr);
            builder.appendChar(' ');
            appendItem(builder, m_points[i + 1].m_length,
                       texts ? &texts->m_points[i + 1] : nullptr);
        }
        builder.appendChar(')');
        break;
    }

    // The box is omitted when it is the default border-box, unless it is the
    // whole value.
    const char* box = boxName(m_box);
    bool omitBox = !box || (m_box == Box::BorderBox && m_type != Type::BoxOnly);
    if (!omitBox) {
        if (m_type != Type::BoxOnly) {
            builder.appendChar(' ');
        }
        builder.appendString(box, strlen(box));
    }
    return builder.finalize();
}
} // namespace Starfish
