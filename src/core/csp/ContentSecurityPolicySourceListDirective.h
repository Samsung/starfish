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

#ifndef __StarfishContentSecurityPolicySourceListDirective__
#define __StarfishContentSecurityPolicySourceListDirective__

namespace Starfish {

struct ContentSecurityPolicySourceURL : public gc {
    ContentSecurityPolicySourceURL()
        : m_protocol(String::emptyString)
        , m_serverName(String::emptyString)
        , m_domainName(String::emptyString)
        , m_path(String::emptyString)
        , m_isStarProtocol(false)
        , m_isStarServer(false)

    {
    }
    String* m_protocol;
    String* m_serverName;
    String* m_domainName;
    String* m_path;

    bool m_isStarProtocol;
    bool m_isStarServer;
};

class ContentSecurityPolicyDirectiveList;

typedef std::unordered_set<std::string> HashSet;

class ContentSecurityPolicySourceListDirective : public gc {
public:
    ContentSecurityPolicySourceListDirective(
        ContentSecurityPolicyDirectiveList* directiveList)
        : m_directiveList(directiveList)
        , m_name(String::emptyString)
        , m_allowStar(false)
        , m_allowInline(false)
        , m_allowEval(false)
        , m_allowSelf(false)
        , m_hashAlgorithmsUsed(0)
    {
    }

    void setDirective(String* name, String* value)
    {
        m_name = name;

        parseSource(value);
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

    bool allowURL(ResourceURL* url);
    bool allowContent(String* content);

protected:
private:
    ContentSecurityPolicyDirectiveList* m_directiveList;
    String* m_name;
    bool m_allowStar;
    bool m_allowInline;
    bool m_allowEval;
    bool m_allowSelf;
    GCVector<ContentSecurityPolicySourceURL*> m_URLSourceList;
    HashSet m_hashes;
    uint32_t m_hashAlgorithmsUsed;

    void parseSource(String* value);
    void parseHost(String* source);
    bool parseHash(String* source);
    bool parseHashA(String* source);
};
}

#endif
