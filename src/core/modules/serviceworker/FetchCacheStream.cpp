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

#include <fstream>
#include <iostream>

#include "StarfishConfig.h"
#include "platform/file/PlatformDirectory.h"
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

class Writer {
public:
    Writer(std::string& path)
    {
        TRACEF(FETCHSTREAM, path.data());
        m_fileStream.open(path.data(), std::ios::binary);
    }

    ~Writer()
    {
        m_fileStream.close();
    }

    template <typename T>
    bool write(const T& value)
    {
        if (!m_fileStream.is_open()) {
            return false;
        }

        TRACE(FETCHSTREAM, "write", value);

        m_fileStream << value;

        return true;
    }

    bool writeBuffer(const char* buffer, const size_t size)
    {
        if (!m_fileStream.is_open()) {
            return false;
        }

        TRACEF(FETCHSTREAM, "size(%zu)", size);

        m_fileStream << size;
        m_fileStream.write(buffer, sizeof(char) * size);
        return true;
    }

    bool writeString(String* string)
    {
        if (!string->bufferAccessData().hasASCIIData()) {
            return false;
        }

        auto utf8String = string->toUTF8NonGCString();

        return writeBuffer(utf8String.data(), utf8String.size());
    }

    bool writeString(const std::string& string)
    {
        return writeBuffer(string.data(), string.size());
    }

    bool writeVector(const std::vector<char>& vector)
    {
        return writeBuffer(vector.data(), vector.size());
    }

private:
    std::ofstream m_fileStream;
};

class Reader {
public:
    Reader(std::string& path)
    {
        TRACEF(FETCHSTREAM, path.data());
        m_fileStream.open(path.data(), std::ios::binary);
    }

    ~Reader()
    {
        m_fileStream.close();
    }

    template <typename T>
    bool read(T& value)
    {
        if (!m_fileStream.is_open()) {
            return false;
        }

        TRACE(FETCHSTREAM, value);

        m_fileStream >> value;

        return true;
    }

    bool readString(String*& string)
    {
        if (!m_fileStream.is_open()) {
            return false;
        }

        size_t size = 0;
        m_fileStream >> size;
        if (size > 0) {
            TRACEF(FETCHSTREAM, "size(%zu)", size);

            auto buffer = reinterpret_cast<char*>(calloc(1, size));
            m_fileStream.read(buffer, sizeof(char) * size);
            string = String::fromUTF8(buffer, size);
            free(reinterpret_cast<void*>(buffer));
        }

        return true;
    }

    bool readVector(std::vector<char>& vector)
    {
        if (!m_fileStream.is_open()) {
            return false;
        }

        STARFISH_ASSERT(vector.size() == 0);

        size_t size = 0;
        m_fileStream >> size;
        if (size > 0) {
            TRACEF(FETCHSTREAM, "size(%zu)", size);

            vector.resize(size);
            m_fileStream.read(vector.data(), sizeof(char) * size);
        }

        return true;
    }

private:
    std::ifstream m_fileStream;
};

FetchCacheStream::FetchCacheStream()
{
    m_cacheDirPath = String::fromUTF8("");
    m_cacheDirPath =
        m_cacheDirPath->concat(ServiceWorkerAgent::instance()->cachesRootDir());
}

bool FetchCacheStream::open(String* origin, String* cacheName)
{
    if (origin->isEmpty() || cacheName->isEmpty()) {
        return false;
    }

    auto hashValue = std::to_string(origin->hashValue());
    m_cacheDirPath = m_cacheDirPath->concat("/")->concat(
        String::fromUTF8(hashValue.data(), hashValue.length()));
    if (!createDirIfNotExists(m_cacheDirPath)) {
        return false;
    }

    m_cacheDirPath = m_cacheDirPath->concat("/")->concat(cacheName);
    if (!createDirIfNotExists(m_cacheDirPath)) {
        return false;
    }

    return true;
}

bool FetchCacheStream::writeResponse(String* url, Response* response)
{
    auto path = getCachePath(url);
    auto streamBuffer = response->body()->streamBuffer();

    Writer writer(path);
    RETURN_FALSE_IF_FAILED(writer.writeString(streamBuffer->mineType()));

    RETURN_FALSE_IF_FAILED(writer.writeVector(streamBuffer->buffer()));

    return true;
}

bool FetchCacheStream::writeResponse(Request* request, Response* response)
{
    return writeResponse(request->url(), response);
}

bool FetchCacheStream::readResponse(String* url, Response* response)
{
    auto path = getCachePath(url);
    response->createReadableStream();
    auto streamBuffer = response->body()->streamBuffer();

    Reader reader(path);
    String* mineType = nullptr;
    RETURN_FALSE_IF_FAILED(reader.readString(mineType));
    response->setMimeType(mineType);

    RETURN_FALSE_IF_FAILED(reader.readVector(streamBuffer->buffer()));

    return true;
}

bool FetchCacheStream::createDirIfNotExists(String* path)
{
    PlatformDirectory* dir = PlatformDirectory::create();

    if (dir->open(path)) {
        return dir->close();
    }

    TRACE(FETCHSTREAM, CSTR(path));
    return dir->mkDir();
}

std::string FetchCacheStream::getCachePath(String* url)
{
    return m_cacheDirPath->toUTF8NonGCString() + "/" +
           std::to_string(url->hashValue());
}

} // namespace Starfish

#endif
