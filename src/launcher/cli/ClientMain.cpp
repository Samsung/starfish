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

#include "Constants.h"
#include "Options.h"
#include "Protocol.h"
#include "SocketPath.h"

#include <fcntl.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace StarfishCLI {

namespace {

    // The longest timeout the client counts. It keeps the time math safe.
    constexpr long long kMaximumTimeoutMs = 1000LL * 60 * 60 * 24 * 30;

    constexpr useconds_t kLockPollMicroseconds = 10000;

    constexpr const char* kClosedData =
        R"({"closed":true,"restoreStatus":"not_configured",)"
        R"("saveStatus":"not_attempted"})";
    constexpr const char* kEmptyData = "{}";
    constexpr const char* kKeyClicked = "clicked";
    constexpr const char* kKeyFilled = "filled";

    // The time left for one blocking invocation. An unset deadline never
    // expires.
    class Deadline {
    public:
        Deadline() = default;

        explicit Deadline(long long milliseconds)
            : m_isSet(true)
            , m_end(std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(
                        std::min(milliseconds, kMaximumTimeoutMs)))
        {
        }

        // Negative when there is no limit.
        long long remainingMs() const
        {
            if (!m_isSet) {
                return -1;
            }
            // Round up, so a wait that ends early by less than one
            // millisecond does not look like a wait that has time left.
            auto left = std::chrono::duration_cast<std::chrono::microseconds>(
                            m_end - std::chrono::steady_clock::now())
                            .count();
            return left > 0 ? (left + 999) / 1000 : 0;
        }

        bool hasExpired() const
        {
            return m_isSet && remainingMs() == 0;
        }

    private:
        bool m_isSet{ false };
        std::chrono::steady_clock::time_point m_end;
    };

    std::string sessionDirectory()
    {
        const char* home = getenv("HOME");
        return std::string(home ? home : "/tmp") + kSessionDirectory;
    }

    std::string socketPath(const std::string& session)
    {
        return sessionDirectory() + "/" + session + kSocketSuffix;
    }

    // The default session keeps the engine's own storage directory.
    std::string storageDirectory(const std::string& session)
    {
        if (session == kDefaultSessionName) {
            return std::string();
        }
        return sessionDirectory() + kStorageSubdirectory + "/" + session;
    }

    std::string siblingEngineBinary()
    {
        char executable[4096];
        ssize_t length =
            readlink("/proc/self/exe", executable, sizeof(executable) - 1);
        if (length <= 0) {
            return kEngineBinaryName;
        }
        executable[length] = '\0';

        std::string path(executable);
        size_t slash = path.rfind('/');
        if (slash == std::string::npos) {
            return kEngineBinaryName;
        }
        return path.substr(0, slash + 1) + kEngineBinaryName;
    }

    int connectToDaemon(const std::string& path)
    {
        // connectUnixSocket keeps the path out of sun_path, so $HOME may be
        // of any length. See SocketPath.h.
        return connectUnixSocket(path);
    }

    bool exchange(const std::string& path, const std::string& request,
                  Response& response, const Deadline& deadline)
    {
        int descriptor = connectToDaemon(path);
        if (descriptor < 0) {
            return false;
        }

        constexpr size_t maximumResponseSize = 16 * 1024 * 1024;
        std::string line;
        bool didRead = writeMessage(descriptor, request) &&
                       readMessage(descriptor, line, maximumResponseSize,
                                   deadline.remainingMs());
        close(descriptor);
        return didRead && parseResponse(line, response);
    }

