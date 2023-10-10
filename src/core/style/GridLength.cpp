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
    : m_fr(0)
    , m_type(GridLengthType::kLength)
{
}

GridLength::GridLength(const Length& length)
    : m_length(length)
    , m_fr(0)
    , m_type(GridLengthType::kLength)
{
}

GridLength::GridLength(double fr)
    : m_fr(fr)
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
        return m_fr == o.fr();
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
        snprintf(temp, sizeof(temp), "%.1f", m_fr);
        String* value = String::fromUTF8(temp, strnlen(temp, sizeof(temp)));
        builder.appendString(value);
        builder.appendString("fr");
    }
    return builder.finalize();
}

String* GridTrackSize::toStringWithGridLengths(GCVector<GridTrackSize>* v)
{
    StringBuilder builder;

    for (size_t i = 0; i < v->size(); i++) {
        builder.appendString((*v)[i].toString());
        if (i != v->size() - 1) {
            builder.appendString(" ");
        }
    }

    return builder.finalize();
}

GridTrackSize::GridTrackSize()
    : m_type(GridTrackType::kLength)
{
}

GridTrackSize::GridTrackSize(const GridLength& length, GridTrackType type)
    : m_data1(length)
    , m_data2(length)
    , m_type(type)
{
}

GridTrackSize::GridTrackSize(const GridLength& min, const GridLength& max,
                             GridTrackType type)
    : m_data1(min)
    , m_data2(max)
    , m_type(type)
{
}

GridTrackSize::GridTrackSize(GridTrackType type)
    : m_type(type)
{
}

bool GridTrackSize::operator==(const GridTrackSize& o) const
{
    if (m_type != o.m_type) {
        return false;
    }

    if (m_type == GridTrackType::kLength) {
        return m_data1 == o.min();
    } else if (m_type == GridTrackType::kFlexibleLength) {
        return m_data1 == o.min();
    } else if (m_type == GridTrackType::kMinMax) {
        return m_data1 == o.min() && m_data2 == o.max();
    }
    return false;
}

String* GridTrackSize::toString() const
{
    StringBuilder builder;
    if (m_type == GridTrackType::kLength ||
        m_type == GridTrackType::kFlexibleLength) {
        builder.appendString(m_data1.toString());
    } else if (m_type == GridTrackType::kMinMax) {
        builder.appendString("minmax(");
        builder.appendString(m_data1.toString());
        builder.appendString(", ");
        builder.appendString(m_data2.toString());
        builder.appendString(")");
    } else if (m_type == GridTrackType::kMinContent) {
        builder.appendString("min-content");
    } else if (m_type == GridTrackType::kMaxContent) {
        builder.appendString("max-content");
    }
    return builder.finalize();
}

} // namespace Starfish
