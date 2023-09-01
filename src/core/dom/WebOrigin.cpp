/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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
#include "core/dom/WebOrigin.h"
#include "platform/loader/ResourceURL.h"

namespace Starfish {
WebOrigin::WebOrigin()
    : WebOrigin(nullptr, true)
{
}

WebOrigin::WebOrigin(ResourceURL* url, bool isOpaque)
    : m_originalURL(url)
    , m_isOpaque(isOpaque)
{
}

WebOrigin* WebOrigin::createDocumentOrigin(ResourceURL* url)
{
    if (url && url->isHTTPFamilyURL()) {
        return new WebOrigin(url, false);
    }
    return new WebOrigin(url, true);
}

// https://html.spec.whatwg.org/multipage/origin.html#ascii-serialisation-of-an-origin
String* WebOrigin::serialize() const
{
    if (isOpaque()) {
        return String::createASCIIString("null");
    }
    // m_originalURL should not be null unless isOpaque is true
    STARFISH_ASSERT(m_originalURL);
    return m_originalURL->origin();
}

Nullable<String*> WebOrigin::domain() const
{
    if (isOpaque()) {
        return nullptr;
    }
    // m_originalURL should not be null unless isOpaque is true
    STARFISH_ASSERT(m_originalURL);

    String* domain = m_originalURL->domain();

    if (domain != String::emptyString) {
        return domain;
    }

    // TODO domain can be changed through the document.domain API.
    // If origin's domain is non-null, then return origin's domain.

    return m_originalURL->host();
}

// https://w3c.github.io/html/browsers.html#same-origin
bool WebOrigin::isSameOrigin(const WebOrigin* otherWebOrigin) const
{
    if (this == otherWebOrigin) {
        return true;
    }

    if ((isOpaque()) && (otherWebOrigin->isOpaque())) {
        return true;
    }

    if ((isOpaque()) || (otherWebOrigin->isOpaque())) {
        return false;
    }

    // m_originalURL should not be null unless isOpaque is true
    STARFISH_ASSERT(m_originalURL);
    STARFISH_ASSERT(otherWebOrigin->m_originalURL);

    if ((m_originalURL->protocol()->equals(
            otherWebOrigin->m_originalURL->protocol())) &&
        (m_originalURL->host()->equals(otherWebOrigin->m_originalURL->host()) ==
         true) &&
        (m_originalURL->port()->equals(otherWebOrigin->m_originalURL->port()) ==
         true)) {
        return true;
    }

    return false;
}

// https://w3c.github.io/html/browsers.html#same-origin-domain
bool WebOrigin::isSameOriginDomain(const WebOrigin* otherWebOrigin) const
{
    if (this == otherWebOrigin) {
        return true;
    }

    if (isOpaque() || otherWebOrigin->isOpaque()) {
        return false;
    }

    // m_originalURL should not be null unless isOpaque is true
    STARFISH_ASSERT(m_originalURL);
    STARFISH_ASSERT(otherWebOrigin->m_originalURL);

    if ((m_originalURL->protocol()->equals(
            otherWebOrigin->m_originalURL->protocol())) &&
        (domain().hasValue()) && (otherWebOrigin->domain().hasValue()) &&
        (domain().getValue()->equals(otherWebOrigin->domain().getValue()) ==
         true)) {
        return true;
    } else if ((isSameOrigin(otherWebOrigin)) && (!domain().hasValue()) &&
               (!otherWebOrigin->domain().hasValue())) {
        return true;
    }

    return false;
}

// https://html.spec.whatwg.org/multipage/nav-history-apis.html#can-have-its-url-rewritten
bool WebOrigin::canRewritten(const WebOrigin* targetWebOrigin) const
{
    if (!targetWebOrigin->m_originalURL.hasValue()) {
        return false;
    }

    if (!isSameOrigin(targetWebOrigin)) {
        return false;
    }

    ResourceURL* targetURL = targetWebOrigin->m_originalURL.value();
    if ((m_originalURL->protocolKind() != targetURL->protocolKind()) ||
        !m_originalURL->username()->equals(targetURL->username()) ||
        !m_originalURL->password()->equals(targetURL->password()) ||
        !m_originalURL->host()->equals(targetURL->host()) ||
        !m_originalURL->port()->equals(targetURL->port())) {
        return false;
    }

    if (targetURL->isHTTPFamilyURL()) {
        return true;
    }

    if (targetURL->isFileURL()) {
        if (!m_originalURL->pathname()->equals(targetURL->pathname())) {
            return false;
        }
    }

    if (!m_originalURL->pathname()->equals(targetURL->pathname()) ||
        !m_originalURL->search()->equals(targetURL->search())) {
        if (!targetURL->isFileURL()) {
            return false;
        }
    }

    return true;
}

} // namespace Starfish