    // Returns the lock descriptor, or -1 on failure. didTimeOut tells whether
    // the deadline passed while waiting.
    int lockSessionStart(const std::string& session, const Deadline& deadline,
                         bool& didTimeOut)
    {
        std::string directory = sessionDirectory();
        if (!ensureDirectories(directory)) {
            return -1;
        }

        std::string path = directory + "/" + session + kStartLockSuffix;
        int descriptor =
            open(path.c_str(), O_CREAT | O_RDWR | O_CLOEXEC | O_NOFOLLOW, 0600);
        if (descriptor < 0) {
            return -1;
        }

        // Without a deadline, flock may block. With one, try and sleep, so the
        // wait ends on time.
        int operation =
            deadline.remainingMs() < 0 ? LOCK_EX : LOCK_EX | LOCK_NB;
        while (flock(descriptor, operation) != 0) {
            if (errno == EWOULDBLOCK) {
                if (deadline.hasExpired()) {
                    didTimeOut = true;
                    close(descriptor);
                    return -1;
                }
                usleep(kLockPollMicroseconds);
            } else if (errno != EINTR) {
                close(descriptor);
                return -1;
            }
        }
        return descriptor;
    }

    void stopDaemon(pid_t process)
    {
        kill(process, SIGTERM);
        while (waitpid(process, nullptr, 0) < 0 && errno == EINTR) {
        }
    }

    // Starts the daemon and waits for it to report. On failure, error holds
    // the reason the daemon sent, or stays empty when it sent nothing.
    bool startDaemon(const std::string& path, const std::string& storage,
                     const Deadline& deadline, std::string& error)
    {
        int readyPipe[2];
        if (pipe(readyPipe) != 0) {
            return false;
        }

        pid_t process = fork();
        if (process < 0) {
            close(readyPipe[0]);
            close(readyPipe[1]);
            return false;
        }
        if (process == 0) {
            close(readyPipe[0]);
            dup2(readyPipe[1], STDOUT_FILENO);
            close(readyPipe[1]);

            int nullDescriptor = open("/dev/null", O_RDWR);
            if (nullDescriptor >= 0) {
                dup2(nullDescriptor, STDIN_FILENO);
                dup2(nullDescriptor, STDERR_FILENO);
                close(nullDescriptor);
            }

            std::string engine = siblingEngineBinary();
            char self[4096];
            ssize_t length = readlink("/proc/self/exe", self, sizeof(self) - 1);
            if (length <= 0) {
                _exit(127);
            }
            self[length] = '\0';
            std::vector<const char*> arguments = { self,       kOptionDaemon,
                                                   "--engine", engine.c_str(),
                                                   "--socket", path.c_str() };
            if (!storage.empty()) {
                arguments.push_back(kDaemonOptionStorage);
                arguments.push_back(storage.c_str());
            }
            arguments.push_back(nullptr);
            execv(self, const_cast<char* const*>(arguments.data()));
            _exit(127);
        }

        close(readyPipe[1]);
        fd_set descriptors;
        FD_ZERO(&descriptors);
        FD_SET(readyPipe[0], &descriptors);
        long long waitMs = kDaemonReadyTimeoutSeconds * 1000LL;
        if (deadline.remainingMs() >= 0) {
            waitMs = std::min(waitMs, deadline.remainingMs());
        }
        timeval timeout = { static_cast<time_t>(waitMs / 1000),
                            static_cast<suseconds_t>(waitMs % 1000 * 1000) };
        bool isReady = false;
        if (select(readyPipe[0] + 1, &descriptors, nullptr, nullptr, &timeout) >
            0) {
            // One line arrives: kReadySignal, or kStartupErrorPrefix and a
            // reason. Nothing arrives when the daemon died before writing.
            char line[512] = {};
            ssize_t length = read(readyPipe[0], line, sizeof(line) - 1);
            size_t prefixLength = strlen(kStartupErrorPrefix);
            if (length == static_cast<ssize_t>(strlen(kReadySignal)) &&
                !memcmp(line, kReadySignal, strlen(kReadySignal))) {
                isReady = true;
            } else if (length > static_cast<ssize_t>(prefixLength) &&
                       !memcmp(line, kStartupErrorPrefix, prefixLength)) {
                error.assign(line + prefixLength,
                             static_cast<size_t>(length) - prefixLength);
                while (!error.empty() && error.back() == '\n') {
                    error.pop_back();
                }
            }
        }
        close(readyPipe[0]);
        if (!isReady) {
            stopDaemon(process);
        }
        return isReady;
    }

