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

#ifndef __StarFishHTMLTableCellElement__
#define __StarFishHTMLTableCellElement__

#include "core/dom/HTMLElement.h"

namespace StarFish {

class HTMLTableCellElement : public HTMLElement {
public:
    HTMLTableCellElement(Document* document)
        : HTMLElement(document)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLTableCellElement() const override;

    /* 4.4 Interface Node */

    virtual String* localName() = 0;
    virtual QualifiedName name() = 0;

    /* table cell related */

    uint32_t colSpan();
    void setColSpan(uint32_t colSpan);

    uint32_t rowSpan();
    void setRowSpan(uint32_t rowSpan);

    /* Not in HTML5 */
    String* bgColor();
    void setBgColor(String* bgColor);
};
}

#endif
