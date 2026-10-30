/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishResponseData__
#define __StarfishResponseData__

#include "core/fetch/stream/ReadableStreamChunk.h"

namespace Starfish {

typedef ReadableStreamChunk ResponseBody;

// https://fetch.spec.whatwg.org/#responsetype
enum class ResponseType { Basic, Cors, Default, Error, Opaque, Opaqueredirect };

class ResponseData : public gc {
public:
    ResponseData();
    ~ResponseData()
    {
    }

    void* operator new(size_t size);
    void clearNativeResources();
    // Objects allocated via GC_finalized_malloc must not be freed with
    // GC_FREE or delete. The no-op operator delete below prevents this.
    void operator delete(void*)
    {
    }
    void operator delete[](void*) = delete;

    ResponseType m_type;
    String* m_url;
    bool m_redirected;
    bool m_ok;
    // The response is "cors" even if its URL is same origin
    // (https://fetch.spec.whatwg.org/#main-fetch): a redirect chain left the
    // request's origin at any hop or ended without a final HTTP response, or
    // a service worker provided the response without its tainting.
    bool m_corsTainted;
    // Set by the network job when the final response of a cross-origin
    // request passed the CORS check
    // (https://fetch.spec.whatwg.org/#concept-cors-check).
    bool m_passedCorsCheck;
    uint32_t m_status;
    String* m_statusText;
    String* m_mimeType;

    ResponseBody m_responseBody;

    // https://fetch.spec.whatwg.org/#concept-response-cors-exposed-header-name-list
    GCVector<String*> m_corsExposedHeaderNameList;

    static String* reponseTypeString(ResponseType type);
};
} // namespace Starfish
#endif
