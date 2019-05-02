/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishDOMMatrix2DInit__
#define __StarfishDOMMatrix2DInit__

namespace Starfish {

struct DOMMatrix2DInit {
public:
    STARFISH_MAKE_STACK_ALLOCATED()
    DOMMatrix2DInit();

    double a() const;
    void setA(double value);

    double b() const;
    void setB(double value);

    double c() const;
    void setC(double value);

    double d() const;
    void setD(double value);

    double e() const;
    void setE(double value);

    double f() const;
    void setF(double value);

    DEFINE_GETTER_SETTER(double, m11, M11);
    DEFINE_GETTER_SETTER(double, m12, M12);
    DEFINE_GETTER_SETTER(double, m21, M21);
    DEFINE_GETTER_SETTER(double, m22, M22);
    DEFINE_GETTER_SETTER(double, m41, M41);
    DEFINE_GETTER_SETTER(double, m42, M42);

private:
    double m_m11;
    double m_m12;
    double m_m21;
    double m_m22;
    double m_m41;
    double m_m42;
};
}

#endif
