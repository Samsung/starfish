/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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
#include "core/style/GridLength.h"

namespace Starfish {

GridLength::GridLength()
    : m_flexibleLength(0)
    , m_type(GridLengthType::kLength)
{
}

GridLength::GridLength(const Length& length)
    : m_length(length)
    , m_flexibleLength(0)
    , m_type(GridLengthType::kLength)
{
}

GridLength::GridLength(double flexibleLength)
    : m_flexibleLength(flexibleLength)
    , m_type(GridLengthType::kFlexibleLength)
{
}

bool GridLength::operator==(const GridLength& o) const
{
    if (m_type != o.m_type) {
        return false;
    }

    if (m_type == GridLengthType::kLength) {
        return m_length == o.length();
    } else if (m_type == GridLengthType::kFlexibleLength) {
        return m_flexibleLength == o.flexibleLength();
    }
    return false;
}

String* GridLength::toString() const
{
    StringBuilder builder;
    if (m_type == GridLengthType::kLength) {
        String* value = m_length.dumpString();
        builder.appendString(value);
    } else {
        char temp[100];
        snprintf(temp, sizeof(temp), "%.1f", m_flexibleLength);
        String* value = String::fromUTF8(temp, strnlen(temp, sizeof(temp)));
        builder.appendString(value);
        builder.appendString("fr");
    }
    return builder.finalize();
}

void GridLength::checkComputed(Length curFontSize, Length rootFontSize,
                               Font* font, LayoutSize windowSize,
                               ComputedStyle* cs)
{
    if (m_type == GridLengthType::kLength) {
        m_length.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                       windowSize.width(), windowSize.height(),
                                       cs);
    }
}

} // namespace Starfish
