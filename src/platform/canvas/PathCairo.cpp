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
#if defined(PORT_CANVAS_BACKEND_CAIRO)

#include "Starfish.h"
#include "core/modules/canvas/Path.h"
#include <cairo.h>
#include "platform/canvas/PathCairo.h"

namespace Starfish {

Path* Path::create()
{
    return new PathCairo();
}

PathCairo::PathCairo()
    : m_cairoContext(nullptr)
    , m_dumyCairoSurface(nullptr)
{
    init();
    GC_REGISTER_FINALIZER_NO_ORDER(this,
                                   [](void* obj, void* cd) {
                                       PathCairo* p = (PathCairo*)obj;
                                       p->finalize();
                                   },
                                   NULL, NULL, NULL);
}

PathCairo::~PathCairo()
{
}

void PathCairo::finalize()
{
    cairo_destroy(m_cairoContext);
    cairo_surface_destroy(m_dumyCairoSurface);
}

void PathCairo::init()
{
    m_dumyCairoSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    m_cairoContext = cairo_create(m_dumyCairoSurface);
}

void PathCairo::clear()
{
    cairo_identity_matrix(m_cairoContext);
    cairo_new_path(m_cairoContext);
}

bool PathCairo::isEmpty()
{
    return !cairo_has_current_point(m_cairoContext);
}

void PathCairo::closePath()
{
    cairo_close_path(m_cairoContext);
}

void PathCairo::moveTo(double x, double y)
{
    m_needNewSubPath = false;
    cairo_move_to(m_cairoContext, x, y);
}

void PathCairo::lineTo(double x, double y)
{
    cairo_line_to(m_cairoContext, x, y);
}

void PathCairo::quadraticCurveTo(double cpx, double cpy, double x, double y)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void PathCairo::bezierCurveTo(double cp1x, double cp1y, double cp2x,
                              double cp2y, double x, double y)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void PathCairo::arcTo(double x1, double y1, double x2, double y2, double radius)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void PathCairo::rect(double x, double y, double w, double h)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void PathCairo::arc(double x, double y, double radius, double startAngle,
                    double endAngle, bool anticlockwise /*=false*/)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void PathCairo::ellipse(double x, double y, double radiusX, double radiusY,
                        double rotation, double startAngle, double endAngle,
                        bool anticlockwise /*=false*/)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}
}
#endif
