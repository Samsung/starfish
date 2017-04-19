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

#ifndef __StarFishHTMLTDElement__
#define __StarFishHTMLTDElement__

#include "dom/HTMLTableCellElement.h"

namespace StarFish {

class HTMLTDElement : public HTMLTableCellElement {
public:
    HTMLTDElement(Document* document)
        : HTMLTableCellElement(document)
    {
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    /* 4.4 Interface Node */

    virtual String* localName();
    virtual QualifiedName name();

    /* Other methods (not in DOM API) */

    virtual bool isHTMLTDElement() const override
    {
        return true;
    }
};
}

#endif
