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

#include "core/csp/ContentSecurityPolicyDirectiveList.h"
#include "core/csp/ContentSecurityPolicy.h"
#include "core/dom/Document.h"

namespace Starfish {
ContentSecurityPolicyDirectiveList::ContentSecurityPolicyDirectiveList(
    ContentSecurityPolicy* contentSecurityPolicy, String* policy, size_t begin,
    size_t end)
    : m_contentSecurityPolicy(contentSecurityPolicy)
    , m_contextURL(contentSecurityPolicy->document()->documentURI())
    , m_scriptSrc(nullptr)
    , m_imgSrc(nullptr)
    , m_styleSrc(nullptr)
    , m_connectSrc(nullptr)
    , m_frameSrc(nullptr)
{
    if (begin == end)
        return;

    size_t current = skipSpace(policy, begin, end);
    size_t directiveBegin = current;

    String* name = nullptr;
    String* value = nullptr;
    while (current <= end) {
        if (policy->charAt(current) == ' ') {
            if (!name && directiveBegin < current) {
                name =
                    policy->substring(directiveBegin, current - directiveBegin);
                directiveBegin = current + 1;
            }
        } else if (policy->charAt(current) == ';' || current == end) {
            if (directiveBegin < current && name != nullptr) {
                value =
                    policy->substring(directiveBegin, current - directiveBegin);
                addDirective(name, value);

                name = nullptr;
                value = nullptr;
                if (current + 1 <= end) {
                    directiveBegin = current =
                        skipSpace(policy, current + 1, end);
                    continue;
                }
            }
        }

        current++;
    }
}

void* ContentSecurityPolicyDirectiveList::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(ContentSecurityPolicyDirectiveList));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(ContentSecurityPolicyDirectiveList)] = {
            0
        };
        ContentSecurityPolicyDirectiveList::fillGCDescriptor(desc);
        descr = GC_make_descriptor(
            desc, GC_WORD_LEN(ContentSecurityPolicyDirectiveList));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void ContentSecurityPolicyDirectiveList::addDirective(String* name,
                                                      String* value)
{
    value = value->trim();
    if (name->equalsIgnoreCase("script-src")) {
        setDirective(m_scriptSrc, name, value);
    } else if (name->equalsIgnoreCase("img-src")) {
        setDirective(m_imgSrc, name, value);
    } else if (name->equalsIgnoreCase("style-src")) {
        setDirective(m_styleSrc, name, value);
    } else if (name->equalsIgnoreCase("connect-src")) {
        setDirective(m_connectSrc, name, value);
    } else if (name->equalsIgnoreCase("frame-src")) {
        setDirective(m_frameSrc, name, value);
    }
}

void ContentSecurityPolicyDirectiveList::setDirective(
    ContentSecurityPolicySourceListDirective*& directive, String* name,
    String* value)
{
    if (directive) {
        m_contentSecurityPolicy->dispatchViolationEvent(name);
        return;
    }
    directive = new ContentSecurityPolicySourceListDirective(this, name, value);
}

bool ContentSecurityPolicyDirectiveList::allowInline(
    ContentSecurityPolicySourceListDirective* sourceList)
{
    if (!sourceList) {
        return true;
    }
    return sourceList->allowInline();
}

ContentSecurityPolicySourceListDirective*
ContentSecurityPolicyDirectiveList::getSourceList(CSPDirectives directive)
{
    ContentSecurityPolicySourceListDirective* sourceList = nullptr;

    switch (directive) {
    case CSPDirectives::ScriptSrc:
        sourceList = m_scriptSrc;
        break;
    case CSPDirectives::StyleSrc:
        sourceList = m_styleSrc;
        break;
    case CSPDirectives::FrameSrc:
        sourceList = m_frameSrc;
        break;
    default:
        break;
    }
    return sourceList;
}

bool ContentSecurityPolicyDirectiveList::allowInline(CSPDirectives directive)
{
    auto sourceList = getSourceList(directive);

    if (sourceList) {
        return sourceList->allowInline();
    }
    return false;
}

bool ContentSecurityPolicyDirectiveList::allowURL(CSPDirectives directive,
                                                  ResourceURL* resUrl)
{
    auto sourceList = getSourceList(directive);

    if (sourceList) {
        return allowURL(sourceList, resUrl);
    }

    return false;
}

