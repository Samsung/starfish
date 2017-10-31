/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#if defined(STARFISH_ENABLE_HTTPCACHE)
#include "StarFishConfig.h"
#include "HTTPCacheEntry.h"
#include "core/modules/profiling/Profiling.h"
#include "platform/loader/ResourceURL.h"

#include <fstream>
#include <iostream>
namespace StarFish {

HTTPCacheEntry::HTTPCacheEntry(ResourceURL* url, time_t date, time_t maxAge)
    : HTTPCacheEntry(url, date, maxAge, String::emptyString)
{
}

HTTPCacheEntry::HTTPCacheEntry(ResourceURL* url, time_t date, time_t maxAge,
                               String* entryFileName)
    : m_url(url)
    , m_maxAge(maxAge)
    , m_entryFileName(entryFileName)
{
}

HTTPCacheEntry::~HTTPCacheEntry()
{
}

size_t HTTPCacheEntry::entryKey() const
{
    return m_url->urlString()->hashValue();
}

void HTTPCacheEntry::setEntryFileNameUsingCachePath(String* cachePath)
{
    StringBuilder builder;
    std::string entryKeystr = std::to_string(entryKey());
    std::string tcnt = std::to_string(longTickCount());

    builder.appendString(cachePath);
    builder.appendString("/");
    builder.appendString(entryKeystr.data());
    builder.appendString("_");
    builder.appendString(tcnt.data());

    m_entryFileName = builder.finalize();
}

bool HTTPCacheEntry::writeRawDataToEntryFile(std::vector<char>& rawData)
{
    STARFISH_ASSERT(m_entryFileName != String::emptyString);

    std::ofstream ofs(m_entryFileName->toUTF8NonGCString().data());

    if (!ofs.good()) {
        return false;
    }

    ofs.write(rawData.data(), rawData.size());
    ofs.flush();
    ofs.close();

    if (!ofs.good()) {
        return false;
    }

    return true;
}

String* HTTPCacheEntry::toString()
{
    StringBuilder builder;
    std::string entryKeystr = std::to_string(entryKey());
    std::string maxAgeStr = std::to_string(maxAge());

    builder.appendString(entryKeystr.data());
    builder.appendString(" ");
    builder.appendString(url()->urlString());
    builder.appendString(" ");
    builder.appendString(maxAgeStr.data());
    builder.appendString(" ");
    builder.appendString(entryFileName());

    return builder.finalize();
}
}
#endif
