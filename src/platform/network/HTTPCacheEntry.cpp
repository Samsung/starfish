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

#if defined(STARFISH_ENABLE_HTTPCACHE)
#include "StarFishConfig.h"
#include "platform/file/File.h"
#include "HTTPCacheEntry.h"
#include "platform/network/http/HTTPHeaderMap.h"
#include "core/modules/threading/Mutex.h"
#include "core/modules/threading/Locker.h"
#include "core/modules/profiling/Profiling.h"
#include "platform/loader/ResourceURL.h"

namespace StarFish {

const char* HTTPCacheEntry::kSeparator = "\037"; // unit separator

HTTPCacheEntry::HTTPCacheEntry(ResourceURL* url, CacheControl& cacheControl,
                               HTTPContentInfo& cinfo, HTTPFreshnessInfo& finfo)
    : m_url(url)
    , m_cacheControl(cacheControl)
    , m_httpContentInfo(cinfo)
    , m_httpFreshnessInfo(finfo)
    , m_entryFileInfo()
    , m_mutex(new Mutex())
    , m_usingCount(0)
    , m_needsRawDataUpdate(false)
    , m_needsPropertiesUpdate(false)
{
}

HTTPCacheEntry::HTTPCacheEntry(ResourceURL* url, CacheControl& cacheControl,
                               HTTPContentInfo& cinfo, HTTPFreshnessInfo& finfo,
                               EntryFileInfo& einfo)
    : HTTPCacheEntry(url, cacheControl, cinfo, finfo)
{
    m_entryFileInfo = einfo;
}

HTTPCacheEntry::~HTTPCacheEntry()
{
}

HTTPCacheEntry::HTTPCacheEntry(const HTTPCacheEntry& rhs)
{
    if (this == &rhs) {
        return;
    }

    m_url = new ResourceURL(*(rhs.m_url));
    m_cacheControl = rhs.m_cacheControl;
    m_httpContentInfo = rhs.m_httpContentInfo;
    m_httpFreshnessInfo = rhs.m_httpFreshnessInfo;
    m_entryFileInfo = rhs.m_entryFileInfo;

    m_mutex = new Mutex();
    m_usingCount = rhs.m_usingCount;
}

size_t HTTPCacheEntry::entryKey() const
{
    Locker<Mutex> locker(*m_mutex);
    return m_url->urlString()->hashValue();
}

void HTTPCacheEntry::setHTTPContentInfo(HTTPContentInfo& info)
{
    Locker<Mutex> locker(*m_mutex);
    m_httpContentInfo = info;
}

void HTTPCacheEntry::setHTTPFreshnessInfo(HTTPFreshnessInfo& info)
{
    Locker<Mutex> locker(*m_mutex);
    m_httpFreshnessInfo = info;
}

void HTTPCacheEntry::setCacheControl(CacheControl& cc)
{
    Locker<Mutex> locker(*m_mutex);
    m_cacheControl = cc;
}

void HTTPCacheEntry::setEntryFileInfo(EntryFileInfo& info)
{
    Locker<Mutex> locker(*m_mutex);
    m_entryFileInfo = info;
}

void HTTPCacheEntry::setEntryFileNameUsingCachePath(String* cachePath)
{
    Locker<Mutex> locker(*m_mutex);
    auto newPath = cachePath->toUTF8NonGCString();

    newPath.append("/");
    newPath.append(std::to_string(m_url->urlString()->hashValue()));
    newPath.append("_");
    newPath.append(std::to_string(longTickCount()));

    m_entryFileInfo.entryFilePath = newPath;
}

bool HTTPCacheEntry::writeRawDataToEntryFile(std::vector<char>& rawData)
{
    STARFISH_ASSERT(m_entryFileInfo.entryFilePath.compare("") != 0);

    Locker<Mutex> locker(*m_mutex);
    if (rawData.size() == 0) {
        return false;
    }

    File* out = File::create();
    if (!out->open(m_entryFileInfo.entryFilePath, File::Write)) {
        return false;
    }

    size_t length = out->write(rawData.data(), sizeof(char), rawData.size());
    bool ret = (rawData.size() == length) & (out->flush() == 0);

    m_entryFileInfo.lastModificationTime = out->lastModificationTime();
    m_entryFileInfo.byteLength = length;

    return ret & (out->close() == 0);
}

bool HTTPCacheEntry::readRawDataFromEntryFile(std::vector<char>& out)
{
    Locker<Mutex> locker(*m_mutex);

    STARFISH_ASSERT(m_entryFileInfo.entryFilePath.compare("") != 0);

    File* in = File::createInNonGCArea(); // Must free
    if (!in->open(m_entryFileInfo.entryFilePath, File::Read)) {
        return false;
    }

    bool ret = in->readAll(out) & (in->close() == 0);

    free(in);

    return ret;
}

void HTTPCacheEntry::readEntryHeaders(HeaderMap& out)
{
    m_mutex->lock();
    HTTPContentInfo copied = m_httpContentInfo;
    m_mutex->unlock();

    if (copied.contentLanguage.size()) {
        out.insert(std::make_pair(HTTPHeaderMap::kContentLanguage,
                                  copied.contentLanguage));
    }
    if (copied.contentLength) {
        out.insert(std::make_pair(HTTPHeaderMap::kContentLength,
                                  std::to_string(copied.contentLength)));
    }
    if (copied.contentType.size()) {
        out.insert(
            std::make_pair(HTTPHeaderMap::kContentType, copied.contentType));
    }
    if (copied.contentTransferEncoding.size()) {
        out.insert(std::make_pair(HTTPHeaderMap::kContentTransferEncoding,
                                  copied.contentTransferEncoding));
    }
}

bool HTTPCacheEntry::isFresh() const
{
    // https://tools.ietf.org/html/rfc7234#section-4.2
    // See 4.2. Freshness
    // response_is_fresh = (freshnessLifetime > currentAge)
    Locker<Mutex> locker(*m_mutex);
    int64_t responeTime = m_httpFreshnessInfo.responseTime;
    int64_t freshnessLifetime = m_cacheControl.maxAge;
    int64_t apparentAge = (0 > (responeTime - m_httpFreshnessInfo.date))
                              ? 0
                              : (responeTime - m_httpFreshnessInfo.date);
    int64_t responseDelay = (responeTime - m_httpFreshnessInfo.requestTime);
    int64_t correctedAgeValue = (m_httpFreshnessInfo.age + responseDelay);
    int64_t correctedInitialAge =
        (apparentAge > correctedAgeValue) ? apparentAge : correctedAgeValue;
    int64_t residentTime = ((timestamp() / 1000) - responeTime);
    int64_t currentAge = correctedInitialAge + residentTime;

    return freshnessLifetime > currentAge;
}

String* HTTPCacheEntry::toString() const
{
    HTTPCacheEntry* copied;
    {
        Locker<Mutex> locker(*m_mutex);
        copied = new HTTPCacheEntry(*this);
    }
    // The toString order starts with entrykey and then follows the order of
    // each member. therefore, it is as follows :
    //  entryKey(UINT) urlString(STRING) no-cache(0|1) mustRevalidate(0|1)
    //  maxAge(UINT) contentLanguage(STRING) contentLength(UINT)
    //  contentType(STRING) contentTransferEncoding(STRING) date(UINT) age(UINT)
    //  rquestTime(UINT) responeTime(UINT) lastModified(UINT) Etag(STRING)
    //  entryFilePath(STRING) lastModificationTime(UINT) byteLength(UINT)

    StringBuilder builder;
    std::string entryKey =
        std::to_string(copied->m_url->urlString()->hashValue());
    builder.appendString(entryKey.data());
    builder.appendString(kSeparator);

    builder.appendString(url()->urlString());
    builder.appendString(kSeparator);

    // cache-contorl
    copied->m_cacheControl.noCache ? builder.appendString("1")
                                   : builder.appendString("0");
    builder.appendString(kSeparator);
    m_cacheControl.mustRevalidate ? builder.appendString("1")
                                  : builder.appendString("0");
    builder.appendString(kSeparator);
    std::string maxAge = std::to_string(copied->m_cacheControl.maxAge);
    builder.appendString(maxAge.data());
    builder.appendString(kSeparator);

    // http content-xxx
    std::string contentLanguage =
        (copied->m_httpContentInfo.contentLanguage.size())
            ? copied->m_httpContentInfo.contentLanguage
            : "null";
    builder.appendString(contentLanguage.data());
    builder.appendString(kSeparator);
    std::string contentLength =
        std::to_string(copied->m_httpContentInfo.contentLength);
    builder.appendString(contentLength.data());
    builder.appendString(kSeparator);
    std::string contentType = (copied->m_httpContentInfo.contentType.size())
                                  ? copied->m_httpContentInfo.contentType
                                  : "null";
    builder.appendString(contentType.data());
    builder.appendString(kSeparator);
    std::string contentTransferEncoding =
        (copied->m_httpContentInfo.contentTransferEncoding.size())
            ? copied->m_httpContentInfo.contentTransferEncoding
            : "null";
    builder.appendString(contentTransferEncoding.data());
    builder.appendString(kSeparator);

    // http freshness info
    std::string date = std::to_string(copied->m_httpFreshnessInfo.date);
    builder.appendString(date.data());
    builder.appendString(kSeparator);
    std::string age = std::to_string(copied->m_httpFreshnessInfo.age);
    builder.appendString(age.data());
    builder.appendString(kSeparator);
    std::string requestTime =
        std::to_string(copied->m_httpFreshnessInfo.requestTime);
    builder.appendString(requestTime.data());
    builder.appendString(kSeparator);
    std::string responseTime =
        std::to_string(copied->m_httpFreshnessInfo.responseTime);
    builder.appendString(responseTime.data());
    builder.appendString(kSeparator);
    std::string lastModified =
        std::to_string(copied->m_httpFreshnessInfo.lastModified);
    builder.appendString(lastModified.data());
    builder.appendString(kSeparator);
    // See https://tools.ietf.org/html/rfc7232#section-2.3
    std::string etag = (copied->m_httpFreshnessInfo.etag.size())
                           ? copied->m_httpFreshnessInfo.etag
                           : "null";
    builder.appendString(etag.data());
    builder.appendString(kSeparator);

    // entry File info
    builder.appendString(copied->m_entryFileInfo.entryFilePath.data());
    builder.appendString(kSeparator);
    std::string lastModificationTime =
        std::to_string(copied->m_entryFileInfo.lastModificationTime);
    builder.appendString(lastModificationTime.data());
    builder.appendString(kSeparator);
    std::string byteLength = std::to_string(copied->m_entryFileInfo.byteLength);
    builder.appendString(byteLength.data());

    return builder.finalize();
}

void HTTPCacheEntry::increaseUsingCount()
{
    Locker<Mutex> locker(*m_mutex);
    m_usingCount++;
}

void HTTPCacheEntry::decreaseUsingCount()
{
    Locker<Mutex> locker(*m_mutex);
    m_usingCount--;
}

void HTTPCacheEntry::setNeedsRawDataUpdate(bool value)
{
    Locker<Mutex> locker(*m_mutex);
    m_needsRawDataUpdate = value;
}

bool HTTPCacheEntry::needsRawDataUpdate()
{
    Locker<Mutex> locker(*m_mutex);
    return m_needsRawDataUpdate;
}

void HTTPCacheEntry::setNeedsPropertiesUpdate(bool value)
{
    Locker<Mutex> locker(*m_mutex);
    m_needsPropertiesUpdate = value;
}

bool HTTPCacheEntry::needsPropertiesUpdate()
{
    Locker<Mutex> locker(*m_mutex);
    return m_needsPropertiesUpdate;
}

bool HTTPCacheEntry::isConsistent()
{
    File* file = File::create();

    if (file->open(m_entryFileInfo.entryFilePath, File::Read)) {
        if (file->lastModificationTime() ==
                m_entryFileInfo.lastModificationTime &&
            file->size() == m_entryFileInfo.byteLength) {
            file->close();
            return true;
        }
    }
    file->close();
    return false;
}
}
#endif
