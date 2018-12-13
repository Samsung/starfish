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
    size_t end, ContentSecurityPolicyHeaderType type,
    ContentSecurityPolicyHeaderSource source)
    : m_contentSecurityPolicy(contentSecurityPolicy)
    , m_contextURL(contentSecurityPolicy->document()->documentURI())
    , m_baseURI(nullptr)
    , m_connectSrc(nullptr)
    , m_childSrc(nullptr)
    , m_defaultSrc(nullptr)
    , m_imgSrc(nullptr)
    , m_mediaSrc(nullptr)
    , m_scriptSrc(nullptr)
    , m_styleSrc(nullptr)
    , m_header(nullptr)
{
    m_headerType = type;

    if (begin == end)
        return;

    m_header = policy->substring(begin, end);

    size_t current = skipSpaceAndNewline(policy, begin, end);
    size_t directiveBegin = current;

    String* name = nullptr;
    String* value = nullptr;
    while (current <= end) {
        if (String::isSpaceOrNewline(policy->charAt(current))) {
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
                        skipSpaceAndNewline(policy, current + 1, end);
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
    if (name->equalsIgnoreCase("base-uri")) {
        setDirective(m_baseURI, name, value);
    } else if (name->equalsIgnoreCase("connect-src")) {
        setDirective(m_connectSrc, name, value);
    } else if (name->equalsIgnoreCase("child-src")) {
        setDirective(m_childSrc, name, value);
    } else if (name->equalsIgnoreCase("default-src")) {
        setDirective(m_defaultSrc, name, value);
    } else if (name->equalsIgnoreCase("frame-src")) {
        STARFISH_LOG_INFO(
            "'frame-src' is deprecated. Using 'child-src' is recommended "
            "instead.\n");
        setDirective(m_childSrc, name, value);
    } else if (name->equalsIgnoreCase("img-src")) {
        setDirective(m_imgSrc, name, value);
    } else if (name->equalsIgnoreCase("media-src")) {
        setDirective(m_mediaSrc, name, value);
    } else if (name->equalsIgnoreCase("script-src")) {
        setDirective(m_scriptSrc, name, value);
    } else if (name->equalsIgnoreCase("style-src")) {
        setDirective(m_styleSrc, name, value);
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

ContentSecurityPolicySourceListDirective*
ContentSecurityPolicyDirectiveList::getSourceList(CSPDirectives directive)
{
    switch (directive) {
    case CSPDirectives::BaseURI:
        return m_baseURI;
    case CSPDirectives::ConnectSrc:
        return m_connectSrc;
    case CSPDirectives::ChildSrc:
        return m_childSrc;
    case CSPDirectives::DefaultSrc:
        return m_defaultSrc;
    case CSPDirectives::ImgSrc:
        return m_imgSrc;
    case CSPDirectives::MediaSrc:
        return m_mediaSrc;
    case CSPDirectives::ScriptSrc:
        return m_scriptSrc;
    case CSPDirectives::StyleSrc:
        return m_styleSrc;
    default:
        break;
    }
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return nullptr;
}

bool ContentSecurityPolicyDirectiveList::allowSource(CSPDirectives directive,
                                                     ResourceURL* resUrl)
{
    auto sourceListDirective = getSourceList(directive);
    if (!sourceListDirective) {
        if (m_defaultSrc && directive != CSPDirectives::BaseURI) {
            return allowSource(CSPDirectives::DefaultSrc, resUrl);
        }
        return true;
    } else if (isMatchingStar(sourceListDirective, resUrl) ||
               isMatchingSelf(sourceListDirective, resUrl) ||
               sourceListDirective->allowScheme(resUrl) ||
               sourceListDirective->allowHost(resUrl)) {
        return true;
    }
    return false;
}

bool ContentSecurityPolicyDirectiveList::isMatchingStar(
    ContentSecurityPolicySourceListDirective* directive, ResourceURL* resUrl)
{
    // 4.2.2.2 https://www.w3.org/TR/CSP2/#match-source-expression
    // If the source expression a consists of a single (*) character and url’s
    // scheme is not one of blob, data, filesystem, then return does match.
    if (resUrl->isDataURL() || resUrl->isBlobURL() || resUrl->isFileURL()) {
        return false;
    }

    STARFISH_ASSERT(directive);
    return directive->allowStar();
}

bool ContentSecurityPolicyDirectiveList::isMatchingSelf(
    ContentSecurityPolicySourceListDirective* directive, ResourceURL* resUrl)
{
    STARFISH_ASSERT(directive);
    if (directive->allowSelf() &&
        m_contextURL->protocol()->equalsIgnoreCase(resUrl->protocol()) &&
        m_contextURL->host()->equalsIgnoreCase(resUrl->host())) {
        return true;
    }
    return false;
}

bool ContentSecurityPolicyDirectiveList::allowNonceOrSource(
    CSPDirectives directive, String* nonce, ResourceURL* resUrl)
{
    auto sourceListDirective = getSourceList(directive);
    if (!sourceListDirective) {
        if (m_defaultSrc) {
            return allowNonceOrSource(CSPDirectives::DefaultSrc, nonce, resUrl);
        }
        return true;
    } else if (sourceListDirective->allowNonce(nonce) ||
               allowSource(directive, resUrl)) {
        return true;
    }
    return false;
}

bool ContentSecurityPolicyDirectiveList::allowInline(CSPDirectives directive,
                                                     String* scriptContent,
                                                     String* nonce)
{
    auto sourceListDirective = getSourceList(directive);
    if (!sourceListDirective) {
        if (m_defaultSrc) {
            return allowInline(CSPDirectives::DefaultSrc, scriptContent, nonce);
        }
        return true;
    } else if ((!sourceListDirective->hasNonceOrHash() &&
                sourceListDirective->allowInline()) ||
               sourceListDirective->allowNonce(nonce) ||
               sourceListDirective->allowContent(scriptContent)) {
        return true;
    }
    return false;
}

bool ContentSecurityPolicyDirectiveList::allowEval(CSPDirectives directive)
{
    auto sourceListDirective = getSourceList(directive);
    if (!sourceListDirective) {
        if (m_defaultSrc) {
            return allowEval(CSPDirectives::DefaultSrc);
        }
        return true;
    } else if (sourceListDirective->allowEval()) {
        return true;
    }
    return false;
}

size_t ContentSecurityPolicyDirectiveList::skipSpaceAndNewline(String* src,
                                                               size_t begin,
                                                               size_t end)
{
    size_t current = begin;
    while (current < end && String::isSpaceOrNewline(src->charAt(current))) {
        current++;
    }
    return current;
}
}
