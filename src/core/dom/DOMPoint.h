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

#ifndef __StarFishDOMPoint__
#define __StarFishDOMPoint__

#include "core/dom/DOMPointReadOnly.h"

namespace StarFish {

struct DOMPointInit {
public:
    DOMPointInit(double inX = 0, double inY = 0, double inZ = 0,
                 double inW = 1);

    double x() const;
    void setX(double x);

    double y() const;
    void setY(double y);

    double z() const;
    void setZ(double z);

    double w() const;
    void setW(double w);

private:
    double m_x;
    double m_y;
    double m_z;
    double m_w;
};

class DOMPoint : public DOMPointReadOnly {
public:
    DOMPoint(Document* document, double x = 0, double y = 0, double z = 0,
             double w = 1);
    DOMPoint(Document* document, const DOMPointInit&);

    void setX(double x)
    {
        m_x = x;
    }

    void setY(double y)
    {
        m_y = y;
    }

    void setZ(double z)
    {
        m_z = z;
    }

    void setW(double w)
    {
        m_w = w;
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isDOMPoint() const override;
};
}

#endif
