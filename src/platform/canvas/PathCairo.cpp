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

#include "core/style/StrokeLineCap.h"
#include "core/style/StrokeLineJoin.h"

namespace Starfish {

// A disclaim proc is handed *every* unmarked slot of the block being swept --
// it "must also tolerate being called with object from free list" (GCutil
// include/private/gc_priv.h), and GC_disclaim_and_reclaim() re-visits the same
// slot on every later cycle. So this runs over slots that were never
// constructed, slots reclaimed in an earlier cycle, and (debug collector) slots
// poisoned by an explicit free.
//
// The liveness sentinel therefore CANNOT be word 0 (the vptr): the instant
// this proc returns 0, GC_reclaim_generic() (GCutil reclaim.c) does
// `obj_link(p) = list;` -- overwrites word 0 with the free-list link -- and
// only *then* clears the rest of the object via GC_clear_block(), which
// explicitly skips word 0 ("skip link field"). So a sentinel written to word 0
// survives only until that same statement runs, and the *next* sweep over an
// object still sitting unreclaimed on the free list sees the free-list link
// (an ordinary, usually non-zero pointer), not our zero -- the "already
// disposed" check misses, and the code below would call clearNativeResources()
// through a dangling/garbage `this`. m_gcDisclaimAlive lives elsewhere in the
// object, which GC_clear_block *does* zero on every pass (first disposal or a
// later re-visit alike), so it reads back 0 reliably from then on.
int GC_CALLBACK PathCairo::disclaimProc(void* obj)
{
#ifdef GC_DEBUG
    // The proc is passed the allocation base; under the debug collector the
    // object itself starts one debug header later.
    obj = GC_USR_PTR_FROM_BASE(obj);
#endif
    PathCairo* path = reinterpret_cast<PathCairo*>(obj);
    if (path->m_gcDisclaimAlive == 0) {
        return 0;
    }
#ifdef GC_DEBUG
    // GC_FREED_MEM_MARKER (GCutil include/private/dbg_mlc.h): whoever freed the
    // object explicitly owned its disposal, and treating the poison as a live
    // object would hand cairo_destroy() a garbage handle.
    const size_t gcFreedMemMarker = sizeof(size_t) == 8
                                        ? (size_t)0xEFBEADDEdeadbeefULL
                                        : (size_t)0xdeadbeef;
    if (path->m_gcDisclaimAlive == gcFreedMemMarker) {
        return 0;
    }
#endif
    path->clearNativeResources();
    return 0; // 0 = OK to reclaim (non-zero would resurrect the object)
}

int PathCairo::gcKind()
{
    // GCutil is built with GC_THREAD_ISOLATE, so a kind lives in the
    // registering thread's own table -- fine to cache in a plain static here,
    // since paths are only ever allocated from the engine main thread, which
    // never changes.
    static int gcKind = 0;
    if (gcKind != 0) {
        return gcKind;
    }

    // The bitmap is applied from the allocation base, so under the debug
    // collector every offset has to be shifted past the debug header --
    // otherwise the traced word lands in that header and the dasharray buffer
    // is never traced at all. (A custom mark proc would not need this: GCutil's
    // GC_mark_and_push_custom() does the base-to-object conversion itself. A
    // bitmap descriptor has no such hook.) One spare bitmap word covers the
    // shift.
#ifdef GC_DEBUG
    const size_t headerWords = GC_get_debug_header_size() / sizeof(GC_word);
#else
    const size_t headerWords = 0;
#endif
    GC_word desc[GC_BITMAP_SIZE(PathCairo) + 1] = { 0 };
    GC_set_bit(desc, headerWords +
                         GC_WORD_OFFSET(
                             PathCairo,
                             m_needsComputeStrokeBoundingRect.strokeDasharray));
    GC_descr descr =
        GC_make_descriptor(desc, headerWords + GC_WORD_LEN(PathCairo));
    // _enumerable (ok_eager_sweep) matches the other custom kind in this tree
    // (BufferedNativeImageData) and gets the block swept before marking, so the
    // cairo handles of a dead path are released in the cycle that killed it.
    gcKind =
        (int)GC_new_kind_enumerable(GC_new_free_list(), descr, FALSE, TRUE);
    // mark_from_all stays FALSE: the disclaim proc touches only the two native
    // cairo handles, never a GC pointer, so there is nothing to keep alive for
    // it -- and TRUE would resurrect any self-referencing path forever (see the
    // note on GC_finalized_kind in GCutil/fnlz_mlc.c).
    GC_register_disclaim_proc(gcKind, disclaimProc, FALSE);
    return gcKind;
}

void* PathCairo::operator new(size_t size)
{
    // PathCairo is mostly floats (the SkMatrix in Path, the bounding rects,
    // the stroke style) plus two cairo handles that are external resources
    // released by the disclaim proc above. The only GC pointer inside is the
    // strokeDasharray buffer (GCAtomicVector<double>; its first word is the
    // buffer pointer). Give this class its own kind with a bitmap descriptor
    // that traces just that word, and a disclaim proc for cleanup -- same
    // "run this on death" behavior a GC_finalized_malloc/
    // GC_REGISTER_FINALIZER_NO_ORDER give, but without either the
    // whole-object conservative scan (which let float bit patterns that
    // happen to match heap addresses pin unrelated objects -- e.g. on
    // 32-bit targets, where the heap covers a large share of the address
    // space, that kept whole discarded documents alive) or the per-object
    // finalizer-hash-table registration cost.
    STARFISH_ASSERT(size == sizeof(PathCairo));
    return GC_GENERIC_MALLOC(size, PathCairo::gcKind());
}

void PathCairo::clearNativeResources()
{
    if (m_cairoContext) {
        cairo_destroy(m_cairoContext);
        m_cairoContext = nullptr;
    }
    if (m_dumyCairoSurface) {
        cairo_surface_destroy(m_dumyCairoSurface);
        m_dumyCairoSurface = nullptr;
    }
    // Marks the disclaim-proc sentinel; see disclaimProc above.
    m_gcDisclaimAlive = 0;
}

Path* Path::create()
{
    return new PathCairo();
}

PathCairo::PathCairo()
    : m_gcDisclaimAlive(1)
    , m_cairoContext(nullptr)
    , m_dumyCairoSurface(nullptr)
{
    m_needsComputeStrokeBoundingRect.strokeMiterLimit = 0;
    m_needsComputeStrokeBoundingRect.strokeLineCap = StrokeLineCap::Butt;
    m_needsComputeStrokeBoundingRect.strokeLineJoin = StrokeLineJoin::Miter;
    notifyBoundingRectDirty();
    init();
}

PathCairo::~PathCairo()
{
}

void PathCairo::finalize()
{
    clearNativeResources();
}

void PathCairo::init()
{
    m_matrix.reset();
    m_dumyCairoSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    m_cairoContext = cairo_create(m_dumyCairoSurface);
    notifyBoundingRectDirty();
}

void PathCairo::clear()
{
    m_matrix.reset();
    cairo_identity_matrix(m_cairoContext);
    cairo_new_path(m_cairoContext);
    notifyBoundingRectDirty();
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
    notifyBoundingRectDirty();
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

bool PathCairo::isPointInStroke(const StrokeStyle& style, float x, float y)
{
    applyStrokeStyle(style);
    return cairo_in_stroke(m_cairoContext, x, y);
}

void PathCairo::applyStrokeStyle(const StrokeStyle& style)
{
    // line-width == stroke-width * 2
    cairo_set_line_width(m_cairoContext, style.strokeWidth * 2);
    cairo_set_line_cap(
        m_cairoContext,
        CanvasCairoUtils::canvasLineCapToCairoLineCap(style.strokeLineCap));
    cairo_set_line_join(
        m_cairoContext,
        CanvasCairoUtils::canvasLineJoinToCairoLineJoin(style.strokeLineJoin));
    cairo_set_miter_limit(m_cairoContext, style.strokeMiterLimit);
}

void PathCairo::closePath()
{
    cairo_close_path(m_cairoContext);
}

void PathCairo::moveTo(float x, float y)
{
    notifyBoundingRectDirty();
    m_needNewSubPath = false;
    cairo_move_to(m_cairoContext, x, y);
    updateBoundingRect();
}

void PathCairo::lineTo(float x, float y)
{
    notifyBoundingRectDirty();
    m_needNewSubPath = false;
    cairo_line_to(m_cairoContext, x, y);
    updateBoundingRect();
}

void PathCairo::translate(float x, float y)
{
    SkMatrix matrix = SkMatrix::I();
    matrix.postTranslate(-x, -y);
    postMatrix(matrix);
}

void PathCairo::quadraticCurveTo(float cpx, float cpy, float x, float y)
{
    notifyBoundingRectDirty();
    m_needNewSubPath = false;

    double x0, y0;
    cairo_get_current_point(m_cairoContext, &x0, &y0);
    cairo_curve_to(m_cairoContext, 2.0 / 3.0 * cpx + 1.0 / 3.0 * x0,
                   2.0 / 3.0 * cpy + 1.0 / 3.0 * y0,
                   2.0 / 3.0 * cpx + 1.0 / 3.0 * x,
                   2.0 / 3.0 * cpy + 1.0 / 3.0 * y, x, y);
    updateBoundingRect();
}

void PathCairo::bezierCurveTo(float cp1x, float cp1y, float cp2x, float cp2y,
                              float x, float y)
{
    notifyBoundingRectDirty();
    m_needNewSubPath = false;
    cairo_curve_to(m_cairoContext, cp1x, cp1y, cp2x, cp2y, x, y);
    updateBoundingRect();
}

void PathCairo::arcTo(float x1, float y1, float x2, float y2, float radius)
{
    notifyBoundingRectDirty();
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
        updateBoundingRect();
        return;
    }
    if (cos_phi == 1) {
        // add infinite far away point
        unsigned int max_length = 65535;
        double factor_max = max_length / p1p0_length;
        float ex = x0 + factor_max * p1p0_x;
        float ey = y0 + factor_max * p1p0_y;
        cairo_line_to(m_cairoContext, ex, ey);
        updateBoundingRect();
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
    updateBoundingRect();

    arc(x, y, radius, sa, ea, anticlockwise);
}

void PathCairo::rect(float x, float y, float w, float h)
{
    notifyBoundingRectDirty();
    m_needNewSubPath = false;
    cairo_rectangle(m_cairoContext, x, y, w, h);
    updateBoundingRect();
}

// the arc functions belows are import from WebKit project
// WebKit/Source/WebCore/html/canvas/CanvasPath.cpp::addArc(2a7a4c9880d8debfd1aa9ae71294c7df06dbbce8)
void PathCairo::arc(float x, float y, float radius, float startAngle,
                    float endAngle, bool anticlockwise /*=false*/)
{
    notifyBoundingRectDirty();
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
    updateBoundingRect();
}

void PathCairo::ellipse(float x, float y, float radiusX, float radiusY,
                        float rotation, float startAngle, float endAngle,
                        bool anticlockwise /*=false*/)
{
    notifyBoundingRectDirty();
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
    updateBoundingRect();
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

Unit::Rect PathCairo::fillBoundingRect()
{
    if (!m_needsComputeFillBoundingRect) {
        return m_computedFillBoundingRect;
    }
    double x0 = 0;
    double x1 = 0;
    double y0 = 0;
    double y1 = 0;

    cairo_fill_extents(m_cairoContext, &x0, &y0, &x1, &y1);
    Unit::Rect result(x0, y0, x1 - x0, y1 - y0);
    m_needsComputeFillBoundingRect = false;
    m_computedFillBoundingRect = result;
    return result;
}

Unit::Rect PathCairo::strokeBoundingRect(const StrokeStyle& style)
{
    if (m_needsComputeStrokeBoundingRect == style) {
        return m_computedStrokeBoundingRect;
    }

    double x0 = 0;
    double x1 = 0;
    double y0 = 0;
    double y1 = 0;

    applyStrokeStyle(style);
    m_needsComputeStrokeBoundingRect = style;

    if (style.strokeWidth) {
        cairo_stroke_extents(m_cairoContext, &x0, &y0, &x1, &y1);
    } else {
        cairo_fill_extents(m_cairoContext, &x0, &y0, &x1, &y1);
    }

    Unit::Rect result(x0, y0, x1 - x0, y1 - y0);
    m_computedStrokeBoundingRect = result;
    return result;
}

void PathCairo::updateBoundingRect()
{
    double x_min = m_boundingRect.x();
    double y_min = m_boundingRect.y();
    double x_max = m_boundingRect.x() + m_boundingRect.width();
    double y_max = m_boundingRect.y() + m_boundingRect.height();

    double x, y;
    cairo_get_current_point(m_cairoContext, &x, &y);

    x_min = std::min(x_min, x);
    y_min = std::min(y_min, y);
    x_max = std::max(x_max, x);
    y_max = std::max(y_max, y);

    m_boundingRect.setX(x_min);
    m_boundingRect.setY(y_min);
    m_boundingRect.setWidth(x_max - x_min);
    m_boundingRect.setHeight(y_max - y_min);
}

Unit::Rect PathCairo::boundingRect()
{
    double x0 = 0;
    double x1 = 0;
    double y0 = 0;
    double y1 = 0;
    cairo_path_extents(m_cairoContext, &x0, &y0, &x1, &y1);

    double x_min = m_boundingRect.x();
    double y_min = m_boundingRect.y();
    double x_max = m_boundingRect.x() + m_boundingRect.width();
    double y_max = m_boundingRect.y() + m_boundingRect.height();

    x_min = std::min(x_min, x0);
    y_min = std::min(y_min, y0);
    x_max = std::max(x_max, x1);
    y_max = std::max(y_max, y1);

    return Unit::Rect(x_min, y_min, x_max - x_min, y_max - y_min);
}

GCAtomicVector<Unit::FloatPoint> PathCairo::pointList()
{
    auto path = cairo_copy_path_flat(m_cairoContext);
    GCAtomicVector<Unit::FloatPoint> points;

    cairo_path_data_t* data;
    int i;
    double lastX = 0, lastY = 0;
    double lastMoveX = 0, lastMoveY = 0;
    for (i = 0; i < path->num_data; i += path->data[i].header.length) {
        data = &path->data[i];
        switch (data->header.type) {
        case CAIRO_PATH_MOVE_TO:
            lastMoveX = data[1].point.x;
            lastMoveY = data[1].point.y;
            points.push_back(
                Unit::FloatPoint(data[1].point.x, data[1].point.y));
            break;
        case CAIRO_PATH_LINE_TO:
            points.push_back(
                Unit::FloatPoint(data[1].point.x, data[1].point.y));
            break;
        case CAIRO_PATH_CLOSE_PATH:
            points.push_back(Unit::FloatPoint(lastMoveX, lastMoveY));
            break;
        default:
            STARFISH_ASSERT_NOT_REACHED();
            break;
        }
    }

    cairo_path_destroy(path);

    return points;
}

} // namespace Starfish
#endif
