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

#ifndef __StarfishPath__
#define __StarfishPath__

namespace Starfish {

class NativPath;

class Path : public gc {
public:
    static Path* create();

    virtual ~Path()
    {
    }

    virtual void init() = 0;
    virtual void clear() = 0;
    virtual bool isEmpty() = 0;
    virtual void currentPoint(float& x, float& y) = 0;
    virtual void copy(Path* src) = 0;

    // For CanvasPath
    virtual void closePath() = 0;
    virtual void moveTo(float x, float y) = 0;
    virtual void lineTo(float x, float y) = 0;
    virtual void quadraticCurveTo(float cpx, float cpy, float x, float y) = 0;
    virtual void bezierCurveTo(float cp1x, float cp1y, float cp2x, float cp2y,
                               float x, float y) = 0;
    virtual void arcTo(float x1, float y1, float x2, float y2,
                       float radius) = 0;
    virtual void rect(float x, float y, float w, float h) = 0;
    virtual void arc(float x, float y, float radius, float startAngle,
                     float endAngle, bool anticlockwise = false) = 0;
    virtual void ellipse(float x, float y, float radiusX, float radiusY,
                         float rotation, float startAngle, float endAngle,
                         bool anticlockwise = false) = 0;

    bool needNewSubPath()
    {
        return m_needNewSubPath;
    }

    void ensureSubPath(float x, float y)
    {
        // https://html.spec.whatwg.org/multipage/canvas.html#ensure-there-is-a-subpath
        if (isEmpty() || needNewSubPath()) {
            moveTo(x, y);
        }
    }

protected:
    Path()
        : m_needNewSubPath(true)
    {
    }

    bool m_needNewSubPath;
};
}

#endif
