/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#include "core/dom/Document.h"
#include "core/page/Location.h"
#include "core/util/URL.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "platform/window/PlatformWindow.h"
#include "WebView.h"

namespace StarFish {

ResourceURL* Location::url()
{
    return document()->documentURI();
}

String* Location::href()
{
    return url()->href();
}

String* Location::host()
{
    String* hostname = url()->hostname();
    String* port = url()->port();
    if (!port->equals(String::emptyString) &&
        !hostname->equals(String::emptyString)) {
        return (hostname->concat(String::fromUTF8(":")))->concat(port);
    } else {
        return hostname;
    }
}

String* Location::hostname()
{
    return url()->hostname();
}

String* Location::protocol()
{
    return url()->protocol();
}

String* Location::pathname()
{
    return url()->pathname();
}

String* Location::search()
{
    return url()->search();
}

String* Location::hash()
{
    return url()->hash();
}

void Location::setHref(String* newURL)
{
    setLocation(newURL);
}

void Location::setHost(String* newHost)
{
    ResourceURL* newUrl = new ResourceURL(url()->urlString());
    newUrl->setHost(newHost);
    assign(newUrl);
}

void Location::setHostname(String* newHostname)
{
    ResourceURL* newUrl = new ResourceURL(url()->urlString());
    newUrl->setHostname(newHostname);
    assign(newUrl);
}

void Location::setProtocol(String* newProtocol)
{
    ResourceURL* newUrl = new ResourceURL(url()->urlString());
    newUrl->setProtocol(newProtocol);
    assign(newUrl);
}

void Location::setPathname(String* newPath, bool needRemovingDots)
{
    ResourceURL* newUrl = new ResourceURL(url()->urlString());
    newUrl->setPathname(newPath, needRemovingDots);
    assign(newUrl);
}

void Location::setSearch(String* search)
{
    ResourceURL* newUrl = new ResourceURL(url()->urlString());
    newUrl->setSearch(search);
    assign(newUrl);
}

void Location::setHash(String* search)
{
    ResourceURL* newUrl = new ResourceURL(url()->urlString());
    newUrl->setHash(search);
    assign(newUrl);
}

bool Location::isValidURL(String* url)
{
    if (url->startsWith("http://")) {
        return true;
    } else {
        return false;
    }
}

void Location::setLocation(String* url)
{
    if (isValidURL(url)) {
        assign(new ResourceURL(url));
    }
}

void Location::assign(String* url)
{
    if (isValidURL(url)) {
        assign(new ResourceURL(url));
    }
}

void Location::assign(ResourceURL* url)
{
    starFish()->platformWindow()->webView()->navigate(url);
}

void Location::replace(String* url)
{
    // TODO: Since history is not implemnted yet, replace() is the same as
    // assign for now
    if (isValidURL(url)) {
        assign(url);
    }
}

void Location::reload()
{
    ResourceURL* newUrl = new ResourceURL(url()->urlString());
    assign(newUrl);
}
}
