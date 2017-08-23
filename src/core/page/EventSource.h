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

    ResourceRequest* resourceRequest()
    {
        return m_resourceRequest;
    }

    enum State : short { Connecting = 0, Open = 1, Closed = 2 };

    String* url() const
    {
        return m_url->urlString();
    }

    bool withCredentials() const
    {
        return m_withCredentials;
    }

    State readyState() const
    {
        return m_state;
    }

    void start(ResourceRequest::MethodType method);
    void initResponseData();

    void didHeaderReceived(const ResponseHeaderMap& headrs);
    void didDataReceived(const char*, size_t length);

    void onMessageEvent(String* type, String* data, String* lastEventId);
    void onReconnectionTimeSet(unsigned long long reconnectionTime);

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
#undef VIRTUAL
#undef OVERRIDE

private:
    void connect();

    ResourceURL* m_url;
    bool m_withCredentials;
    State m_state;
    int32_t m_reconnectDelay;
    ResourceRequest* m_resourceRequest;
    EventSourceParser* m_parser;
};
}

#endif
