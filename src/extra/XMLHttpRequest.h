/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

#include "dom/binding/ScriptWrappable.h"
#include "dom/EventTarget.h"
#include "platform/network/NetworkRequest.h"
#include "util/TextConverter.h"

namespace StarFish {

class XMLHttpRequestEventTarget : public EventTarget {
public:
    XMLHttpRequestEventTarget()
        : EventTarget()
    {
    }
};

class XMLHttpRequest : public XMLHttpRequestEventTarget, public NetworkRequestClient {
    friend class XMLHttpRequestEventEmitter;
public:
    XMLHttpRequest(Document* document);

    enum ResponseType {
        Unspecified,
        Text,
        ArrayBuffer, // TODO
        DocumentType, // TODO
        Blob,
        Json
    };

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    NetworkRequest& networkRequest()
    {
        return *m_networkRequest;
    }

    // https://www.w3.org/TR/XMLHttpRequest/#the-responsetype-attribute
    void setResponseType(ResponseType type);
    ResponseType responseType();

    ScriptValue response();
    String* responseText();

    void open(NetworkRequest::MethodType method, String* url, bool async, String* userName = String::emptyString, String* password = String::emptyString);
    void send(String* body = String::emptyString);
    void abort();

    void setTimeout(uint32_t timeout);

    virtual void onProgressEvent(NetworkRequest* request, bool isExplicitAction);
    virtual void onReadyStateChange(NetworkRequest* request, bool fromExplicit);
protected:
    void initResponseData();
    NetworkRequest* m_networkRequest;
    ResponseType m_responseType;

    // for responseType = "text"
    String* m_responseText;

    // for responseType = "json"
    ScriptValue m_responseJsonObject;

    // TODO implement blob
};
}

#endif
