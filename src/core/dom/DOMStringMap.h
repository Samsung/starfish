/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishDOMStringMap__
#define __StarFishDOMStringMap__

#include "binding/ScriptWrappable.h"

namespace StarFish {

class Element;

class DOMStringMap : public ScriptWrappable {
public:
    DOMStringMap(Element* element)
        : ScriptWrappable(this)
        , m_element(element)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isDOMStringMap() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;

    Nullable<String*> defaultNamedGetter(String* key);
    bool defaultNamedSetter(String* key, String* value);
    bool defaultNamedDeleter(String* key);
    void defaultNamedEnumerator(GCVector<String*>& enums);

private:
    Element* m_element;
};
}

#endif
