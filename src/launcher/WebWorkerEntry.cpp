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
#include "core/modules/worker/host/WebWorker.h"
#include "core/modules/serviceworker/ServiceWorkerAgent.h"
#include <functional>
#include <cstdio>
#include <signal.h>
#ifdef OS_POSIX
#include <unistd.h>
#endif

namespace LWE {
extern Starfish::Starfish* g_starfishInstance;
}

static volatile sig_atomic_t g_workerDoneFlag = 0;

static void setDoneFlag(int sig, siginfo_t* siginfo, void* context)
{
    g_workerDoneFlag = 1;
}

int main(int argc, char* argv[])
{
    if (argc < 3) {
        puts("please specify URL and baseURL");
        return -1;
    }

    LWE::LWE::Initialize("/tmp/Starfish_WebWorkerlocalStorage.txt",
                         "/tmp/Starfish_WebWorkerCookies.txt", "/tmp");

    // Create ServiceWorkerAgent
    Starfish::ServiceWorkerAgent::instance();

    Starfish::WebWorker* webWorker = Starfish::WebWorker::create(
        LWE::g_starfishInstance, "ko-KR", "Asia/Seoul",
        Starfish::String::emptyString);

    webWorker->loadJavaScript(std::string(argv[1]), std::string(argv[2]));

    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_sigaction = setDoneFlag;
    act.sa_flags = SA_SIGINFO;

    if (sigaction(SIGINT, &act, NULL) < 0) {
        perror("sigaction");
        return 1;
    }

    while (!g_workerDoneFlag) {
        usleep(100);
    }

    LWE::LWE::Finalize();

    return 0;
}

#endif
