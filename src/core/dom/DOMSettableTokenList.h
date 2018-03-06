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

#ifndef __StarFishDOMSettableTokenList__
#define __StarFishDOMSettableTokenList__

#include "core/dom/DOMTokenList.h"

namespace StarFish {

class Element;

class DOMSettableTokenList : public DOMTokenList {
public:
    DOMSettableTokenList(ScriptBindingInstance* instance, Element* element,
                         QualifiedName localName)
        : DOMTokenList(element, localName)
    {
    }

    /* 7.2. Interface DOMSettableTokenList */

    String* value()
    {
        return m_value;
    }

    void setValue(String* value)
    {
        m_value = value;
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isDOMSettableTokenList() const override;

private:
    String* m_value;
};
}

#endif
