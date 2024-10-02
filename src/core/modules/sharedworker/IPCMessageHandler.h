/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_SHARED_WORKER)
#ifndef __StarfishIPCMessageHandler__
#define __StarfishIPCMessageHandler__

namespace Starfish {

class IPCMessageSerializer;
class IPCMessageDeserializer;
class Connection;

class IPCMessage {
public:
    virtual IPCMessageSerializer* serialize() = 0;
    virtual void deserialize(IPCMessageDeserializer* deserializer) = 0;
};

class IPCMessageHandler : public gc {
public:
    using MessageReceiveHandler = void (*)(IPCMessageDeserializer*);

    Optional<IPCMessageSerializer*> serialize(IPCMessage& message);

    void sendMessage(Connection* connection, IPCMessage& message);

    void onReceiveMessage(const char* data, size_t length);

    void setMessageReceiveHandler(const std::string& id,
                                  MessageReceiveHandler handler);

private:
    GCAtomicUnorderedMap<std::string, MessageReceiveHandler> m_receiveHandlers;
};

} // namespace Starfish

#endif
#endif
