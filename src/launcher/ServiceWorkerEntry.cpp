/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_SERVICE_WORKER) && defined(STARFISH_WEBWORKER_HOST)

#include "StarfishConfig.h"
#include "LWEWebView.h"
#include "Starfish.h"
#include "LWEServiceWorker.h"
#include <functional>
#include <cstdio>
#include <signal.h>
#ifdef OS_POSIX
#include <unistd.h>
#endif

#include <string.h>

namespace LWE {
extern Starfish::Starfish* g_starfishInstance;
}

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
    LoggerOption::instance()->parseEnv();

    STARFISH_ASSERT(argv != nullptr);
    STARFISH_LOG_INFO("WORKER STARTS");

    std::string scriptURL;
    std::string dataDir;

    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);

        if (startsWith(arg, std::string("--run-script="))) {
            scriptURL = arg.substr(strlen("--run-script="));
        } else if (startsWith(arg, std::string("--debug-cast="))) {
            setenv("DEBUG_CAST", argv[i] + strlen("--debug-cast="), 1);
        } else if (startsWith(arg, std::string("--data-dir="))) {
            dataDir = arg.substr(strlen("--data-dir="));
        }
    }

    LWE::ServiceWorker::Initialize(dataDir);

    LWE::ServiceWorker::RegisterOnStatusChangedHandler(
        [](LWE::ServiceWorker::State state) {
            if (state == LWE::ServiceWorker::State::Terminated) {
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
        usleep(100);
    }

    LWE::ServiceWorker::Finalize();

    STARFISH_LOG_INFO("WORKER ENDS");
    return 0;
}

#endif
