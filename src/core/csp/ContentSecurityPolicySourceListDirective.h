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

#ifndef __StarfishContentSecurityPolicySourceListDirective__
#define __StarfishContentSecurityPolicySourceListDirective__

namespace Starfish {

struct ContentSecurityPolicySource : public gc {
    ContentSecurityPolicySource()
        : protocol(String::emptyString)
        , serverName(String::emptyString)
        , domainName(String::emptyString)
        , port(String::emptyString)
        , path(String::emptyString)
        , isStarProtocol(false)
        , isStarServer(false)
        , isStarDomain(false)
        , isStarPort(false)

    {
    }
    String* protocol;
    String* serverName;
    String* domainName;
    String* port;
    String* path;

    bool isStarProtocol;
    bool isStarServer;
    bool isStarDomain;
    bool isStarPort;
};

class ContentSecurityPolicyDirectiveList;

typedef std::unordered_set<std::string> HashSet;

class ContentSecurityPolicySourceListDirective : public gc {
public:
    ContentSecurityPolicySourceListDirective(
        ContentSecurityPolicyDirectiveList* directiveList, String* name,
        const GCVector<StringView>& token)
        : m_directiveList(directiveList)
        , m_name(name)
        , m_allowStar(false)
        , m_allowInline(false)
        , m_allowEval(false)
        , m_allowSelf(false)
        , m_hashAlgorithmsUsed(0)
    {
        parseSource(token);
    }

    String* name()
    {
        return m_name;
    }

    bool allowStar()
    {
        return m_allowStar;
    }

    bool allowInline()
    {
        return m_allowInline;
    }

    bool allowEval()
    {
        return m_allowEval;
    }

    bool allowSelf()
    {
        return m_allowSelf;
    }

    bool allowContent(String* content);
    bool allowNonce(String* content);
    bool allowScheme(String* str);
    bool allowScheme(ResourceURL* resUrl);
    bool allowHost(ResourceURL* url);

    bool hasNonceOrHash()
    {
        return (m_nonces.size() > 0) || (m_hashes.size() > 0);
    }

    static inline bool isHostCharacter(char32_t c)
    {
        return isASCIIDigit(c) || isASCIIAlpha(c) || c == '-';
    }

    static inline bool isSchemeCharacter(char32_t c)
    {
        // https://tools.ietf.org/html/rfc3986#section-3.1
        return isASCIIAlpha(c) || isASCIIDigit(c) || c == '+' || c == '-' ||
               c == '.';
    }

    static bool matcheScheme(String* scheme, ResourceURL* url);
    static bool matchePort(String* sourcePort, String* sourceProtocol,
                           ResourceURL* url);
    static bool matchePath(String* sourcePath, String* urlPath);

protected:
private:
    ContentSecurityPolicyDirectiveList* m_directiveList;
    String* m_name;
    bool m_allowStar;
    bool m_allowInline;
    bool m_allowEval;
    bool m_allowSelf;
    GCVector<ContentSecurityPolicySource*> m_sourceList;
    HashSet m_hashes;
    HashSet m_nonces;
    uint32_t m_hashAlgorithmsUsed;
    GCVector<String*> m_schemeList;

    void parseSource(const GCVector<StringView>& sources);
    bool parseHash(String* source);
    bool parseNonce(String* source);
    bool isScheme(String* scheme);
    bool parseHostSource(String* source);
    static bool parseScheme(ContentSecurityPolicySource* source,
                            String* sourceString, size_t& position);
    static bool parseHost(ContentSecurityPolicySource* source,
                          String* sourceString, size_t& position);
    static bool parsePort(ContentSecurityPolicySource* source,
                          String* sourceString, size_t& position);
    static bool parsePath(ContentSecurityPolicySource* source,
                          String* sourceString, size_t& position);
};
} // namespace Starfish

#endif
