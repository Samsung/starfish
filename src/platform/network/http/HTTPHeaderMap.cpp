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

#include "StarfishConfig.h"
#include "HTTPHeaderMap.h"
#include <curl/curl.h>
namespace Starfish {

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

HeaderMap::iterator HTTPHeaderMap::findHeader(const std::string& name)
{
    HeaderMap::iterator it = m_headerMap.find(name);
    if (it == m_headerMap.end()) {
        return m_headerMap.end();
    }
    return it;
}

HeaderMap::const_iterator HTTPHeaderMap::findHeader(
    const std::string& name) const
{
    HeaderMap::const_iterator it = m_headerMap.find(name);
    if (it == m_headerMap.end()) {
        return m_headerMap.end();
    }
    return it;
}

bool HTTPHeaderMap::extractHeaderListValues(std::vector<std::string>& out,
                                            const std::string& name) const
{
    auto it = m_headerMap.find(name);
    if (it == m_headerMap.end()) {
        return true;
    }

    const char seperator = ',';
    const auto& values = it->second;
    std::string::size_type prev_pos = 0, pos = 0;
    while ((pos = values.find(seperator, pos)) != std::string::npos) {
        std::string value(values.substr(prev_pos, pos - prev_pos));
        // FIXME :
        // Let extract be the result of extracting header values from header.
        // If extract is failure, then return failure.
        StringUtils::trim(value);
        out.push_back(value);
        prev_pos = ++pos;
    }
    out.push_back(values.substr(prev_pos, pos - prev_pos)); // Last word
    return true;
}

void HTTPHeaderMap::setHeader(const std::string& name, const std::string& value)
{
    auto it = findHeader(name);
    if (it == m_headerMap.end()) {
        m_headerMap[name] = value;
    } else {
        m_headerMap[name] += ", " + value;
    }
}

void HTTPHeaderMap::removeHeader(const std::string& name)
{
    m_headerMap.erase(name);
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

static bool isClientHeader(const std::string& name)
{
    if (name == HTTPHeaderMap::kAcceptLanguage ||
        name == HTTPHeaderMap::kUserAgent || name == HTTPHeaderMap::kHost ||
        name == HTTPHeaderMap::kOrigin) {
        return true;
    }
    return false;
}

curl_slist* HTTPHeaderMap::generateCurlListToPreflightRequest()
{
    curl_slist* list = nullptr;
    std::string header;
    for (auto it = m_headerMap.begin(); it != m_headerMap.end(); ++it) {
        if (isClientHeader(it->first)) {
            header = std::string(it->first) + ": " + it->second;
            list = curl_slist_append(list, header.data());
            if (list == nullptr) {
                STARFISH_ASSERT_NOT_REACHED();
            }
        }
    }
    return list;
}

#ifdef STARFISH_ENABLE_TEST
void HTTPHeaderMap::dump()
{
    STARFISH_LOG_INFO("dump header map\n");
    for (auto it = m_headerMap.begin(); it != m_headerMap.end(); ++it) {
        STARFISH_LOG_INFO("name : %s\n", it->first.c_str());
        STARFISH_LOG_INFO("value : %s\n", it->second.c_str());
    }
}
#endif
}
