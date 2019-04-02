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

#ifndef __StarfishDOMPointReadOnly__
#define __StarfishDOMPointReadOnly__

#include "binding/ScriptWrappable.h"

namespace Starfish {

class DOMPointReadOnly : public ScriptWrappable {
public:
    DOMPointReadOnly(ExecutionContext* executionContext, double x, double y,
                     double z, double w);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(DOMPointReadOnly)

    double x() const
    {
        return m_x;
    }

    double y() const
    {
        return m_y;
    }

    double z() const
    {
        return m_z;
    }

    double w() const
    {
        return m_w;
    }
    // to do : doesn't appear to be supported anywhere yet.
    // DOMPoint matrixTransform(DOMMatrixReadOnly matrix);

    ExecutionContext* executionContext()
    {
        return m_executionContext;
    }

protected:
    ExecutionContext* m_executionContext;
    double m_x;
    double m_y;
    double m_z;
    double m_w;
};
}

#endif