bool ContentSecurityPolicyDirectiveList::allowStar(CSPDirectives directive)
{
    auto sourceList = getSourceList(directive);

    if (sourceList->allowStar()) {
        return true;
    }

    return false;
}

bool ContentSecurityPolicyDirectiveList::allowSelf(CSPDirectives directive,
                                                   ResourceURL* resUrl)
{
    auto sourceList = getSourceList(directive);

    if (sourceList->allowSelf() &&
        m_contextURL->protocol()->equalsIgnoreCase(resUrl->protocol()) &&
        m_contextURL->host()->equalsIgnoreCase(resUrl->host())) {
        return true;
    }
    return false;
}

bool ContentSecurityPolicyDirectiveList::allowContent(CSPDirectives directive,
                                                      String* content)
{
    auto sourceList = getSourceList(directive);

    if (sourceList) {
        return sourceList->allowContent(content);
    }

    return false;
}

bool ContentSecurityPolicyDirectiveList::allowNonce(CSPDirectives directive,
                                                    String* nonce)
{
    auto sourceList = getSourceList(directive);

    if (sourceList) {
        return sourceList->allowNonce(nonce);
    }

    return false;
}

bool ContentSecurityPolicyDirectiveList::allowScheme(CSPDirectives directive,
                                                     ResourceURL* resUrl)
{
    auto sourceList = getSourceList(directive);

    if (sourceList) {
        return allowScheme(sourceList, resUrl);
    }

    return false;
}

bool ContentSecurityPolicyDirectiveList::allowScheme(
    ContentSecurityPolicySourceListDirective* sourceList, ResourceURL* resUrl)
{
    auto origin = resUrl->origin();

    if (origin->equalsIgnoreCase("null")) {
        auto protocol = resUrl->protocol();
        if (sourceList->allowScheme(protocol)) {
            return true;
        }
    }
    return false;
}

bool ContentSecurityPolicyDirectiveList::allowURL(
    ContentSecurityPolicySourceListDirective* sourceList, ResourceURL* url)
{
    // TODO: checking empty, self, and star needs to be removed.
    // the occurrences related should be also removed. e.g) allowURLScript.
    if (!sourceList) {
        return true;
    } else if (sourceList->allowSelf() &&
               m_contextURL->protocol()->equalsIgnoreCase(url->protocol()) &&
               m_contextURL->host()->equalsIgnoreCase(url->host())) {
        return true;
    } else if (sourceList->allowStar()) {
        return true;
    } else if (sourceList->allowURL(url)) {
        return true;
    }

    return false;
}

bool ContentSecurityPolicyDirectiveList::allowInlineScript()
{
    return allowInline(m_scriptSrc);
}

bool ContentSecurityPolicyDirectiveList::allowURLScript(ResourceURL* url)
{
    return allowURL(m_scriptSrc, url);
}

bool ContentSecurityPolicyDirectiveList::allowInlineStyle()
{
    return allowInline(m_styleSrc);
}

bool ContentSecurityPolicyDirectiveList::allowURLStyle(ResourceURL* url)
{
    return allowURL(m_styleSrc, url);
}

bool ContentSecurityPolicyDirectiveList::allowImage(String* src)
{
    if (!m_imgSrc) {
        return true;
    }

    const char dataType[] = "data:";
    if (src->startsWith(dataType, false)) {
        return m_imgSrc->allowScheme(String::createASCIIString(dataType));
    }

    if (m_imgSrc->allowStar()) {
        return true;
    }

    ContentSecurityPolicySourceURL* url = m_imgSrc->parseHost(src);
    if (!url) {
        return true;
    }

    if (m_imgSrc->allowSelf() &&
        m_contextURL->protocol()->equalsIgnoreCase(url->protocol) &&
        m_contextURL->host()->equalsIgnoreCase(url->host)) {
        return true;
    } else if (m_imgSrc->allowScheme(url->protocol)) {
        return true;
    }

    return m_imgSrc->allowURL(url, true);
}

bool ContentSecurityPolicyDirectiveList::allowConnect(ResourceURL* url)
{
    return allowURL(m_connectSrc, url);
}

size_t ContentSecurityPolicyDirectiveList::skipSpace(String* src, size_t begin,
                                                     size_t end)
{
    size_t current = begin;
    if (src->charAt(current) == ' ') {
        while (current <= end && src->charAt(current) == ' ')
            current++;
    }
    return current;
}
}
