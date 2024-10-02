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

#ifndef __StarfishContentSecurityPolicy__
#define __StarfishContentSecurityPolicy__

#include "binding/ScriptWrappable.h"

namespace Starfish {

enum class ContentSecurityPolicyHeaderType { Report, Enforce };

enum class ContentSecurityPolicyHeaderSource {
    HTTP,
    Meta,
    OriginPolicy,
    Inherited,
};

enum class CSPDirectives {
    BaseURI,
    ConnectSrc,
    ChildSrc,
    DefaultSrc,
    FormAction,
    ImgSrc,
    MediaSrc,
    ObjectSrc,
    ScriptSrc,
    StyleSrc,
};

class ContentSecurityPolicyDirectiveList;
class ContentSecurityPolicySourceListDirective;

using SecurityPolicyViolationEventDelegator = void (*)(
    SecurityPolicyViolationEvent* event, ExecutionContext* executionContext);

class ContentSecurityPolicy : public gc {
public:
    ContentSecurityPolicy(ExecutionContext* executionContext);

    void didReceiveHeader(String* header, ContentSecurityPolicyHeaderType type,
                          ContentSecurityPolicyHeaderSource source);

    bool allowInlineEventHandler();
    bool allowSource(
        CSPDirectives directive, ResourceURL* url,
        SecurityPolicyViolationEventDelegator eventDelegator = nullptr);
    bool allowInline(CSPDirectives directive, String* scriptContent,
                     String* nonce = nullptr);
    bool allowEval(CSPDirectives directive);
    bool allowNonceOrSource(CSPDirectives directive, String* nonce,
                            ResourceURL* resUrl);

    void dispatchViolationEvent(
        String* name, String* blockedURI = String::emptyString,
        SecurityPolicyViolationEventDelegator eventDelegator = nullptr);

    void copyFrom(ContentSecurityPolicy* policy);

    ExecutionContext* executionContext()
    {
        return m_executionContext;
    }

private:
    GCVector<ContentSecurityPolicyDirectiveList*> m_policies;
    ExecutionContext* m_executionContext;

    static ScriptOptionalValue checkUnsafeEvalCallback(
        ScriptExecutionState state, bool isEval);
};
} // namespace Starfish

#endif
