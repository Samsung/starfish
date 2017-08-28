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
#include "core/extra/Console.h"
#include "core/page/BrowsingContext.h"
#include "core/page/EventSource.h"
#include "core/page/EventSourceParser.h"
#include "core/page/Window.h"
#include "platform/network/http/HTTPStatusCode.h"

namespace StarFish {

DEFINE_EVENT_LISTENER(EventSource, loadstart);
DEFINE_EVENT_LISTENER(EventSource, progress);
DEFINE_EVENT_LISTENER(EventSource, abort);
DEFINE_EVENT_LISTENER(EventSource, error);
DEFINE_EVENT_LISTENER(EventSource, load);
DEFINE_EVENT_LISTENER(EventSource, timeout);
DEFINE_EVENT_LISTENER(EventSource, loadend);
DEFINE_EVENT_LISTENER(EventSource, message);
DEFINE_EVENT_LISTENER(EventSource, open);

const unsigned long long EventSource::defaultReconnectDelay = 3000;

class EventSourceResourceRequestClient : public ResourceRequestClient {
public:
    EventSourceResourceRequestClient(EventSource* es)
        : m_eventSource(es)
        , m_isResponseValid(false)
        , m_lastResponseReadingPosition(0)
        , m_textConverter(nullptr)
    {
    }

    virtual void onProgressEvent(ResourceRequest* request,
                                 bool isExplicitAction) override
    {
        if (request->progressState() == ResourceRequest::PROGRESS) {
            auto& response = request->response();

            if (m_isResponseValid) {
                STARFISH_ASSERT(m_textConverter);
                STARFISH_ASSERT(m_eventSource->m_parser);

                String* text = m_textConverter->convert(response.data(),
                                                        response.size(), true);
                auto utf8Data = text->toUTF8NonGCString();
                m_eventSource->m_parser->addBytes(utf8Data.data(),
                                                  utf8Data.size());
            }
            response.clear();
        }
    }

