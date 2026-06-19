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

#ifndef __StarfishContentSecurityPolicyDirectiveList__
#define __StarfishContentSecurityPolicyDirectiveList__

namespace Starfish {

class ContentSecurityPolicy;
class ContentSecurityPolicySourceListDirective;

class ContentSecurityPolicyDirectiveList : public gc {
public:
    ContentSecurityPolicyDirectiveList(
        ContentSecurityPolicy* contentSecurityPolicy, String* policy,
        ContentSecurityPolicyHeaderType type,
        ContentSecurityPolicyHeaderSource source);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    void parse(String* policy, size_t begin, size_t end);

    void addDirective(String* value);

    bool allowSource(CSPDirectives directive, ResourceURL* resUrl);
    bool allowNonceOrSource(CSPDirectives directive, String* nonce,
                            ResourceURL* resUrl);
    bool allowInline(CSPDirectives directive, String* scriptContent,
                     String* nonce);
    bool allowEval(CSPDirectives directive);
    // Checks frame-ancestors against all ancestor URLs. No default-src
    // fallback. Returns true if no frame-ancestors directive is set.
    bool allowAncestors(const GCVector<ResourceURL*>& ancestorURLs);

    bool hasFrameAncestors() const
    {
        return m_frameAncestors != nullptr;
    }

    ContentSecurityPolicySourceListDirective* getSourceList(
        CSPDirectives directive);

    const String* header() const
    {
        return m_header;
    }

    ContentSecurityPolicyHeaderType headerType() const
    {
        return m_headerType;
    }

    ContentSecurityPolicyHeaderSource headerSource() const
    {
        return m_headerSource;
    }

    // frame-ancestors is enforced only from a policy delivered with the
    // response; inherited and <meta> policies must not block, per the CSP spec.
    bool isFrameAncestorsEnforceable() const
    {
        return m_headerSource == ContentSecurityPolicyHeaderSource::HTTP;
    }

private:
    ContentSecurityPolicy* m_contentSecurityPolicy;
    ResourceURL* m_contextURL;

    ContentSecurityPolicySourceListDirective* m_baseURI;
    ContentSecurityPolicySourceListDirective* m_connectSrc;
    ContentSecurityPolicySourceListDirective* m_childSrc;
    ContentSecurityPolicySourceListDirective* m_defaultSrc;
    ContentSecurityPolicySourceListDirective* m_formAction;
    ContentSecurityPolicySourceListDirective* m_frameAncestors;
    ContentSecurityPolicySourceListDirective* m_imgSrc;
    ContentSecurityPolicySourceListDirective* m_mediaSrc;
    ContentSecurityPolicySourceListDirective* m_scriptSrc;
    ContentSecurityPolicySourceListDirective* m_styleSrc;

    String* m_header;
    ContentSecurityPolicyHeaderType m_headerType;
    ContentSecurityPolicyHeaderSource m_headerSource;

    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc, GC_WORD_OFFSET(ContentSecurityPolicyDirectiveList,
                                        m_contentSecurityPolicy));
        GC_set_bit(desc, GC_WORD_OFFSET(ContentSecurityPolicyDirectiveList,
                                        m_contextURL));

        GC_set_bit(desc, GC_WORD_OFFSET(ContentSecurityPolicyDirectiveList,
                                        m_baseURI));
        GC_set_bit(desc, GC_WORD_OFFSET(ContentSecurityPolicyDirectiveList,
                                        m_connectSrc));
        GC_set_bit(desc, GC_WORD_OFFSET(ContentSecurityPolicyDirectiveList,
                                        m_childSrc));
        GC_set_bit(desc, GC_WORD_OFFSET(ContentSecurityPolicyDirectiveList,
                                        m_defaultSrc));
        GC_set_bit(desc, GC_WORD_OFFSET(ContentSecurityPolicyDirectiveList,
                                        m_formAction));
        GC_set_bit(desc, GC_WORD_OFFSET(ContentSecurityPolicyDirectiveList,
                                        m_frameAncestors));
        GC_set_bit(
            desc, GC_WORD_OFFSET(ContentSecurityPolicyDirectiveList, m_imgSrc));
        GC_set_bit(desc, GC_WORD_OFFSET(ContentSecurityPolicyDirectiveList,
                                        m_mediaSrc));
        GC_set_bit(desc, GC_WORD_OFFSET(ContentSecurityPolicyDirectiveList,
                                        m_scriptSrc));
        GC_set_bit(desc, GC_WORD_OFFSET(ContentSecurityPolicyDirectiveList,
                                        m_styleSrc));

        GC_set_bit(
            desc, GC_WORD_OFFSET(ContentSecurityPolicyDirectiveList, m_header));
    }

    void setDirective(ContentSecurityPolicySourceListDirective*& directive,
                      String* name, const GCVector<StringView>& token);

    bool isMatchingStar(ContentSecurityPolicySourceListDirective* directive,
                        ResourceURL* resUrl);
    bool isMatchingSelf(ContentSecurityPolicySourceListDirective* directive,
                        ResourceURL* url);
};
} // namespace Starfish

#endif