    int base64Value(char character)
    {
        if (character >= 'A' && character <= 'Z') {
            return character - 'A';
        }
        if (character >= 'a' && character <= 'z') {
            return character - 'a' + 26;
        }
        if (character >= '0' && character <= '9') {
            return character - '0' + 52;
        }
        if (character == '+') {
            return 62;
        }
        if (character == '/') {
            return 63;
        }
        return -1;
    }

    bool decodeBase64(const std::string& encoded, std::string& decoded)
    {
        if (encoded.size() % 4 != 0) {
            return false;
        }

        decoded.clear();
        decoded.reserve(encoded.size() / 4 * 3);
        for (size_t index = 0; index < encoded.size(); index += 4) {
            int first = base64Value(encoded[index]);
            int second = base64Value(encoded[index + 1]);
            bool thirdPadding = encoded[index + 2] == '=';
            bool fourthPadding = encoded[index + 3] == '=';
            int third = thirdPadding ? 0 : base64Value(encoded[index + 2]);
            int fourth = fourthPadding ? 0 : base64Value(encoded[index + 3]);
            bool isLast = index + 4 == encoded.size();
            if (first < 0 || second < 0 || third < 0 || fourth < 0 ||
                (thirdPadding && !fourthPadding) ||
                ((thirdPadding || fourthPadding) && !isLast)) {
                return false;
            }
            if (thirdPadding && (second & 0x0f) != 0) {
                return false;
            }
            if (fourthPadding && !thirdPadding && (third & 0x03) != 0) {
                return false;
            }

            decoded.push_back(static_cast<char>((first << 2) | (second >> 4)));
            if (!thirdPadding) {
                decoded.push_back(
                    static_cast<char>((second << 4) | (third >> 2)));
            }
            if (!fourthPadding) {
                decoded.push_back(static_cast<char>((third << 6) | fourth));
            }
        }
        return true;
    }

    bool isValidUTF8(const std::string& text)
    {
        const unsigned char* bytes =
            reinterpret_cast<const unsigned char*>(text.data());
        size_t index = 0;
        while (index < text.size()) {
            unsigned char first = bytes[index++];
            if (first <= 0x7f) {
                continue;
            }

            size_t continuationCount = 0;
            unsigned char secondMinimum = 0x80;
            unsigned char secondMaximum = 0xbf;
            if (first >= 0xc2 && first <= 0xdf) {
                continuationCount = 1;
            } else if (first >= 0xe0 && first <= 0xef) {
                continuationCount = 2;
                if (first == 0xe0) {
                    secondMinimum = 0xa0;
                } else if (first == 0xed) {
                    secondMaximum = 0x9f;
                }
            } else if (first >= 0xf0 && first <= 0xf4) {
                continuationCount = 3;
                if (first == 0xf0) {
                    secondMinimum = 0x90;
                } else if (first == 0xf4) {
                    secondMaximum = 0x8f;
                }
            } else {
                return false;
            }

            if (index + continuationCount > text.size() ||
                bytes[index] < secondMinimum || bytes[index] > secondMaximum) {
                return false;
            }
            index++;
            for (size_t count = 1; count < continuationCount;
                 count++, index++) {
                if (bytes[index] < 0x80 || bytes[index] > 0xbf) {
                    return false;
                }
            }
        }
        return true;
    }

    bool readStandardInput(std::string& input)
    {
        char buffer[4096];
        while (size_t count = fread(buffer, 1, sizeof(buffer), stdin)) {
            input.append(buffer, count);
        }
        return !ferror(stdin);
    }

