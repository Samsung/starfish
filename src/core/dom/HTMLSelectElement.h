/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#ifndef __StarfishHTMLSelectElement__
#define __StarfishHTMLSelectElement__

#include "core/dom/HTMLFormElement.h"
#include "binding/generated/HTMLElementOrlongUnion.h"
#include "binding/generated/HTMLOptionElementOrHTMLOptGroupElementUnion.h"

namespace Starfish {

class HTMLOptionElement;
class HTMLCollection;
class HTMLOptionsCollection;

class HTMLSelectElement : public HTMLFormControl {
public:
    HTMLSelectElement(Document* document, const QualifiedName& qname);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLSelectElement() const override;

    virtual void didNodeInserted(Node* parent, Node* newChild) override;

    virtual String* value() override;
    virtual void setValue(String* value) override;

    int32_t selectedIndex();
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

    HTMLCollection* selectedOptions();

    // Other methods
    HTMLOptionElement* firstSelectedOptionElement();
    void computeListOfOptionElements(Node* c,
                                     GCVector<HTMLOptionElement*>& list);
    void computeSelectedOptions(GCVector<HTMLOptionElement*>& list);

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
    void addSelectedIndex(StringBuilder& data);
    void addBaseStyle(StringBuilder& data);
    void addChildren(StringBuilder& data);
    void addOption(HTMLOptionElement* option, StringBuilder& data);
    void addSeparator(HTMLBRElement* br, StringBuilder& data);
    void addElementStyle(HTMLElement* option, StringBuilder& data);

    void addProperty(const char* name, String* value, StringBuilder& data);
    void addProperty(const char* name, int value, StringBuilder& data);
    void addProperty(const char* name, bool value, StringBuilder& data);
    void addProperty(const char* name, float value, StringBuilder& data);
    void addProperty(const char* name, double value, StringBuilder& data);
    void addProperty(const char* name, const GCVector<String*>& value,
                     StringBuilder& data);
    void addProperty(const char* name, const Unit::Rect& value,
                     StringBuilder& data);

    void showDropdownMenu();
    void onDropdownMenuItemSelected(int position);

    HTMLCollection* m_selectedOptions;
    HTMLOptionsCollection* m_options;
};
} // namespace Starfish

#endif
