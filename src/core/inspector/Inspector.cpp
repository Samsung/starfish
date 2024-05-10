/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_INSPECTOR)

#include "StarfishConfig.h"
#include "Starfish.h"
#include "Inspector.h"
#include "core/page/WebView.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/threading/ThreadPool.h"

#include <nn.hpp>
#include <nanomsg/pair.h>

namespace Starfish {
struct InspectorRequest {
    Inspector* inspector;
    rapidjson::Document document;
};

Inspector::Inspector(WebView* wv)
    : m_webView(wv)
    , m_nnmSocket(nullptr)
    , m_addr()
    , m_isRunning(false)
{
}

Inspector::~Inspector()
{
    STARFISH_LOG_INFO("Inspector::~Inspector()");
    if (m_isRunning) {
        stop();
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
    auto str = m->toUTF8NonGCString();
    v2 = rapidjson::Value(str.c_str(), str.length());
    document.AddMember(rapidjson::Value("content", document.GetAllocator()), v2,
                       document.GetAllocator());
    rapidjson::StringBuffer strbuf;
    strbuf.Clear();

    rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
    document.Accept(writer);

    std::string ownShipRadarString = strbuf.GetString();
    try {
        m_nnmSocket->send(ownShipRadarString.data(), ownShipRadarString.size(),
                          0);
    } catch (const nn::exception& e) {
        STARFISH_LOG_INFO("sending is failed due to %s", e.what());
    }
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
    try {
        m_nnmSocket->send(ownShipRadarString.data(), ownShipRadarString.size(),
                          0);
    } catch (const nn::exception& e) {
        STARFISH_LOG_INFO("sending is failed due to %s", e.what());
    }
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
    try {
        m_nnmSocket->send(ownShipRadarString.data(), ownShipRadarString.size(),
                          0);
    } catch (const nn::exception& e) {
        STARFISH_LOG_INFO("sending is failed due to %s", e.what());
    }
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
    try {
        m_nnmSocket->send(ownShipRadarString.data(), ownShipRadarString.size(),
                          0);
    } catch (const nn::exception& e) {
        STARFISH_LOG_INFO("sending is failed due to %s", e.what());
    }
}

void Inspector::commandEvaluator(size_t, void* data)
{
    InspectorRequest* r = (InspectorRequest*)data;
    if (std::string(r->document["command"].GetString()) == "eval") {
        const char* str = r->document["content"].GetString();
        STARFISH_ASSERT(str != nullptr);
        String* result = r->inspector->m_webView->evaluateJavaScript(
            String::fromUTF8(str, strlen(str)));
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
    self->m_nnmSocket = new nn::socket(AF_SP, NN_PAIR);

    try {
        self->m_nnmSocket->bind(self->m_addr.c_str());
    } catch (const nn::exception& ex) {
        if (ex.num() == EADDRINUSE) {
            STARFISH_LOG_INFO("The requested address is already in use.");
        }
        goto exit;
    }

    while (self->m_isRunning) {
        char* buffer = nullptr;
        try {
            int nbytes = self->m_nnmSocket->recv(&buffer, NN_MSG, NN_DONTWAIT);

            if (nbytes > 0) {
                InspectorRequest* r = new InspectorRequest;
                r->inspector = self;
                std::string s(buffer, nbytes);
                r->document.Parse(s.c_str());

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
                    self->m_nnmSocket->send(ownShipRadarString.data(),
                                            ownShipRadarString.size(), 0);
                    delete r;
                } else {
                    self->m_webView->messageLoop()
                        ->addIdlerWithNoGCRootingInOtherThread(
                            nullptr, Inspector::commandEvaluator, r);
                }
                nn::freemsg(buffer);
            }
        } catch (const nn::exception& e) {
            STARFISH_LOG_INFO("recv failed due to %s", e.what());
            break;
        }
    }

exit:
    delete self->m_nnmSocket;
    self->m_nnmSocket = nullptr;
    self->m_isRunning = false;
    STARFISH_LOG_INFO("inspector io thread end");
    return nullptr;
}

void Inspector::run(uint32_t port)
{
    m_ioThread = new Thread(m_webView->threadPool());
    m_addr = "ws://0.0.0.0:";
    m_addr += std::to_string(port);
    STARFISH_LOG_INFO("inspector open server %s", m_addr.c_str());
    try {
        m_ioThread->run(m_webView->messageLoop(), Inspector::worker, this);
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
    m_ioThread->joinIfNeeds();
}
} // namespace Starfish
#endif
