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
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012 Apple Inc.
 * All rights reserved.
 * Copyright (C) 2008, 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2008 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2010 Torch Mobile (Beijing) Co. Ltd. All rights reserved.
 * Copyright (C) 2012, 2013 Intel Corporation. All rights reserved.
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

#include "StarfishConfig.h"
#if defined(PORT_CANVAS_BACKEND_SKIA)

#include "Starfish.h"
#include "SkPoint.h"
#include "SkPath.h"
#include "core/dom/canvas/CanvasFillRule.h"
#include "core/modules/canvas/Path.h"
#include "platform/canvas/PathSkia.h"

namespace Starfish {

Path* Path::create()
{
    return new PathSkia();
}

PathSkia::PathSkia()
    : m_skiaPath(nullptr)
{
    init();
    GC_REGISTER_FINALIZER_NO_ORDER(this,
                                   [](void* obj, void* cd) {
                                       PathSkia* p = (PathSkia*)obj;
                                       p->finalize();
                                   },
                                   NULL, NULL, NULL);
}

PathSkia::~PathSkia()
{
}

void PathSkia::finalize()
{
    delete m_skiaPath;
    m_skiaPath = nullptr;
}

void PathSkia::init()
{
    m_skiaPath = new SkPath();
}

void PathSkia::clear()
{
    STARFISH_RELEASE_ASSERT(m_skiaPath != nullptr)
    m_skiaPath->reset();
}

bool PathSkia::isEmpty()
{
    STARFISH_RELEASE_ASSERT(m_skiaPath != nullptr)
    return m_skiaPath->isEmpty();
}

void PathSkia::currentPoint(float& x, float& y)
{
    SkPoint last;
    m_skiaPath->getLastPt(&last);
    SkScalar sx = last.x();
    SkScalar sy = last.y();
    x = SkScalarToFloat(sx);
    y = SkScalarToFloat(sy);
}

void PathSkia::copy(Path* src)
{
    clear();
    *m_skiaPath = *(((PathSkia*)src)->skiaPath());
}

bool PathSkia::isPointInPath(float x, float y, CanvasFillRule fillRule)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return false;
}

bool PathSkia::isPointInStroke(float x, float y)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return false;
}

void PathSkia::applyPathDrawingStyles(Canvas* canvas)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void PathSkia::closePath()
{
    STARFISH_RELEASE_ASSERT(m_skiaPath != nullptr)
    m_skiaPath->close();
}

void PathSkia::moveTo(float x, float y)
{
    STARFISH_RELEASE_ASSERT(m_skiaPath != nullptr)
    m_needNewSubPath = false;
    m_skiaPath->moveTo(SkFloatToScalar(x), SkFloatToScalar(y));
}

void PathSkia::lineTo(float x, float y)
{
    STARFISH_RELEASE_ASSERT(m_skiaPath != nullptr)
    m_skiaPath->lineTo(SkFloatToScalar(x), SkFloatToScalar(y));
}

void PathSkia::quadraticCurveTo(float cpx, float cpy, float x, float y)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void PathSkia::bezierCurveTo(float cp1x, float cp1y, float cp2x, float cp2y,
                             float x, float y)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void PathSkia::arcTo(float x1, float y1, float x2, float y2, float radius)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void PathSkia::rect(float x, float y, float w, float h)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void PathSkia::arc(float x, float y, float radius, float startAngle,
                   float endAngle, bool anticlockwise /*=false*/)
{
    m_needNewSubPath = false;

    float kR2D = 180 / UnitHelper::PI;
    float twoPI = 2 * UnitHelper::PI;
    float convertedEndAngle = endAngle;

    // from chromium project :
    // blink/renderer/modules/canvas/canvas2d/canvas_path.cc::AdjustEndAngle
    if (anticlockwise) {
        if (startAngle - endAngle >= twoPI) {
            convertedEndAngle = startAngle - twoPI;
        } else if (startAngle < endAngle) {
            convertedEndAngle =
                startAngle - (twoPI - fmodf(endAngle - startAngle, twoPI));
        }
    } else {
        if (endAngle - startAngle >= twoPI) {
            convertedEndAngle = startAngle + twoPI;
        } else if (startAngle > endAngle) {
            convertedEndAngle =
                startAngle + (twoPI - fmodf(startAngle - endAngle, twoPI));
        }
    }
    m_skiaPath->addArc(
        SkRect::MakeXYWH(x - radius, y - radius, 2 * radius, 2 * radius),
        SkFloatToScalar(startAngle * kR2D),
        SkFloatToScalar((convertedEndAngle - startAngle) * kR2D));
}

void PathSkia::ellipse(float x, float y, float radiusX, float radiusY,
                       float rotation, float startAngle, float endAngle,
                       bool anticlockwise /*=false*/)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void PathSkia::postMatrix(const SkMatrix& matrix)
{
    m_skiaPath->transform(matrix);
}

void PathSkia::setCTM(const SkMatrix& matrix)
{
}

Unit::Rect PathSkia::boundingRect(bool isFill)
{
    return Unit::Rect();
}
}
#endif