    void printUsage(FILE* stream)
    {
        fprintf(stream, R"(Usage: %s [options] <command>

Commands:
  open <url>                 Open a URL
  snapshot -i                List interactive elements
  click @eN                  Click element by reference
  fill @eN <text>            Replace input value
  eval <js>                  Evaluate JavaScript
  eval -b|--base64 <base64>  Evaluate base64 JavaScript
  eval --stdin               Read JavaScript from standard input
  close                      Close the session

Options, before or after the command:
  --session <name>           Use a named session (default: default)
  --json                     Print one JSON line on standard output
  --timeout <ms>             Fail after this many milliseconds
)",
                kProgramName);
    }

    ParseFailure makeFailure(const char* type, const std::string& message)
    {
        ParseFailure failure;
        failure.type = type;
        failure.message = message;
        return failure;
    }

    ParseFailure missingArguments(const std::string& what)
    {
        return makeFailure(kParseFailureMissingArguments,
                           kMissingArgumentsMessage + what);
    }

    ParseFailure unexpectedArguments(const std::string& what)
    {
        return makeFailure(kParseFailureInvalidValue,
                           "Unexpected arguments for: " + what);
    }

    // arguments[0] is the command word. On failure, failure says why.
    bool makeCommand(const std::vector<std::string>& arguments,
                     std::string& request, ParseFailure& failure)
    {
        if (arguments.empty()) {
            failure = missingArguments("command");
            return false;
        }

        const std::string& command = arguments[0];
        size_t count = arguments.size();
        auto requireCount = [&](size_t minimum, size_t maximum,
                                const std::string& what) {
            if (count < minimum) {
                failure = missingArguments(what);
                return false;
            }
            if (count > maximum) {
                failure = unexpectedArguments(what);
                return false;
            }
            return true;
        };

        if (command == kCommandOpen) {
            if (!requireCount(2, 2, command)) {
                return false;
            }
            request = makeOpenRequest(arguments[1]);
            return true;
        }
        if (command == kCommandSnapshot) {
            if (!requireCount(2, 2, std::string(command) + " -i")) {
                return false;
            }
            if (arguments[1] != kOptionInteractive) {
                failure = unexpectedArguments(command);
                return false;
            }
            request = makeSnapshotRequest();
            return true;
        }
        if (command == kCommandClose) {
            if (!requireCount(1, 1, command)) {
                return false;
            }
            request = makeCloseRequest();
            return true;
        }
        if (command == kCommandClick) {
            if (!requireCount(2, 2, command)) {
                return false;
            }
            request = makeClickRequest(arguments[1]);
            return true;
        }
        if (command == kCommandFill) {
            if (!requireCount(3, count, command)) {
                return false;
            }
            // Join remaining args with spaces so the shell does not need
            // to quote the text.
            std::string text = arguments[2];
            for (size_t i = 3; i < count; i++) {
                text += " ";
                text += arguments[i];
            }
            request = makeFillRequest(arguments[1], text);
            return true;
        }
        if (command == kCommandEval) {
            if (!requireCount(2, count, command)) {
                return false;
            }

            std::string script;
            if (arguments[1] == kOptionStdin) {
                if (!requireCount(2, 2, command)) {
                    return false;
                }
                if (!readStandardInput(script)) {
                    failure = makeFailure(kParseFailureInvalidValue,
                                          "Could not read standard input");
                    return false;
                }
            } else if (arguments[1] == kOptionBase64Short ||
                       arguments[1] == kOptionBase64) {
                if (!requireCount(3, 3, command)) {
                    return false;
                }
                if (!decodeBase64(arguments[2], script)) {
                    failure = makeFailure(kParseFailureInvalidValue,
                                          "Invalid base64 script");
                    return false;
                }
            } else {
                script = arguments[1];
                for (size_t i = 2; i < count; i++) {
                    script += " ";
                    script += arguments[i];
                }
            }
            if (!isValidUTF8(script)) {
                failure = makeFailure(kParseFailureInvalidValue,
                                      "The script is not valid UTF-8");
                return false;
            }
            request = makeEvalRequest(script);
            return true;
        }

        failure = makeFailure(kParseFailureUnknownCommand,
                              "Unknown command: " + command);
        return false;
    }

    // The failures below end the process with exit code 1. In text mode the
    // reason goes to standard error. With --json, one line goes to standard
    // output and standard error stays empty.
    int reportParseFailure(const ClientOptions& options,
                           const ParseFailure& failure, bool showUsage)
    {
        if (options.isJson) {
            printf("%s",
                   makeJsonParseFailure(failure.message, failure.type).c_str());
            return 1;
        }
        fprintf(stderr, "%s: %s\n", kProgramName, failure.message.c_str());
        if (showUsage) {
            printUsage(stderr);
        }
        return 1;
    }

    int reportFailure(const ClientOptions& options, const std::string& message,
                      const std::string& code = std::string())
    {
        if (options.isJson) {
            printf("%s", makeJsonFailure(message, code).c_str());
            return 1;
        }
        fprintf(stderr, "%s: %s\n", kProgramName, message.c_str());
        return 1;
    }

    int reportTimeout(const ClientOptions& options)
    {
        return reportFailure(options, "Timed out after " +
                                          std::to_string(options.timeoutMs) +
                                          "ms");
    }

    std::string successData(const std::string& command,
                            const std::vector<std::string>& arguments,
                            const Response& response)
    {
        if (command == kCommandClick) {
            return makeJsonStringObject(kKeyClicked, arguments[1]);
        }
        if (command == kCommandFill) {
            return makeJsonStringObject(kKeyFilled, arguments[1]);
        }
        if (command == kCommandClose) {
            return kClosedData;
        }
        return response.data.empty() ? kEmptyData : response.data;
    }

} // namespace

