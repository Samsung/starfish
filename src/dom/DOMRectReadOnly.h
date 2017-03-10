/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef __StarFishDOMRectReadOnly__
#define __StarFishDOMRectReadOnly__

#include "util/String.h"
#include "dom/binding/ScriptWrappable.h"

namespace StarFish {

class DOMRectReadOnly : public ScriptWrappable {
public:
    static DOMRectReadOnly* create(double x, double y, double width,
                                   double height);
    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this, instance);
    }

    virtual Type type()
    {
        return ScriptWrappable::Type::DOMRectReadOnlyObject;
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
    DOMRectReadOnly(double x, double y, double width, double height);

    double m_x;
    double m_y;
    double m_width;
    double m_height;
};
}

#endif
