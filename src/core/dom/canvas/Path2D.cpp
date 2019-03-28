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
#ifdef STARFISH_ENABLE_CANVAS

#include "StarfishConfig.h"
#include "core/dom/canvas/CanvasRenderingContext.h"
#include "core/dom/canvas/Path2D.h"

namespace Starfish {

Path2D::Path2D(ExecutionContext* executionContext)
    : ScriptWrappable(this, executionContext)
{
}

void Path2D::closePath()
{
    m_canvasPath->closePath();
}

void Path2D::moveTo(double x, double y)
{
    m_canvasPath->moveTo(x, y);
}

void Path2D::lineTo(double x, double y)
{
    m_canvasPath->lineTo(x, y);
}

void Path2D::quadraticCurveTo(double cpx, double cpy, double x, double y)
{
    m_canvasPath->quadraticCurveTo(cpx, cpy, x, y);
}

void Path2D::bezierCurveTo(double cp1x, double cp1y, double cp2x, double cp2y,
                           double x, double y)
{
    m_canvasPath->bezierCurveTo(cp1x, cp1y, cp2x, cp2y, x, y);
}

void Path2D::arcTo(double x1, double y1, double x2, double y2, double radius)
{
    m_canvasPath->arcTo(x1, y1, x2, y2, radius);
}

void Path2D::rect(double x, double y, double w, double h)
{
    m_canvasPath->rect(x, y, w, h);
}

void Path2D::arc(double x, double y, double radius, double startAngle,
                 double endAngle, bool anticlockwise /*=false*/)
{
    m_canvasPath->arc(x, y, radius, startAngle, endAngle, anticlockwise);
}

void Path2D::ellipse(double x, double y, double radiusX, double radiusY,
                     double rotation, double startAngle, double endAngle,
                     bool anticlockwise /*=false*/)
{
    m_canvasPath->ellipse(x, y, radiusX, radiusY, rotation, startAngle,
                          endAngle, anticlockwise);
}
}
#endif
