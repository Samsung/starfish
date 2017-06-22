/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_INSPECTOR)

#include "StarFishConfig.h"
#include "StarFish.h"
#include "Inspector.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "../third_party/rapidjson/include/rapidjson/document.h"
#include "../third_party/rapidjson/include/rapidjson/stringbuffer.h"
#include "../third_party/rapidjson/include/rapidjson/writer.h"

namespace StarFish {

Inspector::Inspector(StarFish* starFish, uint32_t portNumber)
    : m_starFish(starFish)
    , m_zmqContext(1)
    , m_zmqSocket(m_zmqContext, ZMQ_DEALER)
{
    m_ioThread = new Thread();
    std::string addr;
    addr = "tcp://127.0.0.1:";
    addr += std::to_string(portNumber);
    STARFISH_LOG_INFO("inspector open server %s\n", addr.c_str());
    try {
        m_zmqSocket.bind(addr);
        m_ioThread->run(
            starFish->messageLoop(),
            [](void* data) -> void* {
                Inspector* self = (Inspector*)data;
                while (true) {
                    zmq::message_t request;
                    try {
                        // STARFISH_LOG_INFO("inspector io thread wait\n");
                        if (!self->m_zmqSocket.recv(&request)) {
                            break;
                        }
                        if (request.size()) {
                            struct Request {
                                Inspector* inspector;
                                rapidjson::Document document;
                            };
                            Request* r = new Request;
                            r->inspector = self;
                            std::string s((char*)request.data(),
                                          (char*)request.data() +
                                              request.size());
                            r->document.Parse(s.data());

                            if (std::string(
                                    r->document["command"].GetString()) ==
                                "ping") {
                                rapidjson::Document document;
                                document.Parse("{}");
                                rapidjson::Value v;
                                v = "pong";
                                document.AddMember(
                                    rapidjson::Value("command",
                                                     document.GetAllocator()),
                                    v, document.GetAllocator());
                                rapidjson::Value v2;
                                v2 = rapidjson::Value(
                                    r->document["content"].GetString(),
                                    document.GetAllocator());
                                document.AddMember(
                                    rapidjson::Value("content",
                                                     document.GetAllocator()),
                                    v2, document.GetAllocator());
                                rapidjson::StringBuffer strbuf;
                                strbuf.Clear();

                                rapidjson::Writer<rapidjson::StringBuffer>
                                    writer(strbuf);
                                document.Accept(writer);

                                std::string ownShipRadarString =
                                    strbuf.GetString();
                                zmq::message_t request(
                                    ownShipRadarString.data(),
                                    ownShipRadarString.size());
                                self->m_zmqSocket.send(request, ZMQ_NOBLOCK);
                            } else {
                                self->m_starFish->messageLoop()
                                    ->addIdlerWithNoGCRootingInOtherThread(
                                        nullptr,
                                        [](size_t, void* data) {
                                            Request* r = (Request*)data;
                                            if (std::string(
                                                    r->document["command"]
                                                        .GetString()) ==
                                                "eval") {
                                                String* result =
                                                    r->inspector->m_starFish
                                                        ->evaluate(
                                                            String::fromUTF8(
                                                                r->document["co"
                                                                            "nt"
                                                                            "en"
                                                                            "t"]
                                                                    .GetString()));
                                                if (result->length()) {
                                                    r->inspector
                                                        ->sendInfoMessage(
                                                            result);
                                                }
                                            }
                                            delete r;
                                        },
                                        r);
                            }
                        }
                    } catch (...) {
                        STARFISH_LOG_INFO("inspector io thread error %d\n",
                                          zmq_errno());
                        break;
                    }
                }
                STARFISH_LOG_INFO("inspector io thread end\n");
                return nullptr;
            },
            this);
    } catch (...) {
        m_ioThread = nullptr;
    }
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
    v2 = rapidjson::Value(m->utf8Data(), strlen(m->utf8Data()));
    document.AddMember(rapidjson::Value("content", document.GetAllocator()), v2,
                       document.GetAllocator());
    rapidjson::StringBuffer strbuf;
    strbuf.Clear();

    rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
    document.Accept(writer);

    std::string ownShipRadarString = strbuf.GetString();
    zmq::message_t request(ownShipRadarString.data(),
                           ownShipRadarString.size());
    bool result = m_zmqSocket.send(request, ZMQ_NOBLOCK);
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
    v2 = rapidjson::Value(m->utf8Data(), strlen(m->utf8Data()));
    document.AddMember(rapidjson::Value("content", document.GetAllocator()), v2,
                       document.GetAllocator());
    rapidjson::StringBuffer strbuf;
    strbuf.Clear();

    rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
    document.Accept(writer);

    std::string ownShipRadarString = strbuf.GetString();
    zmq::message_t request(ownShipRadarString.data(),
                           ownShipRadarString.size());
    bool result = m_zmqSocket.send(request, ZMQ_NOBLOCK);
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
    v2 = rapidjson::Value(m->utf8Data(), strlen(m->utf8Data()));
    document.AddMember(rapidjson::Value("content", document.GetAllocator()), v2,
                       document.GetAllocator());
    rapidjson::StringBuffer strbuf;
    strbuf.Clear();

    rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
    document.Accept(writer);

    std::string ownShipRadarString = strbuf.GetString();
    zmq::message_t request(ownShipRadarString.data(),
                           ownShipRadarString.size());
    bool result = m_zmqSocket.send(request, ZMQ_NOBLOCK);
    // STARFISH_LOG_INFO("inspector::sendWarnMessage %d, %d\n", (int)result,
    // zmq_errno());
}

Inspector::~Inspector()
{
    if (!m_ioThread) {
        return;
    }
    m_zmqSocket.close();
    m_zmqContext.close();
}
}
#endif
