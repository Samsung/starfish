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

#include <EscargotPublic.h>
using namespace Escargot;

#include "StarfishConfig.h"
#include "Starfish.h"
#include "PlatformIntegrationData.h"
#include "core/page/Window.h"
#include "core/dom/Document.h"
#include "core/csp/ContentSecurityPolicy.h"
#include "core/csp/ContentSecurityPolicyDirectiveList.h"
#include "core/csp/ContentSecurityPolicySourceListDirective.h"
#include "core/csp/SecurityPolicyViolationEvent.h"
#include "core/util/Cryptographic.h"
#include "core/page/WebView.h"

namespace Starfish {

ContentSecurityPolicy::ContentSecurityPolicy(Window* window)
    : WindowHoldable(window)
{
    window->scriptBindingInstance()
        ->scriptContext()
        ->setSecurityPolicyCheckCallback(
            ContentSecurityPolicy::checkUnsafeEvalCallback);
}

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

void ContentSecurityPolicy::copyFrom(ContentSecurityPolicy* source)
{
    for (auto& policy : source->m_policies) {
        didReceiveHeader(const_cast<String*>(policy->header()),
                         policy->headerType(),
                         ContentSecurityPolicyHeaderSource::Inherited);
    }
}

void ContentSecurityPolicy::didReceiveHeader(
    String* header, ContentSecurityPolicyHeaderType type,
    ContentSecurityPolicyHeaderSource source)
{
    if (window()->document()->webView()->getWebSecurityMode() ==
        LWE::WebSecurityMode::Disable) {
        return;
    }

    ContentSecurityPolicyDirectiveList* policy =
        new ContentSecurityPolicyDirectiveList(this, header, 0,
                                               header->length(), type, source);

    m_policies.push_back(policy);
}

static String* getDirectiveName(CSPDirectives directive)
{
    switch (directive) {
    case CSPDirectives::BaseURI:
        return String::createASCIIString("base-uri");
    case CSPDirectives::ConnectSrc:
        return String::createASCIIString("connect-src");
    case CSPDirectives::DefaultSrc:
        return String::createASCIIString("default-src");
    case CSPDirectives::ChildSrc:
        return String::createASCIIString("frame-src");
    case CSPDirectives::ImgSrc:
        return String::createASCIIString("img-src");
    case CSPDirectives::MediaSrc:
        return String::createASCIIString("media-src");
    case CSPDirectives::ScriptSrc:
        return String::createASCIIString("script-src");
    case CSPDirectives::StyleSrc:
        return String::createASCIIString("style-src");
    default:
        break;
    }
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return String::emptyString;
}

bool ContentSecurityPolicy::allowInlineEventHandler()
{
    return allowInline(CSPDirectives::ScriptSrc, nullptr);
}

bool ContentSecurityPolicy::allowSource(
    CSPDirectives directive, ResourceURL* resUrl,
    SecurityPolicyViolationEventDelegator eventDelegator)
{
    bool isAllowed = true;
    for (auto policy : m_policies) {
        if (!policy->allowSource(directive, resUrl)) {
            dispatchViolationEvent(getDirectiveName(directive),
                                   resUrl->urlString(), eventDelegator);
            STARFISH_LOG_WARN(
                "Refused to use '%s' as a source of '%s' because it violates "
                "the Content Security Policy\n",
                CSTR(resUrl->urlString()), CSTR(getDirectiveName(directive)));
            isAllowed = false;
        }
    }
    return isAllowed;
}

bool ContentSecurityPolicy::allowInline(CSPDirectives directive,
                                        String* scriptContent, String* nonce)
{
    bool isAllowed = true;
    for (auto policy : m_policies) {
        if (!policy->allowInline(directive, scriptContent, nonce)) {
            dispatchViolationEvent(getDirectiveName(directive));
            STARFISH_LOG_WARN(
                "Refused to execute contents as an inline-source of '%s' "
                "because it violates the Content Security Policy\n",
                CSTR(getDirectiveName(directive)));
            isAllowed = false;
        }
    }
    return isAllowed;
}

bool ContentSecurityPolicy::allowNonceOrSource(CSPDirectives directive,
                                               String* nonce,
                                               ResourceURL* resUrl)
{
    bool isAllowed = true;
    for (auto policy : m_policies) {
        if (!policy->allowNonceOrSource(directive, nonce, resUrl)) {
            dispatchViolationEvent(getDirectiveName(directive));
            STARFISH_LOG_WARN(
                "Refused to use '%s' as a source of '%s' because it violates "
                "the Content Security Policy\n",
                CSTR(resUrl->urlString()), CSTR(getDirectiveName(directive)));
            isAllowed = false;
        }
    }
    return isAllowed;
}

bool ContentSecurityPolicy::allowEval(CSPDirectives directive)
{
    bool isAllowed = true;
    for (auto policy : m_policies) {
        if (!policy->allowEval(directive)) {
            dispatchViolationEvent(getDirectiveName(directive));
            STARFISH_LOG_WARN(
                "Refused to execute a string as JavaScript' "
                "because it violates the Content Security Policy\n");
            isAllowed = false;
        }
    }
    return isAllowed;
}

void ContentSecurityPolicy::dispatchViolationEvent(
    String* name, String* blockedURI,
    SecurityPolicyViolationEventDelegator eventDelegator)
{
    String* eventType = window()
                            ->starfish()
                            ->staticStrings()
                            ->m_securitypolicyviolation.localName();
    auto event =
        new SecurityPolicyViolationEvent(window()->document(), eventType);
    event->setViolatedDirective(name);
    event->setBlockedURI(blockedURI);

    if (eventDelegator) {
        eventDelegator(event, window());
    } else {
        window()->document()->dispatchEventIdleTimeByUA(event);
    }
}

ScriptValue ContentSecurityPolicy::checkUnsafeEvalCallback(
    ScriptExecutionState state, bool isEval)
{
    Document* document = fetchDocument(state->context());
    ContentSecurityPolicy* csp = document->contentSecurityPolicy();
    if (!csp->allowEval(CSPDirectives::ScriptSrc)) {
        if (isEval) {
            return ValueRef::create(
                StringRef::fromASCII("Exception EvalError"));
        }
        return ValueRef::create(
            StringRef::fromASCII("Exception function EvalError"));
    }

    return ValueRef::createEmpty();
}
}
