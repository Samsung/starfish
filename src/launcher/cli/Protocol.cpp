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

#include <poll.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <climits>
#include <limits>

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
        return std::string(document[name].GetString(),
                           document[name].GetStringLength());
    }

    std::string jsonText(const std::string& value)
    {
        rapidjson::StringBuffer buffer;
        JsonWriter writer(buffer);
        writeString(writer, value);
        return buffer.GetString();
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

std::string makeClickRequest(const std::string& selector)
{
    rapidjson::StringBuffer buffer;
    JsonWriter writer(buffer);
    writer.StartObject();
    writer.Key("command");
    writer.String(kCommandClick);
    writer.Key("selector");
    writeString(writer, selector);
    writer.EndObject();
    return std::string(buffer.GetString()) + "\n";
}

std::string makeFillRequest(const std::string& selector,
                            const std::string& text)
{
    rapidjson::StringBuffer buffer;
    JsonWriter writer(buffer);
    writer.StartObject();
    writer.Key("command");
    writer.String(kCommandFill);
    writer.Key("selector");
    writeString(writer, selector);
    writer.Key("text");
    writeString(writer, text);
    writer.EndObject();
    return std::string(buffer.GetString()) + "\n";
}

std::string makeEvalRequest(const std::string& script)
{
    rapidjson::StringBuffer buffer;
    JsonWriter writer(buffer);
    writer.StartObject();
    writer.Key("command");
    writer.String(kCommandEval);
    writer.Key("script");
    writeString(writer, script);
    writer.EndObject();
    return std::string(buffer.GetString()) + "\n";
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
    request.selector = stringMember(document, "selector");
    request.text = stringMember(document, "text");
    request.script = stringMember(document, "script");
    return !request.command.empty();
}

bool parseElementReference(const std::string& selector, int& elementRef)
{
    if (selector.size() < 3 || selector[0] != '@' || selector[1] != 'e') {
        return false;
    }

    int value = 0;
    for (size_t i = 2; i < selector.size(); i++) {
        char character = selector[i];
        if (character < '0' || character > '9') {
            return false;
        }
        int digit = character - '0';
        if (value > (std::numeric_limits<int>::max() - digit) / 10) {
            return false;
        }
        value = value * 10 + digit;
    }
    if (value < 1) {
        return false;
    }

    elementRef = value;
    return true;
}

std::string makeOkResponse(const std::string& result, const std::string& data)
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
    if (!data.empty()) {
        writer.Key("data");
        writeString(writer, data);
    }
    writer.EndObject();
    return std::string(buffer.GetString()) + "\n";
}

std::string makeErrorResponse(const std::string& error, const std::string& code)
{
    rapidjson::StringBuffer buffer;
    JsonWriter writer(buffer);
    writer.StartObject();
    writer.Key("ok");
    writer.Bool(false);
    writer.Key("error");
    writeString(writer, error);
    if (!code.empty()) {
        writer.Key("code");
        writeString(writer, code);
    }
    writer.EndObject();
    return std::string(buffer.GetString()) + "\n";
}

std::string makeJsonSuccess(const std::string& dataJson)
{
    return R"({"success":true,"data":)" + dataJson +
           R"(,"error":null})"
           "\n";
}

std::string makeJsonFailure(const std::string& error, const std::string& code)
{
    std::string line =
        R"({"success":false,"data":null,"error":)" + jsonText(error);
    if (!code.empty()) {
        line += R"(,"code":)" + jsonText(code);
    }
    return line + "}\n";
}

std::string makeJsonParseFailure(const std::string& error,
                                 const std::string& type)
{
    return R"({"error":)" + jsonText(error) + R"(,"success":false,"type":)" +
           jsonText(type) + "}\n";
}

std::string makeJsonStringObject(const char* key, const std::string& value)
{
    return "{" + jsonText(key) + ":" + jsonText(value) + "}";
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
    response.data = stringMember(document, "data");
    response.error = stringMember(document, "error");
    response.code = stringMember(document, "code");
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

bool readMessage(int descriptor, std::string& message, size_t maximumSize,
                 long long timeoutMs)
{
    message.clear();
    auto deadline = std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(timeoutMs < 0 ? 0 : timeoutMs);
    char character;
    while (message.size() < maximumSize) {
        if (timeoutMs >= 0) {
            // Round up, so the wait never ends before the deadline.
            auto left = (std::chrono::duration_cast<std::chrono::microseconds>(
                             deadline - std::chrono::steady_clock::now())
                             .count() +
                         999) /
                        1000;
            pollfd entry = { descriptor, POLLIN, 0 };
            int ready = poll(&entry, 1,
                             static_cast<int>(std::max<long long>(
                                 0, std::min<long long>(left, INT_MAX))));
            if (ready < 0 && errno == EINTR) {
                continue;
            }
            if (ready <= 0) {
                return false;
            }
        }
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
