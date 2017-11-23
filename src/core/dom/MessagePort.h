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
