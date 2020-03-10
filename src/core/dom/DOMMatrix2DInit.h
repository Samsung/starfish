/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishDOMMatrix2DInit__
#define __StarfishDOMMatrix2DInit__

namespace Starfish {

struct DOMMatrix2DInit {
public:
    STARFISH_MAKE_STACK_ALLOCATED()

    DOMMatrix2DInit()
    {
    }

    DEFINE_GETTER_SETTER_WITH_HASFLAG(double, a, A);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(double, b, B);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(double, c, C);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(double, d, D);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(double, e, E);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(double, f, F);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(double, m11, M11);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(double, m12, M12);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(double, m21, M21);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(double, m22, M22);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(double, m41, M41);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(double, m42, M42);

private:
    bool m_hasA{ false };
    bool m_hasB{ false };
    bool m_hasC{ false };
    bool m_hasD{ false };
    bool m_hasE{ false };
    bool m_hasF{ false };
    bool m_hasM11{ false };
    bool m_hasM12{ false };
    bool m_hasM21{ false };
    bool m_hasM22{ false };
    bool m_hasM41{ false };
    bool m_hasM42{ false };

    double m_a{ 0 };
    double m_b{ 0 };
    double m_c{ 0 };
    double m_d{ 0 };
    double m_e{ 0 };
    double m_f{ 0 };
    double m_m11{ 0 };
    double m_m12{ 0 };
    double m_m21{ 0 };
    double m_m22{ 0 };
    double m_m41{ 0 };
    double m_m42{ 0 };
};
}

#endif
