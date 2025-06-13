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
#if defined(PORT_CANVAS_BACKEND_MOCK)
#include <SkMatrix.h>
#include "core/modules/canvas/Path.h"
#include "platform/canvas/PathMock.h"

namespace Starfish {

void PathMock::init()
{
}

void PathMock::clear()
{
}

bool PathMock::isEmpty()
{
    return true;
}

void PathMock::currentPoint(float& x, float& y)
{
}

void PathMock::copy(Path* src)
{
}

bool PathMock::isPointInPath(float x, float y, CanvasFillRule fillRule)
{
    return false;
}

bool PathMock::isPointInStroke(const StrokeStyle& style, float x, float y)
{
    return false;
}

void PathMock::closePath()
{
}

void PathMock::moveTo(float x, float y)
{
}

void PathMock::lineTo(float x, float y)
{
}

void PathMock::quadraticCurveTo(float cpx, float cpy, float x, float y)
{
}

void PathMock::bezierCurveTo(float cp1x, float cp1y, float cp2x, float cp2y,
                             float x, float y)
{
}

void PathMock::arcTo(float x1, float y1, float x2, float y2, float radius)
{
}

void PathMock::rect(float x, float y, float w, float h)
{
}

void PathMock::arc(float x, float y, float radius, float startAngle,
                   float endAngle, bool anticlockwise)
{
}

void PathMock::ellipse(float x, float y, float radiusX, float radiusY,
                       float rotation, float startAngle, float endAngle,
                       bool anticlockwise)
{
}

void PathMock::postMatrix(const SkMatrix& matrix)
{
}

void PathMock::setCTM(const SkMatrix& matrix)
{
}

Unit::Rect PathMock::strokeBoundingRect(const StrokeStyle& style)
{
    return Unit::Rect();
}

Unit::Rect PathMock::boundingRect()
{
    return Unit::Rect();
}

Unit::Rect PathMock::fillBoundingRect()
{
    return Unit::Rect();
}

void PathMock::translate(float x, float y)
{
}

void PathMock::append(Path* path)
{
}

Path* Path::create()
{
    return new PathMock();
}
} // namespace Starfish

#endif
