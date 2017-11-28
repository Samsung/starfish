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
#include "core/dom/DOMException.h"
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

void HTMLAnchorElement::didAttributeChanged(QualifiedName name, String* old,
                                            String* val, bool attributeCreated,
                                            bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, val, attributeCreated,
                                     attributeRemoved);

    if (name == starFish()->staticStrings()->m_tabindex) {
        m_tabIndexWasSetExplicitly = true;
        if (m_tabIndex == -1)
            m_tabIndex = 0;
    }
}

bool HTMLAnchorElement::handleDefaultEvent(Event* event)
{
    if (HTMLElement::handleDefaultEvent(event)) {
        return true;
    }
    // TODO : Apply noreferrer
    if (event->type()->equals("click")) {
        auto href = starFish()->staticStrings()->m_href;
        Nullable<String*> hrefAttr = getAttribute(href);
        if (hrefAttr.hasValue()) {
            String* hrefStr = hrefAttr.getValue()->trim();
            if (hrefStr->length()) {
                if (hrefStr->startsWith("#")) {
                    window()->location()->setHash(hrefStr);
                } else if (hrefStr->startsWith("javascript:", false)) {
                    String* ret2 = toBrowserString(
                        window()->scriptBindingInstance(),
                        evaluateString(
                            window()->scriptBindingInstance(),
                            hrefStr->substring(11, hrefStr->length() - 11)));
                    if (!ret2->equalsIgnoreCase("undefined")) {
                        try {
                            GCVector<String*> value0;
                            value0.push_back(ret2);
                            document()->write(value0);
                        } catch (DOMException* e) {
                            // TODO: should throw the exception into onError
                            // event handler
                            STARFISH_RELEASE_ASSERT_NOT_REACHED();
                        }
                    }
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

String* HTMLAnchorElement::host()
{
    Nullable<String*> hrefAttr =
        getAttribute(starFish()->staticStrings()->m_href);
    if (hrefAttr.hasValue()) {
        return (new ResourceURL(hrefAttr.getValue()->trim(),
                                document()->urlString()))
            ->host();
    }
    return String::emptyString;
}

void HTMLAnchorElement::setHost(String* host)
{
    Nullable<String*> hrefAttr =
        getAttribute(starFish()->staticStrings()->m_href);
    if (!hrefAttr.hasValue()) {
        return;
    }
    ResourceURL* resourceURL =
        new ResourceURL(hrefAttr.getValue()->trim(), document()->urlString());
    resourceURL = resourceURL->setHost(host);
    setAttribute(starFish()->staticStrings()->m_href, resourceURL->href());
}

String* HTMLAnchorElement::pathname()
{
    Nullable<String*> hrefAttr =
        getAttribute(starFish()->staticStrings()->m_href);
    if (hrefAttr.hasValue()) {
        return (new ResourceURL(hrefAttr.getValue()->trim(),
                                document()->urlString()))
            ->pathname();
    }
    return String::emptyString;
}

void HTMLAnchorElement::setPathname(String* path)
{
    Nullable<String*> hrefAttr =
        getAttribute(starFish()->staticStrings()->m_href);
    if (!hrefAttr.hasValue()) {
        return;
    }
    ResourceURL* resourceURL =
        new ResourceURL(hrefAttr.getValue()->trim(), document()->urlString());
    resourceURL = resourceURL->setPathname(path);
    setAttribute(starFish()->staticStrings()->m_href, resourceURL->href());
}

String* HTMLAnchorElement::protocol()
{
    Nullable<String*> hrefAttr =
        getAttribute(starFish()->staticStrings()->m_href);
    if (hrefAttr.hasValue()) {
        return (new ResourceURL(hrefAttr.getValue()->trim(),
                                document()->urlString()))
            ->protocol();
    }
    return String::emptyString;
}

void HTMLAnchorElement::setProtocol(String* protocol)
{
    Nullable<String*> hrefAttr =
        getAttribute(starFish()->staticStrings()->m_href);
    if (!hrefAttr.hasValue()) {
        return;
    }
    ResourceURL* resourceURL =
        new ResourceURL(hrefAttr.getValue()->trim(), document()->urlString());
    resourceURL = resourceURL->setProtocol(protocol);
    setAttribute(starFish()->staticStrings()->m_href, resourceURL->href());
}

String* HTMLAnchorElement::target()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_target);
}

void HTMLAnchorElement::setTarget(String* target)
{
    setAttribute(starFish()->staticStrings()->m_target, target);
}
}
