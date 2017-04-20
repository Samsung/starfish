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

#include "dom/DOMPoint.h"

namespace StarFish {
DOMPointInit::DOMPointInit(double inX, double inY, double inZ, double inW)
    : x(inX)
    , y(inY)
    , z(inZ)
    , w(inW)
{
}

DOMPoint::DOMPoint(double x, double y, double z, double w)
    : DOMPointReadOnly(x, y, z, w)
{
}

DOMPoint::DOMPoint(const DOMPointInit& pi)
    : DOMPointReadOnly(pi.x, pi.y, pi.z, pi.w)
{
}
}
