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

WebOrigin::WebOrigin(NULLABLE ResourceURL* url, bool isOpaque)
    : m_originalURL(url)
    , m_isOpaque(isOpaque)
{
}

WebOrigin* WebOrigin::createDocumentOrigin(ResourceURL* url)
{
    if (url != nullptr && url->isHTTPFamilyURL() == true) {
        return new WebOrigin(url, false);
    }
    return new WebOrigin();
}

// https://html.spec.whatwg.org/multipage/origin.html#ascii-serialisation-of-an-origin
String* WebOrigin::serialize() const
{
    if (isOpaque() == true) {
        return String::createASCIIString("null");
    }
    // m_originalURL should not be null unless isOpaque is true
    STARFISH_ASSERT(m_originalURL != nullptr);
    return m_originalURL->origin();
}

Nullable<String*> WebOrigin::domain() const
{
    if (isOpaque() == true) {
        return nullptr;
    }
    // m_originalURL should not be null unless isOpaque is true
    STARFISH_ASSERT(m_originalURL != nullptr);

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

    if ((isOpaque() == true) && (otherWebOrigin->isOpaque() == true)) {
        return true;
    }

    if ((isOpaque() == true) || (otherWebOrigin->isOpaque() == true)) {
        return false;
    }

    // m_originalURL should not be null unless isOpaque is true
    STARFISH_ASSERT(m_originalURL);
    STARFISH_ASSERT(otherWebOrigin->m_originalURL);

    if ((m_originalURL->protocol()->equals(
             otherWebOrigin->m_originalURL->protocol()) == true) &&
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

    if ((isOpaque() == true) && (otherWebOrigin->isOpaque() == true)) {
        return true;
    }

    if (isOpaque() == true || otherWebOrigin->isOpaque() == true) {
        return false;
    }

    // m_originalURL should not be null unless isOpaque is true
    STARFISH_ASSERT(m_originalURL);
    STARFISH_ASSERT(otherWebOrigin->m_originalURL);

    if ((m_originalURL->protocol()->equals(
             otherWebOrigin->m_originalURL->protocol()) == true) &&
        (domain().hasValue() == true) &&
        (otherWebOrigin->domain().hasValue() == true) &&
        (domain().getValue()->equals(otherWebOrigin->domain().getValue()) ==
         true)) {
        return true;
    } else if ((isSameOrigin(otherWebOrigin) == true) &&
               (domain().hasValue() == false) &&
               (otherWebOrigin->domain().hasValue() == false)) {
        return true;
    }

    return false;
}
} // namespace Starfish
