/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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
#include "core/dom/Document.h"
#include "core/dom/Event.h"
#include "core/dom/MessageEvent.h"
#include "core/modules/mediastream/WebRtcManager.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/threading/Thread.h"
#include "core/page/Navigator.h"
#include "core/page/WebBase.h"
#include "core/page/Window.h"
#include "core/page/GlobalScope.h"

namespace Starfish {

static const char* DataStateString(libwebrtc::RTCDataChannelState state)
{
    switch (state) {
    case libwebrtc::RTCDataChannelState::RTCDataChannelConnecting:
        return "connecting";
    case libwebrtc::RTCDataChannelState::RTCDataChannelOpen:
        return "open";
    case libwebrtc::RTCDataChannelState::RTCDataChannelClosing:
        return "closing";
    case libwebrtc::RTCDataChannelState::RTCDataChannelClosed:
        return "closed";
    }
    return "";
}

RTCDataChannelObserver::RTCDataChannelObserver(RTCDataChannel* dataChannel)
    : m_dataChannel(dataChannel)
{
}

void RTCDataChannelObserver::OnStateChange(libwebrtc::RTCDataChannelState state)
{
    if (!m_dataChannel) {
        return;
    }

    if (!isMainThread()) {
        struct Params {
            RTCDataChannelObserver* self;
            libwebrtc::RTCDataChannelState channelState;
        };

        Params* p = new Params{ this, state };
        m_dataChannel->executionContext()
            ->webBase()
            ->messageLoop()
            ->addIdlerWithNoGCRootingInOtherThread(
                m_dataChannel->executionContext()->globalScope(),
                [](size_t, void* data) {
                    Params* p = (Params*)data;
                    p->self->OnStateChange(p->channelState);
                    delete p;
                },
                p);
        return;
    }

    if (m_dataChannel && m_dataChannel->backend()) {
        std::string stateStr = DataStateString(state);
        String* eventType =
            String::createASCIIString(stateStr.data(), stateStr.length());
        Event* e = new Event(m_dataChannel->executionContext(), eventType);
        m_dataChannel->dispatchEventByUA(e);
    }
}

void RTCDataChannelObserver::OnMessage(const char* buffer, int length,
                                       bool binary)
{
#if 0
    if (!m_dataChannel) {
        return;
    }

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
#endif
}

RTCDataChannel::RTCDataChannel(
    ExecutionContext* executionContext, RTCDataChannelInit init,
    libwebrtc::scoped_refptr<libwebrtc::RTCDataChannel> dataChannel)
    : EventTarget()
    , m_executionContext(executionContext)
    , m_backend(dataChannel)
{
    m_protocol = init.m_protocol;
    m_observer = new RTCDataChannelObserver(this);
    m_backend->RegisterObserver(m_observer);

    m_webRtcManager =
        m_executionContext->document()->window()->navigator()->webRtcManager();

    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) { ((RTCDataChannel*)obj)->~RTCDataChannel(); },
        NULL, NULL, NULL);
}

RTCDataChannel::~RTCDataChannel()
{
    dispose();
}

void RTCDataChannel::dispose()
{
    if (!m_backend.get()) {
        STARFISH_LOG_WARN("RTCDataChannel[%p] is already diposed.", this);
        return;
    }

    m_backend->UnregisterObserver();
    m_backend = nullptr;
    m_observer = nullptr;
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
    std::string label = m_backend->label().std_string();
    return String::createASCIIString(label.data(), label.length());
}

bool RTCDataChannel::ordered()
{
    STARFISH_UNSUPPORTED(
        "Indicates messages in the same order where they are sent");
    return false;
}

Nullable<uint32_t> RTCDataChannel::maxPacketLifeTime()
{
    Nullable<uint32_t> ret;
    STARFISH_UNSUPPORTED("Indicates max number of ms for packet life time");
    return ret;
}

Nullable<uint32_t> RTCDataChannel::maxRetransmits()
{
    Nullable<uint32_t> ret;
    STARFISH_UNSUPPORTED("Indicates max number of times to retransmits");
    return ret;
}

bool RTCDataChannel::negotiated()
{
    STARFISH_UNSUPPORTED("Indicates this data channel is negotiated in-band");
    return false;
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
    std::string state = DataStateString(m_backend->state());
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
    // TODO:FIXME!!
    std::string dataStr = data->toUTF8NonGCString();
    m_backend->Send(reinterpret_cast<const uint8_t*>(dataStr.data()),
                    dataStr.length());
}

} // namespace Starfish

#endif
