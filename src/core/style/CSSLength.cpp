
/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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
#include "Style.h"
#include "CSSLength.h"

namespace Starfish {

template <typename T>
static CSSLength::Kind computeLengthUnit(T str)
{
    size_t len = str->length();

    if (len == 2) {
        char32_t c0 = str->operator[](0);
        char32_t c1 = str->operator[](1);

        switch (c0) {
        case 'p':
            if (c1 == 'x') {
                return CSSLength::PX;
            }
            if (c1 == 't') {
                return CSSLength::PT;
            }
            if (c1 == 'c') {
                return CSSLength::PC;
            }
            break;
        case 'e':
            if (c1 == 'm') {
                return CSSLength::EM;
            }
            if (c1 == 'x') {
                return CSSLength::EX;
            }
            break;
        case 'i':
            if (c1 == 'n') {
                return CSSLength::INCH;
            }
            break;
        case 'c':
            if (c1 == 'm') {
                return CSSLength::CM;
            }
            if (c1 == 'h') {
                return CSSLength::CH;
            }
            break;
        case 'm':
            if (c1 == 'm') {
                return CSSLength::MM;
            }
            break;
        case 'v':
            if (c1 == 'w') {
                return CSSLength::VW;
            }
            if (c1 == 'h') {
                return CSSLength::VH;
            }
            break;
        default:
            break;
        }
    } else if (len == 3) {
        char32_t c0 = str->operator[](0);
        char32_t c1 = str->operator[](1);
        char32_t c2 = str->operator[](2);

        switch (c0) {
        case 'r':
            if (c1 == 'e' && c2 == 'm') {
                return CSSLength::REM;
            }
            break;
        default:
            break;
        }
    } else if (len == 4) {
        char32_t c0 = str->operator[](0);
        char32_t c1 = str->operator[](1);
        char32_t c2 = str->operator[](2);
        char32_t c3 = str->operator[](3);

        switch (c0) {
        case 'v':
            if (c1 == 'm' && c2 == 'i' && c3 == 'n') {
                return CSSLength::VMIN;
            }
            if (c1 == 'm' && c2 == 'a' && c3 == 'x') {
                return CSSLength::VMAX;
            }
            break;
        default:
            break;
        }
    }
    return CSSLength::PX;
}

CSSLength::CSSLength(const CSSTokenValue& unit, float f)
{
    m_kind = computeLengthUnit(&unit);
    m_value = f;
}

CSSLength::CSSLength(String* unit, float f)
{
    m_kind = computeLengthUnit(unit);
    m_value = f;
}

Length CSSLength::toLength() const
{
    if (m_kind == PX) { // absolute length
        return Length(Length::Fixed, m_value);
    } else if (m_kind == CM) {
        return Length(Length::Fixed, UnitHelper::convertFromCmToPx(m_value));
    } else if (m_kind == MM) {
        return Length(Length::Fixed, UnitHelper::convertFromMmToPx(m_value));
    } else if (m_kind == INCH) {
        return Length(Length::Fixed, UnitHelper::convertFromInToPx(m_value));
    } else if (m_kind == PC) {
        return Length(Length::Fixed, UnitHelper::convertFromPcToPx(m_value));
    } else if (m_kind == PT) {
        return Length(Length::Fixed, UnitHelper::convertFromPtToPx(m_value));
    } else if (m_kind == EM) { // font-relative length
        return Length(Length::Em, m_value);
    } else if (m_kind == EX) { // font-relative length
        return Length(Length::Ex, m_value);
    } else if (m_kind == VW) {
        return Length(Length::Vw, m_value);
    } else if (m_kind == VH) {
        return Length(Length::Vh, m_value);
    } else if (m_kind == VMIN) {
        return Length(Length::Vmin, m_value);
    } else if (m_kind == VMAX) {
        return Length(Length::Vmax, m_value);
    } else if (m_kind == REM) { // font-relative length
        return Length(Length::Rem, m_value);
    } else if (m_kind == CH) { // font-relative length
        return Length(Length::Ch, m_value);
    }

    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
}

String* CSSLength::toString() const
{
    UTF8StringDataNonGCStd stdStr =
        String::fromFloat(m_value)->toUTF8NonGCString();
    if (m_kind == PX) {
        stdStr.append("px");
        return String::fromUTF8(stdStr.data(), stdStr.size());
    } else if (m_kind == CM) {
        stdStr.append("cm");
        return String::fromUTF8(stdStr.data(), stdStr.size());
    } else if (m_kind == MM) {
        stdStr.append("mm");
        return String::fromUTF8(stdStr.data(), stdStr.size());
    } else if (m_kind == INCH) {
        stdStr.append("in");
        return String::fromUTF8(stdStr.data(), stdStr.size());
    } else if (m_kind == PC) {
        stdStr.append("pc");
        return String::fromUTF8(stdStr.data(), stdStr.size());
    } else if (m_kind == PT) {
        stdStr.append("pt");
        return String::fromUTF8(stdStr.data(), stdStr.size());
    } else if (m_kind == EM) {
        stdStr.append("em");
        return String::fromUTF8(stdStr.data(), stdStr.size());
    } else if (m_kind == EX) {
        stdStr.append("ex");
        return String::fromUTF8(stdStr.data(), stdStr.size());
    } else if (m_kind == VW) {
        stdStr.append("vw");
        return String::fromUTF8(stdStr.data(), stdStr.size());
    } else if (m_kind == VH) {
        stdStr.append("vh");
        return String::fromUTF8(stdStr.data(), stdStr.size());
    } else if (m_kind == VMIN) {
        stdStr.append("vmin");
        return String::fromUTF8(stdStr.data(), stdStr.size());
    } else if (m_kind == VMAX) {
        stdStr.append("vmax");
        return String::fromUTF8(stdStr.data(), stdStr.size());
    } else if (m_kind == REM) {
        stdStr.append("rem");
        return String::fromUTF8(stdStr.data(), stdStr.size());
    } else if (m_kind == CH) {
        stdStr.append("ch");
        return String::fromUTF8(stdStr.data(), stdStr.size());
    }
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
}

} // namespace Starfish
