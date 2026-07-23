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

#include "StarfishConfig.h"
#include "ResponseData.h"

namespace Starfish {

static void responseDataClear(void* obj, void* cd)
{
    ResponseData* self = reinterpret_cast<ResponseData*>(obj);
    self->clearNativeResources();
}

void* ResponseData::operator new(size_t size)
{
    constexpr static GC_finalizer_closure data = { responseDataClear, nullptr };
    return GC_finalized_malloc(size, &data);
}

void ResponseData::clearNativeResources()
{
    ResponseBody().swap(m_responseBody);
}

// https://fetch.spec.whatwg.org/#responses
ResponseData::ResponseData()
    : m_type(ResponseType::Default)
    , m_url(String::emptyString)
    , m_redirected(false)
    , m_ok(false)
    , m_status(200)
    , m_statusText(String::emptyString)
    , m_mimeType(String::emptyString)
    , m_responseBody()
    , m_corsExposedHeaderNameList()
{
}

String* ResponseData::reponseTypeString(ResponseType type)
{
    switch (type) {
    case ResponseType::Basic:
        return String::createASCIIString("basic");
    case ResponseType::Cors:
        return String::createASCIIString("cors");
    case ResponseType::Default:
        return String::createASCIIString("default");
    case ResponseType::Error:
        return String::createASCIIString("error");
    case ResponseType::Opaque:
        return String::createASCIIString("opaque");
    case ResponseType::Opaqueredirect:
        return String::createASCIIString("opaqueredirect");
    default:
        STARFISH_ASSERT_NOT_REACHED();
        return String::emptyString;
    }
}
} // namespace Starfish
