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

#ifndef __StarFishHTMLFieldSetElement__
#define __StarFishHTMLFieldSetElement__

#include "core/dom/HTMLFormElement.h"

namespace StarFish {

class HTMLFieldSetElement : public HTMLFormControl {
public:
    HTMLFieldSetElement(Document* document)
        : HTMLFormControl(document, false)
    {
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLFieldSetElement() const override;

    /* 4.4 Interface Node */
    virtual QualifiedName name();
    virtual String* type() override;
};
}

#endif
