/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_SHARED_WORKER) && defined(STARFISH_WEBWORKER_HOST)

#include <cstdio>
#include <thread>
#include <chrono>
#include <signal.h>
#include <string.h>

#include "LWEWorker.h"

static volatile sig_atomic_t g_workerDoneFlag = 0;

static void setDoneFlag(int sig, siginfo_t* siginfo, void* context)
{
    g_workerDoneFlag = 1;
}

static inline bool startsWith(const std::string& string,
                              const std::string& prefix)
{
    return (string.size() >= prefix.size()) &&
           (string.compare(0, prefix.size(), prefix) == 0);
}

int main(int argc, char* argv[])
{
    std::string dataDir;

    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);

        if (startsWith(arg, std::string("--data-dir="))) {
            dataDir = arg.substr(strlen("--data-dir="));
        }
    }

    LWE::SharedWorker::Initialize(dataDir);

    LWE::SharedWorker::RegisterOnStatusChangedHandler(
        [](LWE::WorkerProcessState state) {
            if (state == LWE::WorkerProcessState::Terminated) {
                g_workerDoneFlag = 1;
            }
        });

    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_sigaction = setDoneFlag;
    act.sa_flags = SA_SIGINFO;

    if (sigaction(SIGINT, &act, NULL) < 0) {
        perror("sigaction");
        return 1;
    }

    while (g_workerDoneFlag == 0) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    LWE::SharedWorker::Finalize();

    return 0;
}

#endif
