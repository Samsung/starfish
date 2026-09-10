/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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

#include "Session.h"

#include "CDPClient.h"
#include "Snapshot.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace StarfishCLI {

namespace {

    std::string jsonString(const std::string& value)
    {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        writer.String(value.c_str(),
                      static_cast<rapidjson::SizeType>(value.size()));
        return buffer.GetString();
    }

} // namespace

Session::Session(CDPClient& client)
    : m_client(client)
{
}

bool Session::command(const std::string& method, const std::string& parameters,
                      std::string& result, std::string& error)
{
    return m_client.sendCommand(method, parameters, result, error, m_sessionId);
}

bool Session::setup(std::string& error)
{
    std::string result;
    if (!m_client.sendCommand("Target.setDiscoverTargets",
                              "{\"discover\":true}", result, error)) {
        return false;
    }
    m_client.expectEvent("Target.attachedToTarget");
    if (!m_client.sendCommand("Target.setAutoAttach",
                              "{\"autoAttach\":true,"
                              "\"waitForDebuggerOnStart\":false,"
                              "\"flatten\":true}",
                              result, error)) {
        return false;
    }

    std::string parameters;
    if (!m_client.waitForEvent("Target.attachedToTarget", parameters, 10000)) {
        error = "timeout waiting for Target.attachedToTarget";
        return false;
    }

    rapidjson::Document document;
    document.Parse(parameters.c_str());
    if (document.HasParseError() || !document.HasMember("sessionId") ||
        !document["sessionId"].IsString()) {
        error = "missing sessionId in Target.attachedToTarget";
        return false;
    }
    m_sessionId = document["sessionId"].GetString();

    if (!command("Page.enable", "{}", result, error)) {
        return false;
    }
    return command("Accessibility.enable", "{}", result, error);
}

bool Session::open(const std::string& url, std::string& error)
{
    std::string result;
    std::string parameters = "{\"url\":" + jsonString(url) + "}";
    m_client.expectEvent("Page.loadEventFired");
    if (!command("Page.navigate", parameters, result, error)) {
        return false;
    }

    std::string eventParameters;
    if (!m_client.waitForEvent("Page.loadEventFired", eventParameters, 30000)) {
        error = "timeout waiting for Page.loadEventFired";
        return false;
    }
    return true;
}

bool Session::snapshotInteractive(std::string& output, std::string& error)
{
    std::string result;
    if (!command("Accessibility.getFullAXTree", "{}", result, error)) {
        return false;
    }

    return formatInteractiveSnapshot(result, output, error);
}

} // namespace StarfishCLI
