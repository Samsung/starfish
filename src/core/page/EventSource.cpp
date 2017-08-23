/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/dom/DOMException.h"
#include "core/dom/MessageEvent.h"
#include "core/page/EventSource.h"
#include "core/page/EventSourceParser.h"
#include "core/page/Window.h"

namespace StarFish {

DEFINE_EVENT_LISTENER(EventSource, loadstart);
DEFINE_EVENT_LISTENER(EventSource, progress);
DEFINE_EVENT_LISTENER(EventSource, abort);
DEFINE_EVENT_LISTENER(EventSource, error);
DEFINE_EVENT_LISTENER(EventSource, load);
DEFINE_EVENT_LISTENER(EventSource, timeout);
DEFINE_EVENT_LISTENER(EventSource, loadend);
DEFINE_EVENT_LISTENER(EventSource, message);

const unsigned long long EventSource::defaultReconnectDelay = 3000;

class EventSourceResourceRequestClient : public ResourceRequestClient {
public:
    EventSourceResourceRequestClient(EventSource* es)
        : m_eventSource(es)
        , m_isResponseValid(false)
        , m_textConverter(nullptr)
    {
    }

    virtual void onProgressEvent(ResourceRequest* request,
                                 bool isExplicitAction) override
    {
    }

    void onReadyStateChange(ResourceRequest* request, bool fromExplicit)
    {
        if (request->readyState() == ResourceRequest::HEADERS_RECEIVED) {
            uint16_t statusCode = request->status();
            bool isMimeTypeValid = request->responseMimeType()->contains(
                String::createASCIIString("text/event-stream"), false);
            ResponseHeaderMap headerMap = request->responseHeaderMap();
            m_isResponseValid = statusCode == 200 && isMimeTypeValid;

            if (m_isResponseValid) {
                auto cs = headerMap.find(std::string("charset"));
                bool isCharsetValid = (cs == headerMap.end()) ||
                                      (cs->second.compare("utf-8") == 0);
                if (!isCharsetValid) {
                    // TODO: use console.log()
                }

                String* lastEventId = String::emptyString;
                if (m_eventSource->m_parser) {
                    lastEventId = m_eventSource->m_parser->lastEventId();
                }

                m_eventSource->m_parser =
                    new EventSourceParser(lastEventId, m_eventSource);

                m_textConverter = new TextConverter(
                    m_eventSource->m_resourceRequest->responseMimeType(),
                    String::fromUTF8("UTF-8"),
                    m_eventSource->m_resourceRequest->response().data(),
                    m_eventSource->m_resourceRequest->response().size());
            }
        } else if (request->readyState() == ResourceRequest::OPEN) {
            if (m_isResponseValid) {
                String* text = m_textConverter->convert(
                    m_eventSource->m_resourceRequest->response().data(),
                    m_eventSource->m_resourceRequest->response().size(), true);

                m_eventSource->m_parser->addBytes(text->utf8Data(),
                                                  text->length());
            }
        } else if (request->readyState() == ResourceRequest::CLOSED) {
        }
    }

private:
    EventSource* m_eventSource;
    bool m_isResponseValid;
    TextConverter* m_textConverter;
};

EventSource::EventSource(::StarFish::Document* document, String* url)
    : EventSource(document, url, EventSourceInit())
{
}

EventSource::EventSource(::StarFish::Document* document, String* url,
                         const EventSourceInit& init)
    : EventTarget(document)
    , m_withCredentials(init.withCredentials())
    , m_state(Connecting)
    , m_reconnectDelay(defaultReconnectDelay)
    , m_resourceRequest(new ResourceRequest(document))
    , m_parser(nullptr)
{
    if (url->isEmpty()) {
        // TODO: throw exception
    }

    ResourceURL* fullURL =
        new ResourceURL(url, document->documentURI()->baseURI());
    if (!ResourceURL::isValidURL(fullURL->urlString())) {
        // TODO: throw exception
    }

    m_resourceRequest->addResourceRequestClient(
        new EventSourceResourceRequestClient(this));

    m_url = fullURL;
    document->window()->setTimeout(
        [](Window* window, void* data) {
            EventSource* self = (EventSource*)data;
            self->connect();
        },
        m_reconnectDelay, this);
}

void EventSource::connect()
{
    STARFISH_ASSERT(m_state == Connecting);

    m_resourceRequest->m_requestHeaders.push_back(
        std::make_pair(String::createASCIIString("Accept"),
                       String::createASCIIString("text/event-stream")));
    m_resourceRequest->m_requestHeaders.push_back(
        std::make_pair(String::createASCIIString("Cache-Control"),
                       String::createASCIIString("no-cache")));

    if (m_parser && !m_parser->lastEventId()->isEmpty()) {
        String* lastId = m_parser->lastEventId();
        m_resourceRequest->m_requestHeaders.push_back(
            std::make_pair(String::createASCIIString("Last-Event-ID"), lastId));
    }

    // TODO: set CORS settings attribute: anonymous or use-credentials
    // TODO: set resource loader using credentials if needed
    // < ------------------------------------------------- >

    start(ResourceRequest::MethodType::GET_METHOD);
}

void EventSource::start(ResourceRequest::MethodType method)
{
    if (m_resourceRequest->timeout() != 0) {
        throw new DOMException(scriptBindingInstance()->ownerDocument(),
                               DOMException::INVALID_ACCESS_ERR,
                               "InvalidAccessError");
    }

    m_resourceRequest->open(method, m_url->urlString(), true,
                            String::emptyString, String::emptyString, true);
    m_resourceRequest->send();
}

void EventSource::didHeaderReceived(const ResponseHeaderMap& headrs)
{
}

void EventSource::onMessageEvent(String* eventType, String* data,
                                 String* lastEventId)
{
    MessageEvent* e = new MessageEvent(document(), eventType);
    e->setBubbles(false);
    e->setCancelable(false);
    e->setComposed(false);
    e->setLastEventId(lastEventId);
    e->setSource(document()->window());
    e->setData(createScriptValue(createScriptString(data)));
    EventTarget::dispatchEvent(this, e);
}

void EventSource::onReconnectionTimeSet(unsigned long long reconnectionTime)
{
}

void EventSource::close()
{
}
}
