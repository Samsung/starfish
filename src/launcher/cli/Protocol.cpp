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

#include "Protocol.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>

namespace StarfishCLI {

namespace {

    using JsonWriter = rapidjson::Writer<rapidjson::StringBuffer>;

    void writeString(JsonWriter& writer, const std::string& value)
    {
        writer.String(value.c_str(),
                      static_cast<rapidjson::SizeType>(value.size()));
    }

    std::string stringMember(const rapidjson::Document& document,
                             const char* name)
    {
        if (!document.HasMember(name) || !document[name].IsString()) {
            return std::string();
        }
        return document[name].GetString();
    }

    std::string makeRequest(const char* command, const std::string& url)
    {
        rapidjson::StringBuffer buffer;
        JsonWriter writer(buffer);
        writer.StartObject();
        writer.Key("command");
        writer.String(command);
        if (!url.empty()) {
            writer.Key("url");
            writeString(writer, url);
        }
        writer.EndObject();
        return std::string(buffer.GetString()) + "\n";
    }

} // namespace

std::string makeOpenRequest(const std::string& url)
{
    return makeRequest(kCommandOpen, url);
}

std::string makeSnapshotRequest()
{
    return makeRequest(kCommandSnapshot, std::string());
}

std::string makeCloseRequest()
{
    return makeRequest(kCommandClose, std::string());
}

bool parseRequest(const std::string& input, Request& request)
{
    rapidjson::Document document;
    document.Parse(input.c_str());
    if (document.HasParseError() || !document.IsObject()) {
        return false;
    }

    request.command = stringMember(document, "command");
    request.url = stringMember(document, "url");
    return !request.command.empty();
}

std::string makeOkResponse(const std::string& result)
{
    rapidjson::StringBuffer buffer;
    JsonWriter writer(buffer);
    writer.StartObject();
    writer.Key("ok");
    writer.Bool(true);
    if (!result.empty()) {
        writer.Key("result");
        writeString(writer, result);
    }
    writer.EndObject();
    return std::string(buffer.GetString()) + "\n";
}

std::string makeErrorResponse(const std::string& error)
{
    rapidjson::StringBuffer buffer;
    JsonWriter writer(buffer);
    writer.StartObject();
    writer.Key("ok");
    writer.Bool(false);
    writer.Key("error");
    writeString(writer, error);
    writer.EndObject();
    return std::string(buffer.GetString()) + "\n";
}

bool parseResponse(const std::string& input, Response& response)
{
    rapidjson::Document document;
    document.Parse(input.c_str());
    if (document.HasParseError() || !document.IsObject() ||
        !document.HasMember("ok") || !document["ok"].IsBool()) {
        return false;
    }

    response.isOk = document["ok"].GetBool();
    response.result = stringMember(document, "result");
    response.error = stringMember(document, "error");
    return true;
}

bool ensureDirectories(const std::string& path)
{
    size_t start = !path.empty() && path[0] == '/' ? 1 : 0;
    while (start < path.size()) {
        size_t slash = path.find('/', start);
        std::string directory = path.substr(0, slash);
        if (mkdir(directory.c_str(), 0700) != 0 && errno != EEXIST) {
            return false;
        }
        if (slash == std::string::npos) {
            break;
        }
        start = slash + 1;
    }
    return true;
}

bool writeMessage(int descriptor, const std::string& message)
{
    size_t written = 0;
    while (written < message.size()) {
        ssize_t count = write(descriptor, message.data() + written,
                              message.size() - written);
        if (count < 0 && errno == EINTR) {
            continue;
        }
        if (count <= 0) {
            return false;
        }
        written += static_cast<size_t>(count);
    }
    return true;
}

bool readMessage(int descriptor, std::string& message, size_t maximumSize)
{
    message.clear();
    char character;
    while (message.size() < maximumSize) {
        ssize_t count = read(descriptor, &character, 1);
        if (count < 0 && errno == EINTR) {
            continue;
        }
        if (count <= 0) {
            return false;
        }
        if (character == '\n') {
            return true;
        }
        message += character;
    }
    return false;
}

} // namespace StarfishCLI
