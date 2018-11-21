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
        : protocol(String::emptyString)
        , serverName(String::emptyString)
        , domainName(String::emptyString)
        , port(String::emptyString)
        , host(String::emptyString)
        , path(String::emptyString)
        , isStarProtocol(false)
        , isStarServer(false)
        , isStarPort(false)

    {
    }
    String* protocol;
    String* serverName;
    String* domainName;
    String* port;
    String* host;
    String* path;

    bool isStarProtocol;
    bool isStarServer;
    bool isStarPort;
};

class ContentSecurityPolicyDirectiveList;

typedef std::unordered_set<std::string> HashSet;

class ContentSecurityPolicySourceListDirective : public gc {
public:
    ContentSecurityPolicySourceListDirective(
        ContentSecurityPolicyDirectiveList* directiveList, String* name,
        String* value)
        : m_directiveList(directiveList)
        , m_name(name)
        , m_allowStar(false)
        , m_allowInline(false)
        , m_allowEval(false)
        , m_allowSelf(false)
        , m_hashAlgorithmsUsed(0)
    {
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

    bool allowContent(String* content);
    bool allowURL(ResourceURL* url, bool ignoreScheme = false);
    bool allowURL(ContentSecurityPolicySourceURL* url,
                  bool ignoreScheme = false);
    bool allowURL(String* scheme, String* serverName, String* domainName,
                  String* port, String* path, bool ignoreScheme = false);
    bool allowScheme(String* str);

    ContentSecurityPolicySourceURL* parseHost(String* source);

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
    GCVector<String*> m_schemeList;

    void parseSource(String* value);
    bool parseHash(String* source);
    bool parseHashA(String* source);
    bool isScheme(String* scheme);
};
}

#endif
