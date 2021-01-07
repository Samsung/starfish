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

#ifndef __StarfishPath2D__
#define __StarfishPath2D__

#ifdef STARFISH_ENABLE_CANVAS

#include "binding/ScriptWrappable.h"
#include "core/dom/canvas/CanvasPathInterfaceMixIn.h"

namespace Starfish {

class CanvasPath;
class Path2DOrDOMString;

class Path2D : public ScriptWrappable, public CanvasPathInterfaceMixIn {
public:
    Path2D(ExecutionContext* executionContext);
    Path2D(ExecutionContext* executionContext, Path2DOrDOMString& path);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(Path2D)

    // CanvasPathInterfaceMixIn methods
    virtual void closePath() override;
    virtual void moveTo(float x, float y) override;
    virtual void lineTo(float x, float y) override;
    virtual void quadraticCurveTo(float cpx, float cpy, float x,
                                  float y) override;
    virtual void bezierCurveTo(float cp1x, float cp1y, float cp2x, float cp2y,
                               float x, float y) override;
    virtual void arcTo(float x1, float y1, float x2, float y2,
                       float radius) override;
    virtual void rect(float x, float y, float w, float h) override;
    virtual void arc(float x, float y, float radius, float startAngle,
                     float endAngle, bool anticlockwise = false) override;
    virtual void ellipse(float x, float y, float radiusX, float radiusY,
                         float rotation, float startAngle, float endAngle,
                         bool anticlockwise = false) override;

    CanvasPath* canvasPath()
    {
        return m_canvasPath;
    }

private:
    void initFromPath2DOrDOMString(Path2DOrDOMString& path);

    ExecutionContext* m_executionContext;
    CanvasPath* m_canvasPath;
};
} // namespace Starfish

#endif
#endif
