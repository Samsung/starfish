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

#ifndef __StarFishHTMLHeadingElement__
#define __StarFishHTMLHeadingElement__

#include "core/dom/HTMLElement.h"

namespace StarFish {

class HTMLHeadingElement : public HTMLElement {
public:
    HTMLHeadingElement(Document* document, AtomicString name);

    virtual void init(ScriptBindingInstance* instance) override;
    virtual bool isHTMLHeadingElement() const override;

    /* 4.4 Interface Node */

    virtual String* localName()
    {
        return m_name.localName();
    }

    virtual QualifiedName name()
    {
        return m_name;
    }

protected:
    QualifiedName m_name;
};
}

#endif
