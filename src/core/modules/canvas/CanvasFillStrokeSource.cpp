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
#include "CanvasFillStrokeSource.h"

namespace Starfish {
CanvasFillStrokeSource::CanvasFillStrokeSource()
    : m_type(CanvasFillStrokeSourceType::Invalid)
    , m_value()
{
}

CanvasFillStrokeSource::CanvasFillStrokeSource(Unit::Color color)
    : m_type(CanvasFillStrokeSourceType::Color)
    , m_value(color)
{
}

CanvasFillStrokeSource::CanvasFillStrokeSource(CanvasStyle canvasStyle)
    : m_type(CanvasFillStrokeSourceType::CanvasStyle)
    , m_value(canvasStyle)
{
}

CanvasFillStrokeSource::CanvasFillStrokeSource(
    const CanvasFillStrokeSource& other)
{
    if (this == &other) {
        return;
    }

    m_type = other.m_type;
    switch (m_type) {
    case CanvasFillStrokeSourceType::Invalid:
    case CanvasFillStrokeSourceType::Color:
        m_value.m_color = other.m_value.m_color;
        break;
    case CanvasFillStrokeSourceType::CanvasStyle:
        m_value.m_canvasStyle = other.m_value.m_canvasStyle;
        break;
    default:
        STARFISH_ASSERT_NOT_REACHED();
        break;
    }
}

CanvasFillStrokeSource& CanvasFillStrokeSource::operator=(
    const CanvasFillStrokeSource& other)
{
    if (this == &other) {
        return *this;
    }

    m_type = other.m_type;
    switch (m_type) {
    case CanvasFillStrokeSourceType::Invalid:
    case CanvasFillStrokeSourceType::Color:
        m_value.m_color = other.m_value.m_color;
        break;
    case CanvasFillStrokeSourceType::CanvasStyle:
        m_value.m_canvasStyle = other.m_value.m_canvasStyle;
        break;
    default:
        STARFISH_ASSERT_NOT_REACHED();
        break;
    }
    return *this;
}

bool CanvasFillStrokeSource::isCanvasAvailableSource()
{
    if (isInvalidType()) {
        return false;
    }
    if (isCanvasStyleType()) {
        if (m_value.m_canvasStyle.isDOMStringValue() ||
            m_value.m_canvasStyle.isNoneValue()) {
            return false;
        }
    }
    return true;
}
} // namespace Starfish
