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
}

FetchCacheStream::FetchCacheStream()
    : m_cacheDirPath(ServiceWorkerAgent::localStorageRootDir())
{
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

    m_cacheDirPath = m_cacheDirPath + "/" + cacheName;
    LocalStorageHelper::File::mkdirIfNotExists(m_cacheDirPath);

    return true;
}

bool FetchCacheStream::writeResponse(size_t urlHashValue,
                                     FetchCacheStreamResponseData* data)
{
    auto path = getCachePath(urlHashValue);

    LocalStorageHelper::Writer writer(path);
    RETURN_FALSE_IF_FAILED(writer.writeString(data->mimeType));

    RETURN_FALSE_IF_FAILED(writer.writeVector(data->buffer));

    return true;
}

bool FetchCacheStream::writeResponse(Request* request, Response* response)
{
    STARFISH_ASSERT(isMainThread());

    auto path = getCachePath(request->url()->hashValue());
    auto streamBuffer = response->body()->streamBuffer();

    LocalStorageHelper::Writer writer(path);
    RETURN_FALSE_IF_FAILED(writer.writeString(streamBuffer->mineType()));

    RETURN_FALSE_IF_FAILED(writer.writeVector(streamBuffer->buffer()));

    return true;
}

bool FetchCacheStream::readResponse(size_t urlHashValue,
                                    FetchCacheStreamResponseData* data)
{
    auto path = getCachePath(urlHashValue);

    LocalStorageHelper::Reader reader(path);
    RETURN_FALSE_IF_FAILED(reader.readString(data->mimeType));
    RETURN_FALSE_IF_FAILED(reader.readVector(data->buffer));

    return true;
}

bool FetchCacheStream::readResponse(String* url, Response* response)
{
    STARFISH_ASSERT(isMainThread());

    auto path = getCachePath(url->hashValue());
    response->createReadableStream();
    auto streamBuffer = response->body()->streamBuffer();

    LocalStorageHelper::Reader reader(path);
    String* mineType = nullptr;
    RETURN_FALSE_IF_FAILED(reader.readString(mineType));
    response->setMimeType(mineType);

    RETURN_FALSE_IF_FAILED(reader.readVector(streamBuffer->buffer()));

    return true;
}

std::string FetchCacheStream::getCachePath(size_t urlHashValue)
{
    return m_cacheDirPath + "/" + std::to_string(urlHashValue);
}

} // namespace Starfish

#endif
