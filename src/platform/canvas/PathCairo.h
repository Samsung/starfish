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

#ifndef __StarfishPathCairo__
#define __StarfishPathCairo__

namespace Starfish {

class Path;

class PathCairo : public Path {
public:
    PathCairo();
    ~PathCairo();

    cairo_t* context()
    {
        return m_cairoContext;
    }
    void finalize();

    virtual void init() override;
    virtual void clear() override;
    virtual bool isEmpty() override;

    // For CanvasPath
    virtual void closePath() override;
    virtual void moveTo(double x, double y) override;
    virtual void lineTo(double x, double y) override;
    virtual void quadraticCurveTo(double cpx, double cpy, double x,
                                  double y) override;
    virtual void bezierCurveTo(double cp1x, double cp1y, double cp2x,
                               double cp2y, double x, double y) override;
    virtual void arcTo(double x1, double y1, double x2, double y2,
                       double radius) override;
    virtual void rect(double x, double y, double w, double h) override;
    virtual void arc(double x, double y, double radius, double startAngle,
                     double endAngle, bool anticlockwise = false) override;
    virtual void ellipse(double x, double y, double radiusX, double radiusY,
                         double rotation, double startAngle, double endAngle,
                         bool anticlockwise = false) override;

private:
    cairo_t* m_cairoContext;
    cairo_surface_t* m_dumyCairoSurface;
};
}
#endif
