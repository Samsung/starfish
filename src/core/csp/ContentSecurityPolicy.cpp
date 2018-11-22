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

static String* getDirectiveName(CSPDirectives directive)
{
    switch (directive) {
    case CSPDirectives::ConnectSrc:
        return String::createASCIIString("connect-src");
    case CSPDirectives::FrameSrc:
        return String::createASCIIString("frame-src");
    case CSPDirectives::ImgSrc:
        return String::createASCIIString("img-src");
    case CSPDirectives::ScriptSrc:
        return String::createASCIIString("script-src");
    case CSPDirectives::StyleSrc:
        return String::createASCIIString("style-src");
    default:
        break;
    }
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
    return String::emptyString;
}

bool ContentSecurityPolicy::allowInlineEventHandlers(String* contextURL)
{
    return allowInline(CSPDirectives::ScriptSrc, nullptr);
}

bool ContentSecurityPolicy::allowSource(CSPDirectives directive,
                                        ResourceURL* resUrl)
{
    for (auto policy : m_policies) {
        if (!policy->getSourceList(directive)) {
            continue;
        } else if (policy->allowStar(directive, resUrl)) {
            return true;
        }

        if (!policy->allowSelf(directive, resUrl) &&
            !policy->allowScheme(directive, resUrl) &&
            !policy->allowHost(directive, resUrl)) {
            // TODO: check the query with default-src policies
            dispatchViolationEvent(getDirectiveName(directive));
            return false;
        }
    }
    return true;
}

bool ContentSecurityPolicy::allowInline(CSPDirectives directive,
                                        String* scriptContent,
                                        String* nonce /*= nullptr*/)
{
    for (auto policy : m_policies) {
        if (!policy->getSourceList(directive)) {
            continue; // allowed if no src-list exists
        } else if (policy->allowInline(directive)) {
            return true;
        }

        if (!policy->allowNonce(directive, nonce) &&
            !policy->allowContent(directive, scriptContent)) {
            dispatchViolationEvent(getDirectiveName(directive));
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
