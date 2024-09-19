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

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/dom/Event.h"
#include "core/dom/HTMLCollection.h"
#include "core/dom/HTMLOptionElement.h"
#include "core/dom/HTMLOptGroupElement.h"
#include "core/dom/HTMLSelectElement.h"

namespace Starfish {

static String* g_markerStr = nullptr;

static String* getMarkerStr()
{
    if (!g_markerStr) {
        g_markerStr = String::createASCIIStringWithNoGC(
            "<svg style=\"float:right;\" height=\"1em\" width=\"1em\" "
            "viewBox=\"0 "
            "0 10 10\"><polygon points=\"2,3 8,3 5,8\" "
            "style=\"stroke:black;stroke-width:1;\"/></svg>");
    }
    return g_markerStr;
}

HTMLOptionElement::HTMLOptionElement(Document* document)
    : HTMLOptionElement(document,
                        document->starfish()->staticStrings()->m_optionTagName,
                        String::emptyString, String::emptyString, false, false)
{
}

HTMLOptionElement::HTMLOptionElement(Document* document, String* text,
                                     String* value, bool defaultSelected)
    : HTMLOptionElement(document,
                        document->starfish()->staticStrings()->m_optionTagName,
                        text, value, defaultSelected, false)
{
}

HTMLOptionElement::HTMLOptionElement(Document* document, String* text,
                                     String* value, bool defaultSelected,
                                     bool selected)
    : HTMLOptionElement(document,
                        document->starfish()->staticStrings()->m_optionTagName,
                        text, value, defaultSelected, selected)
{
}

HTMLOptionElement::HTMLOptionElement(Document* document,
                                     const QualifiedName& qname, String* text,
                                     String* value, bool defaultSelected,
                                     bool selected)

    : HTMLFormControl(document, qname)
    , m_dirtiness(false)
    , m_selectedness(false)
    , m_showMarker(false)
    , m_marker(nullptr)
{
    if (!text->equals(String::emptyString)) {
        setTextContent(text);
    }
    if (!value->equals(String::emptyString)) {
        setValue(value);
    }

    if (defaultSelected) {
        setAttribute(starfish()->staticStrings()->m_selected,
                     String::emptyString);
    }
    if (selected) {
        m_selectedness = true;
    }
}

void HTMLOptionElement::updateExtenedMarker()
{
    if (m_selectedness) {
        m_showMarker = true;
        if (getMarkerStr() && !m_marker) {
            Node* nd = createNodeWithHTML(getMarkerStr());

            // If the content of g_markerStr is changed, this part must also be
            // changed.
            m_marker = nd->firstChild();
        }
        appendChild(m_marker);
    } else {
        if (m_marker && m_showMarker) {
            removeChild(m_marker);
        }
        m_showMarker = false;
    }
}

void* HTMLOptionElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(HTMLOptionElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLOptionElement)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLOptionElement, m_marker));
        HTMLFormControl::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLOptionElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
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
        select->resetFromOption(this);
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

// https://html.spec.whatwg.org/multipage/form-elements.html#concept-option-selectedness
bool HTMLOptionElement::selectedness()
{
    return m_selectedness;
}

void HTMLOptionElement::setSelectedness(bool selectedness)
{
    m_selectedness = selectedness;
    updateExtenedMarker();
}

// The value IDL attribute, on getting, must return the element's value.
// On setting, the element's value content attribute must be set to the new
// value.
// https://html.spec.whatwg.org/multipage/form-elements.html#dom-option-value
String* HTMLOptionElement::value()
{
    String* val = getAttributeOrEmpty(starfish()->staticStrings()->m_value);
    if (val->equals(String::emptyString)) {
        return text();
    }
    return val;
}

void HTMLOptionElement::setValue(String* value)
{
    setAttribute(starfish()->staticStrings()->m_value, value);
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

String* HTMLOptionElement::label()
{
    String* val = getAttributeOrEmpty(starfish()->staticStrings()->m_label);
    if (val->isEmpty()) {
        return text();
    }
    return val;
}

void HTMLOptionElement::setLabel(String* value)
{
    String* old = getAttributeOrEmpty(starfish()->staticStrings()->m_label);
    if (!old->equals(value)) {
        setAttribute(starfish()->staticStrings()->m_label, value);
    }
}

bool HTMLOptionElement::hasLabelOrText()
{
    if (label()->isEmpty()) {
        return false;
    }
    return true;
}

int HTMLOptionElement::index()
{
    HTMLSelectElement* select = selectElement();
    if (!select) {
        return 0;
    }

    for (unsigned index = 0; index < select->length(); index++) {
        if (this == select->item(index))
            return index;
    }

    return 0;
}

bool HTMLOptionElement::defaultSelected()
{
    Nullable<String*> val =
        getAttribute(starfish()->staticStrings()->m_selected);
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

// 1:
// https://html.spec.whatwg.org/multipage/form-elements.html#concept-option-selectedness
// 2: The selected IDL attribute, on getting, must return true if the element's
// selectedness is true, and false otherwise. On setting, it must set the
// element's selectedness to the new value, set its dirtiness to true, and then
// cause the element to ask for a reset.
void HTMLOptionElement::didAttributeChanged(QualifiedName name,
                                            Nullable<String*> old, String* val,
                                            bool attributeCreated,
                                            bool attributeRemoved)
{
    HTMLFormControl::didAttributeChanged(name, old, val, attributeCreated,
                                         attributeRemoved);

    if (name == starfish()->staticStrings()->m_selected) {
        if (!m_dirtiness) {
            // Adding `selected` attribute affects selectness only when its
            // dirtiness flag is false.
            if (attributeCreated) {
                setSelectedness(true);
            } else if (attributeRemoved) {
                setSelectedness(false);
            }

            HTMLSelectElement* select = selectElement();
            if (select) {
                select->resetFromOption(this);
            }
        }
    } else if (name == starfish()->staticStrings()->m_label) {
        if (attributeCreated || !old->equals(val)) {
            setNeedsFrameTreeBuild();
        }
    }
}

void HTMLOptionElement::didNodeInserted(Node* parent, Node* newChild)
{
    HTMLElement::didNodeInserted(parent, newChild);
    if (newChild && newChild->isText()) {
        if (m_showMarker) {
            removeChild(m_marker);
            appendChild(m_marker);
        }
    }
}

} // namespace Starfish
