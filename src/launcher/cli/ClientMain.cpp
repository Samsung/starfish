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
#include "Protocol.h"
#include "SocketPath.h"

#include <fcntl.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace StarfishCLI {

namespace {

    std::string sessionDirectory()
    {
        const char* home = getenv("HOME");
        return std::string(home ? home : "/tmp") + kSessionDirectory;
    }

    std::string socketPath()
    {
        return sessionDirectory() + "/" + kSocketFilename;
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
                  Response& response)
    {
        int descriptor = connectToDaemon(path);
        if (descriptor < 0) {
            return false;
        }

        constexpr size_t maximumResponseSize = 16 * 1024 * 1024;
        std::string line;
        bool didRead = writeMessage(descriptor, request) &&
                       readMessage(descriptor, line, maximumResponseSize);
        close(descriptor);
        return didRead && parseResponse(line, response);
    }

    int lockSessionStart()
    {
        std::string directory = sessionDirectory();
        if (!ensureDirectories(directory)) {
            return -1;
        }

        std::string path = directory + "/" + kStartLockFilename;
        int descriptor =
            open(path.c_str(), O_CREAT | O_RDWR | O_CLOEXEC | O_NOFOLLOW, 0600);
        if (descriptor < 0) {
            return -1;
        }
        while (flock(descriptor, LOCK_EX) != 0) {
            if (errno != EINTR) {
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
    bool startDaemon(const std::string& path, std::string& error)
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
            execl(self, self, "--daemon", "--engine", engine.c_str(),
                  "--socket", path.c_str(), nullptr);
            _exit(127);
        }

        close(readyPipe[1]);
        fd_set descriptors;
        FD_ZERO(&descriptors);
        FD_SET(readyPipe[0], &descriptors);
        timeval timeout = { kDaemonReadyTimeoutSeconds, 0 };
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

    void printUsage(FILE* stream)
    {
        fprintf(stream,
                "Usage: %s <command>\n"
                "\n"
                "Commands:\n"
                "  open <url>    Open a URL\n"
                "  snapshot -i  List interactive elements\n"
                "  close         Close the session\n",
                kProgramName);
    }

    bool makeCommand(int argc, char* argv[], std::string& command,
                     std::string& request)
    {
        if (argc < 2) {
            return false;
        }

        command = argv[1];
        if (command == kCommandOpen && argc == 3) {
            request = makeOpenRequest(argv[2]);
            return true;
        }
        if (command == kCommandSnapshot && argc == 3 &&
            !strcmp(argv[2], "-i")) {
            request = makeSnapshotRequest();
            return true;
        }
        if (command == kCommandClose && argc == 2) {
            request = makeCloseRequest();
            return true;
        }
        return false;
    }

} // namespace

int clientMain(int argc, char* argv[])
{
    if (argc == 2 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h"))) {
        printUsage(stdout);
        return 0;
    }

    std::string command;
    std::string request;
    if (!makeCommand(argc, argv, command, request)) {
        printUsage(stderr);
        return 1;
    }

    std::string path = socketPath();
    Response response;
    std::string startError;
    bool didAnswer = exchange(path, request, response);
    if (!didAnswer && command == kCommandOpen) {
        int startLock = lockSessionStart();
        if (startLock < 0) {
            fprintf(stderr, "%s: could not start the session\n", kProgramName);
            return 1;
        }

        // Another client may have started the session while this one waited
        // for the lock, so try the socket again before starting a daemon.
        didAnswer = exchange(path, request, response);
        if (!didAnswer && startDaemon(path, startError)) {
            didAnswer = exchange(path, request, response);
        }
        close(startLock);
    }

    if (!didAnswer) {
        if (!startError.empty()) {
            fprintf(stderr, "%s: %s\n", kProgramName, startError.c_str());
        } else {
            fprintf(stderr, "%s: no active session\n", kProgramName);
        }
        return 1;
    }
    if (!response.isOk) {
        fprintf(stderr, "%s: %s\n", kProgramName, response.error.c_str());
        return 1;
    }
    if (!response.result.empty()) {
        printf("%s", response.result.c_str());
    }
    return 0;
}

} // namespace StarfishCLI
