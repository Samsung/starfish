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

namespace Starfish {

class Response;
class Request;

class FetchCacheStream : public gc {
public:
    FetchCacheStream();

    bool open(String* origin, String* cacheName);

    bool writeResponse(String* url, Response* response);
    bool writeResponse(Request* request, Response* response);

    bool readResponse(String* url, Response* response);

private:
    String* m_cacheDirPath;

    bool createDirIfNotExists(String* path);

    std::string getCachePath(String* url);
};
} // namespace Starfish

#endif
