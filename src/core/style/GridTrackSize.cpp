/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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
#include "core/style/GridTrackSize.h"

namespace Starfish {

String* GridTrackSize::toStringWithGridLengths(GCVector<GridTrackSize*>* v)
{
    StringBuilder builder;

    for (size_t i = 0; i < v->size(); i++) {
        builder.appendString((*v)[i]->toString());
        if (i != v->size() - 1) {
            builder.appendString(" ");
        }
    }

    return builder.finalize();
}

GridTrackSize::GridTrackSize(GridTrackSizeType type)
    : m_type(type)
{
}

bool GridTrackSize::equals(GridTrackSize* other)
{
    if (m_type != other->m_type) {
        return false;
    }

    if (m_type == GridTrackSizeType::kLength) {
        return as<GridTrackSizeLength>()->gridLegnth() ==
               other->as<GridTrackSizeLength>()->gridLegnth();
    } else if (m_type == GridTrackSizeType::kMinMax) {
        return as<GridTrackSizeMinMax>()->min() ==
                   other->as<GridTrackSizeMinMax>()->min() &&
               as<GridTrackSizeMinMax>()->max() ==
                   other->as<GridTrackSizeMinMax>()->max();
    }
    return false;
}

String* GridTrackSize::toString()
{
    StringBuilder builder;
    if (m_type == GridTrackSizeType::kLength) {
        builder.appendString(
            as<GridTrackSizeLength>()->gridLegnth().toString());
    } else if (m_type == GridTrackSizeType::kMinMax) {
        builder.appendString("minmax(");
        builder.appendString(as<GridTrackSizeMinMax>()->min().toString());
        builder.appendString(", ");
        builder.appendString(as<GridTrackSizeMinMax>()->max().toString());
        builder.appendString(")");
    } else if (m_type == GridTrackSizeType::kMinContent) {
        builder.appendString("min-content");
    } else if (m_type == GridTrackSizeType::kMaxContent) {
        builder.appendString("max-content");
    }
    return builder.finalize();
}

GridTrackSizeLength::GridTrackSizeLength(const GridLength& length)
    : GridTrackSize(GridTrackSizeType::kLength)
    , m_length(length)
{
}

void GridTrackSizeLength::checkComputed(Length curFontSize, Length rootFontSize,
                                        Font* font, LayoutSize windowSize,
                                        ComputedStyle* cs)
{
    m_length.checkComputed(curFontSize, rootFontSize, font, windowSize, cs);
}

GridTrackSizeMinMax::GridTrackSizeMinMax(const GridLength& min,
                                         const GridLength& max)
    : GridTrackSize(GridTrackSizeType::kMinMax)
    , m_min(min)
    , m_max(max)
{
}

void GridTrackSizeMinMax::checkComputed(Length curFontSize, Length rootFontSize,
                                        Font* font, LayoutSize windowSize,
                                        ComputedStyle* cs)
{
    m_min.checkComputed(curFontSize, rootFontSize, font, windowSize, cs);
    m_max.checkComputed(curFontSize, rootFontSize, font, windowSize, cs);
}

GridTrackSizeMinContent::GridTrackSizeMinContent()
    : GridTrackSize(GridTrackSizeType::kMinContent)
{
}

GridTrackSizeMaxContent::GridTrackSizeMaxContent()
    : GridTrackSize(GridTrackSizeType::kMaxContent)
{
}

GridTrackSizeRepeat::GridTrackSizeRepeat(
    GridTrackSizeType type, const GCVector<GridTrackSize*>& gridTrackSizes)
    : GridTrackSize(type)
    , m_gridTrackSizes(gridTrackSizes)
{
}

void GridTrackSizeRepeat::checkComputed(Length curFontSize, Length rootFontSize,
                                        Font* font, LayoutSize windowSize,
                                        ComputedStyle* cs)
{
    for (auto& gridTrackSize : m_gridTrackSizes) {
        gridTrackSize->checkComputed(curFontSize, rootFontSize, font,
                                     windowSize, cs);
    }
}

GridTrackSizeFixedRepeat::GridTrackSizeFixedRepeat(
    const GCVector<GridTrackSize*>& gridTrackSizes, uint32_t repeatCount)
    : GridTrackSizeRepeat(GridTrackSizeType::kFixedRepeat, gridTrackSizes)
    , m_repeatCount(repeatCount)
{
}

GridTrackSizeAutoRepeat::GridTrackSizeAutoRepeat(
    const GCVector<GridTrackSize*>& gridTrackSizes,
    AutoRepeatType autoRepeatType)
    : GridTrackSizeRepeat(GridTrackSizeType::kAutoRepeat, gridTrackSizes)
    , m_autoRepeatType(autoRepeatType)
{
}

} // namespace Starfish
