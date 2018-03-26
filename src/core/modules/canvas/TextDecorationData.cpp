/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#include "core/modules/canvas/TextDecorationData.h"
#include "core/style/ComputedStyle.h"

namespace StarFish {

void TextDecorationData::merge(ComputedStyle* style)
{
    if (style->textDecorationLine() == UnderlineTextDecorationLineValue) {
        m_hasUnderLine = true;
        m_underLineColor = style->color();
    } else if (style->textDecorationLine() ==
               LineThroughTextDecorationLineValue) {
        m_hasLineThrough = true;
        m_lineThroughColor = style->color();
    }

    Nullable<Unit::Color> c =
        style->rareComputedStyleData()->textDecorationColor();
    if (c.hasValue()) {
        m_underLineColor = c.getValue();
        m_lineThroughColor = c.getValue();
    }
}
}
