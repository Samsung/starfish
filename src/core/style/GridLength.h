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

#ifndef __StarfishGridLength__
#define __StarfishGridLength__

namespace Starfish {

enum class GridLengthType {
    kLength,
    kFlexibleLength, // https://drafts.csswg.org/css-grid/#fr-unit
};

class GridLength {
public:
    GridLength();

    GridLength(const Length& length);

    GridLength(double flexibleLength);

    bool isLength() const
    {
        return m_type == GridLengthType::kLength;
    }

    bool isFlexibleLength() const
    {
        return m_type == GridLengthType::kFlexibleLength;
    }

    const Length& length() const
    {
        return m_length;
    }

    double flexibleLength() const
    {
        return m_flexibleLength;
    }

    bool operator==(const GridLength& o) const;

    GridLengthType type() const
    {
        return m_type;
    }

    String* toString() const;

    void checkComputed(Length curFontSize, Length rootFontSize, Font* font,
                       LayoutSize windowSize, ComputedStyle* cs);

private:
    Length m_length;
    double m_flexibleLength = 0.0f;
    GridLengthType m_type;
};

} // namespace Starfish

#endif
