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

#ifndef __StarFishHTMLButtonElement__
#define __StarFishHTMLButtonElement__

#include "core/dom/HTMLFormElement.h"

namespace StarFish {

class HTMLButtonElement : public HTMLFormObject {
public:
    HTMLButtonElement(Document* document);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLButtonElement() const override;

    // 4.4 Interface Node

    virtual String* localName();
    virtual QualifiedName name();

    // 4.10.6 Interface Button
    String* type() const override;

    // Other methods
    bool handleDefaultEvent(Event* event) override;
    bool supportsFocus() const override;
};
}

#endif
