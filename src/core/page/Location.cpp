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
#include "core/page/WebView.h"
#include "core/dom/HTMLIFrameElement.h"
#include "core/dom/HTMLFormElement.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "platform/loader/ResourceURL.h"
#include "platform/window/PlatformWindow.h"

namespace StarFish {

ResourceURL* Location::url()
{
    return document()->documentURI();
}

String* Location::href()
{
    return url()->href();
}

String* Location::origin()
{
    return url()->origin();
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

String* Location::port()
{
    return url()->port();
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
    ResourceURL* newUrl = url()->setHost(newHost);
    assign(newUrl);
}

void Location::setHostname(String* newHostname)
{
    ResourceURL* newUrl = url()->setHostname(newHostname);
    assign(newUrl);
}

void Location::setPort(String* newPort)
{
    ResourceURL* newUrl = url()->setPort(newPort);
    assign(newUrl);
}

void Location::setProtocol(String* newProtocol)
{
    ResourceURL* newUrl = url()->setProtocol(newProtocol);
    assign(newUrl);
}

void Location::setPathname(String* newPath, bool needRemovingDots)
{
    ResourceURL* newUrl = url()->setPathname(newPath, needRemovingDots);
    assign(newUrl);
}

void Location::setSearch(String* search)
{
    ResourceURL* newUrl = new ResourceURL(*url());
    newUrl->setSearch(search);
    assign(newUrl);
}

void Location::setHash(String* search)
{
    ResourceURL* newUrl = url()->setHash(search);
    document()->setDocumentURI(newUrl);
    String* str = newUrl->hash();
    if (str->length() > 1) {
        Element* e =
            document()->getElementById(str->substring(1, str->length()));
        if (e) {
            e->scrollIntoView();
        }
    }
}

void Location::setLocation(String* url)
{
    assign(url);
}

void Location::assign(String* url)
{
    ResourceURL* r =
        new ResourceURL(url, document()->documentURI()->urlString());
    if (r->protocolKind() != ResourceURL::UNKNOWN) {
        assign(r);
    }
}

static void navigateImpl(BrowsingContext* ctx, ResourceURL* url)
{
    if (ctx->isTopLevelBrowsingContext()) {
        ctx->starFish()->messageLoop()->invokeNavigate(
            ctx->webView(), url, ctx->document()->documentURI());
    } else {
        ctx->window()->browsingContext()->navigate(
            url, HistoryManager::Action::Add, ctx->document()->documentURI());
    }
}

void Location::assign(ResourceURL* url, bool force)
{
    if (!url->isJavascriptURL()) {
        if (force || !this->url()->urlString()->equals(url->urlString())) {
            navigateImpl(document()->browsingContext(), url);
        }
    }
}

void Location::replace(String* url)
{
    if (ResourceURL::isValidURL(url)) {
        if (document()->browsingContext()->isTopLevelBrowsingContext()) {
            document()->browsingContext()->webView()->navigate(
                new ResourceURL(url), HistoryManager::Action::Replace,
                document()->documentURI());
        } else {
            document()->browsingContext()->sourceElement()->navigate(
                new ResourceURL(url), HistoryManager::Action::Replace,
                document()->documentURI());
        }
    }
}

void Location::reload()
{
    ResourceURL* newUrl = new ResourceURL(*url());
    assign(newUrl);
}
}
