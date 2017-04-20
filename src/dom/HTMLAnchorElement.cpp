/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

#include "StarFish.h"
#include "dom/Document.h"
#include "dom/Event.h"
#include "dom/HTMLAnchorElement.h"
#include "platform/window/Window.h"

namespace StarFish {
String* HTMLAnchorElement::localName()
{
    return document()
        ->window()
        ->starFish()
        ->staticStrings()
        ->m_aTagName.localName();
}

QualifiedName HTMLAnchorElement::name()
{
    return document()->window()->starFish()->staticStrings()->m_aTagName;
}

void HTMLAnchorElement::handleDefaultEvent(Event* event)
{
    if (((event->isMouseEvent() || event->isTouchEvent())) &&
        event->type()->equals("click")) {
        auto href = document()->window()->starFish()->staticStrings()->m_href;
        size_t s = hasAttribute(href);
        if (s != SIZE_MAX) {
            String* h = getAttribute(s)->trim();
            if (h->length()) {
                if (h->startsWith("#")) {
                    document()->window()->navigateAsync(URL::createURL(
                        document()->urlString()->substring(
                            0, document()->urlString()->indexOf('#')),
                        h));
                } else {
                    document()->window()->navigateAsync(
                        URL::createURL(document()->urlString(), h));
                }
            } else {
                document()->window()->navigateAsync(document()->documentURI());
            }
        }
    }
}

bool HTMLAnchorElement::supportsFocus()
{
    auto href = document()->window()->starFish()->staticStrings()->m_href;
    return hasAttribute(href) != SIZE_MAX ? true : false;
}
}
