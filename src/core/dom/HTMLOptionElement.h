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

#ifndef __StarFishHTMLOptionElement__
#define __StarFishHTMLOptionElement__

#include "core/dom/HTMLElement.h"
#include "core/dom/HTMLFormElement.h"

namespace StarFish {

class FrameOptionBox;
class FrameSelectBox;

class HTMLOptionElement : public HTMLFormObject {
    friend FrameOptionBox;
    friend FrameSelectBox;

public:
    HTMLOptionElement(Document* document);
    HTMLOptionElement(Document* document, String* text, String* value,
                      bool defaultSelected);
    HTMLOptionElement(Document* document, String* text, String* value,
                      bool defaultSelected, bool selected);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLOptionElement() const override;

    /* 4.4 Interface Node */
    virtual QualifiedName name();

    // Interface Option
    bool selected();
    void setSelected(bool selected);
    String* selectedAttributeValue();
    void setSelectedAttributeValue(bool selected);
    void setInternalSelected(bool selected);

    // Other methods
    void didAttributeChanged(QualifiedName name, String* old, String* val,
                             bool attributeCreated, bool attributeRemoved);

private:
    bool m_dirtiness;
    bool m_selectedness;
    bool m_drawOptionBox;
};
}
#endif
