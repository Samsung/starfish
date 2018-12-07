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

#ifndef __StarfishContentSecurityPolicyDirectiveList__
#define __StarfishContentSecurityPolicyDirectiveList__

#include "StarfishConfig.h"
#include "core/csp/ContentSecurityPolicy.h"
#include "core/csp/ContentSecurityPolicySourceListDirective.h"

namespace Starfish {

class ContentSecurityPolicyDirectiveList : public gc {
public:
    ContentSecurityPolicyDirectiveList(
        ContentSecurityPolicy* contentSecurityPolicy, String* policy,
        size_t begin, size_t end);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    void parse(String* policy, size_t begin, size_t end);

    void addDirective(String* name, String* value);

    bool allowSource(CSPDirectives directive, ResourceURL* resUrl);
    bool allowNonce(CSPDirectives directive, String* nonce);
    bool allowInline(CSPDirectives directive, String* scriptContent,
                     String* nonce);
    bool allowEval(CSPDirectives directive);

    ContentSecurityPolicySourceListDirective* getSourceList(
        CSPDirectives directive);

private:
    ContentSecurityPolicy* m_contentSecurityPolicy;
    ResourceURL* m_contextURL;

    ContentSecurityPolicySourceListDirective* m_baseURI;
    ContentSecurityPolicySourceListDirective* m_connectSrc;
    ContentSecurityPolicySourceListDirective* m_defaultSrc;
    ContentSecurityPolicySourceListDirective* m_frameSrc;
    ContentSecurityPolicySourceListDirective* m_imgSrc;
    ContentSecurityPolicySourceListDirective* m_mediaSrc;
    ContentSecurityPolicySourceListDirective* m_scriptSrc;
    ContentSecurityPolicySourceListDirective* m_styleSrc;

    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc, GC_WORD_OFFSET(ContentSecurityPolicyDirectiveList,
                                        m_baseURI));
        GC_set_bit(desc, GC_WORD_OFFSET(ContentSecurityPolicyDirectiveList,
                                        m_connectSrc));
        GC_set_bit(desc, GC_WORD_OFFSET(ContentSecurityPolicyDirectiveList,
                                        m_defaultSrc));
        GC_set_bit(desc, GC_WORD_OFFSET(ContentSecurityPolicyDirectiveList,
                                        m_frameSrc));
        GC_set_bit(
            desc, GC_WORD_OFFSET(ContentSecurityPolicyDirectiveList, m_imgSrc));
        GC_set_bit(desc, GC_WORD_OFFSET(ContentSecurityPolicyDirectiveList,
                                        m_mediaSrc));
        GC_set_bit(desc, GC_WORD_OFFSET(ContentSecurityPolicyDirectiveList,
                                        m_scriptSrc));
        GC_set_bit(desc, GC_WORD_OFFSET(ContentSecurityPolicyDirectiveList,
                                        m_styleSrc));
    }

    void setDirective(ContentSecurityPolicySourceListDirective*& directive,
                      String* name, String* value);

    bool isMatchingStar(ContentSecurityPolicySourceListDirective* directive,
                        ResourceURL* resUrl);
    bool isMatchingSelf(ContentSecurityPolicySourceListDirective* directive,
                        ResourceURL* url);

    static size_t skipSpaceAndNewline(String* src, size_t begin, size_t end);
};
}

#endif
