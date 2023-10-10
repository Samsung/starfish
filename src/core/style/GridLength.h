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

    GridLength(double fr);

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

    Length& mutableLength()
    {
        return m_length;
    }

    double fr() const
    {
        return m_fr;
    }

    bool isPercentage() const
    {
        return m_type == GridLengthType::kLength && m_length.isPercent();
    }

    bool operator==(const GridLength& o) const;

    bool isFixed() const
    {
        return m_type == GridLengthType::kLength && m_length.isFixed();
    }

    bool isAuto() const
    {
        return m_type == GridLengthType::kLength && m_length.isAuto();
    }

    GridLengthType type() const
    {
        return m_type;
    }

    String* toString() const;

private:
    Length m_length;
    double m_fr = 0.0f;
    GridLengthType m_type;
};

// from parsing css properties

enum class GridTrackType {
    kLength,
    kFlexibleLength,
    kMinMax,
    kMinContent,
    kMaxContent,
};

class GridTrackSize : public gc {
public:
    static String* toStringWithGridLengths(GCVector<GridTrackSize>* v);

    GridTrackSize();

    GridTrackSize(const GridLength& length, GridTrackType type);

    GridTrackSize(const GridLength& min, const GridLength& max,
                  GridTrackType type = GridTrackType::kMinMax);

    GridTrackSize(GridTrackType type);

    bool isLength() const
    {
        return m_type == GridTrackType::kLength;
    }

    bool isFlexibleLength() const
    {
        return m_type == GridTrackType::kFlexibleLength;
    }

    bool isMinMax() const
    {
        return m_type == GridTrackType::kMinMax;
    }

    bool isMinContent() const
    {
        return m_type == GridTrackType::kMinContent;
    }

    bool isMaxContent() const
    {
        return m_type == GridTrackType::kMaxContent;
    }

    const Length& length() const
    {
        return m_data1.length();
    }

    Length& mutableLength()
    {
        return m_data1.mutableLength();
    }

    double fr() const
    {
        return m_data1.fr();
    }

    const GridLength& min() const
    {
        return m_data1;
    }

    const GridLength& max() const
    {
        return m_data2;
    }

    bool isPercentage() const
    {
        return m_type == GridTrackType::kLength && m_data1.length().isPercent();
    }

    bool operator==(const GridTrackSize& o) const;

    bool isAuto() const
    {
        return m_type == GridTrackType::kLength && m_data1.length().isAuto();
    }

    String* toString() const;

private:
    GridLength m_data1;
    GridLength m_data2;
    GridTrackType m_type;
};
} // namespace Starfish

#endif
