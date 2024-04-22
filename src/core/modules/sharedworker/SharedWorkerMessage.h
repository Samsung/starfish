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
#ifndef __StarfishSharedWorkerMessage__
#define __StarfishSharedWorkerMessage__

#include "core/modules/sharedworker/IPCMessageHandler.h"

namespace Starfish {

class SharedWorker;
class IPCMessageSerializer;
class IPCMessageDeserializer;

namespace SharedWorkerMessage {

#if defined(STARFISH_ENABLE_TEST)
    class SharedWorkerMessageTest : public IPCMessage {
    public:
        static const char* messageID()
        {
            return "sharedWorkerMessageTest";
        }

        IPCMessageSerializer* serialize() override;
        void deserialize(IPCMessageDeserializer* deserializer) override;
    };
#endif

    class RequestGetSharedWorker : public IPCMessage {
    public:
        static const char* messageID()
        {
            return "requestGetSharedWorker";
        }

        RequestGetSharedWorker() = default;
        RequestGetSharedWorker(SharedWorker* sharedWorker);

        IPCMessageSerializer* serialize() override;
        void deserialize(IPCMessageDeserializer* deserializer) override;

        DEFINE_GETTER(uint32_t, clientID);
        DEFINE_GETTER(size_t, sharedWorkerKey);
        DEFINE_GETTER(const std::string&, url);
        DEFINE_GETTER(const std::string&, name);

    private:
        uint32_t m_clientID;
        size_t m_sharedWorkerKey;
        std::string m_url;
        std::string m_name;
    };

} // namespace SharedWorkerMessage

} // namespace Starfish

#endif
#endif
