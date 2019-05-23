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

/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012 Apple Inc.
 * All rights reserved.
 * Copyright (C) 2008, 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2008 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2010 Torch Mobile (Beijing) Co. Ltd. All rights reserved.
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 * Copyright (C) 2012, 2013 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifdef STARFISH_ENABLE_CANVAS

#include <SkMatrix.h>

#include "StarfishConfig.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/canvas/Path.h"
#include "core/dom/canvas/CanvasPath.h"
#include "core/dom/DOMException.h"

namespace Starfish {

// the normalizeAngles functions belows are import from WebKit project
// WebKit/Source/WebCore/html/canvas/CanvasPath.cpp(13999e71933c7d1523acea94e7f523205247d8d9)
static void normalizeAngles(float& startAngle, float& endAngle,
                            bool anticlockwise)
{
    float newStartAngle = startAngle;
    if (newStartAngle < 0) {
        newStartAngle =
            (2 * UnitHelper::PI) + fmodf(newStartAngle, -(2 * UnitHelper::PI));
    } else {
        newStartAngle = fmodf(newStartAngle, 2 * UnitHelper::PI);
    }

    float delta = newStartAngle - startAngle;
    startAngle = newStartAngle;
    endAngle = endAngle + delta;

    STARFISH_ASSERT(newStartAngle >= 0 && newStartAngle < 2 * UnitHelper::PI);

    if (anticlockwise && startAngle - endAngle >= 2 * UnitHelper::PI) {
        endAngle = startAngle - 2 * UnitHelper::PI;
    } else if (!anticlockwise && endAngle - startAngle >= 2 * UnitHelper::PI) {
        endAngle = startAngle + 2 * UnitHelper::PI;
    }
}

CanvasPath::CanvasPath(ExecutionContext* executionContext)
    : m_path(nullptr)
    , m_executionContext(executionContext)
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

void CanvasPath::moveTo(float x, float y)
{
    if (m_shouldDisable) {
        return;
    }

    if (isInfOrNan(x) || isInfOrNan(y)) {
        return;
    }

    m_path->moveTo(x, y);
}

void CanvasPath::lineTo(float x, float y)
{
    if (m_shouldDisable) {
        return;
    }

    if (isInfOrNan(x) || isInfOrNan(y)) {
        return;
    }

    m_path->ensureSubPath(x, y);
    m_path->lineTo(x, y);
}

void CanvasPath::quadraticCurveTo(float cpx, float cpy, float x, float y)
{
    if (isInfOrNan(cpx) || isInfOrNan(cpy) || isInfOrNan(x) || isInfOrNan(y)) {
        return;
    }
    m_path->ensureSubPath(cpx, cpy);

    float x0, y0;
    m_path->currentPoint(x0, y0);

    if (x0 == x && y0 == y) {
        // https://en.wikipedia.org/wiki/B%C3%A9zier_curve
        // P(t)=((1-t)^2)A+2t(1-t)B+(t^2)C,(0 <= t <= 1)
        const float t = 0.015625; // 2^-6
        float ex =
            (pow((1 - t), 2) * x0) + (2 * t * (1 - t) * cpx) + pow(t, 2) * x;
        float ey =
            (pow((1 - t), 2) * y0) + (2 * t * (1 - t) * cpy) + pow(t, 2) * y;
        lineTo(ex, ey);
    }

    m_path->quadraticCurveTo(cpx, cpy, x, y);
}

void CanvasPath::bezierCurveTo(float cp1x, float cp1y, float cp2x, float cp2y,
                               float x, float y)
{
    if (isInfOrNan(cp1x) || isInfOrNan(cp1y) || isInfOrNan(cp2x) ||
        isInfOrNan(cp2y) || isInfOrNan(x) || isInfOrNan(y)) {
        return;
    }
    m_path->ensureSubPath(cp1x, cp1y);

    float x0, y0;
    m_path->currentPoint(x0, y0);
    if (x0 == x && y0 == y) {
        // https://en.wikipedia.org/wiki/B%C3%A9zier_curve
        // P(t)=((1-t)^3*A)+(3(1-t)^2*t*B)+((3*(1-t)*(t^2)*C)+((t^3)*D),(0<=t<=1)
        const float t = 0.015625; // 2^-6
        float ex = (pow((1 - t), 3) * x0) + (3 * pow((1 - t), 2) * t * cp1x) +
                   (3 * (1 - t) * (pow(t, 2) * cp2x)) + (pow(t, 3) * x);

        float ey = (pow((1 - t), 3) * y0) + (3 * pow((1 - t), 2) * t * cp1y) +
                   (3 * (1 - t) * (pow(t, 2) * cp2y)) + (pow(t, 3) * y);
        lineTo(ex, ey);
    }

    m_path->bezierCurveTo(cp1x, cp1y, cp2x, cp2y, x, y);
}

static inline float areaOfTriangleFormedByPoints(float x0, float y0, float x1,
                                                 float y1, float x2, float y2)
{
    return x0 * (y1 - y2) + x1 * (y2 - y0) + x2 * (y0 - y1);
}

void CanvasPath::arcTo(float x1, float y1, float x2, float y2, float radius)
{
    // https://html.spec.whatwg.org/multipage/canvas.html#dom-context-2d-arcto
    if (isInfOrNan(x1) || isInfOrNan(y1) || isInfOrNan(x2) || isInfOrNan(y2) ||
        isInfOrNan(radius)) {
        return;
    }
    m_path->ensureSubPath(x1, y1);

    if (radius < 0) {
        throw new DOMException(m_executionContext,
                               DOMException::Code::INDEX_SIZE_ERR,
                               "Radius must not be negative.");
    }
    float x0, y0;
    m_path->currentPoint(x0, y0);

    if ((x0 == x1 && y0 == y1) || (x1 == x2 && y1 == y2) || radius == 0.0f ||
        !areaOfTriangleFormedByPoints(x0, y0, x1, y1, x2, y2)) {
        m_path->lineTo(x1, y1);
    } else {
        m_path->arcTo(x1, y1, x2, y2, radius);
    }
}

void CanvasPath::rect(float x, float y, float w, float h)
{
    if (m_shouldDisable) {
        return;
    }

    if (isInfOrNan(x) || isInfOrNan(y) || isInfOrNan(w) || isInfOrNan(h)) {
        return;
    }

    m_path->rect(x, y, w, h);
}

void CanvasPath::arc(float x, float y, float radius, float startAngle,
                     float endAngle, bool anticlockwise /*=false*/)
{
    if (m_shouldDisable) {
        return;
    }

    // https://html.spec.whatwg.org/multipage/canvas.html#dom-context-2d-arc
    if (isInfOrNan(x) || isInfOrNan(y) || isInfOrNan(radius) ||
        isInfOrNan(startAngle) || isInfOrNan(endAngle)) {
        return;
    }
    if (radius < 0) {
        throw new DOMException(m_executionContext,
                               DOMException::Code::INDEX_SIZE_ERR,
                               "Radius must not be negative.");
    }

    normalizeAngles(startAngle, endAngle, anticlockwise);

    if (!radius || startAngle == endAngle) {
        lineTo(x + radius * cosf(startAngle), y + radius * sinf(startAngle));
    }

    m_path->arc(x, y, radius, startAngle, endAngle, anticlockwise);
}

SkMatrix CanvasPath::transfromMatrixInEllipseMethodSteps(float dx, float dy,
                                                         float rotation)
{
    SkMatrix transform = SkMatrix::I();
    SkScalar sx = SkFloatToScalar(dx);
    SkScalar sy = SkFloatToScalar(dy);
    transform.preTranslate(sx, sy);
    transform.preRotate(
        SkFloatToScalar(UnitHelper::convertFromRadToDeg(rotation)));
    return transform;
}

void CanvasPath::fallbackLineToInEllipseMethodSteps(float radius1, float angle1,
                                                    float radius2, float angle2,
                                                    const SkMatrix& matrix)
{
    SkPoint src;
    src.set(radius1 * cosf(angle1), radius2 * sinf(angle2));
    matrix.mapPoints(&src, 1);
    float x = SkScalarToFloat(src.x());
    float y = SkScalarToFloat(src.y());
    lineTo(x, y);
}

void CanvasPath::ellipse(float x, float y, float radiusX, float radiusY,
                         float rotation, float startAngle, float endAngle,
                         bool anticlockwise /*=false*/)
{
    if (isInfOrNan(x) || isInfOrNan(y) || isInfOrNan(radiusX) ||
        isInfOrNan(radiusY) || isInfOrNan(rotation) || isInfOrNan(startAngle) ||
        isInfOrNan(endAngle)) {
        return;
    }
    if (radiusX < 0 || radiusY < 0) {
        throw new DOMException(m_executionContext,
                               DOMException::Code::INDEX_SIZE_ERR,
                               "Radius must not be negative.");
    }

    normalizeAngles(startAngle, endAngle, anticlockwise);

    if ((!radiusX && !radiusY) || startAngle == endAngle) {
        auto matrix = transfromMatrixInEllipseMethodSteps(x, y, rotation);
        fallbackLineToInEllipseMethodSteps(radiusX, startAngle, radiusY,
                                           startAngle, matrix);
        return;
    } else if (!radiusX || !radiusY) {
        auto matrix = transfromMatrixInEllipseMethodSteps(x, y, rotation);
        fallbackLineToInEllipseMethodSteps(radiusX, startAngle, radiusY,
                                           startAngle, matrix);
        float piOverTwoFloat = static_cast<float>(M_PI_2);
        if (!anticlockwise) {
            for (float angle = startAngle - fmodf(startAngle, piOverTwoFloat) +
                               piOverTwoFloat;
                 angle < endAngle; angle += piOverTwoFloat) {
                fallbackLineToInEllipseMethodSteps(radiusX, angle, radiusY,
                                                   angle, matrix);
            }
        } else {
            for (float angle = startAngle - fmodf(startAngle, piOverTwoFloat);
                 angle > endAngle; angle -= piOverTwoFloat) {
                fallbackLineToInEllipseMethodSteps(radiusX, angle, radiusY,
                                                   angle, matrix);
            }
        }
        fallbackLineToInEllipseMethodSteps(radiusX, endAngle, radiusY, endAngle,
                                           matrix);
        return;
    }

    m_path->ellipse(x, y, radiusX, radiusY, rotation, startAngle, endAngle,
                    anticlockwise);
}
}
#endif
