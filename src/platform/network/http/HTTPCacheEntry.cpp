/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_HTTPCACHE)
#include "StarfishConfig.h"
#include "platform/file/File.h"
#include "HTTPCacheEntry.h"
#include "platform/network/http/HTTPHeaderMap.h"
#include "core/modules/threading/Mutex.h"
#include "core/modules/threading/Locker.h"
#include "core/modules/profiling/Profiling.h"
#include "platform/loader/ResourceURL.h"
#include "core/modules/threading/Thread.h"

namespace Starfish {

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
    , m_good(true)
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
    char buf[256];

    newPath.append("/");
    snprintf(buf, sizeof(buf), "%zu", m_url->urlString()->hashValue());
    newPath.append(std::string(buf));
    newPath.append("_");
    snprintf(buf, sizeof(buf), "%ju", longTickCount());
    newPath.append(std::string(buf));

    m_entryFileInfo.entryFilePath = newPath;
}

bool HTTPCacheEntry::writeRawDataToEntryFile(std::vector<char>& rawData)
{
    STARFISH_ASSERT(m_entryFileInfo.entryFilePath.compare("") != 0);
    Locker<Mutex> locker(*m_mutex);

    if (rawData.size() == 0) {
        return m_good = false;
    }

    auto out = File::open(m_entryFileInfo.entryFilePath, File::Write);
    if (!out) {
        return m_good = false;
    }

    size_t length = out->write(rawData.data(), sizeof(char), rawData.size());
    bool ret = (rawData.size() == length) & (out->flush() == 0);

    if (ret) {
        m_entryFileInfo.lastModificationTime = out->lastModificationTime();
        m_entryFileInfo.byteLength = length;
    }

    out.reset();
    return m_good = ret;
}

bool HTTPCacheEntry::readRawDataFromEntryFile(std::vector<char>& out)
{
    Locker<Mutex> locker(*m_mutex);

    STARFISH_ASSERT(m_entryFileInfo.entryFilePath.compare("") != 0);

    auto in = File::open(m_entryFileInfo.entryFilePath, File::Read);
    if (!in) {
        return m_good = false;
    }

    m_good = in->readAll(out);

    in.reset();

    return m_good;
}

