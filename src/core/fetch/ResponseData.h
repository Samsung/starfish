/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

    ResponseType m_type;
    String* m_url;
    bool m_redirected;
    bool m_ok;
    uint32_t m_status;
    String* m_statusText;
    String* m_mimeType;

    ResponseBody m_responseBody;

    // https://fetch.spec.whatwg.org/#concept-response-cors-exposed-header-name-list
    GCVector<String*> m_corsExposedHeaderNameList;

    static String* reponseTypeString(ResponseType type);
};
}
#endif
