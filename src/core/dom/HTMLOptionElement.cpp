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
#include "core/dom/HTMLOptionElement.h"

namespace StarFish {

HTMLOptionElement::HTMLOptionElement(Document* document)
    : HTMLOptionElement(document, String::emptyString, String::emptyString,
                        false)
{
}

HTMLOptionElement::HTMLOptionElement(Document* document, String* text,
                                     String* value, bool defaultSelected)
    : HTMLOptionElement(document, text, value, false, false)
{
}

HTMLOptionElement::HTMLOptionElement(Document* document, String* text,
                                     String* value, bool defaultSelected,
                                     bool selected)
    : HTMLFormObject(document)
    , m_dirtiness(false)
    , m_selectedness(false)
    , m_drawOptionBox(false)
{
    if (defaultSelected) {
        setSelected(true);
        m_dirtiness = false;
    }
    if (selected) {
        m_selectedness = true;
    }
}

void* HTMLOptionElement::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLOptionElement)] = { 0 };
        HTMLElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLOptionElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

QualifiedName HTMLOptionElement::name()
{
    return starFish()->staticStrings()->m_optionTagName;
}

bool HTMLOptionElement::selected()
{
    return m_selectedness;
}

void HTMLOptionElement::setSelected(bool selected)
{
    if (selected) {
        setAttribute(starFish()->staticStrings()->m_selected,
                     String::createASCIIString("true"));
    } else {
        removeAttribute(starFish()->staticStrings()->m_selected);
    }
    m_dirtiness = true;
}

void HTMLOptionElement::didAttributeChanged(QualifiedName name, String* old,
                                            String* val, bool attributeCreated,
                                            bool attributeRemoved)
{
    HTMLFormObject::didAttributeChanged(name, old, val, attributeCreated,
                                        attributeRemoved);
    if (name == starFish()->staticStrings()->m_selected) {
        if (attributeCreated && !m_dirtiness) {
            m_selectedness = true;
        } else if (attributeRemoved && !m_dirtiness) {
            m_selectedness = false;
        }
    }
}
}