    void onReadyStateChange(ResourceRequest* request, bool fromExplicit)
    {
        if (request->readyState() == ResourceRequest::HEADERS_RECEIVED) {
            uint16_t statusCode = request->status();
            bool isMimeTypeValid = request->responseMimeType()->contains(
                String::createASCIIString("text/event-stream"), false);
            const ResponseHeaderMap& headerMap = request->responseHeaderMap();
            m_isResponseValid = statusCode == HTTP_STATUS_OK && isMimeTypeValid;

            auto cs = headerMap.find(std::string("charset"));
            bool isCharsetValid = (cs == headerMap.end()) ||
                                  StringUtils::equalsWithoutCase(
                                      cs->second, std::string("utf-8"));

            m_isResponseValid &= isCharsetValid;
            if (m_isResponseValid) {
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

                String* eventName =
                    request->starFish()->staticStrings()->m_open.localName();
                Event* e = new Event(m_eventSource->document(), eventName);
                e->setBubbles(false);
                e->setCancelable(false);
                e->setComposed(false);
                m_eventSource->dispatchEvent(m_eventSource, e);

                m_eventSource->m_readyState = EventSource::OPEN;
            } else {
                StringBuilder msg;
                if (statusCode != HTTP_STATUS_OK) {
                    msg.appendString(
                        "Failed to load resource: the server responded with a "
                        "status of ");
                    msg.appendString(String::fromInt(statusCode));
                } else if (!isCharsetValid) {
                    msg.appendString(
                        "EventSource's response has a charset (\"");
                    msg.appendString(cs->second.data());
                    msg.appendString(
                        "\") that is not UTF-8. Aborting the connection.");
                } else if (!isMimeTypeValid) {
                    msg.appendString(
                        "EventSource's response has a MIME type (\"");
                    msg.appendString(request->responseMimeType());
                    msg.appendString(
                        "\") that is not \"text/event-stream\". Aborting the "
                        "connection.");
                }

                STARFISH_LOG_ERROR("console.error: %s\n",
                                   msg.finalize()->utf8Data());

                m_eventSource->cancel();
            }
        } else if (request->readyState() == ResourceRequest::LOADING) {
        } else if (request->readyState() == ResourceRequest::DONE) {
            if (m_eventSource->readyState() == EventSource::CLOSED) {
                m_eventSource->cancel();
            } else {
                m_eventSource->failed();
            }

            String* eventName =
                request->starFish()->staticStrings()->m_error.localName();
            Event* e = new Event(m_eventSource->document(), eventName,
                                 EventInit(false, false));
            m_eventSource->dispatchEvent(m_eventSource, e);
        } else if (request->readyState() == ResourceRequest::UNSENT ||
                   request->readyState() == ResourceRequest::OPENED) {
        }
    }

private:
    EventSource* m_eventSource;
    bool m_isResponseValid;
    size_t m_lastResponseReadingPosition;
    TextConverter* m_textConverter;
};

EventSource::EventSource(::StarFish::Document* document, String* url)
    : EventSource(document, url, EventSourceInit())
{
}

EventSource::EventSource(::StarFish::Document* document, String* url,
                         const EventSourceInit& init)
    : EventTarget(document)
    , m_readyState(CONNECTING)
    , m_withCredentials(init.withCredentials())
    , m_delay(10)
    , m_reconnectDelay(defaultReconnectDelay)
    , m_resourceRequest(new ResourceRequest(document))
    , m_parser(nullptr)
    , m_stopReconnect(false)
    , m_time(std::numeric_limits<uint32_t>::max())
{
    if (url->isEmpty()) {
        throw new DOMException(document, DOMException::SYNTAX_ERR,
                               "Cannot open an EventSource to an empty URL.");
    }

    ResourceURL* fullURL =
        new ResourceURL(url, document->documentURI()->baseURI());
    if (fullURL->protocolKind() != ResourceURL::Protocol::HTTP_PROTOCOL &&
        fullURL->protocolKind() != ResourceURL::Protocol::HTTPS_PROTOCOL) {
        StringBuilder msg;
        msg.appendString("Cannot open an EventSource to '");
        msg.appendString(url);
        msg.appendString("'. The URL is invalid.");
        throw new DOMException(document, DOMException::SYNTAX_ERR,
                               msg.finalize()->utf8Data());
    }

    m_resourceRequest->addResourceRequestClient(
        new EventSourceResourceRequestClient(this));

    m_resourceRequest->m_requestHeaders.push_back(
        std::make_pair(String::createASCIIString("Accept"),
                       String::createASCIIString("text/event-stream")));
    m_resourceRequest->m_requestHeaders.push_back(
        std::make_pair(String::createASCIIString("Cache-Control"),
                       String::createASCIIString("no-cache")));

    if (m_parser && !m_parser->lastEventId()->isEmpty()) {
        m_resourceRequest->m_requestHeaders.push_back(
            std::make_pair(String::createASCIIString("Last-Event-ID"),
                           m_parser->lastEventId()));
    }

    m_url = fullURL;
    connectFired();

    document->browsingContext()->addPointerInRootSet(this);
}

void EventSource::connect()
{
    // TODO: set OPTION request : preflightPolicy, crossOriginRequestPolicy,
    // contentSecurityPolicyEnforcement
    // TODO: set resource loader options: allowCredentials,
    // credentialsRequested, dataBufferingPolicy, securityOrigin
    // < ------------------------------------------------- >

    if (m_parser && !m_parser->lastEventId()->isEmpty()) {
        auto& header = m_resourceRequest->m_requestHeaders;
        header.erase(std::remove_if(header.begin(), header.end(),
                                    [](const std::pair<String*, String*>& o) {
                                        return o.first->equals("Last-Event-ID");
                                    }),
                     header.end());
        header.push_back(
            std::make_pair(String::createASCIIString("Last-Event-ID"),
                           m_parser->lastEventId()));
    }

    if (m_readyState == CONNECTING) {
        start(ResourceRequest::MethodType::GET_METHOD);
    }
}

void EventSource::start(ResourceRequest::MethodType method)
{
    if (m_resourceRequest->timeout() != 0) {
        throw new DOMException(scriptBindingInstance()->ownerDocument(),
                               DOMException::INVALID_ACCESS_ERR,
                               "InvalidAccessError");
    }

    if (m_reconnectDelay != defaultReconnectDelay) {
        m_delay = m_reconnectDelay;
    } else {
        m_delay = defaultReconnectDelay;
    }
    m_resourceRequest->open(method, m_url->urlString(), true,
                            String::emptyString, String::emptyString);
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
    dispatchEvent(this, e);
}

void EventSource::onReconnectionTimeSet(unsigned long long reconnectionTime)
{
    m_delay = m_reconnectDelay = reconnectionTime;
}

void EventSource::connectFired()
{
    if (!m_stopReconnect) {
        m_time = document()->window()->setTimeout(
            [](Window* window, void* data) {
                EventSource* self = (EventSource*)data;
                self->connect();
            },
            m_delay, this);
    }
}

void EventSource::scheduleReconnect()
{
    m_readyState = CONNECTING;
    connectFired();
}

void EventSource::failed()
{
    if (m_readyState != CLOSED) {
        scheduleReconnect();
    }
}

void EventSource::cancel()
{
    m_readyState = CLOSED;
    m_resourceRequest->abort(true);
    document()->window()->clearTimeout(m_time);
    document()->browsingContext()->removePointerFromRootSet(this);
}

void EventSource::close()
{
    if (m_readyState == CLOSED) {
        return;
    }

    m_readyState = CLOSED;
    m_resourceRequest->abort(true);
    if (m_parser) {
        m_parser->stop();
    }
    if (!m_stopReconnect) {
        m_stopReconnect = true;
    }
    document()->window()->clearTimeout(m_time);
    document()->browsingContext()->removePointerFromRootSet(this);
}
}
