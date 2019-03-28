/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software{} you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation{} either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY{} without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library{} if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#ifdef STARFISH_ENABLE_CANVAS

#include "StarfishConfig.h"
#include "core/modules/canvas/Path.h"
#include "core/dom/canvas/CanvasPath.h"

namespace Starfish {

CanvasPath::CanvasPath()
    : m_path(nullptr)
{
    init();
}

void CanvasPath::init()
{
    m_path = Path::create();
}

void CanvasPath::closePath()
{
    if (m_path->isEmpty()) {
        return;
    }
    m_path->closePath();
}

void CanvasPath::moveTo(double x, double y)
{
    if (std::isinf(x) || std::isinf(y) || std::isnan(x) || std::isnan(y)) {
        return;
    }
    m_path->moveTo(x, y);
}

void CanvasPath::lineTo(double x, double y)
{
    if (std::isinf(x) || std::isinf(y) || std::isnan(x) || std::isnan(y)) {
        return;
    }

    m_path->ensureSubPath(x, y);
    m_path->lineTo(x, y);
}

void CanvasPath::quadraticCurveTo(double cpx, double cpy, double x, double y)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void CanvasPath::bezierCurveTo(double cp1x, double cp1y, double cp2x,
                               double cp2y, double x, double y)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void CanvasPath::arcTo(double x1, double y1, double x2, double y2,
                       double radius)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void CanvasPath::rect(double x, double y, double w, double h)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void CanvasPath::arc(double x, double y, double radius, double startAngle,
                     double endAngle, bool anticlockwise /*=false*/)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void CanvasPath::ellipse(double x, double y, double radiusX, double radiusY,
                         double rotation, double startAngle, double endAngle,
                         bool anticlockwise /*=false*/)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}
}
#endif
