/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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
#include "core/dom/Document.h"
#include "core/dom/HTMLHyperlinkContainer.h"

namespace StarFish {
String* HTMLHyperlinkContainer::href()
{
    Nullable<String*> hrefAttr =
        getAttribute(starFish()->staticStrings()->m_href);
    if (hrefAttr.hasValue()) {
        return (new ResourceURL(hrefAttr.getValue()->trim(),
                                document()->baseURL()->urlString()))
            ->urlString();
    }
    return String::emptyString;
}

void HTMLHyperlinkContainer::setHref(String* href)
{
    setAttribute(starFish()->staticStrings()->m_href, href);
}

String* HTMLHyperlinkContainer::host()
{
    Nullable<String*> hrefAttr =
        getAttribute(starFish()->staticStrings()->m_href);
    if (hrefAttr.hasValue()) {
        return (new ResourceURL(hrefAttr.getValue()->trim(),
                                document()->baseURL()->urlString()))
            ->host();
    }
    return String::emptyString;
}

void HTMLHyperlinkContainer::setHost(String* host)
{
    Nullable<String*> hrefAttr =
        getAttribute(starFish()->staticStrings()->m_href);
    if (!hrefAttr.hasValue()) {
        return;
    }
    ResourceURL* resourceURL = new ResourceURL(
        hrefAttr.getValue()->trim(), document()->baseURL()->urlString());
    resourceURL = resourceURL->setHost(host);
    setAttribute(starFish()->staticStrings()->m_href, resourceURL->href());
}

String* HTMLHyperlinkContainer::hostname()
{
    Nullable<String*> hrefAttr =
        getAttribute(starFish()->staticStrings()->m_href);
    if (hrefAttr.hasValue()) {
        return (new ResourceURL(hrefAttr.getValue()->trim(),
                                document()->baseURL()->urlString()))
            ->hostname();
    }
    return String::emptyString;
}

void HTMLHyperlinkContainer::setHostname(String* hostname)
{
    Nullable<String*> hrefAttr =
        getAttribute(starFish()->staticStrings()->m_href);
    if (!hrefAttr.hasValue()) {
        return;
    }
    ResourceURL* resourceURL = new ResourceURL(
        hrefAttr.getValue()->trim(), document()->baseURL()->urlString());
    resourceURL = resourceURL->setHostname(hostname);
    setAttribute(starFish()->staticStrings()->m_href, resourceURL->href());
}

String* HTMLHyperlinkContainer::pathname()
{
    Nullable<String*> hrefAttr =
        getAttribute(starFish()->staticStrings()->m_href);
    if (hrefAttr.hasValue()) {
        return (new ResourceURL(hrefAttr.getValue()->trim(),
                                document()->baseURL()->urlString()))
            ->pathname();
    }
    return String::emptyString;
}

void HTMLHyperlinkContainer::setPathname(String* path)
{
    Nullable<String*> hrefAttr =
        getAttribute(starFish()->staticStrings()->m_href);
    if (!hrefAttr.hasValue()) {
        return;
    }
    ResourceURL* resourceURL = new ResourceURL(
        hrefAttr.getValue()->trim(), document()->baseURL()->urlString());
    resourceURL = resourceURL->setPathname(path);
    setAttribute(starFish()->staticStrings()->m_href, resourceURL->href());
}

String* HTMLHyperlinkContainer::port()
{
    Nullable<String*> hrefAttr =
        getAttribute(starFish()->staticStrings()->m_href);
    if (hrefAttr.hasValue()) {
        return (new ResourceURL(hrefAttr.getValue()->trim(),
                                document()->baseURL()->urlString()))
            ->port();
    }
    return String::emptyString;
}

void HTMLHyperlinkContainer::setPort(String* port)
{
    Nullable<String*> hrefAttr =
        getAttribute(starFish()->staticStrings()->m_href);
    if (!hrefAttr.hasValue()) {
        return;
    }
    ResourceURL* resourceURL = new ResourceURL(
        hrefAttr.getValue()->trim(), document()->baseURL()->urlString());
    resourceURL = resourceURL->setPort(port);
    setAttribute(starFish()->staticStrings()->m_href, resourceURL->href());
}

String* HTMLHyperlinkContainer::protocol()
{
    Nullable<String*> hrefAttr =
        getAttribute(starFish()->staticStrings()->m_href);
    if (hrefAttr.hasValue()) {
        return (new ResourceURL(hrefAttr.getValue()->trim(),
                                document()->baseURL()->urlString()))
            ->protocol();
    }
    return String::emptyString;
}

void HTMLHyperlinkContainer::setProtocol(String* protocol)
{
    Nullable<String*> hrefAttr =
        getAttribute(starFish()->staticStrings()->m_href);
    if (!hrefAttr.hasValue()) {
        return;
    }
    ResourceURL* resourceURL = new ResourceURL(
        hrefAttr.getValue()->trim(), document()->baseURL()->baseURI());
    resourceURL = resourceURL->setProtocol(protocol);
    setAttribute(starFish()->staticStrings()->m_href, resourceURL->href());
}

String* HTMLHyperlinkContainer::target()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_target);
}

void HTMLHyperlinkContainer::setTarget(String* target)
{
    setAttribute(starFish()->staticStrings()->m_target, target);
}
}
