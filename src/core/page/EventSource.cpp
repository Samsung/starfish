/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/dom/Document.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/DOMException.h"
#include "core/dom/MessageEvent.h"
#include "core/extra/Console.h"
#include "core/page/BrowsingContext.h"
#include "core/page/EventSource.h"
#include "core/page/EventSourceParser.h"
#include "core/page/Window.h"
#include "platform/network/http/HTTPHeaderMap.h"
#include "platform/network/http/HTTPStatus.h"
#include "platform/loader/ResourceLoader.h"
#include "core/csp/ContentSecurityPolicy.h"

namespace Starfish {

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
        , m_textConverter(nullptr)
    {
    }

    virtual void onProgressEvent(ResourceRequest* request,
                                 bool isExplicitAction) override
    {
        if (request->progressState() == ProgressState::Progress) {
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

    void onReadyStateChange(ResourceRequest* request,
                            bool fromExplicit) override
    {
        if (request->readyState() == ReadyState::HeadersReceived) {
            uint16_t statusCode = request->status();
            bool isMimeTypeValid = request->responseMimeType()->contains(
                "text/event-stream", false);
            const HeaderMap& headerMap = request->responseHeaderMap();
            m_isResponseValid = statusCode == HTTP_STATUS_OK && isMimeTypeValid;

            bool isCharsetValid = true;
            String* charsetValue = String::emptyString;
            auto it = headerMap.find(HTTPHeaderMap::kContentType);
            if (it != headerMap.end()) {
                String* values = String::fromUTF8(it->second.data());
                GCVector<StringView> tokens;
                StringUtils::tokenize(values, ";", 1, tokens);

                for (auto token : tokens) {
                    GCVector<StringView> pair;
                    StringUtils::tokenize(&token, "=", 1, pair);
                    String* key = pair[0].trim();
                    if (key->equalsIgnoreCase("charset")) {
                        String* value = (new StringView(pair[1]))->trim();
                        isCharsetValid = value->equalsIgnoreCase("utf-8") ||
                                         value->equalsIgnoreCase("\"utf-8\"");
                        charsetValue = value;
                    }
                }
            }

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
                    request->starfish()->staticStrings()->m_open.localName();
                Event* e =
                    new Event(m_eventSource->executionContext(), eventName);
                e->setBubbles(false);
                e->setCancelable(false);
                e->setComposed(false);
                m_eventSource->dispatchEventByUA(m_eventSource, e);

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
                    msg.appendString(charsetValue);
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
                auto s = msg.finalize()->toUTF8NonGCString();
                STARFISH_LOG_ERROR("console.error: %s\n", s.data());
                m_eventSource->cancel();

                String* eventName =
                    request->starfish()->staticStrings()->m_error.localName();
                Event* e = new Event(m_eventSource->executionContext(),
                                     eventName, EventInit(false, false));
                m_eventSource->dispatchEventByUA(m_eventSource, e);
            }
        } else if (request->readyState() == ReadyState::Loading) {
        } else if (request->readyState() == ReadyState::Done) {
            if (m_eventSource->readyState() != EventSource::CLOSED) {
                m_eventSource->failed();

                String* eventName =
                    request->starfish()->staticStrings()->m_error.localName();
                Event* e = new Event(m_eventSource->executionContext(),
                                     eventName, EventInit(false, false));
                m_eventSource->dispatchEventByUA(m_eventSource, e);
            }

        } else if (request->readyState() == ReadyState::Unset ||
                   request->readyState() == ReadyState::Opened) {
        }
    }

private:
    EventSource* m_eventSource;
    bool m_isResponseValid;
    TextConverter* m_textConverter;
};

EventSource::EventSource(::Starfish::Document* document, String* url)
    : EventSource(document, url, EventSourceInit())
{
}

EventSource::EventSource(::Starfish::Document* document, String* url,
                         const EventSourceInit& init)
    : EventTarget()
    , DocumentHoldable(document)
    , m_readyState(CONNECTING)
    , m_delay(10)
    , m_reconnectDelay(defaultReconnectDelay)
    , m_resourceRequest(new ResourceRequest(document->executionContext()))
    , m_parser(nullptr)
    , m_stopReconnect(false)
    , m_withCredentials(init.withCredentials())
    , m_time(std::numeric_limits<uint32_t>::max())
{
    // https://html.spec.whatwg.org/multipage/server-sent-events.html#dom-eventsource
    if (url->isEmpty()) {
        throw new DOMException(executionContext(), DOMException::SYNTAX_ERR,
                               "Cannot open an EventSource to an empty URL.");
    }

    ResourceURL* fullURL = new ResourceURL(url, document->baseURL()->baseURI());
    if (!fullURL->isValid()) {
        StringBuilder msg;
        msg.appendString("Cannot open an EventSource to '");
        msg.appendString(url);
        msg.appendString("'. The URL is invalid.");
        auto s = msg.finalize()->toUTF8NonGCString();
        throw new DOMException(executionContext(), DOMException::SYNTAX_ERR,
                               s.data());
    }

    if (!document->contentSecurityPolicy()->allowSource(
            CSPDirectives::ConnectSrc, fullURL)) {
        throw new DOMException(executionContext(), DOMException::SECURITY_ERR);
    }

    m_resourceRequest->addResourceRequestClient(
        new EventSourceResourceRequestClient(this));

    m_url = fullURL;
    connectFired();

    executionContext()->addPointerInRootSet(this);
}

ExecutionContext* EventSource::executionContext() const
{
    return document()->executionContext();
}

void EventSource::connect()
{
    // TODO: set OPTION request : preflightPolicy, crossOriginRequestPolicy,
    // contentSecurityPolicyEnforcement
    // TODO: set resource loader options: allowCredentials,
    // credentialsRequested, dataBufferingPolicy, securityOrigin
    // < ------------------------------------------------- >

    if (!(m_url->protocolKind() == ResourceURL::HTTP_PROTOCOL ||
          m_url->protocolKind() == ResourceURL::HTTPS_PROTOCOL ||
          m_url->protocolKind() == ResourceURL::DATA_PROTOCOL)) {
        failedAccessControlCheck();
    }

    if (m_parser && !m_parser->lastEventId()->isEmpty()) {
        m_resourceRequest->deleteRequestHeader(
            String::createASCIIString(HTTPHeaderMap::kLastEventID));

        m_resourceRequest->setRequestHeader(
            String::createASCIIString(HTTPHeaderMap::kLastEventID),
            m_parser->lastEventId());
    }

    if (m_readyState == CONNECTING) {
        start(String::createASCIIString("GET"));
    }
}

void EventSource::start(String* method)
{
    if (m_resourceRequest->timeout() != 0) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_ACCESS_ERR,
                               "InvalidAccessError");
    }

    if (m_reconnectDelay != defaultReconnectDelay) {
        m_delay = m_reconnectDelay;
    } else {
        m_delay = defaultReconnectDelay;
    }

    RequestData* reqData = new RequestData();
    reqData->m_method = method;
    reqData->m_url = m_url;
    reqData->m_referrer = new ReferrerURL(document()->documentURI(),
                                          document()->referrerPolicy());
    reqData->m_syncLevel = RequestSyncLevel::NeverSync;
    if (m_withCredentials) {
        reqData->m_credentials = RequestCredentials::Include;
    } else {
        reqData->m_credentials = RequestCredentials::SameOrigin;
    }

    m_resourceRequest->open(reqData);

    m_resourceRequest->setRequestHeader(
        String::createASCIIString(HTTPHeaderMap::kAccept),
        String::createASCIIString("text/event-stream"));

    m_resourceRequest->setRequestHeader(
        String::createASCIIString(HTTPHeaderMap::kCacheControl),
        String::createASCIIString("no-cache"));

    if (m_parser && !m_parser->lastEventId()->isEmpty()) {
        m_resourceRequest->setRequestHeader(
            String::createASCIIString(HTTPHeaderMap::kLastEventID),
            m_parser->lastEventId());
    }

    m_resourceRequest->send();
}

