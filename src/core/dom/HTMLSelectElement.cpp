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
        HTMLElement::fillGCDescriptor(desc);
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

HTMLCollection* HTMLSelectElement::selectedOptions()
{
    if (m_selectedOptions) {
        return m_selectedOptions;
    }

    m_selectedOptions = new HTMLCollection(
        this, NodeListImpl::SelectedOptionsFilter, nullptr, false);
    return m_selectedOptions;
}

HTMLOptionElement* HTMLSelectElement::firstSelectedOptionOrFirstOptionElement()
{
    HTMLOptionElement* selected = nullptr;
    HTMLCollection* selectedOptions = HTMLSelectElement::selectedOptions();
    if (selectedOptions->length() > 0) {
        selected = selectedOptions->item(0)->asHTMLOptionElement();
    }

    if (!selected) {
        selected = firstOptionElement();
    }
    // Selected can be nullptr
    return selected;
}

bool HTMLSelectElement::supportsFocus() const
{
    return true;
}
}
