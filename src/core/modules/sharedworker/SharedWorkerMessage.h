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

#include "core/modules/worker/WorkerHostInitData.h"
#include "core/modules/sharedworker/IPCMessageHandler.h"

namespace Starfish {

class SharedWorker;
class IPCMessageSerializer;
class IPCMessageDeserializer;
class SharedWorkerMessagePortConnection;

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
        DEFINE_GETTER(uint32_t, pid);
        DEFINE_GETTER(size_t, sharedWorkerKey);
        DEFINE_GETTER(const std::string&, name);
        DEFINE_GETTER(const WorkerHostInitData&, workerHostInitData);

    private:
        uint32_t m_clientID;
        uint32_t m_pid;
        size_t m_sharedWorkerKey;
        std::string m_name;
        WorkerHostInitData m_workerHostInitData;
    };

    class ResponseGetSharedWorker : public IPCMessage {
    public:
        static const char* messageID()
        {
            return "responseGetSharedWorker";
        }

        ResponseGetSharedWorker() = default;
        ResponseGetSharedWorker(SharedWorkerMessagePortConnection* connection);

        IPCMessageSerializer* serialize() override;
        void deserialize(IPCMessageDeserializer* deserializer) override;

        DEFINE_GETTER(uint32_t, clientID);
        DEFINE_GETTER(uint32_t, pid);
        DEFINE_GETTER(const std::string&, ipcAddress);

    private:
        uint32_t m_clientID;
        uint32_t m_pid;
        std::string m_ipcAddress;
    };

    class RequestCloseSharedWorker : public IPCMessage {
    public:
        static const char* messageID()
        {
            return "requestCloseSharedWorker";
        }

        RequestCloseSharedWorker();

        IPCMessageSerializer* serialize() override;
        void deserialize(IPCMessageDeserializer* deserializer) override;

        DEFINE_GETTER(uint32_t, pid);

    private:
        uint32_t m_pid;
    };

} // namespace SharedWorkerMessage

} // namespace Starfish

#endif
#endif
