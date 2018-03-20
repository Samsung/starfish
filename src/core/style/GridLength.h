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

#ifndef __StarFishGridLength__
#define __StarFishGridLength__

#include "core/layout/LayoutUtil.h"

namespace StarFish {

class GridLength : public gc {
public:
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
        return m_length == o.m_length && m_fr == o.m_fr && m_type == o.m_type;
    }

    bool isAuto() const
    {
        return m_type == LengthType && m_length.isAuto();
    }

    String* toString()
    {
        StringBuilder builder;
        if (m_type == LengthType) {
            String* value = m_length.dumpString();
            builder.appendString(value);
            builder.appendString("px");
        } else {
            char temp[100];
            snprintf(temp, sizeof(temp), "%.1f", m_fr);
            String* value = String::fromUTF8(temp);
            builder.appendString(value);
            builder.appendString("fr");
        }
        return builder.finalize();
    }

    static String* toStringWithGridLengths(GCVector<GridLength>* v)
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
    Length m_length;
    double m_fr;
    enum GridLengthType {
        LengthType,
        FrType,
    };
    GridLengthType m_type;
};
}

#endif
