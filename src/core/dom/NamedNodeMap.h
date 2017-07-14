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
