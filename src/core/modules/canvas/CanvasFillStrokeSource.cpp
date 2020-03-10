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
