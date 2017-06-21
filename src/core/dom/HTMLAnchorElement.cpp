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

#include "core/dom/HTMLAnchorElement.h"

#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/dom/Event.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"

namespace StarFish {
String* HTMLAnchorElement::localName()
{
    return starFish()->staticStrings()->m_aTagName.localName();
}

QualifiedName HTMLAnchorElement::name()
{
    return starFish()->staticStrings()->m_aTagName;
}

void HTMLAnchorElement::handleDefaultEvent(Event* event)
{
    if (((event->isMouseEvent() || event->isTouchEvent())) &&
        event->type()->equals("click")) {
        auto href = starFish()->staticStrings()->m_href;
        Nullable<String*> hrefAttr = getAttribute(href);
        if (hrefAttr.hasValue()) {
            String* hrefStr = hrefAttr.getValue()->trim();
            if (hrefStr->length()) {
                if (hrefStr->startsWith("#")) {
                    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
                } else {
                    window()->browsingContext()->navigateAsync(
                        new ResourceURL(hrefStr, document()->urlString()));
                }
            } else {
                window()->browsingContext()->navigateAsync(
                    document()->documentURI());
            }
        }
    }
}

bool HTMLAnchorElement::supportsFocus()
{
    auto href = starFish()->staticStrings()->m_href;
    return hasAttribute(href) != SIZE_MAX ? true : false;
}
}
