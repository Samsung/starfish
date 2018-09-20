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

#ifndef __StarFishResponse__
#define __StarFishResponse__

#include "core/fetch/Body.h"
#include "core/fetch/Headers.h"
#include "core/fetch/GetSet.h"

namespace StarFish {

class Document;

struct ResponseInit {
public:
    ResponseInit()
        : m_status(200)
        , m_statusText(String::createASCIIString("OK"))
        , m_headers()
    {
    }

    GETTER_SETTER(uint32_t, status, Status);
    GETTER_SETTER(String*, statusText, StatusText);
    GETTER_SETTER(HeadersInit, headers, Headers);

private:
    uint32_t m_status;
    String* m_statusText;
    HeadersInit m_headers;
};

class ScriptWrappable;

class Response final : public ScriptWrappable, public Body {
public:
    enum class ResponseType {
        Basic,
        Cors,
        Default,
        Error,
        Opaque,
        Opaqueredirect
    };

    Response(Document* document, uint32_t status = 200,
             std::string type = "default", std::string statusText = "OK");
    Response(Document* document, Nullable<BodyInit>& body);
    Response(Document* document, Nullable<BodyInit>& body, ResponseInit& init);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;

    virtual ScriptBindingInstance* scriptBindingInstance() override
    {
        return m_instance;
    }

    virtual bool isResponse() const override;

    static bool isValidReasonPhrase(String* text);
    static bool isValidRedirectStatus(uint32_t status);

    static Response* error(Document* document);

    static Response* redirect(Document* document, String* url);
    static Response* redirect(Document* document, String* url,
                              unsigned short status);

    GETTER_SETTER(String*, type, Type);
    GETTER_SETTER(String*, url, Url);
    GETTER_SETTER(bool, redirected, Redirected);
    GETTER_SETTER(uint32_t, status, Status);

    bool ok()
    {
        return 200 <= m_status && 299 >= m_status;
    }

    void setOk(int statusCode)
    {
        m_ok = (statusCode >= 200 && statusCode < 300);
    }

    GETTER_SETTER(String*, statusText, StatusText);

    Headers* headers()
    {
        return &m_headers;
    }

    Response* clone();

private:
    ScriptBindingInstance* m_instance;
    ResponseInit m_responseInit;
    Headers m_headers;
    String* m_type;
    String* m_url;
    bool m_redirected;
    bool m_ok;
    uint32_t m_status;
    String* m_statusText;
    String* m_mimeType;

    void handleBodyInit(Nullable<BodyInit>& body);
    void copyResponseData(Response* destResponse);
};
}

#endif
