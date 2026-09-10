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
constexpr const char* kSocketFilename = "default.sock";
constexpr const char* kStartLockFilename = "start.lock";

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
