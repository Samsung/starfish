/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_INSPECTOR)

#include "StarFishConfig.h"
#include "StarFish.h"
#include "Inspector.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "../third_party/rapidjson/include/rapidjson/document.h"
#include "../third_party/rapidjson/include/rapidjson/stringbuffer.h"
#include "../third_party/rapidjson/include/rapidjson/writer.h"

namespace StarFish {
struct Request {
    Inspector* inspector;
    rapidjson::Document document;
};

Inspector::Inspector(StarFish* starFish)
    : m_starFish(starFish)
    , m_zmqContext(new zmq::context_t(1))
    , m_zmqSocket(nullptr)
    , m_addr()
    , m_isRunning(false)
{
}

void Inspector::sendInfoMessage(String* m)
{
    if (!m_ioThread) {
        return;
    }
    rapidjson::Document document;
    document.Parse("{}");
    rapidjson::Value v;
    v = "console-info";
    document.AddMember(rapidjson::Value("command", document.GetAllocator()), v,
                       document.GetAllocator());
    rapidjson::Value v2;
    auto str = m->toUTF8NonGCString();
    v2 = rapidjson::Value(str.c_str(), str.length());
    document.AddMember(rapidjson::Value("content", document.GetAllocator()), v2,
                       document.GetAllocator());
    rapidjson::StringBuffer strbuf;
    strbuf.Clear();

    rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
    document.Accept(writer);

    std::string ownShipRadarString = strbuf.GetString();
    zmq::message_t request(ownShipRadarString.data(),
                           ownShipRadarString.size());
    bool result = m_zmqSocket->send(request, ZMQ_NOBLOCK);
    // STARFISH_LOG_INFO("inspector::sendInfoMessage %d, %d\n", (int)result,
    // zmq_errno());
}

void Inspector::sendErrorMessage(String* m)
{
    if (!m_ioThread) {
        return;
    }
    rapidjson::Document document;
    document.Parse("{}");
    rapidjson::Value v;
    v = "console-error";
    document.AddMember(rapidjson::Value("command", document.GetAllocator()), v,
                       document.GetAllocator());
    rapidjson::Value v2;
    auto str = m->toUTF8NonGCString();
    v2 = rapidjson::Value(str.c_str(), str.length());
    document.AddMember(rapidjson::Value("content", document.GetAllocator()), v2,
                       document.GetAllocator());
    rapidjson::StringBuffer strbuf;
    strbuf.Clear();

    rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
    document.Accept(writer);

    std::string ownShipRadarString = strbuf.GetString();
    zmq::message_t request(ownShipRadarString.data(),
                           ownShipRadarString.size());
    bool result = m_zmqSocket->send(request, ZMQ_NOBLOCK);
    // STARFISH_LOG_INFO("inspector::sendErrorMessage %d, %d\n", (int)result,
    // zmq_errno());
}

void Inspector::sendWarnMessage(String* m)
{
    if (!m_ioThread) {
        return;
    }
    rapidjson::Document document;
    document.Parse("{}");
    rapidjson::Value v;
    v = "console-warn";
    document.AddMember(rapidjson::Value("command", document.GetAllocator()), v,
                       document.GetAllocator());
    rapidjson::Value v2;
    auto str = m->toUTF8NonGCString();
    v2 = rapidjson::Value(str.c_str(), str.length());
    document.AddMember(rapidjson::Value("content", document.GetAllocator()), v2,
                       document.GetAllocator());
    rapidjson::StringBuffer strbuf;
    strbuf.Clear();

    rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
    document.Accept(writer);

    std::string ownShipRadarString = strbuf.GetString();
    zmq::message_t request(ownShipRadarString.data(),
                           ownShipRadarString.size());
    bool result = m_zmqSocket->send(request, ZMQ_NOBLOCK);
    // STARFISH_LOG_INFO("inspector::sendWarnMessage %d, %d\n", (int)result,
    // zmq_errno());
}

void Inspector::sendDebugMessage(String* m)
{
    if (!m_ioThread) {
        return;
    }
    rapidjson::Document document;
    document.Parse("{}");
    rapidjson::Value v;
    v = "console-debug";
    document.AddMember(rapidjson::Value("command", document.GetAllocator()), v,
                       document.GetAllocator());
    rapidjson::Value v2;
    auto str = m->toUTF8NonGCString();
    v2 = rapidjson::Value(str.c_str(), str.length());
    document.AddMember(rapidjson::Value("content", document.GetAllocator()), v2,
                       document.GetAllocator());
    rapidjson::StringBuffer strbuf;
    strbuf.Clear();

    rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
    document.Accept(writer);

    std::string ownShipRadarString = strbuf.GetString();
    zmq::message_t request(ownShipRadarString.data(),
                           ownShipRadarString.size());
    bool result = m_zmqSocket->send(request, ZMQ_NOBLOCK);
    // STARFISH_LOG_INFO("inspector::sendDebugMessage %d, %d\n", (int)result,
    // zmq_errno());
}

Inspector::~Inspector()
{
    STARFISH_LOG_INFO("Inspector::~Inspector()\n");
    if (m_isRunning) {
        stop();
    }
}

void Inspector::commandEvaluator(size_t, void* data)
{
    Request* r = (Request*)data;
    if (std::string(r->document["command"].GetString()) == "eval") {
        String* result = r->inspector->m_starFish->evaluate(
            String::fromUTF8(r->document["content"].GetString()));
        if (result->length()) {
            r->inspector->sendInfoMessage(result);
        }
    }
    delete r;
}

void* Inspector::worker(void* data)
{
    Inspector* self = (Inspector*)data;
    self->m_isRunning = true;
    self->m_zmqSocket = new zmq::socket_t(*(self->m_zmqContext), ZMQ_DEALER);

    try {
        self->m_zmqSocket->bind(self->m_addr);
    } catch (const zmq::error_t& ex) {
        if (ex.num() == EADDRINUSE) {
            STARFISH_LOG_INFO("The requested address is already in use.\n");
        }
        self->m_isRunning = false;
        return nullptr;
    }

    while (self->m_isRunning) {
        zmq::message_t request;
        try {
            // STARFISH_LOG_INFO("inspector io thread wait\n");
            if (!self->m_zmqSocket->recv(&request)) {
                break;
            }
            if (request.size()) {
                Request* r = new Request;
                r->inspector = self;
                std::string s((char*)request.data(),
                              (char*)request.data() + request.size());
                r->document.Parse(s.data());

                if (std::string(r->document["command"].GetString()) == "ping") {
                    rapidjson::Document document;
                    document.Parse("{}");
                    rapidjson::Value v;
                    v = "pong";
                    document.AddMember(
                        rapidjson::Value("command", document.GetAllocator()), v,
                        document.GetAllocator());
                    rapidjson::Value v2;
                    v2 = rapidjson::Value(r->document["content"].GetString(),
                                          document.GetAllocator());
                    document.AddMember(
                        rapidjson::Value("content", document.GetAllocator()),
                        v2, document.GetAllocator());
                    rapidjson::StringBuffer strbuf;
                    strbuf.Clear();

                    rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
                    document.Accept(writer);

                    std::string ownShipRadarString = strbuf.GetString();
                    zmq::message_t request(ownShipRadarString.data(),
                                           ownShipRadarString.size());
                    self->m_zmqSocket->send(request, ZMQ_NOBLOCK);

                    delete r;
                } else {
                    self->m_starFish->messageLoop()
                        ->addIdlerWithNoGCRootingInOtherThread(
                            nullptr, Inspector::commandEvaluator, r);
                }
            }
        } catch (const zmq::error_t& ex) {
            // recv() throws ETERM when the zmq context is
            // destroyed,
            if (ex.num() == ETERM) {
                STARFISH_LOG_INFO("zmq context was deleted\n");
            } else {
                STARFISH_LOG_INFO("inspector io thread error %d\n",
                                  zmq_errno());
            }
            break;
        }
    }
    delete self->m_zmqSocket;
    STARFISH_LOG_INFO("inspector io thread end\n");
    return nullptr;
}

void Inspector::run(uint32_t port)
{
    m_ioThread = new Thread(m_starFish);
    m_addr = "tcp://0.0.0.0:";
    m_addr += std::to_string(port);
    STARFISH_LOG_INFO("inspector open server %s\n", m_addr.c_str());
    try {
        m_ioThread->run(m_starFish->messageLoop(), Inspector::worker, this);
    } catch (...) {
        m_ioThread = nullptr;
    }
}

void Inspector::stop()
{
    STARFISH_ASSERT(isMainThread());
    m_isRunning = false;

    if (!m_ioThread) {
        return;
    }
    if (m_zmqContext) {
        delete m_zmqContext;
    }
    m_ioThread->joinIfNeeds();
}
} // namespace StarFish
#endif
