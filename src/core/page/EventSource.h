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

#ifndef __StarFishEventSource__
#define __StarFishEventSource__

#include "core/dom/EventTarget.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/page/EventSourceParser.h"

namespace StarFish {

class ResourceURL;
class EventSourceParser;
class ResourceRequest;

struct EventSourceInit {
public:
    STARFISH_MAKE_STACK_ALLOCATED()

    EventSourceInit()
        : m_withCredentials(false)
    {
    }

    bool hasWithCredentials() const
    {
        return !m_withCredentials.hasValue();
    }

    bool withCredentials() const
    {
        return m_withCredentials.getValue();
    }

    void setWithCredentials(bool value)
    {
        m_withCredentials = value;
    }

private:
    Nullable<bool> m_withCredentials;
};

class EventSource : public EventTarget, public EventSourceParser::Client {
    friend class EventSourceResourceRequestClient;

public:
    EventSource(::StarFish::Document* document, String* url);
    EventSource(::StarFish::Document* document, String* url,
                const EventSourceInit& init);

    static const unsigned long long defaultReconnectDelay;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isEventSource() const override;

    enum ResponseType { Unspecified, Text, ArrayBuffer, Document, Blob, Json };
    enum ReadyState { CONNECTING, OPEN, CLOSED };
    ResourceRequest* resourceRequest()
    {
        return m_resourceRequest;
    }

    String* url() const
    {
        return m_url->urlString();
    }

    bool withCredentials() const
    {
        return m_withCredentials;
    }

    ReadyState readyState() const
    {
        return m_readyState;
    }

    void connectFired();
    void start(ResourceRequest::MethodType method);
    void initResponseData();

    void didHeaderReceived(const HeaderMap& headrs);
    void didDataReceived(const char*, size_t length);

    virtual void onMessageEvent(String* type, String* data,
                                String* lastEventId) override;
    virtual void onReconnectionTimeSet(
        unsigned long long reconnectionTime) override;
    void scheduleReconnect();

    void failed();
    void failedAccessControlCheck();
    void cancel();
    void close();

#define VIRTUAL
#define OVERRIDE
    DECLARE_EVENT_LISTENER(loadstart);
    DECLARE_EVENT_LISTENER(progress);
    DECLARE_EVENT_LISTENER(abort);
    DECLARE_EVENT_LISTENER(error);
    DECLARE_EVENT_LISTENER(load);
    DECLARE_EVENT_LISTENER(timeout);
    DECLARE_EVENT_LISTENER(loadend);
    DECLARE_EVENT_LISTENER(message);
    DECLARE_EVENT_LISTENER(open);
#undef VIRTUAL
#undef OVERRIDE

private:
    void connect();
    ReadyState m_readyState;
    ResourceURL* m_url;
    bool m_withCredentials;
    int32_t m_delay;
    int32_t m_reconnectDelay;
    ResourceRequest* m_resourceRequest;
    EventSourceParser* m_parser;
    bool m_stopReconnect;
    uint32_t m_time;
};
}

#endif
