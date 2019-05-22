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

#ifndef __StarfishResponse__
#define __StarfishResponse__

#include "core/fetch/Body.h"
#include "core/fetch/Headers.h"

namespace Starfish {

class ExecutionContext;
class ResponseData;

struct ResponseInit {
public:
    ResponseInit()
    {
    }

    DEFINE_GETTER_SETTER(uint32_t, status, Status);
    DEFINE_GETTER_SETTER(String*, statusText, StatusText);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(HeadersInit, headers, Headers);

private:
    uint32_t m_status{ 0 };
    String* m_statusText{ String::emptyString };

    bool m_hasHeaders{ false };
    HeadersInit m_headers{ scriptUndefined() };
};

class ScriptWrappable;

class Response final : public ScriptWrappable, public Body {
public:
    Response(ExecutionContext* executionContext);
    Response(ExecutionContext* executionContext, Nullable<BodyInit>& body);
    Response(ExecutionContext* executionContext, Nullable<BodyInit>& body,
             ResponseInit& init);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(Response)

    static bool isValidReasonPhrase(String* text);
    static bool isValidRedirectStatus(uint32_t status);

    static Response* error(ExecutionContext* executionContext);

    static Response* redirect(ExecutionContext* executionContext, String* url);
    static Response* redirect(ExecutionContext* executionContext, String* url,
                              unsigned short status);

    String* url();
    void setUrl(String* url);

    bool redirected();
    void setRedirected(bool value);

    uint32_t status();
    void setStatus(uint32_t);

    void setType(ResponseType type);
    ResponseType typeValue();
    String* type();

    String* statusText();
    void setStatusText(String* statusText);

    String* mimeType();
    void setMimeType(String* mimeType);

    bool ok();
    void setOk(bool ok);

    Headers* headers()
    {
        return &m_headers;
    }

    Response* clone();

    void setBody(String* string);

private:
    ExecutionContext* m_executionContext;
    Headers m_headers;
    ResponseData* m_responseData;

    void handleBodyInit(Nullable<BodyInit>& body);
    void copyResponseData(Response* destResponse);
};
}

#endif
