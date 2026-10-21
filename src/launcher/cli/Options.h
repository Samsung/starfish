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

#ifndef __StarfishCLIOptions__
#define __StarfishCLIOptions__

#include <string>
#include <vector>

namespace StarfishCLI {

// Options that every command accepts, before or after the command word.
struct ClientOptions {
    std::string session;
    bool isJson{ false };
    bool hasTimeout{ false };
    long long timeoutMs{ 0 };
    // What is left after the options are removed. The first item is the
    // command word.
    std::vector<std::string> arguments;

    ClientOptions();
};

struct ParseFailure {
    // One of the kParseFailure* values in Protocol.h.
    std::string type;
    std::string message;
};

// Splits the command line arguments, without the program name, into global
// options and command arguments. Returns false and fills failure when an
// option is wrong. Scanning continues after a failure, so options is still
// useful for choosing the output format.
bool parseClientOptions(const std::vector<std::string>& arguments,
                        ClientOptions& options, ParseFailure& failure);

// A session name is not empty and has only ASCII letters, digits, '-' and '_'.
bool isValidSessionName(const std::string& name);

// Accepts a decimal integer of at least 1, with no sign, unit or fraction.
bool parseTimeout(const std::string& text, long long& milliseconds);

} // namespace StarfishCLI

#endif
