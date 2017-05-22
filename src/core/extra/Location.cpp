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
#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/extra/Location.h"
#include "core/util/URL.h"
#include "core/modules/window/Window.h"

namespace StarFish {

URL* Location::url()
{
    return m_starFish->window()->document()->documentURI();
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
    url()->setHost(newHost);
    setLocation(url()->urlString());
}

void Location::setHostname(String* newHostname)
{
    url()->setHostname(newHostname);
    setLocation(url()->urlString());
}

void Location::setProtocol(String* newProtocol)
{
    url()->setProtocol(newProtocol);
    setLocation(url()->urlString());
}

void Location::setPathname(String* newPath, bool needRemovingDots)
{
    url()->setPathname(newPath, needRemovingDots);
    setLocation(url()->urlString());
}

void Location::setSearch(String* search)
{
    url()->setSearch(search);
    setLocation(url()->urlString());
}

void Location::setHash(String* search)
{
    url()->setHash(search);
    setLocation(url()->urlString());
}

void Location::setLocation(String* newURL)
{
    m_starFish->window()->navigateAsync(URL::createURL(
        m_starFish->window()->document()->documentURI()->urlString(), newURL));
}
}
