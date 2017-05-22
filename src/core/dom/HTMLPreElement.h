/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

#ifndef __StarFishHTMLPreElement__
#define __StarFishHTMLPreElement__

#include "core/dom/HTMLElement.h"

namespace StarFish {

class HTMLPreElement : public HTMLElement {
public:
    HTMLPreElement(Document* document)
        : HTMLElement(document)
    {
    }

    virtual void init(ScriptBindingInstance* instance) override;
    virtual bool isHTMLPreElement() const override;

    /* 4.4 Interface Node */

    virtual String* localName();
    virtual QualifiedName name();
};
}

#endif
