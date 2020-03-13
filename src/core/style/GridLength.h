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

#include "core/layout/LayoutUtil.h"

namespace Starfish {

class GridLength {
public:
    enum GridLengthType {
        LengthType,
        FrType,
    };

    GridLength()
        : m_length(Length())
        , m_fr(0)
        , m_type(LengthType)
    {
    }

    GridLength(const Length& length)
        : m_length(length)
        , m_fr(0)
        , m_type(LengthType)
    {
    }

    GridLength(double fr)
        : m_fr(fr)
        , m_type(FrType)
    {
    }

    bool isLength() const
    {
        return m_type == LengthType;
    }

    bool isFr() const
    {
        return m_type == FrType;
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
        return m_type == LengthType && m_length.isPercent();
    }

    bool operator==(const GridLength& o) const
    {
        if (m_type != o.m_type) {
            return false;
        }

        if (m_type == LengthType) {
            return m_length == o.length();
        } else if (m_type == FrType) {
            return m_fr == o.fr();
        }
        return false;
    }

    bool isAuto() const
    {
        return m_type == LengthType && m_length.isAuto();
    }

    GridLengthType type() const
    {
        return m_type;
    }

    String* toString()
    {
        StringBuilder builder;
        if (m_type == LengthType) {
            String* value = m_length.dumpString();
            builder.appendString(value);
            builder.appendString("px");
        } else if (m_type == FrType) {
            char temp[100];
            snprintf(temp, sizeof(temp), "%.1f", m_fr);
            String* value = String::fromUTF8(temp, strnlen(temp, sizeof(temp)));
            builder.appendString(value);
            builder.appendString("fr");
        }
        return builder.finalize();
    }

private:
    Length m_length;
    double m_fr;
    GridLengthType m_type;
};

class GridTrackSize : public gc {
public:
    enum GridTrackType {
        LengthType,
        FrType,
        MinMaxType,
        MinContentType,
        MaxContentType,
    };

    GridTrackSize()
        : m_data1(Length())
        , m_data2(Length())
        , m_type(LengthType)
    {
    }

    GridTrackSize(const GridLength& length, GridTrackType type)
        : m_data1(length)
        , m_data2(length)
        , m_type(type)
    {
    }

    GridTrackSize(const GridLength& min, const GridLength& max,
                  GridTrackType type = MinMaxType)
        : m_data1(min)
        , m_data2(max)
        , m_type(type)
    {
    }

    GridTrackSize(GridTrackType type)
        : m_data1(Length())
        , m_data2(Length())
        , m_type(type)
    {
    }

    bool isLength() const
    {
        return m_type == LengthType;
    }

    bool isFr() const
    {
        return m_type == FrType;
    }

    bool isMinMax() const
    {
        return m_type == MinMaxType;
    }

    bool isMinContent() const
    {
        return m_type == MinContentType;
    }

    bool isMaxContent() const
    {
        return m_type == MaxContentType;
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
        return m_type == LengthType && m_data1.length().isPercent();
    }

    bool operator==(const GridTrackSize& o) const
    {
        if (m_type != o.m_type) {
            return false;
        }

        if (m_type == LengthType) {
            return m_data1 == o.min();
        } else if (m_type == FrType) {
            return m_data1 == o.min();
        } else if (m_type == MinMaxType) {
            return m_data1 == o.min() && m_data2 == o.max();
        }
        return false;
    }

    bool isAuto() const
    {
        return m_type == LengthType && m_data1.length().isAuto();
    }

    String* toString()
    {
        StringBuilder builder;
        if (m_type == LengthType) {
            builder.appendString(m_data1.toString());
            builder.appendString("px");
        } else if (m_type == FrType) {
            builder.appendString(m_data1.toString());
            builder.appendString("fr");
        } else if (m_type == MinMaxType) {
            builder.appendString("minmax(");
            builder.appendString(m_data1.toString());
            builder.appendString(", ");
            builder.appendString(m_data2.toString());
            builder.appendString(")");
        } else if (m_type == MinContentType) {
            builder.appendString("min-content");
        } else if (m_type == MaxContentType) {
            builder.appendString("max-content");
        }
        return builder.finalize();
    }

    static String* toStringWithGridLengths(GCVector<GridTrackSize>* v)
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

private:
    GridLength m_data1;
    GridLength m_data2;
    GridTrackType m_type;
};
}

#endif
