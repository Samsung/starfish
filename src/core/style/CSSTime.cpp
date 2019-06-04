
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

#include "StarfishConfig.h"
#include "Style.h"
#include "CSSTime.h"

namespace Starfish {
CSSTime::CSSTime(String* str, float f)
    : m_kind(S)
{
    if (str->length() == 0 || str->equals("s")) {
        m_kind = S;
    } else if (str->equals("ms")) {
        m_kind = MS;
    }
    m_value = f;
}

CSSTime::CSSTime(const CSSTokenValue& str, float f)
    : m_kind(S)
{
    if (str.length() == 0 || str == "s") {
        m_kind = S;
    } else if (str == "ms") {
        m_kind = MS;
    }
    m_value = f;
}

double CSSTime::toTimeValue() const
{
    if (m_kind == S) {
        return m_value * 1000; // to ms
    } else if (m_kind == MS) {
        return m_value;
    }

    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return m_value;
}

String* CSSTime::toString() const
{
    std::stringstream ss(std::stringstream::in | std::stringstream::out);
    ss << m_value;
    std::string stdStr = ss.str();
    if (m_kind == S) {
        stdStr.append("s");
        return String::fromUTF8(stdStr.data(), stdStr.size());
    } else if (m_kind == MS) {
        stdStr.append("ms");
        return String::fromUTF8(stdStr.data(), stdStr.size());
    }

    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return String::emptyString;
}
}
