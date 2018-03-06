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

#include "core/dom/Event.h"
#include "core/dom/HTMLCollection.h"
#include "core/dom/HTMLOptionElement.h"
#include "core/dom/HTMLOptGroupElement.h"
#include "core/dom/HTMLSelectElement.h"

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
    : HTMLFormControl(document)
    , m_dirtiness(false)
    , m_selectedness(false)
    , m_drawOptionBox(false)
{
    if (!text->equals(String::emptyString)) {
        setTextContent(text);
    }
    if (!value->equals(String::emptyString)) {
        setValue(value);
    }

    if (defaultSelected) {
        setAttribute(starFish()->staticStrings()->m_selected,
                     String::emptyString);
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
        HTMLFormControl::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLOptionElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

QualifiedName HTMLOptionElement::name()
{
    return starFish()->staticStrings()->m_optionTagName;
}

// https://html.spec.whatwg.org/multipage/form-elements.html#dom-option-selected
bool HTMLOptionElement::selected()
{
    return m_selectedness;
}

void HTMLOptionElement::setSelected(bool selected)
{
    if (disabled()) {
        return;
    }

    m_selectedness = selected;
    m_dirtiness = true;

    HTMLSelectElement* select = selectElement();
    if (select) {
        select->reset(this);
    }
}

bool HTMLOptionElement::dirtiness()
{
    return m_dirtiness;
}

void HTMLOptionElement::setDirtiness(bool dirtiness)
{
    m_dirtiness = dirtiness;
}

bool HTMLOptionElement::selectedness()
{
    return m_selectedness;
}

void HTMLOptionElement::setSelectedness(bool selectedness)
{
    m_selectedness = selectedness;
}

// The value IDL attribute, on getting, must return the element's value.
// On setting, the element's value content attribute must be set to the new
// value.
// https://html.spec.whatwg.org/multipage/form-elements.html#dom-option-value
String* HTMLOptionElement::value()
{
    String* val = getAttributeOrEmpty(starFish()->staticStrings()->m_value);
    if (val->equals(String::emptyString)) {
        return text();
    }
    return val;
}

void HTMLOptionElement::setValue(String* value)
{
    setAttribute(starFish()->staticStrings()->m_value, value);
}

bool HTMLOptionElement::isDisabled()
{
    if (disabled()) {
        return true;
    }

    for (Node* p = parentNode(); p; p = p->parentNode()) {
        if (p->isHTMLFormElement() || p->isHTMLIFrameElement()) {
            break;
        } else if (p->isHTMLOptGroupElement()) {
            if (p->asHTMLOptGroupElement()->disabled()) {
                return true;
            }
        }
    }

    return false;
}

HTMLSelectElement* HTMLOptionElement::selectElement()
{
    for (Node* p = parentNode(); p; p = p->parentNode()) {
        if (p->isHTMLIFrameElement() || p->isHTMLFormElement()) {
            break;
        }
        if (p->isHTMLSelectElement()) {
            return p->asHTMLSelectElement();
        }
    }

    return nullptr;
}

String* HTMLOptionElement::text()
{
    Nullable<String*> value = textContent();
    if (value.hasValue()) {
        return value.getValue();
    }
    return String::emptyString;
}

void HTMLOptionElement::setText(String* value)
{
    setTextContent(value);
}

bool HTMLOptionElement::defaultSelected()
{
    Nullable<String*> val =
        getAttribute(starFish()->staticStrings()->m_selected);
    return val.hasValue();
}

bool HTMLOptionElement::handleDefaultEvent(Event* event)
{
    if (HTMLElement::handleDefaultEvent(event)) {
        return true;
    }

    if (isDisabled()) {
        return false;
    }

    if (event->isMouseEvent() || event->isTouchEvent()) {
        if (event->type()->equalsIgnoreCase("click")) {
            HTMLSelectElement* select = selectElement();

            if (select) {
                if (select->multiple()) {
                    // toggle for in multiple mode
                    if (selected()) {
                        setSelected(false);
                    } else {
                        setSelected(true);
                    }
                } else {
                    // single selection only
                    setSelected(true);
                }
            }
        }
    }

    return false;
}

void HTMLOptionElement::didAttributeChanged(QualifiedName name, String* old,
                                            String* val, bool attributeCreated,
                                            bool attributeRemoved)
{
    HTMLFormControl::didAttributeChanged(name, old, val, attributeCreated,
                                         attributeRemoved);
    if (name == starFish()->staticStrings()->m_selected && !m_dirtiness) {
        // Setting`selected` attribute can affect selectness only when
        // its dirtiness flag is false.
        setSelectedness(!attributeRemoved);
        HTMLSelectElement* select = selectElement();
        if (select) {
            select->reset(this);
        }
        return;
    }
}
}
