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

#include "StarFishConfig.h"
#include "StarFish.h"

#include "core/dom/HTMLSelectElement.h"

#include "core/dom/Event.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLOptionElement.h"
#include "core/dom/HTMLCollection.h"
#include "core/dom/HTMLOptionsCollection.h"
#include "core/dom/Node.h"
#include "core/dom/Traverse.h"
#include "core/page/BrowsingContext.h"
#include "platform/window/PlatformWindow.h"

namespace StarFish {

HTMLSelectElement::HTMLSelectElement(Document* document)
    : HTMLFormControl(document)
    , m_selectedOptions(nullptr)
    , m_options(nullptr)
{
}

void* HTMLSelectElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(HTMLSelectElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLSelectElement)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLSelectElement, m_selectedOptions));
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLSelectElement, m_options));
        HTMLFormControl::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLSelectElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

QualifiedName HTMLSelectElement::name()
{
    return starFish()->staticStrings()->m_selectTagName;
}

String* HTMLSelectElement::value()
{
    HTMLOptionElement* firstOptionNode = nullptr;
    GCVector<HTMLOptionElement*> list;
    computeListOfOptionElements(this, list);
    for (Node* c : list) {
        HTMLOptionElement* opt = c->asHTMLOptionElement();
        if (opt->selected()) {
            firstOptionNode = opt;
            break;
        }
    }

    if (firstOptionNode) {
        HTMLOptionElement* opt = firstOptionNode->asHTMLOptionElement();
        return opt->value();
    } else {
        return String::emptyString;
    }
}

void HTMLSelectElement::setValue(String* value)
{
    GCVector<HTMLOptionElement*> list;
    computeListOfOptionElements(this, list);
    for (Node* c : list) {
        HTMLOptionElement* opt = c->asHTMLOptionElement();
        opt->setSelected(false);
    }

    for (Node* c : list) {
        HTMLOptionElement* opt = c->asHTMLOptionElement();
        if (opt->value()->equals(value)) {
            opt->setSelected(true);
            break;
        }
    }

    fireEvent(starFish()->staticStrings()->m_change, true, false);

    setNeedsFrameTreeBuild();
}

// use this function internally to retrive a list of option elements efficiently
void HTMLSelectElement::computeListOfOptionElements(
    Node* parent, GCVector<HTMLOptionElement*>& list)
{
    for (Node* c = parent->firstChild(); c; c = c->nextSibling()) {
        if (c->isHTMLOptionElement()) {
            list.push_back(c->asHTMLOptionElement());
        } else if (c->isHTMLOptGroupElement()) {
            computeListOfOptionElements(c, list);
        }
    }
}

// IDL method
HTMLCollection* HTMLSelectElement::selectedOptions()
{
    if (!m_selectedOptions) {
        m_selectedOptions = new HTMLCollection(
            this, NodeListImpl::SelectedOptionsFilter, nullptr, false);
    }
    return m_selectedOptions;
}

// internal use for faster computation
void HTMLSelectElement::computeSelectedOptions(
    GCVector<HTMLOptionElement*>& list)
{
    GCVector<HTMLOptionElement*> listAll;
    computeListOfOptionElements(this, listAll);

    for (auto item : listAll) {
        if (item->selectedness()) {
            list.push_back(item);
        }
    }
}

String* HTMLSelectElement::type()
{
    if (multiple()) {
        return String::createASCIIString("select-multiple");
    } else {
        return String::createASCIIString("select-one");
    }
}

int HTMLSelectElement::size()
{
    String* size = getAttributeOrEmpty(starFish()->staticStrings()->m_size);
    if (!size->equals(String::emptyString)) {
        return String::parseInt(size);
    }

    return 0;
}

void HTMLSelectElement::setSize(int size)
{
    if (size > 0) {
        setAttribute(starFish()->staticStrings()->m_size,
                     String::fromInt(size));
    }
}

int HTMLSelectElement::displaySize()
{
    int s = size();
    if (s > 0) {
        return s;
    }

    // https://html.spec.whatwg.org/multipage/form-elements.html#the-select-element
    return multiple() ? 4 : 1;
}

