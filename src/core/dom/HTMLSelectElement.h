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

#ifndef __StarFishHTMLSelectElement__
#define __StarFishHTMLSelectElement__

#include "core/dom/HTMLFormElement.h"

namespace StarFish {

class HTMLOptionElement;
class HTMLCollection;
class HTMLOptionsCollection;

class HTMLSelectElement : public HTMLFormObject {
public:
    HTMLSelectElement(Document* document);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLSelectElement() const override;

    virtual void didNodeInserted(Node* parent, Node* newChild) override;

    virtual String* value();
    virtual void setValue(String* value);

    size_t selectedIndex();
    void setSelectedIndex(size_t index);

    String* type();
    bool multiple();
    void setMultiple(bool multiple);
    bool required();
    void setRequired(bool required);
    int size();
    void setSize(int size);

    HTMLOptionsCollection* options();

    /* 4.4 Interface Node */
    virtual QualifiedName name();

    HTMLCollection* selectedOptions();
    HTMLCollection* ensureSelectedOptions();

    // Other methods
    HTMLOptionElement* firstOptionElement();
    HTMLOptionElement* firstSelectedOptionElement();
    void computeListOfOptionElements(Node* c,
                                     GCVector<HTMLOptionElement*>& list);

    bool supportsFocus() const override;

    void reset(HTMLOptionElement* resetFrom);

    int displaySize();

private:
    HTMLCollection* m_selectedOptions;
    HTMLOptionsCollection* m_options;
};
}

#endif
