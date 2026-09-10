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

#include "CDPClient.h"
#include "Constants.h"
#include "Protocol.h"
#include "Session.h"
#include "SocketPath.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>

namespace StarfishCLI {

namespace {

    volatile sig_atomic_t s_shouldStop = 0;

    void handleSignal(int)
    {
        s_shouldStop = 1;
    }

    uint16_t findFreePort()
    {
        int descriptor = socket(AF_INET, SOCK_STREAM, 0);
        if (descriptor < 0) {
            return 0;
        }

        sockaddr_in address = {};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (bind(descriptor, reinterpret_cast<sockaddr*>(&address),
                 sizeof(address)) != 0) {
            close(descriptor);
            return 0;
        }

        socklen_t length = sizeof(address);
        if (getsockname(descriptor, reinterpret_cast<sockaddr*>(&address),
                        &length) != 0) {
            close(descriptor);
            return 0;
        }
        uint16_t port = ntohs(address.sin_port);
        close(descriptor);
        return port;
    }

    void stopEngine(pid_t process)
    {
        if (process <= 0) {
            return;
        }
        kill(process, SIGTERM);
        while (waitpid(process, nullptr, 0) < 0 && errno == EINTR) {
        }
    }

    pid_t startEngine(const std::string& engine, uint16_t port)
    {
        pid_t process = fork();
        if (process != 0) {
            return process;
        }

        pid_t parentProcess = getppid();
        if (prctl(PR_SET_PDEATHSIG, SIGTERM) != 0 ||
            getppid() != parentProcess) {
            _exit(127);
        }

        int nullDescriptor = open("/dev/null", O_RDWR);
        if (nullDescriptor >= 0) {
            dup2(nullDescriptor, STDIN_FILENO);
            dup2(nullDescriptor, STDOUT_FILENO);
            dup2(nullDescriptor, STDERR_FILENO);
            close(nullDescriptor);
        }

        std::string portText = std::to_string(port);
        setenv("STARFISH_ENABLE_CDP", "1", 1);
        setenv("STARFISH_CDP_PORT", portText.c_str(), 1);
        execl(engine.c_str(), engine.c_str(), "about:blank",
              "--disable-console", nullptr);
        _exit(127);
    }

    // Retry until the budget runs out, rather than for a fixed number of
    // tries. A refused connection fails at once, but an engine that started
    // and never answers costs one handshake timeout per try. Counting tries
    // would let that case run for minutes. The client would then kill this
    // process while it was still retrying, and the reason would never reach
    // the user.
    bool connectToEngine(CDPClient& client, pid_t engineProcess, uint16_t port,
                         std::string& error)
    {
        time_t deadline = time(nullptr) + kEngineConnectBudgetSeconds;
        do {
            if (client.connect("127.0.0.1", port, error)) {
                return true;
            }

            // Give up as soon as the engine is gone. A missing or broken
            // binary exits at once, and waiting out the budget would only
            // delay the report.
            int status = 0;
            if (waitpid(engineProcess, &status, WNOHANG) == engineProcess) {
                error = "the engine exited before it served CDP";
                if (WIFEXITED(status)) {
                    error +=
                        ", exit code " + std::to_string(WEXITSTATUS(status));
                }
                return false;
            }
            usleep(100000);
        } while (time(nullptr) < deadline);

        error = "the engine did not answer CDP on port " +
                std::to_string(port) + ": " + error;
        return false;
    }

    // Report why startup failed and exit. The client is reading stdout, which
    // is a pipe it created. stderr is /dev/null, so writing there would drop
    // the reason.
    int failStartup(const std::string& reason)
    {
        writeMessage(STDOUT_FILENO,
                     std::string(kStartupErrorPrefix) + reason + "\n");
        return 1;
    }

    int createServerSocket(const std::string& path)
    {
        std::string directory = path.substr(0, path.rfind('/'));
        if (!ensureDirectories(directory)) {
            return -1;
        }
        // bindUnixSocket keeps the path out of sun_path, so $HOME may be of
        // any length. See SocketPath.h.
        return bindUnixSocket(path, 0600, 4);
    }

    bool readDaemonArguments(int argc, char* argv[], std::string& engine,
                             std::string& socketPath)
    {
        for (int index = 2; index + 1 < argc; index += 2) {
            if (!strcmp(argv[index], "--engine")) {
                engine = argv[index + 1];
            } else if (!strcmp(argv[index], "--socket")) {
                socketPath = argv[index + 1];
            } else {
                return false;
            }
        }
        return !engine.empty() && !socketPath.empty();
    }

} // namespace

int daemonMain(int argc, char* argv[])
{
    std::string engine;
    std::string socketPath;
    if (!readDaemonArguments(argc, argv, engine, socketPath)) {
        return 1;
    }

    uint16_t port = findFreePort();
    if (port == 0) {
        return failStartup("could not reserve a local port for CDP");
    }

    pid_t engineProcess = startEngine(engine, port);
    if (engineProcess < 0) {
        return failStartup("could not start the engine " + engine);
    }

    CDPClient client;
    std::string error;
    if (!connectToEngine(client, engineProcess, port, error)) {
        stopEngine(engineProcess);
        return failStartup(error);
    }

    Session session(client);
    if (!session.setup(error)) {
        client.disconnect();
        stopEngine(engineProcess);
        return failStartup("could not set up the CDP session: " + error);
    }

    int server = createServerSocket(socketPath);
    if (server < 0) {
        client.disconnect();
        stopEngine(engineProcess);
        return failStartup("could not create the socket " + socketPath);
    }

    signal(SIGTERM, handleSignal);
    signal(SIGINT, handleSignal);
    if (!writeMessage(STDOUT_FILENO, kReadySignal)) {
        s_shouldStop = 1;
    }

    while (!s_shouldStop) {
        fd_set descriptors;
        FD_ZERO(&descriptors);
        FD_SET(server, &descriptors);
        timeval timeout = { 1, 0 };
        if (select(server + 1, &descriptors, nullptr, nullptr, &timeout) <= 0) {
            continue;
        }

        int connection = accept(server, nullptr, nullptr);
        if (connection < 0) {
            continue;
        }

        std::string input;
        Request request;
        constexpr size_t maximumRequestSize = 1024 * 1024;
        if (!readMessage(connection, input, maximumRequestSize) ||
            !parseRequest(input, request)) {
            writeMessage(connection, makeErrorResponse("invalid request"));
            close(connection);
            continue;
        }

        if (request.command == kCommandClose) {
            close(server);
            unlink(socketPath.c_str());
            client.disconnect();
            stopEngine(engineProcess);
            writeMessage(connection, makeOkResponse());
            close(connection);
            return 0;
        }

        std::string response;
        if (request.command == kCommandOpen) {
            if (request.url.empty()) {
                response = makeErrorResponse("open requires a URL");
            } else if (session.open(request.url, error)) {
                response = makeOkResponse();
            } else {
                response = makeErrorResponse(error);
            }
        } else if (request.command == kCommandSnapshot) {
            std::string output;
            if (session.snapshotInteractive(output, error)) {
                response = makeOkResponse(output);
            } else {
                response = makeErrorResponse(error);
            }
        } else {
            response = makeErrorResponse("unsupported command");
        }

        writeMessage(connection, response);
        close(connection);
    }

    close(server);
    unlink(socketPath.c_str());
    client.disconnect();
    stopEngine(engineProcess);
    return 0;
}

} // namespace StarfishCLI