void HTMLSelectElement::reset()
{
    GCVector<HTMLOptionElement*> list;
    computeListOfOptionElements(this, list);

    for (auto item : list) {
        if (item->isHTMLOptionElement()) {
            auto o = item->asHTMLOptionElement();
            o->setSelectedness(o->defaultSelected());
            o->setDirtiness(false);
        }
    }
    for (auto item : list) {
        if (item->isHTMLOptionElement()) {
            auto o = item->asHTMLOptionElement();
            resetFromOption(list, o);
        }
    }
}

// IDL method
HTMLOptionsCollection* HTMLSelectElement::options()
{
    if (!m_options) {
        m_options = new HTMLOptionsCollection(
            this, NodeListImpl::OptionElementFilter, nullptr, false);
    }

    return m_options;
}

unsigned HTMLSelectElement::length()
{
    return options()->length();
}

void HTMLSelectElement::setLength(unsigned newLength)
{
    unsigned currentLength = length();

    if (currentLength < newLength) {
        while (currentLength < newLength) {
            appendChild(new HTMLOptionElement(document()));
            currentLength++;
        }
    } else {
        while (currentLength > newLength) {
            remove(currentLength - 1);
            currentLength--;
        }
    }
}

HTMLOptionElement* HTMLSelectElement::item(unsigned index)
{
    if (Element* option = options()->item(index)) {
        return option->asHTMLOptionElement();
    }

    return nullptr;
}

HTMLOptionElement* HTMLSelectElement::namedItem(String* name)
{
    if (Element* option = options()->namedItem(name)) {
        return option->asHTMLOptionElement();
    }

    return nullptr;
}

void HTMLSelectElement::add(HTMLOptionElementOrHTMLOptGroupElement element,
                            Nullable<HTMLElementOrlong> before)
{
    HTMLElement* newElement;
    if (element.isHTMLOptionElementValue()) {
        newElement = element.getHTMLOptionElementValue();
    } else {
        STARFISH_ASSERT(element.isHTMLOptGroupElementValue());
        newElement = element.getHTMLOptGroupElementValue();
    }

    HTMLElement* beforeElement = nullptr;
    if (before.hasValue()) {
        if (before.getValue().isHTMLElementValue()) {
            beforeElement = before.getValue().getHTMLElementValue();
        } else if (before.getValue().islongValue()) {
            beforeElement = item(before.getValue().getlongValue());
        }
    }
    insertBefore(newElement, beforeElement);
}

void HTMLSelectElement::remove(int index)
{
    if (index < 0) {
        return;
    }

    if (HTMLOptionElement* option = item(index)) {
        option->remove();
    }
}

bool HTMLSelectElement::defaultIndexedSetter(unsigned index,
                                             HTMLOptionElement* option)
{
    if (!option) {
        remove(index);
        return true;
    }

    if (index > length()) {
        setLength(index);
    }

    if (index == length()) {
        appendChild(option);
        return true;
    }

    HTMLOptionElement* oldOption = item(index);
    Node* parent = oldOption->parentNode();

    STARFISH_ASSERT(oldOption && parent);

    parent->replaceChild(option, oldOption);
    return true;
}

size_t HTMLSelectElement::selectedIndex()
{
    GCVector<HTMLOptionElement*> list;
    computeListOfOptionElements(this, list);

    for (size_t i = 0; i < list.size(); i++) {
        HTMLOptionElement* opt = list[i];

        if (opt->selected()) {
            return i;
        }
    }

    return -1;
}

void HTMLSelectElement::setSelectedIndex(size_t index)
{
    GCVector<HTMLOptionElement*> list;
    computeListOfOptionElements(this, list);

    for (HTMLOptionElement* opt : list) {
        opt->setSelectedness(false);
    }

    for (size_t i = 0; i < list.size(); i++) {
        HTMLOptionElement* opt = list[i];
        if (i == index) {
            opt->setSelectedness(true);
            opt->setDirtiness(true);
        }
    }
}

void HTMLSelectElement::resetFromOption(GCVector<HTMLOptionElement*>& list,
                                        HTMLOptionElement* resetFrom)
{
    Nullable<String*> val =
        getAttribute(starFish()->staticStrings()->m_multiple);
    if (!val.hasValue()) {
        // single selection
        int selectedOptions = 0;
        for (HTMLOptionElement* opt : list) {
            if (opt->selected()) {
                selectedOptions++;
            }
        }

        if (displaySize() == 1 && selectedOptions == 0) {
            for (HTMLOptionElement* opt : list) {
                if (opt->defaultSelected() && !opt->disabled()) {
                    opt->setSelectedness(true);
                    break;
                }
            }
        } else if (selectedOptions >= 2) {
            for (HTMLOptionElement* opt : list) {
                if (opt != resetFrom) {
                    opt->setSelectedness(false);
                }
            }
        }
    } else {
        // Todo
    }
}

