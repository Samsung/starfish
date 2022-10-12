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

#if defined(STARFISH_ENABLE_SERVICE_WORKER) && \
    !defined(__StarfishServiceWorkerFetchCacheStream__)
#define __StarfishServiceWorkerFetchCacheStream__

namespace Escargot {
class ValueVectorRef;
}

namespace Starfish {

class Response;
class Request;

struct FetchCacheStreamResponseData : public gc {
    std::string mimeType;
    std::vector<char> buffer;

    static FetchCacheStreamResponseData* create(Response* response);
    void applyResponse(Response* response);
};

class FetchCacheStream : public gc {
public:
    FetchCacheStream(const std::string& rootPath);

    bool open(size_t originHashValue, const std::string& cacheName);

    bool writeResponse(size_t urlHashValue, FetchCacheStreamResponseData* data);
    bool writeResponse(Request* request, Response* response);

    bool readResponse(size_t urlHashValue, FetchCacheStreamResponseData* data);
    bool readResponse(String* url, Response* response);

    bool getKeys(size_t cacheScopeDirHash, ValueVectorRef* result);

private:
    std::string m_cacheDirPath;
    std::string m_cacheScopeDirPath;

    std::string getCachePath(size_t urlHashValue);
};
} // namespace Starfish

#endif
