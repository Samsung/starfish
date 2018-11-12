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
#include "core/csp/ContentSecurityPolicySourceListDirective.h"

namespace Starfish {

void ContentSecurityPolicySourceListDirective::parseSource(String* value)
{
    size_t begin = 0;
    size_t length = value->length();

    for (size_t i = 0; i < length; i++) {
        if (value->charAt(i) == ' ' || i == length - 1) {
            size_t end = (i == length - 1) ? i - begin + 1 : i - begin;
            String* source = value->substring(begin, end);
            if (source->equalsIgnoreCase("'self'")) {
                m_allowSelf = true;
            } else if (source->equalsIgnoreCase("*")) {
                m_allowStar = true;
            } else if (source->equalsIgnoreCase("'unsafe-inline'")) {
                m_allowInline = true;
            } else {
                parseHost(source);
            }

            begin = i + 1;
        }
    }
}

void ContentSecurityPolicySourceListDirective::parseHost(String* source)
{
    size_t length = source->length();

    size_t serverNamePos = source->find(".", 0);
    if (serverNamePos == SIZE_MAX) {
        return;
    }
    size_t domainNamePos = source->find(".", serverNamePos + 1);
    if (domainNamePos == SIZE_MAX) {
        return;
    }

    ContentSecurityPolicySourceURL* sourceURL =
        new ContentSecurityPolicySourceURL();

    bool isStarProtocol = true;
    size_t protocolPos = source->find(":");
    if (protocolPos == SIZE_MAX) {
        sourceURL->m_isStarProtocol = true;
        protocolPos = 0;
    } else {
        if (protocolPos + 1 >= length) {
            return;
        }
        sourceURL->m_protocol = source->substring(0, protocolPos + 1);
        sourceURL->m_isStarProtocol = false;
        while (protocolPos < length) {
            auto c = source->charAt(protocolPos);
            if (c != ':' && c != '/')
                break;
            protocolPos++;
        }
    }

    STARFISH_ASSERT(serverNamePos - protocolPos >= 0);
    sourceURL->m_serverName =
        source->substring(protocolPos, serverNamePos - protocolPos);
    sourceURL->m_isStarServer = sourceURL->m_serverName->equals("*");

    size_t pathPos = source->find("/", domainNamePos);
    if (pathPos != SIZE_MAX && length > pathPos) {
        sourceURL->m_path = source->substring(pathPos, length - pathPos);
    } else {
        pathPos = length;
    }

    STARFISH_ASSERT(length - serverNamePos - 1 >= 0);
    sourceURL->m_domainName =
        source->substring(serverNamePos + 1, pathPos - serverNamePos - 1);

    m_URLSourceList.push_back(sourceURL);
}

bool ContentSecurityPolicySourceListDirective::allowURL(ResourceURL* url)
{
    size_t pos = url->host()->find(".");
    String* serverName = url->host()->substring(0, pos);
    String* domainName =
        url->host()->substring(pos + 1, url->host()->length() - pos - 1);

    for (auto source : m_URLSourceList) {
        if (!source->m_isStarProtocol &&
            !source->m_protocol->equalsIgnoreCase(url->protocol())) {
            continue;
        }
        if (!source->m_isStarServer &&
            !source->m_serverName->equalsIgnoreCase(serverName)) {
            continue;
        }
        if (source->m_domainName->equalsIgnoreCase(domainName)) {
            if (source->m_path->length() > 1) {
                if (!source->m_path->equalsIgnoreCase(url->pathname())) {
                    continue;
                }
            }
            return true;
        }
    }

    return false;
}
}
