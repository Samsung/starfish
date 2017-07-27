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

#ifndef __StarFishHTMLUnknownElement__
#define __StarFishHTMLUnknownElement__

#include "core/dom/HTMLElement.h"

namespace StarFish {

class HTMLUnknownElement : public HTMLElement {
public:
    HTMLUnknownElement(Document* document, AtomicString localName);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLUnknownElement() const override;

    /* 4.4 Interface Node */
    virtual QualifiedName name()
    {
        return m_name;
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    QualifiedName m_name;
};
}

#endif
