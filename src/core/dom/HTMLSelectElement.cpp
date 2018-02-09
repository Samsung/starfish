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
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/BrowsingContext.h"

namespace StarFish {

HTMLSelectElement::HTMLSelectElement(Document* document)
    : HTMLFormControl(document)
    , m_selectedOptions(nullptr)
    , m_options(nullptr)
{
}

void* HTMLSelectElement::operator new(size_t size)
{
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

    setNeedsFrameTreeBuild(Node::UpdateAtSelf);
}

HTMLOptionElement* HTMLSelectElement::firstOptionElement()
{
    GCVector<HTMLOptionElement*> list;
    computeListOfOptionElements(this, list);

    if (list.empty()) {
        return nullptr;
    }

    return list[0];
}

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

HTMLCollection* HTMLSelectElement::ensureSelectedOptions()
{
    if (!m_selectedOptions) {
        m_selectedOptions = new HTMLCollection(
            this, NodeListImpl::SelectedOptionsFilter, nullptr, true);
    }
    return m_selectedOptions;
}

HTMLCollection* HTMLSelectElement::selectedOptions()
{
    HTMLCollection* selectedOptions = ensureSelectedOptions();
    if (selectedOptions->length() < 1) {
        HTMLOptionElement* option = firstOptionElement();
        if (option != nullptr && option->selected() != true) {
            option->setSelectedness(true);
            selectedOptions->getNodeListImpl().invalidateCache();
        }
    }
    return selectedOptions;
}

String* HTMLSelectElement::type()
{
    if (multiple()) {
        return String::createASCIIString("select-one");
    } else {
        return String::createASCIIString("select-multiple");
    }

    return String::emptyString;
}

int HTMLSelectElement::size()
{
    String* size = getAttributeOrEmpty(starFish()->staticStrings()->m_size);
    if (size->equals(String::emptyString)) {
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

HTMLOptionsCollection* HTMLSelectElement::options()
{
    if (!m_options) {
        m_options = new HTMLOptionsCollection(
            this, NodeListImpl::OptionElementFilter, nullptr, false);
    }

    return m_options;
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

void HTMLSelectElement::reset(HTMLOptionElement* resetFrom)
{
    Nullable<String*> val =
        getAttribute(starFish()->staticStrings()->m_multiple);
    GCVector<HTMLOptionElement*> list;
    computeListOfOptionElements(this, list);

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
                if (opt->selected() && !opt->disabled()) {
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

void HTMLSelectElement::didNodeInserted(Node* parent, Node* newChild)
{
    if (newChild->isHTMLOptionElement()) {
        HTMLOptionElement* newElement = newChild->asHTMLOptionElement();
        HTMLCollection* selectedOptions = ensureSelectedOptions();

        if (!newElement->selected()) {
            if (selectedOptions->length() < 1) {
                HTMLOptionElement* option = firstOptionElement();
                if (option != nullptr && option->selected() != true) {
                    selectedOptions->getNodeListImpl().invalidateCache();
                    option->setSelectedness(true);
                }
            }
        } else {
            Element* element = this->firstElementChild();
            while (element) {
                if (element->isHTMLOptionElement() &&
                    element->asHTMLOptionElement() != newElement) {
                    HTMLOptionElement* option = element->asHTMLOptionElement();
                    bool hasSelected = option->hasSelectedAttribute();

                    if (hasSelected && option->selected()) {
                        selectedOptions->getNodeListImpl().invalidateCache();
                        option->setSelectedness(false);
                    }
                }
                element = element->nextElementSibling();
            }
        }
    }
}

HTMLOptionElement* HTMLSelectElement::firstSelectedOptionElement()
{
    HTMLCollection* selectedOptions = HTMLSelectElement::selectedOptions();
    if (selectedOptions->length() > 0) {
        return selectedOptions->item(0)->asHTMLOptionElement();
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
                    fireSelectUpdateNotification();
                }
            }
        }
    }

    return false;
}
}
