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
#include "core/dom/canvas/CanvasFillRule.h"

#include "core/style/Style.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Path.h"

#include <cairo.h>
#include "platform/canvas/CanvasCairoUtils.h"
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
    m_matrix.reset();
    m_dumyCairoSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    m_cairoContext = cairo_create(m_dumyCairoSurface);
}

void PathCairo::clear()
{
    m_matrix.reset();
    cairo_identity_matrix(m_cairoContext);
    cairo_new_path(m_cairoContext);
}

bool PathCairo::isEmpty()
{
    return !cairo_has_current_point(m_cairoContext);
}

void PathCairo::currentPoint(float& x, float& y)
{
    double xx, yy;
    cairo_get_current_point(m_cairoContext, &xx, &yy);
    x = static_cast<float>(xx);
    y = static_cast<float>(yy);
}

void PathCairo::copy(Path* src)
{
    clear();
    auto p = cairo_copy_path(((PathCairo*)src)->context());
    cairo_append_path(m_cairoContext, p);
    cairo_path_destroy(p);
}

void PathCairo::append(Path* path)
{
    auto p = cairo_copy_path(((PathCairo*)path)->context());
    cairo_append_path(m_cairoContext, p);
    cairo_path_destroy(p);
}

bool PathCairo::isPointInPath(float x, float y, CanvasFillRule fillRule)
{
    cairo_fill_rule_t backup = cairo_get_fill_rule(m_cairoContext);
    cairo_fill_rule_t rule;

    if (fillRule == CanvasFillRule::EvenOdd) {
        rule = cairo_fill_rule_t::CAIRO_FILL_RULE_EVEN_ODD;
    } else {
        rule = cairo_fill_rule_t::CAIRO_FILL_RULE_WINDING;
    }

    cairo_set_fill_rule(m_cairoContext, rule);
    bool ret = cairo_in_fill(m_cairoContext, x, y);
    cairo_set_fill_rule(m_cairoContext, backup);

    return ret;
}

bool PathCairo::isPointInStroke(float x, float y)
{
    return cairo_in_stroke(m_cairoContext, x, y);
}

void PathCairo::applyPathDrawingStyles(Canvas* canvas)
{
    cairo_set_line_width(m_cairoContext, canvas->lineWidth());
    cairo_set_line_cap(
        m_cairoContext,
        CanvasCairoUtils::cavansLineCapToCairoLineCap(canvas->lineCap()));
    cairo_set_line_join(
        m_cairoContext,
        CanvasCairoUtils::canvasLineJoinToCairoLineJoin(canvas->lineJoine()));
    cairo_set_miter_limit(m_cairoContext, canvas->miterLimit());
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
    m_needNewSubPath = false;
    cairo_line_to(m_cairoContext, x, y);
}

void PathCairo::translate(float x, float y)
{
    SkMatrix matrix = SkMatrix::I();
    matrix.postTranslate(-x, -y);
    postMatrix(matrix);
}

void PathCairo::quadraticCurveTo(float cpx, float cpy, float x, float y)
{
    m_needNewSubPath = false;

    double x0, y0;
    cairo_get_current_point(m_cairoContext, &x0, &y0);
    cairo_curve_to(m_cairoContext, 2.0 / 3.0 * cpx + 1.0 / 3.0 * x0,
                   2.0 / 3.0 * cpy + 1.0 / 3.0 * y0,
                   2.0 / 3.0 * cpx + 1.0 / 3.0 * x,
                   2.0 / 3.0 * cpy + 1.0 / 3.0 * y, x, y);
}

void PathCairo::bezierCurveTo(float cp1x, float cp1y, float cp2x, float cp2y,
                              float x, float y)
{
    m_needNewSubPath = false;
    cairo_curve_to(m_cairoContext, cp1x, cp1y, cp2x, cp2y, x, y);
}

