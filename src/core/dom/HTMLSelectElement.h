/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#ifndef __StarFishHTMLSelectElement__
#define __StarFishHTMLSelectElement__

#include "core/dom/HTMLFormElement.h"

namespace StarFish {

class HTMLOptionElement;
class HTMLCollection;
class HTMLOptionsCollection;

class HTMLSelectElement : public HTMLFormControl {
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

    String* type() override;
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

    void fireSelectUpdateNotification();
    bool handleDefaultEvent(Event* event);

    void reset(HTMLOptionElement* resetFrom);

    int displaySize();

private:
    HTMLCollection* m_selectedOptions;
    HTMLOptionsCollection* m_options;
};
}

#endif
