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

#ifndef __StarFishDOMSettableTokenList__
#define __StarFishDOMSettableTokenList__

#include "dom/binding/ScriptWrappable.h"

namespace StarFish {

class Element;
class DOMSettableTokenList : public DOMTokenList {
public:
    DOMSettableTokenList(ScriptBindingInstance* instance, Element* element, QualifiedName localName)
        : DOMTokenList(instance, element, localName)
    {
    }

    /* 7.2. Interface DOMSettableTokenList */

    // FIXME:mh.byun(wrong behavior)
    String* value()
    {
        return m_value;
    }

    // FIXME:mh.byun(wrong behavior)
    void setValue(String* value)
    {
        m_value = value;
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this, instance);
    }

private:
    String* m_value;
};
}

#endif
