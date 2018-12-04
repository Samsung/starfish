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
#include "core/fetch/GetSet.h"
#include "core/fetch/ResponseData.h"

namespace Starfish {

class Document;

struct ResponseInit {
public:
    ResponseInit()
        : m_status(0)
        , m_statusText(String::emptyString)
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
    Response(Document* document);
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

    Headers* headers()
    {
        return &m_headers;
    }

    Response* clone();

    void setBody(String* string);

private:
    ScriptBindingInstance* m_instance;
    Headers m_headers;
    ResponseData m_responseData;

    void handleBodyInit(Nullable<BodyInit>& body);
    void copyResponseData(Response* destResponse);
};
}

#endif
