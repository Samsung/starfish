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

#include "core/dom/HTMLOptionElement.h"
#include "core/dom/HTMLCollection.h"
#include "core/dom/Node.h"
#include "core/dom/Traverse.h"

namespace StarFish {

HTMLSelectElement::HTMLSelectElement(Document* document)
    : HTMLFormObject(document)
    , m_selectedOptions(nullptr)
{
}

void* HTMLSelectElement::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLSelectElement)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLSelectElement, m_selectedOptions));
        HTMLFormObject::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLSelectElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

QualifiedName HTMLSelectElement::name()
{
    return starFish()->staticStrings()->m_selectTagName;
}

HTMLOptionElement* HTMLSelectElement::firstOptionElement()
{
    Node* firstOptionNode = Traverse::findDescendant(this, [](Node* d) {
        if (d->isHTMLOptionElement()) {
            return true;
        }
        return false;
    });

    return firstOptionNode ? firstOptionNode->asHTMLOptionElement() : nullptr;
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
            option->setInternalSelected(true);
            selectedOptions->getNodeListImpl().invalidateCache();
        }
    }
    return selectedOptions;
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
                    option->setInternalSelected(true);
                }
            }
        } else {
            Element* element = this->firstElementChild();
            while (element) {
                if (element->isHTMLOptionElement() &&
                    element->asHTMLOptionElement() != newElement) {
                    HTMLOptionElement* option = element->asHTMLOptionElement();
                    String* str =
                        option->selectedAttributeValue()->toASCIILower();
                    if (!str->equals("selected") && option->selected()) {
                        selectedOptions->getNodeListImpl().invalidateCache();
                        option->setInternalSelected(false);
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

bool HTMLSelectElement::supportsFocus() const
{
    return true;
}
}
