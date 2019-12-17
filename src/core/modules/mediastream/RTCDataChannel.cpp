/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_WEBRTC)

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/modules/mediastream/RTCDataChannel.h"

#include "core/dom/ExecutionContext.h"
#include "core/dom/Event.h"
#include "core/dom/MessageEvent.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/threading/Thread.h"
#include "core/page/WebBase.h"
#include "core/page/GlobalScope.h"

namespace Starfish {

RTCDataChannelObserver::RTCDataChannelObserver(RTCDataChannel* dataChannel)
    : m_dataChannel(dataChannel)
{
}

void RTCDataChannelObserver::OnStateChange()
{
    if (!isMainThread()) {
        m_dataChannel->executionContext()
            ->webBase()
            ->messageLoop()
            ->addIdlerWithNoGCRootingInOtherThread(
                m_dataChannel->executionContext()->globalScope(),
                [](size_t, void* data) {
                    RTCDataChannelObserver* observer =
                        (RTCDataChannelObserver*)data;
                    observer->OnStateChange();
                },
                this);
        return;
    }

    if (m_dataChannel && m_dataChannel->backend()) {
        std::string state = webrtc::DataChannelInterface::DataStateString(
            m_dataChannel->backend()->state());

        String* eventType =
            String::createASCIIString(state.data(), state.length());
        Event* e = new Event(m_dataChannel->executionContext(), eventType);
        m_dataChannel->dispatchEventByUA(e);
    }
}

void RTCDataChannelObserver::OnMessage(const webrtc::DataBuffer& buffer)
{
    if (!isMainThread()) {
        struct Params {
            RTCDataChannelObserver* self;
            webrtc::DataBuffer buffer;
        };

        Params* p = new Params{ this, buffer };

        m_dataChannel->executionContext()
            ->webBase()
            ->messageLoop()
            ->addIdlerWithNoGCRootingInOtherThread(
                m_dataChannel->executionContext()->globalScope(),
                [](size_t, void* data) {
                    Params* p = (Params*)data;
                    p->self->OnMessage(p->buffer);
                    delete p;
                },
                p);
        return;
    }

    if (m_dataChannel && m_dataChannel->backend()) {
        String* eventType = m_dataChannel->executionContext()
                                ->starfish()
                                ->staticStrings()
                                ->m_message.localName();
        String* data =
            String::createASCIIString(buffer.data.data<char>(), buffer.size());
        MessageEventInit init;
        init.setData(createScriptString(data));
        MessageEvent* e = new MessageEvent(m_dataChannel->executionContext(),
                                           eventType, init);
        m_dataChannel->dispatchEventByUA(e);
    }
}

RTCDataChannel::RTCDataChannel(
    ExecutionContext* executionContext, RTCDataChannelInit init,
    rtc::scoped_refptr<webrtc::DataChannelInterface> dataChannel)
    : EventTarget()
    , m_executionContext(executionContext)
    , m_backend(dataChannel)
{
    m_protocol = init.m_protocol;

    m_observer = new RTCDataChannelObserver(this);
    m_backend->RegisterObserver(m_observer);

    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) { ((RTCDataChannel*)obj)->~RTCDataChannel(); },
        NULL, NULL, NULL);
}

RTCDataChannel::~RTCDataChannel()
{
    if (m_backend) {
        m_backend->UnregisterObserver();
    }
    m_backend = nullptr;
}

ExecutionContext* RTCDataChannel::executionContext() const
{
    return m_executionContext;
}

ScriptBindingInstance* RTCDataChannel::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

String* RTCDataChannel::label()
{
    std::string label = m_backend->label();
    return String::createASCIIString(label.data(), label.length());
}

bool RTCDataChannel::ordered()
{
    return m_backend->ordered();
}

Nullable<uint32_t> RTCDataChannel::maxPacketLifeTime()
{
    Nullable<uint32_t> ret;
    absl::optional<int> r = m_backend->maxPacketLifeTime();
    if (r.has_value()) {
        ret = r.value();
    }
    return ret;
}

Nullable<uint32_t> RTCDataChannel::maxRetransmits()
{
    Nullable<uint32_t> ret;
    absl::optional<int> r = m_backend->maxRetransmitsOpt();
    if (r.has_value()) {
        ret = r.value();
    }
    return ret;
}

bool RTCDataChannel::negotiated()
{
    return m_backend->negotiated();
}

Nullable<uint32_t> RTCDataChannel::id()
{
    Nullable<uint32_t> ret;
    int id = m_backend->id();
    if (id >= 0) {
        ret = id;
    }
    return ret;
}

String* RTCDataChannel::readyState()
{
    std::string state =
        webrtc::DataChannelInterface::DataStateString(m_backend->state());
    return String::createASCIIString(state.data(), state.length());
}

DEFINE_EVENT_LISTENER(RTCDataChannel, open);
DEFINE_EVENT_LISTENER(RTCDataChannel, bufferedamountlow);
DEFINE_EVENT_LISTENER(RTCDataChannel, error);
DEFINE_EVENT_LISTENER(RTCDataChannel, closing);
DEFINE_EVENT_LISTENER(RTCDataChannel, close);
DEFINE_EVENT_LISTENER(RTCDataChannel, message);

String* RTCDataChannel::binaryType()
{
    return m_binaryType;
}

void RTCDataChannel::setBinaryType(String* binaryType)
{
    m_binaryType = binaryType;
}

void RTCDataChannel::send(String* data)
{
    webrtc::DataBuffer buffer(data->toUTF8NonGCString().data());
    m_backend->Send(buffer);
}

} // namespace Starfish

#endif