void HTMLSelectElement::resetFromOption(HTMLOptionElement* resetFrom)
{
    GCVector<HTMLOptionElement*> list;
    computeListOfOptionElements(this, list);
    resetFromOption(list, resetFrom);
}

void HTMLSelectElement::didNodeInserted(Node* parent, Node* newChild)
{
    if (!newChild->isHTMLOptionElement()) {
        return;
    }

    HTMLOptionElement* newElement = newChild->asHTMLOptionElement();
    GCVector<HTMLOptionElement*> selectedOptions;
    computeSelectedOptions(selectedOptions);

    if (!multiple()) {
        if (displaySize() == 1 && selectedOptions.size() == 0) {
            GCVector<HTMLOptionElement*> list;
            computeListOfOptionElements(this, list);
            for (auto option : list) {
                if (!option->isDisabled()) {
                    option->setSelectedness(true);
                    break;
                }
            }
        } else if (selectedOptions.size() >= 2) {
            GCVector<HTMLOptionElement*> list;
            computeListOfOptionElements(this, list);

            for (size_t i = 0; i < list.size() - 1; i++) {
                list[i]->setSelectedness(false);
            }
        }
    }
}

HTMLOptionElement* HTMLSelectElement::firstSelectedOptionElement()
{
    GCVector<HTMLOptionElement*> selectedOptions;
    computeSelectedOptions(selectedOptions);
    if (selectedOptions.size() > 0) {
        return selectedOptions[0];
    }

    return nullptr;
}

// https://html.spec.whatwg.org/multipage/form-elements.html#send-select-update-notifications
void HTMLSelectElement::fireSelectUpdateNotification()
{
    queueEvent(starFish()->staticStrings()->m_input, true, false);
    queueEvent(starFish()->staticStrings()->m_change, true, false);
}

// https://html.spec.whatwg.org/multipage/form-elements.html#the-select-element
bool HTMLSelectElement::handleDefaultEvent(Event* event)
{
    if (HTMLElement::handleDefaultEvent(event)) {
        return true;
    }

    if (isDisabled()) {
        // Don't do anything. Just propagate.
        return false;
    }

    if (event->isMouseEvent() || event->isTouchEvent()) {
        if (event->type()->equalsIgnoreCase("click")) {
            if (event->target()->isHTMLOptionElement()) {
                HTMLOptionElement* option =
                    event->target()->asHTMLOptionElement();
                if (option->selectElement() == this) {
                    showDropdownMenu();
                }
            }
        }
    }

    return false;
}

void HTMLSelectElement::showDropdownMenu()
{
    // register the callback to be called when an item is selected
    document()->starFish()->platformWindow()->registerCallbackHandler(
        std::string("onDropdownMenuItemSelected"), [this](void* param) -> void {
            struct Param {
                int position;
            };
            Param* p = (Param*)param;
            onDropdownMenuItemSelected(p->position);
            delete p;
        });

    // calls the platform's dropdownmenu UI
    struct Param {
        std::vector<std::string>* list;
    };

    Param* p = new Param();
    p->list = new std::vector<std::string>();

    GCVector<HTMLOptionElement*> list;
    computeListOfOptionElements(this, list);
    for (auto item : list) {
        if (item->isHTMLOptionElement()) {
            auto o = item->asHTMLOptionElement();
            p->list->push_back(o->text()->toUTF8NonGCString().data());
        }
    }

    document()->starFish()->platformWindow()->callHandler(
        std::string("showDropdownMenu"), (void*)p);
}

void HTMLSelectElement::onDropdownMenuItemSelected(int position)
{
    GCVector<HTMLOptionElement*> list;
    computeListOfOptionElements(this, list);

    if (0 <= position && (size_t)position < list.size()) {
        if (!multiple()) {
            if (!list[position]->selectedness()) {
                setSelectedIndex(position);
                setNeedsFrameTreeBuild();
                fireSelectUpdateNotification();
            }
        } else {
            // TODO: multiple selection
        }
    }
}
}
