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

#ifndef __StarFishDOMStringList__
#define __StarFishDOMStringList__

#include "binding/ScriptWrappable.h"

namespace StarFish {

class Document;
class DOMStringList : public ScriptWrappable, public GCVector<String*> {
public:
    DOMStringList(Document* document);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isDOMStringList() const override;

    virtual ScriptBindingInstance* scriptBindingInstance() override
    {
        return m_scriptBindingInstance;
    }

    size_t length() const
    {
        return m_size;
    }

    Nullable<String*> item(unsigned long index)
    {
        if (index < size()) {
            return at(index);
        }
        return Nullable<String*>();
    }

    bool contains(String* item)
    {
        return std::find(begin(), end(), item) != end();
    }

private:
    ScriptBindingInstance* m_scriptBindingInstance;
};
}
#endif
