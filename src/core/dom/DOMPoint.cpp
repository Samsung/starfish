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

#include "StarFishConfig.h"
#include "core/dom/DOMPoint.h"

namespace StarFish {
DOMPointInit::DOMPointInit(double inX, double inY, double inZ, double inW)
    : m_x(inX)
    , m_y(inY)
    , m_z(inZ)
    , m_w(inW)
{
}

double DOMPointInit::x() const
{
    return m_x;
}

void DOMPointInit::setX(double x)
{
    m_x = x;
}

double DOMPointInit::y() const
{
    return m_y;
}

void DOMPointInit::setY(double y)
{
    m_y = y;
}

double DOMPointInit::z() const
{
    return m_z;
}

void DOMPointInit::setZ(double z)
{
    m_z = z;
}

double DOMPointInit::w() const
{
    return m_w;
}

void DOMPointInit::setW(double w)
{
    m_w = w;
}

DOMPoint::DOMPoint(Document* document, double x, double y, double z, double w)
    : DOMPointReadOnly(document, x, y, z, w)
{
}

DOMPoint::DOMPoint(Document* document, const DOMPointInit& pi)
    : DOMPointReadOnly(document, pi.x(), pi.y(), pi.z(), pi.w())
{
}
}
