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

#ifndef __StarfishDOMRectReadOnly__
#define __StarfishDOMRectReadOnly__

#include "binding/ScriptWrappable.h"
#include "core/dom/DOMRectInit.h"

namespace Starfish {

class DOMRectReadOnly : public ScriptWrappable {
public:
    DOMRectReadOnly(ExecutionContext* executionContext, double x, double y,
                    double width, double height);
    DOMRectReadOnly(ExecutionContext* executionContext,
                    const DOMRectInit& init);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(DOMRectReadOnly)

    double x() const
    {
        return m_x;
    }

    double y() const
    {
        return m_y;
    }

    double width() const
    {
        return m_width;
    }

    double height() const
    {
        return m_height;
    }

    double top() const
    {
        return std::min(m_y, m_y + m_height);
    }

    double right() const
    {
        return std::max(m_x, m_x + m_width);
    }

    double bottom() const
    {
        return std::max(m_y, m_y + m_height);
    }

    double left() const
    {
        return std::min(m_x, m_x + m_width);
    }

    ExecutionContext* executionContext()
    {
        return m_executionContext;
    }

    ScriptObject toJSON();

    bool isEmpty();

    bool equals(const DOMRectReadOnly* other) const;

protected:
    ExecutionContext* m_executionContext = nullptr;
    double m_x = 0.0;
    double m_y = 0.0;
    double m_width = 0.0;
    double m_height = 0.0;
};
} // namespace Starfish

#endif
