
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
#include "CSSAngle.h"

namespace Starfish {

CSSAngle::CSSAngle(String* str, float f)
    : m_kind(DEG)
{
    if (str->length() == 0 || str->equals("deg")) {
        m_kind = DEG;
    } else if (str->equals("grad")) {
        m_kind = GRAD;
    } else if (str->equals("rad")) {
        m_kind = RAD;
    } else if (str->equals("turn")) {
        m_kind = TURN;
    }
    m_value = f;
}

CSSAngle::CSSAngle(const CSSTokenValue& str, float f)
    : m_kind(DEG)
{
    if (str.length() == 0 || str == "deg") {
        m_kind = DEG;
    } else if (str == "grad") {
        m_kind = GRAD;
    } else if (str == "rad") {
        m_kind = RAD;
    } else if (str == "turn") {
        m_kind = TURN;
    }
    m_value = f;
}

float CSSAngle::toDegreeValue() const
{
    if (m_kind == DEG) {
        return m_value;
    } else if (m_kind == RAD) {
        return UnitHelper::convertFromRadToDeg(m_value);
    } else if (m_kind == GRAD) {
        return UnitHelper::convertFromGradToDeg(m_value);
    } else if (m_kind == TURN) {
        return UnitHelper::convertFromTurnToDeg(m_value);
    }

    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return m_value;
}

String* CSSAngle::toString() const
{
    std::stringstream ss(std::stringstream::in | std::stringstream::out);
    ss << m_value;
    std::string stdStr = ss.str();
    if (m_kind == DEG) {
        stdStr.append("deg");
        return String::fromUTF8(stdStr.data(), stdStr.size());
    } else if (m_kind == RAD) {
        stdStr.append("rad");
        return String::fromUTF8(stdStr.data(), stdStr.size());
    } else if (m_kind == GRAD) {
        stdStr.append("grad");
        return String::fromUTF8(stdStr.data(), stdStr.size());
    } else if (m_kind == TURN) {
        stdStr.append("turn");
        return String::fromUTF8(stdStr.data(), stdStr.size());
        ;
    }

    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return String::emptyString;
}
} // namespace Starfish
