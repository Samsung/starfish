/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef __StarFishXMLHttpRequest__
#define __StarFishXMLHttpRequest__

#include "dom/EventTarget.h"
#include "platform/network/NetworkRequest.h"

namespace StarFish {

class Blob;
class Document;

class XMLHttpRequestEventTarget : public EventTarget {
public:
    XMLHttpRequestEventTarget(Document* document)
        : EventTarget(document)
    {
    }

    DECLARE_EVENT_LISTENER(loadstart);
    DECLARE_EVENT_LISTENER(progress);
    DECLARE_EVENT_LISTENER(abort);
    DECLARE_EVENT_LISTENER(error);
    DECLARE_EVENT_LISTENER(load);
    DECLARE_EVENT_LISTENER(timeout);
    DECLARE_EVENT_LISTENER(loadend);
    DECLARE_EVENT_LISTENER(readystatechange);
};

class XMLHttpRequest : public XMLHttpRequestEventTarget,
                       public NetworkRequestClient {
    friend class XMLHttpRequestEventEmitter;

public:
    XMLHttpRequest(Document* document);

    enum ResponseType {
        Unspecified,
        Text,
        ArrayBuffer,
        DocumentType, // TODO
        BlobType,
        Json
    };

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual bool isXMLHttpRequest() const override
    {
        return true;
    }

    NetworkRequest* networkRequest()
    {
        return m_networkRequest;
    }

    // https://www.w3.org/TR/XMLHttpRequest/#the-responsetype-attribute
    void setResponseType(ResponseType type);
    ResponseType responseType();

    ScriptValue response();
    String* responseText();

    void open(NetworkRequest::MethodType method, String* url, bool async,
              String* userName = String::emptyString,
              String* password = String::emptyString);
    void send(String* body = String::emptyString);
    void abort();

    void setTimeout(uint32_t timeout);
    void setRequestHeader(String* h, String* c);

    virtual void onProgressEvent(NetworkRequest* request,
                                 bool isExplicitAction);
    virtual void onReadyStateChange(NetworkRequest* request, bool fromExplicit);

protected:
    void initResponseData();
    NetworkRequest* m_networkRequest;
    ResponseType m_responseType;

    // for responseType = "text"
    String* m_responseText;

    // for responseType = "json"
    ScriptValue m_responseJsonObject;

    // for responseType = "blob
    Blob* m_responseBlob;

#ifdef USE_ES6_FEATURE
    // for responseType = "arraybuffer"
    ScriptValue m_responseArrayBuffer;
#endif
};
}

#endif
