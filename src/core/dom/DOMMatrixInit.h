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

#ifndef __StarfishDOMMatrixInit__
#define __StarfishDOMMatrixInit__

#include "core/dom/DOMMatrix2DInit.h"

namespace Starfish {
struct DOMMatrixInit : public DOMMatrix2DInit {
public:
    DOMMatrixInit();

    DEFINE_GETTER_SETTER(double, m13, M13);
    DEFINE_GETTER_SETTER(double, m14, M14);
    DEFINE_GETTER_SETTER(double, m23, M23);
    DEFINE_GETTER_SETTER(double, m24, M24);
    DEFINE_GETTER_SETTER(double, m31, M31);
    DEFINE_GETTER_SETTER(double, m32, M32);
    DEFINE_GETTER_SETTER(double, m33, M33);
    DEFINE_GETTER_SETTER(double, m34, M34);
    DEFINE_GETTER_SETTER(double, m43, M43);
    DEFINE_GETTER_SETTER(double, m44, M44);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(bool, is2D, Is2D);

private:
    bool m_hasIs2D{ false };

    double m_m13{ 0 };
    double m_m14{ 0 };
    double m_m23{ 0 };
    double m_m24{ 0 };
    double m_m31{ 0 };
    double m_m32{ 0 };
    double m_m33{ 1 };
    double m_m34{ 0 };
    double m_m43{ 0 };
    double m_m44{ 1 };
    bool m_is2D{ true };
};
} // namespace Starfish
#endif
