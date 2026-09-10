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

struct Request {
    std::string command;
    std::string url;
};

struct Response {
    bool isOk{ false };
    std::string result;
    std::string error;
};

std::string makeOpenRequest(const std::string& url);
std::string makeSnapshotRequest();
std::string makeCloseRequest();
bool parseRequest(const std::string& input, Request& request);

std::string makeOkResponse(const std::string& result = std::string());
std::string makeErrorResponse(const std::string& error);
bool parseResponse(const std::string& input, Response& response);

bool ensureDirectories(const std::string& path);
bool writeMessage(int descriptor, const std::string& message);
bool readMessage(int descriptor, std::string& message, size_t maximumSize);

} // namespace StarfishCLI

#endif
