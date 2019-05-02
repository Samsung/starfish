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

#include "StarfishConfig.h"
#include "core/dom/DOMMatrix2DInit.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {

DOMMatrix2DInit::DOMMatrix2DInit()
    : m_m11(1.0)
    , m_m12(0)
    , m_m21(0)
    , m_m22(1.0)
    , m_m41(0)
    , m_m42(0)
{
}

double DOMMatrix2DInit::a() const
{
    return m_m11;
}

void DOMMatrix2DInit::setA(double value)
{
    m_m11 = value;
}

double DOMMatrix2DInit::b() const
{
    return m_m12;
}

void DOMMatrix2DInit::setB(double value)
{
    m_m12 = value;
}

double DOMMatrix2DInit::c() const
{
    return m_m21;
}

void DOMMatrix2DInit::setC(double value)
{
    m_m21 = value;
}

double DOMMatrix2DInit::d() const
{
    return m_m22;
}

void DOMMatrix2DInit::setD(double value)
{
    m_m22 = value;
}

double DOMMatrix2DInit::e() const
{
    return m_m41;
}

void DOMMatrix2DInit::setE(double value)
{
    m_m41 = value;
}

double DOMMatrix2DInit::f() const
{
    return m_m42;
}

void DOMMatrix2DInit::setF(double value)
{
    m_m42 = value;
}
}
