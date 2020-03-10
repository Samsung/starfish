/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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
#include "core/csp/ContentSecurityPolicy.h"
#include "core/csp/ContentSecurityPolicyDirectiveList.h"
#include "core/csp/ContentSecurityPolicy.h"
#include "core/csp/ContentSecurityPolicySourceListDirective.h"
#include "core/dom/ExecutionContext.h"
#include "platform/loader/ResourceURL.h"

namespace Starfish {
ContentSecurityPolicyDirectiveList::ContentSecurityPolicyDirectiveList(
    ContentSecurityPolicy* contentSecurityPolicy, String* policy,
    ContentSecurityPolicyHeaderType type,
    ContentSecurityPolicyHeaderSource source)
    : m_contentSecurityPolicy(contentSecurityPolicy)
    , m_contextURL(contentSecurityPolicy->executionContext()->documentURI())
    , m_baseURI(nullptr)
    , m_connectSrc(nullptr)
    , m_childSrc(nullptr)
    , m_defaultSrc(nullptr)
    , m_formAction(nullptr)
    , m_imgSrc(nullptr)
    , m_mediaSrc(nullptr)
    , m_scriptSrc(nullptr)
    , m_styleSrc(nullptr)
    , m_header(nullptr)
{
    m_headerType = type;
    size_t end = policy->length();
    if (end == 0)
        return;

    m_header = policy;

    size_t current = 0;
    size_t directiveBegin = 0;

    while (current < end) {
        if (policy->charAt(current) == ';' || policy->charAt(current) == ',') {
            if (directiveBegin < current) {
                addDirective(policy->substring(directiveBegin,
                                               current - directiveBegin));
                directiveBegin = current + 1;
            }
        }
        current++;
    }

    if (directiveBegin < current) {
        addDirective(
            policy->substring(directiveBegin, current - directiveBegin));
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

void ContentSecurityPolicyDirectiveList::addDirective(String* value)
{
    GCVector<StringView> tokens;
    StringUtils::wordTokenizer(value, tokens);
    if (tokens.size() == 0) {
        return;
    }
    auto name = tokens[0].substring();
    if (name->equalsIgnoreCase("base-uri")) {
        setDirective(m_baseURI, name, tokens);
    } else if (name->equalsIgnoreCase("connect-src")) {
        setDirective(m_connectSrc, name, tokens);
    } else if (name->equalsIgnoreCase("child-src")) {
        setDirective(m_childSrc, name, tokens);
    } else if (name->equalsIgnoreCase("default-src")) {
        setDirective(m_defaultSrc, name, tokens);
    } else if (name->equalsIgnoreCase("form-action")) {
        setDirective(m_formAction, name, tokens);
    } else if (name->equalsIgnoreCase("frame-src")) {
        STARFISH_LOG_INFO(
            "'frame-src' is deprecated. Using 'child-src' is recommended "
            "instead.\n");
        setDirective(m_childSrc, name, tokens);
    } else if (name->equalsIgnoreCase("img-src")) {
        setDirective(m_imgSrc, name, tokens);
    } else if (name->equalsIgnoreCase("media-src")) {
        setDirective(m_mediaSrc, name, tokens);
    } else if (name->equalsIgnoreCase("script-src")) {
        setDirective(m_scriptSrc, name, tokens);
    } else if (name->equalsIgnoreCase("style-src")) {
        setDirective(m_styleSrc, name, tokens);
    }
}

void ContentSecurityPolicyDirectiveList::setDirective(
    ContentSecurityPolicySourceListDirective*& directive, String* name,
    const GCVector<StringView>& token)
{
    if (directive) {
        m_contentSecurityPolicy->dispatchViolationEvent(name);
        return;
    }
    directive = new ContentSecurityPolicySourceListDirective(this, name, token);
}

ContentSecurityPolicySourceListDirective*
ContentSecurityPolicyDirectiveList::getSourceList(CSPDirectives directive)
{
    switch (directive) {
    case CSPDirectives::BaseURI:
        return m_baseURI;
    case CSPDirectives::ChildSrc:
        return m_childSrc;
    case CSPDirectives::ConnectSrc:
        return m_connectSrc;
    case CSPDirectives::DefaultSrc:
        return m_defaultSrc;
    case CSPDirectives::FormAction:
        return m_formAction;
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

static bool isAllowedCascadigDefault(CSPDirectives directive)
{
    switch (directive) {
    case CSPDirectives::ChildSrc:
    case CSPDirectives::ConnectSrc:
    case CSPDirectives::ImgSrc:
    case CSPDirectives::MediaSrc:
    case CSPDirectives::ScriptSrc:
    case CSPDirectives::StyleSrc:
        return true;
    default:
        return false;
    }
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return false;
}

bool ContentSecurityPolicyDirectiveList::allowSource(CSPDirectives directive,
                                                     ResourceURL* resUrl)
{
    if (resUrl->isAboutURL()) {
        return true;
    }

    auto sourceListDirective = getSourceList(directive);
    if (!sourceListDirective) {
        if (m_defaultSrc && isAllowedCascadigDefault(directive)) {
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
    if (directive->allowSelf()) {
        if (m_contextURL->origin()->equalsIgnoreCase(resUrl->origin())) {
            return true;
        } else if (m_contextURL->protocol()->equalsIgnoreCase(
                       resUrl->protocol()) &&
                   m_contextURL->host()->equalsIgnoreCase(resUrl->host())) {
            return ContentSecurityPolicySourceListDirective::matchePort(
                m_contextURL->port(), m_contextURL->protocol(), resUrl);
        }
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
}
