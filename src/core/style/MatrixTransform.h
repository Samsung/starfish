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

#ifndef __StarFishMatrixTransform__
#define __StarFishMatrixTransform__

#include "core/style/Style.h"

namespace StarFish {

class MatrixTransform : public gc {
public:
    MatrixTransform(double a, double b, double c, double d, double e, double f)
        : m_a(a)
        , m_b(b)
        , m_c(c)
        , m_d(d)
        , m_e(e)
        , m_f(f)
    {
    }

    ~MatrixTransform()
    {
    }

    void setData(double a, double b, double c, double d, double e, double f)
    {
        m_a = a;
        m_b = b;
        m_c = c;
        m_d = d;
        m_e = e;
        m_f = f;
    }

    double a()
    {
        return m_a;
    }
    double b()
    {
        return m_b;
    }
    double c()
    {
        return m_c;
    }
    double d()
    {
        return m_d;
    }
    double e()
    {
        return m_e;
    }
    double f()
    {
        return m_f;
    }

    bool operator==(const MatrixTransform& o)
    {
        return this->m_a == o.m_a && this->m_b == o.m_b && this->m_c == o.m_c &&
               this->m_d == o.m_d && this->m_e == o.m_e && this->m_f == o.m_f;
    }

    bool operator!=(const MatrixTransform& o)
    {
        return !operator==(o);
    }

private:
    double m_a;
    double m_b;
    double m_c;
    double m_d;
    double m_e;
    double m_f;
};
}

#endif
