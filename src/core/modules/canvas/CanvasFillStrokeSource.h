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

#ifndef __StarfishCanvasFillStrokeSource__
#define __StarfishCanvasFillStrokeSource__

#include "binding/DOMStringOrCanvasGradientOrCanvasPatternUnion.h"

namespace Starfish {

typedef DOMStringOrCanvasGradientOrCanvasPattern CanvasStyle;

enum class CanvasFillStrokeSourceType {
    Invalid,
    Color,
    CanvasStyle,
};

union CanvasFillStrokeSourceValue {
    Unit::Color m_color;
    CanvasStyle m_canvasStyle;

    ~CanvasFillStrokeSourceValue()
    {
    }

    CanvasFillStrokeSourceValue()
        : m_color()
    {
    }

    CanvasFillStrokeSourceValue(Unit::Color color)
        : m_color(color)
    {
    }

    CanvasFillStrokeSourceValue(CanvasStyle canvasStyle)
        : m_canvasStyle(canvasStyle)
    {
    }
};

class CanvasFillStrokeSource : public gc {
public:
    CanvasFillStrokeSource();
    CanvasFillStrokeSource(Unit::Color color);
    CanvasFillStrokeSource(CanvasStyle canvasStyle);

    ~CanvasFillStrokeSource()
    {
    }

    bool isInvalidType()
    {
        return m_type == CanvasFillStrokeSourceType::Invalid;
    }

    bool isColorType()
    {
        return m_type == CanvasFillStrokeSourceType::Color;
    }

    bool isCanvasStyleType()
    {
        return m_type == CanvasFillStrokeSourceType::CanvasStyle;
    }

    Unit::Color getColorValue()
    {
        STARFISH_ASSERT(isColorType());
        return m_value.m_color;
    }

    CanvasStyle getCanvasStyleValue()
    {
        STARFISH_ASSERT(isCanvasStyleType());
        return m_value.m_canvasStyle;
    }

    bool isCanvasAvailableSource();

private:
    CanvasFillStrokeSourceType m_type;
    CanvasFillStrokeSourceValue m_value;
};
}
#endif
