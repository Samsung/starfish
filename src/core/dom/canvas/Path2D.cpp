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

Path2D::Path2D(CanvasRenderingContext* context)
    : ScriptWrappable(this, context->executionContext())
    , m_canvasRenderingContext(context)
{
}

ScriptBindingInstance* Path2D::scriptBindingInstance()
{
    return m_canvasRenderingContext->scriptBindingInstance();
}

void Path2D::closePath()
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void Path2D::moveTo(double x, double y)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void Path2D::lineTo(double x, double y)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}
void Path2D::bezierCurveTo(double x1, double y1, double x2, double y2,
                           double x3, double y3)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void Path2D::rect(double x, double y, double w, double h)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void Path2D::arc(double x, double y, double radius, double startAngle,
                 double endAngle, bool anticlockwise)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void Path2D::ellipse(double x, double y, double radiusX, double radiusY,
                     double rotation, double startAngle, double endAngle,
                     bool anticlockwise)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}
}
#endif