void EventSource::didHeaderReceived(const HeaderMap& headrs)
{
}

void EventSource::onMessageEvent(String* eventType, String* data,
                                 String* lastEventId)
{
    MessageEvent* e = new MessageEvent(executionContext(), eventType);
    e->setBubbles(false);
    e->setCancelable(false);
    e->setComposed(false);
    e->setLastEventId(lastEventId);
    e->setSource(MessageEventSource::createWindow(document()->window()));
    e->setData(createScriptValue(createScriptString(data)));
    dispatchEventByUA(this, e);
}

void EventSource::onReconnectionTimeSet(unsigned long long reconnectionTime)
{
    m_delay = m_reconnectDelay = reconnectionTime;
}

bool EventSource::withCredentials() const
{
    return m_withCredentials;
}

void EventSource::connectFired()
{
    if (!m_stopReconnect) {
        m_time = document()->window()->setTimeout(
            [](void* data) {
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

void EventSource::failedAccessControlCheck()
{
    StringBuilder msg;
    msg.appendString("EventSource cannot load ");
    msg.appendString(m_url->urlString());
    msg.appendString(". ");
    msg.appendString(
        "Cross origin requests are only supported for protocol schemes: http, "
        "https, data.");
    auto s = msg.finalize()->toUTF8NonGCString();
    STARFISH_LOG_ERROR("console.error: %s\n", s.data());

    m_readyState = CLOSED;
    m_resourceRequest->abort(true);
    document()->window()->clearTimeout(m_time);
    executionContext()->removePointerFromRootSet(this);
}

void EventSource::cancel()
{
    m_readyState = CLOSED;
    m_resourceRequest->abort(true);
    document()->window()->clearTimeout(m_time);
    executionContext()->removePointerFromRootSet(this);
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
    executionContext()->removePointerFromRootSet(this);
}
}
