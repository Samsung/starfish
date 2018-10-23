/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"
#include "Starfish.h"
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

namespace Starfish {

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
    setLocation(newURL, document()->documentURI());
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
            document()->getElementById(str->substring(1, str->length() - 1));
        if (e) {
            e->scrollIntoView();
        }
    }
}

void Location::setLocation(String* url, ResourceURL* referrerURL)
{
    assign(url, referrerURL);
}

void Location::assign(String* url)
{
    ResourceURL* r = new ResourceURL(url, document()->baseURL()->urlString());
    assign(r, document()->documentURI());
}

void Location::assign(String* url, ResourceURL* referrerURL)
{
    ResourceURL* r = new ResourceURL(url, document()->baseURL()->urlString());
    assign(r, referrerURL);
}

static void navigateImpl(BrowsingContext* ctx, ResourceURL* url,
                         ResourceURL* referrerURL, HistoryManagerAction action)
{
    if (ctx->isTopLevelBrowsingContext()) {
        ctx->webView()->messageLoop()->invokeNavigate(ctx->webView(), url,
                                                      referrerURL, action);
    } else {
        ctx->sourceElement()->navigate(url, action, referrerURL);
    }
}

void Location::assign(ResourceURL* url, bool force)
{
    assign(url, document()->documentURI(), force);
}

class HeaderResourceClient : public ResourceClient {
public:
    HeaderResourceClient(Location* location, ResourceURL* url,
                         ResourceURL* referrerURL, bool force, Resource* res)
        : ResourceClient(res)
        , m_location(location)
        , m_url(url)
        , m_referrerURL(referrerURL)
        , m_force(force)
    {
    }

    // https://html.spec.whatwg.org/multipage/browsing-the-web.html#process-a-navigate-response
    virtual void didHeaderReceived(
        const std::unordered_map<std::string, std::string>& headers) override
    {
        auto itr = headers.find("Content-type");
        if (itr != headers.end()) {
            String* mimetype = String::createASCIIString(itr->second.c_str());
            if (mimetype->contains("application", false)) {
                checkHeader(false, headers);
                return;
            }
        }

        itr = headers.find("Content-Disposition");
        if (itr != headers.end()) {
            String* contentDisposition =
                String::createASCIIString(itr->second.c_str());
            if (contentDisposition->contains("attachment")) {
                checkHeader(false, headers);
                return;
            }
        }

        checkHeader(true, headers);
    }

private:
    void checkHeader(
        bool isBrowsableContent,
        const std::unordered_map<std::string, std::string>& headers)
    {
        bool canNavigate = isBrowsableContent &&
                           (m_url->protocolKind() != ResourceURL::UNKNOWN);

        if (m_location->webView()->containsPublicWebViewHandler(
                ShouldOverrideUrlLoading)) {
            struct Param : public gc {
                ResourceURL* url;
                ResourceURL* referrerUrl;
                bool canNavigate;
                bool force;
            };
            Param* p = new Param();
            p->url = m_url;
            p->referrerUrl = m_referrerURL;
            p->canNavigate = canNavigate;
            p->force = m_force;
            m_location->webView()->callPublicWebViewHandler(
                ShouldOverrideUrlLoading, (void*)p);
        } else {
            if (canNavigate) {
                if (m_force ||
                    !m_location->url()->urlString()->equals(
                        m_url->urlString())) {
                    navigateImpl(m_location->document()->browsingContext(),
                                 m_url, m_referrerURL,
                                 HistoryManagerAction::Add);
                }
            }
        }

        if (!isBrowsableContent &&
            m_location->webView()->containsPublicWebViewHandler(
                OnDownloadStart)) {
            struct Param {
                std::string url;
                std::string userAgent;
                std::string contentDisposition;
                std::string mimetype;
                long contentLength;
            };

            Param* p = new Param();
            p->url = m_url->urlString()->toUTF8NonGCString();
            p->userAgent = m_location->document()
                               ->webView()
                               ->userAgent()
                               ->toUTF8NonGCString();

            auto it = headers.find(HTTPHeaderMap::kContentDispoition);
            if (it != headers.end()) {
                p->contentDisposition = it->second;
            } else {
                p->contentDisposition = "";
            }

            it = headers.find(HTTPHeaderMap::kContentType);
            if (it != headers.end()) {
                p->mimetype = it->second;
            } else {
                p->mimetype = "";
            }

            it = headers.find(HTTPHeaderMap::kContentLength);
            if (it != headers.end()) {
                p->contentLength = atol(it->second.c_str());
            } else {
                p->contentLength = -1;
            }
            m_location->document()->webView()->callPublicWebViewHandler(
                OnDownloadStart, (void*)p);
        }
    }

private:
    Location* m_location;
    ResourceURL* m_url;
    ResourceURL* m_referrerURL;
    bool m_force;
};

void Location::assign(ResourceURL* url, ResourceURL* referrerURL, bool force)
{
    // check whether the resource can be displayed
    HeaderResource* resource = document()->resourceLoader().fetchHeader(url);
    resource->addResourceClient(
        new HeaderResourceClient(this, url, referrerURL, force, resource));
    resource->setNavigationResoure(true);
    resource->request(Resource::ResourceRequestSyncLevel::NeverSync,
                      referrerURL, true, MethodType::HEAD);
}

void Location::replace(String* url)
{
    if (ResourceURL::isValidURL(url)) {
        navigateImpl(document()->browsingContext(), new ResourceURL(url),
                     document()->documentURI(), HistoryManagerAction::Replace);
    }
}

void Location::reload(bool force)
{
    ResourceURL* newUrl = new ResourceURL(*url());
    assign(newUrl, force);
}
}
