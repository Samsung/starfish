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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/page/Window.h"
#include "core/csp/ContentSecurityPolicy.h"
#include "core/csp/ContentSecurityPolicyDirectiveList.h"
#include "core/csp/ContentSecurityPolicySourceListDirective.h"
#include "core/csp/SecurityPolicyViolationEvent.h"
#include "core/util/Cryptographic.h"

namespace Starfish {

void* ContentSecurityPolicy::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(ContentSecurityPolicy));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(ContentSecurityPolicy)] = { 0 };
        ContentSecurityPolicy::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(ContentSecurityPolicy));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void ContentSecurityPolicy::didReceiveHeader(
    String* header, ContentSecurityPolicyHeaderType type,
    ContentSecurityPolicyHeaderSource source)
{
#ifdef STARFISH_ENABLE_CSP
    ContentSecurityPolicyDirectiveList* policy =
        new ContentSecurityPolicyDirectiveList(this, header, 0,
                                               header->length());

    m_policies.push_back(policy);
#endif
}

bool ContentSecurityPolicy::findHashOfContentInSourceList(
    ContentSecurityPolicySourceListDirective* sourceList, String* content)
{
    if (sourceList->allowContent(content)) {
        return true;
    }
    return false;
}

bool ContentSecurityPolicy::allowInlineEventHandlers(String* contextURL)
{
    return allowInlineScript(contextURL, nullptr);
}

bool ContentSecurityPolicy::allowInlineScript(String* contextURL,
                                              String* scriptContent)
{
    for (auto policy : m_policies) {
        auto src = policy->scriptSrc();

        if (!src) {
            continue;
        } else if (policy->allowInlineScript() ||
                   findHashOfContentInSourceList(src, scriptContent)) {
            continue;
        } else {
            dispatchViolationEvent(src->name());
            return false;
        }
    }
    return true;
}

bool ContentSecurityPolicy::allowURLScript(ResourceURL* url)
{
    for (auto policy : m_policies) {
        if (!policy->allowURLScript(url)) {
            dispatchViolationEvent(policy->scriptSrc()->name());
            return false;
        }
    }
    return true;
}

static String* getDirectiveName(CSPDirectives directive)
{
    String* name = String::emptyString;

    switch (directive) {
    case CSPDirectives::ScriptSrc:
        name = String::createASCIIString("script-src");
        break;
    case CSPDirectives::StyleSrc:
        name = String::createASCIIString("style-src");
        break;
    case CSPDirectives::FrameSrc:
        name = String::createASCIIString("frame-src");
        break;
    default:
        break;
    }
    return name;
}

bool ContentSecurityPolicy::allowSource(CSPDirectives directive,
                                        ResourceURL* resUrl)
{
    for (auto policy : m_policies) {
        if (!policy->getSourceList(directive)) {
            continue;
        } else if (policy->allowStar(directive)) {
            return true;
        }

        if (!policy->allowSelf(directive, resUrl) &&
            !policy->allowScheme(directive, resUrl) &&
            !policy->allowURL(directive, resUrl)) {
            // TODO: check the query with default-src policies
            dispatchViolationEvent(getDirectiveName(directive));
            return false;
        }
    }
    return true;
}

bool ContentSecurityPolicy::allowInlineStyle(String* contextURL,
                                             String* styleContent)
{
    for (auto policy : m_policies) {
        auto src = policy->styleSrc();

        if (!src) {
            continue;
        } else if (policy->allowInlineStyle() ||
                   findHashOfContentInSourceList(src, styleContent)) {
            continue;
        } else {
            dispatchViolationEvent(src->name());
            return false;
        }
    }
    return true;
}

bool ContentSecurityPolicy::allowURLStyle(ResourceURL* url)
{
    for (auto policy : m_policies) {
        if (!policy->allowURLStyle(url)) {
            dispatchViolationEvent(policy->styleSrc()->name());
            return false;
        }
    }
    return true;
}

bool ContentSecurityPolicy::allowImage(String* src)
{
    for (auto policy : m_policies) {
        if (!policy->allowImage(src)) {
            dispatchViolationEvent(policy->imgSrc()->name());
            return false;
        }
    }
    return true;
}

bool ContentSecurityPolicy::allowConnect(ResourceURL* url)
{
    for (auto policy : m_policies) {
        if (!policy->allowConnect(url)) {
            dispatchViolationEvent(policy->connectSrc()->name());
            return false;
        }
    }
    return true;
}

void ContentSecurityPolicy::dispatchViolationEvent(String* name)
{
    String* eventType = window()
                            ->starfish()
                            ->staticStrings()
                            ->m_securitypolicyviolation.localName();
    auto event =
        new SecurityPolicyViolationEvent(window()->document(), eventType);
    event->setViolatedDirective(name);
    window()->dispatchEventIdleTimeByUA(event);
}
}
