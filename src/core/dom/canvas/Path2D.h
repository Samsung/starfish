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

#ifndef __StarfishPath2D__
#define __StarfishPath2D__

#ifdef STARFISH_ENABLE_CANVAS

#include "binding/ScriptWrappable.h"

namespace Starfish {

class CanvasRenderingContext;

class Path2D : public ScriptWrappable {
public:
    Path2D(CanvasRenderingContext* context)
        : ScriptWrappable(this)
        , m_canvasRenderingContext(context)
    {
    }
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isPath2D() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;

    // CanvasPath
    void closePath();
    void moveTo(double x, double y);
    void lineTo(double x, double y);
    void bezierCurveTo(double x1, double y1, double x2, double y2, double x3,
                       double y3);
    void rect(double x, double y, double w, double h);
    void arc(double x, double y, double radius, double startAngle,
             double endAngle, bool anticlockwise = false);
    void ellipse(double x, double y, double radiusX, double radiusY,
                 double rotation, double startAngle, double endAngle,
                 bool anticlockwise = false);

private:
    CanvasRenderingContext* m_canvasRenderingContext;
};
}

#endif
#endif