void PathCairo::arcTo(float x1, float y1, float x2, float y2, float radius)
{
    m_needNewSubPath = false;

    double x0, y0;
    cairo_get_current_point(m_cairoContext, &x0, &y0);

    // import from WebKit project
    // WebKit/Source/WebCore/platform/graphics/cairo/PathCairo.cpp(4c17da14daaa3afc2a968eeb227bba5d7a58ba32)
    float p1p0_x = x0 - x1;
    float p1p0_y = y0 - y1;
    float p1p2_x = x2 - x1;
    float p1p2_y = y2 - y1;
    float p1p0_length = sqrtf(p1p0_x * p1p0_x + p1p0_y * p1p0_y);
    float p1p2_length = sqrtf(p1p2_x * p1p2_x + p1p2_y * p1p2_y);

    double cos_phi =
        (p1p0_x * p1p2_x + p1p0_y * p1p2_y) / (p1p0_length * p1p2_length);
    // all points on a line logic
    if (cos_phi == -1) {
        cairo_line_to(m_cairoContext, x1, y1);
        return;
    }
    if (cos_phi == 1) {
        // add infinite far away point
        unsigned int max_length = 65535;
        double factor_max = max_length / p1p0_length;
        float ex = x0 + factor_max * p1p0_x;
        float ey = y0 + factor_max * p1p0_y;
        cairo_line_to(m_cairoContext, ex, ey);
        return;
    }

    float tangent = radius / tan(acos(cos_phi) / 2);
    float factor_p1p0 = tangent / p1p0_length;
    float t_p1p0_x = x1 + factor_p1p0 * p1p0_x;
    float t_p1p0_y = y1 + factor_p1p0 * p1p0_y;

    float orth_p1p0_x = p1p0_y;
    float orth_p1p0_y = -p1p0_x;

    float orth_p1p0_length =
        sqrt(orth_p1p0_x * orth_p1p0_x + orth_p1p0_y * orth_p1p0_y);
    float factor_ra = radius / orth_p1p0_length;

    // angle between orth_p1p0 and p1p2 to get the right vector orthographic to
    // p1p0
    double cos_alpha = (orth_p1p0_x * p1p2_x + orth_p1p0_y * p1p2_y) /
                       (orth_p1p0_length * p1p2_length);
    if (cos_alpha < 0.f) {
        orth_p1p0_x = -orth_p1p0_x;
        orth_p1p0_y = -orth_p1p0_y;
    }
    float x = t_p1p0_x + factor_ra * orth_p1p0_x;
    float y = t_p1p0_y + factor_ra * orth_p1p0_y;

    // calculate angles for addArc
    orth_p1p0_x = -orth_p1p0_x;
    orth_p1p0_y = -orth_p1p0_y;
    float sa = acos(orth_p1p0_x / orth_p1p0_length);

    if (orth_p1p0_y < 0.f) {
        sa = 2 * M_PI - sa;
    }

    // anticlockwise logic
    bool anticlockwise = false;

    float factor_p1p2 = tangent / p1p2_length;

    float t_p1p2_x = x1 + factor_p1p2 * p1p2_x;
    float t_p1p2_y = y1 + factor_p1p2 * p1p2_y;

    float orth_p1p2_x = t_p1p2_x - x;
    float orth_p1p2_y = t_p1p2_y - y;

    float orth_p1p2_length =
        sqrtf(orth_p1p2_x * orth_p1p2_x + orth_p1p2_y * orth_p1p2_y);
    float ea = acos(orth_p1p2_x / orth_p1p2_length);
    if (orth_p1p2_y < 0) {
        ea = 2 * M_PI - ea;
    }
    if ((sa > ea) && ((sa - ea) < M_PI)) {
        anticlockwise = true;
    }
    if ((sa < ea) && ((ea - sa) > M_PI)) {
        anticlockwise = true;
    }

    cairo_line_to(m_cairoContext, t_p1p0_x, t_p1p0_y);

    arc(x, y, radius, sa, ea, anticlockwise);
}

void PathCairo::rect(float x, float y, float w, float h)
{
    m_needNewSubPath = false;
    cairo_rectangle(m_cairoContext, x, y, w, h);
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
    m_needNewSubPath = false;

    cairo_save(m_cairoContext);
    cairo_translate(m_cairoContext, x, y);
    cairo_rotate(m_cairoContext, rotation);
    cairo_scale(m_cairoContext, radiusX, radiusY);

    if (anticlockwise) {
        cairo_arc_negative(m_cairoContext, 0, 0, 1, startAngle, endAngle);
    } else {
        cairo_arc(m_cairoContext, 0, 0, 1, startAngle, endAngle);
    }
    cairo_restore(m_cairoContext);
}

void PathCairo::postMatrix(const SkMatrix& matrix)
{
    Path::postMatrix(matrix);

    cairo_matrix_t result_matrix;
    cairo_matrix_init_identity(&result_matrix);

    cairo_matrix_t a_matrix;
    cairo_matrix_t b_matrix;
    cairo_get_matrix(m_cairoContext, &a_matrix);
    cairo_matrix_init(&b_matrix, matrix.getScaleX(), matrix.getSkewY(),
                      matrix.getSkewX(), matrix.getScaleY(),
                      matrix.getTranslateX(), matrix.getTranslateY());
    cairo_matrix_multiply(&result_matrix, &b_matrix, &a_matrix);
    cairo_set_matrix(m_cairoContext, &result_matrix);
}

void PathCairo::setCTM(const SkMatrix& matrix)
{
    m_matrix = matrix;

    cairo_matrix_t result_matrix;
    cairo_matrix_t a_matrix;
    cairo_matrix_t b_matrix;

    cairo_matrix_init_identity(&result_matrix);
    cairo_matrix_init_identity(&a_matrix);
    cairo_matrix_init(&b_matrix, matrix.getScaleX(), matrix.getSkewY(),
                      matrix.getSkewX(), matrix.getScaleY(),
                      matrix.getTranslateX(), matrix.getTranslateY());
    cairo_matrix_multiply(&result_matrix, &a_matrix, &b_matrix);
    cairo_set_matrix(m_cairoContext, &result_matrix);
}

Unit::Rect PathCairo::boundingRect(bool isFill)
{
    double x0 = 0;
    double x1 = 0;
    double y0 = 0;
    double y1 = 0;
    if (isFill) {
        cairo_fill_extents(m_cairoContext, &x0, &y0, &x1, &y1);
    } else {
        cairo_stroke_extents(m_cairoContext, &x0, &y0, &x1, &y1);
    }
    return Unit::Rect(x0, y0, x1 - x0, y1 - y0);
}
}
#endif
