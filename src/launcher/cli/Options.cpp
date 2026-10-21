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

#include "Options.h"

#include "Constants.h"
#include "Protocol.h"

#include <limits>

namespace StarfishCLI {

namespace {

    bool isCommandWord(const std::string& word)
    {
        return word == kCommandOpen || word == kCommandSnapshot ||
               word == kCommandClose || word == kCommandClick ||
               word == kCommandFill || word == kCommandEval;
    }

    // An option value is missing when the arguments end, or when the next
    // argument is another option or the command word. This keeps
    // "--session open <url>" from using "open" as the session name.
    bool hasValue(const std::vector<std::string>& arguments, size_t index)
    {
        if (index + 1 >= arguments.size()) {
            return false;
        }
        const std::string& next = arguments[index + 1];
        return !isCommandWord(next) && next.compare(0, 2, "--") != 0;
    }

} // namespace

ClientOptions::ClientOptions()
    : session(kDefaultSessionName)
{
}

bool isValidSessionName(const std::string& name)
{
    if (name.empty()) {
        return false;
    }
    for (char character : name) {
        bool isAllowed = (character >= 'a' && character <= 'z') ||
                         (character >= 'A' && character <= 'Z') ||
                         (character >= '0' && character <= '9') ||
                         character == '-' || character == '_';
        if (!isAllowed) {
            return false;
        }
    }
    return true;
}

bool parseTimeout(const std::string& text, long long& milliseconds)
{
    if (text.empty()) {
        return false;
    }

    long long value = 0;
    for (char character : text) {
        if (character < '0' || character > '9') {
            return false;
        }
        int digit = character - '0';
        if (value > (std::numeric_limits<long long>::max() - digit) / 10) {
            return false;
        }
        value = value * 10 + digit;
    }
    if (value < 1) {
        return false;
    }

    milliseconds = value;
    return true;
}

bool parseClientOptions(const std::vector<std::string>& arguments,
                        ClientOptions& options, ParseFailure& failure)
{
    options = ClientOptions();
    bool hasFailure = false;
    auto fail = [&](const char* type, const std::string& message) {
        // Report the first problem only.
        if (!hasFailure) {
            hasFailure = true;
            failure.type = type;
            failure.message = message;
        }
    };

    for (size_t index = 0; index < arguments.size(); index++) {
        const std::string& argument = arguments[index];
        if (argument == kOptionJson) {
            options.isJson = true;
        } else if (argument == kOptionSession || argument == kOptionTimeout) {
            if (!hasValue(arguments, index)) {
                fail(kParseFailureMissingArguments,
                     std::string(kMissingArgumentsMessage) + argument);
                continue;
            }

            const std::string& value = arguments[++index];
            if (argument == kOptionSession) {
                if (isValidSessionName(value)) {
                    options.session = value;
                } else {
                    fail(kParseFailureInvalidSessionName,
                         "Invalid session name '" + value +
                             "'. Only alphanumeric characters, hyphens, and "
                             "underscores are allowed.");
                }
            } else if (parseTimeout(value, options.timeoutMs)) {
                options.hasTimeout = true;
            } else {
                fail(kParseFailureInvalidValue,
                     std::string(kOptionTimeout) +
                         " expects a number in ms, got '" + value + "'");
            }
        } else if (options.arguments.empty() && argument.size() > 2 &&
                   argument.compare(0, 2, "--") == 0) {
            // A bare "--" is an ordinary word, not an option.
            // After the command word, option-like words belong to the command.
            fail(kParseFailureUnknownCommand, "Unknown option: " + argument);
        } else {
            options.arguments.push_back(argument);
        }
    }
    return !hasFailure;
}

} // namespace StarfishCLI
