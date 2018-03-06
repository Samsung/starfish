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

#ifndef __StarFishDOMRectReadOnly__
#define __StarFishDOMRectReadOnly__

#include "binding/ScriptWrappable.h"

namespace StarFish {

class DOMRectReadOnly : public ScriptWrappable {
public:
    DOMRectReadOnly(Document* document, double x, double y, double width,
                    double height);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isDOMRectReadOnly() const override;
    virtual ScriptBindingInstance* scriptBindingInstance()
    {
        return m_scriptBindingInstance;
    }

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

protected:
    ScriptBindingInstance* m_scriptBindingInstance;
    double m_x;
    double m_y;
    double m_width;
    double m_height;
};
}

#endif
