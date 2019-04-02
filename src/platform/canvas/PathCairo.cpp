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

/*
 * Copyright (C) 2003, 2006, 2009, 2016 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Rob Buis <buis@kde.org>
 * Copyright (C) 2007-2008 Torch Mobile, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

void PathCairo::moveTo(float x, float y)
{
    m_needNewSubPath = false;
    cairo_move_to(m_cairoContext, x, y);
}

void PathCairo::lineTo(float x, float y)
{
    cairo_line_to(m_cairoContext, x, y);
}

void PathCairo::quadraticCurveTo(float cpx, float cpy, float x, float y)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void PathCairo::bezierCurveTo(float cp1x, float cp1y, float cp2x, float cp2y,
                              float x, float y)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void PathCairo::arcTo(float x1, float y1, float x2, float y2, float radius)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void PathCairo::rect(float x, float y, float w, float h)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

// the arc functions belows are import from WebKit project
// WebKit/Source/WebCore/html/canvas/CanvasPath.cpp::addArc(2a7a4c9880d8debfd1aa9ae71294c7df06dbbce8)
void PathCairo::arc(float x, float y, float radius, float startAngle,
                    float endAngle, bool anticlockwise /*=false*/)
{
    m_needNewSubPath = false;

    float sweep = endAngle - startAngle;
    float twoPI = 2 * UnitHelper::PI;

    if ((sweep <= -twoPI || sweep >= twoPI) &&
        ((anticlockwise && (endAngle < startAngle)) ||
         (!anticlockwise && (startAngle < endAngle)))) {
        if (anticlockwise) {
            cairo_arc_negative(m_cairoContext, x, y, radius, startAngle,
                               startAngle - twoPI);
        } else {
            cairo_arc(m_cairoContext, x, y, radius, startAngle,
                      startAngle + twoPI);
        }
        cairo_new_sub_path(m_cairoContext);
        cairo_arc(m_cairoContext, x, y, radius, endAngle, endAngle);
    } else {
        if (anticlockwise) {
            cairo_arc_negative(m_cairoContext, x, y, radius, startAngle,
                               endAngle);
        } else {
            cairo_arc(m_cairoContext, x, y, radius, startAngle, endAngle);
        }
    }
}

void PathCairo::ellipse(float x, float y, float radiusX, float radiusY,
                        float rotation, float startAngle, float endAngle,
                        bool anticlockwise /*=false*/)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}
}
#endif
