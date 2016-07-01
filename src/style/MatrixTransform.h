/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef __StarFishMatrixTransformData__
#define __StarFishMatrixTransformData__

#include "style/Style.h"

namespace StarFish {

class MatrixTransform: public gc {
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

    double a() { return m_a; }
    double b() { return m_b; }
    double c() { return m_c; }
    double d() { return m_d; }
    double e() { return m_e; }
    double f() { return m_f; }

    bool operator==(const MatrixTransform& o)
    {
        return this->m_a == o.m_a && this->m_b == o.m_b && this->m_c == o.m_c
            && this->m_d == o.m_d && this->m_e == o.m_e && this->m_f == o.m_f;
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
