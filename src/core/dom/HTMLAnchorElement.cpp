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
#include "core/page/BrowsingContext.h"
#include "core/dom/Document.h"
#include "core/dom/Event.h"
#include "core/page/Location.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"

#include "browser/history/HistoryManager.h"

namespace StarFish {

QualifiedName HTMLAnchorElement::name()
{
    return starFish()->staticStrings()->m_aTagName;
}

bool HTMLAnchorElement::handleDefaultEvent(Event* event)
{
    if (HTMLElement::handleDefaultEvent(event)) {
        return true;
    }

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
                    window()->location()->setHref(hrefStr);
                }
            } else {
                window()->location()->setHref(
                    document()->documentURI()->urlString());
            }
            return true;
        }
    }
    return false;
}

bool HTMLAnchorElement::supportsFocus() const
{
    auto href = starFish()->staticStrings()->m_href;
    return const_cast<HTMLAnchorElement*>(this)->hasAttribute(href) != SIZE_MAX
               ? true
               : false;
}

String* HTMLAnchorElement::href()
{
    Nullable<String*> hrefAttr =
        getAttribute(starFish()->staticStrings()->m_href);
    if (hrefAttr.hasValue()) {
        return (new ResourceURL(hrefAttr.getValue()->trim(),
                                document()->urlString()))
            ->urlString();
    }
    return String::emptyString;
}

void HTMLAnchorElement::setHref(String* href)
{
    setAttribute(starFish()->staticStrings()->m_href, href);
}
}
