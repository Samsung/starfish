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

#if defined(STARFISH_ENABLE_CDP)

#include "StarfishConfig.h"
#include "Starfish.h"
#include "CDPCommand.h"
#include "CDPDispatcher.h"
#include "CDPServer.h"

#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace Starfish {

CDPCommand::CDPCommand(CDPDispatcher* d, Optional<int64_t> id,
                       const std::string& sessionId, rapidjson::Value* params)
    : m_dispatcher(d)
    , m_id(id)
    , m_sessionId(sessionId)
    , m_params(params)
{
}

void CDPCommand::emit(rapidjson::Document& doc)
{
    rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
    if (!m_sessionId.empty()) {
        doc.AddMember(
            "sessionId",
            rapidjson::Value(m_sessionId.c_str(), m_sessionId.size(), alloc),
            alloc);
    }
    rapidjson::StringBuffer buf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
    doc.Accept(writer);
    m_dispatcher->server()->sendText(
        std::string(buf.GetString(), buf.GetSize()));
}

void CDPCommand::sendResult(rapidjson::Value& result, rapidjson::Document& doc)
{
    rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
    doc.SetObject();
    if (m_id.hasValue()) {
        doc.AddMember("id", (int64_t)m_id.value(), alloc);
    }
    doc.AddMember("result", result, alloc);
    emit(doc);
}

void CDPCommand::sendResultEmpty()
{
    rapidjson::Document doc;
    rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
    doc.SetObject();
    if (m_id.hasValue()) {
        doc.AddMember("id", (int64_t)m_id.value(), alloc);
    }
    rapidjson::Value empty(rapidjson::kObjectType);
    doc.AddMember("result", empty, alloc);
    emit(doc);
}

void CDPCommand::sendError(int code, const char* message)
{
    rapidjson::Document doc;
    rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
    doc.SetObject();
    if (m_id.hasValue()) {
        doc.AddMember("id", (int64_t)m_id.value(), alloc);
    }
    rapidjson::Value error(rapidjson::kObjectType);
    error.AddMember("code", code, alloc);
    error.AddMember("message", rapidjson::Value(message, alloc), alloc);
    doc.AddMember("error", error, alloc);
    emit(doc);
}

void CDPCommand::sendEvent(const char* method, rapidjson::Value& params,
                           rapidjson::Document& doc)
{
    rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
    doc.SetObject();
    doc.AddMember("method", rapidjson::Value(method, alloc), alloc);
    doc.AddMember("params", params, alloc);
    emit(doc);
}

} // namespace Starfish

#endif
