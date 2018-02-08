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

#include "core/dom/HTMLButtonElement.h"

#include "core/dom/Document.h"
#include "core/dom/Event.h"
#include "core/page/BrowsingContext.h"

namespace StarFish {

HTMLButtonElement::HTMLButtonElement(Document* document)
    : HTMLFormControl(document)
{
    setAttribute(starFish()->staticStrings()->m_name, String::emptyString);
}

void* HTMLButtonElement::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLButtonElement)] = { 0 };
        HTMLFormControl::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLButtonElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

String* HTMLButtonElement::localName()
{
    return starFish()->staticStrings()->m_buttonTagName.localName();
}

QualifiedName HTMLButtonElement::name()
{
    return starFish()->staticStrings()->m_buttonTagName;
}

String* HTMLButtonElement::type()
{
    String* typeAttr = getAttributeOrEmpty(starFish()->staticStrings()->m_type);
    typeAttr = typeAttr->toASCIILower();

    if (typeAttr->equals("submit")) {
        return typeAttr;
    } else if (typeAttr->equals("reset")) {
        return typeAttr;
    } else if (typeAttr->equals("button")) {
        return typeAttr;
    }

    return starFish()->staticStrings()->m_submit.localName();
}

bool HTMLButtonElement::handleDefaultEvent(Event* event)
{
    if (HTMLElement::handleDefaultEvent(event)) {
        return true;
    }

    if (event->isMouseEvent() || event->isTouchEvent()) {
        if (event->type()->equalsIgnoreCase("click")) {
            if (formOwner()) {
                if (type()->equals("submit")) {
                    fireSubmitEvent();
                } else if (type()->equals("reset")) {
                    // TODO
                }
                return true;
            }
        }
    }

    return false;
}
}
