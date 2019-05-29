/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#ifdef STARFISH_WEBWORKER_HOST

#include "StarfishConfig.h"
#include "LWEWebView.h"
#include "Starfish.h"
#include "core/modules/serviceworker/host/ServiceWorkerExecutor.h"
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
    STARFISH_ASSERT(argv != nullptr);
    STARFISH_LOG_INFO("WORKER STARTS\n");

    std::string scriptURL = "";

    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);

        std::string option("--run-script=");
        if (startsWith(arg, option) == true) {
            scriptURL = arg.substr(option.size());
        }
    }

    LWE::LWE::Initialize("/tmp/Starfish_WebWorkerlocalStorage.txt",
                         "/tmp/Starfish_WebWorkerCookies.txt", "/tmp");

    Starfish::ServiceWorkerExecutor::initialize(LWE::g_starfishInstance);

    Starfish::ServiceWorkerExecutor::registerOnStatusChangedHandler(
        [](Starfish::ServiceWorkerAgentState state) {
            if (state == Starfish::ServiceWorkerAgentState::Terminated) {
                g_workerDoneFlag = 1;
            }
        });

    if (scriptURL.empty() == false) {
        Starfish::ServiceWorkerExecutor::runServiceWorker(scriptURL);
    }

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

    Starfish::ServiceWorkerExecutor::finalize();

    LWE::LWE::Finalize();

    STARFISH_LOG_INFO("WORKER ENDS\n");
    return 0;
}

#endif
