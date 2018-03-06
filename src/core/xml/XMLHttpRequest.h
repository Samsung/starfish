/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishXMLHttpRequest__
#define __StarFishXMLHttpRequest__

#include "core/dom/EventTarget.h"
#include "core/modules/resource_request/ResourceRequest.h"

namespace StarFish {

class XMLHttpRequestEventTarget : public EventTarget {
public:
    XMLHttpRequestEventTarget(Document* document)
        : EventTarget(document)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isXMLHttpRequestEventTarget() const override;

#define VIRTUAL
#define OVERRIDE
    DECLARE_EVENT_LISTENER(loadstart);
    DECLARE_EVENT_LISTENER(progress);
    DECLARE_EVENT_LISTENER(abort);
    DECLARE_EVENT_LISTENER(error);
    DECLARE_EVENT_LISTENER(load);
    DECLARE_EVENT_LISTENER(timeout);
    DECLARE_EVENT_LISTENER(loadend);
#undef VIRTUAL
#undef OVERRIDE
};

class XMLHttpRequest : public XMLHttpRequestEventTarget {
    friend class XMLHttpRequestEventEmitter;
    friend class XMLHttpRequestResourceRequestClient;

public:
    XMLHttpRequest(::StarFish::Document* document);

    enum ResponseType { Unspecified, Text, ArrayBuffer, Document, Blob, Json };

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isXMLHttpRequest() const override;

    ResourceRequest* resourceRequest()
    {
        return m_resourceRequest;
    }

    // https://www.w3.org/TR/XMLHttpRequest/#the-responsetype-attribute
    void setResponseType(ResponseType type);
    void setResponseType(String* typeStr);
    ResponseType responseTypeValue() const;
    String* responseType() const;

    ScriptValue response() const;
    String* responseText() const;

    uint8_t readyState() const;
    uint16_t status() const;
    String* statusText() const;

    void open(String* method, String* url);
    void open(String* method, String* url, bool async,
              Nullable<String*> userName, Nullable<String*> password);
    void open(ResourceRequest::MethodType method, String* url, bool async,
              String* userName = String::emptyString,
              String* password = String::emptyString);
    void send(Nullable<String*> body);
    void send(String* body);
    void abort();

    uint32_t timeout() const;
    void setTimeout(uint32_t timeout);
    void setRequestHeader(String* h, String* c);

    String* getAllResponseHeaders();
    Nullable<String*> getResponseHeader(String* name);

#define VIRTUAL
#define OVERRIDE
    DECLARE_EVENT_LISTENER(readystatechange);
#undef VIRTUAL
#undef OVERRIDE

protected:
    void initResponseData();
    ResourceRequest* m_resourceRequest;
    ResponseType m_responseType;

    // for responseType = "text"
    String* m_responseText;

    // for responseType = "json"
    ScriptValue m_responseJsonObject;

    // for responseType = "blob
    ::StarFish::Blob* m_responseBlob;

    // for responseType = "arraybuffer"
    ScriptValue m_responseArrayBuffer;
};
}

#endif
