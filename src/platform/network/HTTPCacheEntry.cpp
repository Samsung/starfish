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
#include "platform/file/File.h"
#include "HTTPCacheEntry.h"
#include "core/modules/threading/Mutex.h"
#include "core/modules/threading/Locker.h"
#include "core/modules/profiling/Profiling.h"
#include "platform/loader/ResourceURL.h"

namespace StarFish {

HTTPCacheEntry::HTTPCacheEntry(ResourceURL* url, EntryFreshnessInfo& info,
                               CacheControl& cacheControl)
    : m_url(url)
    , m_entryFreshnessInfo(info)
    , m_cacheControl(cacheControl)
    , m_entryFileName(nullptr)
    , m_mutex(new Mutex())
{
}

HTTPCacheEntry::HTTPCacheEntry(ResourceURL* url, EntryFreshnessInfo& info,
                               CacheControl& cacheControl,
                               String* entryFileName)
    : HTTPCacheEntry(url, info, cacheControl)
{
    m_entryFileName = entryFileName;
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

    Locker<Mutex> locker(*m_mutex);
    if (rawData.size() == 0) {
        return false;
    }

    File* out = File::create();
    if (!out->open(m_entryFileName, File::Write)) {
        return false;
    }

    size_t writeSize = out->write(rawData.data(), sizeof(char), rawData.size());
    bool ret = (rawData.size() == writeSize) & (out->flush() == 0);

    return ret & (out->close() == 0);
}

bool HTTPCacheEntry::readRawDataFromEntryFile(std::vector<char>& out)
{
    STARFISH_ASSERT(m_entryFileName != String::emptyString);
    Locker<Mutex> locker(*m_mutex);

    File* in = File::createInNonGCArea(); // Must free
    if (!in->open(m_entryFileName, File::Read)) {
        return false;
    }

    bool ret = in->readAll(out) & (in->close() == 0);

    free(in);

    return ret;
}

String* HTTPCacheEntry::toString()
{
    StringBuilder builder;
    std::string entryKeystr = std::to_string(entryKey());
    std::string dateStr = std::to_string(m_entryFreshnessInfo.date);
    std::string ageStr = std::to_string(m_entryFreshnessInfo.age);
    std::string requestTimeStr =
        std::to_string(m_entryFreshnessInfo.requestTime);
    std::string responseTimeStr =
        std::to_string(m_entryFreshnessInfo.responseTime);
    std::string contentLengthStr =
        std::to_string(m_entryFreshnessInfo.contentLength);
    std::string maxAgeStr = std::to_string(m_cacheControl.maxAge);

    // entryKey(UINT) urlString(STRING) date(UINT) age(UINT)
    // rquestTime(UINT) responeTime(UINT) contentLength(UINT)
    // maxAge(UINT) no-cache(0|1) mustRevalidate(0|1) entryFileName(STRING)

    builder.appendString(entryKeystr.data());
    builder.appendString(" ");
    builder.appendString(url()->urlString());
    builder.appendString(" ");
    builder.appendString(dateStr.data());
    builder.appendString(" ");
    builder.appendString(ageStr.data());
    builder.appendString(" ");
    builder.appendString(requestTimeStr.data());
    builder.appendString(" ");
    builder.appendString(responseTimeStr.data());
    builder.appendString(" ");
    builder.appendString(contentLengthStr.data());
    builder.appendString(" ");
    builder.appendString(maxAgeStr.data());
    builder.appendString(" ");
    m_cacheControl.noCache ? builder.appendString("1")
                           : builder.appendString("0");
    builder.appendString(" ");
    m_cacheControl.mustRevalidate ? builder.appendString("1")
                                  : builder.appendString("0");
    builder.appendString(" ");
    builder.appendString(entryFileName());

    return builder.finalize();
}
}
#endif