void HTTPCacheEntry::readEntryHeaders(HeaderMap& out)
{
    char buf[256];
    Locker<Mutex> locker(*m_mutex);

    if (m_httpContentInfo.contentLanguage.size()) {
        out.insert(std::make_pair(HTTPHeaderMap::kContentLanguage,
                                  m_httpContentInfo.contentLanguage));
    }

    if (m_httpContentInfo.contentLength) {
        snprintf(buf, sizeof(buf), "%zu", m_httpContentInfo.contentLength);
        out.insert(
            std::make_pair(HTTPHeaderMap::kContentLength, std::string(buf)));
    }
    if (m_httpContentInfo.contentType.size()) {
        out.insert(std::make_pair(HTTPHeaderMap::kContentType,
                                  m_httpContentInfo.contentType));
    }
    if (m_httpContentInfo.contentTransferEncoding.size()) {
        out.insert(std::make_pair(HTTPHeaderMap::kContentTransferEncoding,
                                  m_httpContentInfo.contentTransferEncoding));
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

static void appendStringHelper(StringBuilder& builder, const char* str)
{
    STARFISH_ASSERT(str != nullptr);
    builder.appendString(str, strlen(str));
}

String* HTTPCacheEntry::toString() const
{
    Locker<Mutex> locker(*m_mutex);
    HTTPCacheEntry* copied = (HTTPCacheEntry*)this;
    // The toString order starts with entrykey and then follows the order of
    // each member. therefore, it is as follows :
    //  entryKey(UINT) urlString(STRING) no-cache(0|1) mustRevalidate(0|1)
    //  maxAge(UINT) contentLanguage(STRING) contentLength(UINT)
    //  contentType(STRING) contentTransferEncoding(STRING) date(UINT) age(UINT)
    //  rquestTime(UINT) responeTime(UINT) lastModified(UINT) Etag(STRING)
    //  entryFilePath(STRING) lastModificationTime(UINT) byteLength(UINT)

    STARFISH_ASSERT(isMainThread() == true);

    char buf[256];
    StringBuilder builder;
    snprintf(buf, sizeof(buf), "%zu", copied->m_url->urlString()->hashValue());
    std::string entryKey(buf);
    builder.appendString(entryKey.data(), entryKey.size());
    appendStringHelper(builder, kSeparator);

    builder.appendString(url()->urlString());
    appendStringHelper(builder, kSeparator);

    // cache-contorl
    copied->m_cacheControl.noCache ? builder.appendString("1")
                                   : builder.appendString("0");
    appendStringHelper(builder, kSeparator);
    m_cacheControl.mustRevalidate ? builder.appendString("1")
                                  : builder.appendString("0");
    appendStringHelper(builder, kSeparator);
    snprintf(buf, sizeof(buf), "%ju", copied->m_cacheControl.maxAge);
    std::string maxAge(buf);

    builder.appendString(maxAge.data(), maxAge.size());
    appendStringHelper(builder, kSeparator);

    // http content-xxx
    std::string contentLanguage =
        (copied->m_httpContentInfo.contentLanguage.size())
            ? copied->m_httpContentInfo.contentLanguage
            : "null";
    builder.appendString(contentLanguage.data(), contentLanguage.size());
    appendStringHelper(builder, kSeparator);
    snprintf(buf, sizeof(buf), "%zu", copied->m_httpContentInfo.contentLength);
    std::string contentLength(buf);
    builder.appendString(contentLength.data(), contentLength.size());
    appendStringHelper(builder, kSeparator);
    std::string contentType = (copied->m_httpContentInfo.contentType.size())
                                  ? copied->m_httpContentInfo.contentType
                                  : "null";
    builder.appendString(contentType.data(), contentType.size());
    appendStringHelper(builder, kSeparator);
    std::string contentTransferEncoding =
        (copied->m_httpContentInfo.contentTransferEncoding.size())
            ? copied->m_httpContentInfo.contentTransferEncoding
            : "null";
    builder.appendString(contentTransferEncoding.data(),
                         contentTransferEncoding.size());
    appendStringHelper(builder, kSeparator);

    // http freshness info
    snprintf(buf, sizeof(buf), "%ju", copied->m_httpFreshnessInfo.date);
    std::string date(buf);
    builder.appendString(date.data(), date.size());
    appendStringHelper(builder, kSeparator);
    snprintf(buf, sizeof(buf), "%ju", copied->m_httpFreshnessInfo.age);
    std::string age(buf);
    builder.appendString(age.data(), age.size());
    appendStringHelper(builder, kSeparator);
    snprintf(buf, sizeof(buf), "%ju", copied->m_httpFreshnessInfo.requestTime);
    std::string requestTime(buf);
    builder.appendString(requestTime.data(), requestTime.size());
    appendStringHelper(builder, kSeparator);
    snprintf(buf, sizeof(buf), "%ju", copied->m_httpFreshnessInfo.responseTime);
    std::string responseTime(buf);
    builder.appendString(responseTime.data(), responseTime.size());
    appendStringHelper(builder, kSeparator);
    snprintf(buf, sizeof(buf), "%ju", copied->m_httpFreshnessInfo.lastModified);
    std::string lastModified(buf);
    builder.appendString(lastModified.data(), lastModified.size());
    appendStringHelper(builder, kSeparator);
    // See https://tools.ietf.org/html/rfc7232#section-2.3
    std::string etag = (copied->m_httpFreshnessInfo.etag.size())
                           ? copied->m_httpFreshnessInfo.etag
                           : "null";
    builder.appendString(etag.data(), etag.size());
    appendStringHelper(builder, kSeparator);

    // entry File info
    builder.appendString(copied->m_entryFileInfo.entryFilePath.data(),
                         copied->m_entryFileInfo.entryFilePath.size());
    appendStringHelper(builder, kSeparator);
    snprintf(buf, sizeof(buf), "%ju",
             copied->m_entryFileInfo.lastModificationTime);
    std::string lastModificationTime(buf);
    builder.appendString(lastModificationTime.data(),
                         lastModificationTime.size());
    appendStringHelper(builder, kSeparator);
    snprintf(buf, sizeof(buf), "%zu", copied->m_entryFileInfo.byteLength);
    std::string byteLength(buf);
    builder.appendString(byteLength.data(), byteLength.size());

    return builder.finalize();
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
    STARFISH_ASSERT(isMainThread());

    if (!m_good) {
        return m_good;
    }

    auto file = File::open(m_entryFileInfo.entryFilePath, File::Read);

    if (file) {
        if (file->lastModificationTime() ==
                m_entryFileInfo.lastModificationTime &&
            file->size() == m_entryFileInfo.byteLength) {
            return true;
        }
    }
    return m_good = false;
}
bool HTTPCacheEntry::good()
{
    STARFISH_ASSERT(isMainThread());
    return m_good;
}
void HTTPCacheEntry::setToBad()
{
    STARFISH_ASSERT(isMainThread());
    m_good = false;
}
} // namespace Starfish
#endif
