
/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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
#include "Style.h"
#include "CSSAngle.h"

namespace StarFish {

CSSAngle::CSSAngle(String* str, float f)
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
        return convertFromRadToDeg(m_value);
    } else if (m_kind == GRAD) {
        return convertFromGradToDeg(m_value);
    } else if (m_kind == TURN) {
        return convertFromTurnToDeg(m_value);
    }

    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

String* CSSAngle::toString() const
{
    std::stringstream ss(std::stringstream::in | std::stringstream::out);
    ss << m_value;
    std::string stdStr = ss.str();
    if (m_kind == DEG) {
        return String::fromUTF8(stdStr.append("deg").c_str());
    } else if (m_kind == RAD) {
        return String::fromUTF8(stdStr.append("rad").c_str());
    } else if (m_kind == GRAD) {
        return String::fromUTF8(stdStr.append("grad").c_str());
    } else if (m_kind == TURN) {
        return String::fromUTF8(stdStr.append("turn").c_str());
    }

    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}
} // namespace StarFish
