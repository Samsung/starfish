/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishDOMRect__
#define __StarfishDOMRect__

#include "core/dom/DOMRectReadOnly.h"

namespace Starfish {

struct DOMRectInit {
    DOMRectInit(double inX = 0, double inY = 0, double inWidth = 0,
                double inHeight = 0);

    double x;
    double y;
    double width;
    double height;
};

class DOMRect : public DOMRectReadOnly {
public:
    DOMRect(ExecutionContext* executionContext, double x = 0, double y = 0,
            double width = 0, double height = 0);
    DOMRect(DOMRectReadOnly*);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isDOMRect() const override;

    void setX(double x)
    {
        m_x = x;
    }

    void setY(double y)
    {
        m_y = y;
    }

    void setWidth(double width)
    {
        m_width = width;
    }

    void setHeight(double height)
    {
        m_height = height;
    }

    void unite(const DOMRect*);
};
}

#endif
