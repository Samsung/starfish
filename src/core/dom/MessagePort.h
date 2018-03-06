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

#ifndef __StarFishMessagePort__
#define __StarFishMessagePort__

#include "core/dom/EventTarget.h"
#include "core/page/Serializer.h"

namespace StarFish {

class EventListener;
class MessagePort;

class PortMessageQueue : public gc {
public:
    PortMessageQueue()
        : m_enabled(false)
    {
    }
    void moveAllTasks(PortMessageQueue* other)
    {
        STARFISH_ASSERT(m_innerQueue.size() == 0);
        m_innerQueue.swap(other->m_innerQueue);
    }
    void clearAllTasks()
    {
        m_innerQueue.clear();
        m_innerQueue.shrink_to_fit();
    }
    void addTask(MessagePort* target, MessageEvent* event);
    void enableBy(MessagePort* target);
    bool enabled()
    {
        return m_enabled;
    }

protected:
    void registerTaskToMessageLoop(MessagePort* target, MessageEvent* event);

protected:
    bool m_enabled;
    GCVector<MessageEvent*> m_innerQueue;
};

class MessagePort : public EventTarget, public Transferable {
public:
    MessagePort(Document* document);

    // Interface ScriptWrappable
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isMessagePort() const override;
    virtual bool isTransferable() const override;
    virtual Transferable* toTransferable() const override;

    // Interface Transferable
    virtual TransferedData* transfer() override;
    virtual void transferReceive(TransferedData* transfered) override;

    void postMessage(ScriptValue message);
    void postMessage(ScriptValue message, GCVector<ScriptValue>& transfer);
    void start();
    void close();

    EventListener* onmessage();
    void setOnmessage(EventListener* listener);
    EventListener* onmessageerror();
    void setOnmessageerror(EventListener* listener);

    static void entangle(MessagePort* port1, MessagePort* port2);
    void disentangle();
    MessagePort* entangledPort() const
    {
        return m_entangledPort;
    }
    void setEntangledPort(MessagePort* port)
    {
        m_entangledPort = port;
    }
    void clearEntangledPort()
    {
        m_entangledPort = nullptr;
    }

    bool hasBeenShipped()
    {
        return m_hasBeenShipped;
    }
    void setHasBeenShipped()
    {
        m_hasBeenShipped = true;
    }

    PortMessageQueue* portMessageQueue()
    {
        return m_portMessageQueue;
    }

    void dispatchMessageEvent(MessageEvent* event);

protected:
    MessagePort* m_entangledPort;
    bool m_hasBeenShipped;
    PortMessageQueue* m_portMessageQueue;
};

class TransferedMessagePort : public TransferedPlatformObjectData {
public:
    TransferedMessagePort(MessagePort* origin)
        : m_remotePort(origin->entangledPort())
        , m_portMessageQueue(origin->portMessageQueue())
    {
    }

    ScriptWrappable* createTransferReceivingInstance(
        Document* document) const override
    {
        return new MessagePort(document);
    }
    MessagePort* remotePort()
    {
        return m_remotePort;
    }
    PortMessageQueue* portMessageQueue()
    {
        return m_portMessageQueue;
    }

protected:
    MessagePort* m_remotePort;
    PortMessageQueue* m_portMessageQueue;
};
}

#endif
