/*
 * Copyright (c) 2022-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_SERVICE_WORKER)

#include "StarfishConfig.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/profiling/Profiling.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/modules/serviceworker/util/LocalStorageHelper.h"
#include "core/modules/serviceworker/ServiceWorkerAgent.h"
#include "core/modules/serviceworker/WorkerConfig.h"
#include "core/fetch/Response.h"
#include "core/fetch/ResponseData.h"
#include "core/fetch/Body.h"
#include "core/fetch/Request.h"
#include "core/fetch/stream/ReadableStreamBuffer.h"
#include "core/fetch/stream/ReadableStream.h"
#include "core/modules/serviceworker/FetchCacheStream.h"

namespace Starfish {

FetchCacheStreamResponseData* FetchCacheStreamResponseData::create(
    Response* response)
{
    STARFISH_ASSERT(isMainThread());

    auto data = new FetchCacheStreamResponseData;
    data->mimeType = response->mimeType()->toUTF8NonGCString();
    auto responseBuffer = response->body()->streamBuffer()->buffer();
    data->buffer.assign(responseBuffer.begin(), responseBuffer.end());
    return data;
}

void FetchCacheStreamResponseData::applyResponse(Response* response)
{
    STARFISH_ASSERT(isMainThread());

    response->setMimeType(String::fromUTF8(mimeType.data(), mimeType.size()));
    response->createReadableStream();
    response->body()->streamBuffer()->buffer().assign(buffer.begin(),
                                                      buffer.end());
    if (!cachePath.empty()) {
        response->setCachePath(cachePath);
    }
}

FetchCacheStream::FetchCacheStream(const std::string& rootPath,
                                   bool useComplexKey)
    : m_cacheDirPath(rootPath)
    , m_useComplexKey(useComplexKey)
{
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            FetchCacheStream* self = static_cast<FetchCacheStream*>(obj);
            self->~FetchCacheStream();
        },
        nullptr, nullptr, nullptr);
}

FetchCacheStream::~FetchCacheStream()
{
    //  Destructors should be called for members that allocate memory
    //  internally, such as std::string, but are not gc targets.
}

bool FetchCacheStream::open(size_t originHashValue,
                            const std::string& cacheName)
{
    if (cacheName.empty()) {
        return false;
    }

    auto hashValue = std::to_string(originHashValue);
    m_cacheDirPath = m_cacheDirPath + "/" + hashValue;
    LocalStorageHelper::File::mkdirIfNotExists(m_cacheDirPath);

    m_cacheScopeDirPath = m_cacheDirPath;

    m_cacheDirPath = m_cacheDirPath + "/" + cacheName;
    LocalStorageHelper::File::mkdirIfNotExists(m_cacheDirPath);

    return true;
}

bool FetchCacheStream::open(const std::string& dirName)
{
    m_cacheDirPath = m_cacheDirPath + "/" + dirName;
    LocalStorageHelper::File::mkdirIfNotExists(m_cacheDirPath);

    return true;
}

bool FetchCacheStream::writeResponse(size_t urlHashValue,
                                     FetchCacheStreamResponseData* data)
{
    TRACE(SVCWORKER);
    auto path = getCachePath(urlHashValue);

    LocalStorageHelper::Writer writer(path);
    RETURN_FALSE_IF_FAILED(writer.writeString(data->mimeType));

    RETURN_FALSE_IF_FAILED(writer.writeVector(data->buffer));

    data->cachePath = path;

    return true;
}

bool FetchCacheStream::writeResponse(size_t urlHashValue, Response* response)
{
    STARFISH_ASSERT(isMainThread());
    TRACE(SVCWORKER);

    auto path = getCachePath(urlHashValue);
    response->createReadableStream();
    auto streamBuffer = response->body()->streamBuffer();

    LocalStorageHelper::Writer writer(path);
    RETURN_FALSE_IF_FAILED(writer.writeString(streamBuffer->mineType()));

    RETURN_FALSE_IF_FAILED(writer.writeVector(streamBuffer->buffer()));

    response->setCachePath(path);

    return true;
}

bool FetchCacheStream::writeResponse(Request* request, Response* response)
{
    return writeResponse(request->url()->hashValue(), response);
}

bool FetchCacheStream::readResponse(size_t urlHashValue,
                                    FetchCacheStreamResponseData* data)
{
    TRACE(SVCWORKER);
    auto path = getCachePath(urlHashValue);

    LocalStorageHelper::Reader reader(path);
    RETURN_FALSE_IF_FAILED(reader.readString(data->mimeType));
    RETURN_FALSE_IF_FAILED(reader.readVector(data->buffer));

    data->cachePath = path;

    return true;
}

bool FetchCacheStream::readResponse(String* url, Response* response)
{
    STARFISH_ASSERT(isMainThread());
    TRACE(SVCWORKER);

    auto path = getCachePath(url->hashValue());
    response->createReadableStream();
    auto streamBuffer = response->body()->streamBuffer();

    LocalStorageHelper::Reader reader(path);
    String* mineType = nullptr;
    RETURN_FALSE_IF_FAILED(reader.readString(mineType));
    response->setMimeType(mineType);

    RETURN_FALSE_IF_FAILED(reader.readVector(streamBuffer->buffer()));

    response->setCachePath(path);

    return true;
}

bool FetchCacheStream::readResponseFromFile(const std::string& path,
                                            ResourceRequest* resourceRequest)
{
    if (!LocalStorageHelper::File::exists(path)) {
        return false;
    }
    TRACE(SVCWORKER);

    LocalStorageHelper::Reader reader(path);
    RETURN_FALSE_IF_FAILED(
        reader.readString(resourceRequest->m_responseData->m_mimeType));
    RETURN_FALSE_IF_FAILED(
        reader.readVector(resourceRequest->m_responseData->m_responseBody));

    return true;
}

bool FetchCacheStream::getKeys(size_t cacheScopeDirHash,
                               Escargot::ValueVectorRef* result)
{
    STARFISH_ASSERT(result != nullptr);

    std::vector<std::string> entries;

    std::string cacheScopeDirPath =
        m_cacheDirPath + "/" + std::to_string(cacheScopeDirHash);

    if (!LocalStorageHelper::File::getFileNamesInDirectory(
            entries, cacheScopeDirPath,
            LocalStorageHelper::File::Type::DIRECTORY)) {
        return false;
    }

    for (const auto& entry : entries) {
        result->pushBack(
            StringRef::createFromUTF8(entry.data(), entry.length()));
    }
    return true;
}

std::string FetchCacheStream::getCachePath(size_t urlHashValue)
{
    auto path = m_cacheDirPath + "/" + std::to_string(urlHashValue);
    if (m_useComplexKey) {
        path.append("_").append(std::to_string(longTickCount()));
    }

    return path;
}

} // namespace Starfish

#endif
