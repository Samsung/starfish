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
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace StarfishCLI {

namespace {

    // Seventeen significant digits always read back as the same double.
    constexpr int kMaxDoubleDigits = 17;

    std::string jsonString(const std::string& value)
    {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        writer.String(value.c_str(),
                      static_cast<rapidjson::SizeType>(value.size()));
        return buffer.GetString();
    }

    // Format a number the way JSON.stringify does. This follows the
    // Number::toString steps of ECMAScript. It uses the shortest digits that
    // read back as the same double.
    std::string formatNumber(double number)
    {
        if (!std::isfinite(number)) {
            return "null";
        }
        if (number == 0) {
            return "0";
        }

        char scientific[32];
        for (int precision = 1; precision <= kMaxDoubleDigits; precision++) {
            snprintf(scientific, sizeof(scientific), "%.*e", precision - 1,
                     number);
            if (strtod(scientific, nullptr) == number) {
                break;
            }
        }

        // snprintf gives "[-]d.ddde[+-]xx". Split it into digits and exponent.
        const char* exponentText = strchr(scientific, 'e');
        std::string digits;
        for (const char* cursor = scientific; cursor < exponentText; cursor++) {
            if (isdigit(static_cast<unsigned char>(*cursor))) {
                digits.push_back(*cursor);
            }
        }
        while (digits.size() > 1 && digits.back() == '0') {
            digits.pop_back();
        }

        int digitCount = static_cast<int>(digits.size());
        int pointPosition = atoi(exponentText + 1) + 1;
        std::string text = number < 0 ? "-" : "";
        if (digitCount <= pointPosition && pointPosition <= 21) {
            text += digits + std::string(pointPosition - digitCount, '0');
        } else if (0 < pointPosition && pointPosition <= 21) {
            text += digits.substr(0, pointPosition) + "." +
                    digits.substr(pointPosition);
        } else if (-6 < pointPosition && pointPosition <= 0) {
            text += "0." + std::string(-pointPosition, '0') + digits;
        } else {
            text += digits.substr(0, 1);
            if (digitCount > 1) {
                text += "." + digits.substr(1);
            }
            int exponent = pointPosition - 1;
            text += exponent < 0 ? "e-" : "e+";
            text += std::to_string(std::abs(exponent));
        }
        return text;
    }

    // PrettyWriter prints doubles in its own form, such as "1e21" or "42.0".
    // This writer prints them as JSON.stringify does.
    class ValueWriter
        : public rapidjson::PrettyWriter<rapidjson::StringBuffer> {
    public:
        explicit ValueWriter(rapidjson::StringBuffer& buffer)
            : PrettyWriter(buffer)
        {
        }

        bool Double(double number)
        {
            PrettyPrefix(rapidjson::kNumberType);
            std::string text = formatNumber(number);
            for (char character : text) {
                os_->Put(character);
            }
            return true;
        }
    };

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
    m_client.expectEvent("Accessibility.loadComplete");
    if (!command("Page.navigate", parameters, result, error)) {
        return false;
    }

    std::string eventParameters;
    if (!m_client.waitForEvent("Accessibility.loadComplete", eventParameters,
                               30000)) {
        error = "timeout waiting for Accessibility.loadComplete";
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

    return formatInteractiveSnapshot(result, output, error, m_snapshotNodeIds);
}

bool Session::click(int elementRef, std::string& error)
{
    if (elementRef < 1 ||
        static_cast<size_t>(elementRef) > m_snapshotNodeIds.size()) {
        error = "element reference out of range";
        return false;
    }

    int backendNodeId = m_snapshotNodeIds[elementRef - 1];

    // resolveNode: backendNodeId -> RemoteObject objectId.
    std::string resolveParams =
        "{\"backendNodeId\":" + std::to_string(backendNodeId) + "}";
    std::string resolveResult;
    if (!command("DOM.resolveNode", resolveParams, resolveResult, error)) {
        return false;
    }

    rapidjson::Document resolveDoc;
    resolveDoc.Parse(resolveResult.c_str());
    if (resolveDoc.HasParseError() || !resolveDoc.IsObject() ||
        !resolveDoc.HasMember("object") || !resolveDoc["object"].IsObject() ||
        !resolveDoc["object"].HasMember("objectId") ||
        !resolveDoc["object"]["objectId"].IsString()) {
        error = "could not resolve node";
        return false;
    }
    std::string objectId = resolveDoc["object"]["objectId"].GetString();

    // getBoxModel: objectId -> border quad [x1,y1, x2,y2, x3,y3, x4,y4].
    std::string boxParams = "{\"objectId\":" + jsonString(objectId) + "}";
    std::string boxResult;
    if (!command("DOM.getBoxModel", boxParams, boxResult, error)) {
        return false;
    }

    rapidjson::Document boxDoc;
    boxDoc.Parse(boxResult.c_str());
    if (boxDoc.HasParseError() || !boxDoc.IsObject() ||
        !boxDoc.HasMember("model") || !boxDoc["model"].IsObject() ||
        !boxDoc["model"].HasMember("border") ||
        !boxDoc["model"]["border"].IsArray() ||
        boxDoc["model"]["border"].Size() < 8) {
        error = "could not get box model";
        return false;
    }

    const rapidjson::Value& border = boxDoc["model"]["border"];
    for (rapidjson::SizeType i = 0; i < 8; i++) {
        if (!border[i].IsNumber()) {
            error = "could not get box model";
            return false;
        }
    }
    double x1 = border[0].GetDouble();
    double y1 = border[1].GetDouble();
    double x3 = border[4].GetDouble();
    double y3 = border[5].GetDouble();
    double centerX = (x1 + x3) / 2.0;
    double centerY = (y1 + y3) / 2.0;

    // dispatchMouseEvent: mousePressed + mouseReleased at center.
    std::string pressedParams =
        "{\"type\":\"mousePressed\",\"x\":" + std::to_string(centerX) +
        ",\"y\":" + std::to_string(centerY) +
        ",\"button\":\"left\",\"clickCount\":1}";
    std::string releasedParams =
        "{\"type\":\"mouseReleased\",\"x\":" + std::to_string(centerX) +
        ",\"y\":" + std::to_string(centerY) +
        ",\"button\":\"left\",\"clickCount\":1}";

    std::string inputResult;
    if (!command("Input.dispatchMouseEvent", pressedParams, inputResult,
                 error)) {
        return false;
    }
    return command("Input.dispatchMouseEvent", releasedParams, inputResult,
                   error);
}

bool Session::fill(int elementRef, const std::string& text, std::string& error)
{
    if (elementRef < 1 ||
        static_cast<size_t>(elementRef) > m_snapshotNodeIds.size()) {
        error = "element reference out of range";
        return false;
    }

    int backendNodeId = m_snapshotNodeIds[elementRef - 1];

    // resolveNode: backendNodeId -> RemoteObject objectId.
    std::string resolveParams =
        "{\"backendNodeId\":" + std::to_string(backendNodeId) + "}";
    std::string resolveResult;
    if (!command("DOM.resolveNode", resolveParams, resolveResult, error)) {
        return false;
    }

    rapidjson::Document resolveDoc;
    resolveDoc.Parse(resolveResult.c_str());
    if (resolveDoc.HasParseError() || !resolveDoc.IsObject() ||
        !resolveDoc.HasMember("object") || !resolveDoc["object"].IsObject() ||
        !resolveDoc["object"].HasMember("objectId") ||
        !resolveDoc["object"]["objectId"].IsString()) {
        error = "could not resolve node";
        return false;
    }
    std::string objectId = resolveDoc["object"]["objectId"].GetString();

    // focus the input element.
    std::string focusParams = "{\"objectId\":" + jsonString(objectId) + "}";
    std::string focusResult;
    if (!command("DOM.focus", focusParams, focusResult, error)) {
        return false;
    }

    // TODO: Clear through the editing path so input events and contenteditable
    // elements follow the same behavior as typed text.
    std::string clearParams =
        "{\"objectId\":" + jsonString(objectId) +
        ",\"functionDeclaration\":\"function(){this.value='';}\"}";
    std::string clearResult;
    if (!command("Runtime.callFunctionOn", clearParams, clearResult, error)) {
        return false;
    }

    // insertText: dispatch keyDown per character through the input
    // event path.
    std::string insertParams = "{\"text\":" + jsonString(text) + "}";
    std::string insertResult;
    return command("Input.insertText", insertParams, insertResult, error);
}

bool Session::evaluate(const std::string& script, std::string& output,
                       std::string& error)
{
    std::string parameters = "{\"expression\":" + jsonString(script) +
                             ",\"returnByValue\":true,\"awaitPromise\":true}";
    std::string result;
    if (!command("Runtime.evaluate", parameters, result, error)) {
        return false;
    }

    rapidjson::Document document;
    document.Parse<rapidjson::kParseFullPrecisionFlag>(result.c_str());
    if (document.HasParseError() || !document.IsObject()) {
        error = "invalid Runtime.evaluate response";
        return false;
    }

    if (document.HasMember("exceptionDetails") &&
        document["exceptionDetails"].IsObject()) {
        const rapidjson::Value& details = document["exceptionDetails"];
        std::string description;
        if (details.HasMember("exception") && details["exception"].IsObject() &&
            details["exception"].HasMember("description") &&
            details["exception"]["description"].IsString()) {
            description = details["exception"]["description"].GetString();
        } else if (details.HasMember("text") && details["text"].IsString()) {
            description = details["text"].GetString();
        } else {
            description = "JavaScript exception";
        }
        error = "Evaluation error: " + description;
        return false;
    }

    if (!document.HasMember("result") || !document["result"].IsObject()) {
        error = "invalid Runtime.evaluate response";
        return false;
    }

    const rapidjson::Value& remoteObject = document["result"];
    if (!remoteObject.HasMember("value")) {
        output = "null\n";
        return true;
    }

    rapidjson::StringBuffer buffer;
    ValueWriter writer(buffer);
    writer.SetIndent(' ', 2);
    remoteObject["value"].Accept(writer);
    output.assign(buffer.GetString(), buffer.GetSize());
    output += "\n";
    return true;
}

} // namespace StarfishCLI
