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

    bool allowInlineScript();
    bool allowURLScript(ResourceURL* url);
    bool allowImage(String* src);
    bool allowInlineStyle();
    bool allowURLStyle(ResourceURL* url);
    bool allowConnect(ResourceURL* url);

    ContentSecurityPolicySourceListDirective* scriptSrc()
    {
        return m_scriptSrc;
    }
    ContentSecurityPolicySourceListDirective* imgSrc()
    {
        return m_imgSrc;
    }
    ContentSecurityPolicySourceListDirective* styleSrc()
    {
        return m_styleSrc;
    }
    ContentSecurityPolicySourceListDirective* connectSrc()
    {
        return m_connectSrc;
    }

    bool allowInline(ContentSecurityPolicySourceListDirective* sourceList);
    bool allowURL(ContentSecurityPolicySourceListDirective* sourceList,
                  ResourceURL* url);

private:
    ContentSecurityPolicy* m_contentSecurityPolicy;
    ResourceURL* m_contextURL;
    ContentSecurityPolicySourceListDirective* m_scriptSrc;
    ContentSecurityPolicySourceListDirective* m_imgSrc;
    ContentSecurityPolicySourceListDirective* m_styleSrc;
    ContentSecurityPolicySourceListDirective* m_connectSrc;

    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc, GC_WORD_OFFSET(ContentSecurityPolicyDirectiveList,
                                        m_scriptSrc));
        GC_set_bit(
            desc, GC_WORD_OFFSET(ContentSecurityPolicyDirectiveList, m_imgSrc));
        GC_set_bit(desc, GC_WORD_OFFSET(ContentSecurityPolicyDirectiveList,
                                        m_styleSrc));
        GC_set_bit(desc, GC_WORD_OFFSET(ContentSecurityPolicyDirectiveList,
                                        m_connectSrc));
    }

    static size_t skipSpace(String* src, size_t begin, size_t end);
};
}

#endif
