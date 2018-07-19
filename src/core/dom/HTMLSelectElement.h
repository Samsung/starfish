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
#include "binding/HTMLElementOrlongUnion.h"
#include "binding/HTMLOptionElementOrHTMLOptGroupElementUnion.h"

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

    virtual String* value() override;
    virtual void setValue(String* value) override;

    size_t selectedIndex();
    void setSelectedIndex(size_t index);

    String* type() override;
    int size();
    void setSize(int size);

    HTMLOptionsCollection* options();

    unsigned length();
    void setLength(unsigned length);

    HTMLOptionElement* item(unsigned index);
    HTMLOptionElement* namedItem(String* name);

    void add(HTMLOptionElementOrHTMLOptGroupElement element,
             Nullable<HTMLElementOrlong> before);
    using Node::remove;
    void remove(int index);
    bool defaultIndexedSetter(unsigned index, HTMLOptionElement* option);

    /* 4.4 Interface Node */
    virtual QualifiedName name() override;

    HTMLCollection* selectedOptions();
    HTMLCollection* ensureSelectedOptions();

    // Other methods
    HTMLOptionElement* firstOptionElement();
    HTMLOptionElement* firstSelectedOptionElement();
    void computeListOfOptionElements(Node* c,
                                     GCVector<HTMLOptionElement*>& list);

    void fireSelectUpdateNotification();
    bool handleDefaultEvent(Event* event) override;

    void resetFromOption(HTMLOptionElement* resetFrom);

    int displaySize();

    virtual void reset() override;

    virtual bool isListedElement() override
    {
        return true;
    }

    virtual bool isResettableElement() override
    {
        return true;
    }

private:
    void resetFromOption(GCVector<HTMLOptionElement*>& list,
                         HTMLOptionElement* resetFrom);

    void showDropdownMenu();

    HTMLCollection* m_selectedOptions;
    HTMLOptionsCollection* m_options;
};
}

#endif
