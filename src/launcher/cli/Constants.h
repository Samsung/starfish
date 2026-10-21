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

#ifndef __StarfishCLIConstants__
#define __StarfishCLIConstants__

namespace StarfishCLI {

constexpr const char* kProgramName = LWE_CLI_PROGRAM_NAME;
constexpr const char* kEngineBinaryName = LWE_CLI_ENGINE_BINARY_NAME;
constexpr const char* kSessionDirectory = "/lightweight-web-engine/cli";
// Every session has its own files, named after the session.
constexpr const char* kSocketSuffix = ".sock";
constexpr const char* kStartLockSuffix = ".start.lock";
// Named sessions keep cookies and storage under this directory, inside
// kSessionDirectory. The default session uses the engine's own default.
constexpr const char* kStorageSubdirectory = "/storage";

// The daemon option that carries the storage directory of a named session,
// and the engine option it becomes.
constexpr const char* kDaemonOptionStorage = "--storage";
constexpr const char* kStorageDirOptionPrefix = "--storage-dir=";

constexpr const char* kDefaultSessionName = "default";

constexpr const char* kOptionSession = "--session";
constexpr const char* kOptionJson = "--json";
constexpr const char* kOptionTimeout = "--timeout";
constexpr const char* kOptionDaemon = "--daemon";
constexpr const char* kOptionInteractive = "-i";
constexpr const char* kOptionStdin = "--stdin";
constexpr const char* kOptionBase64Short = "-b";
constexpr const char* kOptionBase64 = "--base64";
// Text output of successful commands.
// open prints the title, then the address on an indented line.
constexpr const char* kOpenedPrefix = "\xE2\x9C\x93 ";
constexpr const char* kOpenedUrlPrefix = "\n  ";
constexpr const char* kDoneText = "\xE2\x9C\x93 Done\n";
constexpr const char* kBrowserClosedText = "\xE2\x9C\x93 Browser closed\n";
constexpr const char* kPageAddressErrorMessage =
    "could not read the page address";
constexpr const char* kInvalidEvaluateResponseMessage =
    "invalid Runtime.evaluate response";
constexpr const char* kMissingArgumentsMessage = "Missing arguments for: ";

// Values of "type" in a JSON parse failure.
constexpr const char* kParseFailureUnknownCommand = "unknown_command";
constexpr const char* kParseFailureMissingArguments = "missing_arguments";
constexpr const char* kParseFailureInvalidValue = "invalid_value";
constexpr const char* kParseFailureInvalidSessionName = "invalid_session_name";

// Value of "code" for a ref whose node is gone.
constexpr const char* kStaleRefCode = "stale_ref";

// The daemon writes one line to its stdout pipe before it starts serving:
// kReadySignal when the session is up, or kStartupErrorPrefix and a reason
// when it gives up. Its stderr goes to /dev/null, so this pipe is the only
// way a failure reason reaches the user.
constexpr const char* kReadySignal = "READY\n";
constexpr const char* kStartupErrorPrefix = "ERROR ";

// How long the daemon keeps retrying the CDP connection to the engine.
constexpr int kEngineConnectBudgetSeconds = 8;

// How long a client waits for the line above. This must stay larger than
// kEngineConnectBudgetSeconds. If the client gave up first it would kill the
// daemon mid-startup, and the reason would be lost.
constexpr int kDaemonReadyTimeoutSeconds = kEngineConnectBudgetSeconds + 4;

} // namespace StarfishCLI

#endif
