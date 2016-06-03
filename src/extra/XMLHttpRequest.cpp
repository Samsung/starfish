#include "StarFishConfig.h"
#include "XMLHttpRequest.h"
#include "dom/Event.h"
#include "dom/DOMException.h"

namespace StarFish {

XMLHttpRequest::XMLHttpRequest(Document* document)
    : m_networkRequest(new NetworkRequest(document))
    , m_resposeText(String::emptyString)
{
    m_networkRequest->addNetworkRequestClient(this);
}

void XMLHttpRequest::send(String* body)
{
    if (m_networkRequest->readyState() != NetworkRequest::OPENED) {
        throw new DOMException(m_networkRequest->starFish()->scriptBindingInstance(), DOMException::INVALID_STATE_ERR, "InvalidStateError");
    }
    m_networkRequest->send(body);
}

void XMLHttpRequest::open(NetworkRequest::MethodType method, String* url, bool async, String* userName, String* password)
{
    if (!async && m_networkRequest->timeout() != 0)
        throw new DOMException(m_networkRequest->starFish()->scriptBindingInstance(), DOMException::INVALID_ACCESS_ERR, "InvalidAccessError");

    m_networkRequest->open(method, url, async);
}

void XMLHttpRequest::abort()
{
    m_networkRequest->abort();
}

String* XMLHttpRequest::responseText()
{
    // if (!(m_responseType == DEFAULT_RESPONSE || m_responseType == TEXT_RESPONSE))
    //     throw new DOMException(m_bindingInstance, DOMException::INVALID_STATE_ERR, "InvalidStateError");
    return m_resposeText;
}

void XMLHttpRequest::setTimeout(uint32_t timeout)
{
    if (m_networkRequest->isSync() == true)
        throw new DOMException(m_networkRequest->starFish()->scriptBindingInstance(), DOMException::INVALID_ACCESS_ERR, "InvalidAccessError");
    m_networkRequest->setTimeout(timeout);
}

void XMLHttpRequest::onProgressEvent(NetworkRequest* request, bool isExplicitAction)
{
    String* eventName = String::emptyString;
    NetworkRequest::ProgressState progState = request->progressState();
    if (progState == NetworkRequest::PROGRESS)
        eventName = request->starFish()->staticStrings()->m_progress.localName();
    else if (progState == NetworkRequest::ERROR)
        eventName = request->starFish()->staticStrings()->m_error.localName();
    else if (progState == NetworkRequest::ABORT) {
        if (isExplicitAction) {
            return;
        }
        eventName = request->starFish()->staticStrings()->m_abort.localName();
    } else if (progState == NetworkRequest::TIMEOUT)
        eventName = request->starFish()->staticStrings()->m_timeout.localName();
    else if (progState == NetworkRequest::LOAD) {
        eventName = request->starFish()->staticStrings()->m_load.localName();
    } else if (progState == NetworkRequest::LOADEND)
        eventName = request->starFish()->staticStrings()->m_loadend.localName();
    else if (progState == NetworkRequest::LOADSTART)
        eventName = request->starFish()->staticStrings()->m_loadstart.localName();
    else
        STARFISH_RELEASE_ASSERT_NOT_REACHED();

    ProgressEvent* pe = new ProgressEvent(eventName, ProgressEventInit(false, false, request->total() > 0, request->loaded(), request->total()));
    EventTarget::dispatchEvent(this, pe);
}

void XMLHttpRequest::onReadyStateChange(NetworkRequest* request, bool fromExplicit)
{
    if (fromExplicit) {
        if (request->readyState() == NetworkRequest::DONE)
            m_resposeText =  String::fromUTF8(m_networkRequest->responseData().data(), m_networkRequest->responseData().size());

        String* eventType = request->starFish()->staticStrings()->m_readystatechange.localName();
        Event* e = new Event(eventType, EventInit(true, true));
        EventTarget::dispatchEvent(this, e);
    }
}

}
