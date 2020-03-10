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
#ifdef STARFISH_ENABLE_CANVAS

#include <SkMatrix.h>

#include "StarfishConfig.h"
#include "core/dom/canvas/CanvasPath.h"
#include "binding/Path2DOrDOMStringUnion.h"
#include "core/modules/canvas/Path.h"
#include "core/dom/canvas/Path2D.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {

Path2D::Path2D(ExecutionContext* executionContext)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_canvasPath(nullptr)
{
    m_canvasPath = new CanvasPath(executionContext);
}

Path2D::Path2D(ExecutionContext* executionContext, Path2DOrDOMString& path)
    : Path2D(executionContext)
{
    initFromPath2DOrDOMString(path);
}

void Path2D::initFromPath2DOrDOMString(Path2DOrDOMString& path)
{
    if (path.isPath2DValue()) {
        m_canvasPath->path()->copy(path.getPath2DValue()->m_canvasPath->path());
    } else if (path.isDOMStringValue()) {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
}

ScriptBindingInstance* Path2D::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

void Path2D::closePath()
{
    m_canvasPath->closePath();
}

void Path2D::moveTo(float x, float y)
{
    m_canvasPath->moveTo(x, y);
}

void Path2D::lineTo(float x, float y)
{
    m_canvasPath->lineTo(x, y);
}

void Path2D::quadraticCurveTo(float cpx, float cpy, float x, float y)
{
    m_canvasPath->quadraticCurveTo(cpx, cpy, x, y);
}

void Path2D::bezierCurveTo(float cp1x, float cp1y, float cp2x, float cp2y,
                           float x, float y)
{
    m_canvasPath->bezierCurveTo(cp1x, cp1y, cp2x, cp2y, x, y);
}

void Path2D::arcTo(float x1, float y1, float x2, float y2, float radius)
{
    m_canvasPath->arcTo(x1, y1, x2, y2, radius);
}

void Path2D::rect(float x, float y, float w, float h)
{
    m_canvasPath->rect(x, y, w, h);
}

void Path2D::arc(float x, float y, float radius, float startAngle,
                 float endAngle, bool anticlockwise /*=false*/)
{
    m_canvasPath->arc(x, y, radius, startAngle, endAngle, anticlockwise);
}

void Path2D::ellipse(float x, float y, float radiusX, float radiusY,
                     float rotation, float startAngle, float endAngle,
                     bool anticlockwise /*=false*/)
{
    m_canvasPath->ellipse(x, y, radiusX, radiusY, rotation, startAngle,
                          endAngle, anticlockwise);
}
}
#endif
