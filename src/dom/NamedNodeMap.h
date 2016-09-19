/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

#include "dom/binding/ScriptWrappable.h"

namespace StarFish {

class Element;
class Attr;
class NamedNodeMap : public ScriptWrappable {
public:
    NamedNodeMap(ScriptBindingInstance* instance, Element* element)
        : ScriptWrappable(this), m_element(element), m_instance(instance)
    {
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this, instance);
    }

    virtual Type type()
    {
        return ScriptWrappable::Type::NamedNodeMapObject;
    }

    unsigned long length();
    Attr* item(unsigned long index);
    Attr* getNamedItem(QualifiedName name);
    Attr* setNamedItem(Attr* attr);
    void removeNamedItem(QualifiedName name);
    ScriptBindingInstance* striptBindingInstance() { return m_instance; }
    Element* element() { return m_element; }
private:
    Element* m_element;
    ScriptBindingInstance* m_instance;
};

}

#endif
