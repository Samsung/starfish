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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/DOMException.h"
#include "core/dom/Document.h"
#include "core/dom/MessageEvent.h"
#include "core/dom/MessagePort.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/Serializer.h"
#include "core/page/Window.h"

namespace StarFish {

MessagePort::MessagePort(Document* document)
    : EventTarget(document)
    , m_entangledPort(nullptr)
    , m_hasBeenShipped(false)
    , m_portMessageQueue(new PortMessageQueue())
{
}

TransferedData* MessagePort::transfer()
{
    setHasBeenShipped();
    if (m_entangledPort) {
        m_entangledPort->setHasBeenShipped();
    }
    return new TransferedMessagePort(this);
}

void MessagePort::transferReceive(TransferedData* transfered)
{
    STARFISH_ASSERT(transfered->isTransferedPlatformObjectData());
    TransferedMessagePort* holder =
        (TransferedMessagePort*)(transfered->asTransferedPlatformObjectData());
    setHasBeenShipped();
    holder->portMessageQueue()->moveAllTasks(m_portMessageQueue);
    entangle(this, holder->remotePort());
}

void MessagePort::entangle(MessagePort* port1, MessagePort* port2)
{
    STARFISH_ASSERT(port1 != port2);
    if (port1->entangledPort() == port2) {
        STARFISH_ASSERT(port2->entangledPort() == port1);
        return;
    }
    // If one of the ports is already entangled, then disentangle it and
    // the port that it was entangled with.
    port1->disentangle();
    port2->disentangle();
    // Associate the two ports to be entangled, so that they form the two
    // parts of a new channel.
    port1->setEntangledPort(port2);
    port2->setEntangledPort(port1);
}

void MessagePort::disentangle()
{
    if (m_entangledPort) {
        STARFISH_ASSERT(m_entangledPort->entangledPort() == this);
        m_entangledPort->clearEntangledPort();
        clearEntangledPort();
    }
}

void MessagePort::postMessage(ScriptValue message)
{
    GCVector<ScriptValue> emptyTransfer;
    postMessage(message, emptyTransfer);
}

void MessagePort::postMessage(ScriptValue message,
                              GCVector<ScriptValue>& transfer)
{
    // https://html.spec.whatwg.org/multipage/web-messaging.html#dom-messageport-postmessage
    // Let targetPort be the port with which this MessagePort is entangled,
    // if any; otherwise let it be null.
    MessagePort* targetPort = m_entangledPort;
    // Let doomed be false.
    bool doomed = false;
    // If any of the objects in transfer are this MessagePort,
    // then throw a "DataCloneError" DOMException.
    //
    // If targetPort is not null and any of the objects in transfer are
    // targetPort, then set doomed to true, and optionally report to a developer
    // console that the target port was posted to itself, causing the
    // communication channel to be lost.
    for (size_t i = 0; i < transfer.size(); i++) {
        ScriptWrappable* sw = toScriptWrappable(transfer[i]);
        if (sw && sw->isMessagePort()) {
            MessagePort* port = sw->asMessagePort();
            if (port == this) {
                throw new DOMException(document(),
                                       DOMException::DATA_CLONE_ERR);
            } else if (targetPort && !doomed && port == targetPort) {
                doomed = true;
            }
        }
    }
    // Let serializeWithTransferResult be
    // StructuredSerializeWithTransfer(message, transfer).
    // Rethrow any exceptions.
    SerializeWithTransferResult* serializedRecord =
        new (GC) SerializeWithTransferResult();
    // TODO use memoryMap to check targetPort has been transfered
    Serializer::serializeWithTransfer(document(), message, transfer,
                                      *serializedRecord);
    // If there is no targetPort (i.e. if this MessagePort is not entangled),
    // or if doomed is true, then return.
    if (!targetPort || doomed) {
        return;
    }
    // NOTE addIder would hold serializedRecord
    starFish()->messageLoop()->addIdler(
        document()->browsingContext(),
        [](size_t handle, void* data, void* data1) {
            MessagePort* self = (MessagePort*)data;
            SerializeWithTransferResult* serializedRecord =
                (SerializeWithTransferResult*)data1;
            DeserializeWithTransferResult deserializedRecord;
            try {
                Serializer::deserializeWithTransfer(
                    self->document(), *serializedRecord, deserializedRecord);
            } catch (DOMException* exc) {
                MessageEvent* e = new MessageEvent(
                    self->document(), self->starFish()
                                          ->staticStrings()
                                          ->m_messageerror.localName());
                self->entangledPort()->dispatchMessageEvent(e);
                return;
            }
            GCVector<MessagePort*> newPorts;
            for (size_t i = 0;
                 i < deserializedRecord.m_deserializedTransfer.size(); i++) {
                ScriptValue item = deserializedRecord.m_deserializedTransfer[i];
                STARFISH_ASSERT(isObjectScriptValue(item));
                ScriptWrappable* sw = toScriptWrappable(item);
                if (sw && sw->isMessagePort()) {
                    newPorts.push_back(sw->asMessagePort());
                }
            }
            MessageEvent* e = new MessageEvent(
                self->document(),
                self->starFish()->staticStrings()->m_message.localName());
            e->setData(deserializedRecord.m_deserialized);
            e->setPorts(newPorts);
            self->entangledPort()->dispatchMessageEvent(e);
        },
        this, serializedRecord);
}

void MessagePort::start()
{
    m_portMessageQueue->enableBy(this);
}

void MessagePort::close()
{
    disentangle();
    m_portMessageQueue->clearAllTasks();
}

EventListener* MessagePort::onmessage()
{
    QualifiedName attr = window()->starFish()->staticStrings()->m_message;
    return attributeEventListener(attr);
}

void MessagePort::setOnmessage(EventListener* listener)
{
    QualifiedName attr = window()->starFish()->staticStrings()->m_message;
    if (listener) {
        setAttributeEventListener(attr, listener);
    } else {
        clearAttributeEventListener(attr);
    }
    // The first time a MessagePort object's onmessage IDL attribute is set,
    // the port's port message queue must be enabled, as if the start() method
    // had been called.
    start();
}

EventListener* MessagePort::onmessageerror()
{
    QualifiedName attr = window()->starFish()->staticStrings()->m_messageerror;
    return attributeEventListener(attr);
}

void MessagePort::setOnmessageerror(EventListener* listener)
{
    QualifiedName attr = window()->starFish()->staticStrings()->m_messageerror;
    if (listener) {
        setAttributeEventListener(attr, listener);
    } else {
        clearAttributeEventListener(attr);
    }
}

void MessagePort::dispatchMessageEvent(MessageEvent* event)
{
    m_portMessageQueue->addTask(this, event);
}

void PortMessageQueue::addTask(MessagePort* target, MessageEvent* event)
{
    if (m_enabled) {
        registerTaskToMessageLoop(target, event);
    } else {
        m_innerQueue.push_back(event);
    }
}

void PortMessageQueue::enableBy(MessagePort* target)
{
    if (!m_enabled) {
        m_enabled = true;
        for (size_t i = 0; i < m_innerQueue.size(); i++) {
            registerTaskToMessageLoop(target, m_innerQueue[i]);
        }
        clearAllTasks();
    }
#ifndef NDEBUG
    else {
        STARFISH_ASSERT(m_innerQueue.size() == 0);
    }
#endif
}

void PortMessageQueue::registerTaskToMessageLoop(MessagePort* target,
                                                 MessageEvent* event)
{
    STARFISH_ASSERT(m_enabled);
    target->starFish()->messageLoop()->addIdler(
        target->document()->browsingContext(),
        [](size_t, void* data, void* data1) {
            MessagePort* target = (MessagePort*)data;
            MessageEvent* event = (MessageEvent*)data1;
            target->dispatchEventByUA(event);
        },
        target, event);
}
}
