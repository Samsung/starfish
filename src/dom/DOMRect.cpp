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
#include "StarFishConfig.h"
#include "dom/DOMRect.h"

namespace StarFish {
DOMRectInit::DOMRectInit(double inX, double inY, double inWidth, double inHeight)
    : x(inX)
    , y(inY)
    , width(inWidth)
    , height(inHeight)
{ }

DOMRect* DOMRect::create(double x, double y, double width, double height)
{
    return new DOMRect(x, y, width, height);
}

DOMRect* DOMRect::create(const DOMRectReadOnly* rect)
{
    return new DOMRect(rect->x(), rect->y(), rect->width(), rect->height());
}

DOMRect::DOMRect(double x, double y, double width, double height)
    : DOMRectReadOnly(x, y, width, height)
{ }

void DOMRect::unite(const DOMRectReadOnly* other)
{
    if (other->width() <= 0 || other->height() <= 0)
        return;

    double left = std::min(x(), other->x());
    double top = std::min(y(), other->y());
    double right = std::max(x()+width(), other->x() + other->width());
    double bottom = std::max(y()+height(), other->y() + other->height());

    setX(left);
    setY(top);
    setWidth(right - left);
    setHeight(bottom - top);
}

}
