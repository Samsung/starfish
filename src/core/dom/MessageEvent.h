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

#ifndef __StarFishMessageEvent__
#define __StarFishMessageEvent__

#include "Event.h"

namespace StarFish {

class MessagePort;

// https://html.spec.whatwg.org/multipage/comms.html#messageeventinit
struct MessageEventInit : EventInit {
    STARFISH_MAKE_STACK_ALLOCATED()
public:
    MessageEventInit()
        : EventInit()
        , m_data(scriptNull())
        , m_origin(String::emptyString)
        , m_lastEventId(String::emptyString)
        , m_source(nullptr)
    {
    }

    ScriptValue data() const
    {
        return m_data;
    }

    void setData(ScriptValue data)
    {
        m_data = data;
    }

    GCVector<MessagePort*> ports() const
    {
        return m_ports;
    }

    void setPorts(GCVector<MessagePort*>& ports)
    {
        m_ports = ports;
    }

    String* origin() const
    {
        return m_origin;
    }

    void setOrigin(String* origin)
    {
        m_origin = origin;
    }

    String* lastEventId() const
    {
        return m_lastEventId;
    }

    void setLastEventId(String* lastEventId)
    {
        m_lastEventId = lastEventId;
    }

    Window* source() const
    {
        return m_source;
    }

    void setSource(Window* source)
    {
        m_source = source;
    }

private:
    ScriptValue m_data;
    GCVector<MessagePort*> m_ports;
    String* m_origin;
    String* m_lastEventId;
    // FIXME: Should change this type to MessageEventSource*
    Window* m_source;
};

class MessageEvent : public Event {
public:
    MessageEvent(Document* document)
        : Event(document)
        , m_data(scriptNull())
        , m_origin(String::emptyString)
        , m_lastEventId(String::emptyString)
        , m_source(nullptr)
    {
    }

    MessageEvent(Document* document, String* eventType)
        : Event(document, eventType)
        , m_data(scriptNull())
        , m_origin(String::emptyString)
        , m_lastEventId(String::emptyString)
        , m_source(nullptr)
    {
    }

    MessageEvent(Document* document, String* eventType,
                 const MessageEventInit& init)
        : Event(document, eventType, init)
        , m_data(init.data())
        , m_ports(init.ports())
        , m_origin(init.origin())
        , m_lastEventId(init.lastEventId())
        , m_source(nullptr)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isMessageEvent() const override;

    ScriptValue data() const
    {
        return m_data;
    }

    void setData(ScriptValue data)
    {
        m_data = data;
    }

    GCVector<MessagePort*> ports()
    {
        return m_ports;
    }

    void setPorts(GCVector<MessagePort*>& ports)
    {
        m_ports = ports;
    }

    String* origin() const
    {
        return m_origin;
    }

    void setOrigin(String* origin)
    {
        m_origin = origin;
    }

    String* lastEventId() const
    {
        return m_lastEventId;
    }

    void setLastEventId(String* lastEventId)
    {
        m_lastEventId = lastEventId;
    }

    Window* source() const
    {
        return m_source;
    }

    void setSource(Window* source)
    {
        m_source = source;
    }

    void initMessageEvent(String* type, bool bubbles, bool cancelable,
                          ScriptValue data, String* origin, String* lastEventId,
                          Window* source)
    {
        initEvent(type, bubbles, cancelable);
        m_data = data;
        m_origin = origin;
        m_lastEventId = lastEventId;
        m_source = source;
    }

private:
    ScriptValue m_data;
    GCVector<MessagePort*> m_ports;
    String* m_origin;
    String* m_lastEventId;
    // FIXME: Should change this type to MessageEventSource*
    Window* m_source;
};
}

#endif
