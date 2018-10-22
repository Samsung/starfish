/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include "HTTPHeaderMap.h"
#include <curl/curl.h>
namespace StarFish {

#define DEFINE_HEADERS(name, value) const char HTTPHeaderMap::name[] = value;
FOR_EACH_HTTPHEADERS(DEFINE_HEADERS);
#undef DEFINE_HEADERS

HTTPHeaderMap::HTTPHeaderMap()
{
}

HTTPHeaderMap::~HTTPHeaderMap()
{
}

HTTPHeaderMap::HTTPHeaderMap(const HTTPHeaderMap& rhs)
{
    *this = rhs;
}

HTTPHeaderMap& HTTPHeaderMap::operator=(const HTTPHeaderMap& rhs)
{
    if (this == &rhs) {
        return *this;
    }
    this->m_headerMap = rhs.m_headerMap;

    return *this;
}

unsigned long HTTPHeaderMap::length()
{
    return m_headerMap.size();
}

HeaderMap::iterator HTTPHeaderMap::findHeader(const std::string& key)
{
    HeaderMap::iterator it = m_headerMap.find(key);
    if (it == m_headerMap.end()) {
        return m_headerMap.end();
    }
    return it;
}

HeaderMap::const_iterator HTTPHeaderMap::findHeader(
    const std::string& key) const
{
    HeaderMap::const_iterator it = m_headerMap.find(key);
    if (it == m_headerMap.end()) {
        return m_headerMap.end();
    }
    return it;
}

void HTTPHeaderMap::setHeader(const std::string& key, const std::string& value)
{
    auto it = findHeader(key);
    if (it == m_headerMap.end()) {
        m_headerMap[key] = value;
    } else {
        m_headerMap[key] += ", " + value;
    }
}

void HTTPHeaderMap::removeHeader(const std::string& key)
{
    m_headerMap.erase(key);
}

void HTTPHeaderMap::clear()
{
    m_headerMap.clear();
}

struct curl_slist* HTTPHeaderMap::generateCurlList()
{
    curl_slist* list = nullptr;
    std::string header;
    for (auto it = m_headerMap.begin(); it != m_headerMap.end(); ++it) {
        header = std::string(it->first) + ": " + it->second;
        list = curl_slist_append(list, header.data());
        if (list == nullptr) {
            STARFISH_ASSERT_NOT_REACHED();
        }
    }

    return list;
}

#ifdef STARFISH_ENABLE_TEST
void HTTPHeaderMap::dump()
{
    STARFISH_LOG_INFO("dump header map\n");
    for (auto it = m_headerMap.begin(); it != m_headerMap.end(); ++it) {
        STARFISH_LOG_INFO("key : %s\n", it->first.c_str());
        STARFISH_LOG_INFO("value : %s\n", it->second.c_str());
    }
}
#endif
}
