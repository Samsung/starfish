#ifndef __StarFishXMLHttpRequest__
#define __StarFishXMLHttpRequest__

#include "dom/binding/ScriptWrappable.h"
#include "dom/EventTarget.h"
#include "platform/network/NetworkRequest.h"

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

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    NetworkRequest& networkRequest()
    {
        return *m_networkRequest;
    }

    String* responseText();

    void open(NetworkRequest::MethodType method, String* url, bool async, String* userName = String::emptyString, String* password = String::emptyString);
    void send(String* body = String::emptyString);
    void abort();

    void setTimeout(uint32_t timeout);

    virtual void onProgressEvent(NetworkRequest* request, bool isExplicitAction);
    virtual void onReadyStateChange(NetworkRequest* request, bool fromExplicit);
protected:
    NetworkRequest* m_networkRequest;
    String* m_resposeText;
};
}

#endif
