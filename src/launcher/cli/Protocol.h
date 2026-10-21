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

#ifndef __StarfishCLIProtocol__
#define __StarfishCLIProtocol__

#include <cstddef>
#include <string>

namespace StarfishCLI {

constexpr const char* kCommandOpen = "open";
constexpr const char* kCommandSnapshot = "snapshot";
constexpr const char* kCommandClose = "close";
constexpr const char* kCommandClick = "click";
constexpr const char* kCommandFill = "fill";
constexpr const char* kCommandEval = "eval";

struct Request {
    std::string command;
    std::string url;
    std::string selector;
    std::string text;
    std::string script;
};

struct Response {
    bool isOk{ false };
    // Text for the terminal in text mode.
    std::string result;
    // The "data" object for --json, as JSON text. Empty when the command
    // has no data from the daemon.
    std::string data;
    std::string error;
    std::string code;
};

std::string makeOpenRequest(const std::string& url);
std::string makeSnapshotRequest();
std::string makeCloseRequest();
std::string makeClickRequest(const std::string& selector);
std::string makeFillRequest(const std::string& selector,
                            const std::string& text);
std::string makeEvalRequest(const std::string& script);
bool parseRequest(const std::string& input, Request& request);
bool parseElementReference(const std::string& selector, int& elementRef);

std::string makeOkResponse(const std::string& result = std::string(),
                           const std::string& data = std::string());
std::string makeErrorResponse(const std::string& error,
                              const std::string& code = std::string());
bool parseResponse(const std::string& input, Response& response);

// Builds the one line that --json prints. Each ends with a newline.
// dataJson is JSON text. code may be empty.
std::string makeJsonSuccess(const std::string& dataJson);
std::string makeJsonFailure(const std::string& error, const std::string& code);
std::string makeJsonParseFailure(const std::string& error,
                                 const std::string& type);
// Builds {"<key>":"<value>"} with the value escaped.
std::string makeJsonStringObject(const char* key, const std::string& value);

bool ensureDirectories(const std::string& path);
bool writeMessage(int descriptor, const std::string& message);
// Reads one line. A negative timeoutMs waits without limit. Otherwise it is the
// total time for the whole line, and false means the line did not arrive.
bool readMessage(int descriptor, std::string& message, size_t maximumSize,
                 long long timeoutMs = -1);

} // namespace StarfishCLI

#endif
