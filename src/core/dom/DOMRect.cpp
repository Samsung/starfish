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
#include "core/dom/DOMRect.h"

namespace StarFish {
DOMRectInit::DOMRectInit(double inX, double inY, double inWidth,
                         double inHeight)
    : x(inX)
    , y(inY)
    , width(inWidth)
    , height(inHeight)
{
}

DOMRect::DOMRect(Document* document, double x, double y, double width,
                 double height)
    : DOMRectReadOnly(document, x, y, width, height)
{
}

DOMRect::DOMRect(DOMRectReadOnly* rect)
    : DOMRect(rect->scriptBindingInstance()->ownerDocument(), rect->x(),
              rect->y(), rect->width(), rect->height())
{
}

void DOMRect::unite(const DOMRect* other)
{
    if (other->width() <= 0 || other->height() <= 0) {
        return;
    }

    double left = std::min(x(), other->x());
    double top = std::min(y(), other->y());
    double right = std::max(x() + width(), other->x() + other->width());
    double bottom = std::max(y() + height(), other->y() + other->height());

    setX(left);
    setY(top);
    setWidth(right - left);
    setHeight(bottom - top);
}
}
