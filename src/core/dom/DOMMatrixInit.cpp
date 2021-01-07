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

#include "StarfishConfig.h"
#include "core/dom/DOMMatrixInit.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {

// currently only supprt 2D only

DOMMatrixInit::DOMMatrixInit()
    : m_m13(0)
    , m_m14(0)
    , m_m23(0)
    , m_m24(0)
    , m_m31(0)
    , m_m32(0)
    , m_m33(1.0)
    , m_m34(0)
    , m_m43(0)
    , m_m44(1.0)
    , m_is2D(true)
{
}
} // namespace Starfish
