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

#ifndef __StarfishRequest__
#define __StarfishRequest__

#include "binding/ScriptWrappable.h"
#include "core/fetch/RequestInit.h"
#include "core/fetch/Body.h"
#include "core/fetch/RequestData.h"

namespace Starfish {

class RequestOrUSVString;
typedef RequestOrUSVString RequestInfo;

class Request : public ScriptWrappable {
public:
    Request(ExecutionContext* executionContext, RequestInfo& input);
    Request(ExecutionContext* executionContext, RequestInfo& input,
            RequestInit& init);
    Request(ExecutionContext* executionContext, RequestData* data);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(Request)

    String* method();
    String* url();
    String* destination();
    String* referrer();
    String* referrerPolicy();

    String* mode();
    String* credentials();
    String* cache();
    String* redirect();
    String* integrity();

    Headers* headers();

    bool keepalive();
    bool isReloadNavigation();
    bool isHistoryNavigation();

    Request* clone();

    RequestData* requestData()
    {
        return m_data;
    }

    Body* requestBody() const
    {
        return m_body;
    }

    Nullable<BodyInit> bodyInit() const
    {
        return m_body->bodyInit();
    }

    ExecutionContext* executionContext()
    {
        return m_body->executionContext();
    }

    void createReadableStream()
    {
        return m_body->createReadableStream();
    }

    // IDL Body getters
    Promise* arrayBuffer()
    {
        return m_body->arrayBuffer();
    }
    Promise* blob()
    {
        return m_body->blob();
    }
    Promise* formData()
    {
        return m_body->formData();
    }
    Promise* json()
    {
        return m_body->json();
    }
    Promise* text()
    {
        return m_body->text();
    }
    bool bodyUsed()
    {
        return m_body->bodyUsed();
    }
    ReadableStream* body()
    {
        return m_body->body();
    }

private:
    void initialize(RequestInfo* input, NULLABLE RequestInit* init = nullptr);

    void buildRequestInit(RequestInit* init, String* fallbackMode);

    void checkMethodCanHaveBody();

protected:
    RequestData* m_data;
    Headers* m_headers;
    Body* m_body;
};
} // namespace Starfish

#endif
