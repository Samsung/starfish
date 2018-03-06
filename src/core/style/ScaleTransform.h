/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishScaleTransform__
#define __StarFishScaleTransform__

#include "core/style/Style.h"

namespace StarFish {

class ScaleTransform : public gc {
public:
    ScaleTransform(double a, double b)
        : m_x(a)
        , m_y(b)
    {
    }

    ~ScaleTransform()
    {
    }

    void setData(double a, double b)
    {
        m_x = a;
        m_y = b;
    }

    double x()
    {
        return m_x;
    }
    double y()
    {
        return m_y;
    }

    bool operator==(const ScaleTransform& o)
    {
        return this->m_x == o.m_x && this->m_y == o.m_y;
    }

    bool operator!=(const ScaleTransform& o)
    {
        return !operator==(o);
    }

private:
    double m_x;
    double m_y;
};
}

#endif
