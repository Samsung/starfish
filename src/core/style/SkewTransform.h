/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishSkewTransform__
#define __StarFishSkewTransform__

#include "core/style/Style.h"

namespace StarFish {

class SkewTransform : public gc {
public:
    SkewTransform(double anglex, double angley)
        : m_angleX(anglex)
        , m_angleY(angley)
    {
    }

    ~SkewTransform()
    {
    }

    void setData(double anglex, double angley)
    {
        m_angleX = anglex;
        m_angleY = angley;
    }

    double angleX()
    {
        return m_angleX;
    }
    double angleY()
    {
        return m_angleY;
    }

    bool operator==(const SkewTransform& o)
    {
        return (this->m_angleX == o.m_angleX) && (this->m_angleY == o.m_angleY);
    }

    bool operator!=(const SkewTransform& o)
    {
        return !operator==(o);
    }

private:
    double m_angleX;
    double m_angleY;
};
}

#endif
