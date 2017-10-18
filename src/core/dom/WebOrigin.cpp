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
#include "core/dom/WebOrigin.h"
#include "platform/loader/ResourceURL.h"

namespace StarFish {
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
    if (url && (url->m_protocol == ResourceURL::HTTP_PROTOCOL ||
                url->m_protocol == ResourceURL::HTTPS_PROTOCOL)) {
        return new WebOrigin(url, false);
    }
    return new WebOrigin();
}

// https://html.spec.whatwg.org/multipage/origin.html#ascii-serialisation-of-an-origin
String* WebOrigin::serialize()
{
    if (isOpaque()) {
        return String::createASCIIString("null");
    }
    // m_originalURL should not be null unless isOpaque is true
    STARFISH_ASSERT(m_originalURL != nullptr);
    return m_originalURL->origin();
}

Nullable<String*> WebOrigin::domain()
{
    if (isOpaque()) {
        return nullptr;
    }
    // m_originalURL should not be null unless isOpaque is true
    STARFISH_ASSERT(m_originalURL != nullptr);

    // TODO domain can be changed through the document.domain API.
    // If origin's domain is non-null, then return origin's domain.

    return m_originalURL->host();
}

// https://w3c.github.io/html/browsers.html#same-origin
bool WebOrigin::isSameOrigin(WebOrigin* otherWebOrigin)
{
    // If A and B are the same opaque origin, then return true.
    // TODO: comparing for the same opaque origin is not supported yet
    if (isOpaque() || otherWebOrigin->isOpaque()) {
        return false;
    }

    // m_originalURL should not be null unless isOpaque is true
    STARFISH_ASSERT(m_originalURL);
    STARFISH_ASSERT(otherWebOrigin->m_originalURL);

    if (m_originalURL->protocol()->equals(
            otherWebOrigin->m_originalURL->protocol()) &&
        m_originalURL->host()->equals(otherWebOrigin->m_originalURL->host()) &&
        m_originalURL->port()->equals(otherWebOrigin->m_originalURL->port())) {
        return true;
    }

    return false;
}

// https://w3c.github.io/html/browsers.html#same-origin-domain
bool WebOrigin::isSameOriginDomain(WebOrigin* otherWebOrigin)
{
    // If A and B are the same opaque origin, then return true.
    // TODO: comparing for the same opaque origin is not supported yet
    if (isOpaque() || otherWebOrigin->isOpaque()) {
        return false;
    }

    // m_originalURL should not be null unless isOpaque is true
    STARFISH_ASSERT(m_originalURL);
    STARFISH_ASSERT(otherWebOrigin->m_originalURL);

    if (m_originalURL->protocol()->equals(
            otherWebOrigin->m_originalURL->protocol()) &&
        domain().hasValue() && otherWebOrigin->domain().hasValue() &&
        domain().getValue()->equals(otherWebOrigin->domain().getValue())) {
        return true;
    } else if (isSameOrigin(otherWebOrigin) && !domain().hasValue() &&
               !otherWebOrigin->domain().hasValue()) {
        return true;
    }

    return false;
}
}
