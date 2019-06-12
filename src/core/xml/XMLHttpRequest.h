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

#ifndef __StarfishXMLHttpRequest__
#define __StarfishXMLHttpRequest__

#include "core/dom/EventTarget.h"

namespace Starfish {

enum class BodyType;
enum class MethodType;
class ResourceRequest;
using XMLHttpRequestResponseType = BodyType;

class XMLHttpRequestEventTarget : public EventTarget, public DocumentHoldable {
public:
    XMLHttpRequestEventTarget(Document* document)
        : EventTarget()
        , DocumentHoldable(document)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isXMLHttpRequestEventTarget() const override;

    virtual ExecutionContext* executionContext() const override;

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

class XMLHttpRequestUpload : public XMLHttpRequestEventTarget {
public:
    XMLHttpRequestUpload(Document* document)
        : XMLHttpRequestEventTarget(document)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isXMLHttpRequestUpload() const override;
    bool hasEventListeners() const;
};

class XMLHttpRequest : public XMLHttpRequestEventTarget {
    friend class XMLHttpRequestEventEmitter;
    friend class XMLHttpRequestResourceRequestClient;

public:
    XMLHttpRequest(::Starfish::Document* document);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isXMLHttpRequest() const override;

    ResourceRequest* resourceRequest()
    {
        return m_resourceRequest;
    }

    // https://xhr.spec.whatwg.org/#dom-xmlhttprequest-responsetype
    void setResponseType(XMLHttpRequestResponseType type);
    void setResponseType(String* typeStr);
    XMLHttpRequestResponseType responseTypeValue() const;
    String* responseType() const;

    ScriptValue response() const;
    String* responseText() const;
    ::Starfish::Document* responseXML() const;

    uint8_t readyState() const;
    uint16_t status() const;
    String* statusText() const;

    void open(String* method, String* url);
    void open(String* method, String* url, bool async,
              Nullable<String*> userName, Nullable<String*> password);
    void open(String* method, String* url, bool async,
              String* userName = String::emptyString,
              String* password = String::emptyString);
    void send(Nullable<String*> body);
    void send(String* body);
    void abort();

    uint32_t timeout() const;
    void setTimeout(uint32_t timeout);

    bool withCredentials() const;
    void setWithCredentials(bool value);

    XMLHttpRequestUpload* upload() const;

    void setRequestHeader(String* header, String* value);

    void overrideMimeType(String* mime);
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
    XMLHttpRequestResponseType m_responseType;
    bool m_withCredentials;
    XMLHttpRequestUpload* m_upload;

    // overrideMimeType
    String* m_overrideMimeType;

    // for responseType = "text"
    String* m_responseText;

    // for responseType = "json"
    ScriptValue m_responseJsonObject;

    // for responseType = "blob
    ::Starfish::Blob* m_responseBlob;

    // for responseType = "arraybuffer"
    ScriptValue m_responseArrayBuffer;

    // for responseType = "document"
    ::Starfish::Document* m_responseXML;
};
}

#endif
