/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
