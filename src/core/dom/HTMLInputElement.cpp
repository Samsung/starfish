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

#include "core/dom/Document.h"
#include "core/dom/Event.h"
#include "core/dom/HTMLFormElement.h"
#include "core/dom/Node.h"
#include "core/dom/Traverse.h"

namespace StarFish {

HTMLInputElement::HTMLInputElement(Document* document)
    : HTMLElement(document)
{
    setAttribute(starFish()->staticStrings()->m_name, String::emptyString);
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

void HTMLInputElement::handleDefaultEvent(Event* event)
{
    if (((event->isMouseEvent() || event->isTouchEvent())) &&
        event->type()->equalsWithoutCase("click")) {
        if (type()->equalsWithoutCase("submit")) {
            GCVector<Element*> inputNodes;
            HTMLFormElement* formNode = form();
            if (formNode) {
                Traverse::collectDescendants(
                    inputNodes, formNode->asNode(),
                    [](Node* node) -> bool {
                        return node->isHTMLInputElement();
                    },
                    false);

                for (Element* node : inputNodes) {
                    STARFISH_ASSERT(node->isHTMLInputElement());
                    if (node->asHTMLInputElement()->type()->equalsWithoutCase(
                            "text")) {
                        // TODO: send (name, value) pairs to the POST module
                    }
                }
            }
        }
    }
}
}
