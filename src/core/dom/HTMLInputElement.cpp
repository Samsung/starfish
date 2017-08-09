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

#include "core/dom/HTMLInputElement.h"

#include "core/dom/Event.h"
#include "core/dom/HTMLFormElement.h"

namespace StarFish {

HTMLInputElement::HTMLInputElement(Document* document)
    : HTMLElement(document)
{
    setAttribute(starFish()->staticStrings()->m_name, String::emptyString);
    setTabIndex(0, false);
}

QualifiedName HTMLInputElement::name()
{
    return starFish()->staticStrings()->m_inputTagName;
}

String* HTMLInputElement::domName()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_name);
}

void HTMLInputElement::setDomName(String* name)
{
    setAttribute(starFish()->staticStrings()->m_name, name);
}

String* HTMLInputElement::type()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_type);
}

void HTMLInputElement::setType(String* type)
{
    setAttribute(starFish()->staticStrings()->m_type, type);
}

String* HTMLInputElement::value()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_value);
}

void HTMLInputElement::setValue(String* value)
{
    setAttribute(starFish()->staticStrings()->m_value, value);
}

String* HTMLInputElement::formEnctype()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_formEnctype);
}

void HTMLInputElement::setFormEnctype(String* enctype)
{
    setAttribute(starFish()->staticStrings()->m_formEnctype, enctype);
}

String* HTMLInputElement::formMethod()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_formMethod);
}

void HTMLInputElement::setFormMethod(String* method)
{
    setAttribute(starFish()->staticStrings()->m_formMethod, method);
}

String* HTMLInputElement::formTarget()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_formTarget);
}

void HTMLInputElement::setFormTarget(String* target)
{
    setAttribute(starFish()->staticStrings()->m_formTarget, target);
}

String* HTMLInputElement::formAction()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_formAction);
}

void HTMLInputElement::setFormAction(String* formAction)
{
    setAttribute(starFish()->staticStrings()->m_formAction, formAction);
}

HTMLFormElement* HTMLInputElement::form()
{
    for (Node* p = parentNode(); p; p = p->parentNode()) {
        if (p == nullptr) {
            break;
        } else if (p->isHTMLIFrameElement()) {
            return nullptr;
        } else if (p->isHTMLFormElement()) {
            return p->asHTMLFormElement();
        }
    }
    return nullptr;
}

bool HTMLInputElement::handleDefaultEvent(Event* event)
{
    if (HTMLElement::handleDefaultEvent(event)) {
        return true;
    }

    if (((event->isMouseEvent() || event->isTouchEvent())) &&
        event->type()->equalsWithoutCase("click")) {
        if (type()->equalsWithoutCase("submit") ||
            type()->equalsWithoutCase("button")) {
            HTMLFormElement* formNode = form();
            if (formNode) {
                formNode->setSubmitter(this);
                formNode->submit();
                return true;
            }
        }
    }
    return false;
}

bool HTMLInputElement::supportsFocus() const
{
    auto type = starFish()->staticStrings()->m_type;
    return !const_cast<HTMLInputElement*>(this)
                ->getAttributeOrEmpty(type)
                ->equalsWithoutCase("hidden");
}
}
