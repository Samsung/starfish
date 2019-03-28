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

#ifndef __StarfishCanvasPath__
#define __StarfishCanvasPath__

#ifdef STARFISH_ENABLE_CANVAS

namespace Starfish {

class Path;

class CanvasPathInterfaceMixIn {
    // Shared path API methods
public:
    CanvasPathInterfaceMixIn() = default;
    virtual ~CanvasPathInterfaceMixIn() = default;
    virtual void closePath() = 0;
    virtual void moveTo(double x, double y) = 0;
    virtual void lineTo(double x, double y) = 0;
    virtual void quadraticCurveTo(double cpx, double cpy, double x,
                                  double y) = 0;
    virtual void bezierCurveTo(double cp1x, double cp1y, double cp2x,
                               double cp2y, double x, double y) = 0;
    virtual void arcTo(double x1, double y1, double x2, double y2,
                       double radius) = 0;
    virtual void rect(double x, double y, double w, double h) = 0;
    virtual void arc(double x, double y, double radius, double startAngle,
                     double endAngle, bool anticlockwise = false) = 0;
    virtual void ellipse(double x, double y, double radiusX, double radiusY,
                         double rotation, double startAngle, double endAngle,
                         bool anticlockwise = false) = 0;
};

class CanvasPath : public CanvasPathInterfaceMixIn, public gc {
public:
    CanvasPath();
    virtual ~CanvasPath()
    {
    }
    // CanvasPathInterfaceMixIn
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

    Path* path()
    {
        return m_path;
    }

private:
    void init();
    Path* m_path;
};
}

#endif
#endif