int clientMain(int argc, char* argv[])
{
    if (argc == 2 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h"))) {
        printUsage(stdout);
        return 0;
    }

    ClientOptions options;
    ParseFailure failure;
    if (!parseClientOptions(std::vector<std::string>(argv + 1, argv + argc),
                            options, failure)) {
        return reportParseFailure(options, failure, false);
    }

    std::string request;
    if (!makeCommand(options.arguments, request, failure)) {
        return reportParseFailure(options, failure, true);
    }
    const std::string& command = options.arguments[0];

    Deadline deadline;
    if (options.hasTimeout) {
        deadline = Deadline(options.timeoutMs);
    }

    std::string path = socketPath(options.session);
    Response response;
    std::string startError;
    bool didAnswer = exchange(path, request, response, deadline);
    if (!didAnswer && command == kCommandOpen && !deadline.hasExpired()) {
        bool didTimeOut = false;
        int startLock = lockSessionStart(options.session, deadline, didTimeOut);
        if (startLock < 0) {
            if (didTimeOut) {
                return reportTimeout(options);
            }
            return reportFailure(options, "could not start the session");
        }

        // Another client may have started the session while this one waited
        // for the lock, so try the socket again before starting a daemon.
        didAnswer = exchange(path, request, response, deadline);
        if (!didAnswer && !deadline.hasExpired() &&
            startDaemon(path, storageDirectory(options.session), deadline,
                        startError)) {
            didAnswer = exchange(path, request, response, deadline);
        }
        close(startLock);
    }

    if (!didAnswer) {
        if (deadline.hasExpired()) {
            return reportTimeout(options);
        }
        return reportFailure(options, startError.empty() ? "no active session"
                                                         : startError);
    }
    if (!response.isOk) {
        return reportFailure(options, response.error, response.code);
    }
    if (options.isJson) {
        printf("%s", makeJsonSuccess(
                         successData(command, options.arguments, response))
                         .c_str());
        return 0;
    }
    if (!response.result.empty()) {
        printf("%s", response.result.c_str());
    } else if (command == kCommandClick || command == kCommandFill) {
        printf("%s", kDoneText);
    } else if (command == kCommandClose) {
        printf("%s", kBrowserClosedText);
    }
    return 0;
}

} // namespace StarfishCLI
