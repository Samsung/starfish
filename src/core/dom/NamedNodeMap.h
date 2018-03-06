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

#ifndef __StarFishNamedNodeMap__
#define __StarFishNamedNodeMap__

#include "binding/ScriptWrappable.h"

namespace StarFish {

class Element;
class Attr;
class NamedNodeMap : public ScriptWrappable {
public:
    NamedNodeMap(Element* element)
        : ScriptWrappable(this)
        , m_element(element)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isNamedNodeMap() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;

    size_t length();
    Attr* item(unsigned long index);

    Attr* getNamedItem(String* name);
    Attr* getNamedItemNS(Nullable<String*> ns, String* localName);

    Attr* setNamedItem(Attr* attr);
    Attr* setNamedItemNS(Attr* attr);

    Attr* removeNamedItem(String* name);
    Attr* removeNamedItemNS(Nullable<String*> ns, String* localName);

    Element* element()
    {
        return m_element;
    }

private:
    Element* m_element;
};
}

#endif
